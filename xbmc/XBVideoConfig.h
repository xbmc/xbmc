#pragma once

#define XC_VIDEO_FLAGS 8
#define MAX_RESOLUTIONS 32

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
  void GetDesktopResolution(int &w, int &h);
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
  RESOLUTION_INFO m_ResInfo[MAX_RESOLUTIONS];
};

extern XBVideoConfig g_videoConfig;
