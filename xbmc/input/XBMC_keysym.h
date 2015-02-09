/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with XBMC; see the file COPYING.  If not, see
    <http://www.gnu.org/licenses/>.

    Sam Lantinga
    slouken@libsdl.org
*/

#ifndef XBMC_keysym_h
#define XBMC_keysym_h

// The XBMC_keysym identifies a physical key on the keyboard i.e. it is
// analogous to a scan code but is hardware independant.
// These values are bazsed on the SDL_keysym standards, see:
//
//   http://www.libsdl.org/tmp/SDL-1.3-docs/SDL__keysym_8h.html
//
// On SDL_KEYDOWN messages the keysym.sym will be one of these values.
//
// On OSs that don't support SDL (i.e. Windows) the OS dependant key
// handling code converts keypresses to an XBMC_keysym value.

typedef enum {
  // The keyboard syms have been cleverly chosen to map to ASCII
  XBMCK_UNKNOWN     = 0x00,
  XBMCK_FIRST       = 0x00,
  XBMCK_BACKSPACE   = 0x08,
  XBMCK_TAB         = 0x09,
  XBMCK_CLEAR       = 0x0C,
  XBMCK_RETURN      = 0x0D,
  XBMCK_PAUSE       = 0x13,
  XBMCK_ESCAPE      = 0x1B,
  XBMCK_SPACE       = 0x20,
  XBMCK_EXCLAIM     = 0x21,
  XBMCK_QUOTEDBL    = 0x22,
  XBMCK_HASH        = 0x23,
  XBMCK_DOLLAR      = 0x24,
  XBMCK_PERCENT     = 0x25,
  XBMCK_AMPERSAND   = 0x26,
  XBMCK_QUOTE       = 0x27,
  XBMCK_LEFTPAREN   = 0x28,
  XBMCK_RIGHTPAREN  = 0x29,
  XBMCK_ASTERISK    = 0x2A,
  XBMCK_PLUS        = 0x2B,
  XBMCK_COMMA       = 0x2C,
  XBMCK_MINUS       = 0x2D,
  XBMCK_PERIOD      = 0x2E,
  XBMCK_SLASH       = 0x2F,
  XBMCK_0           = 0x30,
  XBMCK_1           = 0x31,
  XBMCK_2           = 0x32,
  XBMCK_3           = 0x33,
  XBMCK_4           = 0x34,
  XBMCK_5           = 0x35,
  XBMCK_6           = 0x36,
  XBMCK_7           = 0x37,
  XBMCK_8           = 0x38,
  XBMCK_9           = 0x39,
  XBMCK_COLON       = 0x3A,
  XBMCK_SEMICOLON   = 0x3B,
  XBMCK_LESS        = 0x3C,
  XBMCK_EQUALS      = 0x3D,
  XBMCK_GREATER     = 0x3E,
  XBMCK_QUESTION    = 0x3F,
  XBMCK_AT          = 0x40,
  // Skip uppercase letters
  XBMCK_LEFTBRACKET = 0x5B,
  XBMCK_BACKSLASH   = 0x5C,
  XBMCK_RIGHTBRACKET = 0x5D,
  XBMCK_CARET       = 0x5E,
  XBMCK_UNDERSCORE  = 0x5F,
  XBMCK_BACKQUOTE   = 0x60,
  XBMCK_a           = 0x61,
  XBMCK_b           = 0x62,
  XBMCK_c           = 0x63,
  XBMCK_d           = 0x64,
  XBMCK_e           = 0x65,
  XBMCK_f           = 0x66,
  XBMCK_g           = 0x67,
  XBMCK_h           = 0x68,
  XBMCK_i           = 0x69,
  XBMCK_j           = 0x6A,
  XBMCK_k           = 0x6B,
  XBMCK_l           = 0x6C,
  XBMCK_m           = 0x6D,
  XBMCK_n           = 0x6E,
  XBMCK_o           = 0x6F,
  XBMCK_p           = 0x70,
  XBMCK_q           = 0x71,
  XBMCK_r           = 0x72,
  XBMCK_s           = 0x73,
  XBMCK_t           = 0x74,
  XBMCK_u           = 0x75,
  XBMCK_v           = 0x76,
  XBMCK_w           = 0x77,
  XBMCK_x           = 0x78,
  XBMCK_y           = 0x79,
  XBMCK_z           = 0x7A,
  XBMCK_LEFTBRACE   = 0x7b,
  XBMCK_PIPE        = 0x7C,
  XBMCK_RIGHTBRACE  = 0x7D,
  XBMCK_TILDE       = 0x7E,
  XBMCK_DELETE = 0x7F,
  // End of ASCII mapped keysyms

  // Multimedia keys
  // These are the Windows VK_ codes. SDL doesn't define codes for
  // these keys.
  XBMCK_BROWSER_BACK         = 0xA6,
  XBMCK_BROWSER_FORWARD      = 0xA7,
  XBMCK_BROWSER_REFRESH      = 0xA8,
  XBMCK_BROWSER_STOP         = 0xA9,
  XBMCK_BROWSER_SEARCH       = 0xAA,
  XBMCK_BROWSER_FAVORITES    = 0xAB,
  XBMCK_BROWSER_HOME         = 0xAC,
  XBMCK_VOLUME_MUTE          = 0xAD,
  XBMCK_VOLUME_DOWN          = 0xAE,
  XBMCK_VOLUME_UP            = 0xAF,
  XBMCK_MEDIA_NEXT_TRACK     = 0xB0,
  XBMCK_MEDIA_PREV_TRACK     = 0xB1,
  XBMCK_MEDIA_STOP           = 0xB2,
  XBMCK_MEDIA_PLAY_PAUSE     = 0xB3,
  XBMCK_LAUNCH_MAIL          = 0xB4,
  XBMCK_LAUNCH_MEDIA_SELECT  = 0xB5,
  XBMCK_LAUNCH_APP1          = 0xB6,
  XBMCK_LAUNCH_APP2          = 0xB7,
  XBMCK_LAUNCH_FILE_BROWSER  = 0xB8,
  XBMCK_LAUNCH_MEDIA_CENTER  = 0xB9,
  XBMCK_MEDIA_REWIND         = 0xBA,
  XBMCK_MEDIA_FASTFORWARD    = 0xBB,

  // Numeric keypad
  XBMCK_KP0         = 0x100,
  XBMCK_KP1         = 0x101,
  XBMCK_KP2         = 0x102,
  XBMCK_KP3         = 0x103,
  XBMCK_KP4         = 0x104,
  XBMCK_KP5         = 0x105,
  XBMCK_KP6         = 0x106,
  XBMCK_KP7         = 0x107,
  XBMCK_KP8         = 0x108,
  XBMCK_KP9         = 0x109,
  XBMCK_KP_PERIOD   = 0x10A,
  XBMCK_KP_DIVIDE   = 0x10B,
  XBMCK_KP_MULTIPLY = 0x10C,
  XBMCK_KP_MINUS    = 0x10D,
  XBMCK_KP_PLUS     = 0x10E,
  XBMCK_KP_ENTER    = 0x10F,
  XBMCK_KP_EQUALS   = 0x110,

  // Arrows + Home/End pad
  XBMCK_UP          = 0x111,
  XBMCK_DOWN        = 0x112,
  XBMCK_RIGHT       = 0x113,
  XBMCK_LEFT        = 0x114,
  XBMCK_INSERT      = 0x115,
  XBMCK_HOME        = 0x116,
  XBMCK_END         = 0x117,
  XBMCK_PAGEUP      = 0x118,
  XBMCK_PAGEDOWN    = 0x119,

  // Function keys
  XBMCK_F1          = 0x11A,
  XBMCK_F2          = 0x11B,
  XBMCK_F3          = 0x11C,
  XBMCK_F4          = 0x11D,
  XBMCK_F5          = 0x11E,
  XBMCK_F6          = 0x11F,
  XBMCK_F7          = 0x120,
  XBMCK_F8          = 0x121,
  XBMCK_F9          = 0x122,
  XBMCK_F10         = 0x123,
  XBMCK_F11         = 0x124,
  XBMCK_F12         = 0x125,
  XBMCK_F13         = 0x126,
  XBMCK_F14         = 0x127,
  XBMCK_F15         = 0x128,

  // Key state modifier keys
  XBMCK_NUMLOCK     = 0x12C,
  XBMCK_CAPSLOCK    = 0x12D,
  XBMCK_SCROLLOCK   = 0x12E,
  XBMCK_RSHIFT      = 0x12F,
  XBMCK_LSHIFT      = 0x130,
  XBMCK_RCTRL       = 0x131,
  XBMCK_LCTRL       = 0x132,
  XBMCK_RALT        = 0x133,
  XBMCK_LALT        = 0x134,
  XBMCK_RMETA       = 0x135,
  XBMCK_LMETA       = 0x136,
  XBMCK_LSUPER      = 0x137,    // Left "Windows" key
  XBMCK_RSUPER      = 0x138,    // Right "Windows" key
  XBMCK_MODE        = 0x139,    // "Alt Gr" key
  XBMCK_COMPOSE     = 0x13A,    // Multi-key compose key

  // Miscellaneous function keys
  XBMCK_HELP        = 0x13B,
  XBMCK_PRINT       = 0x13C,
  XBMCK_SYSREQ      = 0x13D,
  XBMCK_BREAK       = 0x13E,
  XBMCK_MENU        = 0x13F,
  XBMCK_POWER       = 0x140,    // Power Macintosh power key
  XBMCK_EURO        = 0x141,    // Some european keyboards
  XBMCK_UNDO        = 0x142,    // Atari keyboard has Undo
  XBMCK_SLEEP       = 0x143,    // Sleep button on Nyxboard remote (and others?)
  XBMCK_GUIDE       = 0x144,
  XBMCK_SETTINGS    = 0x145,
  XBMCK_INFO        = 0x146,
  XBMCK_RED         = 0x147,
  XBMCK_GREEN       = 0x148,
  XBMCK_YELLOW      = 0x149,
  XBMCK_BLUE        = 0x14a,

  // Add any other keys here

	/* Media keys */
  XBMCK_EJECT             = 333,
  XBMCK_STOP              = 337,
  XBMCK_RECORD            = 338,
  XBMCK_REWIND            = 339,
  XBMCK_PHONE             = 340,
  XBMCK_PLAY              = 341,
  XBMCK_SHUFFLE           = 342,
  XBMCK_FASTFORWARD       = 343,

  XBMCK_LAST
} XBMCKey;

// Enumeration of valid key mods (possibly OR'd together)
typedef enum {
  XBMCKMOD_NONE     = 0x0000,
  XBMCKMOD_LSHIFT   = 0x0001,
  XBMCKMOD_RSHIFT   = 0x0002,
  XBMCKMOD_LSUPER   = 0x0010,
  XBMCKMOD_RSUPER   = 0x0020,
  XBMCKMOD_LCTRL    = 0x0040,
  XBMCKMOD_RCTRL    = 0x0080,
  XBMCKMOD_LALT     = 0x0100,
  XBMCKMOD_RALT     = 0x0200,
  XBMCKMOD_LMETA    = 0x0400,
  XBMCKMOD_RMETA    = 0x0800,
  XBMCKMOD_NUM      = 0x1000,
  XBMCKMOD_CAPS     = 0x2000,
  XBMCKMOD_MODE     = 0x4000,
  XBMCKMOD_RESERVED = 0x8000
} XBMCMod;

#define XBMCKMOD_CTRL  (XBMCKMOD_LCTRL  | XBMCKMOD_RCTRL)
#define XBMCKMOD_SHIFT (XBMCKMOD_LSHIFT | XBMCKMOD_RSHIFT)
#define XBMCKMOD_ALT   (XBMCKMOD_LALT   | XBMCKMOD_RALT)
#define XBMCKMOD_META  (XBMCKMOD_LMETA  | XBMCKMOD_RMETA)
#define XBMCKMOD_SUPER (XBMCKMOD_LSUPER | XBMCKMOD_RSUPER)

#endif // XBMC_keysym_h
