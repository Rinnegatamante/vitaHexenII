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

#include "quakedef.h"
#include <vitasdk.h>

// mouse variables
cvar_t	m_filter = {"m_filter","0"};
extern cvar_t always_run, inverted;

cvar_t	pstv_rumble = {"pstv_rumble","1"};
cvar_t	retrotouch = {"retrotouch","0"};
cvar_t always_run = {"always_run","0"};
cvar_t inverted = {"invert_camera","0"};
#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)
uint64_t rumble_tick = 0;
SceCtrlData oldanalogs, analogs;

void IN_Init (void)
{
  if ( COM_CheckParm ("-nomouse") )
    return;

  Cvar_RegisterVariable (&m_filter);
  Cvar_RegisterVariable (&retrotouch);
  Cvar_RegisterVariable (&always_run);
  Cvar_RegisterVariable (&inverted);
  Cvar_RegisterVariable (&pstv_rumble);
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
	SceCtrlActuator handle;
	handle.small = 100;
	handle.large = 100;
	sceCtrlSetActuator(1, &handle);
	rumble_tick = sceKernelGetProcessTimeWide();
}

void IN_StopRumble (void)
{
	SceCtrlActuator handle;
	handle.small = 0;
	handle.large = 0;
	sceCtrlSetActuator(1, &handle);
	rumble_tick = 0;
}

void IN_Move (usercmd_t *cmd)
{

	if ((in_speed.state & 1) || always_run.value){
		cl_forwardspeed.value = 400;
		cl_backspeed.value = 400;
		cl_sidespeed.value = 700;
	}else{
		cl_forwardspeed.value = 200;
		cl_backspeed.value = 200;
		cl_sidespeed.value = 300;
	}
	
	sceCtrlPeekBufferPositive(0, &analogs, 1);
	int left_x = analogs.lx - 127;
	int left_y = analogs.ly - 127;
	int right_x = analogs.rx - 127;
	int right_y = analogs.ry - 127;
	
	// Left analog support for player movement
	int x_mov = abs(left_x) < 30 ? 0 : (left_x * cl_sidespeed.value) * 0.005;
	int y_mov = abs(left_y) < 30 ? 0 : (left_y * (left_y > 0 ? cl_backspeed.value : cl_forwardspeed.value)) * 0.006;
	cmd->forwardmove -= y_mov;
	cmd->sidemove += x_mov;
	
	// Right analog support for camera movement
	IN_RescaleAnalog(&right_x, &right_y, 30);
	float x_cam = (right_x * sensitivity.value) * 0.008;
	float y_cam = (right_y * sensitivity.value) * 0.008;
	cl.viewangles[YAW] -= x_cam;
	V_StopPitchDrift();
	if (inverted.value) cl.viewangles[PITCH] -= y_cam;
	else cl.viewangles[PITCH] += y_cam;
	
	// TOUCH SUPPORT
	
	// Retrotouch support for camera movement
	SceTouchData touch;
	if (retrotouch.value){
		sceTouchPeek(SCE_TOUCH_PORT_BACK, &touch, 1);
		if (touch.reportNum > 0) {
			int raw_x = lerp(touch.report[0].x, 1919, 960);
			int raw_y = lerp(touch.report[0].y, 1087, 544);
			int touch_x = raw_x - 480;
			int touch_y = raw_y - 272;
			x_cam = abs(touch_x) < 20 ? 0 : touch_x * sensitivity.value * 0.008;
			y_cam = abs(touch_y) < 20 ? 0 : touch_y * sensitivity.value * 0.008;
			cl.viewangles[YAW] -= x_cam;
			V_StopPitchDrift();
			if (inverted.value) cl.viewangles[PITCH] -= y_cam;
			else cl.viewangles[PITCH] += y_cam;
		}
	}

}
