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

#include "cfg.h"

#ifdef __XBOX__
#ifdef LINKSBOKS_STANDALONE

#include "xbox-input.h"
#include <xtl.h>

INT g_MouseSensitivityX, g_MouseSensitivityY;
INT g_KeyRepeatRate;
FLOAT g_MouseX = 320, g_MouseY = 240;
FLOAT g_OldMouseX = 320, g_OldMouseY = 240;
FLOAT g_Borders[4];
XBGAMEPAD*             g_Gamepad;
XBGAMEPAD              g_DefaultGamepad;
DWORD g_lastKeyRepeat = 0;
BOOL g_kpWaitingStickRelease = FALSE;
BYTE g_lastMouseButtons;


static void xbox_translate_key( XINPUT_DEBUG_KEYSTROKE *stroke, int *key, int *flag )
{
	*key  = 0;
	*flag = 0;

	if( (stroke->Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_CTRL) &&
		(stroke->Ascii == 'c') )
	{
		*key = KBD_CTRL_C;
		return;
	}

	/* setting Shift seems to break things
	*
	*  if (event->modifiers & DIMM_SHIFT)
	*     *flag |= KBD_SHIFT;
	*/

	if( stroke->Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_CTRL )
		*flag |= KBD_CTRL;
	if( stroke->Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_ALT )
		*flag |= KBD_ALT;

	switch( stroke->VirtualKey )
	{
		case VK_RETURN:	*key = KBD_ENTER;     break;
		case VK_BACK:	*key = KBD_BS;        break;
		case VK_TAB:	*key = KBD_TAB;       break;
		case VK_ESCAPE:	*key = KBD_ESC;       break;
		case VK_UP:		*key = KBD_UP;        break;
		case VK_DOWN:	*key = KBD_DOWN;      break;
		case VK_LEFT:	*key = KBD_LEFT;      break;
		case VK_RIGHT:	*key = KBD_RIGHT;     break;
		case VK_INSERT:	*key = KBD_INS;       break;
		case VK_DELETE:	*key = KBD_DEL;       break;
		case VK_HOME:	*key = KBD_HOME;      break;
		case VK_END:	*key = KBD_END;       break;
		case VK_PRIOR:	*key = KBD_PAGE_UP;   break;
		case VK_NEXT:	*key = KBD_PAGE_DOWN; break;
		case VK_F1:		*key = KBD_F1;        break;
		case VK_F2:		*key = KBD_F2;        break;
		case VK_F3:		*key = KBD_F3;        break;
		case VK_F4:		*key = KBD_F4;        break;
		case VK_F5:		*key = KBD_F5;        break;
		case VK_F6:		*key = KBD_F6;        break;
		case VK_F7:		*key = KBD_F7;        break;
		case VK_F8:		*key = KBD_F8;        break;
		case VK_F9:		*key = KBD_F9;        break;
		case VK_F10:	*key = KBD_F10;       break;
		case VK_F11:	*key = KBD_F11;       break;
		case VK_F12:	*key = KBD_F12;       break;

		default:
			*key = stroke->Ascii;
			break;
	}
}


HRESULT InitXboxInput()
{
    HRESULT hr;
	
	XInitDevices( 0, NULL );

    // Create the gamepad devices
	OutputDebugString( "XBApp: Creating gamepad devices...\n" );
    if( FAILED( hr = XBInput_CreateGamepads( &g_Gamepad ) ) )
    {
		OutputDebugString( "Call to CreateGamepads() failed!\n" );
        return hr;
    }

    // Initialize the debug keyboard
    if( FAILED( hr = XBInput_InitDebugKeyboard() ) )
	{
		OutputDebugString( "Failed to initialize keyboard!\n" );
        return hr;
	}

    // Initialize the debug mouse
    if( FAILED( hr = XBInput_InitDebugMouse() ) )
	{
		OutputDebugString( "Failed to initialize mouse!\n" );
        return hr;
	}

	return S_OK;
}


VOID UpdateInput()
{
	DWORD i;

    //-----------------------------------------
    // Handle input
    //-----------------------------------------

	if( (g_lastKeyRepeat > 0) && (timeGetTime( ) - g_lastKeyRepeat > g_KeyRepeatRate) )
	{
		// Handle d-pad repeat rate!
	    for( i=0; i<4; i++ )
			g_Gamepad[i].wLastButtons &= 0x11111110;

		g_lastKeyRepeat = 0;
	}


	// Read the input for all connected gamepads
    XBInput_GetInput( g_Gamepad );




    // Lump inputs of all connected gamepads into one common structure.
    // This is done so apps that need only one gamepad can function with
    // any gamepad.
    ZeroMemory( &g_DefaultGamepad, sizeof(g_DefaultGamepad) );
    INT nThumbLX = 0;
    INT nThumbLY = 0;
    INT nThumbRX = 0;
    INT nThumbRY = 0;

    for( i=0; i<4; i++ )
    {
        if( g_Gamepad[i].hDevice )
        {
            // Only account for thumbstick info beyond the deadzone
            if( g_Gamepad[i].sThumbLX > XINPUT_DEADZONE ||
                g_Gamepad[i].sThumbLX < -XINPUT_DEADZONE )
                nThumbLX += g_Gamepad[i].sThumbLX;
            if( g_Gamepad[i].sThumbLY > XINPUT_DEADZONE ||
                g_Gamepad[i].sThumbLY < -XINPUT_DEADZONE )
                nThumbLY += g_Gamepad[i].sThumbLY;
            if( g_Gamepad[i].sThumbRX > XINPUT_DEADZONE ||
                g_Gamepad[i].sThumbRX < -XINPUT_DEADZONE )
                nThumbRX += g_Gamepad[i].sThumbRX;
            if( g_Gamepad[i].sThumbRY > XINPUT_DEADZONE ||
                g_Gamepad[i].sThumbRY < -XINPUT_DEADZONE )
                nThumbRY += g_Gamepad[i].sThumbRY;

            g_DefaultGamepad.fX1 += g_Gamepad[i].fX1;
            g_DefaultGamepad.fY1 += g_Gamepad[i].fY1;
            g_DefaultGamepad.fX2 += g_Gamepad[i].fX2;
            g_DefaultGamepad.fY2 += g_Gamepad[i].fY2;
            g_DefaultGamepad.wButtons         |= g_Gamepad[i].wButtons;
            g_DefaultGamepad.wPressedButtons  |= g_Gamepad[i].wPressedButtons;
            g_DefaultGamepad.wReleasedButtons |= g_Gamepad[i].wReleasedButtons;
            g_DefaultGamepad.wLastButtons     |= g_Gamepad[i].wLastButtons;

            for( DWORD b=0; b<8; b++ )
            {
                g_DefaultGamepad.bAnalogButtons[b]         |= g_Gamepad[i].bAnalogButtons[b];
                g_DefaultGamepad.bPressedAnalogButtons[b]  |= g_Gamepad[i].bPressedAnalogButtons[b];
                g_DefaultGamepad.bReleasedAnalogButtons[b] |= g_Gamepad[i].bReleasedAnalogButtons[b];
                g_DefaultGamepad.bLastAnalogButtons[b]     |= g_Gamepad[i].bLastAnalogButtons[b];
            }
        }
    }

    // Clamp summed thumbstick values to proper range
    g_DefaultGamepad.sThumbLX = SHORT( nThumbLX );
    g_DefaultGamepad.sThumbLY = SHORT( nThumbLY );
    g_DefaultGamepad.sThumbRX = SHORT( nThumbRX );
    g_DefaultGamepad.sThumbRY = SHORT( nThumbRY );

    // Handle special input combo to trigger a reboot to the Xbox Dashboard
    if( g_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 0 )
    {
        if( g_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 0 )
        {
            if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] )
            {
                LD_LAUNCH_DASHBOARD LaunchData = { XLD_LAUNCH_DASHBOARD_MAIN_MENU };
			    XLaunchNewImage( NULL, (LAUNCH_DATA*)&LaunchData );
            }
        }
    }

}

VOID HandleGeneralBindings(struct graphics_device *dev)
{

	// Back pressed => change input mode
	if( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_BACK) == XINPUT_GAMEPAD_BACK
		|| (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_LEFT_THUMB) == XINPUT_GAMEPAD_LEFT_THUMB)
	{
		g_InputMode = (g_InputMode == NAVIGATION_MODE) ? TEXT_INPUT_MODE : NAVIGATION_MODE;
	}

	// That's it for the video calibration mode
	if( g_InputMode == VIDEO_CALIBRATION_MODE )
		return;

	// Debug keyboard: simply "translate" the keystroke and pass it to the keyboard handler
	XINPUT_DEBUG_KEYSTROKE *kbdStroke;
	int kbkey, kbflag;
	while( kbdStroke = XBInput_GetKeyboardInput() )
	{
		xbox_translate_key( kbdStroke, &kbkey, &kbflag );
		dev->keyboard_handler( dev, kbkey, kbflag );
	}

	// Start pressed => ENTER
	if( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_START) == XINPUT_GAMEPAD_START )
	{
		if( dev )
			dev->keyboard_handler( dev, KBD_ENTER, 0 );
	}

	// Pressing right thumbstick pastes
/*	if( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB) )
	{
		if( dev )
			dev->keyboard_handler( dev, KBD_PASTE, 0 );
	}
doesn't work
*/

	// Directions on the dpad => equivalent arrow keys
	if( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_LEFT) == XINPUT_GAMEPAD_DPAD_LEFT )
	{
		g_lastKeyRepeat = timeGetTime( );
		if( dev )
			dev->keyboard_handler( dev, KBD_LEFT, 0 );
	}
	if( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_RIGHT) == XINPUT_GAMEPAD_DPAD_RIGHT )
	{
		g_lastKeyRepeat = timeGetTime( );
		if( dev )
			dev->keyboard_handler( dev, KBD_RIGHT, 0 );
	}
	if( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_UP) == XINPUT_GAMEPAD_DPAD_UP )
	{
		g_lastKeyRepeat = timeGetTime( );
		if( dev )
			dev->keyboard_handler( dev, KBD_UP, 0 );
	}
	if( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_DOWN) == XINPUT_GAMEPAD_DPAD_DOWN )
	{
		g_lastKeyRepeat = timeGetTime( );
		if( dev )
			dev->keyboard_handler( dev, KBD_DOWN, 0 );
	}

}


/*****************************************************************************/
/***************************** NAVIGATION MODE *******************************/
/*****************************************************************************/

VOID HandleInputNavMode( struct graphics_device *dev )
{
	BYTE mousePressedButtons, mouseReleasedButtons;

	//-----------------------------------------
    // Update mouse position
    //-----------------------------------------

	// Poll debug mouse too
    DWORD dwMousePortsChanged = XBInput_GetMouseInput();
    XINPUT_STATE CurrentMouseState;
    ZeroMemory( &CurrentMouseState, sizeof(XINPUT_STATE) );

    for( DWORD i=0; i < XGetPortCount(); i++ )
    {
        if( dwMousePortsChanged & ( 1 << i ) ) 
            CurrentMouseState = g_MouseState[i];
    }

    if( dwMousePortsChanged )
    {
		// TRUE HACK: Change the values in the gamepad structures
        FLOAT fRotX1 = 0.0f, fRotY1 = 0.0f;
        g_DefaultGamepad.fX1 += CurrentMouseState.DebugMouse.cMickeysX / (128.0f / (float)g_MouseSensitivityX);
        g_DefaultGamepad.fY1 -= CurrentMouseState.DebugMouse.cMickeysY / (128.0f / (float)g_MouseSensitivityY);
		g_lastMouseButtons = CurrentMouseState.DebugMouse.bButtons;
	}
	mousePressedButtons = (g_lastMouseButtons ^ CurrentMouseState.DebugMouse.bButtons) & CurrentMouseState.DebugMouse.bButtons;
	mouseReleasedButtons = (g_lastMouseButtons ^ CurrentMouseState.DebugMouse.bButtons) & (CurrentMouseState.DebugMouse.bButtons ^ 0xFF);


	// END debug mouse


	int g_OldMouseX = (int)g_MouseX, g_OldMouseY = (int)g_MouseY;
	g_MouseX += g_DefaultGamepad.fX1 * g_MouseSensitivityX;
	if( g_MouseX < 0 )
		g_MouseX = 0;
	if( g_MouseX > VIEWPORT_WIDTH )
		g_MouseX = (float)VIEWPORT_WIDTH;

	g_MouseY -= g_DefaultGamepad.fY1 * g_MouseSensitivityY;
	if( g_MouseY < 0 )
		g_MouseY = 0;
	if( g_MouseY > VIEWPORT_HEIGHT )
		g_MouseY = (float)VIEWPORT_HEIGHT;

	//-----------------------------------------
    // Call handlers
    //-----------------------------------------
	int flags;	


	// "Wheel movement" (right thumbstick or mouse wheel)
	if( abs(g_DefaultGamepad.fY2) > 0.2 )
	{
		// Up/down
		if( g_DefaultGamepad.fY2 > 0 )
			dev->mouse_handler( dev, (int)g_MouseX, (int)g_MouseY,
					(abs(g_DefaultGamepad.fY2) > 0.8) ? B_MOVE | B_WHEELUP : B_MOVE | B_WHEELUP1 );
		else
			dev->mouse_handler( dev, (int)g_MouseX, (int)g_MouseY,
					(abs(g_DefaultGamepad.fY2) > 0.8) ? B_MOVE | B_WHEELDOWN : B_MOVE | B_WHEELDOWN1 );
	}
	if( abs(g_DefaultGamepad.fX2) > 0.2 )
	{
		// Left/right
		if( g_DefaultGamepad.fX2 > 0 )
			dev->mouse_handler( dev, (int)g_MouseX, (int)g_MouseY,
					(abs(g_DefaultGamepad.fX2) > 0.8) ? B_MOVE | B_WHEELRIGHT : B_MOVE | B_WHEELRIGHT1 );
		else
			dev->mouse_handler( dev, (int)g_MouseX, (int)g_MouseY,
					(abs(g_DefaultGamepad.fX2) > 0.8) ? B_MOVE | B_WHEELLEFT : B_MOVE | B_WHEELLEFT1 );
	}
	if( CurrentMouseState.DebugMouse.cWheel )
	{
		// Up/down (debug mouse wheel)
		if( CurrentMouseState.DebugMouse.cWheel > 0 )
			dev->mouse_handler( dev, (int)g_MouseX, (int)g_MouseY,
					(abs(CurrentMouseState.DebugMouse.cWheel) > 2) ? B_MOVE | B_WHEELUP : B_MOVE | B_WHEELUP1 );
		else
			dev->mouse_handler( dev, (int)g_MouseX, (int)g_MouseY,
					(abs(CurrentMouseState.DebugMouse.cWheel) > 2) ? B_MOVE | B_WHEELDOWN : B_MOVE | B_WHEELDOWN1 );
	}


	// Mouse movement (left thumbstick)
	if( ((int)g_MouseX != (int)g_OldMouseX) || ((int)g_MouseY != (int)g_OldMouseY) )
	{
		flags = 0;

		if( (g_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_A] > XINPUT_GAMEPAD_MAX_CROSSTALK)
			|| (CurrentMouseState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_LEFT_BUTTON))
			flags = B_LEFT | B_DRAG;

		if( (g_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X] > XINPUT_GAMEPAD_MAX_CROSSTALK)
			|| (CurrentMouseState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_RIGHT_BUTTON))
			flags = B_RIGHT | B_DRAG;

		if( (g_DefaultGamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
			|| (CurrentMouseState.DebugMouse.bButtons & XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON))
			flags = B_MIDDLE | B_DRAG;

		if( !flags )
			flags |= B_MOVE;
		
		if( dev )
			dev->mouse_handler(dev, (int)g_MouseX, (int)g_MouseY, flags);
	}


	// A (left mouse button) or X (right mouse button) pressed
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A]
		|| g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_X]
		|| (mousePressedButtons & XINPUT_DEBUG_MOUSE_LEFT_BUTTON)
		|| (mousePressedButtons & XINPUT_DEBUG_MOUSE_RIGHT_BUTTON)
		|| (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
		|| (mousePressedButtons & XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON))
	{
		flags = 0;

		if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A]
		|| (mousePressedButtons & XINPUT_DEBUG_MOUSE_LEFT_BUTTON))
		{
			flags |= B_LEFT;
//			OutputDebugString( "left down\n" );
		}

		if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_X]
		|| (mousePressedButtons & XINPUT_DEBUG_MOUSE_RIGHT_BUTTON))
		{
			flags |= B_RIGHT;
//			OutputDebugString( "right down\n" );
		}

		if( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
		|| (mousePressedButtons & XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON))
		{
			flags |= B_MIDDLE;
//			OutputDebugString( "middle down\n" );
		}


		flags |= B_DOWN;
		

		if( dev )
			dev->mouse_handler(dev, (int)g_MouseX, (int)g_MouseY, flags);

		return;
	}

	// A (left mouse button) or X (right mouse button) released
	if( g_DefaultGamepad.bReleasedAnalogButtons[XINPUT_GAMEPAD_A]
		|| g_DefaultGamepad.bReleasedAnalogButtons[XINPUT_GAMEPAD_X]
		|| (mouseReleasedButtons & XINPUT_DEBUG_MOUSE_LEFT_BUTTON)
		|| (mouseReleasedButtons & XINPUT_DEBUG_MOUSE_RIGHT_BUTTON)
		|| (g_DefaultGamepad.wReleasedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
		|| (mouseReleasedButtons & XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON))
{
		flags = 0;

		if( g_DefaultGamepad.bReleasedAnalogButtons[XINPUT_GAMEPAD_A]
		|| (mouseReleasedButtons & XINPUT_DEBUG_MOUSE_LEFT_BUTTON))
		{		
			flags |= B_LEFT;
//			OutputDebugString( "left up\n" );
		}

		if( g_DefaultGamepad.bReleasedAnalogButtons[XINPUT_GAMEPAD_X]
		|| (mouseReleasedButtons & XINPUT_DEBUG_MOUSE_RIGHT_BUTTON))
		{
			flags |= B_RIGHT;
//			OutputDebugString( "right up\n" );
		}

		if( (g_DefaultGamepad.wReleasedButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
		|| (mouseReleasedButtons & XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON))
		{
			flags |= B_MIDDLE;
//			OutputDebugString( "middle up\n" );
		}

		flags |= B_UP;
		
		if( dev )
			dev->mouse_handler(dev, (int)g_MouseX, (int)g_MouseY, flags);

		return;
	}

	// left trigger pressed => back
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] ||
		(mousePressedButtons & XINPUT_DEBUG_MOUSE_XBUTTON1))
	{
		if( dev )
			dev->keyboard_handler( dev, (int)'z', 0 );
	}

	// right trigger pressed => forward
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] ||
		(mousePressedButtons & XINPUT_DEBUG_MOUSE_XBUTTON2))
	{
		if( dev )
			dev->keyboard_handler( dev, (int)'`', 0 );
	}

	// B pressed => ESC
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_B] )
	{
		if( dev )
			dev->keyboard_handler( dev, KBD_ESC, 0 );
	}

	// Y pressed => close tab
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_Y] )
	{
		if( dev )
			dev->keyboard_handler( dev, (int)'c', 0 );
	}

	// Black pressed => bookmarks
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] )
	{
		if( dev )
			dev->keyboard_handler( dev, (int)'s', 0 );
	}

	// White pressed => goto URL
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE] )
	{
		if( dev )
			dev->keyboard_handler( dev, (int)'g', 0 );
	}
}


/*****************************************************************************/
/***************************** TEXT INPUT MODE *******************************/
/*****************************************************************************/

GamepadKeyInput g_keysNormal[9] =
{
	// center
	{{
		{	L"Space",	L' '	},
		{	L"BkSp",	KBD_BS	},
		{	L"Del",		KBD_DEL	},
		{	L".",		NULL	},
	}},
	// bottom-left
	{{
		{	L"a",		NULL	},
		{	L"b",		NULL	},
		{	L"c",		NULL	},
		{	L"d",		NULL	},
	}},
	// middle-left
	{{
		{	L"e",		NULL	},
		{	L"f",		NULL	},
		{	L"g",		NULL	},
		{	L"h",		NULL	},
	}},
	// top-left
	{{
		{	L"i",		NULL	},
		{	L"j",		NULL	},
		{	L"k",		NULL	},
		{	L"l",		NULL	},
	}},
	// top-center
	{{
		{	L"m",		NULL	},
		{	L"n",		NULL	},
		{	L"o",		NULL	},
		{	L"p",		NULL	},
	}},
	// top-right
	{{
		{	L"q",		NULL	},
		{	L"r",		NULL	},
		{	L"s",		NULL	},
		{	L"t",		NULL	},
	}},
	// middle-right
	{{
		{	L"u",		NULL	},
		{	L"v",		NULL	},
		{	L"w",		NULL	},
		{	L"x",		NULL	},
	}},
	// bottom-right
	{{
		{	L"y",		NULL	},
		{	L"z",		NULL	},
		{	L"/",		NULL	},
		{	L"~",		NULL	},
	}},
	// bottom-middle
	{{
		{	L".com",	NULL	},
		{	L".net",	NULL	},
		{	L".org",	NULL	},
		{	L"-",		NULL	},
	}},
};

struct _GamepadKeyInput g_keysCaps[9] =
{
	// center
	{{
		{	L"Space",	L' '	},
		{	L"BkSp",	KBD_BS	},
		{	L"Del",		KBD_DEL	},
		{	L".",		NULL	},
	}},
	// bottom-left
	{{
		{	L"A",		NULL	},
		{	L"B",		NULL	},
		{	L"C",		NULL	},
		{	L"D",		NULL	},
	}},
	// middle-left
	{{
		{	L"E",		NULL	},
		{	L"F",		NULL	},
		{	L"G",		NULL	},
		{	L"H",		NULL	},
	}},
	// top-left
	{{
		{	L"I",		NULL	},
		{	L"J",		NULL	},
		{	L"K",		NULL	},
		{	L"L",		NULL	},
	}},
	// top-center
	{{
		{	L"M",		NULL	},
		{	L"N",		NULL	},
		{	L"O",		NULL	},
		{	L"P",		NULL	},
	}},
	// top-right
	{{
		{	L"Q",		NULL	},
		{	L"R",		NULL	},
		{	L"S",		NULL	},
		{	L"T",		NULL	},
	}},
	// middle-right
	{{
		{	L"U",		NULL	},
		{	L"V",		NULL	},
		{	L"W",		NULL	},
		{	L"X",		NULL	},
	}},
	// bottom-right
	{{
		{	L"Y",		NULL	},
		{	L"Z",		NULL	},
		{	L"/",		NULL	},
		{	L"~",		NULL	},
	}},
	// bottom-middle
	{{
		{	L".com",	NULL	},
		{	L".net",	NULL	},
		{	L".org",	NULL	},
		{	L"-",		NULL	},
	}},
};

struct _GamepadKeyInput g_keysSymbols[9] =
{
	// center
	{{
		{	L"Space",	L' '	},
		{	L"BkSp",	KBD_BS	},
		{	L"Del",		KBD_DEL	},
		{	L".",		NULL	},
	}},
	// bottom-left
	{{
		{	L".",		NULL	},
		{	L",",		NULL	},
		{	L";",		NULL	},
		{	L":",		NULL	},
	}},
	// middle-left
	{{
		{	L"+",		NULL	},
		{	L"-",		NULL	},
		{	L"*",		NULL	},
		{	L"/",		NULL	},
	}},
	// top-left
	{{
		{	L"=",		NULL	},
		{	L"_",		NULL	},
		{	L"'",		NULL	},
		{	L"\"",		NULL	},
	}},
	// top-center
	{{
		{	L"(",		NULL	},
		{	L")",		NULL	},
		{	L"[",		NULL	},
		{	L"]",		NULL	},
	}},
	// top-right
	{{
		{	L"{",		NULL	},
		{	L"}",		NULL	},
		{	L"\\",		NULL	},
		{	L"|",		NULL	},
	}},
	// middle-right
	{{
		{	L"<",		NULL	},
		{	L">",		NULL	},
		{	L"?",		NULL	},
		{	L"!",		NULL	},
	}},
	// bottom-right
	{{
		{	L"@",		NULL	},
		{	L"#",		NULL	},
		{	L"$",		NULL	},
		{	L"%",		NULL	},
	}},
	// bottom-middle
	{{
		{	L"&",	NULL	},
		{	L"`",	NULL	},
		{	L"^",	NULL	},
		{	L"€",	NULL	},
	}},
};

int GetKeyGroupOffset()
{
	if( STICKCENTER && STICKMIDDLE )
		return 0;
	if( STICKLEFT && STICKBOTTOM )
		return 1;
	if( STICKLEFT && STICKMIDDLE )
		return 2;
	if( STICKLEFT && STICKTOP )
		return 3;
	if( STICKCENTER && STICKTOP )
		return 4;
	if( STICKRIGHT && STICKTOP )
		return 5;
	if( STICKRIGHT && STICKMIDDLE )
		return 6;
	if( STICKRIGHT && STICKBOTTOM )
		return 7;
	if( STICKCENTER && STICKBOTTOM )
		return 8;

	// Shouldn't get there
	return 0;
}

struct keygroup *GetKeyGroup( int i )
{
	if( g_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > XINPUT_GAMEPAD_MAX_CROSSTALK )
		return( (struct keygroup *)(&g_keysCaps[i]) );
	else if( g_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > XINPUT_GAMEPAD_MAX_CROSSTALK )
		return( (struct keygroup *)(&g_keysSymbols[i]) );
	else
		return( (struct keygroup *)(&g_keysNormal[i]) );
}

struct keygroup *GetCurrentKeyGroup()
{
	return GetKeyGroup( GetKeyGroupOffset() );
}



VOID TextModeKeyStroke( struct graphics_device *dev, struct keygroup kg )
{
	if( kg.links_code )
		dev->keyboard_handler( dev, kg.links_code, 0 );
	else
		for( int i = 0; i < wcslen( kg.text ); i++ )
		{
			dev->keyboard_handler( dev, (int)(kg.text[i]), 0 );
		}
}

VOID HandleInputTextMode( struct graphics_device *dev, struct keygroup *kg )
{
	if( !dev )
		return;

	// Go to keypad mode
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] )
		g_InputMode = KEYPAD_MODE;

	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A] )
		TextModeKeyStroke( dev, kg[0] );
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_B] )
		TextModeKeyStroke( dev, kg[1] );
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_X] )
		TextModeKeyStroke( dev, kg[2] );
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_Y] )
		TextModeKeyStroke( dev, kg[3] );

}


/*****************************************************************************/
/******************************* KEYPAD MODE *********************************/
/*****************************************************************************/

WCHAR* g_KeyPad[12] = { L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L":", L"0", L"." };
INT g_kpActiveKey = 4;

VOID HandleInputKeypadMode( struct graphics_device *dev )
{
	if( !dev )
		return;

	if( g_kpWaitingStickRelease )
	{
		if( STICKCENTER && STICKMIDDLE )
			g_kpWaitingStickRelease = FALSE;

		if( timeGetTime( ) - g_lastKeyRepeat > g_KeyRepeatRate )
			g_kpWaitingStickRelease = FALSE;
	}
	else
	{
		if( STICKTOP )
			g_kpActiveKey = (g_kpActiveKey > 2) ? g_kpActiveKey - 3 : g_kpActiveKey + 9;

		if( STICKBOTTOM )
			g_kpActiveKey = (g_kpActiveKey < 9) ? g_kpActiveKey + 3 : g_kpActiveKey - 9;

		if( STICKRIGHT )
			g_kpActiveKey = (g_kpActiveKey % 3 != 2) ? g_kpActiveKey + 1 : g_kpActiveKey - 2;

		if( STICKLEFT )
			g_kpActiveKey = (g_kpActiveKey % 3 != 0) ? g_kpActiveKey - 1 : g_kpActiveKey + 2;

		if( !STICKCENTER || !STICKMIDDLE )
		{
			g_lastKeyRepeat = timeGetTime( );
			g_kpWaitingStickRelease = TRUE;
		}
		
	}

	// Send keystroke
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_A] )
		dev->keyboard_handler( dev, (int)(*g_KeyPad[g_kpActiveKey]), 0 );

	// Backspace
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_B] )
		dev->keyboard_handler( dev, KBD_BS, 0 );

	// Delete
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_X] )
		dev->keyboard_handler( dev, KBD_DEL, 0 );

	// Return to text input mode
	if( g_DefaultGamepad.bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] )
		g_InputMode = TEXT_INPUT_MODE;
}

/*****************************************************************************/
/************************* VIDEO CALIBRATION MODE ****************************/
/*****************************************************************************/

extern "C" void EnterVideoCalibrationMode()
{
	g_Borders[0] = (FLOAT)g_ViewPort.margin_top;
	g_Borders[1] = (FLOAT)g_ViewPort.margin_bottom;
	g_Borders[2] = (FLOAT)g_ViewPort.margin_left;
	g_Borders[3] = (FLOAT)g_ViewPort.margin_right;
	g_InputMode = VIDEO_CALIBRATION_MODE;
}

VOID HandleInputCalibrationMode( struct graphics_device *dev )
{
	int oldBorders[4];
	BOOL changed = FALSE;
	unsigned char *varnames[4] = {
		(unsigned char *)"xbox_margin_top",
		(unsigned char *)"xbox_margin_bottom",
		(unsigned char *)"xbox_margin_left",
		(unsigned char *)"xbox_margin_right",
	};

	for( int i = 0; i < 4; i++ )
		oldBorders[i] = (int)g_Borders[i];

	// That's sooo ugly!
	g_Borders[0] -= (abs(g_DefaultGamepad.fY1) >= STICK_THRESHOLD) ? ((g_DefaultGamepad.fY1 > 0) ? 0.5 : -0.5) : 0;
	g_Borders[1] += (abs(g_DefaultGamepad.fY2) >= STICK_THRESHOLD) ? ((g_DefaultGamepad.fY2 > 0) ? 0.5 : -0.5) : 0;
	g_Borders[2] += (abs(g_DefaultGamepad.fX1) >= STICK_THRESHOLD) ? ((g_DefaultGamepad.fX1 > 0) ? 0.5 : -0.5) : 0;
	g_Borders[3] -= (abs(g_DefaultGamepad.fX2) >= STICK_THRESHOLD) ? ((g_DefaultGamepad.fX2 > 0) ? 0.5 : -0.5) : 0;

	for( i = 0; i < 4; i++ )
	{
		if( g_Borders[i] < 0 )
			g_Borders[i] = 0;

		if( g_Borders[i] > 125 )
			g_Borders[i] = 125;

		if( oldBorders[i] == (int)g_Borders[i] )
		{
			options_set_int( varnames[i], (int)g_Borders[i] );
			changed = TRUE;
		}
	}

	/* Change the font size with the dpad! */
	if( ( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_UP) == XINPUT_GAMEPAD_DPAD_UP ) &&
		( options_get_int( (unsigned char *)"html_font_size" ) < 30 ) )
	{
		options_set_int( (unsigned char *)"html_font_size", options_get_int( (unsigned char *)"html_font_size" ) + 1 );
	}

	/* Change the font size with the dpad! */
	if( ( (g_DefaultGamepad.wPressedButtons & XINPUT_GAMEPAD_DPAD_DOWN) == XINPUT_GAMEPAD_DPAD_DOWN ) &&
		( options_get_int( (unsigned char *)"html_font_size" ) > 10 ) )
	{
		options_set_int( (unsigned char *)"html_font_size", options_get_int( (unsigned char *)"html_font_size" ) - 1 );
	}

	if( changed )
		UpdateXboxOptions();
}


#endif
#endif