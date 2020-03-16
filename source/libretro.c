#ifdef HAVE_OPENGL
#include <glsym/rglgen_private_headers.h>
#include <glsym/glsym.h>
#endif

#include <libretro.h>
#include <retro_dirent.h>
#include <features/features_cpu.h>
#include <file/file_path.h>

#include "libretro_core_options.h"

#include "quakedef.h"

#ifdef HAVE_OPENGL
#include <glsm/glsm.h>
#endif

#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include "errno.h"
#define u64 uint64_t
#include "sys.h"

#include "fnmatch.h"
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

/* in_psp2.c */

#include "quakedef.h"

#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)

// mouse variables
cvar_t	m_filter = {"m_filter","0"};
extern cvar_t always_run, inverted;

cvar_t pstv_rumble = {"pstv_rumble","1", true};
cvar_t retrotouch = {"retrotouch","0", true};
cvar_t always_run = {"always_run","0", true};
cvar_t inverted = {"invert_camera","0", true};
cvar_t motioncam = {"motioncam", "0", true};
cvar_t motion_horizontal_sensitivity = {"motioncam", "0", true};
cvar_t motion_vertical_sensitivity = {"motioncam", "0", true};

uint64_t rumble_tick = 0;
SceCtrlData oldanalogs, analogs;
SceMotionState motionstate;

void IN_Init (void)
{
  if ( COM_CheckParm ("-nomouse") )
    return;

  Cvar_RegisterVariable (&m_filter);
  Cvar_RegisterVariable (&retrotouch);
  Cvar_RegisterVariable (&always_run);
  Cvar_RegisterVariable (&inverted);
  Cvar_RegisterVariable (&pstv_rumble);
  Cvar_RegisterVariable (&motioncam);
  Cvar_RegisterVariable (&motion_horizontal_sensitivity);
  Cvar_RegisterVariable (&motion_vertical_sensitivity);
  
  sceMotionReset();
  sceMotionStartSampling();
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

void IN_RescaleAnalog(int *x, int *y, int dead) {
	//radial and scaled deadzone
	//http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html

	float analogX = (float) *x;
	float analogY = (float) *y;
	float deadZone = (float) dead;
	float maximum = 128.0f;
	float magnitude = sqrt(analogX * analogX + analogY * analogY);
	if (magnitude >= deadZone)
	{
		float scalingFactor = maximum / magnitude * (magnitude - deadZone) / (maximum - deadZone);		
		*x = (int) (analogX * scalingFactor);
		*y = (int) (analogY * scalingFactor);
	} else {
		*x = 0;
		*y = 0;
	}
}

void IN_StartRumble (void)
{
	if (!pstv_rumble->value) return;
	
	uint16_t strength_strong = 0xffff;
	if (!rumble.set_rumble_state)
		return;

	rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, strength_strong);
	rumble_tick = cpu_features_get_time_usec();
}

void IN_StopRumble (void)
{
	if (!rumble.set_rumble_state)
		return;

	rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0);
	rumble_tick = 0;
}

void IN_Move (usercmd_t *cmd)
{
	#if 0
   static int cur_mx;
   static int cur_my;
   int mx, my;
#endif
   int lsx, lsy, rsx, rsy;
   float speed;
   
   if ( (in_speed.state & 1) ^ (int)cl_run->value)
       speed = 2;
   else
       speed = 1;
   
   /*if (quake_devices[0] == RETRO_DEVICE_KEYBOARD) {
      mx = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      my = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      if (mx != cur_mx || my != cur_my)
      {
         mx *= sensitivity->value;
         my *= sensitivity->value;
         cl.viewangles[YAW] -= m_yaw->value * mx;
         cl.viewangles[PITCH] += m_pitch->value * my;
         if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
         if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
         cur_mx = mx;
         cur_my = my;
      }
   } else */if (quake_devices[0] != RETRO_DEVICE_NONE && quake_devices[0] != RETRO_DEVICE_KEYBOARD)
   {
      /* Left stick move */
      lsx = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
               RETRO_DEVICE_ID_ANALOG_X);
      lsy = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
               RETRO_DEVICE_ID_ANALOG_Y);

      if (lsx > analog_deadzone || lsx < -analog_deadzone) {
         if (lsx > analog_deadzone)
            lsx = lsx - analog_deadzone;
         if (lsx < -analog_deadzone)
            lsx = lsx + analog_deadzone;
         if (gl_xflip->value)
            cmd->sidemove -= speed * cl_sidespeed->value * lsx / (ANALOG_RANGE - analog_deadzone);
         else
            cmd->sidemove += speed * cl_sidespeed->value * lsx / (ANALOG_RANGE - analog_deadzone);
      }

      if (lsy > analog_deadzone || lsy < -analog_deadzone) {
         if (lsy > analog_deadzone)
            lsy = lsy - analog_deadzone;
         if (lsy < -analog_deadzone)
            lsy = lsy + analog_deadzone;
         cmd->forwardmove -= speed * cl_forwardspeed->value * lsy / (ANALOG_RANGE - analog_deadzone);
      }

      /* Right stick Look */
      rsx = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
               RETRO_DEVICE_ID_ANALOG_X);
      rsy = invert_y_axis * input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
               RETRO_DEVICE_ID_ANALOG_Y);

      if (rsx > analog_deadzone || rsx < -analog_deadzone)
      {
         if (rsx > analog_deadzone)
            rsx = rsx - analog_deadzone;
         if (rsx < -analog_deadzone)
            rsx = rsx + analog_deadzone;
         /* For now we are sharing the sensitivity with the mouse setting */
         if (gl_xflip->value)
            cl.viewangles[YAW] += (float)(sensitivity->value * rsx / (ANALOG_RANGE - analog_deadzone)) / (framerate / 60.0f);
         else
            cl.viewangles[YAW] -= (float)(sensitivity->value * rsx / (ANALOG_RANGE - analog_deadzone)) / (framerate / 60.0f);
      }

      if (rsy > analog_deadzone || rsy < -analog_deadzone) {
         if (rsy > analog_deadzone)
            rsy = rsy - analog_deadzone;
         if (rsy < -analog_deadzone)
            rsy = rsy + analog_deadzone;
         cl.viewangles[PITCH] -= (float)(sensitivity->value * rsy / (ANALOG_RANGE - analog_deadzone)) / (framerate / 60.0f);
      }

      if (cl.viewangles[PITCH] > 80)
         cl.viewangles[PITCH] = 80;
      if (cl.viewangles[PITCH] < -70)
         cl.viewangles[PITCH] = -70;
   }
}

/* gl_vidpsp2.c */

#include <stdarg.h>
#include <stdio.h>
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

float *gVertexBuffer;
float *gColorBuffer;
float *gTexCoordBuffer;
float *gVertexBufferPtr;
float *gColorBufferPtr;
float *gTexCoordBufferPtr;

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
cvar_t		show_fps = {"show_fps", "0", true};
cvar_t		gl_outline = {"gl_outline", "0", true};

extern cvar_t vid_vsync;

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

void GL_LoadShader(const char* filename, GLuint idx, GLboolean fragment){
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
GLint monocolor;
GLint modulcolor[2];

void GL_SetProgram(){
	switch (state_mask){
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
			state_mask |= 0x01;
			break;
		case GL_COLOR_ARRAY:
			state_mask |= 0x02;
			break;
		case GL_MODULATE:
			state_mask |= 0x04;
			break;
		case GL_REPLACE:
			state_mask &= ~0x04;
			break;
		case GL_ALPHA_TEST:
			state_mask |= 0x08;
			break;
	}
	GL_SetProgram();
}

void GL_DisableState(GLenum state){	
	switch (state){
		case GL_TEXTURE_COORD_ARRAY:
			state_mask &= ~0x01;
			break;
		case GL_COLOR_ARRAY:
			state_mask &= ~0x02;
			break;
		case GL_ALPHA_TEST:
			state_mask &= ~0x08;
			break;
		default:
			break;
	}
	GL_SetProgram();
}

static float cur_clr[4];

void GL_DrawPolygon(GLenum prim, int num){
	if (state_mask == 0x05) glUniform4fv(modulcolor[0], 1, cur_clr);
	else if (state_mask == 0x0D) glUniform4fv(modulcolor[1], 1, cur_clr);
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

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	Cvar_RegisterVariable(&show_fps);
	Cvar_RegisterVariable(&vid_vsync);
	
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	GL_EnableState(GL_REPLACE);
	
	int i;
	indices = (uint16_t*)malloc(sizeof(uint16_t)*MAX_INDICES);
	for (i=0;i<MAX_INDICES;i++){
		indices[i] = i;
	}
	gVertexBufferPtr = (float*)malloc(0x400000);
	gColorBufferPtr = (float*)malloc(0x200000);
	gTexCoordBufferPtr = (float*)malloc(0x200000);
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;

	vglStartRendering();
	vglIndexPointerMapped(indices);
	gVertexBuffer = gVertexBufferPtr;
	gColorBuffer = gColorBufferPtr;
	gTexCoordBuffer = gTexCoordBufferPtr;
}


void GL_EndRendering (void)
{
	GL_DrawFPS();
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
	int		width, height;
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
	Cvar_RegisterVariable (&gl_outline);

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