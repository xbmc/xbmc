#ifndef WINDOW_SYSTEM_OSX_GL_H
#define WINDOW_SYSTEM_OSX_GL_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "WinSystemOSX.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/GlobalsHandling.h"

class CWinSystemOSXGL : public CWinSystemOSX, public CRenderSystemGL, public virtual xbmcutil::Referenced
{
public:
  CWinSystemOSXGL();
  virtual ~CWinSystemOSXGL();
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);

protected:
  virtual bool PresentRenderImpl();
  virtual void SetVSyncImpl(bool enable);  
};

XBMC_GLOBAL_REF(CWinSystemOSXGL,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemOSXGL)

#endif // WINDOW_SYSTEM_H
