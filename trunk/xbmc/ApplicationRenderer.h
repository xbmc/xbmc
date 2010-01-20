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

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "GUIDialogBusy.h"

class CApplicationRenderer : public CThread
{
public:
  CApplicationRenderer(void);
  ~CApplicationRenderer(void);

  bool Start();
  void Stop();
  void Render(bool bFullscreen = false);
  void Disable();
  void Enable();
  bool IsBusy() const;
  void SetBusy(bool bBusy);
private:
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  void UpdateBusyCount();
#ifdef HAS_DX
  bool CopySurface(LPDIRECT3DSURFACE9 pSurfaceSource, const RECT* rcSource, LPDIRECT3DSURFACE9 pSurfaceDest, const RECT* rcDest);
#endif

  unsigned int m_time;
  bool m_enabled;
  int m_explicitbusy;
  int m_busycount;
  int m_prevbusycount;
  bool m_busyShown;
  CCriticalSection m_criticalSection;
#ifdef HAS_DX
  LPDIRECT3DSURFACE9 m_lpSurface;
#endif
  CGUIDialogBusy* m_pWindow;
  RESOLUTION m_Resolution;
};
extern CApplicationRenderer g_ApplicationRenderer;



