#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __SCREENSAVER_TYPES_H__
#define __SCREENSAVER_TYPES_H__

#include "xbmc_addon_types.h"

extern "C"
{
  struct SCR_INFO
  {
    int dummy;
  };

  struct ScreenSaver
  {
#if !defined(_LINUX) && !defined(HAS_SDL)
    ADDON_STATUS (__cdecl* Create)(ADDON_HANDLE hdl, LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float pixelRatio);
#else
    ADDON_STATUS (__cdecl* Create)(ADDON_HANDLE hdl, void* pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float pixelRatio);
#endif
    void (__cdecl* Start) ();
    void (__cdecl* Render) ();
    void (__cdecl* Stop) ();
    void (__cdecl* GetInfo)(SCR_INFO *info);
  };
}

#endif // __SCREENSAVER_TYPES_H__
