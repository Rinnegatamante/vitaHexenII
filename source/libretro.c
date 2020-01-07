#ifdef HAVE_OPENGL
#include <glsym/rglgen_private_headers.h>
#include <glsym/glsym.h>
#endif

#include <libretro.h>
#include <retro_dirent.h>
#include <features/features_cpu.h>
#include <file/file_path.h>

#include "libretro_core_options.h"

#include "../client/client.h"
#include "../client/qmenu.h"

#ifdef HAVE_OPENGL
#include "../ref_gl/gl_local.h"

#include <glsm/glsm.h>
#endif

#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include "errno.h"
#define u64 uint64_t
#include "sys.h"
#include "quakedef.h"
#include "fnmatch_mod.h"
#include <features/features_cpu.h>

qboolean gl_set = false;
bool is_soft_render = false;

#ifdef HAVE_OPENGL
static bool libretro_shared_context = false;
#endif
extern cvar_t *sw_texfilt;

/* TODO/FIXME - should become float for better accuracy */
int      framerate    = 60;
unsigned framerate_ms = 16;

#ifdef HAVE_OPENGL
void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglBindFramebuffer )(GLenum target, GLuint framebuffer);
void ( APIENTRY * qglGenerateMipmap )(GLenum target);
void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglDepthMask )(GLboolean flag);
void ( APIENTRY * qglPushMatrix )(void);
void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( APIENTRY * qglClear )(GLbitfield mask);
void ( APIENTRY * qglEnable )(GLenum cap);
void ( APIENTRY * qglDisable )(GLenum cap);
void ( APIENTRY * qglPopMatrix )(void);
void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat *params);
void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
void ( APIENTRY * qglLoadIdentity )(void);
void ( APIENTRY * qglMatrixMode )(GLenum mode);
void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY * qglCullFace )(GLenum mode);
void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY * qglClearStencil )(GLint s);
void ( APIENTRY * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglEnableClientState )(GLenum array);
void ( APIENTRY * qglDisableClientState )(GLenum array);
void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglDepthFunc )(GLenum func);
void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglAlphaFunc )(GLenum func,  GLclampf ref);

#define GL_FUNCS_NUM 44

typedef struct api_entry{
	void *ptr;
} api_entry;

api_entry funcs[GL_FUNCS_NUM];
#endif

char g_rom_dir[1024], g_pak_path[1024], g_save_dir[1024];

#ifdef HAVE_OPENGL
extern struct retro_hw_render_callback hw_render;

#define MAX_INDICES 4096
uint16_t* indices;

float *gVertexBufferPtr;
float *gColorBufferPtr;
float *gTexCoordBufferPtr;
#endif

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
static retro_input_poll_t poll_cb;
static retro_input_state_t input_cb;
static struct retro_rumble_interface rumble;
static bool libretro_supports_bitmasks = false;

static bool initialize_gl()
{
	funcs[0].ptr  = qglTexImage2D         = hw_render.get_proc_address ("glTexImage2D");
	funcs[1].ptr  = qglTexSubImage2D      = hw_render.get_proc_address ("glTexSubImage2D");
	funcs[2].ptr  = qglTexParameteri      = hw_render.get_proc_address ("glTexParameteri");
	funcs[3].ptr  = qglBindFramebuffer    = hw_render.get_proc_address ("glBindFramebuffer");
	funcs[4].ptr  = qglGenerateMipmap     = hw_render.get_proc_address ("glGenerateMipmap");
	funcs[5].ptr  = qglBlendFunc          = hw_render.get_proc_address ("glBlendFunc");
	funcs[6].ptr  = qglTexSubImage2D      = hw_render.get_proc_address ("glTexSubImage2D");
	funcs[7].ptr  = qglDepthMask          = hw_render.get_proc_address ("glDepthMask");
	funcs[8].ptr  = qglPushMatrix         = hw_render.get_proc_address ("glPushMatrix");
	funcs[9].ptr  = qglRotatef            = hw_render.get_proc_address ("glRotatef");
	funcs[10].ptr = qglTranslatef         = hw_render.get_proc_address ("glTranslatef");
	funcs[11].ptr = qglDepthRange         = hw_render.get_proc_address ("glDepthRange");
	funcs[12].ptr = qglClear              = hw_render.get_proc_address ("glClear");
	funcs[13].ptr = qglCullFace           = hw_render.get_proc_address ("glCullFace");
	funcs[14].ptr = qglClearColor         = hw_render.get_proc_address ("glClearColor");
	funcs[15].ptr = qglEnable             = hw_render.get_proc_address ("glEnable");
	funcs[16].ptr = qglDisable            = hw_render.get_proc_address ("glDisable");
	funcs[17].ptr = qglEnableClientState  = hw_render.get_proc_address ("glEnableClientState");
	funcs[18].ptr = qglDisableClientState = hw_render.get_proc_address ("glDisableClientState");
	funcs[19].ptr = qglPopMatrix          = hw_render.get_proc_address ("glPopMatrix");
	funcs[20].ptr = qglGetFloatv          = hw_render.get_proc_address ("glGetFloatv");
	funcs[21].ptr = qglOrtho              = hw_render.get_proc_address ("glOrtho");
	funcs[22].ptr = qglFrustum            = hw_render.get_proc_address ("glFrustum");
	funcs[23].ptr = qglLoadMatrixf        = hw_render.get_proc_address ("glLoadMatrixf");
	funcs[24].ptr = qglLoadIdentity       = hw_render.get_proc_address ("glLoadIdentity");
	funcs[25].ptr = qglMatrixMode         = hw_render.get_proc_address ("glMatrixMode");
	funcs[26].ptr = qglBindTexture        = hw_render.get_proc_address ("glBindTexture");
	funcs[27].ptr = qglReadPixels         = hw_render.get_proc_address ("glReadPixels");
	funcs[28].ptr = qglPolygonMode        = hw_render.get_proc_address ("glPolygonMode");
	funcs[29].ptr = qglVertexPointer      = hw_render.get_proc_address ("glVertexPointer");
	funcs[30].ptr = qglTexCoordPointer    = hw_render.get_proc_address ("glTexCoordPointer");
	funcs[31].ptr = qglColorPointer       = hw_render.get_proc_address ("glColorPointer");
	funcs[32].ptr = qglDrawElements       = hw_render.get_proc_address ("glDrawElements");
	funcs[33].ptr = qglViewport           = hw_render.get_proc_address ("glViewport");
	funcs[34].ptr = qglDeleteTextures     = hw_render.get_proc_address ("glDeleteTextures");
	funcs[35].ptr = qglClearStencil       = hw_render.get_proc_address ("glClearStencil");
	funcs[36].ptr = qglColor4f            = hw_render.get_proc_address ("glColor4f");
	funcs[37].ptr = qglScissor            = hw_render.get_proc_address ("glScissor");
	funcs[38].ptr = qglStencilFunc        = hw_render.get_proc_address ("glStencilFunc");
	funcs[39].ptr = qglStencilOp          = hw_render.get_proc_address ("glStencilOp");
	funcs[40].ptr = qglScalef             = hw_render.get_proc_address ("glScalef");
	funcs[41].ptr = qglDepthFunc          = hw_render.get_proc_address ("glDepthFunc");
	funcs[42].ptr = qglTexEnvi            = hw_render.get_proc_address ("glTexEnvi");
	funcs[43].ptr = qglAlphaFunc          = hw_render.get_proc_address ("glAlphaFunc");
	
	if (log_cb) {
		int i;
		for (i = 0; i < GL_FUNCS_NUM; i++) {
			if (!funcs[i].ptr) log_cb(RETRO_LOG_ERROR, "vitaQuakeII: cannot get GL function #%d symbol.\n", i);
		}
	}
	
	return true;
}

static void context_destroy(void) 
{
	context_needs_reinit = true;
}

bool first_reset = true;

static void context_reset(void)
{
	if (!context_needs_reinit)
		return;
#ifdef HAVE_OPENGL
	glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, NULL);

   if (!libretro_shared_context)
      if (!glsm_ctl(GLSM_CTL_STATE_SETUP, NULL))
         return;
	
	if (!is_soft_render) {
		initialize_gl();
		//if (!first_reset)
		//	restore_textures();
		first_reset = false;
	}
#endif
	context_needs_reinit = false;
}

#ifdef HAVE_OPENGL
static bool context_framebuffer_lock(void *data)
{
    return false;
}

bool initialize_opengl(void)
{
   glsm_ctx_params_t params = {0};

   params.context_type     = RETRO_HW_CONTEXT_OPENGL;
   params.context_reset    = context_reset;
   params.context_destroy  = context_destroy;
   params.environ_cb       = environ_cb;
   params.stencil          = true;
   params.framebuffer_lock = context_framebuffer_lock;

   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params))
   {
      log_cb(RETRO_LOG_ERROR, "Could not setup glsm.\n");
      return false;
   }

   if (environ_cb(RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT, NULL))
      libretro_shared_context = true;
   else
      libretro_shared_context = false;

   return true;
}

void destroy_opengl(void)
{
   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_DESTROY, NULL))
   {
      log_cb(RETRO_LOG_ERROR, "Could not destroy glsm context.\n");
   }

   libretro_shared_context = false;
}
#endif

typedef struct {
   struct retro_input_descriptor desc[GP_MAXBINDS];
   struct {
      char *key;
      char *com;
   } bind[GP_MAXBINDS];
} gp_layout_t;

gp_layout_t modern = {
   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Swim Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Strafe Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Strafe Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Swim Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Previous Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Next Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Show Scores" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+moveleft"},     {"JOY_RIGHT", "+moveright"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+movedown"},     {"JOY_A",     "+moveright"},
      {"JOY_X",     "+moveup"},       {"JOY_Y",     "+moveleft"},
      {"JOY_L",     "impulse 12"},    {"JOY_R",     "impulse 10"},
      {"JOY_L2",    "+jump"},         {"JOY_R2",    "+attack"},
      {"JOY_SELECT","+showscores"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};
gp_layout_t classic = {

   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Cycle Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Freelook" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Strafe Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Strafe Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Look Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Look Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "Move Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "Swim Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Toggle Run Mode" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+left"},         {"JOY_RIGHT", "+right"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+jump"} ,        {"JOY_A",     "impulse 10"},
      {"JOY_X",     "+klook"},        {"JOY_Y",     "+attack"},
      {"JOY_L",     "+moveleft"},     {"JOY_R",     "+moveright"},
      {"JOY_L2",    "+lookup"},       {"JOY_R2",    "+lookdown"},
      {"JOY_L3",    "+movedown"},     {"JOY_R3",    "+moveup"},
      {"JOY_SELECT","+togglewalk"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};
gp_layout_t classic_alt = {

   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Look Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Look Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Look Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Look Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Run" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Next Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "Move Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "Previous Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Toggle Run Mode" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+moveleft"},     {"JOY_RIGHT", "+moveright"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+lookdown"},     {"JOY_A",     "+right"},
      {"JOY_X",     "+lookup"},       {"JOY_Y",     "+left"},
      {"JOY_L",     "+jump"},         {"JOY_R",     "+attack"},
      {"JOY_L2",    "+speed"},          {"JOY_R2",    "impulse 10"},
      {"JOY_L3",    "+movedown"},     {"JOY_R3",    "impulse 12"},
      {"JOY_SELECT","+togglewalk"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};

gp_layout_t *gp_layoutp = NULL;

/* sys.c */

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
}

void Sys_Error (char *error, ...)
{
	
	va_list         argptr;

	char buf[256];
	va_start(argptr, error);
	vsnprintf(buf, sizeof(buf), error, argptr);
	va_end(argptr);
	printf("%s\n", buf);
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
	return cpu_features_get_time_usec() * 0.001;
}

void Sys_SendKeyEvents (void)
{
	int port;

	if (!poll_cb)
		return;

	poll_cb();

	if (!input_cb)
		return;

	for (port = 0; port < MAX_PADS; port++)
	{
		if (!input_cb)
			break;

		switch (quake_devices[port])
		{
		case RETRO_DEVICE_JOYPAD:
		case RETRO_DEVICE_JOYPAD_ALT:
		case RETRO_DEVICE_MODERN:
		{
			unsigned i;
			int16_t ret    = 0;
			if (libretro_supports_bitmasks)
				ret = input_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
			else
			{
				for (i=RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R3; ++i)
				{
					if (input_cb(port, RETRO_DEVICE_JOYPAD, 0, i))
						ret |= (1 << i);
				}
			}

			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
				Sys_SetKeys(K_UPARROW, 1);
			else
				Sys_SetKeys(K_UPARROW, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
				Sys_SetKeys(K_DOWNARROW, 1);
			else
				Sys_SetKeys(K_DOWNARROW, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
				Sys_SetKeys(K_LEFTARROW, 1);
			else
				Sys_SetKeys(K_LEFTARROW, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
				Sys_SetKeys(K_RIGHTARROW, 1);
			else
				Sys_SetKeys(K_RIGHTARROW, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_START))
				Sys_SetKeys(K_ESCAPE, 1);
			else
				Sys_SetKeys(K_ESCAPE, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT))
				Sys_SetKeys(K_ENTER, 1);
			else
				Sys_SetKeys(K_ENTER, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_Y))
				Sys_SetKeys(K_AUX3, 1);
			else
				Sys_SetKeys(K_AUX3, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_X))
				Sys_SetKeys(K_AUX4, 1);
			else
				Sys_SetKeys(K_AUX4, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_B))
			{
				Sys_SetKeys(K_AUX1, 1);
			}
			else
			{
				Sys_SetKeys(K_AUX1, 0);
			}
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_A))
				Sys_SetKeys(K_AUX2, 1);
			else
				Sys_SetKeys(K_AUX2, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_L))
				Sys_SetKeys(K_AUX5, 1);
			else
				Sys_SetKeys(K_AUX5, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_R))
				Sys_SetKeys(K_AUX7, 1);
			else
				Sys_SetKeys(K_AUX7, 0);
		}
		break;
		/*
		case RETRO_DEVICE_KEYBOARD:
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT))
				Sys_SetKeys(K_MOUSE1, 1);
			else
				Sys_SetKeys(K_MOUSE1, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT))
				Sys_SetKeys(K_MOUSE2, 1);
			else
				Sys_SetKeys(K_MOUSE2, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE))
				Sys_SetKeys(K_MOUSE3, 1);
			else
				Sys_SetKeys(K_MOUSE3, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP))
				Sys_SetKeys(K_MOUSE4, 1);
			else
				Sys_SetKeys(K_MOUSE4, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN))
				Sys_SetKeys(K_MOUSE5, 1);
			else
				Sys_SetKeys(K_MOUSE5, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP))
				Sys_SetKeys(K_MOUSE6, 1);
			else
				Sys_SetKeys(K_MOUSE6, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN))
				Sys_SetKeys(K_MOUSE7, 1);
			else
				Sys_SetKeys(K_MOUSE7, 0);
			if (quake_devices[0] == RETRO_DEVICE_KEYBOARD) {
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP))
					Sys_SetKeys(K_UPARROW, 1);
				else
					Sys_SetKeys(K_UPARROW, 0);
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN))
					Sys_SetKeys(K_DOWNARROW, 1);
				else
					Sys_SetKeys(K_DOWNARROW, 0);
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT))
					Sys_SetKeys(K_LEFTARROW, 1);
				else
					Sys_SetKeys(K_LEFTARROW, 0);
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT))
					Sys_SetKeys(K_RIGHTARROW, 1);
				else
					Sys_SetKeys(K_RIGHTARROW, 0);
			}
			break;
		*/
		case RETRO_DEVICE_NONE:
			break;
		}
	}
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

/* snddma.c */
#include <stdio.h>

#define u8 uint8_t

#define SAMPLE_RATE   48000
#define AUDIOSIZE   16384

int snd_inited;
int chn = -1;
int stop_audio = false;
//int update = false;

uint64_t initial_tick;

static int16_t audio_buffer[BUFFER_SIZE];
static unsigned audio_buffer_ptr;

static void audio_callback(void)
{
	unsigned read_first, read_second;
	float samples_per_frame = (2 * SAMPLE_RATE) / framerate;
	unsigned read_end       = audio_buffer_ptr + samples_per_frame;

	if (read_end > BUFFER_SIZE)
		read_end = BUFFER_SIZE;

	read_first  = read_end - audio_buffer_ptr;
	read_second = samples_per_frame - read_first;

	audio_batch_cb(audio_buffer + audio_buffer_ptr, read_first / (dma.samplebits / 8));
	audio_buffer_ptr += read_first;
	if (read_second >= 1) {
		audio_batch_cb(audio_buffer, read_second / (dma.samplebits / 8));
		audio_buffer_ptr = read_second;
	}
}

qboolean SNDDMA_Init(void)
{
	snd_initialized = 0;

	/* Fill the audio DMA information block */
	shm = &sn;
	shm->splitbuffer = 0;
	shm->samplebits = 16;
	shm->speed = SAMPLE_RATE;
	shm->channels = 1;
	shm->samples = BUFFER_SIZE;
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = audiobuffer;
	
	initial_tick         = cpu_features_get_time_usec();

	snd_initialized = 1;
	return 1;
}

int SNDDMA_GetDMAPos(void)
{
	if(!snd_initialized)
		return 0;

	return shm->samplepos * audio_buffer_ptr;
}

void SNDDMA_Shutdown(void)
{
  if(snd_initialized){
	stop_audio = true;
	chn = -1;
  }
}

/*
==============
SNDDMA_Submit
Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
  //if(snd_initialized)
	//update = true;
}