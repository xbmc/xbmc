/**
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
 * $Id: sdltoprojectM.hpp,v 1.1 2004/10/08 00:35:28 cvs Exp $
 *
 * Translates SDL -> projectM variables
 *
 * $Log: sdltoprojectM.hpp,v $
 * Revision 1.1  2004/10/08 00:35:28  cvs
 * Moved and imported
 *
 * Revision 1.1.1.1  2004/10/04 12:56:00  cvs
 * Imported
 *
 */

#ifndef _SDLTOPROJECTM_H
#define _SDLTOPROJECTM_H

#include "event.h"

 //#include "projectM/projectM.hpp"
#ifdef WIN32
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

inline projectMEvent sdl2pmEvent( SDL_Event event ) { \
							
    switch ( event.type ) { \
        case SDL_VIDEORESIZE:
            return PROJECTM_VIDEORESIZE; \
        case SDL_KEYUP: \
            return PROJECTM_KEYUP; \
        case SDL_KEYDOWN: \
            return PROJECTM_KEYDOWN; \
        default:
            return PROJECTM_KEYUP; \
      } \
  } \

inline projectMKeycode sdl2pmKeycode( SDLKey keysym ) { \
    switch ( keysym ) { \
        case SDLK_F1: \
            return PROJECTM_K_F1; \
        case SDLK_F2: \
            return PROJECTM_K_F2; \
        case SDLK_F3: \
            return PROJECTM_K_F3; \
        case SDLK_F4: \
            return PROJECTM_K_F4; \
        case SDLK_F5: \
            return PROJECTM_K_F5; \
        case SDLK_F6: \
            return PROJECTM_K_F6; \
        case SDLK_F7: \
            return PROJECTM_K_F7; \
        case SDLK_F8: \
            return PROJECTM_K_F8; \
        case SDLK_F9: \
            return PROJECTM_K_F9; \
        case SDLK_F10: \
            return PROJECTM_K_F10; \
        case SDLK_F11: \
            return PROJECTM_K_F11; \
        case SDLK_F12: \
            return PROJECTM_K_F12; \
	  case SDLK_ESCAPE: \
	    return PROJECTM_K_ESCAPE; 
    case SDLK_a:
      return PROJECTM_K_a;
    case SDLK_b:
      return PROJECTM_K_b;
    case SDLK_c:  
      return PROJECTM_K_c;
    case SDLK_d: 
      return PROJECTM_K_d; 
    case SDLK_e:
      return PROJECTM_K_e; 
    case SDLK_f: 
      return PROJECTM_K_f; 
    case SDLK_g: 
      return PROJECTM_K_g; 
    case SDLK_h: 
      return PROJECTM_K_h; 
    case SDLK_i: 
      return PROJECTM_K_i; 
    case SDLK_j:
      return PROJECTM_K_j;
    case SDLK_k:
      return PROJECTM_K_k;
    case SDLK_l:  
      return PROJECTM_K_l;
    case SDLK_m: 
      return PROJECTM_K_m; 
    case SDLK_n:
      return PROJECTM_K_n; 
    case SDLK_o: 
      return PROJECTM_K_o; 
    case SDLK_p: 
      return PROJECTM_K_p; 
    case SDLK_q: 
      return PROJECTM_K_q; 
    case SDLK_r: 
      return PROJECTM_K_r; 
    case SDLK_s: 
      return PROJECTM_K_s; 
    case SDLK_t:
      return PROJECTM_K_t; 
    case SDLK_u: 
      return PROJECTM_K_u; 
    case SDLK_v: 
      return PROJECTM_K_v; 
    case SDLK_w: 
      return PROJECTM_K_w; 
    case SDLK_x: 
      return PROJECTM_K_x; 
    case SDLK_y: 
      return PROJECTM_K_y; 
    case SDLK_z: 
      return PROJECTM_K_z; 
    case SDLK_UP:
      return PROJECTM_K_UP;
    case SDLK_RETURN:
      return PROJECTM_K_RETURN;
    case SDLK_RIGHT:
      return PROJECTM_K_RIGHT;
    case SDLK_LEFT:
      return PROJECTM_K_LEFT;
    case SDLK_DOWN:
      return PROJECTM_K_DOWN;
    case SDLK_PAGEUP:
      return PROJECTM_K_PAGEUP;
    case SDLK_PAGEDOWN:
      return PROJECTM_K_PAGEDOWN;
   
        default: \
            return PROJECTM_K_NONE; \
      } \
  } \

inline projectMModifier sdl2pmModifier( SDLMod mod ) { \
    return PROJECTM_KMOD_LSHIFT; \
  } \

#endif /** _SDLTOPROJECTM_H */
