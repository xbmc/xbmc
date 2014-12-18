#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define EGL_QUIRK_NONE (0)

/*! \brief Enable this if the implementation does not know its native
     resolution until a surface has been created. Used, for example, on Android
     where we have no control over the resolution, so we query it once the
     surface exists.
*/
#define EGL_QUIRK_NEED_WINDOW_FOR_RES (1 << 0)

/*! \brief Enable this if the implementation should have its native window
     destroyed when the surface is destroyed. In practice this means that a new
     native window will be created each time the main XBMC window is recreated.
*/
#define EGL_QUIRK_DESTROY_NATIVE_WINDOW_WITH_SURFACE (1 << 1)

/*! \brief The intel driver on wayland is broken and always returns a surface
           size of -1, -1. Work around it for now
*/
#define EGL_QUIRK_DONT_TRUST_SURFACE_SIZE (1 << 2)

/*! \brief Some drivers destroy the native display on resolution change. xbmc's EGL
    implementation is not aware of this change. In that case a Reinit of the display
    needs to be done.
*/
#define EGL_QUIRK_RECREATE_DISPLAY_ON_CREATE_WINDOW (1 << 3)
