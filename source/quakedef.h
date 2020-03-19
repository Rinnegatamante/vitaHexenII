// quakedef.h -- primary header for client

/*
 * $Header: /H2 Mission Pack/Quakedef.h 8     3/19/98 12:53p Jmonroe $
 */

//#define	GLTEST			// experimental stuff

#ifndef _QUAKEDEF_H_
#define _QUAKEDEF_H_

#define	QUAKE_GAME			// as opposed to utilities

#define HEXEN2_VERSION		1.12

//define	PARANOID			// speed sapping error checking

#ifdef __LIBRETRO__
#include <glsym/rglgen_private_headers.h>
#else
#include <GL/gl.h>
#endif

#ifdef QUAKE2
#define	GAMENAME	"data1"		// directory to look in by default
#else
#define	GAMENAME	"data1"
#endif

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__	1
#endif

void	VID_LockBuffer (void);
void	VID_UnlockBuffer (void);

#else

#define	VID_LockBuffer()
#define	VID_UnlockBuffer()

#endif

#if defined __i386__ // && !defined __sun__
#define id386	0
#else
#define id386	0
#endif

#if id386
#define UNALIGNED_OK	1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK	0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

#define	MINIMUM_MEMORY			0x550000
#define	MINIMUM_MEMORY_LEVELPAK	(MINIMUM_MEMORY + 0x100000)

#define MAX_NUM_ARGVS	50

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


// Timing macros
#define HX_FRAME_TIME		0.05
#define HX_FPS				20


#define	MAX_QPATH		64			// max length of a quake game pathname
#define	MAX_OSPATH		128			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon

//#define	MAX_MSGLEN		8000		// max length of a reliable message
//#define	MAX_MSGLEN		16000		// max length of a reliable message
#define	MAX_MSGLEN		20000		// for mission pack tibet2

#define	MAX_DATAGRAM	1024		// max length of unreliable message
//#define	MAX_DATAGRAM	2048		// max length of unreliable message  TEMP: This only for E3

//
// per-level limits
//
//#define       MAX_EDICTS      600     // FIXME: ouch! ouch! ouch!
#define	MAX_EDICTS	600	// FIXME: Arbitrary increase, make dynamic?
#define	MAX_LIGHTSTYLES	64

#define	MAX_MODELS		512			// Sent over the net as a word

#define	MAX_SOUNDS		512			// Sent over the net as a byte


#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		32
#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster

#define	MAX_INVENTORY			15		// Max inventory array size

// stock defines

#define	IT_SHOTGUN				1
#define	IT_SUPER_SHOTGUN		2
#define	IT_NAILGUN				4
#define	IT_SUPER_NAILGUN		8
#define	IT_GRENADE_LAUNCHER		16
#define	IT_ROCKET_LAUNCHER		32
#define	IT_LIGHTNING			64
#define IT_SUPER_LIGHTNING      128
#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048
#define IT_AXE                  4096
#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768
#define IT_SUPERHEALTH          65536
#define IT_KEY1                 131072
#define IT_KEY2                 262144
#define	IT_INVISIBILITY			524288
#define	IT_INVULNERABILITY		1048576
#define	IT_SUIT					2097152
#define	IT_QUAD					4194304
#define IT_SIGIL1               (1<<28)
#define IT_SIGIL2               (1<<29)
#define IT_SIGIL3               (1<<30)
#define IT_SIGIL4               (1<<31)

#define ART_HASTE					1
#define ART_INVINCIBILITY			2
#define ART_TOMEOFPOWER				4
#define ART_INVISIBILITY			8
#define ARTFLAG_FROZEN				128
#define ARTFLAG_STONED				256
#define ARTFLAG_DIVINE_INTERVENTION 512

//===========================================

#ifdef NO_PRAVEUS
#define NUM_CLASSES					4
#else
#define NUM_CLASSES					5
#endif
#define ABILITIES_STR_INDEX			400

#ifdef DEMOBUILD
	#define	MAX_SCOREBOARD		8
#else
	#define	MAX_SCOREBOARD		16
#endif

#define	MAX_SCOREBOARDNAME	32

#define	SOUND_CHANNELS		8

// This makes anyone on id's net privileged
// Use for multiplayer testing only - VERY dangerous!!!
// #define IDGODS

#include "common.h"
#include "bspfile.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"
#include "mathlib.h"

typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;

#if RJNET
	byte	ClearCount[32];
#endif
} entity_state_t;

typedef struct
{
	byte	flags;
	short	index;

	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;
} entity_state2_t;

typedef struct
{
	byte	flags;

	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;
} entity_state3_t;

#define MAX_CLIENT_STATES 150
#define MAX_FRAMES 5
#define MAX_CLIENTS 8
#define CLEAR_LIMIT 2

#define ENT_STATE_ON		1
#define ENT_CLEARED			2

typedef struct 
{
	entity_state2_t states[MAX_CLIENT_STATES];
//	unsigned long frame;
//	unsigned long flags;
	int count;
} client_frames_t;

typedef struct 
{
	entity_state2_t states[MAX_CLIENT_STATES*2];
	int count;
} client_frames2_t;

typedef struct
{
	client_frames_t frames[MAX_FRAMES+2]; // 0 = base, 1-max = proposed, max+1 = too late
} client_state2_t;


#include "wad.h"
#include "draw.h"
#include "cvar.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "render.h"
#include "cl_effect.h"
#include "progs.h"
#include "client.h"
#include "server.h"

#ifdef GLQUAKE
#include "gl_model.h"
#else
#include "model.h"
#include "d_iface.h"
#endif

#include "input.h"
#include "world.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "crc.h"
#include "cdaudio.h"

#ifdef GLQUAKE
#include "glquake.h"
#endif

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char	*basedir;
	char	*cachedir;		// for development over ISDN lines
	int	argc;
	char	**argv;
	void	*membase;
	int	memsize;
} quakeparms_t;


//=============================================================================



extern qboolean noclip_anglehack;


//
// host
//
extern	quakeparms_t host_parms;

extern	cvar_t		sys_ticrate;
extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;

extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	byte		*host_basepal;
extern	byte		*host_colormap;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset

void Host_ClearMemory (void);
void Host_ServerFrame (void);
void Host_InitCommands (void);
void Host_Init (quakeparms_t *parms);
void Host_Shutdown(void);
void Host_Error (char *error, ...);
void Host_EndGame (char *message, ...);
void Host_Frame (float time);
void Host_Quit_f (void);
void Host_ClientCommands (char *fmt, ...);
void Host_ShutdownServer (qboolean crash);

extern qboolean		msg_suppress_1;		// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss
extern int			current_skill;		// skill level for currently loaded level (in case
										//  the user changes the cvar while the level is
										//  running, this reflects the level actually in use)

extern qboolean		isDedicated;

extern int			minimum_memory;

extern int		sv_kingofhill;
extern qboolean		intro_playing;
extern qboolean		skip_start;
extern int		num_intro_msg;
extern qboolean		check_bottom;
//
// chase
//
extern	cvar_t	chase_active;

void Chase_Init (void);
void Chase_Reset (void);
void Chase_Update (void);

void DrawQuad_NoTex(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void DrawQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat u, GLfloat v, GLfloat uw, GLfloat vh);

extern float *gVertexBuffer;
extern float *gColorBuffer;
extern float *gTexCoordBuffer;

// Fragment shaders
#define MODULATE_WITH_COLOR  0
#define MODULATE             1
#define REPLACE              2
#define MONO_COLOR           4
#define MODULATE_COLOR_A     5
#define MODULATE_A           6
#define RGBA_A               7
#define REPLACE_A            8

// Vertex shaders
#define TEXTURE2D            0
#define TEXTURE2D_WITH_COLOR 1
#define COLOR                2
#define VERTEX_ONLY          3

// Shader programs
#define TEX2D_REPL      0
#define TEX2D_MODUL     1
#define TEX2D_MODUL_CLR 2
#define RGBA_COLOR      3
#define NO_COLOR        4
#define TEX2D_REPL_A    5
#define TEX2D_MODUL_A   6
#define RGBA_CLR_A      7
#define FULL_A          8

extern GLuint fs[9];
extern GLuint vs[4];
extern GLuint programs[9];
extern GLint monocolor;
extern GLint modulcolor[2];

void GL_EnableState(GLenum state);
void GL_DisableState(GLenum state);
void GL_DrawPolygon(GLenum prim, int num);
void GL_Color(float r, float g, float b, float a);
void GL_ResetShaders();
void GL_DrawFPS(void);

int M_DrawBigCharacter (int x, int y, int num, int numNext);
void GL_Set2D (void);

extern float sintablef[17];
extern float costablef[17];

extern  void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
extern  void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
extern  void ( APIENTRY * qglBindFramebuffer )(GLenum target, GLuint framebuffer);
extern  void ( APIENTRY * qglGenerateMipmap )(GLenum target);
extern  void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( APIENTRY * qglDepthMask )(GLboolean flag);
extern  void ( APIENTRY * qglPushMatrix )(void);
extern  void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
extern  void ( APIENTRY * qglClear )(GLbitfield mask);
extern  void ( APIENTRY * qglEnable )(GLenum cap);
extern  void ( APIENTRY * qglDisable )(GLenum cap);
extern  void ( APIENTRY * qglPopMatrix )(void);
extern  void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat *params);
extern  void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern  void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern  void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
extern  void ( APIENTRY * qglLoadIdentity )(void);
extern  void ( APIENTRY * qglMatrixMode )(GLenum mode);
extern  void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
extern  void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern  void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
extern  void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern  void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern  void ( APIENTRY * qglCullFace )(GLenum mode);
extern  void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);
extern  void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
extern  void ( APIENTRY * qglClearStencil )(GLint s);
extern  void ( APIENTRY * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern  void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
extern  void ( APIENTRY * qglEnableClientState )(GLenum array);
extern  void ( APIENTRY * qglDisableClientState )(GLenum array);
extern  void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
extern  void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
extern  void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglDepthFunc )(GLenum func);
extern  void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
extern  void ( APIENTRY * qglAlphaFunc )(GLenum func,  GLclampf ref);
extern	void ( APIENTRY * qglLineWidth )(GLfloat width);
extern	void ( APIENTRY * qglFrontFace )(GLenum mode);

void vglVertexAttribPointerMapped(int id, void* ptr);

/*
 * $Log: /H2 Mission Pack/Quakedef.h $
 * 
 * 8     3/19/98 12:53p Jmonroe
 * 
 * 7     3/13/98 5:01a Mgummelt
 * May have finally fixed that damn monster stair-stepping problem...
 * 
 * 6     3/12/98 6:31p Mgummelt
 * 
 * 5     3/06/98 4:55p Mgummelt
 * 
 * 4     3/03/98 1:41p Jmonroe
 * removed old mp stuff
 * 
 * 3     3/01/98 7:30p Jweier
 * 
 * 54    10/29/97 5:39p Jheitzman
 * 
 * 53    10/28/97 2:58p Jheitzman
 * 
 * 51    10/06/97 6:04p Rjohnson
 * Fix for save games and version update
 * 
 * 50    9/25/97 11:56p Rjohnson
 * Version update
 * 
 * 49    9/23/97 8:56p Rjohnson
 * Updates
 * 
 * 48    9/15/97 11:15a Rjohnson
 * Updates
 * 
 * 47    9/04/97 4:44p Rjohnson
 * Updates
 * 
 * 46    9/02/97 12:24a Rjohnson
 * Version Update
 * 
 * 45    8/30/97 6:17p Rjohnson
 * Network changes
 * 
 * 44    8/29/97 2:49p Rjohnson
 * Network updates
 * 
 * 43    8/28/97 3:36p Rjohnson
 * Version Update
 * 
 * 42    8/27/97 12:11p Rjohnson
 * Version Update
 * 
 * 41    8/26/97 8:17a Rjohnson
 * Just a few changes
 * 
 * 40    8/21/97 10:12p Rjohnson
 * Version Update
 * 
 * 39    8/20/97 2:59p Rjohnson
 * Version Update
 * 
 * 38    8/16/97 10:25a Rjohnson
 * Version Update
 * 
 * 37    8/11/97 2:52p Rlove
 * 
 * 36    8/09/97 10:39a Rjohnson
 * Version Update
 * 
 * 35    8/09/97 1:13a Bgokey
 * 
 * 34    8/05/97 3:55p Rjohnson
 * Version # Update
 * 
 * 33    8/05/97 3:49p Rjohnson
 * Fix for ipx networking
 * 
 * 32    8/01/97 6:25p Rjohnson
 * 
 * 31    7/30/97 1:56p Rjohnson
 * Changed version
 * 
 * 30    7/21/97 9:25p Rjohnson
 * Reduces the amount of memory used by RJNET
 * 
 * 29    7/11/97 5:21p Rjohnson
 * RJNET Updates
 * 
 * 28    6/16/97 5:38p Rlove
 * Temporary E3 fix of the max packet size MAX_DATAGRAM
 * 
 * 27    6/16/97 3:46p Rjohnson
 * Increased the reliable packet size
 * 
 * 26    6/06/97 11:21a Bgokey
 * 
 * 25    5/31/97 1:17p Bgokey
 * 
 * 24    5/27/97 4:46p Rjohnson
 * Added the smoke puff effect
 * 
 * 23    5/20/97 11:32a Rjohnson
 * Revised Effects
 * 
 * 22    5/19/97 2:54p Rjohnson
 * Added new client effects
 * 
 * 21    5/09/97 3:52p Rjohnson
 * Change to allow more than 256 precache models
 * 
 * 20    5/03/97 2:13p Bgokey
 * 
 * 19    4/20/97 5:05p Rjohnson
 * Networking Update
 * 
 * 18    4/15/97 9:02p Bgokey
 * 
 * 17    4/04/97 3:06p Rjohnson
 * Networking updates and corrections
 * 
 * 16    4/02/97 1:03p Bgokey
 * 
 * 15    3/07/97 1:57p Rjohnson
 * Id Updates
 * 
 * 14    2/27/97 4:12p Rjohnson
 * Added Midi prags
 * 
 * 13    2/24/97 3:51p Bgokey
 * Added HX_FRAME_TIME and HX_FPS.
 * 
 * 12    2/19/97 11:44a Rjohnson
 * Id Updates
 * 
 * 11    2/13/97 1:53p Bgokey
 * 
 * 10    2/07/97 1:37p Rlove
 * Artifact of Invincibility
 * 
 * 9     1/28/97 10:28a Rjohnson
 * Added experience and level advancement
 */
#endif
