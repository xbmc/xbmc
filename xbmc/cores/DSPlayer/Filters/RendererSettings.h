/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#pragma once
#ifndef RENDERERSETTINGS_H
#define RENDERERSETTINGS_H

enum AP_SURFACE_USAGE
{
  VIDRNDT_AP_SURFACE,
  VIDRNDT_AP_TEXTURE2D,
  VIDRNDT_AP_TEXTURE3D,
};

enum DS_RESIZERS
{
  DS_NEAREST_NEIGHBOR=0,
  DS_BILINEAR,
  DS_BILINEAR_2,
  DS_BILINEAR_2_60,
  DS_BILINEAR_2_75,
  DS_BILINEAR_2_100
};

enum DS_STATS
{
  DS_STATS_NONE = 0,
  DS_STATS_1 = 3,
  DS_STATS_2 = 2,
  DS_STATS_3 = 1
};

enum EVR_OUTPUT_RANGE
{
  OUTPUT_RANGE_0_255 = 0,
  OUTPUT_RANGE_16_235
};

class CRendererSettings
{
public:
  CRendererSettings()
  {
    SetDefault();
  }
  virtual void SetDefault()
  {
    apSurfaceUsage = VIDRNDT_AP_TEXTURE3D; // Fixed setting
    resizer = DS_BILINEAR; // On GUI
    displayStats = DS_STATS_NONE; // On GUI
    vSyncOffset = 0;
    vSyncAccurate = true;
    //TODO: Check if the flag is correctly set when creating the D3D Device
    fullscreenGUISupport = true;
    alterativeVSync = false;
    vSync = true;
    disableDesktopComposition = false;

    flushGPUBeforeVSync = true; //Flush GPU before VSync
    flushGPUAfterPresent = false; //Flush GPU after Present
    flushGPUWait = false; //Wait for flushes

    d3dFullscreen = true;
  }

public:
  DS_STATS displayStats;
  bool alterativeVSync;
  int vSyncOffset;
  bool vSyncAccurate;
  bool fullscreenGUISupport; // TODO: Not sure if it's really needed
  bool vSync;
  bool disableDesktopComposition;
  bool flushGPUBeforeVSync;
  bool flushGPUAfterPresent;
  bool flushGPUWait;
  AP_SURFACE_USAGE apSurfaceUsage;
  DS_RESIZERS resizer;
  bool d3dFullscreen;
};

class CEVRRendererSettings: public CRendererSettings
{
public:
  CEVRRendererSettings()
  {
    SetDefault();
  }
  void SetDefault()
  {
    CRendererSettings::SetDefault();

    highColorResolution = false;
    enableFrameTimeCorrection = false;
    outputRange = OUTPUT_RANGE_0_255;
    buffers = 4;
  }

public:
  bool highColorResolution;
  bool enableFrameTimeCorrection;
  EVR_OUTPUT_RANGE outputRange;
  int buffers;
};

class CVMR9RendererSettings: public CRendererSettings
{
public:
  CVMR9RendererSettings()
  {
    SetDefault();
  }
  void SetDefault()
  {
    CRendererSettings::SetDefault();

    mixerMode = true;
  };

public:
  bool mixerMode;
};

class CDSSettings
{
public:
  CStdString    m_strD3DX9Version;
  HINSTANCE     m_hD3DX9Dll;
  int           m_nDXSdkRelease;
  CStdStringW   D3D9RenderDevice;

  CRendererSettings *pRendererSettings;

  //TODO
  bool  IsD3DFullscreen() {return false;};

public:
  CDSSettings(void);
  virtual ~CDSSettings(void);
  void Initialize();

  void LoadConfig();

  HINSTANCE GetD3X9Dll();
  int GetDXSdkRelease() { return m_nDXSdkRelease; };

private:
  bool m_isEVR;
};
extern class CDSSettings g_dsSettings;
extern bool g_bNoDuration;
extern bool g_bExternalSubtitleTime;

#endif