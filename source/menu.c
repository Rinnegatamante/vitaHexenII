/*
 * $Header: /H2 Mission Pack/Menu.c 41    3/20/98 2:03p Jmonroe $
 */

#include "quakedef.h"
/*#ifdef _WIN32
#include "winquake.h"
#endif*/

#include <string.h>
#include "net_dgrm.h"

//extern cvar_t	accesspoint;

extern char res_string[256];
extern	float introTime;
extern cvar_t	crosshair;
extern void VID_ChangeRes(float);
extern cvar_t	inverted;
extern cvar_t	pstv_rumble;
extern cvar_t	retrotouch;
extern cvar_t	always_run;
extern cvar_t	show_fps;
extern cvar_t	motioncam;
extern cvar_t	motion_sensitivity;
extern cvar_t	motion_horizontal_sensitivity;
extern cvar_t	motion_vertical_sensitivity;
extern cvar_t	gl_outline;
int cfg_width;
int cfg_height;
extern cvar_t gl_bilinear;
cvar_t m_oldmission = {"m_oldmission","1",true};
cvar_t vid_vsync = {"vid_vsync", "1", true};
int msaa = 0;
extern uint8_t is_uma0;

void SetResolution(int w, int h){
	char res_str[64];
	FILE *f = NULL;
	if (is_uma0) f = fopen("uma0:data/Hexen II/resolution.cfg", "wb");
	else f = fopen("ux0:data/Hexen II/resolution.cfg", "wb");
	sprintf(res_str, "%dx%d", w, h);
	fwrite(res_str, 1, strlen(res_str), f);
	fclose(f);
	cfg_width = w;
	cfg_height = h;
}

void SetAntiAliasing(int m){
	char res_str[64];
	FILE *f = NULL;
	if (is_uma0) f = fopen("uma0:data/Hexen II/antialiasing.cfg", "wb");
	else f = fopen("ux0:data/Hexen II/antialiasing.cfg", "wb");
	sprintf(res_str, "%d", m);
	fwrite(res_str, 1, strlen(res_str), f);
	fclose(f);
}

void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);

enum {m_none, m_main, m_singleplayer, m_load, m_save, m_multiplayer, m_setup, m_net, m_options, m_video, 
		m_keys, m_help, m_quit, m_serialconfig, m_modemconfig, m_lanconfig, m_gameoptions, m_search, m_slist, 
		m_class, m_difficulty, m_mload, m_msave, m_osk} m_state;

void CL_RemoveGIPFiles (char *path);

void M_Menu_Main_f (void);
void M_Menu_SinglePlayer_f (void);
void M_Menu_Load_f (void);
void M_Menu_Save_f (void);
void M_Menu_MultiPlayer_f (void);
void M_Menu_Setup_f (void);
void M_Menu_Net_f (void);
void M_Menu_Options_f (void);
void M_Menu_Keys_f (void);
void M_Menu_Video_f (void);
void M_Menu_Help_f (void);
void M_Menu_Quit_f (void);
void M_Menu_OSK_f (char *input, char *output, int outlen);
void M_Menu_SerialConfig_f (void);
void M_Menu_ModemConfig_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_Search_f (void);
void M_Menu_ServerList_f (void);

void M_Main_Draw (void);
void M_SinglePlayer_Draw (void);
void M_Load_Draw (void);
void M_Save_Draw (void);
void M_MultiPlayer_Draw (void);
void M_Setup_Draw (void);
void M_Net_Draw (void);
void M_Options_Draw (void);
void M_Keys_Draw (void);
void M_Video_Draw (void);
void M_Help_Draw (void);
void M_Quit_Draw (void);
void M_OSK_Draw (void);
void M_SerialConfig_Draw (void);
void M_ModemConfig_Draw (void);
void M_LanConfig_Draw (void);
void M_GameOptions_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);

void M_Main_Key (int key);
void M_SinglePlayer_Key (int key);
void M_Load_Key (int key);
void M_Save_Key (int key);
void M_MultiPlayer_Key (int key);
void M_Setup_Key (int key);
void M_Net_Key (int key);
void M_Options_Key (int key);
void M_Keys_Key (int key);
void M_Video_Key (int key);
void M_Help_Key (int key);
void M_Quit_Key (int key);
void M_OSK_Key (int key);
void M_SerialConfig_Key (int key);
void M_ModemConfig_Key (int key);
void M_LanConfig_Key (int key);
void M_GameOptions_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

int			m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];
char 		osk_buffer[128];

static float TitlePercent = 0;
static float TitleTargetPercent = 1;
static float LogoPercent = 0;
static float LogoTargetPercent = 1;

int		setup_class;

static char *message,*message2;
static double message_time;

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame	(m_multiplayer_cursor == 0)
#define SerialConfig	(m_net_cursor == 0)
#define DirectConfig	(m_net_cursor == 1)
#define	IPXConfig	(m_net_cursor == 2)
#define	TCPIPConfig	(m_net_cursor == 3)
#define NUM_DIFFLEVELS	4

void M_ConfigureNetSubsystem(void);
void M_Menu_Class_f (void);

void Con_SetOSKActive(qboolean active);

extern qboolean introPlaying;

extern float introTime;

#ifndef NO_PRAVEUS
char *ClassNames[NUM_CLASSES] = 
{
	"Paladin",
	"Crusader",
	"Necromancer",
	"Assassin",
	"Demoness"
};

char *ClassNamesU[NUM_CLASSES] = 
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"DEMONESS"
};

char *DiffNames[NUM_CLASSES][4] =
{
	{	// Paladin
		"APPRENTICE",
		"SQUIRE",
		"ADEPT",
		"LORD"
	},
	{	// Crusader
		"GALLANT",
		"HOLY AVENGER",
		"DIVINE HERO",
		"LEGEND"
	},
	{	// Necromancer
		"SORCERER",
		"DARK SERVANT",
		"WARLOCK",
		"LICH KING"
	},
	{	// Assassin
		"ROGUE",
		"CUTTHROAT",
		"EXECUTIONER",
		"WIDOW MAKER"
	},
	{	// Demoness
		"LARVA",
		"SPAWN",
		"FIEND",
		"SHE BITCH"
	}
};
#else
char *ClassNames[NUM_CLASSES] = 
{
	"Paladin",
	"Crusader",
	"Necromancer",
	"Assassin"
};

char *ClassNamesU[NUM_CLASSES] = 
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN"
};

char *DiffNames[NUM_CLASSES][4] =
{
	{	// Paladin
		"APPRENTICE",
		"SQUIRE",
		"ADEPT",
		"LORD"
	},
	{	// Crusader
		"GALLANT",
		"HOLY AVENGER",
		"DIVINE HERO",
		"LEGEND"
	},
	{	// Necromancer
		"SORCERER",
		"DARK SERVANT",
		"WARLOCK",
		"LICH KING"
	},
	{	// Assassin
		"ROGUE",
		"CUTTHROAT",
		"EXECUTIONER",
		"WIDOW MAKER"
	}
};

#endif

//=============================================================================
/* Support Routines */
void M_DrawCheckbox (int x, int y, int on)
{
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}
/*
================
M_DrawCharacter

Draws one solid graphics character, centered, on line
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line, num);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

/*
================
M_DrawCharacter2

Draws one solid graphics character, centered H and V
================
*/
void M_DrawCharacter2 (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line + ((vid.height - 200)>>1), num);
}

void M_Print2 (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter2 (cx, cy, ((unsigned char)(*str))+256);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (unsigned char)*str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawTransPic2 (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y + ((vid.height - 200)>>1), pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x + ((vid.width - 320)>>1), y, pic);
}

void M_DrawTransPicCropped (int x, int y, qpic_t *pic)
{
	Draw_TransPicCropped (x + ((vid.width - 320)>>1), y, pic);
}

byte identityTable[256];
byte translationTable[256];
extern int color_offsets[NUM_CLASSES];
extern byte *playerTranslation;
extern int setup_class;

void M_BuildTranslationTable(int top, int bottom)
{
	int		j;
	byte	*dest, *source, *sourceA, *sourceB, *colorA, *colorB;

	for (j = 0; j < 256; j++)
		identityTable[j] = j;
	dest = translationTable;
	source = identityTable;
	memcpy (dest, source, 256);

	if (top > 10) top = 0;
	if (bottom > 10) bottom = 0;

	top -= 1;
	bottom -= 1;

	colorA = playerTranslation + 256 + color_offsets[(int)setup_class-1];
	colorB = colorA + 256;
	sourceA = colorB + 256 + (top * 256);
	sourceB = colorB + 256 + (bottom * 256);
	for(j=0;j<256;j++,colorA++,colorB++,sourceA++,sourceB++)
	{
		if (top >= 0 && (*colorA != 255)) 
			dest[j] = source[*sourceA];
		if (bottom >= 0 && (*colorB != 255)) 
			dest[j] = source[*sourceB];
	}

}


void M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
{
	Draw_TransPicTranslate (x + ((vid.width - 320)>>1), y, pic, translationTable);
}


void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p,*tm,*bm;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	tm = Draw_CachePic ("gfx/box_tm.lmp");
	bm = Draw_CachePic ("gfx/box_bm.lmp");
	while (width > 0)
	{
		cy = y;
		M_DrawTransPic (cx, cy, tm);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		M_DrawTransPic (cx, cy+8, bm);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}


void M_DrawTextBox2 (int x, int y, int width, int lines, qboolean bottom)
{
	qpic_t	*p,*tm,*bm;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy, p);
	else
		M_DrawTransPic2 (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		if(bottom)
			M_DrawTransPic (cx, cy, p);
		else
			M_DrawTransPic2 (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy+8, p);
	else
		M_DrawTransPic2 (cx, cy+8, p);

	// draw middle
	cx += 8;
	tm = Draw_CachePic ("gfx/box_tm.lmp");
	bm = Draw_CachePic ("gfx/box_bm.lmp");
	while (width > 0)
	{
		cy = y;
		
		if(bottom)
			M_DrawTransPic (cx, cy, tm);
		else
			M_DrawTransPic2 (cx, cy, tm);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			if(bottom)
				M_DrawTransPic (cx, cy, p);
			else
				M_DrawTransPic2 (cx, cy, p);
		}
		if(bottom)
			M_DrawTransPic (cx, cy+8, bm);
		else
			M_DrawTransPic2 (cx, cy+8, bm);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy, p);
	else
		M_DrawTransPic2 (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		if(bottom)
			M_DrawTransPic (cx, cy, p);
		else
			M_DrawTransPic2 (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	if(bottom)
		M_DrawTransPic (cx, cy+8, p);
	else
		M_DrawTransPic2 (cx, cy+8, p);
}

//=============================================================================

int m_save_demonum;
		
/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			LogoTargetPercent = TitleTargetPercent = 1;
			LogoPercent = TitlePercent = 0;
			M_Menu_Main_f ();
			return;
		}
		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		LogoTargetPercent = TitleTargetPercent = 1;
		LogoPercent = TitlePercent = 0;
		M_Menu_Main_f ();
	}
}

char BigCharWidth[27][27];
static char unused_filler;  // cuz the COM_LoadStackFile puts a 0 at the end of the data

void M_BuildBigCharWidth (void)
{
#ifdef BUILD_BIG_CHAR
	qpic_t	*p;
	int ypos,xpos;
	byte			*source;
	int biggestX,adjustment;
	char After[20], Before[20];
	int numA,numB;
	FILE *FH;
	char temp[MAX_OSPATH];

	p = Draw_CachePic ("gfx/menu/bigfont.lmp");

	for(numA = 0; numA < 27; numA++)
	{
		memset(After,20,sizeof(After));
		source = p->data + ((numA % 8) * 20) + (numA / 8 * p->width * 20);
		biggestX = 0;

		for(ypos=0;ypos < 19;ypos++)
		{
			for(xpos=0;xpos<19;xpos++,source++)
			{
				if (*source) 
				{
					After[ypos] = xpos;
					if (xpos > biggestX) biggestX = xpos;
				}
			}
			source += (p->width - 19);
		}
		biggestX++;

		for(numB = 0; numB < 27; numB++)
		{
			memset(Before,0,sizeof(Before));
			source = p->data + ((numB % 8) * 20) + (numB / 8 * p->width * 20);
			adjustment = 0;

			for(ypos=0;ypos < 19;ypos++)
			{
				for(xpos=0;xpos<19;xpos++,source++)
				{
					if (!(*source))
					{
						Before[ypos]++;
					}
					else break;
				}
				source += (p->width - xpos);
			}


			while(1)
			{
				for(ypos=0;ypos<19;ypos++)
				{
					if (After[ypos] - Before[ypos] >= 15) break;
					Before[ypos]--;
				}
				if (ypos < 19) break;
				adjustment--;
			}
			BigCharWidth[numA][numB] = adjustment+biggestX;
		}
	}

	sprintf(temp,"%s\\gfx\\menu\\fontsize.lmp",com_gamedir);
	FH = fopen(temp,"wb");
	fwrite(BigCharWidth,1,sizeof(BigCharWidth),FH);
	fclose(FH);
#else
	COM_LoadStackFile ("gfx/menu/fontsize.lmp",BigCharWidth,sizeof(BigCharWidth)+1);
#endif
}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/

#ifndef	GLQUAKE

int M_DrawBigCharacter (int x, int y, int num, int numNext)
{
	qpic_t	*p;
	int ypos,xpos;
	byte			*dest;
	byte			*source;
	int				add;

	if (num == ' ') return 32;

	if (num == '/') num = 26;
	else num -= 65;

	if (num < 0 || num >= 27)  // only a-z and /
		return 0;

	if (numNext == '/') numNext = 26;
	else numNext -= 65;

	p = Draw_CachePic ("gfx/menu/bigfont.lmp");
	source = p->data + ((num % 8) * 20) + (num / 8 * p->width * 20);

    for(ypos=0;ypos < 19;ypos++)
	{
		dest = vid.buffer + (y+ypos) * vid.rowbytes + x;
		for(xpos=0;xpos<19;xpos++,dest++,source++)
		{
			if (*source) 
			{
				*dest = *source;
			}
		}
		source += (p->width - 19);
	}

	if (numNext < 0 || numNext >= 27) return 0;

	add = 0;
	if (num == (int)'C'-65 && numNext == (int)'P'-65)
		add = 3;

	return BigCharWidth[num][numNext] + add;
}
#endif

void M_DrawBigString(int x, int y, char *string)
{
	int c,length;

	x += ((vid.width - 320)>>1);

	length = strlen(string);
	for(c=0;c<length;c++)
	{
		x += M_DrawBigCharacter(x,y,string[c],string[c+1]);
	}
}






void ScrollTitle (char *name)
{
	float delta;
	qpic_t	*p;
	static char *LastName = "";
	int finaly;
	static qboolean CanSwitch = true;

	if (TitlePercent < TitleTargetPercent)
	{
		delta = ((TitleTargetPercent-TitlePercent)/0.5)*host_frametime;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		TitlePercent += delta;
		if (TitlePercent > TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
		}
	}
	else if (TitlePercent > TitleTargetPercent)
	{
		delta = ((TitlePercent-TitleTargetPercent)/0.15)*host_frametime;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		TitlePercent -= delta;
		if (TitlePercent <= TitleTargetPercent)
		{
			TitlePercent = TitleTargetPercent;
			CanSwitch = true;
		}
	}

	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent-LogoPercent)/.15)*host_frametime;
		if (delta < 0.02)
		{
			delta = 0.02;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	if (strcasecmp(LastName,name) != 0 && TitleTargetPercent != 0) 
		TitleTargetPercent = 0;

    if (CanSwitch) 
	{
		LastName = name;
		CanSwitch = false;
		TitleTargetPercent = 1;
	}

	p = Draw_CachePic(LastName);
	finaly = ((float)p->height * TitlePercent) - p->height;
	M_DrawTransPicCropped( (320-p->width)/2, finaly , p);

	if (m_state != m_keys)
	{
		p = Draw_CachePic("gfx/menu/hplaque.lmp");
		finaly = ((float)p->height * LogoPercent) - p->height;
		M_DrawTransPicCropped(10, finaly, p);
	}		
}

//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
#define	MAIN_ITEMS	5


void M_Menu_Main_f (void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;
}
				

void M_Main_Draw (void)
{
	int		f;

	ScrollTitle("gfx/menu/title0.lmp");
	M_DrawBigString (72,60+(0*20),"SINGLE PLAYER");
	M_DrawBigString (72,60+(1*20),"MULTIPLAYER");
	M_DrawBigString (72,60+(2*20),"OPTIONS");
	M_DrawBigString (72,60+(3*20),"HELP");
	M_DrawBigString (72,60+(4*20),"QUIT");


	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_main_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
	
	M_Print (-40, 300, "Huge thanks for their awesome support on Patreon to:");
	M_Print (-40, 308, "- RaveHeart");
	M_Print (-40, 316, "- nobodywasishere");
	M_Print (-40, 324, "- Tain Sueiras");
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo ();
		break;
		
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_CROSS:
	case K_CIRCLE:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_SinglePlayer_f ();
			break;

		case 1:
			M_Menu_MultiPlayer_f ();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Help_f ();
			break;

		case 4:
			M_Menu_Quit_f ();
			break;
		}
	}
}

char	*plaquemessage = NULL;   // Pointer to current plaque message
char    *errormessage = NULL;

//=============================================================================
/* DIFFICULTY MENU */
void M_Menu_Difficulty_f (void)
{
	key_dest = key_menu;
	m_state = m_difficulty;
}

int	m_diff_cursor;
int m_enter_portals;
#define	DIFF_ITEMS	NUM_DIFFLEVELS

void M_Difficulty_Draw (void)
{
	int		f, i;

	ScrollTitle("gfx/menu/title5.lmp");

	setup_class = cl_playerclass.value;

	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
	setup_class--;
	
	for(i = 0; i < NUM_DIFFLEVELS; ++i)
		M_DrawBigString (72,60+(i*20),DiffNames[setup_class][i]);

	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_diff_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}

void M_Difficulty_Key (int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_Class_f ();
		break;
		
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_diff_cursor >= DIFF_ITEMS)
			m_diff_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_diff_cursor < 0)
			m_diff_cursor = DIFF_ITEMS - 1;
		break;
	case K_CROSS:
	case K_CIRCLE:
		Cvar_SetValue ("skill", m_diff_cursor);
		m_entersound = true;
		// Con_ToggleConsole_f();
		if (m_enter_portals)
		{
			introTime = 0.0;
			cl.intermission = 12;
			cl.completed_time = cl.time;
			key_dest = key_game;
			m_state = m_none;
			cls.demonum = m_save_demonum;
		}
		else
			Cbuf_AddText ("map demo1\n");
		break;
	default:
		key_dest = key_game;
		m_state = m_none;
		break;
	}
}

//=============================================================================
/* CLASS CHOICE MENU */
int class_flag;

void M_Menu_Class_f (void)
{
	class_flag=0;
	key_dest = key_menu;
	m_state = m_class;
}

void M_Menu_Class2_f (void)
{
	key_dest = key_menu;
	m_state = m_class;
	class_flag=1;
}


int	m_class_cursor;
#define	CLASS_ITEMS	NUM_CLASSES

void M_Class_Draw (void)
{
	int		f, i;

	ScrollTitle("gfx/menu/title2.lmp");

	for(i = 0; i < NUM_CLASSES; ++i)
		M_DrawBigString (72,60+(i*20),ClassNamesU[i]);

	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_class_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );

	M_DrawPic (251,54 + 21, Draw_CachePic (va("gfx/cport%d.lmp", m_class_cursor + 1)));
	M_DrawTransPic (242,54, Draw_CachePic ("gfx/menu/frame.lmp"));
}

void M_Class_Key (int key)
{
	switch (key)
	{
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_SinglePlayer_f ();
		break;
		
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_class_cursor >= CLASS_ITEMS)
			m_class_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_class_cursor < 0)
			m_class_cursor = CLASS_ITEMS - 1;
		break;

	case K_CROSS:
	case K_CIRCLE:
//		sv_player->v.playerclass=m_class_cursor+1;
		Cbuf_AddText ( va ("playerclass %d\n", m_class_cursor+1) );
		m_entersound = true;
		if (!class_flag)
		{		
			M_Menu_Difficulty_f();
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}
		break;
	default:
		key_dest = key_game;
		m_state = m_none;
		break;
	}

}


//=============================================================================
/* SINGLE PLAYER MENU */

int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	5


void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
	Cvar_SetValue ("timelimit", 0);		//put this here to help play single after dm
}
				
void M_SinglePlayer_Draw (void)
{
	int		f;
	

	ScrollTitle("gfx/menu/title1.lmp");
	
#ifndef NO_PRAVEUS
	M_DrawBigString (72,60+(0*20),"NEW MISSION");
#else 
	M_DrawBigString (72,60+(0*20),"NEW GAME");
#endif
	M_DrawBigString (72,60+(1*20),"LOAD");
	M_DrawBigString (72,60+(2*20),"SAVE");

#ifndef NO_PRAVEUS
	if (m_oldmission.value)
		M_DrawBigString (72,60+(3*20),"OLD MISSION");
#endif
	
	M_DrawBigString (72,60+(4*20),"VIEW DEMO");
	
	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 54 + m_singleplayer_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_Main_f ();
		break;
		
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		if (!m_oldmission.value)
		{
			if (m_singleplayer_cursor ==3)
				m_singleplayer_cursor =4;
		}
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		if (!m_oldmission.value)
		{
			if (m_singleplayer_cursor ==3)
				m_singleplayer_cursor =2;
		}		break;

	case K_CROSS:
	case K_CIRCLE:
		m_entersound = true;

		m_enter_portals = 0;
		switch (m_singleplayer_cursor)
		{
		case 0:
#ifndef NO_PRAVEUS
			m_enter_portals = 1;
#endif			
		case 3:
			// if (sv.active)
			//	if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
			//		break;
			key_dest = key_game;
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			CL_RemoveGIPFiles(NULL);
			Cbuf_AddText ("maxplayers 1\n");
			M_Menu_Class_f ();
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;

		case 4:
			key_dest = key_game;
			Cbuf_AddText("playdemo demo1\n");
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "%s/s%i/info.dat", com_gamedir, i);
		f = fopen (name, "r");
		if (!f)
			continue;
		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], name, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		fclose (f);			
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves ();
}
				

void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	
	ScrollTitle("gfx/menu/load.lmp");
	
	for (i=0 ; i< MAX_SAVEGAMES; i++)
		M_Print (16, 60 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 60 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int		i;

	ScrollTitle("gfx/menu/save.lmp");

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print (16, 60 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 60 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Load_Key (int k)
{	
	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_CROSS:
	case K_CIRCLE:
		S_LocalSound ("raven/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;
	
		// Host_Loadgame_f can't bring up the loading plaque because too much
		// stack space has been used, so do it now
	//	SCR_BeginLoadingPlaque ();
	
		// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_CROSS:
	case K_CIRCLE:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;	
	}
}








//=============================================================================
/* MULTIPLAYER LOAD/SAVE MENU */

void M_ScanMSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "%s/ms%i/info.dat", com_gamedir, i);
		f = fopen (name, "r");
		if (!f)
			continue;
		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], name, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
		fclose (f);			
	}
}

void M_Menu_MLoad_f (void)
{
	m_entersound = true;
	m_state = m_mload;
	key_dest = key_menu;
	M_ScanMSaves ();
}
				

void M_Menu_MSave_f (void)
{
	if (!sv.active || cl.intermission || svs.maxclients == 1)
	{
		message = "Only a network server";
		message2 = "can save a multiplayer game";
		message_time = host_time;
		return;
	}
	m_entersound = true;
	m_state = m_msave;
	key_dest = key_menu;
	M_ScanMSaves ();
}


void M_MLoad_Key (int k)
{	
	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_CROSS:
	case K_CIRCLE:
		S_LocalSound ("raven/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

		if (sv.active)
			Cbuf_AddText ("disconnect\n");
		Cbuf_AddText ("listen 1\n");	// so host_netport will be re-examined

		// Host_Loadgame_f can't bring up the loading plaque because too much
		// stack space has been used, so do it now
	//	SCR_BeginLoadingPlaque ();
		
		// issue the load command
		Cbuf_AddText (va ("load ms%i\n", load_cursor) );
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_MSave_Key (int k)
{
	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_CROSS:
	case K_CIRCLE:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va("save ms%i\n", load_cursor));
		return;
	
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;	
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	5

void M_Menu_MultiPlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;

	message = NULL;
}
				

void M_MultiPlayer_Draw (void)
{
	int		f;
	

	ScrollTitle("gfx/menu/title4.lmp");

	M_Print (72,60+(0*20),"JOIN A GAME");
	M_Print (72,60+(1*20),"NEW GAME");
	M_Print (72,60+(2*20),"SETUP");

	f = (int)(host_time * 10)%8;
	// M_DrawTransPic (43, 54 + m_multiplayer_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
	
	if (message)
	{
		M_PrintWhite ((320/2) - ((27*8)/2), 168, message);
		M_PrintWhite ((320/2) - ((27*8)/2), 176, message2);
		if (host_time - 5 > message_time)
			message = NULL;
	}
	//M_Print (72, 60+(3*20),  "Adhoc          ");
	//M_DrawCheckbox (220, 60+(3*20), adhocAvailable);
	// M_DrawCheckbox (220, 60+(3*20), adhocAvailable);
	
	M_Print (72, 60+(4*20), "Infrastructure ");
	M_DrawCheckbox (220, 60+(4*20), tcpipAvailable );

	M_DrawCharacter (60, 60 + m_multiplayer_cursor*20, 12+((int)(realtime*4)&1));

	//if (serialAvailable || ipxAvailable || tcpipAvailable || adhocAvailable)
	//	return;
	M_PrintWhite ((320/2) - ((27*8)/2), 160, "No Communications Available");
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_Main_f ();
		break;
		
	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_CROSS:
	case K_CIRCLE:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			//if (serialAvailable || ipxAvailable || tcpipAvailable || adhocAvailable)
			//	M_Menu_Net_f ();
			break;

		case 1:
			//if (serialAvailable || ipxAvailable || tcpipAvailable || adhocAvailable)
			//	M_Menu_Net_f ();
			break;

		case 2:
			M_Menu_Setup_f ();
			break;

		case 3:
			//Datagram_Shutdown();
			
			//adhocAvailable = !adhocAvailable;
			//tcpipAvailable = 0;

			//if(adhocAvailable)
			//	Datagram_Init();

			break;

		case 4:
			Datagram_Shutdown();
			
			tcpipAvailable = !tcpipAvailable;
			//adhocAvailable = 0;

			if(tcpipAvailable)
				Datagram_Init();

			break;

		}
	}
}

//=============================================================================
/* SETUP MENU */

int		setup_cursor = 4;
int		setup_cursor_table[] = {40, 56, 72, 88, 120, 136, 156};

char    setup_accesspoint[64];
char	setup_hostname[16];
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;

#define	NUM_SETUP_CMDS	7

void M_Menu_Setup_f (void)
{
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	strcpy(setup_myname, cl_name.string);
	strcpy(setup_hostname, hostname.string);
	setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
	setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
	setup_class = cl_playerclass.value;
	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
}
				

void M_Setup_Draw (void)
{
	qpic_t	*p;

	ScrollTitle("gfx/menu/title4.lmp");
	
	M_Print (64, 40, "Access Point");
	M_DrawTextBox (160, 32, 16, 1);
	M_Print (168, 40, setup_accesspoint);

	M_Print (64, 56, "Hostname");
	M_DrawTextBox (160, 48, 16, 1);
	M_Print (168, 56, setup_hostname);

	M_Print (64, 72, "Your name");
	M_DrawTextBox (160, 64, 16, 1);
	M_Print (168, 72, setup_myname);

	M_Print (64, 88, "Current Class: ");
	M_Print (88, 104, ClassNames[setup_class-1]);

	M_Print (64, 120, "First color patch");
	M_Print (64, 136, "Second color patch");
	
	M_DrawTextBox (64, 156-8, 14, 1);
	M_Print (72, 156, "Accept Changes");

	p = Draw_CachePic (va("gfx/menu/netp%i.lmp",setup_class));
	M_BuildTranslationTable(setup_top, setup_bottom);

	/* garymct */
	M_DrawTransPicTranslate (220, 72, p);

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

	if (setup_cursor == 1)
		M_DrawCharacter (168 + 8*strlen(setup_hostname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));

	if (setup_cursor == 2)
		M_DrawCharacter (168 + 8*strlen(setup_myname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
}


void M_Setup_Key (int k)
{
	int			l;

	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
		if (setup_cursor == 0)
		{
			S_LocalSound ("misc/menu3.wav");
			/*if(accesspoint.value > 1)
			{
				Cvar_SetValue("accesspoint", accesspoint.value-1);		
			}*/
		}
		if (setup_cursor < 3)
			return;
		S_LocalSound ("raven/menu3.wav");
		if (setup_cursor == 3)
		{
			setup_class--;
			if (setup_class < 1) 
				setup_class = NUM_CLASSES;

		}
		if (setup_cursor == 4)
			setup_top = setup_top - 1;
		if (setup_cursor == 5)
			setup_bottom = setup_bottom - 1;
		break;
	case K_RIGHTARROW:
		if (setup_cursor == 0)
		{
			S_LocalSound ("misc/menu3.wav");
		}
		if (setup_cursor < 3)
			return;
forward:
		S_LocalSound ("raven/menu3.wav");
		if (setup_cursor == 3)
		{
			setup_class++;
			if (setup_class > NUM_CLASSES) 
				setup_class = 1;

		}
		if (setup_cursor == 4)
			setup_top = setup_top + 1;
		if (setup_cursor == 5)
			setup_bottom = setup_bottom + 1;
		break;
	case K_INS:
		if (setup_cursor == 1)
		{
			M_Menu_OSK_f(setup_hostname, setup_hostname, 16);
			break;
		}

		if (setup_cursor == 2)
		{
			M_Menu_OSK_f(setup_myname, setup_myname,16);
			break;
		}
		break;
	case K_CROSS:
	case K_CIRCLE:
		if (setup_cursor < 6)
			break;
		if (setup_cursor == 2 || setup_cursor == 3 || setup_cursor == 4)
			goto forward;
		if (strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (strcmp(hostname.string, setup_hostname) != 0)
			Cvar_Set("hostname", setup_hostname);
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText( va ("color %i %i\n", setup_top, setup_bottom) );
		Cbuf_AddText ( va ("playerclass %d\n", setup_class) );
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;
	
	case K_BACKSPACE:
		if (setup_cursor == 1)
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname)-1] = 0;
		}

		if (setup_cursor == 2)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
		break;
		
	default:
		if (k < 32 || k > 127)
			break;
		if (setup_cursor == 1)
		{
			l = strlen(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l+1] = 0;
				setup_hostname[l] = k;
			}
		}
		if (setup_cursor == 2)
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l+1] = 0;
				setup_myname[l] = k;
			}
		}
	}

	if (setup_top > 10)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 10;
	if (setup_bottom > 10)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 10;
}

//=============================================================================
/* NET MENU */

int	m_net_cursor = 0;
int m_net_items;
int m_net_saveHeight;

char *net_helpMessage [] = 
{
/* .........1.........2.... */
  "                        ",
  " Two computers connected",
  "   through two modems.  ",
  "                        ",
 
  "                        ",
  " Two computers connected",
  " by a null-modem cable. ",
  "                        ",

  " Novell network LANs    ",
  " or Windows 95 DOS-box. ",
  "                        ",
  "(LAN=Local Area Network)",

  " Commonly used to play  ",
  " over the Internet, but ",
  " also used on a Local   ",
  " Area Network.          "
};

void M_Menu_Net_f (void)
{
	key_dest = key_menu;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 4;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;
	qpic_t	*p;

	ScrollTitle("gfx/menu/title4.lmp");

	f = 32;

	f += 19;
	f += 19;
	M_DrawBigString (72,f,"IPX");

	f += 19;
	M_DrawBigString (72,f,"TCP/IP");

	f = (320-26*8)/2;
	M_DrawTextBox (f, 134, 24, 4);
	f += 8;
	M_Print (f, 142, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 150, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 158, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 166, net_helpMessage[m_net_cursor*4+3]);

	f = (int)(host_time * 10)%8;
	M_DrawTransPic (43, 24 + m_net_cursor * 20,Draw_CachePic( va("gfx/menu/menudot%i.lmp", f+1 ) ) );
}

void M_Net_Key (int k)
{
again:
	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_MultiPlayer_f ();
		break;
		
	case K_DOWNARROW:
// Tries to re-draw the menu here, and m_net_cursor could be set to -1
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_CROSS:
	case K_CIRCLE:
		m_entersound = true;

		switch (m_net_cursor)
		{
		case 0:
			M_Menu_SerialConfig_f ();
			break;

		case 1:
			M_Menu_SerialConfig_f ();
			break;

		case 2:
			M_Menu_LanConfig_f ();
			break;

		case 3:
			M_Menu_LanConfig_f ();
			break;

		case 4:
// multiprotocol
			break;
		}
	}

	if (m_net_cursor == 0 && !serialAvailable)
		goto again;
	if (m_net_cursor == 1 && !serialAvailable)
		goto again;
	if (m_net_cursor == 2 && !ipxAvailable)
		goto again;
	if (m_net_cursor == 3 && !(tcpipAvailable))
		goto again;
	switch (k)
	{
		case K_DOWNARROW:
		case K_UPARROW:
			S_LocalSound ("raven/menu1.wav");
			break;
	}
}

//=============================================================================
/* OPTIONS MENU */

#define	SLIDER_RANGE	10
#define OPTIONS_NUM 24

int		options_cursor;

void M_Menu_Options_f (void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;

	//if ((options_cursor == OPT_USEMOUSE))
		options_cursor = 0;
}

int w_res[] = {480, 640, 720, 960};
int h_res[] = {272, 368, 408, 544};
int r_idx = -1;

void M_AdjustSliders (int dir)
{
	S_LocalSound ("raven/menu3.wav");

	switch (options_cursor)
	{
	case 3:	// screen size
		scr_viewsize.value += dir * 10;
		if (scr_viewsize.value < 30)
			scr_viewsize.value = 30;
		if (scr_viewsize.value > 120)
			scr_viewsize.value = 120;
		Cvar_SetValue ("viewsize", scr_viewsize.value);
		SB_ViewSizeChanged();
		vid.recalc_refdef = 1;
		break;
	case 4:	// gamma
		v_gamma.value -= dir * 0.05;
		if (v_gamma.value < 0.5)
			v_gamma.value = 0.5;
		if (v_gamma.value > 1)
			v_gamma.value = 1;
		Cvar_SetValue ("gamma", v_gamma.value);
		break;
	case 5:	// mouse speed
		sensitivity.value += dir * 0.5;
		if (sensitivity.value < 1)
			sensitivity.value = 1;
		if (sensitivity.value > 11)
			sensitivity.value = 11;
		Cvar_SetValue ("sensitivity", sensitivity.value);
		break;
	
	case 6:	// music volume
		bgmvolume.value += dir * 0.1;
		if (bgmvolume.value < 0)
			bgmvolume.value = 0;
		if (bgmvolume.value > 1)
			bgmvolume.value = 1;
		Cvar_SetValue ("bgmvolume", bgmvolume.value);
		break;
	
	case 7:	// sfx volume
		volume.value += dir * 0.1;
		if (volume.value < 0)
			volume.value = 0;
		if (volume.value > 1)
			volume.value = 1;
		Cvar_SetValue ("volume", volume.value);
		break;
		
	case 8:	// always run
		Cvar_SetValue ("always_run", !always_run.value);
		break;
	
	case 9:	// invert camera
		Cvar_SetValue ("invert_camera", !inverted.value);
		break;
	
	case 10: // retrotouch
		Cvar_SetValue ("retrotouch", !retrotouch.value);
		break;
	
	case 11: // motion camera
		Cvar_SetValue ("motioncam", !motioncam.value);
		break;
	
	case 12:	// motion camera sensitivity horizontal
		motion_horizontal_sensitivity.value += dir * 0.5;
		if (motion_horizontal_sensitivity.value < 0)
			motion_horizontal_sensitivity.value = 0;
		if (motion_horizontal_sensitivity.value > 10)
			motion_horizontal_sensitivity.value = 10;
		Cvar_SetValue ("motion_horizontal_sensitivity", motion_horizontal_sensitivity.value);
		break;

	case 13:	// motion camera sensitivity vertical
		motion_vertical_sensitivity.value += dir * 0.5;
		if (motion_vertical_sensitivity.value < 0)
			motion_vertical_sensitivity.value = 0;
		if (motion_vertical_sensitivity.value > 10)
			motion_vertical_sensitivity.value = 10;
		Cvar_SetValue ("motion_vertical_sensitivity", motion_vertical_sensitivity.value);
		break;
	
	case 14: // crosshair
		Cvar_SetValue ("crosshair", !crosshair.value);
		break;

	case 15: // rumble effect
		Cvar_SetValue ("pstv_rumble", !pstv_rumble.value);
		break;
		
	case 16: // show fps
		Cvar_SetValue ("show_fps", !show_fps.value);
		break;
		
	case 17: // specular mode
		Cvar_SetValue ("gl_xflip", !gl_xflip.value);
		break;
	
	case 18: // bilinear filtering
		Cvar_SetValue ("gl_bilinear", !gl_bilinear.value);
		if (gl_bilinear.value) Cbuf_AddText("gl_texturemode GL_LINEAR\n");
		else Cbuf_AddText("gl_texturemode GL_NEAREST\n");
		break;
		
	case 19: // msaa
		msaa += dir;
		if (msaa < 0) msaa = 2;
		else if (msaa > 2) msaa = 0;
		SetAntiAliasing(msaa);
		break;
		
	case 20: // resolution
		if (r_idx == -1) {
			for (r_idx = 0; r_idx < 4; r_idx++) {
				if (cfg_width == w_res[r_idx]) break;
			}
		}
		r_idx += dir;
		if (r_idx > 3) r_idx = 0;
		else if (r_idx < 0) r_idx = 3;
		SetResolution(w_res[r_idx], h_res[r_idx]);
		break;
	
	case 21: // v-sync
		Cvar_SetValue ("vid_vsync", !vid_vsync.value);
		break;
	
	case 22: // dynamic shadows
		Cvar_SetValue ("r_shadows", !r_shadows.value);
		break;
		
	case 23: // cel shading
		gl_outline.value += dir;
		if (gl_outline.value > 6) gl_outline.value = 6;
		else if (gl_outline.value < 0) gl_outline.value = 0;
		Cvar_SetValue ("gl_outline",gl_outline.value);
		break;
		
	}
}


void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 256);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 257);
	M_DrawCharacter (x+i*8, y, 258);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 259);
}



void M_Options_Draw (void)
{
	float		r;
	
	ScrollTitle("gfx/menu/title3.lmp");
	
	M_Print (16, 60+(0*8), "    Customize controls");
	M_Print (16, 60+(1*8), "         Go to console");
	M_Print (16, 60+(2*8), "     Reset to defaults");

	M_Print (16, 60+(3*8), "           Screen size");
	r = (scr_viewsize.value - 30) / (120 - 30);
	M_DrawSlider (220, 60+(3*8), r);

	M_Print (16, 60+(4*8), "            Brightness");
	r = (1.0 - v_gamma.value) / 0.5;
	M_DrawSlider (220, 60+(4*8), r);

	M_Print (16, 60+(5*8), "    Camera Sensitivity");
	r = (sensitivity.value - 1)/10;
	M_DrawSlider (220, 60+(5*8), r);
	
	M_Print (16, 60+(6*8), "          Music Volume");
	r = bgmvolume.value;
	M_DrawSlider (220, 60+(6*8), r);

	M_Print (16, 60+(7*8), "          Sound Volume");
	r = volume.value;
	M_DrawSlider (220, 60+(7*8), r);

	M_Print (16, 60+(8*8),	"            Always Run");
	M_DrawCheckbox (220, 60+(8*8), always_run.value);

	M_Print (16, 60+(9*8),  "         Invert Camera");
	M_DrawCheckbox (220, 60+(9*8), inverted.value);

	M_Print (16, 60+(10*8), "        Use Retrotouch");
	M_DrawCheckbox (220, 60+(10*8), retrotouch.value);
	
	M_Print (16, 60+(11*8), "         Use Gyroscope");
	M_DrawCheckbox (220, 60+(11*8), motioncam.value);
	
	M_Print (16, 60+(12*8), "    Gyro X Sensitivity");
	r = motion_horizontal_sensitivity.value/10;
	M_DrawSlider (220, 60+(12*8), r);
	
	M_Print (16, 60+(13*8), "    Gyro Y Sensitivity");
	r = motion_vertical_sensitivity.value/10;
	M_DrawSlider (220, 60+(13*8), r);
	
	M_Print (16, 60+(14*8),	"        Show Crosshair");
	M_DrawCheckbox (220, 60+(14*8), crosshair.value);

	M_Print (16, 60+(15*8), "         Rumble Effect");
	M_DrawCheckbox (220, 60+(15*8), pstv_rumble.value);
	
	M_Print (16, 60+(16*8), "        Show Framerate");
	M_DrawCheckbox (220, 60+(16*8), show_fps.value);
	
	M_Print (16, 60+(17*8), "         Specular Mode");
	M_DrawCheckbox (220, 60+(17*8), gl_xflip.value);
	
	M_Print (16, 60+(18*8), "    Bilinear Filtering");
	M_DrawCheckbox (220, 60+(18*8), gl_bilinear.value);
	
	M_Print (16, 60+(19*8), "         Anti-Aliasing");
	if (msaa == 0) M_Print (220, 60+(19*8), "Disabled");
	else if (msaa == 1) M_Print (220, 60+(19*8), "MSAA 2x");
	else M_Print (220, 60+(19*8), "MSAA 4x");
	
	char res_str[64];
	sprintf(res_str, "%dx%d", cfg_width, cfg_height);
	M_Print (16, 60+(20*8), "            Resolution");
	M_Print (220, 60+(20*8), res_str);
	
	M_Print (16, 60+(21*8), "                V-Sync");
	M_DrawCheckbox (220, 60+(21*8), vid_vsync.value);

	M_Print (16, 60+(22*8), "       Dynamic Shadows");
	M_DrawCheckbox (220, 60+(22*8), r_shadows.value);
	
	M_Print (16, 60+(23*8), "           Cel Shading");
	r = gl_outline.value / 6;
	M_DrawSlider (220, 60+(23*8), r);

// cursor
	M_DrawCharacter (200, 60 + options_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Options_Key (int k)
{
	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_Main_f ();
		break;
		
	case K_CROSS:
	case K_CIRCLE:
		m_entersound = true;
		switch (options_cursor)
		{
		case 0:
			M_Menu_Keys_f ();
			break;
		case 1: // Dummy
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		case 2:
			Cbuf_AddText ("exec default.cfg\n");
			
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

			Cbuf_AddText ("gl_texturemode GL_LINEAR\n");
			scr_viewsize.value = 120;
			v_gamma.value = 1;
			sensitivity.value = 5;
			inverted.value = 0;
			bgmvolume.value = 1.0;
			volume.value = 0.7;
			retrotouch.value = 0;
			pstv_rumble.value = 1.0;
			show_fps.value = 0;
			crosshair.value = 1;
			r_shadows.value = 0;
			gl_xflip.value = 0;
			vid_vsync.value = 1;
			gl_bilinear.value = 1;
			motioncam.value = 0;
			motion_horizontal_sensitivity.value = 3;
			motion_vertical_sensitivity.value = 3;
			Cvar_SetValue ("viewsize", scr_viewsize.value);
			Cvar_SetValue ("v_gamma", v_gamma.value);
			Cvar_SetValue ("sensitivity", sensitivity.value);
			Cvar_SetValue ("invert_camera", inverted.value);
			Cvar_SetValue ("bgmvolume", bgmvolume.value);
			Cvar_SetValue ("volume", volume.value);
			Cvar_SetValue ("retrotouch", retrotouch.value);
			Cvar_SetValue ("pstv_rumble", pstv_rumble.value);
			Cvar_SetValue ("show_fps", show_fps.value);
			Cvar_SetValue ("crosshair", crosshair.value);
			Cvar_SetValue ("r_shadows", r_shadows.value);
			Cvar_SetValue ("gl_xflip", gl_xflip.value);
			Cvar_SetValue ("vid_vsync", vid_vsync.value);
			Cvar_SetValue ("gl_bilinear", gl_bilinear.value);
			Cvar_SetValue ("motioncam", motioncam.value);
			Cvar_SetValue ("motion_horizontal_sensitivity", motion_horizontal_sensitivity.value);
			Cvar_SetValue ("motion_vertical_sensitivity", motion_vertical_sensitivity.value);
			SetResolution(960, 544);
			SetAntiAliasing(msaa);
			r_idx = -1;
			msaa = 0;
			break;
		default:
			M_AdjustSliders (1);
			break;
		}
		return;
	
	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
			options_cursor = OPTIONS_NUM - 1;
		else if (options_cursor == 2)
			options_cursor = 0;
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_NUM)
			options_cursor = 0;
		else if (options_cursor == 0)
			options_cursor = 2;
		break;	

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}

	/*if (options_cursor == OPT_USEMOUSE)
	{
		if (k == K_UPARROW)
			options_cursor = OPT_ALWAYSMLOOK;
		else
			options_cursor = 0;
	}*/
}

//=============================================================================
/* KEYS MENU */

char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"impulse 10", 		"change weapon"},
{"+jump", 			"jump / swim up"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+crouch",			"crouch"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
#ifdef PSP
{"+mlook", 			"analog nub look"},
#else
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
#endif
{"+moveup",			"swim up"},
{"+movedown",		"swim down"},
{"impulse 13", 		"lift object"},
{"invuse",			"use inv item"},
{"impulse 44",		"drop inv item"},
{"+showinfo",		"full inventory"},
{"+showdm",			"info / frags"},
{"toggle_dm",		"toggle frags"},
{"+infoplaque",		"objectives"},
{"invleft",			"inv move left"},
{"invright",		"inv move right"},
{"impulse 100",		"inv:torch"},
{"impulse 101",		"inv:qrtz flask"},
{"impulse 102",		"inv:mystic urn"},
{"impulse 103",		"inv:krater"},
{"impulse 104",		"inv:chaos devc"},
{"impulse 105",		"inv:tome power"},
{"impulse 106",		"inv:summon stn"},
{"impulse 107",		"inv:invisiblty"},
{"impulse 108",		"inv:glyph"},
{"impulse 109",		"inv:boots"},
{"impulse 110",		"inv:repulsion"},
{"impulse 111",		"inv:bo peep"},
{"impulse 112",		"inv:flight"},
{"impulse 113",		"inv:force cube"},
{"impulse 114",		"inv:icon defn"}
};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

#define KEYS_SIZE 14

int		keys_cursor;
int		bind_grab;
int		keys_top = 0;

void M_Menu_Keys_f (void)
{
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l,l2;
	char	*b;

	twokeys[0] = twokeys[1] = -1; 
	l = strlen(command); 
	count = 0;
	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j]; 
		if (!b)
			continue; 
		if (!strncmp (b, command, l))
		{
			l2= strlen(b); 
			if (l == l2)
			{		
				twokeys[count] = j; 
				count++; 
				if (count == 2)
					break;
			}		
		}
	}
}

void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	char	*name;
	int		x, y;
	

	ScrollTitle("gfx/menu/title6.lmp");
#ifdef PSP
	if (bind_grab)
		M_Print (12, 64, "Press a button for this action");
	else
		M_Print (18, 64, "Press CROSS to change, SQUARE to clear");
#else
	if (bind_grab)
		M_Print (12, 64, "Press a key or button for this action");
	else
		M_Print (18, 64, "Enter to change, backspace to clear");
#endif		
	if (keys_top) 
		M_DrawCharacter (6, 80, 128);
	if (keys_top + KEYS_SIZE < NUMCOMMANDS)
		M_DrawCharacter (6, 80 + ((KEYS_SIZE-1)*8), 129);

// search for known bindings
	for (i=0 ; i<KEYS_SIZE ; i++)
	{
		y = 80 + 8*i;

		M_Print (16, y, bindnames[i+keys_top][1]);

		l = strlen (bindnames[i+keys_top][0]);
		
		M_FindKeysForCommand (bindnames[i+keys_top][0], keys);
		
		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = strlen(name) * 8;
			if (keys[1] != -1)
			{
				M_Print (140 + x + 8, y, "or");
				M_Print (140 + x + 32, y, Key_KeynumToString (keys[1]));
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter (130, 80 + (keys_cursor-keys_top)*8, '=');
	else
		M_DrawCharacter (130, 80 + (keys_cursor-keys_top)*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[2];
	
	if (bind_grab)
	{	// defining a key
		S_LocalSound ("raven/menu1.wav");
		if (k == K_ENTER)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor][0]);			
			Cbuf_InsertText (cmd);
		}
		
		bind_grab = false;
		return;
	}
	
	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_CROSS:
	case K_CIRCLE:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		S_LocalSound ("raven/menu2.wav");
		if (keys[1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_LocalSound ("raven/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}

	if (keys_cursor < keys_top) 
		keys_top = keys_cursor;
	else if (keys_cursor >= keys_top+KEYS_SIZE)
		keys_top = keys_cursor - KEYS_SIZE + 1;
}

//=============================================================================
/* VIDEO MENU */

void M_Menu_Video_f (void)
{
	key_dest = key_menu;
	m_state = m_video;
	m_entersound = true;
}


void M_Video_Draw (void)
{
	(*vid_menudrawfn) ();
}


void M_Video_Key (int key)
{
	(*vid_menukeyfn) (key);
}

//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	5


void M_Menu_Help_f (void)
{
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}



void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic ( va("gfx/menu/help%02i.lmp", help_page+1)) );
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_Main_f ();
		break;
		
	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		m_quit_prevstate;
qboolean	wasInMenus;

#ifndef	_WIN32
char *quitMessage [] = 
{
/* .........1.........2.... */
  "   Look! Behind you!    ",
  "  There's a big nasty   ",
  "   thing - shoot it!    ",
  "                        ",
 
  "  You can't go now, I   ",
  "   was just getting     ",
  "    warmed up.          ",
  "                        ",

  "    One more game.      ",
  "      C'mon...          ",
  "   Who's gonna know?    ",
  "                        ",

  "   What's the matter?   ",
  "   Palms too sweaty to  ",
  "     keep playing?      ",
  "                        ",
 
  "  Watch your local store",
  "      for Hexen 2       ",
  "    plush toys and      ",
  "    greeting cards!     ",
 
  "  Hexen 2...            ",
  "                        ",
  "    Too much is never   ",
  "        enough.         ",
 
  "  Sure go ahead and     ",
  "  leave.  But I know    ",
  "  you'll be back.       ",
  "                        ",
 
  "                        ",
  "  Insert cute phrase    ",
  "        here            ",
  "                        "
};
#endif

static float LinePos;
static int LineTimes;
static int MaxLines;
char **LineText;
static qboolean SoundPlayed;


#define MAX_LINES 146
char *CreditText[MAX_LINES] =
{
   "vitaHexenII version 2.2",
   "  PSVITA port by Rinnegatamante ",
   "      based on vitaQuake and    ",
   "  original Hexen II source code ",
   "",
   "",
   "",
   "Project Director: James Monroe",
   "Creative Director: Brian Raffel",
   "Project Coordinator: Kevin Schilder",
   "",
   "Lead Programmer: James Monroe",
   "",
   "Programming:",
   "   Mike Gummelt",
   "   Josh Weier",
   "",
   "Additional Programming:",
   "   Josh Heitzman",
   "   Nathan Albury",
   "   Rick Johnson",
   "",
   "Assembly Consultant:",
   "   Mr. John Scott",
   "",
   "Lead Design: Jon Zuk",
   "",
   "Design:",
   "   Tom Odell",
   "   Jeremy Statz",
   "   Mike Renner",
   "   Eric Biessman",
   "   Kenn Hoekstra",
   "   Matt Pinkston",
   "   Bobby Duncanson",
   "   Brian Raffel",
   "",
   "Art Director: Les Dorscheid",
   "",
   "Art:",
   "   Kim Lathrop",
   "   Gina Garren",
   "   Joe Koberstein",
   "   Kevin Long",
   "   Jeff Butler",
   "   Scott Rice",
   "   John Payne",
   "   Steve Raffel",
   "",
   "Animation:",
   "   Eric Turman",
   "   Chaos (Mike Werckle)",
   "",
   "Music:",
   "   Kevin Schilder",
   "",
   "Sound:",
   "   Chia Chin Lee",
   "",
   "Activision",
   "",
   "Producer:",
   "   Steve Stringer",
   "",
   "Marketing Product Manager:",
   "   Henk Hartong",
   "",
   "Marketing Associate:",
   "   Kevin Kraff",
   "",
   "Senior Quality",
   "Assurance Lead:",
   "   Tim Vanlaw",
   "",
   "Quality Assurance Lead:",
   "   Doug Jacobs",
   "",
   "Quality Assurance Team:",
   "   Steve Rosenthal, Steve Elwell,",
   "   Chad Bordwell, David Baker,",
   "   Aaron Casillas, Damien Fischer,",
   "   Winnie Lee, Igor Krinitskiy,",
   "   Samantha Lee, John Park",
   "   Ian Stevens, Chris Toft",
   "",
   "Production Testers:",
   "   Steve Rosenthal and",
   "   Chad Bordwell",
   "",
   "Additional QA and Support:",
   "    Tony Villalobos",
   "    Jason Sullivan",
   "",
   "Installer by:",
   "   Steve Stringer, Adam Goldberg,",
   "   Tanya Martino, Eric Schmidt,",
   "   Ronnie Lane",
   "",
   "Art Assistance by:",
   "   Carey Chico and Franz Boehm",
   "",
   "BizDev Babe:",
   "   Jamie Bafus",
   "",
   "And...",
   "",
   "Our Big Toe:",
   "   Mitch Lasky",
   "",
   "",
   "Special Thanks to:",
   "  Id software",
   "  The original Hexen2 crew",
   "   We couldn't have done it",
   "   without you guys!",
   "",
   "",
   "Published by Id Software, Inc.",
   "Distributed by Activision, Inc.",
   "",
   "The Id Software Technology used",
   "under license in Hexen II (tm)",
   "(c) 1996, 1997 Id Software, Inc.",
   "All Rights Reserved.",
   "",
   "Hexen(r) is a registered trademark",
   "of Raven Software Corp.",
   "Hexen II (tm) and the Raven logo",
   "are trademarks of Raven Software",
   "Corp.  The Id Software name and",
   "id logo are trademarks of",
   "Id Software, Inc.  Activision(r)",
   "is a registered trademark of",
   "Activision, Inc. All other",
   "trademarks are the property of",
   "their respective owners.",
   "",
   "",
   "",
   "Send bug descriptions to:",
   "   h2bugs@mail.ravensoft.com",
   "",
   "",
   "No yaks were harmed in the",
   "making of this game!"
};

#define MAX_LINES2 150

char *Credit2Text[MAX_LINES2] =
{
   "PowerTrip: James (emorog) Monroe",
   "Cartoons: Brian Raffel",
   "         (use more puzzles)",
   "Doc Keeper: Kevin Schilder",
   "",
   "Whip cracker: James Monroe",
   "",
   "Whipees:",
   "   Mike (i didn't break it) Gummelt",
   "   Josh (extern) Weier",
   "",
   "We don't deserve whipping:",
   "   Josh (I'm not on this project)",
   "         Heitzman",
   "   Nathan (deer hunter) Albury",
   "   Rick (model crusher) Johnson",
   "",
   "Bit Packer:",
   "   Mr. John (Slaine) Scott",
   "",
   "Lead Slacker: Jon (devil boy) Zuk",
   "",
   "Other Slackers:",
   "   Tom (can i have an office) Odell",
   "   Jeremy (nt crashed again) Statz",
   "   Mike (i should be doing my ",
   "         homework) Renner",
   "   Eric (the nose) Biessman",
   "   Kenn (.plan) Hoekstra",
   "   Matt (big elbow) Pinkston",
   "   Bobby (needs haircut) Duncanson",
   "   Brian (they're in my town) Raffel",
   "",
   "Use the mouse: Les Dorscheid",
   "",
   "What's a mouse?:",
   "   Kim (where's my desk) Lathrop",
   "   Gina (i can do your laundry)",
   "        Garren",
   "   Joe (broken axle) Koberstein",
   "   Kevin (titanic) Long",
   "   Jeff (norbert) Butler",
   "   Scott (what's the DEL key for?)",
   "          Rice",
   "   John (Shpluuurt!) Payne",
   "   Steve (crash) Raffel",
   "",
   "Boners:",
   "   Eric (terminator) Turman",
   "   Chaos Device",
   "",
   "Drum beater:",
   "   Kevin Schilder",
   "",
   "Whistle blower:",
   "   Chia Chin (bruce) Lee",
   "",
   "",
   "Activision",
   "",
   "Producer:",
   "   Steve 'Ferris' Stringer",
   "",
   "Marketing Product Manager:",
   "   Henk 'GODMODE' Hartong",
   "",
   "Marketing Associate:",
   "   Kevin 'Kraffinator' Kraff",
   "",
   "Senior Quality",
   "Assurance Lead:",
   "   Tim 'Outlaw' Vanlaw",
   "",
   "Quality Assurance Lead:",
   "   Doug Jacobs",
   "",
   "Shadow Finders:",
   "   Steve Rosenthal, Steve Elwell,",
   "   Chad Bordwell,",
   "   David 'Spice Girl' Baker,",
   "   Error Casillas, Damien Fischer,",
   "   Winnie Lee,"
   "   Ygor Krynytyskyy,",
   "   Samantha (Crusher) Lee, John Park",
   "   Ian Stevens, Chris Toft",
   "",
   "Production Testers:",
   "   Steve 'Damn It's Cold!'",
   "       Rosenthal and",
   "   Chad 'What Hotel Receipt?'",
   "        Bordwell",
   "",
   "Additional QA and Support:",
   "    Tony Villalobos",
   "    Jason Sullivan",
   "",
   "Installer by:",
   "   Steve 'Bahh' Stringer,",
   "   Adam Goldberg, Tanya Martino,",
   "   Eric Schmidt, Ronnie Lane",
   "",
   "Art Assistance by:",
   "   Carey 'Damien' Chico and",
   "   Franz Boehm",
   "",
   "BizDev Babe:",
   "   Jamie Bafus",
   "",
   "And...",
   "",
   "Our Big Toe:",
   "   Mitch Lasky",
   "",
   "",
   "Special Thanks to:",
   "  Id software",
   "  Anyone who ever worked for Raven,",
   "  (except for Alex)",
   "",
   "",
   "Published by Id Software, Inc.",
   "Distributed by Activision, Inc.",
   "",
   "The Id Software Technology used",
   "under license in Hexen II (tm)",
   "(c) 1996, 1997 Id Software, Inc.",
   "All Rights Reserved.",
   "",
   "Hexen(r) is a registered trademark",
   "of Raven Software Corp.",
   "Hexen II (tm) and the Raven logo",
   "are trademarks of Raven Software",
   "Corp.  The Id Software name and",
   "id logo are trademarks of",
   "Id Software, Inc.  Activision(r)",
   "is a registered trademark of",
   "Activision, Inc. All other",
   "trademarks are the property of",
   "their respective owners.",
   "",
   "",
   "",
   "Send bug descriptions to:",
   "   h2bugs@mail.ravensoft.com",
   "",
   "Special Thanks To:",
   "   E.H.S., The Osmonds,",
   "   B.B.V.D., Daisy The Lovin' Lamb,",
   "  'You Killed' Kenny,",
   "   and Baby Biessman.",
   "",
};

#define QUIT_SIZE 18

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;

	LinePos = 0;
	LineTimes = 0;
	LineText = CreditText;
	MaxLines = MAX_LINES;
	SoundPlayed = false;
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}
		break;

	case K_CROSS:
	case K_CIRCLE:
#ifdef PSP
	case K_AUX1:
	case K_AUX4:
	case K_INS:
#endif		
	key_dest = key_console;
		Host_Quit_f ();
		break;

	default:
		break;
	}

}
	
void M_Quit_Draw (void)
{
	int i,x,y,place,topy;
	qpic_t	*p;

	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	LinePos += host_frametime*1.75;
	if (LinePos > MaxLines + QUIT_SIZE + 2)
	{
		LinePos = 0;
		SoundPlayed = false;
		LineTimes++;
		if (LineTimes >= 2)
		{
			MaxLines = MAX_LINES2;
			LineText = Credit2Text;
			CDAudio_Play (12, false);
		}
	}

	y = 12;
	M_DrawTextBox (0, 0, 38, 23);
	M_PrintWhite (16, y,  "        Hexen II version 1.12       ");	y += 8;
	M_PrintWhite (16, y,  "         by Raven Software          ");	y += 16;

	if (LinePos > 55 && !SoundPlayed && LineText == Credit2Text)
	{
		S_LocalSound ("rj/steve.wav");
		SoundPlayed = true;
	}
	topy = y;
	place = floorf(LinePos);
	y -= floorf((LinePos - place) * 8);
	for(i=0;i<QUIT_SIZE;i++,y+=8)
	{
		if (i+place-QUIT_SIZE >= MaxLines)
			break;
		if (i+place < QUIT_SIZE) 
			continue;

		if (LineText[i+place-QUIT_SIZE][0] == ' ')
			M_PrintWhite(24,y,LineText[i+place-QUIT_SIZE]);
		else
			M_Print(24,y,LineText[i+place-QUIT_SIZE]);
	}

	p = Draw_CachePic ("gfx/box_mm2.lmp");
	x = 24;
	y = topy-8;
	for(i=4;i<38;i++,x+=8)
	{
		M_DrawPic(x, y, p);	//background at top for smooth scroll out
		M_DrawPic(x, y+(QUIT_SIZE*8), p);	//draw at bottom for smooth scroll in
	}

	y += (QUIT_SIZE*8)+8;
#if defined PSP
	M_PrintWhite (16, y-5,  "   Press CROSS or SQUARE to quit,   ");
	M_PrintWhite (16, y+5,  "       or CIRCLE to continue.       ");
#else

	M_PrintWhite (16, y,    "          Press X to exit           ");
#endif
}

int  osk_pos_x = 0;
int  osk_pos_y = 0;
int  max_len   = 0;
int  m_old_state = 0;

char* osk_out_buff = NULL;
#define CHAR_SIZE 8
#define MAX_Y 8
#define MAX_X 12

#define MAX_CHAR_LINE 36
#define MAX_CHAR      72

char *osk_text [] = 
	{ 
		" 1 2 3 4 5 6 7 8 9 0 - = ` ",
		" q w e r t y u i o p [ ]   ",
		"   a s d f g h j k l ; ' \\ ",
		"     z x c v b n m   , . / ",
		"                           ",
		" ! @ # $ % ^ & * ( ) _ + ~ ",
		" Q W E R T Y U I O P { }   ",
		"   A S D F G H J K L : \" | ",
		"     Z X C V B N M   < > ? "
	};

char *osk_help [] = 
	{ 
		"CONFIRM: ",
		" SQUARE  ",
		"CANCEL:  ",
		" CIRCLE  ",
		"DELETE:  ",
		" TRIAGLE ",
		"ADD CHAR:",
		" CROSS   ",
		""
	};

void M_Menu_OSK_f (char *input, char *output, int outlen)
{
	key_dest = key_menu;
	m_old_state = m_state;
	m_state = m_osk;
	m_entersound = false;
	max_len = outlen;
	strncpy(osk_buffer,input,max_len);
	osk_buffer[outlen] = '\0';
	osk_out_buff = output; 
}

void Con_OSK_f (char *input, char *output, int outlen)
{
	max_len = outlen;
	strncpy(osk_buffer,input,max_len);
	osk_buffer[outlen] = '\0';
	osk_out_buff = output; 
}


void M_OSK_Draw (void)
{
#ifdef PSP
	int x,y;
	int i;
	
	char *selected_line = osk_text[osk_pos_y]; 
	char selected_char[2];
	
	selected_char[0] = selected_line[1+(2*osk_pos_x)];
	selected_char[1] = '\0';
	if (selected_char[0] == ' ' || selected_char[0] == '\t') 
		selected_char[0] = 'X';
		
	y = 20;
	x = 16;

	M_DrawTextBox (10, 10, 		     26, 10);
	M_DrawTextBox (10+(26*CHAR_SIZE),    10,  10, 10);
	M_DrawTextBox (10, 10+(10*CHAR_SIZE),36,  3);
	
	for(i=0;i<=MAX_Y;i++) 
	{
		M_PrintWhite (x, y+(CHAR_SIZE*i), osk_text[i]);
		if (i % 2 == 0)
			M_Print      (x+(27*CHAR_SIZE), y+(CHAR_SIZE*i), osk_help[i]);
		else			
			M_PrintWhite (x+(27*CHAR_SIZE), y+(CHAR_SIZE*i), osk_help[i]);
	}
	
	int text_len = strlen(osk_buffer);
	if (text_len > MAX_CHAR_LINE) {
		
		char oneline[MAX_CHAR_LINE+1];
		strncpy(oneline,osk_buffer,MAX_CHAR_LINE);
		oneline[MAX_CHAR_LINE] = '\0';
		
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+2)), oneline );
		
		strncpy(oneline,osk_buffer+MAX_CHAR_LINE, text_len - MAX_CHAR_LINE);
		oneline[text_len - MAX_CHAR_LINE] = '\0';
		
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+3)), oneline );
		M_PrintWhite (x+4+(CHAR_SIZE*(text_len - MAX_CHAR_LINE)), y+4+(CHAR_SIZE*(MAX_Y+3)),"_");
	}
	else {
		M_Print (x+4, y+4+(CHAR_SIZE*(MAX_Y+2)), osk_buffer );
		M_PrintWhite (x+4+(CHAR_SIZE*(text_len)), y+4+(CHAR_SIZE*(MAX_Y+2)),"_");
	}
	M_Print      (x+((((osk_pos_x)*2)+1)*CHAR_SIZE), y+(osk_pos_y*CHAR_SIZE), selected_char);

#endif
}

void M_OSK_Key (int key)
{
#ifdef PSP
	switch (key)
	{
	case K_RIGHTARROW:
		osk_pos_x++;
		if (osk_pos_x > MAX_X)
			osk_pos_x = MAX_X;
		break;
	case K_LEFTARROW:
		osk_pos_x--;
		if (osk_pos_x < 0)
			osk_pos_x = 0;
		break;
	case K_DOWNARROW:
		osk_pos_y++;
		if (osk_pos_y > MAX_Y)
			osk_pos_y = MAX_Y;
		break;
	case K_UPARROW:
		osk_pos_y--;
		if (osk_pos_y < 0)
			osk_pos_y = 0;
		break;
	case K_CROSS:
	case K_CIRCLE: 
		if (max_len > strlen(osk_buffer)) {
			char *selected_line = osk_text[osk_pos_y]; 
			char selected_char[2];
			
			selected_char[0] = selected_line[1+(2*osk_pos_x)];
			
			if (selected_char[0] == '\t')
				selected_char[0] = ' ';
			
			selected_char[1] = '\0';
			strcat(osk_buffer,selected_char);		
		}
		break;
	case K_DEL:
		if (strlen(osk_buffer) > 0) {
			osk_buffer[strlen(osk_buffer)-1] = '\0';	
		}
		break;
	case K_INS:
		strncpy(osk_out_buff,osk_buffer,max_len);
		
		m_state = m_old_state;
		break;
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		m_state = m_old_state;
		break;
	default:
		break;
	}
#endif		
}

void Con_OSK_Key (int key)
{
#ifdef PSP
	switch (key)
	{
	case K_RIGHTARROW:
		osk_pos_x++;
		if (osk_pos_x > MAX_X)
			osk_pos_x = MAX_X;
		break;
	case K_LEFTARROW:
		osk_pos_x--;
		if (osk_pos_x < 0)
			osk_pos_x = 0;
		break;
	case K_DOWNARROW:
		osk_pos_y++;
		if (osk_pos_y > MAX_Y)
			osk_pos_y = MAX_Y;
		break;
	case K_UPARROW:
		osk_pos_y--;
		if (osk_pos_y < 0)
			osk_pos_y = 0;
		break;
	case K_AUX1:
	case K_AUX4: 
		if (max_len > strlen(osk_buffer)) {
			char *selected_line = osk_text[osk_pos_y]; 
			char selected_char[2];
			
			selected_char[0] = selected_line[1+(2*osk_pos_x)];
			
			if (selected_char[0] == '\t')
				selected_char[0] = ' ';
			
			selected_char[1] = '\0';
			strcat(osk_buffer,selected_char);		
		}
		break;
	case K_DEL:
		if (strlen(osk_buffer) > 0) {
			osk_buffer[strlen(osk_buffer)-1] = '\0';	
		}
		break;
	case K_INS:
		strncpy(osk_out_buff,osk_buffer,max_len);
		Con_SetOSKActive(false);
		break;
	case K_ENTER:
		Con_SetOSKActive(false);
		break;
	default:
		break;
	}
#endif		
}	

//=============================================================================

/* SERIAL CONFIG MENU */

int		serialConfig_cursor;
int		serialConfig_cursor_table[] = {48, 64, 80, 96, 112, 132};
#define	NUM_SERIALCONFIG_CMDS	6

static int ISA_uarts[]	= {0x3f8,0x2f8,0x3e8,0x2e8};
static int ISA_IRQs[]	= {4,3,4,3};
int serialConfig_baudrate[] = {9600,14400,19200,28800,38400,57600};

int		serialConfig_comport;
int		serialConfig_irq ;
int		serialConfig_baud;
char	serialConfig_phone[16];

void M_Menu_SerialConfig_f (void)
{
	int		n;
	int		port;
	int		baudrate;
	qboolean	useModem;

	key_dest = key_menu;
	m_state = m_serialconfig;
	m_entersound = true;
	if (JoiningGame && SerialConfig)
		serialConfig_cursor = 4;
	else
		serialConfig_cursor = 5;

	(*GetComPortConfig) (0, &port, &serialConfig_irq, &baudrate, &useModem);

	// map uart's port to COMx
	for (n = 0; n < 4; n++)
		if (ISA_uarts[n] == port)
			break;
	if (n == 4)
	{
		n = 0;
		serialConfig_irq = 4;
	}
	serialConfig_comport = n + 1;

	// map baudrate to index
	for (n = 0; n < 6; n++)
		if (serialConfig_baudrate[n] == baudrate)
			break;
	if (n == 6)
		n = 5;
	serialConfig_baud = n;

	m_return_onerror = false;
	m_return_reason[0] = 0;
}
				

void M_SerialConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*directModem;

	ScrollTitle("gfx/menu/title4.lmp");

	p = Draw_CachePic ("gfx/menu/title4.lmp");
	basex = (320-p->width)/2;

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	if (SerialConfig)
		directModem = "Modem";
	else
		directModem = "Direct Connect";
	M_Print (basex, 32, va ("%s - %s", startJoin, directModem));
	basex += 8;

	M_Print (basex, serialConfig_cursor_table[0], "Port");
	M_DrawTextBox (160, 40, 4, 1);
	M_Print (168, serialConfig_cursor_table[0], va("COM%u", serialConfig_comport));

	M_Print (basex, serialConfig_cursor_table[1], "IRQ");
	M_DrawTextBox (160, serialConfig_cursor_table[1]-8, 1, 1);
	M_Print (168, serialConfig_cursor_table[1], va("%u", serialConfig_irq));

	M_Print (basex, serialConfig_cursor_table[2], "Baud");
	M_DrawTextBox (160, serialConfig_cursor_table[2]-8, 5, 1);
	M_Print (168, serialConfig_cursor_table[2], va("%u", serialConfig_baudrate[serialConfig_baud]));

	if (SerialConfig)
	{
		M_Print (basex, serialConfig_cursor_table[3], "Modem Setup...");
		if (JoiningGame)
		{
			M_Print (basex, serialConfig_cursor_table[4], "Phone number");
			M_DrawTextBox (160, serialConfig_cursor_table[4]-8, 16, 1);
			M_Print (168, serialConfig_cursor_table[4], serialConfig_phone);
		}
	}

	if (JoiningGame)
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 7, 1);
		M_Print (basex+8, serialConfig_cursor_table[5], "Connect");
	}
	else
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 2, 1);
		M_Print (basex+8, serialConfig_cursor_table[5], "OK");
	}

	M_DrawCharacter (basex-8, serialConfig_cursor_table [serialConfig_cursor], 12+((int)(realtime*4)&1));

	if (serialConfig_cursor == 4)
		M_DrawCharacter (168 + 8*strlen(serialConfig_phone), serialConfig_cursor_table [serialConfig_cursor], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_SerialConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		serialConfig_cursor--;
		if (serialConfig_cursor < 0)
			serialConfig_cursor = NUM_SERIALCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		serialConfig_cursor++;
		if (serialConfig_cursor >= NUM_SERIALCONFIG_CMDS)
			serialConfig_cursor = 0;
		break;

	case K_LEFTARROW:
		if (serialConfig_cursor > 2)
			break;
		S_LocalSound ("raven/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport--;
			if (serialConfig_comport == 0)
				serialConfig_comport = 4;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq--;
			if (serialConfig_irq == 6)
				serialConfig_irq = 5;
			if (serialConfig_irq == 1)
				serialConfig_irq = 7;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud--;
			if (serialConfig_baud < 0)
				serialConfig_baud = 5;
		}

		break;

	case K_RIGHTARROW:
		if (serialConfig_cursor > 2)
			break;
forward:
		S_LocalSound ("raven/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport++;
			if (serialConfig_comport > 4)
				serialConfig_comport = 1;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq++;
			if (serialConfig_irq == 6)
				serialConfig_irq = 7;
			if (serialConfig_irq == 8)
				serialConfig_irq = 2;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud++;
			if (serialConfig_baud > 5)
				serialConfig_baud = 0;
		}

		break;

	case K_CROSS:
	case K_CIRCLE:
		if (serialConfig_cursor < 3)
			goto forward;

		m_entersound = true;

		if (serialConfig_cursor == 3)
		{
			(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

			M_Menu_ModemConfig_f ();
			break;
		}

		if (serialConfig_cursor == 4)
		{
			serialConfig_cursor = 5;
			break;
		}

		// serialConfig_cursor == 5 (OK/CONNECT)
		(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

		M_ConfigureNetSubsystem ();

		if (StartingGame)
		{
			M_Menu_GameOptions_f ();
			break;
		}

		m_return_state = m_state;
		m_return_onerror = true;
		key_dest = key_game;
		m_state = m_none;

		if (SerialConfig)
			Cbuf_AddText (va ("connect \"%s\"\n", serialConfig_phone));
		else
			Cbuf_AddText ("connect\n");
		break;
	
	case K_BACKSPACE:
		if (serialConfig_cursor == 4)
		{
			if (strlen(serialConfig_phone))
				serialConfig_phone[strlen(serialConfig_phone)-1] = 0;
		}
		break;
		
	default:
		if (key < 32 || key > 127)
			break;
		if (serialConfig_cursor == 4)
		{
			l = strlen(serialConfig_phone);
			if (l < 15)
			{
				serialConfig_phone[l+1] = 0;
				serialConfig_phone[l] = key;
			}
		}
	}

	if (DirectConfig && (serialConfig_cursor == 3 || serialConfig_cursor == 4))
	{
		if (key == K_UPARROW)
			serialConfig_cursor = 2;
		else
			serialConfig_cursor = 5;
	}
	if (SerialConfig && StartingGame && serialConfig_cursor == 4)
	{
		if (key == K_UPARROW)
			serialConfig_cursor = 3;
		else
			serialConfig_cursor = 5;
}
}

//=============================================================================
/* MODEM CONFIG MENU */

int		modemConfig_cursor;
int		modemConfig_cursor_table [] = {40, 56, 88, 120, 156};
#define NUM_MODEMCONFIG_CMDS	5

char	modemConfig_dialing;
char	modemConfig_clear [16];
char	modemConfig_init [32];
char	modemConfig_hangup [16];

void M_Menu_ModemConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_modemconfig;
	m_entersound = true;
	(*GetModemConfig) (0, &modemConfig_dialing, modemConfig_clear, modemConfig_init, modemConfig_hangup);
}
				

void M_ModemConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;

	ScrollTitle("gfx/menu/title4.lmp");
	p = Draw_CachePic ("gfx/menu/title4.lmp");
	basex = (320-p->width)/2;
	basex += 8;

	if (modemConfig_dialing == 'P')
		M_Print (basex, modemConfig_cursor_table[0], "Pulse Dialing");
	else
		M_Print (basex, modemConfig_cursor_table[0], "Touch Tone Dialing");

	M_Print (basex, modemConfig_cursor_table[1], "Clear");
	M_DrawTextBox (basex, modemConfig_cursor_table[1]+4, 16, 1);
	M_Print (basex+8, modemConfig_cursor_table[1]+12, modemConfig_clear);
	if (modemConfig_cursor == 1)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_clear), modemConfig_cursor_table[1]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[2], "Init");
	M_DrawTextBox (basex, modemConfig_cursor_table[2]+4, 30, 1);
	M_Print (basex+8, modemConfig_cursor_table[2]+12, modemConfig_init);
	if (modemConfig_cursor == 2)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_init), modemConfig_cursor_table[2]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[3], "Hangup");
	M_DrawTextBox (basex, modemConfig_cursor_table[3]+4, 16, 1);
	M_Print (basex+8, modemConfig_cursor_table[3]+12, modemConfig_hangup);
	if (modemConfig_cursor == 3)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_hangup), modemConfig_cursor_table[3]+12, 10+((int)(realtime*4)&1));

	M_DrawTextBox (basex, modemConfig_cursor_table[4]-8, 2, 1);
	M_Print (basex+8, modemConfig_cursor_table[4], "OK");

	M_DrawCharacter (basex-8, modemConfig_cursor_table [modemConfig_cursor], 12+((int)(realtime*4)&1));
}


void M_ModemConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_SerialConfig_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		modemConfig_cursor--;
		if (modemConfig_cursor < 0)
			modemConfig_cursor = NUM_MODEMCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		modemConfig_cursor++;
		if (modemConfig_cursor >= NUM_MODEMCONFIG_CMDS)
			modemConfig_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_RIGHTARROW:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			S_LocalSound ("raven/menu1.wav");
		}
		break;

	case K_CROSS:
	case K_CIRCLE:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			m_entersound = true;
		}

		if (modemConfig_cursor == 4)
		{
			(*SetModemConfig) (0, va ("%c", modemConfig_dialing), modemConfig_clear, modemConfig_init, modemConfig_hangup);
			m_entersound = true;
			M_Menu_SerialConfig_f ();
		}
		break;
	
	case K_BACKSPACE:
		if (modemConfig_cursor == 1)
		{
			if (strlen(modemConfig_clear))
				modemConfig_clear[strlen(modemConfig_clear)-1] = 0;
		}

		if (modemConfig_cursor == 2)
		{
			if (strlen(modemConfig_init))
				modemConfig_init[strlen(modemConfig_init)-1] = 0;
		}

		if (modemConfig_cursor == 3)
		{
			if (strlen(modemConfig_hangup))
				modemConfig_hangup[strlen(modemConfig_hangup)-1] = 0;
		}
		break;
		
	default:
		if (key < 32 || key > 127)
			break;

		if (modemConfig_cursor == 1)
		{
			l = strlen(modemConfig_clear);
			if (l < 15)
			{
				modemConfig_clear[l+1] = 0;
				modemConfig_clear[l] = key;
			}
		}

		if (modemConfig_cursor == 2)
		{
			l = strlen(modemConfig_init);
			if (l < 29)
			{
				modemConfig_init[l+1] = 0;
				modemConfig_init[l] = key;
			}
		}

		if (modemConfig_cursor == 3)
		{
			l = strlen(modemConfig_hangup);
			if (l < 15)
			{
				modemConfig_hangup[l+1] = 0;
				modemConfig_hangup[l] = key;
			}
		}
	}
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
int		lanConfig_cursor_table [] = {100, 120, 140, 172};
#define NUM_LANCONFIG_CMDS	4

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[30];

void M_Menu_LanConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor >= 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;

	setup_class = cl_playerclass.value;
	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
	setup_class--;
}
				

void M_LanConfig_Draw (void)
{
	int		basex;
	char	*startJoin;
	char	*protocol;

	ScrollTitle("gfx/menu/title4.lmp");
	basex = 48;

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	if (IPXConfig)
		protocol = "IPX";
	else
		protocol = "TCP/IP";
	M_Print (basex, 60, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 80, "Address:");
	if (IPXConfig)
		M_Print (basex+9*8, 80, my_ipx_address);
	else
		M_Print (basex+9*8, 80, my_tcpip_address);

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, 6, 1);
	M_Print (basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Class:");
		M_Print (basex+8*7, lanConfig_cursor_table[1], ClassNames[setup_class]);

		M_Print (basex, lanConfig_cursor_table[2], "Search for local games...");
		M_Print (basex, 156, "Join game at:");
		M_DrawTextBox (basex, lanConfig_cursor_table[3]-8, 30, 1);
		M_Print (basex+8, lanConfig_cursor_table[3], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		M_Print (basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [0], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 3)
		M_DrawCharacter (basex+8 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [3], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 192, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ENTER:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		lanConfig_cursor--;

		if (JoiningGame)
		{
			if (lanConfig_cursor < 0)
				lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		}
		else
		{
			if (lanConfig_cursor < 0)
				lanConfig_cursor = NUM_LANCONFIG_CMDS-2;
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;
	case K_INS:
		if (lanConfig_cursor == 0)
		{
			M_Menu_OSK_f(lanConfig_portname, lanConfig_portname, 6);
			break;
		}

		if (lanConfig_cursor == 2)
		{
			M_Menu_OSK_f(lanConfig_joinname, lanConfig_joinname, 22);
			break;
		}
		break;
	case K_CROSS:
	case K_CIRCLE:
		if ((JoiningGame && lanConfig_cursor <= 1) ||
			(!JoiningGame && lanConfig_cursor == 0))
			break;

		m_entersound = true;
		if (JoiningGame)
			Cbuf_AddText ( va ("playerclass %d\n", setup_class+1) );

		M_ConfigureNetSubsystem ();

		if ((JoiningGame && lanConfig_cursor == 2) ||
			(!JoiningGame && lanConfig_cursor == 1))
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 3)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			key_dest = key_game;
			m_state = m_none;
			Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			break;
		}

		break;
	
	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 3)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;
		
	case K_LEFTARROW:
		if (lanConfig_cursor != 1 || !JoiningGame)
			break;

		S_LocalSound ("raven/menu3.wav");
		setup_class--;
		if (setup_class < 0) 
			setup_class = NUM_CLASSES -1;
		break;

	case K_RIGHTARROW:
		if (lanConfig_cursor != 1 || !JoiningGame)
			break;

		S_LocalSound ("raven/menu3.wav");
		setup_class++;
		if (setup_class > NUM_CLASSES - 1) 
			setup_class = 0;
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 3)
		{
			l = strlen(lanConfig_joinname);
			if (l < 29)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = strlen(lanConfig_portname);
			if (l < 5)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
	{
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;
	}

	l =  atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	char	*name;
	char	*description;
} level_t;

level_t		levels[] =
{
	{"demo1", "Blackmarsh"},							// 0
	{"demo2", "Barbican"},								// 1

	{"ravdm1", "Deathmatch 1"},							// 2

	{"demo1","Blackmarsh"},								// 3
	{"demo2","Barbican"},								// 4
	{"demo3","The Mill"},								// 5
	{"village1","King's Court"},						// 6
	{"village2","Inner Courtyard"},						// 7
	{"village3","Stables"},								// 8
	{"village4","Palace Entrance"},						// 9
	{"village5","The Forgotten Chapel"},				// 10
	{"rider1a","Famine's Domain"},						// 11

	{"meso2","Plaza of the Sun"},						// 12
	{"meso1","The Palace of Columns"},					// 13
	{"meso3","Square of the Stream"},					// 14
	{"meso4","Tomb of the High Priest"},				// 15
	{"meso5","Obelisk of the Moon"},					// 16
	{"meso6","Court of 1000 Warriors"},					// 17
	{"meso8","Bridge of Stars"},						// 18
	{"meso9","Well of Souls"},							// 19

	{"egypt1","Temple of Horus"},						// 20
	{"egypt2","Ancient Temple of Nefertum"},			// 21
	{"egypt3","Temple of Nefertum"},					// 22
	{"egypt4","Palace of the Pharaoh"},					// 23
	{"egypt5","Pyramid of Anubis"},						// 24
	{"egypt6","Temple of Light"},						// 25
	{"egypt7","Shrine of Naos"},						// 26
	{"rider2c","Pestilence's Lair"},					// 27

	{"romeric1","The Hall of Heroes"},					// 28
	{"romeric2","Gardens of Athena"},					// 29
	{"romeric3","Forum of Zeus"},						// 30
	{"romeric4","Baths of Demetrius"},					// 31
	{"romeric5","Temple of Mars"},						// 32
	{"romeric6","Coliseum of War"},						// 33
	{"romeric7","Reflecting Pool"},						// 34

	{"cath","Cathedral"},								// 35
	{"tower","Tower of the Dark Mage"},					// 36
	{"castle4","The Underhalls"},						// 37
	{"castle5","Eidolon's Ordeal"},						// 38
	{"eidolon","Eidolon's Lair"},						// 39
  
	{"ravdm1","Atrium of Immolation"},					// 40
	{"ravdm2","Total Carnage"},							// 41
	{"ravdm3","Reckless Abandon"},						// 42
	{"ravdm4","Temple of RA"},							// 43
	{"ravdm5","Tom Foolery"},							// 44

	{"ravdm1", "Deathmatch 1"},							// 45

//OEM
	{"demo1","Blackmarsh"},								// 46
	{"demo2","Barbican"},								// 47
	{"demo3","The Mill"},								// 48
	{"village1","King's Court"},						// 49
	{"village2","Inner Courtyard"},						// 50
	{"village3","Stables"},								// 51
	{"village4","Palace Entrance"},						// 52
	{"village5","The Forgotten Chapel"},				// 53
	{"rider1a","Famine's Domain"},						// 54

//Mission Pack
	{"keep1",	"Eidolon's Lair"},						// 55
	{"keep2",	"Village of Turnabel"},					// 56
	{"keep3",	"Duke's Keep"},							// 57
	{"keep4",	"The Catacombs"},						// 58
	{"keep5",	"Hall of the Dead"},					// 59

	{"tibet1",	"Tulku"},								// 60
	{"tibet2",	"Ice Caverns"},							// 61
	{"tibet3",	"The False Temple"},					// 62
	{"tibet4",	"Courtyards of Tsok"},					// 63
	{"tibet5",	"Temple of Kalachakra"},				// 64
	{"tibet6",	"Temple of Bardo"},						// 65
	{"tibet7",	"Temple of Phurbu"},					// 66
	{"tibet8",	"Palace of Emperor Egg Chen"},			// 67
	{"tibet9",	"Palace Inner Chambers"},				// 68
	{"tibet10",	"The Inner Sanctum of Praevus"},		// 69
};

typedef struct
{
	char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	// Demo
	{"Demo", 0, 2},
	{"Demo Deathmatch", 2, 1},

	// Registered
	{"Village", 3, 9},
	{"Meso", 12, 8},
	{"Egypt", 20, 8},
	{"Romeric", 28, 7},
	{"Cathedral", 35, 5},
	{"MISSION PACK", 55, 15},
	{"Deathmatch", 40, 5},

	// OEM
	{"Village", 46, 9},
	{"Deathmatch", 45, 1},
};

#define OEM_START 9
#define REG_START 2
#define MP_START 7
#define DM_START 8

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 104, 112, 128, 136};
#define	NUM_GAMEOPTIONS	11
int		gameoptions_cursor;

void M_Menu_GameOptions_f (void)
{
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;

	setup_class = cl_playerclass.value;
	if (setup_class < 1 || setup_class > NUM_CLASSES)
		setup_class = NUM_CLASSES;
	setup_class--;

	if (oem.value && startepisode < OEM_START)
		startepisode = OEM_START;

	if (registered.value && (startepisode < REG_START || startepisode >= OEM_START))
		startepisode = REG_START;

	if (coop.value)
	{
		startlevel = 0;
		if (startepisode == 1)
			startepisode = 0;
		else if (startepisode == DM_START)
			startepisode = REG_START;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS-1)
			gameoptions_cursor = 0;
	}
#ifndef NO_PRAVEUS
	if (!m_oldmission.value)
	{
		startepisode = MP_START;
	}
#endif
}

void M_GameOptions_Draw (void)
{

	ScrollTitle("gfx/menu/title4.lmp");

	M_DrawTextBox (152+8, 60, 10, 1);
	M_Print (160+8, 68, "begin game");
	
	M_Print (0+8, 84, "      Max players");
	M_Print (160+8, 84, va("%i", maxplayers) );

	M_Print (0+8, 92, "        Game Type");
	if (coop.value)
		M_Print (160+8, 92, "Cooperative");
	else
		M_Print (160+8, 92, "Deathmatch");

	M_Print (0+8, 100, "        Teamplay");
	{
		char *msg;

		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
		M_Print (160+8, 100, msg);
	}

	M_Print (0+8, 108, "            Class");
	M_Print (160+8, 108, ClassNames[setup_class]);

	M_Print (0+8, 116, "       Difficulty");

	M_Print (160+8, 116, DiffNames[setup_class][(int)skill.value]);

	M_Print (0+8, 124, "       Frag Limit");
	if (fraglimit.value == 0)
		M_Print (160+8, 124, "none");
	else
		M_Print (160+8, 124, va("%i frags", (int)fraglimit.value));

	M_Print (0+8, 132, "       Time Limit");
	if (timelimit.value == 0)
		M_Print (160+8, 132, "none");
	else
		M_Print (160+8, 132, va("%i minutes", (int)timelimit.value));

	M_Print (0+8, 140, "     Random Class");
	if (randomclass.value)
		M_Print (160+8, 140, "on");
	else
		M_Print (160+8, 140, "off");

	M_Print (0+8, 156, "         Episode");
	M_Print (160+8, 156, episodes[startepisode].description);

	M_Print (0+8, 164, "           Level");
	M_Print (160+8, 164, levels[episodes[startepisode].firstLevel + startlevel].name);
	M_Print (96, 180, levels[episodes[startepisode].firstLevel + startlevel].description);

// line cursor
	M_DrawCharacter (172-16, gameoptions_cursor_table[gameoptions_cursor]+28, 12+((int)(realtime*4)&1));
}


void M_NetStart_Change (int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_SetValue ("coop", coop.value ? 0 : 1);
		if (coop.value)
		{
			startlevel = 0;
			if (startepisode == 1)
				startepisode = 0;
			else if (startepisode == DM_START)
				startepisode = REG_START;

#ifndef NO_PRAVEUS
			if (!m_oldmission.value)
			{
				startepisode = MP_START;
			}
#endif
		}
		break;

	case 3:
			count = 2;

		Cvar_SetValue ("teamplay", teamplay.value + dir);
		if (teamplay.value > count)
			Cvar_SetValue ("teamplay", 0);
		else if (teamplay.value < 0)
			Cvar_SetValue ("teamplay", count);
		break;

	case 4:
		setup_class += dir;
		if (setup_class < 0) 
			setup_class = NUM_CLASSES - 1;
		if (setup_class > NUM_CLASSES - 1) 
			setup_class = 0;
		break;

	case 5:
		Cvar_SetValue ("skill", skill.value + dir);
		if (skill.value > 3)
			Cvar_SetValue ("skill", 0);
		if (skill.value < 0)
			Cvar_SetValue ("skill", 3);
		break;

	case 6:
		Cvar_SetValue ("fraglimit", fraglimit.value + dir*10);
		if (fraglimit.value > 100)
			Cvar_SetValue ("fraglimit", 0);
		if (fraglimit.value < 0)
			Cvar_SetValue ("fraglimit", 100);
		break;

	case 7:
		Cvar_SetValue ("timelimit", timelimit.value + dir*5);
		if (timelimit.value > 60)
			Cvar_SetValue ("timelimit", 0);
		if (timelimit.value < 0)
			Cvar_SetValue ("timelimit", 60);
		break;

	case 8:
		if (randomclass.value)
			Cvar_SetValue ("randomclass", 0);
		else
			Cvar_SetValue ("randomclass", 1);
		break;

	case 9:
		startepisode += dir;

		if (registered.value)
		{
			count = DM_START;
			if (!coop.value)
				count++;
			else
			{
#ifndef NO_PRAVEUS
				if (!m_oldmission.value)
				{
					startepisode = MP_START;
				}
#endif
			}
			if (startepisode < REG_START)
				startepisode = count - 1;

			if (startepisode >= count)
				startepisode = REG_START;

			startlevel = 0;
		}
		else if (oem.value)
		{
			count = 10;

			if (startepisode < 8)
				startepisode = count - 1;

			if (startepisode >= count)
				startepisode = 8;

			startlevel = 0;
		}
		else
		{
			count = 2;

			if (startepisode < 0)
				startepisode = count - 1;

			if (startepisode >= count)
				startepisode = 0;

			startlevel = 0;
		}
		break;

	case 10:
		if (coop.value)
		{
			startlevel = 0;
			break;
		}
		startlevel += dir;
		count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("raven/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
		{
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
			if (coop.value)
				gameoptions_cursor--;
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("raven/menu1.wav");
		gameoptions_cursor++;
		if (coop.value)
		{
			if (gameoptions_cursor >= NUM_GAMEOPTIONS-1)
				gameoptions_cursor = 0;
		}
		else
		{
			if (gameoptions_cursor >= NUM_GAMEOPTIONS)
				gameoptions_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("raven/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("raven/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_CROSS:
	case K_CIRCLE:
		S_LocalSound ("raven/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ( va ("playerclass %d\n", setup_class+1) );
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText ( va ("maxplayers %u\n", maxplayers) );
			//SCR_BeginLoadingPlaque ();

			Cbuf_AddText ( va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name) );

			return;
		}
		
		M_NetStart_Change (1);
		break;	
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;

void M_Menu_Search_f (void)
{
	key_dest = key_menu;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw (void)
{
	int x;

	ScrollTitle("gfx/menu/title4.lmp");

	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 60, 12, 1);
	M_Print (x, 68, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 92, "No Hexen II servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

int		slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n;
	char	string [64],*name;
	

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int	i,j;
			hostcache_t temp;
			for (i = 0; i < hostCacheCount; i++)
				for (j = i+1; j < hostCacheCount; j++)
					if (strcmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}

	ScrollTitle("gfx/menu/title4.lmp");
	for (n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].driver == 0)
			name = net_drivers[hostcache[n].driver].name;
		else
			name = net_landrivers[hostcache[n].ldriver].name;

		if (hostcache[n].maxusers)
			sprintf(string, "%-11.11s %-8.8s %-10.10s %2u/%2u\n", hostcache[n].name, name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		else
			sprintf(string, "%-11.11s %-8.8s %-10.10s\n", hostcache[n].name, name, hostcache[n].map);
		M_Print (16, 60 + 8*n, string);
	}
	M_DrawCharacter (0, 60 + slist_cursor*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 176, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ENTER:
	case K_START:
	case K_TRIANGLE:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("raven/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("raven/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_CROSS:
	case K_CIRCLE:
		S_LocalSound ("raven/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		key_dest = key_game;
		m_state = m_none;
		Cbuf_AddText ( va ("connect \"%s\"\n", hostcache[slist_cursor].cname) );
		break;

	default:
		break;
	}

}

//=============================================================================
/* Menu Subsystem */


void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	Cmd_AddCommand ("menu_class", M_Menu_Class2_f);

	M_BuildBigCharWidth();
	Cvar_RegisterVariable (&m_oldmission);
	m_oldmission.value = 1;
}


void M_Draw (void)
{
	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			//VID_UnlockBuffer ();
			S_ExtraUpdate ();
			//VID_LockBuffer ();
		}
		else
			Draw_FadeScreen ();

		scr_fullupdate = 0;
	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_difficulty:
		M_Difficulty_Draw ();
		break;

	case m_class:
		M_Class_Draw ();
		break;

	case m_load:
	case m_mload:
		M_Load_Draw ();
		break;

	case m_save:
	case m_msave:
		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_net:
		M_Net_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		M_Quit_Draw ();
		break;

	case m_serialconfig:
		M_SerialConfig_Draw ();
		break;

	case m_modemconfig:
		M_ModemConfig_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;

	case m_osk:
		M_OSK_Draw();
		break;	
	}

	if (m_entersound)
	{
		S_LocalSound ("raven/menu2.wav");
		m_entersound = false;
	}

	//VID_UnlockBuffer ();
	S_ExtraUpdate ();
	//VID_LockBuffer ();
}


void M_Keydown (int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_difficulty:
		M_Difficulty_Key (key);
		return;

	case m_class:
		M_Class_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_mload:
		M_MLoad_Key (key);
		return;

	case m_msave:
		M_MSave_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_net:
		M_Net_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_video:
		M_Video_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_serialconfig:
		M_SerialConfig_Key (key);
		return;

	case m_modemconfig:
		M_ModemConfig_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;

	case m_osk:
		M_OSK_Key(key);		
	}
}


void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config

	Cbuf_AddText ("stopdemo\n");
	if (SerialConfig || DirectConfig)
	{
		Cbuf_AddText ("com1 enable\n");
	}

	if (IPXConfig || TCPIPConfig)
		net_hostport = lanConfig_port;
}

/*
 * $Log: /H2 Mission Pack/Menu.c $
 * 
 * 41    3/20/98 2:03p Jmonroe
 * changed default to not allow old missions.
 * 
 * 40    3/19/98 1:21p Jmonroe
 * 
 * 39    3/19/98 12:58a Jmonroe
 * 
 * 38    3/18/98 11:34p Jmonroe
 * fixed gl renderheight in intermission, fixed bottom plaque draw, added
 * credit cd track
 * 
 * 37    3/18/98 1:09p Jmonroe
 * 
 * 36    3/17/98 8:11p Jmonroe
 * 
 * 35    3/17/98 4:22p Jmonroe
 * 
 * 34    3/17/98 4:15p Jmonroe
 * 
 * 33    3/17/98 11:51a Jmonroe
 * 
 * 32    3/16/98 5:33p Jweier
 * 
 * 31    3/16/98 3:52p Jmonroe
 * fixed info_masks for load/save changelevel
 * 
 * 30    3/16/98 12:01a Jweier
 * 
 * 29    3/15/98 10:33p Jweier
 * 
 * 28    3/14/98 5:39p Jmonroe
 * made info_plaque draw safe, fixed bit checking
 * 
 * 27    3/14/98 12:50p Jmonroe
 * 
 * 26    3/13/98 6:25p Jmonroe
 * 
 * 25    3/13/98 12:22p Jweier
 * 
 * 24    3/13/98 12:19a Jmonroe
 * fixed lookspring
 * 
 * 23    3/11/98 7:12p Mgummelt
 * 
 * 22    3/11/98 6:20p Mgummelt
 * 
 * 21    3/10/98 11:38a Jweier
 * 
 * 20    3/04/98 11:42a Jmonroe
 * 
 * 19    3/03/98 1:41p Jmonroe
 * removed old mp stuff
 * 
 * 18    3/02/98 11:04p Jmonroe
 * changed start sound back to byte, added stopsound, put in a hack fix
 * for touchtriggers area getting removed
 * 
 * 17    3/02/98 2:22p Jmonroe
 * ADDED map names in dm start menu 
 * 
 * 16    3/01/98 8:20p Jmonroe
 * removed the slow "quake" version of common functions
 * 
 * 15    2/27/98 11:53p Jweier
 * 
 * 14    2/26/98 3:09p Jmonroe
 * fixed gl for numclasses
 * 
 * 13    2/23/98 9:18p Jmonroe
 * 
 * 12    2/23/98 4:54p Jmonroe
 * added show objectives to customize controls
 * 
 * 11    2/23/98 2:54p Jmonroe
 * added crosshair and mlook to the option menu
 * added start mission pack to the single play menu
 * 
 * 10    2/19/98 3:32p Jweier
 * 
 * 9     2/08/98 6:08p Mgummelt
 * 
 * 8     2/08/98 6:07p Jweier
 * 
 * 7     1/22/98 9:06p Jmonroe
 * final map names  , no descriptions yet
 * 
 * 6     1/21/98 10:29a Plipo
 * 
 * 5     1/18/98 8:06p Jmonroe
 * all of rick's patch code is in now
 * 
 * 4     1/15/98 10:04p Jmonroe
 * added stub mpack menu stuff
 * 
 * 91    10/29/97 5:39p Jheitzman
 * 
 * 90    10/28/97 6:26p Jheitzman
 * 
 * 89    10/28/97 2:58p Jheitzman
 * 
 * 87    10/06/97 6:04p Rjohnson
 * Fix for save games and version update
 * 
 * 86    9/25/97 11:56p Rjohnson
 * Version update
 * 
 * 85    9/25/97 11:47a Rjohnson
 * Update
 * 
 * 84    9/23/97 8:56p Rjohnson
 * Updates
 * 
 * 83    9/15/97 11:15a Rjohnson
 * Updates
 * 
 * 82    9/09/97 10:49a Rjohnson
 * Updates
 * 
 * 81    9/04/97 5:33p Rjohnson
 * Id updates
 * 
 * 80    9/04/97 4:44p Rjohnson
 * Updates
 * 
 * 79    9/02/97 12:25a Rjohnson
 * Font Update
 * 
 * 78    9/01/97 2:08a Rjohnson
 * Updates
 * 
 * 77    8/31/97 9:27p Rjohnson
 * GL Updates
 * 
 * 76    8/31/97 3:45p Rjohnson
 * Map Updates
 * 
 * 75    8/30/97 6:16p Rjohnson
 * Centering text
 * 
 * 74    8/28/97 3:36p Rjohnson
 * Version Update
 * 
 * 73    8/27/97 12:11p Rjohnson
 * Adjustments
 * 
 * 72    8/26/97 8:58p Rjohnson
 * Credit Update
 * 
 * 71    8/26/97 8:51p Rjohnson
 * Credit Changes
 * 
 * 70    8/26/97 8:17a Rjohnson
 * Just a few changes
 * 
 * 69    8/25/97 11:40a Rjohnson
 * Activision People
 * 
 * 68    8/25/97 11:23a Rjohnson
 * name Update
 * 
 * 67    8/24/97 11:45a Rjohnson
 * Gary change
 * 
 * 66    8/24/97 11:06a Rjohnson
 * Fix
 * 
 * 65    8/24/97 10:56a Rjohnson
 * Bob Update
 * 
 * 64    8/24/97 10:52a Rjohnson
 * Updates
 * 
 * 63    8/21/97 10:12p Rjohnson
 * Version Update
 * 
 * 62    8/21/97 2:11p Rjohnson
 * Menu Update
 * 
 * 61    8/21/97 11:45a Rjohnson
 * Fix for menu
 * 
 * 60    8/21/97 11:42a Rjohnson
 * Credit Text
 * 
 * 59    8/21/97 1:09a Rjohnson
 * Credits Update
 * 
 * 58    8/20/97 8:30p Rlove
 * 
 * 57    8/20/97 8:25p Rjohnson
 * Added player class to net menus
 * 
 * 56    8/20/97 5:07p Rjohnson
 * Added a toggle for the quick frag list
 * 
 * 55    8/20/97 4:58p Rjohnson
 * Difficulty name change
 * 
 * 54    8/20/97 3:13p Rlove
 * 
 * 53    8/20/97 2:05p Rjohnson
 * fix for internationalization
 * 
 * 52    8/20/97 11:09a Rjohnson
 * Fix for console font
 * 
 * 51    8/19/97 11:59p Rjohnson
 * Difficulty levels based on class
 * 
 * 50    8/19/97 7:45p Rjohnson
 * Fix for menu
 * 
 * 49    8/18/97 11:44p Rjohnson
 * Fixes for loading
 * 
 * 48    8/18/97 4:47p Rjohnson
 * Difficulty name update
 * 
 * 47    8/18/97 2:16p Rjohnson
 * Fix for gl version
 * 
 * 46    8/18/97 2:06p Rjohnson
 * Difficulty Names
 * 
 * 45    8/17/97 6:27p Rjohnson
 * Added frag and inventory keys to menu
 * 
 * 44    8/17/97 3:28p Rjohnson
 * Fix for color selection
 * 
 * 43    8/16/97 10:52a Rjohnson
 * Level Update
 * 
 * 42    8/15/97 3:10p Rjohnson
 * Precache Update
 * 
 * 41    8/15/97 1:58p Rlove
 * 
 * 40    8/15/97 6:32a Rlove
 * Added a few more keys to keyboard config screen
 * 
 * 39    8/14/97 11:59p Rjohnson
 * Fix
 * 
 * 38    8/14/97 11:45p Rjohnson
 * Connection type info
 * 
 * 37    8/14/97 10:14p Rjohnson
 * Menu Updates
 * 
 * 36    8/14/97 2:37p Rjohnson
 * Fix for save games
 * 
 * 35    8/14/97 12:27p Rlove
 * 
 * 34    8/14/97 9:22a Rlove
 * Added portraits to Player Class Menu
 * 
 * 33    8/12/97 8:17p Rjohnson
 * Change for menu
 * 
 * 32    8/04/97 3:51p Rjohnson
 * Fix for the menus
 * 
 * 31    8/01/97 3:17p Rjohnson
 * Added new menu graphics
 * 
 * 30    7/18/97 4:37p Rjohnson
 * Added deathmatch
 * 
 * 29    7/15/97 5:52p Rjohnson
 * Added help screens
 * 
 * 28    7/15/97 2:21p Rjohnson
 * Added Difficulty graphic
 * 
 * 27    7/14/97 2:52p Rjohnson
 * Demo stuff
 * 
 * 26    7/11/97 5:59p Rjohnson
 * Got menus working for single player
 * 
 * 25    7/11/97 5:21p Rjohnson
 * RJNET Updates
 * 
 * 24    6/17/97 10:28a Bgokey
 * 
 * 23    4/22/97 5:19p Rjohnson
 * More menu updates
 * 
 * 22    4/18/97 6:24p Rjohnson
 * More menu mods
 * 
 * 21    4/18/97 12:24p Rjohnson
 * Added scrolly hexen plaque
 * 
 * 20    4/18/97 12:10p Rjohnson
 * Added josh to the credits
 * 
 * 19    4/18/97 11:25a Rjohnson
 * Changed the quit message
 * 
 * 18    4/17/97 3:42p Rjohnson
 * Modifications for the gl version for menus
 * 
 * 17    4/17/97 12:25p Rjohnson
 * Modifications for the gl version
 * 
 * 16    4/15/97 6:40p Rjohnson
 * Position update of the hexen plaque
 * 
 * 15    4/15/97 5:58p Rjohnson
 * More menu bits
 * 
 * 14    4/14/97 5:02p Rjohnson
 * More menu refinement
 * 
 * 13    4/14/97 12:17p Rjohnson
 * Menus now use new font
 * 
 * 12    4/08/97 11:23a Rjohnson
 * Modified the menu sounds
 * 
 * 11    4/01/97 4:38p Rjohnson
 * Modification to fix a crash in the network options
 * 
 * 10    3/31/97 7:24p Rjohnson
 * Added a playerclass field and made sure the server/clients handle it
 * properly
 * 
 * 9     3/31/97 4:09p Rlove
 * Removing references to Quake
 * 
 * 8     3/14/97 9:21a Rlove
 * Plaques are done 
 * 
 * 7     2/20/97 12:13p Rjohnson
 * Code fixes for id update
 * 
 * 6     2/19/97 12:29p Rjohnson
 * More Id Updates
 * 
 * 5     2/18/97 12:17p Rjohnson
 * Id Updates
 * 
 * 4     1/02/97 1:33p Rlove
 * Class and Difficulty Menus added
 * 
 */
 