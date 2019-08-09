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
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <psp2/io/dirent.h>
#include "errno.h"
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/rtc.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/power.h>
#define u64 uint64_t
#include "sys.h"
#include "quakedef.h"
#include "fnmatch_mod.h"

extern int old_char;
extern int isDanzeff;
extern uint64_t rumble_tick;
qboolean		isDedicated;

uint64_t initialTime = 0;
int hostInitialized = 0;
SceCtrlData pad, oldpad;
uint8_t is_uma0 = 0;
extern int msaa;
extern int scr_width;
extern int scr_height;
extern int cfg_width;
extern int cfg_height;
extern cvar_t vid_vsync;

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             findhandle (void)
{
	int             i;

	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int             pos;
	int             end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;

	return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime (char *path)
{
	FILE    *f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}

void Sys_mkdir (char *path)
{
	sceIoMkdir(path, 0777);
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}

void Sys_Quit (void)
{
	Host_Shutdown();
	sceKernelExitProcess(0);
}

void Sys_Error (char *error, ...)
{
	
	va_list         argptr;

	char buf[256];
	va_start(argptr, error);
	vsnprintf(buf, sizeof(buf), error, argptr);
	va_end(argptr);
	sprintf(buf, "%s\n", buf);
	FILE* f = NULL;
	if (is_uma0) f = fopen("uma0:/data/Hexen II/log.txt", "a+");
	else f = fopen("ux0:/data/Hexen II/log.txt", "a+");
	fwrite(buf, 1, strlen(buf), f);
	fclose(f);
	Sys_Quit();
}

void Sys_Printf (char *fmt, ...)
{
	//if(hostInitialized)
		return;

	/*va_list argptr;
	char buf[256];
	va_start (argptr,fmt);
	vsnprintf (buf, sizeof(buf), fmt,argptr);
	va_end (argptr);
	INFO(buf);*/
	
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

double Sys_FloatTime (void)
{
	u64 ticks;
	sceRtcGetCurrentTick(&ticks);
	return ticks * 0.000001;
}

void SCE_KeyDown(int keys){
	if( keys & SCE_CTRL_SELECT)
		Key_Event(K_SELECT, true);
	if( keys & SCE_CTRL_START)
		Key_Event(K_START, true);
	if( keys & SCE_CTRL_UP)
		Key_Event(K_UPARROW, true);
	if( keys & SCE_CTRL_DOWN)
		Key_Event(K_DOWNARROW, true);
	if( keys & SCE_CTRL_LEFT)
		Key_Event(K_LEFTARROW, true);
	if( keys & SCE_CTRL_RIGHT)
		Key_Event(K_RIGHTARROW, true);
	if( keys & SCE_CTRL_SQUARE)
		Key_Event(K_SQUARE, true);
	if( keys & SCE_CTRL_TRIANGLE)
		Key_Event(K_TRIANGLE, true);
	if( keys & SCE_CTRL_CROSS)
		Key_Event(K_CROSS, true);
	if( keys & SCE_CTRL_CIRCLE)
		Key_Event(K_CIRCLE, true);
	if( keys & SCE_CTRL_LTRIGGER)
		Key_Event(K_LEFTTRIGGER, true);
	if( keys & SCE_CTRL_RTRIGGER)
		Key_Event(K_RIGHTTRIGGER, true);
}

void SCE_KeyUp(int keys, int oldkeys){
	if ((!(keys & SCE_CTRL_SELECT)) && (oldkeys & SCE_CTRL_SELECT))
		Key_Event(K_SELECT, false);
	if ((!(keys & SCE_CTRL_START)) && (oldkeys & SCE_CTRL_START))
		Key_Event(K_START, false);
	if ((!(keys & SCE_CTRL_UP)) && (oldkeys & SCE_CTRL_UP))
		Key_Event(K_UPARROW, false);
	if ((!(keys & SCE_CTRL_DOWN)) && (oldkeys & SCE_CTRL_DOWN))
		Key_Event(K_DOWNARROW, false);
	if ((!(keys & SCE_CTRL_LEFT)) && (oldkeys & SCE_CTRL_LEFT))
		Key_Event(K_LEFTARROW, false);
	if ((!(keys & SCE_CTRL_RIGHT)) && (oldkeys & SCE_CTRL_RIGHT))
		Key_Event(K_RIGHTARROW, false);
	if ((!(keys & SCE_CTRL_SQUARE)) && (oldkeys & SCE_CTRL_SQUARE))
		Key_Event(K_SQUARE, false);
	if ((!(keys & SCE_CTRL_TRIANGLE)) && (oldkeys & SCE_CTRL_TRIANGLE))
		Key_Event(K_TRIANGLE, false);
	if ((!(keys & SCE_CTRL_CROSS)) && (oldkeys & SCE_CTRL_CROSS))
		Key_Event(K_CROSS, false);
	if ((!(keys & SCE_CTRL_CIRCLE)) && (oldkeys & SCE_CTRL_CIRCLE))
		Key_Event(K_CIRCLE, false);
	if ((!(keys & SCE_CTRL_LTRIGGER)) && (oldkeys & SCE_CTRL_LTRIGGER))
		Key_Event(K_LEFTTRIGGER, false);
	if ((!(keys & SCE_CTRL_RTRIGGER)) && (oldkeys & SCE_CTRL_RTRIGGER))
		Key_Event(K_RIGHTTRIGGER, false);
}

void Sys_SendKeyEvents (void)
{
	sceCtrlPeekBufferPositive(0, &pad, 1);
	int kDown = pad.buttons;
	int kUp = oldpad.buttons;
	if(kDown)
		SCE_KeyDown(kDown);
	if(kUp != kDown)
		SCE_KeyUp(kDown, kUp);
		
	// Touchscreen support for game status showing
	SceTouchData touch;
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
	if (touch.reportNum > 0) Key_Event(K_TOUCH, true);
	else Key_Event(K_TOUCH, false);
		
	oldpad = pad;
}

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

/*
=================================================
simplified findfirst/findnext implementation:
Sys_FindFirstFile and Sys_FindNextFile return
filenames only, not a dirent struct. this is
what we presently need in this engine.
=================================================
*/

static SceUID	*finddir;
static struct SceIoDirent	*finddata;
static char		*findpath, *findpattern;

char *Sys_FindFirstFile (char *path, char *pattern)
{
	size_t	tmp_len;

	if (finddir)
		Sys_Error ("Sys_FindFirst without FindClose");

	finddir = sceIoDopen(path);
	if (!finddir)
		return NULL;

	tmp_len = strlen (pattern);
	findpattern = (char*) malloc (tmp_len + 1);
	if (!findpattern)
		return NULL;
	strcpy (findpattern, pattern);
	findpattern[tmp_len] = '\0';
	tmp_len = strlen (path);
	findpath = (char*) malloc (tmp_len + 1);
	if (!findpath)
		return NULL;
	strcpy (findpath, path);
	findpath[tmp_len] = '\0';

	return Sys_FindNextFile();
}

char *Sys_FindNextFile (void)
{
	struct stat	test;

	if (!finddir)
		return NULL;

	do {
		sceIoDread(finddir, finddata);
		if (finddata != NULL)
		{
			if (!fnmatch_mod(findpattern, finddata->d_name, FNM_PATHNAME))
			{
				if ( (stat(va("%s/%s", findpath, finddata->d_name), &test) == 0) && S_ISREG(test.st_mode) )
					return finddata->d_name;
			}
		}
	} while (finddata != NULL);

	return NULL;
}

void Sys_FindClose (void)
{
	if (finddir != NULL)
		sceIoDclose(finddir);
	if (findpath != NULL)
		free (findpath);
	if (findpattern != NULL)
		free (findpattern);
	finddir = NULL;
	findpath = NULL;
	findpattern = NULL;
}

//=============================================================================
int _newlib_heap_size_user = 192 * 1024 * 1024;

int main (int argc, char **argv)
{
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);

	// Checking for uma0 support
	FILE *f = fopen("uma0:/data/Hexen II/data1/pak0.pak", "rb");
	if (f) {
		fclose(f);
		is_uma0 = 1;
	}

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
	
	const float tickRate = 1.0f / sceRtcGetTickResolution();
	
	static quakeparms_t    parms;

	parms.memsize = 20*1024*1024;
	parms.membase = malloc (parms.memsize);
	parms.basedir = ".";

	// Loading resolution and MSAA mode from config files, those are not handled via Host cause Host_Init requires vitaGL to be working
	char res_str[64];
	if (is_uma0) f = fopen("uma0:data/Hexen II/resolution.cfg", "rb");
	else f = fopen("ux0:data/Hexen II/resolution.cfg", "rb");
	if (f != NULL){
		fread(res_str, 1, 64, f);
		fclose(f);
		sscanf(res_str, "%dx%d", &scr_width, &scr_height);
	}
	if (is_uma0) f = fopen("uma0:data/Hexen II/antialiasing.cfg", "rb");
	else f = fopen("ux0:data/Hexen II/antialiasing.cfg", "rb");
	if (f != NULL){
		fread(res_str, 1, 64, f);
		fclose(f);
		sscanf(res_str, "%d", &msaa);
	}
	cfg_width = scr_width;
	cfg_height = scr_height;

	// Initializing vitaGL
	// Initializing vitaGL
	switch (msaa) {
	case 1:
		vglInitExtended(0x1400000, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_2X);
		break;
	case 2:
		vglInitExtended(0x1400000, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_4X);
		break;
	default:
		vglInitExtended(0x1400000, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_NONE);
		break;
	}
	vglUseVram(GL_TRUE);

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;
	
	Host_Init (&parms);
	hostInitialized = 1;
	//Sys_Init();
	
	// Set default PSVITA controls
	Cbuf_AddText ("unbindall\n");
	Cbuf_AddText ("bind CROSS +jump\n"); // Cross
	Cbuf_AddText ("bind SQUARE +attack\n"); // Square
	Cbuf_AddText ("bind CIRCLE +crouch\n"); // Circle
	Cbuf_AddText ("bind TRIANGLE \"impulse 10\"\n"); // Triangle
	Cbuf_AddText ("bind LTRIGGER +speed\n"); // Left Trigger
	Cbuf_AddText ("bind RTRIGGER +attack\n"); // Right Trigger
	Cbuf_AddText ("bind UPARROW +showinfo\n"); // Up
	Cbuf_AddText ("bind DOWNARROW invuse\n"); // Down
	Cbuf_AddText ("bind LEFTARROW invleft\n"); // Left
	Cbuf_AddText ("bind RIGHTARROW invright\n"); // Right
	Cbuf_AddText ("sensitivity 5\n"); // Right Analog Sensitivity
	
	// Loading default config file
	Cbuf_AddText ("exec config.cfg\n");
	
	u64 lastTick;
	sceRtcGetCurrentTick(&lastTick);
		
	vglWaitVblankStart(vid_vsync.value);
	int old_vsync = vid_vsync.value;
	
	while (1)
	{
		// Changing V-Sync setting in realtime
		if (old_vsync != vid_vsync.value) {
			vglWaitVblankStart(vid_vsync.value);
			old_vsync = vid_vsync.value;
		}
		
		// Prevent screen power-off
		sceKernelPowerTick(0);
		
		// Rumble effect managing (PSTV only)
		if (rumble_tick != 0){
			if (sceKernelGetProcessTimeWide() - rumble_tick > 500000) IN_StopRumble(); // 0.5 sec
		}
		
		// Get current frame
		u64 tick;
		sceRtcGetCurrentTick(&tick);
		const unsigned int deltaTick  = tick - lastTick;
		const float   deltaSecond = deltaTick * tickRate;
		Host_Frame(deltaSecond);
		lastTick = tick;
		
	}
	
	free(parms.membase);
	return 0;
}
