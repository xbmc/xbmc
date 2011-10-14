#ifndef WINDOW_SYSTEM_DFB_H
#define WINDOW_SYSTEM_DFB_H

#pragma once

/*
 *      Copyright (C) 2011 Team XBMC
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

#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GlobalsHandling.h"
#include "windowing/WinSystem.h"
#include <directfb/directfbgl2.h>

typedef struct _IDirectFB IDirectFB;
typedef struct _IDirectFBSurface IDirectFBSurface;
typedef struct _IDirectFBDisplayLayer IDirectFBDisplayLayer;

class CWinSystemDFB : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CWinSystemDFB();
  virtual ~CWinSystemDFB();

  virtual bool  InitWindowSystem();
  virtual bool  DestroyWindowSystem();
  virtual bool  CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool  DestroyWindow();
  virtual bool  ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool  SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void  UpdateResolutions();

  virtual void  NotifyAppActiveChange(bool bActivated) {return;}
  virtual void  ShowOSMouse(bool show)  {return;};
  virtual bool  Minimize()              {return false;};
  virtual bool  Restore()               {return false;};
  virtual bool  Hide()                  {return false;};
  virtual bool  Show(bool raise = true) {return false;};

  IDirectFB*              GetIDirectFB() const   {return m_dfb;};
  IDirectFBGL2Context*    GetGLContext() const   {return m_gl2context;};
  IDirectFBSurface*       GetSurface() const     {return m_dfb_surface;};

protected:
  virtual bool  PresentRenderImpl(const CDirtyRegionList &dirty);
  virtual void  SetVSyncImpl(bool enable);

  IDirectFB             *m_dfb;
  IDirectFBDisplayLayer *m_dfb_layer;
  IDirectFBSurface      *m_dfb_surface;
  IDirectFBGL2          *m_gl2;
  IDirectFBGL2Context   *m_gl2context;
  DFBSurfaceFlipFlags    m_flipflags;
  DFBSurfaceCapabilities m_buffermode;
};

XBMC_GLOBAL_REF(CWinSystemDFB,g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(CWinSystemDFB)

#endif // WINDOW_SYSTEM_DFB_H
