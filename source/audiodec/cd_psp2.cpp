/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <psp2/kernel/threadmgr.h>
#include <psp2/audioout.h>
#include <psp2/io/dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern "C"{
	#include "../quakedef.h"
}
#include "audio_decoder.h"

extern uint8_t is_uma0;

#define BUFSIZE 8192 // Max dimension of audio buffer size
#define NSAMPLES 2048 // Number of samples for output

// Music block struct
struct DecodedMusic{
	uint8_t* audiobuf;
	uint8_t* audiobuf2;
	uint8_t* cur_audiobuf;
	FILE* handle;
	bool isPlaying;
	bool loop;
	volatile bool pauseTrigger;
	volatile bool closeTrigger;
	volatile bool changeVol;
};

// Internal stuffs
DecodedMusic* BGM = NULL;
std::unique_ptr<AudioDecoder> audio_decoder;
SceUID thread, Audio_Mutex, Talk_Mutex;
volatile bool mustExit = false;
float old_vol = 1.0;
bool trackNotation = true;

// Audio thread code
static int bgmThread(unsigned int args, void* arg){
	
	// Initializing audio port
	int ch = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, NSAMPLES, 48000, SCE_AUDIO_OUT_MODE_STEREO);
	sceAudioOutSetConfig(ch, -1, -1, (SceAudioOutMode)-1);
	old_vol = bgmvolume.value;
	int vol = 32767 * bgmvolume.value;
	int vol_stereo[] = {vol, vol};
	sceAudioOutSetVolume(ch, (SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), vol_stereo);
	
	DecodedMusic* mus;
	for (;;){
		
		// Waiting for an audio output request
		sceKernelWaitSema(Audio_Mutex, 1, NULL);
		
		// Fetching track
		mus = BGM;
		
		// Checking if a new track is available
		if (mus == NULL){
			
			//If we enter here, we probably are in the exiting procedure
			if (mustExit){
				sceAudioOutReleasePort(ch);
				mustExit = false;
				sceKernelExitThread(0);
			}
		
		}
		
		// Initializing audio decoder
		audio_decoder = AudioDecoder::Create(mus->handle, "Track");
		audio_decoder->Open(mus->handle);
		audio_decoder->SetLooping(mus->loop);
		audio_decoder->SetFormat(48000, AudioDecoder::Format::S16, 2);
		
		// Initializing audio buffers
		mus->audiobuf = (uint8_t*)malloc(BUFSIZE);
		mus->audiobuf2 = (uint8_t*)malloc(BUFSIZE);
		mus->cur_audiobuf = mus->audiobuf;
		
		// Audio playback loop
		for (;;){
		
			// Check if the music must be paused
			if (mus->pauseTrigger || mustExit){
			
				// Check if the music must be closed
				if (mus->closeTrigger){
					free(mus->audiobuf);
					free(mus->audiobuf2);
					audio_decoder.reset();
					free(mus);
					BGM = NULL;
					if (!mustExit){
						sceKernelSignalSema(Talk_Mutex, 1);
						break;
					}
				}
				
				// Check if the thread must be closed
				if (mustExit){
				
					// Check if the audio stream has already been closed
					if (mus != NULL){
						mus->closeTrigger = true;
						continue;
					}
					
					// Recursively closing all the threads
					sceAudioOutReleasePort(ch);
					mustExit = false;
					sceKernelExitDeleteThread(0);
					
				}
			
				mus->isPlaying = !mus->isPlaying;
				mus->pauseTrigger = false;
			}
			
			// Check if a volume change is required
			if (mus->changeVol){
				old_vol = bgmvolume.value;
				int vol = 32767 * bgmvolume.value;
				int vol_stereo[] = {vol, vol};
				sceAudioOutSetVolume(ch, (SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), vol_stereo);
				mus->changeVol = false;
			}
			
			if (mus->isPlaying){
				
				// Check if audio playback finished
				if ((!mus->loop) && audio_decoder->IsFinished()) mus->isPlaying = false;
				
				// Update audio output
				if (mus->cur_audiobuf == mus->audiobuf) mus->cur_audiobuf = mus->audiobuf2;
				else mus->cur_audiobuf = mus->audiobuf;
				audio_decoder->Decode(mus->cur_audiobuf, BUFSIZE);	
				sceAudioOutOutput(ch, mus->cur_audiobuf);
				
			}
			
		}
		
	}
	
}

void CDAudio_Play(byte track, qboolean looping)
{
	CDAudio_Stop();
	char basename[256];
	char fname[256];
	if (is_uma0) sprintf(basename,"uma0:/data/Hexen II/cdtracks/");
	else sprintf(basename,"ux0:/data/Hexen II/cdtracks/");
	if (trackNotation) sprintf(fname, "%strack%02d", basename, track);
	else{
		switch (track){
			case 2:
				sprintf(fname, "%sCasa1", basename);
				break;
			case 3:
				sprintf(fname, "%sCasa2", basename);
				break;
			case 4:
				sprintf(fname, "%sCasa3", basename);
				break;
			case 5:
				sprintf(fname, "%sCasa4", basename);
				break;
			case 6:
				sprintf(fname, "%sEgyp1", basename);
				break;
			case 7:
				sprintf(fname, "%sEgyp2", basename);
				break;
			case 8:
				sprintf(fname, "%sEgyp3", basename);
				break;
			case 9:
				sprintf(fname, "%sMeso1", basename);
				break;
			case 10:
				sprintf(fname, "%sMeso2", basename);
				break;
			case 11:
				sprintf(fname, "%sMeso3", basename);
				break;
			case 12:
				sprintf(fname, "%sRoma1", basename);
				break;
			case 13:
				sprintf(fname, "%sRoma2", basename);
				break;
			case 14:
				sprintf(fname, "%sRoma3", basename);
				break;
			case 15:
				sprintf(fname, "%sCasb1", basename);
				break;
			case 16:
				sprintf(fname, "%sCasb2", basename);
				break;
			case 17:
				sprintf(fname, "%sCasb3", basename);
				break;
			default:
				sprintf(fname, "%sUnkn1", basename);
				break;
		}
	}
	char fullname[256];
	sprintf(fullname,"%s.mid", fname);
	
	FILE* fd = fopen(fullname, "rb");
	if (fd == NULL){
		sprintf(fullname,"%s.mp3", fname);
		fd = fopen(fullname, "rb");
	}
	if (fd == NULL){
		sprintf(fullname,"%s.ogg", fname);
		fd = fopen(fullname, "rb");
	}
	if (fd == NULL) return;
	DecodedMusic* memblock = (DecodedMusic*)malloc(sizeof(DecodedMusic));
	memblock->handle = fd;
	memblock->pauseTrigger = false;
	memblock->closeTrigger = false;
	memblock->isPlaying = true;
	memblock->loop = looping;
	BGM = memblock;
	sceKernelSignalSema(Audio_Mutex, 1);
}

void CDAudio_Stop(void)
{
	if (BGM != NULL){
		BGM->closeTrigger = true;
		BGM->pauseTrigger = true;
		sceKernelWaitSema(Talk_Mutex, 1, NULL);
	}
}


void CDAudio_Pause(void)
{
	if (BGM != NULL) BGM->pauseTrigger = true;
}


void CDAudio_Resume(void)
{
	if (BGM != NULL) BGM->pauseTrigger = true;
}


void CDAudio_Update(void)
{
	if (BGM != NULL){
		if (old_vol != bgmvolume.value) BGM->changeVol = true;
	}
}


int CDAudio_Init(void)
{

	// Checking what kind of filenames the system should use
	SceIoDirent g_dir;
	SceUID fd;
	if (is_uma0) fd = sceIoDopen("uma0:/data/Hexen II/cdtracks");
	else fd = sceIoDopen("ux0:/data/Hexen II/cdtracks");
	if (fd >= 0){
		while (sceIoDread(fd, &g_dir) > 0) {
			if (strncmp(g_dir.d_name, "Casa1", 5) == 0){
				trackNotation = false;
				break;
			}
		}
		sceIoDclose(fd);
	}
	
	// Creating audio mutex
	Audio_Mutex = sceKernelCreateSema("Audio Mutex", 0, 0, 1, NULL);
	Talk_Mutex = sceKernelCreateSema("Talk Mutex", 0, 0, 1, NULL);
	
	// Creating audio thread
	thread = sceKernelCreateThread("Audio Thread", &bgmThread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(thread, sizeof(thread), &thread);
	
	return 0;
}


void CDAudio_Shutdown(void)
{	
	mustExit = true;
	sceKernelSignalSema(Audio_Mutex, 1);
	sceKernelWaitThreadEnd(thread, NULL, NULL);
	sceKernelDeleteSema(Audio_Mutex);
	sceKernelDeleteSema(Talk_Mutex);
	sceKernelDeleteThread(thread);
}
