/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
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
 * $Id: event.h,v 1.2 2004/10/08 10:54:27 cvs Exp $
 *
 * projectM keycodes. Enables translation from various event schemes such as Win32, SDL
 * &c.
 *
 * $Log: event.h,v $
 */

#ifndef _PROJECTM_EVENT_H
#define _PROJECTM_EVENT_H

typedef enum {
    /** Event types */
    PROJECTM_KEYUP,
    PROJECTM_KEYDOWN,
    PROJECTM_VIDEORESIZE,
    PROJECTM_VIDEOQUIT,
  } projectMEvent;

typedef enum {
    /** Keycodes */
    PROJECTM_K_RETURN,
    PROJECTM_K_RIGHT,
    PROJECTM_K_LEFT,
    PROJECTM_K_UP,
    PROJECTM_K_DOWN,
    PROJECTM_K_PAGEUP,
    PROJECTM_K_PAGEDOWN,
    PROJECTM_K_INSERT,
    PROJECTM_K_DELETE,
    PROJECTM_K_ESCAPE,
    PROJECTM_K_LSHIFT,
    PROJECTM_K_RSHIFT,
    PROJECTM_K_CAPSLOCK,
    PROJECTM_K_LCTRL,
    PROJECTM_K_HOME,
    PROJECTM_K_END,
    PROJECTM_K_BACKSPACE,

    PROJECTM_K_F1,
    PROJECTM_K_F2,
    PROJECTM_K_F3,
    PROJECTM_K_F4,
    PROJECTM_K_F5,
    PROJECTM_K_F6,
    PROJECTM_K_F7,
    PROJECTM_K_F8,
    PROJECTM_K_F9,
    PROJECTM_K_F10,
    PROJECTM_K_F11,
    PROJECTM_K_F12,

    PROJECTM_K_0 = 48,
    PROJECTM_K_1,
    PROJECTM_K_2,
    PROJECTM_K_3,
    PROJECTM_K_4,
    PROJECTM_K_5,
    PROJECTM_K_6,
    PROJECTM_K_7,
    PROJECTM_K_8,
    PROJECTM_K_9,

    PROJECTM_K_A = 65,
    PROJECTM_K_B,
    PROJECTM_K_C,
    PROJECTM_K_D,
    PROJECTM_K_E,
    PROJECTM_K_F,
    PROJECTM_K_G,
    PROJECTM_K_H,
    PROJECTM_K_I,
    PROJECTM_K_J,
    PROJECTM_K_K,
    PROJECTM_K_L,
    PROJECTM_K_M,
    PROJECTM_K_N,
    PROJECTM_K_O,
    PROJECTM_K_P,
    PROJECTM_K_Q,
    PROJECTM_K_R,
    PROJECTM_K_S,
    PROJECTM_K_T,
    PROJECTM_K_U,
    PROJECTM_K_V,
    PROJECTM_K_W,
    PROJECTM_K_X,
    PROJECTM_K_Y,
    PROJECTM_K_Z,

    PROJECTM_K_a = 97,
    PROJECTM_K_b,
    PROJECTM_K_c,
    PROJECTM_K_d,
    PROJECTM_K_e,
    PROJECTM_K_f,
    PROJECTM_K_g,
    PROJECTM_K_h,
    PROJECTM_K_i,
    PROJECTM_K_j,
    PROJECTM_K_k,
    PROJECTM_K_l,
    PROJECTM_K_m,
    PROJECTM_K_n,
    PROJECTM_K_o,
    PROJECTM_K_p,
    PROJECTM_K_q,
    PROJECTM_K_r,
    PROJECTM_K_s,
    PROJECTM_K_t,
    PROJECTM_K_u,
    PROJECTM_K_v,
    PROJECTM_K_w,
    PROJECTM_K_x,
    PROJECTM_K_y,
    PROJECTM_K_z,
    PROJECTM_K_NONE,
  } projectMKeycode;

typedef enum {
    /** Modifiers */
    PROJECTM_KMOD_LSHIFT,
    PROJECTM_KMOD_RSHIFT,
    PROJECTM_KMOD_CAPS,
    PROJECTM_KMOD_LCTRL,
    PROJECTM_KMOD_RCTRL,
  } projectMModifier;

#endif /** !_PROJECTM_EVENT_H */

