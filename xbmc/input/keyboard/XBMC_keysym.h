/*
 *  SDL - Simple DirectMedia Layer
 *  Copyright (C) 1997-2009 Sam Lantinga
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 *
 *  Sam Lantinga
 *  slouken@libsdl.org
 */

#pragma once

/// \ingroup keyboard
/// \{

// The XBMC_keysym identifies a physical key on the keyboard i.e. it is
// analogous to a scan code but is hardware independent.
// These values are based on the SDL_keysym standards, see:
//
//   http://www.libsdl.org/tmp/SDL-1.3-docs/SDL__keysym_8h.html
//
// On SDL_KEYDOWN messages the keysym.sym will be one of these values.
//
// On OSs that don't support SDL (i.e. Windows) the OS dependent key
// handling code converts keypresses to an XBMC_keysym value.

enum XBMCKey
{
  // The keyboard syms have been cleverly chosen to map to ASCII
  XBMCK_UNKNOWN = 0x00,
  XBMCK_FIRST = 0x00,
  XBMCK_CTRLF = 0x06,
  XBMCK_BACKSPACE = 0x08,
  XBMCK_TAB = 0x09,
  XBMCK_CLEAR = 0x0C,
  XBMCK_RETURN = 0x0D,
  XBMCK_PAUSE = 0x13,
  XBMCK_ESCAPE = 0x1B,
  XBMCK_SPACE = 0x20,
  XBMCK_EXCLAIM = 0x21,
  XBMCK_QUOTEDBL = 0x22,
  XBMCK_HASH = 0x23,
  XBMCK_DOLLAR = 0x24,
  XBMCK_PERCENT = 0x25,
  XBMCK_AMPERSAND = 0x26,
  XBMCK_QUOTE = 0x27,
  XBMCK_LEFTPAREN = 0x28,
  XBMCK_RIGHTPAREN = 0x29,
  XBMCK_ASTERISK = 0x2A,
  XBMCK_PLUS = 0x2B,
  XBMCK_COMMA = 0x2C,
  XBMCK_MINUS = 0x2D,
  XBMCK_PERIOD = 0x2E,
  XBMCK_SLASH = 0x2F,
  XBMCK_0 = 0x30,
  XBMCK_1 = 0x31,
  XBMCK_2 = 0x32,
  XBMCK_3 = 0x33,
  XBMCK_4 = 0x34,
  XBMCK_5 = 0x35,
  XBMCK_6 = 0x36,
  XBMCK_7 = 0x37,
  XBMCK_8 = 0x38,
  XBMCK_9 = 0x39,
  XBMCK_COLON = 0x3A,
  XBMCK_SEMICOLON = 0x3B,
  XBMCK_LESS = 0x3C,
  XBMCK_EQUALS = 0x3D,
  XBMCK_GREATER = 0x3E,
  XBMCK_QUESTION = 0x3F,
  XBMCK_AT = 0x40,
  // Skip uppercase letters
  XBMCK_LEFTBRACKET = 0x5B,
  XBMCK_BACKSLASH = 0x5C,
  XBMCK_RIGHTBRACKET = 0x5D,
  XBMCK_CARET = 0x5E,
  XBMCK_UNDERSCORE = 0x5F,
  XBMCK_BACKQUOTE = 0x60,
  XBMCK_a = 0x61,
  XBMCK_b = 0x62,
  XBMCK_c = 0x63,
  XBMCK_d = 0x64,
  XBMCK_e = 0x65,
  XBMCK_f = 0x66,
  XBMCK_g = 0x67,
  XBMCK_h = 0x68,
  XBMCK_i = 0x69,
  XBMCK_j = 0x6A,
  XBMCK_k = 0x6B,
  XBMCK_l = 0x6C,
  XBMCK_m = 0x6D,
  XBMCK_n = 0x6E,
  XBMCK_o = 0x6F,
  XBMCK_p = 0x70,
  XBMCK_q = 0x71,
  XBMCK_r = 0x72,
  XBMCK_s = 0x73,
  XBMCK_t = 0x74,
  XBMCK_u = 0x75,
  XBMCK_v = 0x76,
  XBMCK_w = 0x77,
  XBMCK_x = 0x78,
  XBMCK_y = 0x79,
  XBMCK_z = 0x7A,
  XBMCK_LEFTBRACE = 0x7b,
  XBMCK_PIPE = 0x7C,
  XBMCK_RIGHTBRACE = 0x7D,
  XBMCK_TILDE = 0x7E,
  XBMCK_DELETE = 0x7F,
  // End of ASCII mapped keysyms

  // Multimedia keys
  // These are the Windows VK_ codes. SDL doesn't define codes for
  // these keys.
  XBMCK_BROWSER_BACK = 0xA6,
  XBMCK_BROWSER_FORWARD = 0xA7,
  XBMCK_BROWSER_REFRESH = 0xA8,
  XBMCK_BROWSER_STOP = 0xA9,
  XBMCK_BROWSER_SEARCH = 0xAA,
  XBMCK_BROWSER_FAVORITES = 0xAB,
  XBMCK_BROWSER_HOME = 0xAC,
  XBMCK_VOLUME_MUTE = 0xAD,
  XBMCK_VOLUME_DOWN = 0xAE,
  XBMCK_VOLUME_UP = 0xAF,
  XBMCK_MEDIA_NEXT_TRACK = 0xB0,
  XBMCK_MEDIA_PREV_TRACK = 0xB1,
  XBMCK_MEDIA_STOP = 0xB2,
  XBMCK_MEDIA_PLAY_PAUSE = 0xB3,
  XBMCK_LAUNCH_MAIL = 0xB4,
  XBMCK_LAUNCH_MEDIA_SELECT = 0xB5,
  XBMCK_LAUNCH_APP1 = 0xB6,
  XBMCK_LAUNCH_APP2 = 0xB7,
  XBMCK_LAUNCH_FILE_BROWSER = 0xB8,
  XBMCK_LAUNCH_MEDIA_CENTER = 0xB9,
  XBMCK_MEDIA_REWIND = 0xBA,
  XBMCK_MEDIA_FASTFORWARD = 0xBB,

  // This key is not present on standard US keyboard layouts. For European
  // layouts it's usually located to the right of left-shift key, with '\' as
  // its main function.
  XBMCK_OEM_102 = 0xE2,

  // Numeric keypad
  XBMCK_KP0 = 0x100,
  XBMCK_KP1 = 0x101,
  XBMCK_KP2 = 0x102,
  XBMCK_KP3 = 0x103,
  XBMCK_KP4 = 0x104,
  XBMCK_KP5 = 0x105,
  XBMCK_KP6 = 0x106,
  XBMCK_KP7 = 0x107,
  XBMCK_KP8 = 0x108,
  XBMCK_KP9 = 0x109,
  XBMCK_KP_PERIOD = 0x10A,
  XBMCK_KP_DIVIDE = 0x10B,
  XBMCK_KP_MULTIPLY = 0x10C,
  XBMCK_KP_MINUS = 0x10D,
  XBMCK_KP_PLUS = 0x10E,
  XBMCK_KP_ENTER = 0x10F,
  XBMCK_KP_EQUALS = 0x110,

  // Arrows + Home/End pad
  XBMCK_UP = 0x111,
  XBMCK_DOWN = 0x112,
  XBMCK_RIGHT = 0x113,
  XBMCK_LEFT = 0x114,
  XBMCK_INSERT = 0x115,
  XBMCK_HOME = 0x116,
  XBMCK_END = 0x117,
  XBMCK_PAGEUP = 0x118,
  XBMCK_PAGEDOWN = 0x119,

  // Function keys
  XBMCK_F1 = 0x11A,
  XBMCK_F2 = 0x11B,
  XBMCK_F3 = 0x11C,
  XBMCK_F4 = 0x11D,
  XBMCK_F5 = 0x11E,
  XBMCK_F6 = 0x11F,
  XBMCK_F7 = 0x120,
  XBMCK_F8 = 0x121,
  XBMCK_F9 = 0x122,
  XBMCK_F10 = 0x123,
  XBMCK_F11 = 0x124,
  XBMCK_F12 = 0x125,
  XBMCK_F13 = 0x126,
  XBMCK_F14 = 0x127,
  XBMCK_F15 = 0x128,

  // Key state modifier keys
  XBMCK_NUMLOCK = 0x12C,
  XBMCK_CAPSLOCK = 0x12D,
  XBMCK_SCROLLOCK = 0x12E,
  XBMCK_RSHIFT = 0x12F,
  XBMCK_LSHIFT = 0x130,
  XBMCK_RCTRL = 0x131,
  XBMCK_LCTRL = 0x132,
  XBMCK_RALT = 0x133,
  XBMCK_LALT = 0x134,
  XBMCK_RMETA = 0x135,
  XBMCK_LMETA = 0x136,
  XBMCK_LSUPER = 0x137, // Left "Windows" key
  XBMCK_RSUPER = 0x138, // Right "Windows" key
  XBMCK_MODE = 0x139, // "Alt Gr" key
  XBMCK_COMPOSE = 0x13A, // Multi-key compose key

  // Miscellaneous function keys
  XBMCK_HELP = 0x13B,
  XBMCK_PRINT = 0x13C,
  XBMCK_SYSREQ = 0x13D,
  XBMCK_BREAK = 0x13E,
  XBMCK_MENU = 0x13F,
  XBMCK_POWER = 0x140, // Power Macintosh power key
  XBMCK_EURO = 0x141, // Some european keyboards
  XBMCK_UNDO = 0x142, // Atari keyboard has Undo
  XBMCK_SLEEP = 0x143, // Sleep button on Nyxboard remote (and others?)
  XBMCK_GUIDE = 0x144,
  XBMCK_SETTINGS = 0x145,
  XBMCK_INFO = 0x146,
  XBMCK_RED = 0x147,
  XBMCK_GREEN = 0x148,
  XBMCK_YELLOW = 0x149,
  XBMCK_BLUE = 0x14a,
  XBMCK_ZOOM = 0x14b,
  XBMCK_TEXT = 0x14c,
  XBMCK_FAVORITES = 0x14d,
  XBMCK_HOMEPAGE = 0x14e,
  XBMCK_CONFIG = 0x14f,
  XBMCK_EPG = 0x150,

  // Add any other keys here

  /* Dead keys */
  XBMCK_GRAVE = 0x0060,
  XBMCK_ACUTE = 0x00B4,
  XBMCK_CIRCUMFLEX = 0x005E,
  XBMCK_PERISPOMENI = 0x1FC0,
  XBMCK_MACRON = 0x00AF,
  XBMCK_BREVE = 0x02D8,
  XBMCK_ABOVEDOT = 0x02D9,
  XBMCK_DIAERESIS = 0x00A8,
  XBMCK_ABOVERING = 0x02DA,
  XBMCK_DOUBLEACUTE = 0x030B,
  XBMCK_CARON = 0x030C,
  XBMCK_CEDILLA = 0x0327,
  XBMCK_OGONEK = 0x0328,
  XBMCK_IOTA = 0x0345,
  XBMCK_VOICESOUND = 0x3099,
  XBMCK_SEMIVOICESOUND = 0x309A,
  XBMCK_BELOWDOT = 0x0323,
  XBMCK_HOOK = 0x0309,
  XBMCK_HORN = 0x031B,
  XBMCK_STROKE = 0x0335,
  XBMCK_ABOVECOMMA = 0x0313,
  XBMCK_ABOVEREVERSEDCOMMA = 0x0314,
  XBMCK_DOUBLEGRAVE = 0x30F,
  XBMCK_BELOWRING = 0x325,
  XBMCK_BELOWMACRON = 0x331,
  XBMCK_BELOWCIRCUMFLEX = 0x32D,
  XBMCK_BELOWTILDE = 0x330,
  XBMCK_BELOWBREVE = 0x32e,
  XBMCK_BELOWDIAERESIS = 0x324,
  XBMCK_INVERTEDBREVE = 0x32f,
  XBMCK_BELOWCOMMA = 0x326,
  XBMCK_LOWLINE = 0x332,
  XBMCK_ABOVEVERTICALLINE = 0x30D,
  XBMCK_BELOWVERTICALLINE = 0x329,
  XBMCK_LONGSOLIDUSOVERLAY = 0x338,
  XBMCK_DEAD_A = 0x363,
  XBMCK_DEAD_E = 0x364,
  XBMCK_DEAD_I = 0x365,
  XBMCK_DEAD_O = 0x366,
  XBMCK_DEAD_U = 0x367,
  XBMCK_SCHWA = 0x1DEA,

  /* Media keys */
  XBMCK_STOP = 337,
  XBMCK_RECORD = 338,
  XBMCK_REWIND = 339,
  XBMCK_PHONE = 340,
  XBMCK_PLAY = 341,
  XBMCK_SHUFFLE = 342,
  XBMCK_FASTFORWARD = 343,
  XBMCK_EJECT = 344,

  /*
   * Extended keypad key symbols for Num Lock state-specific behaviors
   *
   * These key symbols address the dual behavior of the numeric keypad's keys,
   * observed within the context of the Linux platform, using the XKB common
   * library.
   *
   * Depending on the Num Lock state — enabled or disabled — the key on the
   * numeric keypad emits different keycodes.
   */
  XBMCK_XKB_KP_HOME = 0xFF95, // XBMCK_KP7
  XBMCK_XKB_KP_LEFT = 0xFF96, // XBMCK_KP4
  XBMCK_XKB_KP_UP = 0xFF97, // XBMCK_KP8
  XBMCK_XKB_KP_RIGHT = 0xFF98, // XBMCK_KP6
  XBMCK_XKB_KP_DOWN = 0xFF99, // XBMCK_KP2
  XBMCK_XKB_KP_PAGE_UP = 0xFF9A, // XBMCK_KP9
  XBMCK_XKB_KP_PAGE_DOWN = 0xFF9B, // XBMCK_KP3
  XBMCK_XKB_KP_END = 0xFF9C, // XBMCK_KP1
  XBMCK_XKB_KP_BEGIN = 0xFF9D, // XBMCK_KP5
  XBMCK_XKB_KP_INSERT = 0xFF9E, // XBMCK_KP0
  XBMCK_XKB_KP_DELETE = 0xFF9F, // XBMCK_KP_PERIOD

  XBMCK_XKB_KP_DECIMAL = 0xFFAE, // XBMCK_KP_PERIOD
  XBMCK_XKB_KP0 = 0xFFB0, // XBMCK_KP0
  XBMCK_XKB_KP1 = 0xFFB1, // XBMCK_KP1
  XBMCK_XKB_KP2 = 0xFFB2, // XBMCK_KP2
  XBMCK_XKB_KP3 = 0xFFB3, // XBMCK_KP3
  XBMCK_XKB_KP4 = 0xFFB4, // XBMCK_KP4
  XBMCK_XKB_KP5 = 0xFFB5, // XBMCK_KP5
  XBMCK_XKB_KP6 = 0xFFB6, // XBMCK_KP6
  XBMCK_XKB_KP7 = 0xFFB7, // XBMCK_KP7
  XBMCK_XKB_KP8 = 0xFFB8, // XBMCK_KP8
  XBMCK_XKB_KP9 = 0xFFB9, // XBMCK_KP9

  /*
   * Extended keypad key symbols for Num Lock state-independent behaviors
   *
   * These key symbols address the behavior of the numeric keypad's keys,
   * observed within the context of the Linux platform, using the XKB common
   * library.
   *
   * The keycodes emitted by these keys are independent of the Num Lock state.
   */
  XBMCK_XKB_KP_SPACE = 0xFF80,
  XBMCK_XKB_KP_TAB = 0xFF89,
  XBMCK_XKB_KP_ENTER = 0xFF8D, // XBMCK_KP_ENTER
  XBMCK_XKB_KP_F1 = 0xFF91,
  XBMCK_XKB_KP_F2 = 0xFF92,
  XBMCK_XKB_KP_F3 = 0xFF93,
  XBMCK_XKB_KP_F4 = 0xFF94,
  XBMCK_XKB_KP_EQUALS = 0xFFBD, // XBMCK_KP_EQUALS
  XBMCK_XKB_KP_MULTIPLY = 0xFFAA, // XBMCK_KP_MULTIPLY
  XBMCK_XKB_KP_ADD = 0xFFAB, // XBMCK_KP_PLUS
  XBMCK_XKB_KP_SEPARATOR = 0xFFAC,
  XBMCK_XKB_KP_SUBTRACT = 0xFFAD, // XBMCK_KP_MINUS
  XBMCK_XKB_KP_DIVIDE = 0xFFAF, // XBMCK_KP_DIVIDE

  XBMCK_LAST
};

// Enumeration of valid key mods (possibly OR'd together)
enum XBMCMod
{
  XBMCKMOD_NONE = 0x0000,
  XBMCKMOD_LSHIFT = 0x0001,
  XBMCKMOD_RSHIFT = 0x0002,
  XBMCKMOD_LSUPER = 0x0010,
  XBMCKMOD_RSUPER = 0x0020,
  XBMCKMOD_LCTRL = 0x0040,
  XBMCKMOD_RCTRL = 0x0080,
  XBMCKMOD_LALT = 0x0100,
  XBMCKMOD_RALT = 0x0200,
  XBMCKMOD_LMETA = 0x0400,
  XBMCKMOD_RMETA = 0x0800,
  XBMCKMOD_NUM = 0x1000,
  XBMCKMOD_CAPS = 0x2000,
  XBMCKMOD_MODE = 0x4000,
  XBMCKMOD_RESERVED = 0x8000
};

#define XBMCKMOD_CTRL (XBMCKMOD_LCTRL | XBMCKMOD_RCTRL)
#define XBMCKMOD_SHIFT (XBMCKMOD_LSHIFT | XBMCKMOD_RSHIFT)
#define XBMCKMOD_ALT (XBMCKMOD_LALT | XBMCKMOD_RALT)
#define XBMCKMOD_META (XBMCKMOD_LMETA | XBMCKMOD_RMETA)
#define XBMCKMOD_SUPER (XBMCKMOD_LSUPER | XBMCKMOD_RSUPER)

/// \}
