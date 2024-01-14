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

/* Include file for SDL keyboard event handling */

#include "XBMC_keysym.h"

#include <stdint.h>

/// \ingroup keyboard
/// \{

/* Keysym structure
   - The scancode is hardware dependent, and should not be used by general
     applications.  If no hardware scancode is available, it will be 0.

   - The 'unicode' translated character is only available when character
     translation is enabled by the XBMC_EnableUNICODE() API.  If non-zero,
     this is a UNICODE character corresponding to the keypress.  If the
     high 9 bits of the character are 0, then this maps to the equivalent
     ASCII character:
        char ch;
        if ( (keysym.unicode & 0xFF80) == 0 ) {
                ch = keysym.unicode & 0x7F;
        } else {
                An international character..
        }
 */
struct XBMC_keysym
{
  unsigned char scancode; /* hardware specific scancode */
  XBMCKey sym; /* SDL virtual keysym */
  XBMCMod mod; /* current key modifiers */
  uint16_t unicode; /* translated character */
};

/* This is the mask which refers to all hotkey bindings */
#define XBMC_ALL_HOTKEYS 0xFFFFFFFF

/// \}
