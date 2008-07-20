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

#include <libvisual/libvisual.h>

projectMEvent lv2pmEvent( VisEventType event ) { 
							
    switch ( event ) { 
        case VISUAL_EVENT_RESIZE:
            return PROJECTM_VIDEORESIZE; 
        case VISUAL_EVENT_KEYUP: 
            return PROJECTM_KEYUP; 
        case VISUAL_EVENT_KEYDOWN: 
            return PROJECTM_KEYDOWN;
        default:
            return PROJECTM_KEYUP; 
      } 
  } 
projectMKeycode lv2pmKeycode( VisKey keysym ) 
{ 
  switch ( keysym ) 
    { 
    case VKEY_F1: 
      return PROJECTM_K_F1; 
    case VKEY_F2: 
      return PROJECTM_K_F2; 
    case VKEY_F3:
      return PROJECTM_K_F3; 
    case VKEY_F4: 
      return PROJECTM_K_F4; 
    case VKEY_F5: 
      return PROJECTM_K_F5; 
    case VKEY_F6: 
      return PROJECTM_K_F6; 
    case VKEY_F7: 
      return PROJECTM_K_F7; 
    case VKEY_F8: 
      return PROJECTM_K_F8; 
    case VKEY_F9: 
      return PROJECTM_K_F9; 
    case VKEY_F10: 
      return PROJECTM_K_F10; 
    case VKEY_F11:
      return PROJECTM_K_F11; 
    case VKEY_F12: 
      return PROJECTM_K_F12; 
    case VKEY_ESCAPE: 
      return PROJECTM_K_ESCAPE; 
    case VKEY_a:
      return PROJECTM_K_a;
    case VKEY_b:
      return PROJECTM_K_b;
    case VKEY_c:  
      return PROJECTM_K_c;
    case VKEY_d: 
      return PROJECTM_K_d; 
    case VKEY_e:
      return PROJECTM_K_e; 
    case VKEY_f: 
      return PROJECTM_K_f; 
    case VKEY_g: 
      return PROJECTM_K_g; 
    case VKEY_h: 
      return PROJECTM_K_h; 
    case VKEY_i: 
      return PROJECTM_K_i; 
    case VKEY_j:
      return PROJECTM_K_j;
    case VKEY_k:
      return PROJECTM_K_k;
    case VKEY_l:  
      return PROJECTM_K_l;
    case VKEY_m: 
      return PROJECTM_K_m; 
    case VKEY_n:
      return PROJECTM_K_n; 
    case VKEY_o: 
      return PROJECTM_K_o; 
    case VKEY_p: 
      return PROJECTM_K_p; 
    case VKEY_q: 
      return PROJECTM_K_q; 
    case VKEY_r: 
      return PROJECTM_K_r; 
    case VKEY_s: 
      return PROJECTM_K_s; 
    case VKEY_t:
      return PROJECTM_K_t; 
    case VKEY_u: 
      return PROJECTM_K_u; 
    case VKEY_v: 
      return PROJECTM_K_v; 
    case VKEY_w: 
      return PROJECTM_K_w; 
    case VKEY_x: 
      return PROJECTM_K_x; 
    case VKEY_y: 
      return PROJECTM_K_y; 
    case VKEY_z: 
      return PROJECTM_K_z; 
    case VKEY_UP:
      return PROJECTM_K_UP;
    case VKEY_RETURN:
      return PROJECTM_K_RETURN;
    case VKEY_RIGHT:
      return PROJECTM_K_RIGHT;
    case VKEY_LEFT:
      return PROJECTM_K_LEFT;
    case VKEY_DOWN:
      return PROJECTM_K_DOWN;
    case VKEY_PAGEUP:
      return PROJECTM_K_PAGEUP;
    case VKEY_PAGEDOWN:
      return PROJECTM_K_PAGEDOWN;
   

    default:  
      return PROJECTM_K_NONE;
      break;
    } 
  } 

projectMModifier lv2pmModifier( int mod ) { 
    return mod && VKMOD_LSHIFT; 
  }
