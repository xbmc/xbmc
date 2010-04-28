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
#include "cores/DSPlayer/DShowUtil/DSGeometry.h"

#include <vector>
enum
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

typedef struct
{
  bool fValid;
  Com::SmartSize size;
  int bpp, freq;
  DWORD dmDisplayFlags;
} dispmode;

typedef struct
{
  bool bEnabled;
  dispmode dmFullscreenRes24Hz;
  dispmode dmFullscreenRes25Hz;
  dispmode dmFullscreenRes30Hz;
  dispmode dmFullscreenResOther;
  bool bApplyDefault;
  dispmode dmFullscreenRes23d976Hz;
  dispmode dmFullscreenRes29d97Hz;
}  AChFR;


class TiXmlElement;

class CDsSettings
{
public:
  bool    m_fTearingTest;
  int     m_fDisplayStats;
  bool    m_bResetStats; // Set to reset the presentation statistics
  CStdString    m_strVersion;
  CStdString    m_strD3DX9Version;
  CStdString    m_AudioRendererDisplayName_CL;
  HINSTANCE     m_hD3DX9Dll;
  int           m_nDXSdkRelease;
  bool fResetDevice;

    class CRendererSettingsShared
    {
    public:
      CRendererSettingsShared()
      {
        SetDefault();
      }
      bool fVMR9AlterativeVSync;
      int  iVMR9VSyncOffset;
      bool iVMR9VSyncAccurate;
      bool iVMR9FullscreenGUISupport;
      bool iVMR9VSync;
      bool iVMRDisableDesktopComposition;
      bool iVMRFlushGPUBeforeVSync;
      bool iVMRFlushGPUAfterPresent;
      bool iVMRFlushGPUWait;

      // SyncRenderer settings Still dont know if this renderer will be added
      bool bSynchronizeVideo;
      bool bSynchronizeDisplay;
      bool bSynchronizeNearest;
      int iLineDelta;
      int iColumnDelta;
      double fCycleDelta;
      double fTargetSyncOffset;
      double fControlLimit;

      void SetDefault();
      void SetOptimal();
    };
    class CRendererSettingsEVR : public CRendererSettingsShared
    {
    public:
      bool iEVRHighColorResolution;
      bool iEVREnableFrameTimeCorrection;
      int iEVROutputRange;
      int iEvrBuffers;

      CRendererSettingsEVR()
      {
        SetDefault();
      }
      void SetDefault()
      {
        CRendererSettingsShared::SetDefault();

        iEVRHighColorResolution = false;
        iEVREnableFrameTimeCorrection = false;
        iEVROutputRange = 0;
        iEvrBuffers = 4; // Needed because painting and rendering are not in the same thread
      }
      void SetOptimal()
      {
        CRendererSettingsShared::SetOptimal();
        iEVRHighColorResolution = false;
      }
    };

    CRendererSettingsEVR m_RenderSettings;

  int iDSVideoRendererType;
  int iRMVideoRendererType;
  int iQTVideoRendererType;
  int iAPSurfaceUsage;
//    bool fVMRSyncFix;
  int iDX9Resizer;
  bool fVMR9MixerMode;
  bool fVMR9MixerYUV;

  int nVolume;
  int nBalance;
  bool fMute;
  int nLoops;
  bool fLoopForever;
  bool fRewind;
  int iZoomLevel;
  // int iVideoRendererType; 
  CStdStringW AudioRendererDisplayName;
  bool fAutoloadAudio;
  bool fAutoloadSubtitles;
  bool fBlockVSFilter;
  bool fEnableWorkerThreadForOpening;
  bool fReportFailedPins;

  CStdStringW f_hmonitor;
  bool fAssociatedWithIcons;
  CStdStringW f_lastOpenDir;

  bool fAllowMultipleInst;
  int iTitleBarTextStyle;
  bool fTitleBarTextTitle;
  int iOnTop;
  bool fTrayIcon;
  bool fRememberZoomLevel;
  bool fShowBarsWhenFullScreen;
  int nShowBarsWhenFullScreenTimeOut;
  AChFR AutoChangeFullscrRes;
  bool fExitFullScreenAtTheEnd;
  bool fRestoreResAfterExit;
  bool fRememberWindowPos;
  bool fRememberWindowSize;
  bool fSnapToDesktopEdges;
  Com::SmartRect rcLastWindowPos;
  UINT lastWindowType;
  Com::SmartSize AspectRatio;
  bool fKeepHistory;

  CStdString sDVDPath;
  bool fUseDVDPath;
  LCID idMenuLang, idAudioLang, idSubtitlesLang;
  bool fAutoSpeakerConf;

  bool fOverridePlacement;
  int nHorPos, nVerPos;
  int nSPCSize;
  int nSPCMaxRes;
  int nSubDelayInterval;
  bool fSPCPow2Tex;
  bool fSPCAllowAnimationWhenBuffering;
  bool fEnableSubtitles;
  bool fUseDefaultSubtitlesStyle;

  bool fDisableXPToolbars;
  bool fUseWMASFReader;
  int nJumpDistS;
  int nJumpDistM;
  int nJumpDistL;
  bool fLimitWindowProportions;
  bool fNotifyMSN;
  bool fNotifyGTSdll;

  bool fEnableAudioSwitcher;
  bool fDownSampleTo441;
  bool fAudioTimeShift;
  int tAudioTimeShift;
  bool fCustomChannelMapping;
  DWORD pSpeakerToChannelMap[18][18];
  bool fAudioNormalize;
  bool fAudioNormalizeRecover;
  float AudioBoost;

  bool fIntRealMedia;
  int iQuickTimeRenderer;
  float RealMediaQuickTimeFPS;

  std::vector<CStdString> m_pnspresets;
  HACCEL hAccel;

  bool fWinLirc;
  CStdString WinLircAddr;
  bool fGlobalMedia;

  bool      fD3DFullscreen;
  bool      fMonitorAutoRefreshRate;
  bool      fLastFullScreen;
  bool      fEnableEDLEditor;
  float      dBrightness;
  float      dContrast;
  float      dHue;
  float      dSaturation;
  CStdString      strShaderList;
  CStdString      strShaderListScreenSpace;
  bool      m_bToggleShader;
  bool      m_bToggleShaderScreenSpace;

  bool      fRememberDVDPos;
  bool      fRememberFilePos;
  bool      fShowOSD;
  int        iLanguage;


  HWND      hMasterWnd;
//TODO
  bool      IsD3DFullscreen(){return false;};

public:
  CDsSettings(void);
  virtual ~CDsSettings(void);
  void SetDefault();

  void LoadConfig();
  void GetBoolean(const TiXmlElement* pRootElement, const char *tagName, bool& iValue, const bool iDefault);
  void GetInteger(const TiXmlElement* pRootElement, const char *tagName, int& fValue, const int fDefault, const int fMin, const int fMax);
  void GetDouble(const TiXmlElement* pRootElement, const char *tagName, double& fValue, const double fDefault, const double fMin, const double fMax);
  //void UpdateData(bool fSave);
  HINSTANCE          GetD3X9Dll();
  int              GetDXSdkRelease() { return m_nDXSdkRelease; };
  bool m_fPreventMinimize;
  bool m_fUseWin7TaskBar;
  bool m_fExitAfterPlayback;
  bool m_fNextInDirAfterPlayback;
  bool m_fDontUseSearchInFolder;
  int  nOSD_Size;
  CStdString m_OSD_Font;
  CStdStringW m_subtitlesLanguageOrder;
  CStdStringW m_audiosLanguageOrder;

  int fnChannels;

  CStdStringW D3D9RenderDevice;
};
extern class CDsSettings g_dsSettings;
extern bool g_bNoDuration;
extern bool g_bExternalSubtitleTime;

#endif