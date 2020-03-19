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

#define GP_MAXBINDS 32

qboolean gl_set = false;
bool is_soft_render = false;

#ifdef HAVE_OPENGL
static bool libretro_shared_context = false;
#endif
extern cvar_t *sw_texfilt;

/* TODO/FIXME - should become float for better accuracy */
int      framerate    = 60;
unsigned framerate_ms = 16;

static bool context_needs_reinit = true;

#define RETRO_DEVICE_MODERN  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 2)
#define RETRO_DEVICE_JOYPAD_ALT  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

#define MAX_PADS 1
static unsigned quake_devices[1];
static int invert_y_axis = 1;

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
void ( APIENTRY * qglLineWidth )(GLfloat width);
void ( APIENTRY * qglFrontFace )(GLenum mode);

#define GL_FUNCS_NUM 46

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

void GL_DrawPolygon(GLenum prim, int num){
	qglDrawElements(prim, num, GL_UNSIGNED_SHORT, indices);
}

void vglVertexAttribPointerMapped(int id, void* ptr)
{
   switch (id)
   {
      case 0: /* Vertex */
         qglVertexPointer(3, GL_FLOAT, 0, ptr);
         break;
      case 1: /* TexCoord */
         qglTexCoordPointer(2, GL_FLOAT, 0, ptr);
         break;
      case 2: /* Color */
         qglColorPointer(4, GL_FLOAT, 0, ptr);
         break;
   }
}
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

/* System analog stick range is -0x8000 to 0x8000 */
#define ANALOG_RANGE 0x8000
/* Default deadzone: 15% */
static int analog_deadzone = (int)(0.15f * ANALOG_RANGE);

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
	funcs[44].ptr = qglLineWidth          = hw_render.get_proc_address ("glLineWidth");
	funcs[45].ptr = qglFrontFace          = hw_render.get_proc_address ("glFrontFace");
	
	if (log_cb) {
		int i;
		for (i = 0; i < GL_FUNCS_NUM; i++) {
			if (!funcs[i].ptr) log_cb(RETRO_LOG_ERROR, "vitaHexenII: cannot get GL function #%d symbol.\n", i);
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

static void audio_callback(void);

#define SAMPLE_RATE   	48000
#define BUFFER_SIZE 	2048

/* sys.c */

extern int old_char;
extern int isDanzeff;
extern uint64_t rumble_tick;
qboolean		isDedicated;

uint64_t initialTime = 0;
int hostInitialized = 0;
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
int filelength_internal (FILE *f)
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

	return filelength_internal(f);
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
	path_mkdir(path);
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
				Key_Event(K_UPARROW, 1);
			else
				Key_Event(K_UPARROW, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
				Key_Event(K_DOWNARROW, 1);
			else
				Key_Event(K_DOWNARROW, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
				Key_Event(K_LEFTARROW, 1);
			else
				Key_Event(K_LEFTARROW, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
				Key_Event(K_RIGHTARROW, 1);
			else
				Key_Event(K_RIGHTARROW, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_START))
				Key_Event(K_START, 1);
			else
				Key_Event(K_START, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT))
				Key_Event(K_SELECT, 1);
			else
				Key_Event(K_SELECT, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_Y))
				Key_Event(K_SQUARE, 1);
			else
				Key_Event(K_SQUARE, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_X))
				Key_Event(K_TRIANGLE, 1);
			else
				Key_Event(K_TRIANGLE, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_B))
				Key_Event(K_CROSS, 1);
			else
				Key_Event(K_CROSS, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_A))
				Key_Event(K_CIRCLE, 1);
			else
				Key_Event(K_CIRCLE, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_L))
				Key_Event(K_LEFTTRIGGER, 1);
			else
				Key_Event(K_LEFTTRIGGER, 0);
			if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_R))
				Key_Event(K_RIGHTTRIGGER, 1);
			else
				Key_Event(K_RIGHTTRIGGER, 0);
		}
		break;
		/*
		case RETRO_DEVICE_KEYBOARD:
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT))
				Key_Event(K_MOUSE1, 1);
			else
				Key_Event(K_MOUSE1, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT))
				Key_Event(K_MOUSE2, 1);
			else
				Key_Event(K_MOUSE2, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE))
				Key_Event(K_MOUSE3, 1);
			else
				Key_Event(K_MOUSE3, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP))
				Key_Event(K_MOUSE4, 1);
			else
				Key_Event(K_MOUSE4, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN))
				Key_Event(K_MOUSE5, 1);
			else
				Key_Event(K_MOUSE5, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP))
				Key_Event(K_MOUSE6, 1);
			else
				Key_Event(K_MOUSE6, 0);
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN))
				Key_Event(K_MOUSE7, 1);
			else
				Key_Event(K_MOUSE7, 0);
			if (quake_devices[0] == RETRO_DEVICE_KEYBOARD) {
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP))
					Key_Event(K_UPARROW, 1);
				else
					Key_Event(K_UPARROW, 0);
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN))
					Key_Event(K_DOWNARROW, 1);
				else
					Key_Event(K_DOWNARROW, 0);
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT))
					Key_Event(K_LEFTARROW, 1);
				else
					Key_Event(K_LEFTARROW, 0);
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT))
					Key_Event(K_RIGHTARROW, 1);
				else
					Key_Event(K_RIGHTARROW, 0);
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

static RDIR	*finddir = NULL;
static char		*findpath, *findpattern;

char *Sys_FindFirstFile (char *path, char *pattern)
{
	size_t	tmp_len;

	if (finddir != NULL)
		Sys_Error ("Sys_FindFirst without FindClose");

	finddir = retro_opendir(path);
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
	if (!finddir)
		return NULL;
	
	while ((retro_readdir(finddir)) > 0)
	{
	  if (!fnmatch_mod(findpattern, retro_dirent_get_name(finddir), FNM_PATHNAME))
      {
            return retro_dirent_get_name(finddir);
      }
	}

	return NULL;
}

void Sys_FindClose (void)
{
	if (finddir != NULL)
		retro_closedir(finddir);
	if (findpath != NULL)
		free (findpath);
	if (findpattern != NULL)
		free (findpattern);
	finddir = NULL;
	findpath = NULL;
	findpattern = NULL;
}

//=============================================================================
bool initial_resolution_set = false;
static void update_variables(bool startup)
{
	struct retro_variable var;

   var.key = "vitahexenii_framerate";
   var.value = NULL;

   if (startup)
   {
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
      {
         if (!strcmp(var.value, "auto"))
         {
            float target_framerate = 0.0f;
            if (!environ_cb(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE,
                     &target_framerate))
               target_framerate = 60.0f;
            framerate = (unsigned)target_framerate;
         }
         else
            framerate = atoi(var.value);
      }
      else
      {
         framerate    = 60;
         framerate_ms = 16;
      }

      switch (framerate)
      {
         case 50:
            framerate_ms = 20;
            break;
         case 60:
            framerate_ms = 16;
            break;
         case 72:
            framerate_ms = 14;
            break;
         case 75:
            framerate_ms = 13;
            break;
         case 90:
            framerate_ms = 11;
            break;
         case 100:
            framerate_ms = 10;
            break;
         case 119:
         case 120:
            framerate_ms = 8;
            break;
         case 144:
            framerate_ms = 7;
            break;
         case 155:
         case 160:
         case 165:
            framerate_ms = 6;
            break;
         case 180:
         case 200:
            framerate_ms = 5;
            break;
         case 240:
         case 244:
            framerate_ms = 4;
            break;
         case 300:
         case 360:
            framerate_ms = 3;
            break;
      }

      var.key = "vitahexenii_resolution";
      var.value = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && !initial_resolution_set)
      {
         char *pch;
         char str[100];
         snprintf(str, sizeof(str), "%s", var.value);

         pch = strtok(str, "x");
         if (pch)
            scr_width = strtoul(pch, NULL, 0);
         pch = strtok(NULL, "x");
         if (pch)
            scr_height = strtoul(pch, NULL, 0);

         if (log_cb)
            log_cb(RETRO_LOG_INFO, "Got size: %u x %u.\n", scr_width, scr_height);

         initial_resolution_set = true;
      }

/*#ifdef HAVE_OPENGL
      var.key = "vitaquakeii_renderer";
      var.value = NULL;

      enable_opengl = !environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || strcmp(var.value, "software") != 0;
#endif*/
   }
   
	var.key = "vitahexenii_invert_y_axis";
	var.value = NULL;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "disabled") == 0)
			invert_y_axis = 1;
		else
			invert_y_axis = -1;
	}
   
	/* We need Qcommon_Init to be executed to be able to set Cvars */
	if (!startup) {
		var.key = "vitahexenii_rumble";
		var.value = NULL;
	
		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "pstv_rumble", 0 );
			else
				Cvar_SetValue( "pstv_rumble", 1 );
		}
		
/*		var.key = "vitahexenii_dithered_filtering";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && sw_texfilt)
		{
			if (strcmp(var.value, "enabled") == 0)
				Cvar_SetValue( "sw_texfilt", 1 );
			else
				Cvar_SetValue( "sw_texfilt", 0);
		}*/
#ifdef HAVE_OPENGL	
		var.key = "vitahexenii_specular";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "gl_xflip", 0 );
			else
				Cvar_SetValue( "gl_xflip", 1 );
		}
#endif		
		var.key = "vitahexenii_xhair";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "crosshair", 0 );
			else
				Cvar_SetValue( "crosshair", 1 );
		}
		
		var.key = "vitahexenii_fps";
		var.value = NULL;

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "cl_drawfps", 0 );
			else
				Cvar_SetValue( "cl_drawfps", 1 );
		}
		
		var.key = "vitahexenii_shadows";
		var.value = NULL;
#ifdef HAVE_OPENGL
		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !is_soft_render)
		{
			if (strcmp(var.value, "disabled") == 0)
				Cvar_SetValue( "gl_shadows", 0 );
			else
				Cvar_SetValue( "gl_shadows", 1 );
		}
#endif		
	}

}

void retro_init(void)
{
   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;
}

void retro_deinit(void)
{
   libretro_supports_bitmasks = false;
}

static void extract_basename(char *buf, const char *path, size_t size)
{
   char *ext        = NULL;
   const char *base = strrchr(path, '/');
   if (!base)
      base = strrchr(path, '\\');
   if (!base)
      base = path;

   if (*base == '\\' || *base == '/')
      base++;

   strncpy(buf, base, size - 1);
   buf[size - 1] = '\0';

   ext = strrchr(buf, '.');
   if (ext)
      *ext = '\0';
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void gp_layout_set_bind(gp_layout_t gp_layout)
{
   char buf[100];
   unsigned i;
   for (i=0; gp_layout.bind[i].key; ++i)
   {
      snprintf(buf, sizeof(buf), "bind %s \"%s\"\n", gp_layout.bind[i].key,
                                                   gp_layout.bind[i].com);
      Cbuf_AddText(buf);
   }
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   if (port == 0)
   {
      switch (device)
      {
         case RETRO_DEVICE_JOYPAD:
            quake_devices[port] = RETRO_DEVICE_JOYPAD;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, classic.desc);
            gp_layout_set_bind(classic);
            break;
         case RETRO_DEVICE_JOYPAD_ALT:
            quake_devices[port] = RETRO_DEVICE_JOYPAD;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, classic_alt.desc);
            gp_layout_set_bind(classic_alt);
            break;
         case RETRO_DEVICE_MODERN:
            quake_devices[port] = RETRO_DEVICE_MODERN;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, modern.desc);
            gp_layout_set_bind(modern);
            break;
         case RETRO_DEVICE_KEYBOARD:
            quake_devices[port] = RETRO_DEVICE_KEYBOARD;
            break;
         case RETRO_DEVICE_NONE:
         default:
            quake_devices[port] = RETRO_DEVICE_NONE;
            if (log_cb)
               log_cb(RETRO_LOG_ERROR, "[libretro]: Invalid device.\n");
      }
   }
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "vitaHexenII";
   info->library_version  = "v2.2" ;
   info->need_fullpath    = true;
   info->valid_extensions = "pak";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps            = framerate;
   info->timing.sample_rate    = SAMPLE_RATE;

   info->geometry.base_width   = scr_width;
   info->geometry.base_height  = scr_height;
   info->geometry.max_width    = scr_width;
   info->geometry.max_height   = scr_height;
   info->geometry.aspect_ratio = (scr_width * 1.0f) / (scr_height * 1.0f);
}

void retro_set_environment(retro_environment_t cb)
{
   static const struct retro_controller_description port_1[] = {
      { "Gamepad Classic", RETRO_DEVICE_JOYPAD },
      { "Gamepad Classic Alt", RETRO_DEVICE_JOYPAD_ALT },
      { "Gamepad Modern", RETRO_DEVICE_MODERN },
      { "Keyboard + Mouse", RETRO_DEVICE_KEYBOARD },
   };

   static const struct retro_controller_info ports[] = {
      { port_1, 4 },
      { 0 },
   };

   environ_cb = cb;

   libretro_set_core_options(environ_cb);
   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

   struct retro_log_callback log;

   if(environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;
}

void retro_reset(void)
{
}

void retro_set_rumble_strong(void)
{
   uint16_t strength_strong = 0xffff;
   if (!rumble.set_rumble_state)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, strength_strong);
}

void retro_unset_rumble_strong(void)
{
   if (!rumble.set_rumble_state)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

static void extract_directory(char *buf, const char *path, size_t size)
{
   char *base = NULL;

   strncpy(buf, path, size - 1);
   buf[size - 1] = '\0';

   base = strrchr(buf, '/');
   if (!base)
      base = strrchr(buf, '\\');

   if (base)
      *base = '\0';
   else
    {
       buf[0] = '.';
       buf[1] = '\0';
    }
}

bool retro_load_game(const struct retro_game_info *info)
{
	int i;
	char path_lower[256];
#if defined(_WIN32)
	char slash = '\\';
#else
	char slash = '/';
#endif
	bool use_external_savedir = false;
	const char *base_save_dir = NULL;
#if 0
	struct retro_keyboard_callback cb = { keyboard_cb };
#endif
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;

	update_variables(true);

	if (/*enable_opengl &&*/ !environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
	{
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
		return false;
	}

	if (/*!enable_opengl*/
#ifdef HAVE_OPENGL
	    /*||*/ !initialize_opengl()
#endif
	    )
	{
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "vitaQuakeII: using software renderer.\n");
		
		fmt = RETRO_PIXEL_FORMAT_RGB565;
		if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
		{
			if (log_cb)
				log_cb(RETRO_LOG_INFO, "RGB565 is not supported.\n");
			return false;
		}
		is_soft_render = true;
	} else {
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "vitaQuakeII: using OpenGL renderer.\n");
	}
	

	if (!info)
		return false;
	
	sprintf(path_lower, "%s", info->path);
	
	for (i=0; path_lower[i]; ++i)
		path_lower[i] = tolower(path_lower[i]);
	
#if 0
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
#endif
	
	extract_directory(g_rom_dir, info->path, sizeof(g_rom_dir));
	
	snprintf(g_pak_path, sizeof(g_pak_path), "%s", info->path);
	
	if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &base_save_dir) && base_save_dir)
	{
		if (strlen(base_save_dir) > 0)
		{
			/* Get game 'name' (i.e. subdirectory) */
			char game_name[1024];
			extract_basename(game_name, g_rom_dir, sizeof(game_name));
			
			/* > Build final save path */
			snprintf(g_save_dir, sizeof(g_save_dir), "%s%c%s", base_save_dir, slash, game_name);
			use_external_savedir = true;
			
			/* > Create save directory, if required */
			if (!path_is_directory(g_save_dir))
				use_external_savedir = path_mkdir(g_save_dir);
		}
	}
	
	/* > Error check */
	if (!use_external_savedir)
	{
		/* > Use ROM directory fallback... */
		snprintf(g_save_dir, sizeof(g_save_dir), "%s", g_rom_dir);
	}
	else
	{
		/* > Final check: is the save directory the same as the 'rom' directory?
		 *   (i.e. ensure logical behaviour if user has set a bizarre save path...) */
		use_external_savedir = (strcmp(g_save_dir, g_rom_dir) != 0);
	}
	
	if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
		log_cb(RETRO_LOG_INFO, "Rumble environment supported.\n");
	else
		log_cb(RETRO_LOG_INFO, "Rumble environment not supported.\n");
	
	if (strstr(path_lower, "data1"))
		extract_directory(g_rom_dir, g_rom_dir, sizeof(g_rom_dir));

	return true;
}

bool first_boot = true;

static void audio_process(void)
{
}

void retro_run(void)
{
	bool updated = false;
#ifdef HAVE_OPENGL
	if (!is_soft_render) {
      if (!libretro_shared_context)
         glsm_ctl(GLSM_CTL_STATE_BIND, NULL);
		qglBindFramebuffer(RARCH_GL_FRAMEBUFFER, hw_render.get_current_framebuffer());
		qglEnable(GL_TEXTURE_2D);
	}
#endif
	if (first_boot)
	{
		static quakeparms_t    parms;
		
		parms.memsize = 20*1024*1024;
		parms.membase = malloc (parms.memsize);
		parms.basedir = ".";
		
		const char *argv[32];
		const char *empty_string = "";
	
		argv[0] = empty_string;
		int argc = 1;
		COM_InitArgv (argc, argv);
		
		parms.argc = com_argc;
		parms.argv = com_argv;
		
		Host_Init (&parms);
		hostInitialized = 1;
		
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
	
		update_variables(false);
		first_boot = false;
	}
	
	if (rumble_tick != 0)
		if (cpu_features_get_time_usec() - rumble_tick > 500000)
         IN_StopRumble(); /* 0.5 sec */
	
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
		update_variables(false);

	Host_Frame(1.0 / framerate);

	/*if (shutdown_core)
		return;*/

	/*if (is_soft_render) video_cb(tex_buffer, scr_width, scr_height, scr_width << 1);
	else*/ {
#ifdef HAVE_OPENGL
      if (!libretro_shared_context)
         glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);
		video_cb(RETRO_HW_FRAME_BUFFER_VALID, scr_width, scr_height, 0);
#endif
	}
	
	audio_process();
	audio_callback();
  
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
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

	audio_batch_cb(audio_buffer + audio_buffer_ptr, read_first / (shm->samplebits / 8));
	audio_buffer_ptr += read_first;
	if (read_second >= 1) {
		audio_batch_cb(audio_buffer, read_second / (shm->samplebits / 8));
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
	shm->buffer = audio_buffer;
	
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
	if (!pstv_rumble.value) return;
	
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
   
   if ((in_speed.state & 1) || always_run.value){
		cl_forwardspeed.value = 400;
		cl_backspeed.value = 400;
		cl_sidespeed.value = 700;
	}else{
		cl_forwardspeed.value = 200;
		cl_backspeed.value = 200;
		cl_sidespeed.value = 300;
	}
   
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
         if (gl_xflip.value)
            cmd->sidemove -= speed * cl_sidespeed.value * lsx / (ANALOG_RANGE - analog_deadzone);
         else
            cmd->sidemove += speed * cl_sidespeed.value * lsx / (ANALOG_RANGE - analog_deadzone);
      }

      if (lsy > analog_deadzone || lsy < -analog_deadzone) {
         if (lsy > analog_deadzone)
            lsy = lsy - analog_deadzone;
         if (lsy < -analog_deadzone)
            lsy = lsy + analog_deadzone;
         cmd->forwardmove -= speed * cl_forwardspeed.value * lsy / (ANALOG_RANGE - analog_deadzone);
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
         if (gl_xflip.value)
            cl.viewangles[YAW] += (float)(sensitivity.value * rsx / (ANALOG_RANGE - analog_deadzone)) / (framerate / 60.0f);
         else
            cl.viewangles[YAW] -= (float)(sensitivity.value * rsx / (ANALOG_RANGE - analog_deadzone)) / (framerate / 60.0f);
      }

      if (rsy > analog_deadzone || rsy < -analog_deadzone) {
         if (rsy > analog_deadzone)
            rsy = rsy - analog_deadzone;
         if (rsy < -analog_deadzone)
            rsy = rsy + analog_deadzone;
         cl.viewangles[PITCH] -= (float)(sensitivity.value * rsy / (ANALOG_RANGE - analog_deadzone)) / (framerate / 60.0f);
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
uint16_t *indices;

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

	qglClearColor (1,0,0,0);
	qglCullFace(GL_FRONT);
	
	qglEnable(GL_ALPHA_TEST);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	//glAlphaFunc(GL_GREATER, 0.666);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	//->glShadeModel (GL_FLAT);

	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	Cvar_RegisterVariable(&show_fps);
	Cvar_RegisterVariable(&vid_vsync);
	
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
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

	gVertexBuffer = gVertexBufferPtr;
	gColorBuffer = gColorBufferPtr;
	gTexCoordBuffer = gTexCoordBufferPtr;
	qglEnableClientState(GL_VERTEX_ARRAY);
}


void GL_EndRendering (void)
{
	GL_DrawFPS();
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

/* cd_null.c */

void CDAudio_Play(byte track, qboolean looping)
{
}


void CDAudio_Stop(void)
{
}


void CDAudio_Pause(void)
{
}


void CDAudio_Resume(void)
{
}


void CDAudio_Update(void)
{
}


int CDAudio_Init(void)
{
	return 0;
}


void CDAudio_Shutdown(void)
{
}
