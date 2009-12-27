r/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id: carbontoprojectM.hpp,v 1.2 2004/11/12 15:12:58 cvs Exp $
 *
 * Translates CARBON -> projectM variables
 *
 * $Log$
 */

#ifndef _CARBONTOPROJECTM_H
#define _CARBONTOPROJECTM_H

#include "event.h"

#ifdef WIN32
#else
#endif

projectMEvent carbon2pmEvent( EventRecord *event ) { \
\
    switch ( event->what ) { \
        case updateEvt: \
            return PROJECTM_VIDEORESIZE; \
        case keyUp: \
            return PROJECTM_KEYUP; \
        case keyDown: \
            return PROJECTM_KEYDOWN; \
        default:
            return PROJECTM_KEYUP; \
      } \
  } \

projectMKeycode carbon2pmKeycode( EventRecord *event ) { \
    projectMKeycode char_code = (projectMKeycode)(event->message & charCodeMask); \
    switch ( char_code ) { \
        case kFunctionKeyCharCode: { \
            switch ( ( event->message << 16 ) >> 24 ) { \
                case 111: { \
                    return PROJECTM_K_F12; \
                  } \
                case 103: { \
                    return PROJECTM_K_F11; \
                  } \
                case 109: { \
                    return PROJECTM_K_F10; \
                  } \
                case 101: { \
                    return PROJECTM_K_F9; \
                  } \
                case 100: { \
                    return PROJECTM_K_F8; \
                  } \
                case 98: { \
                    return PROJECTM_K_F7; \
                  } \
                case 97: { \
                    return PROJECTM_K_F6; \
                  } \
                case 96: { \
                    return PROJECTM_K_F5; \
                  } \
                case 118: { \
                    return PROJECTM_K_F4; \
                  } \
                case 99: { \
                    return PROJECTM_K_F3; \
                  } \
                case 120: { \
                    return PROJECTM_K_F2; \
                  } \
                case 122: { \
                    return PROJECTM_K_F1; \
                  } \
              } \
          } \
        default: { \
            return char_code; \
          } \
      } \
  } \

projectMModifier carbon2pmModifier( EventRecord *event ) { \
    return (projectMModifier)PROJECTM_K_LSHIFT; \
  } \

#endif /** _CARBONTOPROJECTM_H */
