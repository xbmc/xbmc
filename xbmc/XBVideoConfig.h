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

#include "GraphicContext.h"

#define XC_VIDEO_FLAGS 8
// FIXME: MAX_RESOLUTIONS is only used in GUISettings Addint for videoscreen.resolution
// because the vector m_ResInfo isn't filled at this stage I can't determine the iMax value
#define MAX_RESOLUTIONS 128

class XBVideoConfig
{
public:
  XBVideoConfig();
  ~XBVideoConfig();

  bool HasPAL() const;
  bool HasPAL60() const;
  bool HasNTSC() const;
  bool HasWidescreen() const;
  bool HasLetterbox() const;
  bool Has480p() const;
  bool Has720p() const;
  bool Has1080i() const;

  bool HasHDPack() const;
  CStdString GetAVPack() const;

#ifndef HAS_SDL
  void GetModes(LPDIRECT3D8 pD3D);
  RESOLUTION GetInitialMode(LPDIRECT3D8 pD3D, D3DPRESENT_PARAMETERS *p3dParams);
#else
  void GetModes();
  RESOLUTION GetInitialMode();
#endif
  RESOLUTION GetSafeMode() const;
  RESOLUTION GetBestMode() const;
#ifdef HAS_SDL
  void GetCurrentResolution(RESOLUTION_INFO &info) const;
  int GetNumberOfResolutions() { return m_iNumResolutions; }
  void GetResolutionInfo(int num, RESOLUTION_INFO &info) { info = m_ResInfo[num]; }
#endif
  VSYNC GetVSyncMode() const { return m_VSyncMode; }
  bool IsValidResolution(RESOLUTION res) const;
  void PrintInfo() const;

  void Set480p(bool bEnable);
  void Set720p(bool bEnable);
  void Set1080i(bool bEnable);

  void SetNormal();
  void SetLetterbox(bool bEnable);
  void SetWidescreen(bool bEnable);

  void SetVSyncMode(VSYNC mode) { m_VSyncMode = mode; }

  bool NeedsSave();
  void Save();

private:
  bool bHasPAL;
  bool bHasNTSC;
  DWORD m_dwVideoFlags;
  VSYNC m_VSyncMode;
  int m_iNumResolutions;
  std::vector<RESOLUTION_INFO> m_ResInfo;
};

extern XBVideoConfig g_videoConfig;
