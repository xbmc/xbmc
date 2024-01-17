/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/// \ingroup keyboard
/// \{

// Reserved 0 - 255
//  IRRemote.h
//  XINPUT_IR_REMOTE-*

/*
 * EventServer "gamepad" keys based on original Xbox controller
 */
// Analogue - don't change order
#define KEY_BUTTON_A 256
#define KEY_BUTTON_B 257
#define KEY_BUTTON_X 258
#define KEY_BUTTON_Y 259
#define KEY_BUTTON_BLACK 260
#define KEY_BUTTON_WHITE 261
#define KEY_BUTTON_LEFT_TRIGGER 262
#define KEY_BUTTON_RIGHT_TRIGGER 263

#define KEY_BUTTON_LEFT_THUMB_STICK 264
#define KEY_BUTTON_RIGHT_THUMB_STICK 265

#define KEY_BUTTON_RIGHT_THUMB_STICK_UP 266 // right thumb stick directions
#define KEY_BUTTON_RIGHT_THUMB_STICK_DOWN 267 // for defining different actions per direction
#define KEY_BUTTON_RIGHT_THUMB_STICK_LEFT 268
#define KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT 269

// Digital - don't change order
#define KEY_BUTTON_DPAD_UP 270
#define KEY_BUTTON_DPAD_DOWN 271
#define KEY_BUTTON_DPAD_LEFT 272
#define KEY_BUTTON_DPAD_RIGHT 273

#define KEY_BUTTON_START 274
#define KEY_BUTTON_BACK 275

#define KEY_BUTTON_LEFT_THUMB_BUTTON 276
#define KEY_BUTTON_RIGHT_THUMB_BUTTON 277

#define KEY_BUTTON_LEFT_ANALOG_TRIGGER 278
#define KEY_BUTTON_RIGHT_ANALOG_TRIGGER 279

#define KEY_BUTTON_LEFT_THUMB_STICK_UP 280 // left thumb stick directions
#define KEY_BUTTON_LEFT_THUMB_STICK_DOWN 281 // for defining different actions per direction
#define KEY_BUTTON_LEFT_THUMB_STICK_LEFT 282
#define KEY_BUTTON_LEFT_THUMB_STICK_RIGHT 283

// 0xF000 -> 0xF200 is reserved for the keyboard; a keyboard press is either:
//   - A virtual key/functional key e.g. cursor left
#define KEY_VKEY 0xF000
//   - Another printable character whose range is not included in this KEY code
#define KEY_UNICODE 0xF200

// 0xE000 -> 0xEFFF is reserved for mouse actions
#define KEY_VMOUSE 0xEFFF

#define KEY_MOUSE_START 0xE000
#define KEY_MOUSE_CLICK 0xE000
#define KEY_MOUSE_RIGHTCLICK 0xE001
#define KEY_MOUSE_MIDDLECLICK 0xE002
#define KEY_MOUSE_DOUBLE_CLICK 0xE010
#define KEY_MOUSE_LONG_CLICK 0xE020
#define KEY_MOUSE_WHEEL_UP 0xE101
#define KEY_MOUSE_WHEEL_DOWN 0xE102
#define KEY_MOUSE_MOVE 0xE103
#define KEY_MOUSE_DRAG 0xE104
#define KEY_MOUSE_DRAG_START 0xE105
#define KEY_MOUSE_DRAG_END 0xE106
#define KEY_MOUSE_RDRAG 0xE107
#define KEY_MOUSE_RDRAG_START 0xE108
#define KEY_MOUSE_RDRAG_END 0xE109
#define KEY_MOUSE_NOOP 0xEFFF
#define KEY_MOUSE_END 0xEFFF

// 0xD000 -> 0xD0FF is reserved for WM_APPCOMMAND messages
#define KEY_APPCOMMAND 0xD000

#define KEY_INVALID 0xFFFF

/// \}
