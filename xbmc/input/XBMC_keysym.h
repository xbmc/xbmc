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

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifndef _XBMC_keysym_h
#define _XBMC_keysym_h

/* What we really want is a mapping of every raw key on the keyboard.
   To support international keyboards, we use the range 0xA1 - 0xFF
   as international virtual keycodes.  We'll follow in the footsteps of X11...
   The names of the keys
 */
 
typedef enum {
	/* The keyboard syms have been cleverly chosen to map to ASCII */
	XBMCK_UNKNOWN		= 0,
	XBMCK_FIRST		= 0,
	XBMCK_BACKSPACE		= 8,
	XBMCK_TAB		= 9,
	XBMCK_CLEAR		= 12,
	XBMCK_RETURN		= 13,
	XBMCK_PAUSE		= 19,
	XBMCK_ESCAPE		= 27,
	XBMCK_SPACE		= 32,
	XBMCK_EXCLAIM		= 33,
	XBMCK_QUOTEDBL		= 34,
	XBMCK_HASH		= 35,
	XBMCK_DOLLAR		= 36,
	XBMCK_AMPERSAND		= 38,
	XBMCK_QUOTE		= 39,
	XBMCK_LEFTPAREN		= 40,
	XBMCK_RIGHTPAREN		= 41,
	XBMCK_ASTERISK		= 42,
	XBMCK_PLUS		= 43,
	XBMCK_COMMA		= 44,
	XBMCK_MINUS		= 45,
	XBMCK_PERIOD		= 46,
	XBMCK_SLASH		= 47,
	XBMCK_0			= 48,
	XBMCK_1			= 49,
	XBMCK_2			= 50,
	XBMCK_3			= 51,
	XBMCK_4			= 52,
	XBMCK_5			= 53,
	XBMCK_6			= 54,
	XBMCK_7			= 55,
	XBMCK_8			= 56,
	XBMCK_9			= 57,
	XBMCK_COLON		= 58,
	XBMCK_SEMICOLON		= 59,
	XBMCK_LESS		= 60,
	XBMCK_EQUALS		= 61,
	XBMCK_GREATER		= 62,
	XBMCK_QUESTION		= 63,
	XBMCK_AT			= 64,
	/* 
	   Skip uppercase letters
	 */
	XBMCK_LEFTBRACKET	= 91,
	XBMCK_BACKSLASH		= 92,
	XBMCK_RIGHTBRACKET	= 93,
	XBMCK_CARET		= 94,
	XBMCK_UNDERSCORE		= 95,
	XBMCK_BACKQUOTE		= 96,
	XBMCK_a			= 97,
	XBMCK_b			= 98,
	XBMCK_c			= 99,
	XBMCK_d			= 100,
	XBMCK_e			= 101,
	XBMCK_f			= 102,
	XBMCK_g			= 103,
	XBMCK_h			= 104,
	XBMCK_i			= 105,
	XBMCK_j			= 106,
	XBMCK_k			= 107,
	XBMCK_l			= 108,
	XBMCK_m			= 109,
	XBMCK_n			= 110,
	XBMCK_o			= 111,
	XBMCK_p			= 112,
	XBMCK_q			= 113,
	XBMCK_r			= 114,
	XBMCK_s			= 115,
	XBMCK_t			= 116,
	XBMCK_u			= 117,
	XBMCK_v			= 118,
	XBMCK_w			= 119,
	XBMCK_x			= 120,
	XBMCK_y			= 121,
	XBMCK_z			= 122,
	XBMCK_DELETE		= 127,
	/* End of ASCII mapped keysyms */

	/* International keyboard syms */
	XBMCK_WORLD_0		= 160,		/* 0xA0 */
	XBMCK_WORLD_1		= 161,
	XBMCK_WORLD_2		= 162,
	XBMCK_WORLD_3		= 163,
	XBMCK_WORLD_4		= 164,
	XBMCK_WORLD_5		= 165,

	// Multimedia keys
	XBMCK_BROWSER_BACK        = 0xA6,
	XBMCK_BROWSER_FORWARD     = 0xA7,
	XBMCK_BROWSER_REFRESH     = 0xA8,
	XBMCK_BROWSER_STOP        = 0xA9,
	XBMCK_BROWSER_SEARCH      = 0xAA,
	XBMCK_BROWSER_FAVORITES   = 0xAB,
	XBMCK_BROWSER_HOME        = 0xAC,
	XBMCK_VOLUME_MUTE         = 0xAD,
	XBMCK_VOLUME_DOWN         = 0xAE,
	XBMCK_VOLUME_UP           = 0xAF,
	XBMCK_MEDIA_NEXT_TRACK    = 0xB0,
	XBMCK_MEDIA_PREV_TRACK    = 0xB1,
	XBMCK_MEDIA_STOP          = 0xB2,
	XBMCK_MEDIA_PLAY_PAUSE    = 0xB3,
	XBMCK_LAUNCH_MAIL         = 0xB4,
	XBMCK_LAUNCH_MEDIA_SELECT = 0xB5,
	XBMCK_LAUNCH_APP1         = 0xB6,
	XBMCK_LAUNCH_APP2         = 0xB7,

	/* International keyboard syms */
	XBMCK_WORLD_24		= 184,
	XBMCK_WORLD_25		= 185,
	XBMCK_WORLD_26		= 186,
	XBMCK_WORLD_27		= 187,
	XBMCK_WORLD_28		= 188,
	XBMCK_WORLD_29		= 189,
	XBMCK_WORLD_30		= 190,
	XBMCK_WORLD_31		= 191,
	XBMCK_WORLD_32		= 192,
	XBMCK_WORLD_33		= 193,
	XBMCK_WORLD_34		= 194,
	XBMCK_WORLD_35		= 195,
	XBMCK_WORLD_36		= 196,
	XBMCK_WORLD_37		= 197,
	XBMCK_WORLD_38		= 198,
	XBMCK_WORLD_39		= 199,
	XBMCK_WORLD_40		= 200,
	XBMCK_WORLD_41		= 201,
	XBMCK_WORLD_42		= 202,
	XBMCK_WORLD_43		= 203,
	XBMCK_WORLD_44		= 204,
	XBMCK_WORLD_45		= 205,
	XBMCK_WORLD_46		= 206,
	XBMCK_WORLD_47		= 207,
	XBMCK_WORLD_48		= 208,
	XBMCK_WORLD_49		= 209,
	XBMCK_WORLD_50		= 210,
	XBMCK_WORLD_51		= 211,
	XBMCK_WORLD_52		= 212,
	XBMCK_WORLD_53		= 213,
	XBMCK_WORLD_54		= 214,
	XBMCK_WORLD_55		= 215,
	XBMCK_WORLD_56		= 216,
	XBMCK_WORLD_57		= 217,
	XBMCK_WORLD_58		= 218,
	XBMCK_WORLD_59		= 219,
	XBMCK_WORLD_60		= 220,
	XBMCK_WORLD_61		= 221,
	XBMCK_WORLD_62		= 222,
	XBMCK_WORLD_63		= 223,
	XBMCK_WORLD_64		= 224,
	XBMCK_WORLD_65		= 225,
	XBMCK_WORLD_66		= 226,
	XBMCK_WORLD_67		= 227,
	XBMCK_WORLD_68		= 228,
	XBMCK_WORLD_69		= 229,
	XBMCK_WORLD_70		= 230,
	XBMCK_WORLD_71		= 231,
	XBMCK_WORLD_72		= 232,
	XBMCK_WORLD_73		= 233,
	XBMCK_WORLD_74		= 234,
	XBMCK_WORLD_75		= 235,
	XBMCK_WORLD_76		= 236,
	XBMCK_WORLD_77		= 237,
	XBMCK_WORLD_78		= 238,
	XBMCK_WORLD_79		= 239,
	XBMCK_WORLD_80		= 240,
	XBMCK_WORLD_81		= 241,
	XBMCK_WORLD_82		= 242,
	XBMCK_WORLD_83		= 243,
	XBMCK_WORLD_84		= 244,
	XBMCK_WORLD_85		= 245,
	XBMCK_WORLD_86		= 246,
	XBMCK_WORLD_87		= 247,
	XBMCK_WORLD_88		= 248,
	XBMCK_WORLD_89		= 249,
	XBMCK_WORLD_90		= 250,
	XBMCK_WORLD_91		= 251,
	XBMCK_WORLD_92		= 252,
	XBMCK_WORLD_93		= 253,
	XBMCK_WORLD_94		= 254,
	XBMCK_WORLD_95		= 255,		/* 0xFF */

	/* Numeric keypad */
	XBMCK_KP0		= 256,
	XBMCK_KP1		= 257,
	XBMCK_KP2		= 258,
	XBMCK_KP3		= 259,
	XBMCK_KP4		= 260,
	XBMCK_KP5		= 261,
	XBMCK_KP6		= 262,
	XBMCK_KP7		= 263,
	XBMCK_KP8		= 264,
	XBMCK_KP9		= 265,
	XBMCK_KP_PERIOD		= 266,
	XBMCK_KP_DIVIDE		= 267,
	XBMCK_KP_MULTIPLY	= 268,
	XBMCK_KP_MINUS		= 269,
	XBMCK_KP_PLUS		= 270,
	XBMCK_KP_ENTER		= 271,
	XBMCK_KP_EQUALS		= 272,

	/* Arrows + Home/End pad */
	XBMCK_UP			= 273,
	XBMCK_DOWN		= 274,
	XBMCK_RIGHT		= 275,
	XBMCK_LEFT		= 276,
	XBMCK_INSERT		= 277,
	XBMCK_HOME		= 278,
	XBMCK_END		= 279,
	XBMCK_PAGEUP		= 280,
	XBMCK_PAGEDOWN		= 281,

	/* Function keys */
	XBMCK_F1			= 282,
	XBMCK_F2			= 283,
	XBMCK_F3			= 284,
	XBMCK_F4			= 285,
	XBMCK_F5			= 286,
	XBMCK_F6			= 287,
	XBMCK_F7			= 288,
	XBMCK_F8			= 289,
	XBMCK_F9			= 290,
	XBMCK_F10		= 291,
	XBMCK_F11		= 292,
	XBMCK_F12		= 293,
	XBMCK_F13		= 294,
	XBMCK_F14		= 295,
	XBMCK_F15		= 296,

	/* Key state modifier keys */
	XBMCK_NUMLOCK		= 300,
	XBMCK_CAPSLOCK		= 301,
	XBMCK_SCROLLOCK		= 302,
	XBMCK_RSHIFT		= 303,
	XBMCK_LSHIFT		= 304,
	XBMCK_RCTRL		= 305,
	XBMCK_LCTRL		= 306,
	XBMCK_RALT		= 307,
	XBMCK_LALT		= 308,
	XBMCK_RMETA		= 309,
	XBMCK_LMETA		= 310,
	XBMCK_LSUPER		= 311,		/* Left "Windows" key */
	XBMCK_RSUPER		= 312,		/* Right "Windows" key */
	XBMCK_MODE		= 313,		/* "Alt Gr" key */
	XBMCK_COMPOSE		= 314,		/* Multi-key compose key */

	/* Miscellaneous function keys */
	XBMCK_HELP		= 315,
	XBMCK_PRINT		= 316,
	XBMCK_SYSREQ		= 317,
	XBMCK_BREAK		= 318,
	XBMCK_MENU		= 319,
	XBMCK_POWER		= 320,		/* Power Macintosh power key */
	XBMCK_EURO		= 321,		/* Some european keyboards */
	XBMCK_UNDO		= 322,		/* Atari keyboard has Undo */

	/* Add any other keys here */

	XBMCK_LAST
} XBMCKey;

/* Enumeration of valid key mods (possibly OR'd together) */
typedef enum {
	XBMCKMOD_NONE  = 0x0000,
	XBMCKMOD_LSHIFT= 0x0001,
	XBMCKMOD_RSHIFT= 0x0002,
	XBMCKMOD_LSUPER= 0x0010,
	XBMCKMOD_RSUPER= 0x0020,
	XBMCKMOD_LCTRL = 0x0040,
	XBMCKMOD_RCTRL = 0x0080,
	XBMCKMOD_LALT  = 0x0100,
	XBMCKMOD_RALT  = 0x0200,
	XBMCKMOD_LMETA = 0x0400,
	XBMCKMOD_RMETA = 0x0800,
	XBMCKMOD_NUM   = 0x1000,
	XBMCKMOD_CAPS  = 0x2000,
	XBMCKMOD_MODE  = 0x4000,
	XBMCKMOD_RESERVED = 0x8000
} XBMCMod;

#define XBMCKMOD_CTRL	(XBMCKMOD_LCTRL|XBMCKMOD_RCTRL)
#define XBMCKMOD_SHIFT	(XBMCKMOD_LSHIFT|XBMCKMOD_RSHIFT)
#define XBMCKMOD_ALT	(XBMCKMOD_LALT|XBMCKMOD_RALT)
#define XBMCKMOD_META	(XBMCKMOD_LMETA|XBMCKMOD_RMETA)
#define XBMCKMOD_SUPER	(XBMCKMOD_LSUPER|XBMCKMOD_RSUPER)

#endif /* _XBMC_keysym_h */
