/*
 * LinksBoks
 * Copyright (c) 2003-2004 ysbox
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _XBOX_INPUT_H_
#define _XBOX_INPUT_H_

#include "../xbox/mouse.h"
#include "../xbox/keyboard.h"
#include "../xbox/xbinput.h"
#include "../xbox/xbfont.h"

extern "C" {
#include "links.h"
}

struct videomode {
	const WCHAR *name;
	int width, height;
	int margin_left, margin_right;
	int margin_top, margin_bottom;
	DWORD standard;
	DWORD flags_in, flags_out;
};

typedef enum {
	NAVIGATION_MODE,
	TEXT_INPUT_MODE,
	KEYPAD_MODE,
	VIDEO_CALIBRATION_MODE,
} INPUT_MODE;

struct keygroup {
	WCHAR* text;
	int links_code;
};

typedef struct _GamepadKeyInput
{
	struct keygroup group[4];
} GamepadKeyInput;

extern GamepadKeyInput g_keysNormal[9];
extern GamepadKeyInput g_keysCaps[9];
extern GamepadKeyInput g_keysSymbols[9];

extern WCHAR* g_KeyPad[12];
extern INT g_kpActiveKey;

extern struct videomode g_ViewPort;
extern INPUT_MODE g_InputMode;

#define STICK_THRESHOLD		0.3
#define STICKLEFT	(g_DefaultGamepad.fX1 <= -STICK_THRESHOLD)
#define STICKCENTER	(g_DefaultGamepad.fX1 > -STICK_THRESHOLD && g_DefaultGamepad.fX1 < STICK_THRESHOLD)
#define STICKRIGHT	(g_DefaultGamepad.fX1 >= STICK_THRESHOLD)
#define STICKBOTTOM	(g_DefaultGamepad.fY1 <= -STICK_THRESHOLD)
#define STICKMIDDLE	(g_DefaultGamepad.fY1 > -STICK_THRESHOLD && g_DefaultGamepad.fY1 < STICK_THRESHOLD)
#define STICKTOP	(g_DefaultGamepad.fY1 >= STICK_THRESHOLD)


// Deadzone for the gamepad inputs
const SHORT XINPUT_DEADZONE = (SHORT)( 0.24f * FLOAT(0x7FFF) );

extern int g_MouseSensitivityX, g_MouseSensitivityY, g_KeyRepeatRate;
#define MOUSE_HOTSPOT_X		8
#define MOUSE_HOTSPOT_Y		4


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
extern LPDIRECT3D8             g_pD3D;
extern LPDIRECT3DDEVICE8       g_pd3dDevice;
extern LPDIRECT3DSURFACE8      g_pdSurface;
extern LPDIRECT3DSURFACE8      g_pdBkBuffer;
extern LPDIRECT3DTEXTURE8		g_pMousePtr;

extern BOOL					g_bWantFlip;

extern FLOAT g_MouseX, g_MouseY;
extern XBGAMEPAD*             g_Gamepad;
extern XBGAMEPAD              g_DefaultGamepad;


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
HRESULT InitXboxInput();
VOID HandleMouseInput(struct graphics_device *dev);
VOID UpdateInput();
VOID HandleInputNavMode(struct graphics_device *dev);
VOID HandleInputTextMode(struct graphics_device *dev, struct keygroup *kg);
VOID HandleInputKeypadMode(struct graphics_device *dev);
VOID HandleInputCalibrationMode( struct graphics_device *dev );
VOID HandleGeneralBindings(struct graphics_device *dev);
extern "C" void UpdateXboxOptions();
struct keygroup *GetKeyGroup(int i);
struct keygroup *GetCurrentKeyGroup();

#endif