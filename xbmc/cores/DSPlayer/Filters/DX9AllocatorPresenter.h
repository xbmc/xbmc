/*
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
 *
 *  Copyright (C) 2010-2013 Eduard Kytmanov
 *  http://www.avmedia.su
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "AllocatorCommon.h"
#include "RendererSettings.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "guilib/D3DResource.h"
#include "IPaintCallback.h"
#include "Utils/smartlist.h"
#include "threads/Event.h"
#include "DSGraph.h"
#include "..\ExternalPixelShader.h"
#include "EvrSharedRender.h"

// Support ffdshow queueing.
// This interface is used to check version of Media Player Classic.
// {A273C7F6-25D4-46b0-B2C8-4F7FADC44E37}
//DEFINE_GUID(IID_IVMRffdshow9,
//0xa273c7f6, 0x25d4, 0x46b0, 0xb2, 0xc8, 0x4f, 0x7f, 0xad, 0xc4, 0x4e, 0x37);


#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib") 

MIDL_INTERFACE("A273C7F6-25D4-46b0-B2C8-4F7FADC44E37")
IVMRffdshow9 : public IUnknown
{
public:
  virtual STDMETHODIMP support_ffdshow(void) = 0;
};

#define VMRBITMAP_UPDATE        0x80000000
#define MAX_PICTURE_SLOTS      (60+3)        // Last 2 for pixels shader + last one for final rendering!

#define NB_JITTER          126

class CDX9AllocatorPresenter
  : public ISubPicAllocatorPresenterImpl,
  public ID3DResource,
  public IPaintCallback,
  public IEvrAllocatorCallback
{
public:
  CCritSec        m_VMR9AlphaBitmapLock;
  CEvent          m_drawingIsDone;
  void            UpdateAlphaBitmap();

protected:
  Com::SmartSize                        m_ScreenSize;
  Com::SmartRect                        m_pScreenSize;
  uint32_t                              m_RefreshRate;
  CExternalPixelShader                  m_bscShader; // Brightness, saturation & contrast shader
  bool                                  m_bAlternativeVSync;
  bool                                  m_bHighColorResolution;
  bool                                  m_bCompositionEnabled;
  bool                                  m_bIsEVR;
  int                                   m_OrderedPaint;
  uint8_t                               m_VSyncMode;
  bool                                  m_bDesktopCompositionDisabled;
  bool                                  m_bIsFullscreen;
  bool                                  m_bNeedCheckSample;
  HMODULE                               m_hD3D9;
  CCritSec                              m_RenderLock;
  Com::SmartPtr<IDirect3D9Ex>           m_pD3D;
  IDirect3DDevice9*                     m_pD3DDev; // No need to store reference, we want to be able to delete the device anytime
  Com::SmartPtr<IDirect3DTexture9>      m_pVideoTexture[MAX_PICTURE_SLOTS];
  Com::SmartPtr<IDirect3DSurface9>      m_pVideoSurface[MAX_PICTURE_SLOTS];
  Com::SmartPtr<IDirect3DTexture9>      m_pOSDTexture;
  Com::SmartPtr<IDirect3DSurface9>      m_pOSDSurface;
  Com::SmartPtr<ID3DXLine>              m_pLine;
  Com::SmartPtr<ID3DXFont>              m_pFont;
  Com::SmartPtr<ID3DXSprite>            m_pSprite;
  Com::SmartPtr<IDirect3DPixelShader9>  m_pResizerPixelShader[4]; // bl, bc1, bc2_1, bc2_2
  Com::SmartPtr<IDirect3DTexture9>      m_pScreenSizeTemporaryTexture[2];
  D3DFORMAT                             m_SurfaceType;
  D3DFORMAT                             m_DisplayType;
  D3DTEXTUREFILTERTYPE                  m_filter;
  D3DCAPS9                              m_caps;
  std::auto_ptr<CPixelShaderCompiler>   m_pPSC;

  // Thread stuff
  HANDLE                                m_hEvtQuit;      // Stop rendering thread event
  HANDLE                                m_hVSyncThread;
  float                                 m_bicubicA;

  int                                   m_nTearingPos;
  VMR9AlphaBitmap                       m_VMR9AlphaBitmap;
  Com::SmartAutoVectorPtr<uint8_t>      m_VMR9AlphaBitmapData;
  Com::SmartRect                        m_VMR9AlphaBitmapRect;
  int                                   m_VMR9AlphaBitmapWidthBytes;
  size_t                                m_nNbDXSurface;          // Total number of DX Surfaces
  size_t                                m_nVMR9Surfaces;         // Total number of DX Surfaces
  size_t                                m_iVMR9Surface;
  size_t                                m_nCurSurface;           // Surface currently displayed
  size_t                                m_nUsedBuffer;
  double                                m_fAvrFps;               // Estimate the real FPS
  double                                m_fJitterStdDev;         // Estimate the Jitter std dev
  double                                m_fJitterMean;
  double                                m_fSyncOffsetStdDev;
  double                                m_fSyncOffsetAvr;
  double                                m_DetectedRefreshRate;

  CCritSec                              m_RefreshRateLock;
  double                                m_DetectedRefreshTime;
  double                                m_DetectedRefreshTimePrim;
  double                                m_DetectedScanlineTime;
  double                                m_DetectedScanlineTimePrim;
  double                                m_DetectedScanlinesPerFrame;

  double                                m_ldDetectedRefreshRateList[100];
  double                                m_ldDetectedScanlineRateList[100];
  int                                   m_DetectedRefreshRatePos;
  bool                                  m_bSyncStatsAvailable;
  int64_t                               m_pllJitter[NB_JITTER];        // Jitter buffer for stats
  int64_t                               m_pllSyncOffset[NB_JITTER];    // Jitter buffer for stats
  int64_t                               m_llLastPerf;
  int64_t                               m_JitterStdDev;
  int64_t                               m_MaxJitter;
  int64_t                               m_MinJitter;
  int64_t                               m_MaxSyncOffset;
  int64_t                               m_MinSyncOffset;
  int                                   m_nNextJitter;
  int                                   m_nNextSyncOffset;
  int64_t                               m_rtTimePerFrame;
  double                                m_DetectedFrameRate;
  double                                m_DetectedFrameTime;
  double                                m_DetectedFrameTimeStdDev;
  bool                                  m_DetectedLock;
  int64_t                               m_DetectedFrameTimeHistory[60];
  double                                m_DetectedFrameTimeHistoryHistory[500];
  int                                   m_DetectedFrameTimePos;
  int                                   m_bInterlaced;
  double                                m_TextScale;
  int                                   m_VBlankEndWait;
  int                                   m_VBlankStartWait;
  int64_t                               m_VBlankWaitTime;
  int64_t                               m_VBlankLockTime;
  int                                   m_VBlankMin;
  int                                   m_VBlankMinCalc;
  int                                   m_VBlankMax;
  int                                   m_VBlankEndPresent;
  int64_t                               m_VBlankStartMeasureTime;
  int                                   m_VBlankStartMeasure;

  int64_t                               m_PresentWaitTime;
  int64_t                               m_PresentWaitTimeMin;
  int64_t                               m_PresentWaitTimeMax;

  int64_t                               m_PaintTime;
  int64_t                               m_PaintTimeMin;
  int64_t                               m_PaintTimeMax;
  int64_t                               m_beforePresentTime;

  int64_t                               m_WaitForGPUTime;

  int64_t                               m_RasterStatusWaitTime;
  int64_t                               m_RasterStatusWaitTimeMin;
  int64_t                               m_RasterStatusWaitTimeMax;
  int64_t                               m_RasterStatusWaitTimeMaxCalc;

  double                                m_ClockDiffCalc;
  double                                m_ClockDiffPrim;
  double                                m_ClockDiff;

  double                                m_TimeChangeHistory[100];
  double                                m_ClockChangeHistory[100];
  int                                   m_ClockTimeChangeHistoryPos;
  double                                m_ModeratedTimeSpeed;
  double                                m_ModeratedTimeSpeedPrim;
  double                                m_ModeratedTimeSpeedDiff;

  bool                                  m_bCorrectedFrameTime;
  bool                                  m_bTakenLock;
  bool                                  m_bPaintWasCalled;
  int                                   m_FrameTimeCorrection;
  int64_t                               m_LastFrameDuration;
  int64_t                               m_LastSampleTime;

  CStdString                            m_strStatsMsg[10];
  CStdString                            m_D3D9Device;

  static DWORD WINAPI                   VSyncThreadStatic(LPVOID lpParam);
  void                                  VSyncThread();
  void                                  StartWorkerThreads();
  void                                  StopWorkerThreads();
  STDMETHODIMP_(void)                   SetTime(REFERENCE_TIME rtNow);
  uint32_t                              GetAdapter(IDirect3D9 *pD3D, bool GetAdapter = false);
  HRESULT(__stdcall *m_pD3DXCreateSprite)(LPDIRECT3DDEVICE9 pDevice, LPD3DXSPRITE * ppSprite);
  HRESULT                               InitResizers(float bicubicA, bool bNeedScreenSizeTexture);
  bool                                  GetVBlank(int &_ScanLine, int &_bInVBlank, bool _bMeasureTime);
  bool                                  WaitForVBlankRange(int &_RasterStart, int _RasterEnd, bool _bWaitIfInside, bool _bNeedAccurate, bool _bMeasure, bool &_bTakenLock);
  bool                                  WaitForVBlank(bool &_Waited, bool &_bTakenLock);
  int                                   GetVBlackPos();
  void                                  CalculateJitter(int64_t PerformanceCounter);

  HRESULT                               DrawRect(DWORD _Color, DWORD _Alpha, const Com::SmartRect &_Rect);
  HRESULT                               TextureCopy(Com::SmartPtr<IDirect3DTexture9> pTexture);
  HRESULT                               TextureResize(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], D3DTEXTUREFILTERTYPE filter, const Com::SmartRect &SrcRect);
  HRESULT                               TextureResizeBilinear(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect);
  HRESULT                               TextureResizeBicubic1pass(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect);
  HRESULT                               TextureResizeBicubic2pass(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect);
  HRESULT                               AlphaBlt(RECT* pSrc, RECT* pDst, Com::SmartPtr<IDirect3DTexture9> pTexture);

  void                                  DrawText(const RECT &rc, const CStdString &strText, int _Priority);
  void                                  DrawStats();
  double                                GetFrameTime();
  double                                GetFrameRate();

  //D3D9Device

  HRESULT                               InitD3D9(HWND hwnd);
  void                                  BuildPresentParameters();
  HRESULT                               ResetRenderParam();
  void                                  SetMonitor(HMONITOR monitor);
  BOOL                                  IsDepthFormatOk(D3DFORMAT DepthFormat, D3DFORMAT RenderTargetFormat);
  bool                                  IsSurfaceFormatOk(D3DFORMAT surfFormat, DWORD usage);
  D3DDEVTYPE                            m_devType;
  D3DPRESENT_PARAMETERS		              m_D3DPP;
  D3DDISPLAYMODEEX                      m_D3DDMEX;
  CEvrSharedRender*                     m_pEvrShared;
  bool                                  m_firstBoot;
  bool                                  m_useWindowedDX;
  HWND                                  m_hDeviceWnd;
  unsigned int                          m_nBackBufferWidth;
  unsigned int                          m_nBackBufferHeight;
  bool                                  m_bVSync;
  float                                 m_fRefreshRate;
  bool                                  m_interlaced;
  int                                   m_adapter;
  int                                   m_kodiGuiDirtyAlgo;

  CRect                                 m_activeVideoRect;

  virtual HRESULT                       CreateDevice(CStdString &_Error);
  virtual HRESULT                       AllocSurfaces(D3DFORMAT Format = D3DFMT_A8R8G8B8);
  virtual void                          DeleteSurfaces();
  virtual void                          OnVBlankFinished(bool fAll, int64_t PerformanceCounter) {}
  virtual void                          BeforeDeviceReset();
  virtual void                          AfterDeviceReset();

  // Casimir666
  typedef HRESULT(WINAPI * D3DXLoadSurfaceFromMemoryPtr)(
    LPDIRECT3DSURFACE9  pDestSurface,
    CONST PALETTEENTRY*  pDestPalette,
    CONST RECT*    pDestRect,
    LPCVOID      pSrcMemory,
    D3DFORMAT    SrcFormat,
    UINT      SrcPitch,
    CONST PALETTEENTRY*  pSrcPalette,
    CONST RECT*    pSrcRect,
    DWORD      Filter,
    D3DCOLOR    ColorKey);

  typedef HRESULT(WINAPI* D3DXCreateLinePtr) (LPDIRECT3DDEVICE9   pDevice, LPD3DXLINE* ppLine);

  typedef HRESULT(WINAPI* D3DXCreateFontPtr)(
    LPDIRECT3DDEVICE9  pDevice,
    int      Height,
    UINT      Width,
    UINT      Weight,
    UINT      MipLevels,
    bool      Italic,
    DWORD      CharSet,
    DWORD      OutputPrecision,
    DWORD      Quality,
    DWORD      PitchAndFamily,
    LPCWSTR      pFaceName,
    LPD3DXFONT*    ppFont);

  D3DXLoadSurfaceFromMemoryPtr            m_pD3DXLoadSurfaceFromMemory;
  D3DXCreateLinePtr                       m_pD3DXCreateLine;
  D3DXCreateFontPtr                       m_pD3DXCreateFont;

  double GetRefreshRate()
  {
    if (m_DetectedRefreshRate)
      return m_DetectedRefreshRate;
    return m_RefreshRate;
  }

  long GetScanLines()
  {
    if (m_DetectedRefreshRate)
      return (long)m_DetectedScanlinesPerFrame;
    return m_ScreenSize.cy;
  }

  void LockD3DDevice()
  {
    if (m_pD3DDev)
    {
      _RTL_CRITICAL_SECTION *pCritSec = (_RTL_CRITICAL_SECTION *)((size_t)m_pD3DDev + sizeof(size_t));

      if (!IsBadReadPtr(pCritSec, sizeof(*pCritSec)) && !IsBadWritePtr(pCritSec, sizeof(*pCritSec))
        && !IsBadReadPtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))) && !IsBadWritePtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))))
      {
        if (pCritSec->DebugInfo->CriticalSection == pCritSec)
          EnterCriticalSection(pCritSec);
      }
    }
  }

  void UnlockD3DDevice()
  {
    if (m_pD3DDev)
    {
      _RTL_CRITICAL_SECTION *pCritSec = (_RTL_CRITICAL_SECTION *)((size_t)m_pD3DDev + sizeof(size_t));

      if (!IsBadReadPtr(pCritSec, sizeof(*pCritSec)) && !IsBadWritePtr(pCritSec, sizeof(*pCritSec))
        && !IsBadReadPtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))) && !IsBadWritePtr(pCritSec->DebugInfo, sizeof(*(pCritSec->DebugInfo))))
      {
        if (pCritSec->DebugInfo->CriticalSection == pCritSec)
          LeaveCriticalSection(pCritSec);
      }
    }
  }

public:
  CDX9AllocatorPresenter(HWND hWnd, HRESULT& hr, bool bIsEVR, CStdString &_Error);
  ~CDX9AllocatorPresenter();

  // ISubPicAllocatorPresenter
  STDMETHODIMP                        CreateRenderer(IUnknown** ppRenderer);
  STDMETHODIMP_(bool)                 Paint(bool fAll);
  STDMETHODIMP                        GetDIB(BYTE* lpDib, DWORD* size);

  // ID3DResource
  virtual void                        OnLostDevice();
  virtual void                        OnDestroyDevice();
  virtual void                        OnCreateDevice();
  virtual void                        OnResetDevice();

  // IPainCallback
  virtual void                        OnPaint(CRect destRect);
  virtual void                        OnAfterPresent();
  virtual void                        OnReset();

  // IEvrAllocatorCallback
  virtual CRect GetActiveVideoRect() { return m_activeVideoRect; };

  static bool                         bPaintAll;
};