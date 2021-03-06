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

#include <psp2/display.h>
#include <vita2d.h>
#include "quakedef.h"
#include "d_local.h"
#define u16 uint16_t
#define u8 uint8_t

cvar_t res_val = {"render_res","1.0"};
cvar_t vsync = {"vsync", "0.0"};
viddef_t	vid;				// global video state
int old_char = 0;
float fixpalette = 0;
float rend_scale = 1.0;
char res_string[256];
const int widths[4] = {480, 640, 720, 960};
const int heights[4] = {272, 362, 408, 544};
const float scales[4] = {2.0, 1.5, 1.3333, 1.0};
extern cvar_t res_val;

#define SURFCACHE_SIZE 10485760

short	zbuffer[960*544];
byte*	surfcache;
vita2d_texture *tex_buffer,*gpu_buffer;
u16	d_8to16table[256];
byte globalcolormap[VID_GRADES*256], lastglobalcolor = 0;
byte *lastsourcecolormap = NULL;
uint32_t palette_tbl[256];

void	VID_SetPalette (unsigned char *palette)
{
	int i;
	uint32_t* palette_tbl = vita2d_texture_get_palette(tex_buffer);
	uint32_t* palette_tbl2 = vita2d_texture_get_palette(gpu_buffer);
	u8* pal = palette;
	unsigned r, g, b;
	
	for(i=0; i<256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		palette_tbl[i] = r | (g << 8) | (b << 16) | (0xFF << 24);
		palette_tbl2[i] = r | (g << 8) | (b << 16) | (0xFF << 24);
		pal += 3;
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette(palette);
}

void	VID_Init (unsigned char *palette)
{
	
	// Init GPU
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
	vita2d_set_vblank_wait(0);
	gpu_buffer = vita2d_create_empty_texture_format(widths[3], heights[3], SCE_GXM_TEXTURE_BASE_FORMAT_P8);
	vita2d_texture_set_alloc_memblock_type(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW);
	
	// Init GPU texture
	tex_buffer = vita2d_create_empty_texture_format(widths[3], heights[3], SCE_GXM_TEXTURE_BASE_FORMAT_P8);
	
	// Set Quake Engine parameters
	vid.maxwarpwidth = vid.width = vid.conwidth = widths[3];
	vid.maxwarpheight = vid.height = vid.conheight = heights[3];
	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 2;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vid.direct = vita2d_texture_get_datap(tex_buffer);
	vid.rowbytes = vid.conrowbytes = widths[3];
	
	// Set correct palette for the texture
	VID_SetPalette(palette);
	
	// Init Quake Cache
	d_pzbuffer = zbuffer;
	surfcache = malloc(SURFCACHE_SIZE);
	D_InitCaches (surfcache, SURFCACHE_SIZE);
	
	sprintf(res_string,"Current Resolution: %ld x %ld", widths[3], heights[3]);
	Cvar_RegisterVariable (&res_val);
	Cvar_RegisterVariable (&vsync);

}

void VID_ChangeRes(float scale){

	// Freeing texture
	vita2d_wait_rendering_done();
	vita2d_free_texture(tex_buffer);
	vita2d_free_texture(gpu_buffer);
	
	int idx = (scale / 0.333);
	
	// Changing renderer resolution
	int width = widths[idx];
	int height = heights[idx];
	vita2d_texture_set_alloc_memblock_type(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW);
	gpu_buffer = vita2d_create_empty_texture_format(width, height, SCE_GXM_TEXTURE_BASE_FORMAT_P8);
	vita2d_texture_set_alloc_memblock_type(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW);
	tex_buffer = vita2d_create_empty_texture_format(width, height, SCE_GXM_TEXTURE_BASE_FORMAT_P8);
	vid.maxwarpwidth = vid.width = vid.conwidth = width;
	vid.maxwarpheight = vid.height = vid.conheight = height;
	vid.rowbytes = vid.conrowbytes = width;	
	vid.buffer = vid.conbuffer = vid.direct = vita2d_texture_get_datap(tex_buffer);
	sprintf(res_string,"Current Resolution: %ld x %ld", widths[idx], heights[idx]);
	
	// Forcing a palette restoration
	fixpalette = v_gamma.value;
	Cvar_SetValue ("gamma", 0.1);
	
	// Changing scale value
	rend_scale = scales[idx];
	
}

void	VID_Shutdown (void)
{
	vita2d_wait_rendering_done();
	vita2d_free_texture(tex_buffer);
	vita2d_fini();
	free(surfcache);
}

void	VID_Update (vrect_t *rects)
{
	
	if (fixpalette > 0){
		Cvar_SetValue ("gamma", fixpalette);
		fixpalette = 0;
	}
	memcpy(vita2d_texture_get_datap(gpu_buffer),vita2d_texture_get_datap(tex_buffer),vita2d_texture_get_stride(gpu_buffer)*vita2d_texture_get_height(gpu_buffer));
	vita2d_start_drawing();
	vita2d_draw_texture_scale(gpu_buffer, 0, 0, rend_scale, rend_scale);
	vita2d_end_drawing();
	vita2d_wait_rendering_done();
	vita2d_swap_buffers();
	if (vsync.value) sceDisplayWaitVblankStart();
	
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}