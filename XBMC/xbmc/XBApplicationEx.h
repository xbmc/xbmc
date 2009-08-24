/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef XBAPPLICATIONEX_H
#define XBAPPLICATIONEX_H

#include "IWindowManagerCallback.h"
#include "MouseStat.h"
#if defined (HAS_SDL) && defined (HAS_SDL_JOYSTICK)
#include "common/SDLJoystick.h"
#endif
#ifdef HAS_IRSERVERSUITE
#include "common/IRServerSuite/IRServerSuite.h"
#elif defined(HAS_LIRC)
#include "common/LIRC.h"
#endif

class CXBApplicationEx : public IWindowManagerCallback
{
public:
  CXBApplicationEx();
  ~CXBApplicationEx();

  // Main objects used for creating and rendering the 3D scene
#ifdef HAS_DX
  LPDIRECT3D9 m_pD3D;              // The D3D enumerator object
  LPDIRECT3DDEVICE9 m_pd3dDevice;        // The D3D rendering device
  LPDIRECT3DSURFACE9 m_pBackBuffer;       // The back buffer
  //LPDIRECT3DSURFACE9    m_pDepthBuffer;      // The depth buffer
#endif

  // Variables for timing
  bool m_bStop;
  bool m_AppActive;
  bool m_AppFocused;

  // Overridable functions for the 3D scene created by the app
  virtual HRESULT Initialize() { return S_OK; }
  virtual HRESULT Cleanup() { return S_OK; }
  //elis void ReadInput(XBMC_Event newEvent);

  /* eli
  bool ProcessOSShortcuts(XBMC_Event &event);
  bool ProcessWin32Shortcuts(XBMC_Event &event);
  bool ProcessLinuxShortcuts(XBMC_Event &event);
  bool ProcessOSXShortcuts(XBMC_Event &event);
  */


public:
  // Functions to create, run, and clean up the application
  virtual HRESULT Create(HWND hWnd);
  INT Run();
  VOID Destroy();

private:
};

#endif /* XBAPPLICATIONEX_H */
