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

extern "C"
{
  struct SCR_INFO
  {
    int dummy;
  };

  struct SCR_PROPS
  {
    void *device;
    int x;
    int y;
    int width;
    int height;
    float pixelRatio;
    const char *name;
    const char *presets;
    const char *profile;
  };

  struct ScreenSaver
  {
    void (__cdecl* Start) ();
    void (__cdecl* Render) ();
    void (__cdecl* GetInfo)(SCR_INFO *info);
  };
}

#endif // __SCREENSAVER_TYPES_H__
