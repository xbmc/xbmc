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

  void GetModes(LPDIRECT3D8 pD3D);
  RESOLUTION GetSafeMode() const;
  RESOLUTION GetBestMode() const;
  bool IsValidResolution(RESOLUTION res) const;
  RESOLUTION GetInitialMode(LPDIRECT3D8 pD3D, D3DPRESENT_PARAMETERS *p3dParams);
  void PrintInfo() const;

  void Set480p(bool bEnable);
  void Set720p(bool bEnable);
  void Set1080i(bool bEnable);

  void SetNormal();
  void SetLetterbox(bool bEnable);
  void SetWidescreen(bool bEnable);


  bool NeedsSave();
  void Save();

private:
  bool bHasPAL;
  bool bHasNTSC;
  DWORD m_dwVideoFlags;
};

extern XBVideoConfig g_videoConfig;
