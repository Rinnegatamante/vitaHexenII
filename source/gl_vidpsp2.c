// gl_vidnt.c -- NT GL vid component

#include <stdarg.h>
#include <stdio.h>
#include <vitasdk.h>
#include <vitaGL.h>
#include "quakedef.h"

#define MAX_MODE_LIST	30
#define VID_ROW_SIZE	3
#define WARP_WIDTH		320
#define WARP_HEIGHT		200
#define MAXWIDTH		10000
#define MAXHEIGHT		10000
#define BASEWIDTH		320
#define BASEHEIGHT		200

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

int scr_width = 960, scr_height = 544;

byte globalcolormap[VID_GRADES*256];

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

qboolean		DDActive;
qboolean		scr_skipupdate;

qboolean	vid_initialized = false;

unsigned char	vid_curpal[256*3];
float RTint[256],GTint[256],BTint[256];

glvert_t glv;

cvar_t	gl_ztrick = {"gl_ztrick","0"};

viddef_t	vid;				// global video state

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];
unsigned	d_8to24TranslucentTable[256];

float		gldepthmin, gldepthmax;

char *VID_GetModeDescription (int mode);
void ClearAllStates (void);
void VID_UpdateWindowStatus (void);
void GL_Init (void);

//====================================

cvar_t		vid_mode = {"vid_mode","0", false};
// Note that 0 is MODE_WINDOWED
cvar_t		_vid_default_mode = {"_vid_default_mode","0", true};
// Note that 3 is MODE_FULLSCREEN_DEFAULT
cvar_t		_vid_default_mode_win = {"_vid_default_mode_win","3", true};
cvar_t		vid_wait = {"vid_wait","0"};
cvar_t		vid_nopageflip = {"vid_nopageflip","0", true};
cvar_t		_vid_wait_override = {"_vid_wait_override", "0", true};
cvar_t		vid_config_x = {"vid_config_x","960", true};
cvar_t		vid_config_y = {"vid_config_y","544", true};
cvar_t		vid_stretch_by_2 = {"vid_stretch_by_2","1", true};
cvar_t		_windowed_mouse = {"_windowed_mouse","0", true};

int			window_center_x, window_center_y, window_x, window_y, window_width, window_height;

float sintablef[17] = {
	 0.000000f, 0.382683f, 0.707107f,
	 0.923879f, 1.000000f, 0.923879f,
	 0.707107f, 0.382683f, 0.000000f,
	-0.382683f,-0.707107f,-0.923879f,
	-1.000000f,-0.923879f,-0.707107f,
	-0.382683f, 0.000000f
};
	
float costablef[17] = {
	 1.000000f, 0.923879f, 0.707107f,
	 0.382683f, 0.000000f,-0.382683f,
	-0.707107f,-0.923879f,-1.000000f,
	-0.923879f,-0.707107f,-0.382683f,
	 0.000000f, 0.382683f, 0.707107f,
	 0.923879f, 1.000000f
};

// direct draw software compatability stuff

void VID_HandlePause (qboolean pause)
{
}

void VID_ForceLockState (int lk)
{
}

int VID_ForceUnlockedAndReturnState (void)
{
	return 0;
}

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

#define MAX_INDICES 4096
uint16_t* indices;

GLuint fs[9];
GLuint vs[4];
GLuint programs[9];

void* GL_LoadShader(const char* filename, GLuint idx, GLboolean fragment){
	FILE* f = fopen(filename, "rb");
	fseek(f, 0, SEEK_END);
	long int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	void* res = malloc(size);
	fread(res, 1, size, f);
	fclose(f);
	if (fragment) glShaderBinary(1, &fs[idx], 0, res, size);
	else glShaderBinary(1, &vs[idx], 0, res, size);
	free(res);
}

static int state_mask = 0;
static int texenv_mask = 0;
static int texcoord_state = 0;
static int alpha_state = 0;
static int color_state = 0;
GLint monocolor;
GLint modulcolor[2];

void GL_SetProgram(){
	switch (state_mask + texenv_mask){
		case 0x00: // Everything off
		case 0x04: // Modulate
		case 0x08: // Alpha Test
		case 0x0C: // Alpha Test + Modulate
			glUseProgram(programs[NO_COLOR]);
			break;
		case 0x01: // Texcoord
		case 0x03: // Texcoord + Color
			glUseProgram(programs[TEX2D_REPL]);
			break;
		case 0x02: // Color
		case 0x06: // Color + Modulate
			glUseProgram(programs[RGBA_COLOR]);
			break;
		case 0x05: // Modulate + Texcoord
			glUseProgram(programs[TEX2D_MODUL]);
			break;
		case 0x07: // Modulate + Texcoord + Color
			glUseProgram(programs[TEX2D_MODUL_CLR]);
			break;
		case 0x09: // Alpha Test + Texcoord
		case 0x0B: // Alpha Test + Color + Texcoord
			glUseProgram(programs[TEX2D_REPL_A]);
			break;
		case 0x0A: // Alpha Test + Color
		case 0x0E: // Alpha Test + Modulate + Color
			glUseProgram(programs[RGBA_CLR_A]);
			break;
		case 0x0D: // Alpha Test + Modulate + Texcoord
			glUseProgram(programs[TEX2D_MODUL_A]);
			break;
		case 0x0F: // Alpha Test + Modulate + Texcood + Color
			glUseProgram(programs[FULL_A]);
			break;
		default:
			break;
	}
}

void GL_EnableState(GLenum state){	
	switch (state){
		case GL_TEXTURE_COORD_ARRAY:
			if (!texcoord_state){
				state_mask += 0x01;
				texcoord_state = 1;
			}
			break;
		case GL_COLOR_ARRAY:
			if (!color_state){
				state_mask += 0x02;
				color_state = 1;
			}
			break;
		case GL_MODULATE:
			texenv_mask = 0x04;
			break;
		case GL_REPLACE:
			texenv_mask = 0;
			break;
		case GL_ALPHA_TEST:
			if (!alpha_state){
				state_mask += 0x08;
				alpha_state = 1;
			} 
			break;
	}
	GL_SetProgram();
}

void GL_DisableState(GLenum state){	
	switch (state){
		case GL_TEXTURE_COORD_ARRAY:
			if (texcoord_state){
				state_mask -= 0x01;
				texcoord_state = 0;
			}
			break;
		case GL_COLOR_ARRAY:
			if (color_state){
				state_mask -= 0x02;
				color_state = 0;
			}
			break;
		case GL_ALPHA_TEST:
			if (alpha_state){
				state_mask -= 0x08;
				alpha_state = 0;
			} 
			break;
		default:
			break;
	}
	GL_SetProgram();
}

static float cur_clr[4];

void GL_DrawPolygon(GLenum prim, int num){
	if ((state_mask + texenv_mask) == 0x05) glUniform4fv(modulcolor[0], 1, cur_clr);
	else if ((state_mask + texenv_mask) == 0x0D) glUniform4fv(modulcolor[1], 1, cur_clr);
	vglDrawObjects(prim, num, GL_TRUE);
}

void GL_Color(float r, float g, float b, float a){
	cur_clr[0] = r;
	cur_clr[1] = g;
	cur_clr[2] = b;
	cur_clr[3] = a;
}

qboolean shaders_set = false;
void GL_ResetShaders(){
	glFinish();
	int i;
	if (shaders_set){
		for (i=0;i<9;i++){
			glDeleteProgram(programs[i]);
		}
		for (i=0;i<9;i++){
			glDeleteShader(fs[i]);
		}
		for (i=0;i<4;i++){
			glDeleteShader(vs[i]);
		}
	}else shaders_set = true; 
	
	// Loading shaders
	for (i=0;i<9;i++){
		fs[i] = glCreateShader(GL_FRAGMENT_SHADER);
	}
	for (i=0;i<4;i++){
		vs[i] = glCreateShader(GL_VERTEX_SHADER);
	}
	
	GL_LoadShader("app0:shaders/modulate_f.gxp", MODULATE, GL_TRUE);
	GL_LoadShader("app0:shaders/modulate_rgba_f.gxp", MODULATE_WITH_COLOR, GL_TRUE);
	GL_LoadShader("app0:shaders/replace_f.gxp", REPLACE, GL_TRUE);
	GL_LoadShader("app0:shaders/modulate_alpha_f.gxp", MODULATE_A, GL_TRUE);
	GL_LoadShader("app0:shaders/modulate_rgba_alpha_f.gxp", MODULATE_COLOR_A, GL_TRUE);
	GL_LoadShader("app0:shaders/replace_alpha_f.gxp", REPLACE_A, GL_TRUE);
	GL_LoadShader("app0:shaders/texture2d_v.gxp", TEXTURE2D, GL_FALSE);
	GL_LoadShader("app0:shaders/texture2d_rgba_v.gxp", TEXTURE2D_WITH_COLOR, GL_FALSE);
	
	GL_LoadShader("app0:shaders/rgba_f.gxp", RGBA_COLOR, GL_TRUE);
	GL_LoadShader("app0:shaders/vertex_f.gxp", MONO_COLOR, GL_TRUE);
	GL_LoadShader("app0:shaders/rgba_alpha_f.gxp", RGBA_A, GL_TRUE);
	GL_LoadShader("app0:shaders/rgba_v.gxp", COLOR, GL_FALSE);
	GL_LoadShader("app0:shaders/vertex_v.gxp", VERTEX_ONLY, GL_FALSE);
	
	// Setting up programs
	for (i=0;i<9;i++){
		programs[i] = glCreateProgram();
		switch (i){
			case TEX2D_REPL:
				glAttachShader(programs[i], fs[REPLACE]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				break;
			case TEX2D_MODUL:
				glAttachShader(programs[i], fs[MODULATE]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				modulcolor[0] = glGetUniformLocation(programs[i], "vColor");
				break;
			case TEX2D_MODUL_CLR:
				glAttachShader(programs[i], fs[MODULATE_WITH_COLOR]);
				glAttachShader(programs[i], vs[TEXTURE2D_WITH_COLOR]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				vglBindAttribLocation(programs[i], 2, "color", 4, GL_FLOAT);
				break;
			case RGBA_COLOR:
				glAttachShader(programs[i], fs[RGBA_COLOR]);
				glAttachShader(programs[i], vs[COLOR]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "aColor", 4, GL_FLOAT);
				break;
			case NO_COLOR:
				glAttachShader(programs[i], fs[MONO_COLOR]);
				glAttachShader(programs[i], vs[VERTEX_ONLY]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				monocolor = glGetUniformLocation(programs[i], "color");
				break;
			case TEX2D_REPL_A:
				glAttachShader(programs[i], fs[REPLACE_A]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				break;
			case TEX2D_MODUL_A:
				glAttachShader(programs[i], fs[MODULATE_A]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				modulcolor[1] = glGetUniformLocation(programs[i], "vColor");
				break;
			case FULL_A:
				glAttachShader(programs[i], fs[MODULATE_COLOR_A]);
				glAttachShader(programs[i], vs[TEXTURE2D_WITH_COLOR]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				vglBindAttribLocation(programs[i], 2, "color", 4, GL_FLOAT);
				break;
			case RGBA_CLR_A:
				glAttachShader(programs[i], fs[RGBA_A]);
				glAttachShader(programs[i], vs[COLOR]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "aColor", 4, GL_FLOAT);
				break;
		}
		glLinkProgram(programs[i]);
	}
}

void VID_Shutdown(void)
{
}

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

	glClearColor (1,0,0,0);
	glCullFace(GL_FRONT);
	
	GL_ResetShaders();

	GL_EnableState(GL_ALPHA_TEST);
	GL_EnableState(GL_TEXTURE_COORD_ARRAY);
	//glAlphaFunc(GL_GREATER, 0.666);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	//->glShadeModel (GL_FLAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	GL_EnableState(GL_REPLACE);
	
	int i;
	indices = (uint16_t*)malloc(sizeof(uint16_t*)*MAX_INDICES);
	for (i=0;i<MAX_INDICES;i++){
		indices[i] = i;
	}
	
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	extern cvar_t gl_clear;

	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;

	vglStartRendering();
	vglIndexPointerMapped(indices);
}


void GL_EndRendering (void)
{
	vglStopRendering();
}


int ColorIndex[16] =
{
	0, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 199, 207, 223, 231
};

unsigned ColorPercent[16] =
{
	25, 51, 76, 102, 114, 127, 140, 153, 165, 178, 191, 204, 216, 229, 237, 247
};

static int ConvertTrueColorToPal( unsigned char *true_color, unsigned char *palette )
{
	int i;
	long min_dist;
	int min_index;
	long r, g, b;

	min_dist = 256 * 256 + 256 * 256 + 256 * 256;
	min_index = -1;
	r = ( long )true_color[0];
	g = ( long )true_color[1];
	b = ( long )true_color[2];

	for( i = 0; i < 256; i++ )
	{
		long palr, palg, palb, dist;
		long dr, dg, db;

		palr = palette[3*i];
		palg = palette[3*i+1];
		palb = palette[3*i+2];
		dr = palr - r;
		dg = palg - g;
		db = palb - b;
		dist = dr * dr + dg * dg + db * db;
		if( dist < min_dist )
		{
			min_dist = dist;
			min_index = i;
		}
	}
	return min_index;
}

void VID_SetPalette (unsigned char *palette)
{
	byte	*pal;
	int		r,g,b,v;
	int		i,c,p;
	unsigned	*table;

//
// 8 8 8 encoding
//
	pal = palette;
	table = d_8to24table;
	
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		
//		v = (255<<24) + (r<<16) + (g<<8) + (b<<0);
//		v = (255<<0) + (r<<8) + (g<<16) + (b<<24);
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
	}

	d_8to24table[255] &= 0xffffff;	// 255 is transparent

	pal = palette;
	table = d_8to24TranslucentTable;

	for (i=0; i<16;i++)
	{
		c = ColorIndex[i]*3;

		r = pal[c];
		g = pal[c+1];
		b = pal[c+2];

		for(p=0;p<16;p++)
		{
			v = (ColorPercent[15-p]<<24) + (r<<0) + (g<<8) + (b<<16);
			//v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
			*table++ = v;

			RTint[i*16+p] = ((float)r) / ((float)ColorPercent[15-p]) ;
			GTint[i*16+p] = ((float)g) / ((float)ColorPercent[15-p]);
			BTint[i*16+p] = ((float)b) / ((float)ColorPercent[15-p]);
		}
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
	extern	byte ramps[3][256];
	
//	VID_SetPalette (palette);

//	gammaworks = SetDeviceGammaRamp (maindc, ramps);
}

/*
===================
VID_Init
===================
*/
void	VID_Init (unsigned char *palette)
{
	int		i, existingmode;
	int		basenummodes, width, height, bpp, findbpp, done;
	byte	*ptmp;
	char	gldir[MAX_OSPATH];
	
	width = scr_width, height = scr_height;

	Cvar_RegisterVariable (&vid_mode);
	Cvar_RegisterVariable (&vid_wait);
	Cvar_RegisterVariable (&vid_nopageflip);
	Cvar_RegisterVariable (&_vid_wait_override);
	Cvar_RegisterVariable (&_vid_default_mode);
	Cvar_RegisterVariable (&_vid_default_mode_win);
	Cvar_RegisterVariable (&vid_config_x);
	Cvar_RegisterVariable (&vid_config_y);
	Cvar_RegisterVariable (&vid_stretch_by_2);
	Cvar_RegisterVariable (&_windowed_mouse);
	Cvar_RegisterVariable (&gl_ztrick);

	vid_initialized = true;

	vid.maxwarpwidth = width;
	vid.maxwarpheight = height;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.aspect = (float) width / (float) height;
	vid.numpages = 2;
	vid.rowbytes = 2 * width;
	vid.width = width;
	vid.height = height;

	vid.conwidth = width;
	vid.conheight = height;

	GL_Init ();

	sprintf (gldir, "%s/glhexen", com_gamedir);
	Sys_mkdir (gldir);
	sprintf (gldir, "%s/glhexen/boss", com_gamedir);
	Sys_mkdir (gldir);
	sprintf (gldir, "%s/glhexen/puzzle", com_gamedir);
	Sys_mkdir (gldir);

	VID_SetPalette (palette);
	
	Con_SafePrintf ("Video mode %dx%d initialized.\n", width, height);
	
	vid.recalc_refdef = 1; // force a surface cache flush
}

void D_ShowLoadingSize(void)
{
	/*if (!vid_initialized)
		return;

	glDrawBuffer  (GL_FRONT);

	SCR_DrawLoading();

	glDrawBuffer  (GL_BACK);*/
}
