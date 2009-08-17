/*!
\file Surface.h
\brief
*/

#ifndef GUILIB_SURFACE_H
#define GUILIB_SURFACE_H

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

#include <string>
#include "TextureManager.h"

namespace Surface {
#if defined(_WIN32PC)
/*!
 \ingroup graphics
 \brief
 */
enum ONTOP {
  ONTOP_NEVER = 0,
  ONTOP_ALWAYS = 1,
  ONTOP_FULLSCREEN = 2,
  ONTOP_AUTO = 3 // When fullscreen on Intel GPU
};
#endif

#if defined(_LINUX) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

using namespace std;

class CSurface
{
public:
  CSurface();
  CSurface(CSurface* src);
  CSurface(int width, int height, bool doublebuffer, CSurface* shared,
    CSurface* associatedWindow, CBaseTexture* parent=0, bool fullscreen=false,
           bool offscreen=false, bool pbuffer=false, int antialias=0);
  
  virtual ~CSurface(void);

  virtual bool Create(string name, int width, int height, bool fullScreen){return false;}
  virtual bool ResizeSurface(int newWidth, int newHeight){return false;}
  virtual bool SetFullScreen(bool fullScreen, int width, int height){return false;}
  virtual bool Destroy(){return false;}

  int GetWidth() const { return m_iWidth; }
  int GetHeight() const { return m_iHeight; }
  bool IsShared() const { return m_pShared?true:false; }
  bool IsFullscreen() const { return m_bFullscreen; }
  bool IsDoublebuffered() const { return m_bDoublebuffer; }
  bool IsValid() { return m_bOK; }
  DWORD GetNextSwap();
  void NotifyAppFocusChange(bool bGaining);
#ifdef _WIN32
  void SetOnTop(ONTOP iOnTop);
  bool IsOnTop();
#endif

  virtual void Flip() = 0;
  virtual bool MakeCurrent() = 0;
  virtual void ReleaseContext() = 0;
  virtual void EnableVSync(bool enable=true) = 0;
  
  virtual void RefreshCurrentContext() = 0;
  virtual  void* GetRenderWindow() = 0;

 protected:
  CSurface* m_pShared;
  int m_iWidth;
  int m_iHeight;
  bool m_bFullscreen;
  bool m_bDoublebuffer;
  bool m_bOK;
  bool m_bVSync;
  int m_iVSyncMode;
  int m_iVSyncErrors;
  __int64 m_iSwapStamp;
  __int64 m_iSwapRate;
  __int64 m_iSwapTime;
  bool m_bVsyncInit;
  short int m_iRedSize;
  short int m_iGreenSize;
  short int m_iBlueSize;
  short int m_iAlphaSize;
 
#ifdef _WIN32
  bool m_bCoversScreen;
  ONTOP m_iOnTop;
#endif

  CBaseTexture* m_Surface;
};

}

#endif // GUILIB_SURFACE_H
