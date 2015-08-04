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

#ifdef HAS_DS_PLAYER

#include "DX9AllocatorPresenter.h"
#include <initguid.h>
#include "Subtitles/SubPic/DX9SubPic.h"
#include "Utils/IPinHook.h"
#include "DSUtil/DSUtil.h"
#include "StreamsManager.h"
#include "Utils/TimeUtils.h"
#include "utils/Log.h"
#include "windowing/WindowingFactory.h"
#include "application.h"
#include "FileSystem/File.h"
#include "PixelShaderList.h"
#include "settings/MediaSettings.h"
#include "DSPlayer.h"

#ifndef TRACE
#define TRACE(x)
#endif

CCritSec g_ffdshowReceive;
bool queue_ffdshow_support = false;

#define FRAMERATE_MAX_DELTA      3000
//#pragma optimize("", off)
//#pragma inline_depth(0)

#pragma pack(push, 1)
template<int texcoords>
struct MYD3DVERTEX
{
  float x, y, z, rhw;
  struct
  {
    float u, v;
  } t[texcoords];
};
template<>
struct MYD3DVERTEX < 0 >
{
  float x, y, z, rhw;
  DWORD Diffuse;
};
#pragma pack(pop)

template<int texcoords>
static void AdjustQuad(MYD3DVERTEX<texcoords>* v, double dx, double dy)
{
  double offset = 0.5;

  for (int i = 0; i < 4; i++)
  {
    v[i].x -= (float)offset;
    v[i].y -= (float)offset;

    for (int j = 0; j < std::max(texcoords - 1, 1); j++)
    {
      v[i].t[j].u -= (float)(offset*dx);
      v[i].t[j].v -= (float)(offset*dy);
    }

    if (texcoords > 1)
    {
      v[i].t[texcoords - 1].u -= (float)offset;
      v[i].t[texcoords - 1].v -= (float)offset;
    }
  }
}

template<int texcoords>
static HRESULT TextureBlt(IDirect3DDevice9* pD3DDev, MYD3DVERTEX<texcoords> v[4], D3DTEXTUREFILTERTYPE filter = D3DTEXF_LINEAR)
{
  if (!pD3DDev)
    return E_POINTER;

  DWORD FVF = 0;
  switch (texcoords)
  {
  case 1: FVF = D3DFVF_TEX1; break;
  case 2: FVF = D3DFVF_TEX2; break;
  case 3: FVF = D3DFVF_TEX3; break;
  case 4: FVF = D3DFVF_TEX4; break;
  case 5: FVF = D3DFVF_TEX5; break;
  case 6: FVF = D3DFVF_TEX6; break;
  case 7: FVF = D3DFVF_TEX7; break;
  case 8: FVF = D3DFVF_TEX8; break;
  default: return E_FAIL;
  }

  HRESULT hr;

  //Those are needed to avoid conflict with the xbmc gui
  hr = pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  hr = pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  hr = pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  hr = pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  hr = pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  hr = pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);

  for (int i = 0; i < texcoords; i++)
  {
    hr = pD3DDev->SetSamplerState(i, D3DSAMP_MAGFILTER, filter);
    hr = pD3DDev->SetSamplerState(i, D3DSAMP_MINFILTER, filter);
    hr = pD3DDev->SetSamplerState(i, D3DSAMP_MIPFILTER, filter);
    hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    hr = pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  }
  hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | FVF);

  MYD3DVERTEX<texcoords> tmp = v[2];
  v[2] = v[3];
  v[3] = tmp;
  hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

  for (int i = 0; i < texcoords; i++)
  {
    pD3DDev->SetTexture(i, NULL);
  }

  return S_OK;
}

static HRESULT DrawRect(IDirect3DDevice9* pD3DDev, MYD3DVERTEX<0> v[4])
{
  if (!pD3DDev)
    return E_POINTER;

  HRESULT hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  hr = pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  hr = pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  //D3DRS_COLORVERTEX 
  hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

  hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE |
    D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
  hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX0 | D3DFVF_DIFFUSE);

  MYD3DVERTEX<0> tmp = v[2];
  v[2] = v[3];
  v[3] = tmp;
  hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));

  return S_OK;
}

// CDX9AllocatorPresenter

bool CDX9AllocatorPresenter::bPaintAll = true;

CDX9AllocatorPresenter::CDX9AllocatorPresenter(HWND hWnd, HRESULT& hr, bool bIsEVR, CStdString &_Error)
  : ISubPicAllocatorPresenterImpl(hWnd, hr)
  , m_ScreenSize(0, 0)
  , m_RefreshRate(0)
  , m_bicubicA(0)
  , m_nTearingPos(0)
  , m_nNbDXSurface(1)
  , m_nVMR9Surfaces(0)
  , m_iVMR9Surface(0)
  , m_nCurSurface(0)
  , m_rtTimePerFrame(0)
  , m_bInterlaced(false)
  , m_nUsedBuffer(0)
  , m_OrderedPaint(0)
  , m_bCorrectedFrameTime(0)
  , m_FrameTimeCorrection(0)
  , m_LastSampleTime(0)
  , m_LastFrameDuration(0)
  , m_bAlternativeVSync(0)
  , m_bIsEVR(bIsEVR)
  , m_VSyncMode(0)
  , m_TextScale(1.0)
  , m_pScreenSize(0, 0, 0, 0)
  , m_drawingIsDone()
  , m_bPaintWasCalled(false)
  , m_bNeedCheckSample(true)
  , m_hVSyncThread(NULL)
  , m_hEvtQuit(NULL)
  , m_bscShader("contrast_brightness.psh", "ps_2_0")
{
  g_Windowing.Register(this);
  g_renderManager.PreInit(RENDERER_DSHOW);

  g_renderManager.RegisterCallback(this);
  m_bIsFullscreen = g_dsSettings.IsD3DFullscreen();

  if (FAILED(hr))
  {
    _Error += L"ISubPicAllocatorPresenterImpl failed\n";
    return;
  }

  bPaintAll = true;
  m_bscShader.SetEnabled(true);
  m_bscShader.SetName("Brightness and contrast support");

  m_pD3DXLoadSurfaceFromMemory = NULL;
  m_pD3DXCreateLine = NULL;
  m_pD3DXCreateFont = NULL;
  m_pD3DXCreateSprite = NULL;
  HINSTANCE hDll = g_dsSettings.GetD3X9Dll();
  if (hDll)
  {
    (FARPROC&)m_pD3DXLoadSurfaceFromMemory = GetProcAddress(hDll, "D3DXLoadSurfaceFromMemory");
    (FARPROC&)m_pD3DXCreateLine = GetProcAddress(hDll, "D3DXCreateLine");
    (FARPROC&)m_pD3DXCreateFont = GetProcAddress(hDll, "D3DXCreateFontW");
    (FARPROC&)m_pD3DXCreateSprite = GetProcAddress(hDll, "D3DXCreateSprite");
  }
  else
  {
    _Error += L"No D3DX9 dll found. To enable stats, shaders and complex resizers, please make sure to install the latest DirectX End-User Runtime.\n";
  }

  m_hD3D9 = LoadLibrary("d3d9.dll");

  m_DetectedFrameRate = 0.0;
  m_DetectedFrameTime = 0.0;
  m_DetectedFrameTimeStdDev = 0.0;
  m_DetectedLock = false;
  ZeroMemory(m_DetectedFrameTimeHistory, sizeof(m_DetectedFrameTimeHistory));
  ZeroMemory(m_DetectedFrameTimeHistoryHistory, sizeof(m_DetectedFrameTimeHistoryHistory));
  m_DetectedFrameTimePos = 0;
  ZeroMemory(&m_VMR9AlphaBitmap, sizeof(m_VMR9AlphaBitmap));

  ZeroMemory(m_ldDetectedRefreshRateList, sizeof(m_ldDetectedRefreshRateList));
  ZeroMemory(m_ldDetectedScanlineRateList, sizeof(m_ldDetectedScanlineRateList));
  m_DetectedRefreshRatePos = 0;
  m_DetectedRefreshTimePrim = 0;
  m_DetectedScanlineTime = 0;
  m_DetectedScanlineTimePrim = 0;
  m_DetectedRefreshRate = 0;

  if (g_dsSettings.pRendererSettings->disableDesktopComposition)
  {
    m_bDesktopCompositionDisabled = true;
    if (g_dsSettings.m_pDwmEnableComposition)
      g_dsSettings.m_pDwmEnableComposition(0);
  }
  else
  {
    m_bDesktopCompositionDisabled = false;
  }

  hr = CreateDevice(_Error);

  m_pPSC.reset(new CPixelShaderCompiler(false));

  memset(m_pllJitter, 0, sizeof(m_pllJitter));
  memset(m_pllSyncOffset, 0, sizeof(m_pllSyncOffset));
  m_nNextJitter = 0;
  m_nNextSyncOffset = 0;
  m_llLastPerf = 0;
  m_fAvrFps = 0.0;
  m_fJitterStdDev = 0.0;
  m_fSyncOffsetStdDev = 0.0;
  m_fSyncOffsetAvr = 0.0;
  m_bSyncStatsAvailable = false;
}

CDX9AllocatorPresenter::~CDX9AllocatorPresenter()
{
  g_Windowing.Unregister(this);
  g_renderManager.UnregisterCallback();
  g_renderManager.UnInit();
  if (m_bDesktopCompositionDisabled)
  {
    m_bDesktopCompositionDisabled = false;
    if (g_dsSettings.m_pDwmEnableComposition)
      g_dsSettings.m_pDwmEnableComposition(1);
  }

  StopWorkerThreads();
  m_pFont = NULL;
  m_pLine = NULL;
  m_pD3DDev = NULL;
  m_pD3D = NULL;

  if (m_hD3D9)
  {
    FreeLibrary(m_hD3D9);
    m_hD3D9 = NULL;
  }
}

void ModerateFloat(double& Value, double Target, double& ValuePrim, double ChangeSpeed);

#if 0
class CRandom31
{
public:

  CRandom31()
  {
    m_Seed = 12164;
  }

  void f_SetSeed(int32 _Seed)
  {
    m_Seed = _Seed;
  }
  int32 f_GetSeed()
  {
    return m_Seed;
  }
  /*
  Park and Miller's psuedo-random number generator.
  */
  int32 m_Seed;
  int32 f_Get()
  {
    static const int32 A = 16807;
    static const int32 M = 2147483647;   // 2^31 - 1
    static const int32 q = M / A;       // M / A
    static const int32 r = M % A;         // M % A
    m_Seed = A * (m_Seed % q) - r * (m_Seed / q);
    if (m_Seed < 0)
      m_Seed += M;
    return m_Seed;
  }

  static int32 fs_Max()
  {
    return 2147483646;
  }

  double f_GetFloat()
  {
    return double(f_Get()) * (1.0 / double(fs_Max()));
  }
};

class CVSyncEstimation
{
private:
  class CHistoryEntry
  {
  public:
    CHistoryEntry()
    {
      m_Time = 0;
      m_ScanLine = -1;
    }
    int64_t m_Time;
    int m_ScanLine;
  };

  class CSolution
  {
  public:
    CSolution()
    {
      m_ScanLines = 1000;
      m_ScanLinesPerSecond = m_ScanLines * 100;
    }
    int m_ScanLines;
    double m_ScanLinesPerSecond;
    double m_SqrSum;

    void f_Mutate(double _Amount, CRandom31 &_Random, int _MinScans)
    {
      int ToDo = _Random.f_Get() % 10;
      if (ToDo == 0)
        m_ScanLines = m_ScanLines / 2;
      else if (ToDo == 1)
        m_ScanLines = m_ScanLines * 2;

      m_ScanLines = m_ScanLines * (1.0 + (_Random.f_GetFloat() * _Amount) - _Amount * 0.5);
      m_ScanLines = max(m_ScanLines, _MinScans);

      if (ToDo == 2)
        m_ScanLinesPerSecond /= (_Random.f_Get() % 4) + 1;
      else if (ToDo == 3)
        m_ScanLinesPerSecond *= (_Random.f_Get() % 4) + 1;

      m_ScanLinesPerSecond *= 1.0 + (_Random.f_GetFloat() * _Amount) - _Amount * 0.5;
    }

    void f_SpawnInto(CSolution &_Other, CRandom31 &_Random, int _MinScans)
    {
      _Other = *this;
      _Other.f_Mutate(_Random.f_GetFloat() * 0.1, _Random, _MinScans);
    }

    static int fs_Compare(const void *_pFirst, const void *_pSecond)
    {
      const CSolution *pFirst = (const CSolution *)_pFirst;
      const CSolution *pSecond = (const CSolution *)_pSecond;
      if (pFirst->m_SqrSum < pSecond->m_SqrSum)
        return -1;
      else if (pFirst->m_SqrSum > pSecond->m_SqrSum)
        return 1;
      return 0;
    }


  };

  enum
  {
    ENumHistory = 128
  };

  CHistoryEntry m_History[ENumHistory];
  int m_iHistory;
  CSolution m_OldSolutions[2];

  CRandom31 m_Random;


  double fp_GetSquareSum(double _ScansPerSecond, double _ScanLines)
  {
    double SquareSum = 0;
    int nHistory = min(m_nHistory, ENumHistory);
    int iHistory = m_iHistory - nHistory;
    if (iHistory < 0)
      iHistory += ENumHistory;
    for (int i = 1; i < nHistory; ++i)
    {
      int iHistory0 = iHistory + i - 1;
      int iHistory1 = iHistory + i;
      if (iHistory0 < 0)
        iHistory0 += ENumHistory;
      iHistory0 = iHistory0 % ENumHistory;
      iHistory1 = iHistory1 % ENumHistory;
      ASSERT(m_History[iHistory0].m_Time != 0);
      ASSERT(m_History[iHistory1].m_Time != 0);

      double DeltaTime = (m_History[iHistory1].m_Time - m_History[iHistory0].m_Time) / 10000000.0;
      double PredictedScanLine = m_History[iHistory0].m_ScanLine + DeltaTime * _ScansPerSecond;
      PredictedScanLine = fmod(PredictedScanLine, _ScanLines);
      double Delta = (m_History[iHistory1].m_ScanLine - PredictedScanLine);
      double DeltaSqr = Delta * Delta;
      SquareSum += DeltaSqr;
    }
    return SquareSum;
  }

  int m_nHistory;
public:

  CVSyncEstimation()
  {
    m_iHistory = 0;
    m_nHistory = 0;
  }

  void f_AddSample(int _ScanLine, int64_t _Time)
  {
    m_History[m_iHistory].m_ScanLine = _ScanLine;
    m_History[m_iHistory].m_Time = _Time;
    ++m_nHistory;
    m_iHistory = (m_iHistory + 1) % ENumHistory;
  }

  void f_GetEstimation(double &_RefreshRate, int &_ScanLines, int _ScreenSizeY, int _WindowsRefreshRate)
  {
    _RefreshRate = 0;
    _ScanLines = 0;

    int iHistory = m_iHistory;
    // We have a full history
    if (m_nHistory > 10)
    {
      for (int l = 0; l < 5; ++l)
      {
        const int nSol = 3 + 5 + 5 + 3;
        CSolution Solutions[nSol];

        Solutions[0] = m_OldSolutions[0];
        Solutions[1] = m_OldSolutions[1];
        Solutions[2].m_ScanLines = _ScreenSizeY;
        Solutions[2].m_ScanLinesPerSecond = _ScreenSizeY * _WindowsRefreshRate;

        int iStart = 3;
        for (int i = iStart; i < iStart + 5; ++i)
          Solutions[0].f_SpawnInto(Solutions[i], m_Random, _ScreenSizeY);
        iStart += 5;
        for (int i = iStart; i < iStart + 5; ++i)
          Solutions[1].f_SpawnInto(Solutions[i], m_Random, _ScreenSizeY);
        iStart += 5;
        for (int i = iStart; i < iStart + 3; ++i)
          Solutions[2].f_SpawnInto(Solutions[i], m_Random, _ScreenSizeY);

        int Start = 2;
        if (l == 0)
          Start = 0;
        for (int i = Start; i < nSol; ++i)
          Solutions[i].m_SqrSum = fp_GetSquareSum(Solutions[i].m_ScanLinesPerSecond, Solutions[i].m_ScanLines);

        qsort(Solutions, nSol, sizeof(Solutions[0]), &CSolution::fs_Compare);
        for (int i = 0; i < 2; ++i)
          m_OldSolutions[i] = Solutions[i];
      }

      _ScanLines = m_OldSolutions[0].m_ScanLines + 0.5;
      _RefreshRate = 1.0 / (m_OldSolutions[0].m_ScanLines / m_OldSolutions[0].m_ScanLinesPerSecond);
    }
    else
    {
      m_OldSolutions[0].m_ScanLines = _ScreenSizeY;
      m_OldSolutions[1].m_ScanLines = _ScreenSizeY;
    }
  }
};
#endif

void CDX9AllocatorPresenter::VSyncThread()
{
  HANDLE        hEvts[] = { m_hEvtQuit };
  bool        bQuit = false;
  TIMECAPS      tc;
  DWORD        dwResolution;
  DWORD        dwUser = 0;
  DWORD        dwTaskIndex = 0;

  timeGetDevCaps(&tc, sizeof(TIMECAPS));
  dwResolution = std::min(std::max(tc.wPeriodMin, (UINT)0), tc.wPeriodMax);
  dwUser = timeBeginPeriod(dwResolution);

  while (!bQuit)
  {

    DWORD dwObject = WaitForMultipleObjects(countof(hEvts), hEvts, FALSE, 1);
    switch (dwObject)
    {
    case WAIT_OBJECT_0:
      bQuit = true;
      break;
    case WAIT_TIMEOUT:
    {
      // Do our stuff
      if (m_pD3DDev && g_dsSettings.pRendererSettings->vSync)
      {

        int VSyncPos = GetVBlackPos();
        int WaitRange = std::max((int)(m_ScreenSize.cy / 40), 5);
        int MinRange = std::max(std::min(int(0.003 * double(m_ScreenSize.cy) * double(m_RefreshRate) + 0.5),
          int(m_ScreenSize.cy / 3)), 5); // 1.8  ms or max 33 % of Time

        VSyncPos += MinRange + WaitRange;

        VSyncPos = VSyncPos % m_ScreenSize.cy;
        if (VSyncPos < 0)
          VSyncPos += m_ScreenSize.cy;

        int ScanLine = 0;
        int StartScanLine = ScanLine;
        int LastPos = ScanLine;
        ScanLine = (VSyncPos + 1) % m_ScreenSize.cy;
        if (ScanLine < 0)
          ScanLine += m_ScreenSize.cy;
        int ScanLineMiddle = ScanLine + m_ScreenSize.cy / 2;
        ScanLineMiddle = ScanLineMiddle % m_ScreenSize.cy;
        if (ScanLineMiddle < 0)
          ScanLineMiddle += m_ScreenSize.cy;

        int ScanlineStart = ScanLine;
        bool bTakenLock;
        WaitForVBlankRange(ScanlineStart, 5, true, true, false, bTakenLock);
        int64_t TimeStart = CTimeUtils::GetPerfCounter();

        WaitForVBlankRange(ScanLineMiddle, 5, true, true, false, bTakenLock);
        int64_t TimeMiddle = CTimeUtils::GetPerfCounter();

        int ScanlineEnd = ScanLine;
        WaitForVBlankRange(ScanlineEnd, 5, true, true, false, bTakenLock);
        int64_t TimeEnd = CTimeUtils::GetPerfCounter();

        double nSeconds = double(TimeEnd - TimeStart) / 10000000.0;
        int64_t DiffMiddle = TimeMiddle - TimeStart;
        int64_t DiffEnd = TimeEnd - TimeMiddle;
        double DiffDiff;
        if (DiffEnd > DiffMiddle)
          DiffDiff = double(DiffEnd) / double(DiffMiddle);
        else
          DiffDiff = double(DiffMiddle) / double(DiffEnd);
        if (nSeconds > 0.003 && DiffDiff < 1.3)
        {
          double ScanLineSeconds;
          double nScanLines;
          if (ScanLineMiddle > ScanlineEnd)
          {
            ScanLineSeconds = double(TimeMiddle - TimeStart) / 10000000.0;
            nScanLines = ScanLineMiddle - ScanlineStart;
          }
          else
          {
            ScanLineSeconds = double(TimeEnd - TimeMiddle) / 10000000.0;
            nScanLines = ScanlineEnd - ScanLineMiddle;
          }

          double ScanLineTime = ScanLineSeconds / nScanLines;

          int iPos = m_DetectedRefreshRatePos % 100;
          m_ldDetectedScanlineRateList[iPos] = ScanLineTime;
          if (m_DetectedScanlineTime && ScanlineStart != ScanlineEnd)
          {
            int Diff = ScanlineEnd - ScanlineStart;
            nSeconds -= double(Diff) * m_DetectedScanlineTime;
          }
          m_ldDetectedRefreshRateList[iPos] = nSeconds;
          double Average = 0;
          double AverageScanline = 0;
          int nPos = std::min(iPos + 1, 100);
          for (int i = 0; i < nPos; ++i)
          {
            Average += m_ldDetectedRefreshRateList[i];
            AverageScanline += m_ldDetectedScanlineRateList[i];
          }

          if (nPos)
          {
            Average /= double(nPos);
            AverageScanline /= double(nPos);
          }
          else
          {
            Average = 0;
            AverageScanline = 0;
          }

          double ThisValue = Average;

          if (Average > 0.0 && AverageScanline > 0.0)
          {
            CAutoLock Lock(&m_RefreshRateLock);
            ++m_DetectedRefreshRatePos;
            if (m_DetectedRefreshTime == 0 || m_DetectedRefreshTime / ThisValue > 1.01 || m_DetectedRefreshTime / ThisValue < 0.99)
            {
              m_DetectedRefreshTime = ThisValue;
              m_DetectedRefreshTimePrim = 0;
            }
            ModerateFloat(m_DetectedRefreshTime, ThisValue, m_DetectedRefreshTimePrim, 1.5);
            if (m_DetectedRefreshTime > 0.0)
              m_DetectedRefreshRate = 1.0 / m_DetectedRefreshTime;
            else
              m_DetectedRefreshRate = 0.0;

            if (m_DetectedScanlineTime == 0 || m_DetectedScanlineTime / AverageScanline > 1.01 || m_DetectedScanlineTime / AverageScanline < 0.99)
            {
              m_DetectedScanlineTime = AverageScanline;
              m_DetectedScanlineTimePrim = 0;
            }
            ModerateFloat(m_DetectedScanlineTime, AverageScanline, m_DetectedScanlineTimePrim, 1.5);
            if (m_DetectedScanlineTime > 0.0)
              m_DetectedScanlinesPerFrame = m_DetectedRefreshTime / m_DetectedScanlineTime;
            else
              m_DetectedScanlinesPerFrame = 0;
          }
          //TRACE("Refresh: %f\n", RefreshRate);
        }
      }
      else
      {
        m_DetectedRefreshRate = 0.0;
        m_DetectedScanlinesPerFrame = 0.0;
      }
    }
      break;
    }
  }

  timeEndPeriod(dwResolution);
  //  if (pfAvRevertMmThreadCharacteristics) pfAvRevertMmThreadCharacteristics (hAvrt);
}


DWORD WINAPI CDX9AllocatorPresenter::VSyncThreadStatic(LPVOID lpParam)
{
  //SetThreadName((DWORD)-1, "CDX9Presenter::VSyncThread");
  CDX9AllocatorPresenter*    pThis = (CDX9AllocatorPresenter*)lpParam;
  pThis->VSyncThread();
  return 0;
}

void CDX9AllocatorPresenter::StartWorkerThreads()
{
  DWORD dwThreadId;

  if (m_bIsEVR)
  {
    m_hEvtQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hEvtQuit != NULL) // Don't create a thread with no stop switch
    {
      m_hVSyncThread = ::CreateThread(NULL, 0, VSyncThreadStatic, (LPVOID)this, 0, &dwThreadId);
      if (m_hVSyncThread != NULL)
        SetThreadPriority(m_hVSyncThread, THREAD_PRIORITY_HIGHEST);
    }
  }
}

void CDX9AllocatorPresenter::StopWorkerThreads()
{
  if (m_bIsEVR)
  {
    if (m_hEvtQuit != NULL)
    {
      SetEvent(m_hEvtQuit);
      m_drawingIsDone.Set();

      if (m_hVSyncThread != NULL)
      {
        if (WaitForSingleObject(m_hVSyncThread, 10000) == WAIT_TIMEOUT)
        {
          ASSERT(FALSE);
          TerminateThread(m_hVSyncThread, 0xDEAD);
        }

        CloseHandle(m_hVSyncThread);
        m_hVSyncThread = NULL;
      }

      CloseHandle(m_hEvtQuit);
      m_hEvtQuit = NULL;
    }
  }
}

void CDX9AllocatorPresenter::SetTime(REFERENCE_TIME rtNow)
{
  __super::SetTime(rtNow);
  if (CStreamsManager::Get() && CStreamsManager::Get()->SubtitleManager)
    CStreamsManager::Get()->SubtitleManager->SetTime(rtNow);
}

HRESULT CDX9AllocatorPresenter::CreateDevice(CStdString &_Error)
{
  StopWorkerThreads();
  m_VBlankEndWait = 0;
  m_VBlankMin = 300000;
  m_VBlankMinCalc = 300000;
  m_VBlankMax = 0;
  m_VBlankStartWait = 0;
  m_VBlankWaitTime = 0;
  m_VBlankLockTime = 0;
  m_PresentWaitTime = 0;
  m_PresentWaitTimeMin = 3000000000;
  m_PresentWaitTimeMax = 0;

  //m_LastRendererSettings = g_dsSettings.pRendererSettings->

  m_VBlankEndPresent = -100000;
  m_VBlankStartMeasureTime = 0;
  m_VBlankStartMeasure = 0;

  m_PaintTime = 0;
  m_PaintTimeMin = 3000000000;
  m_PaintTimeMax = 0;

  m_RasterStatusWaitTime = 0;
  m_RasterStatusWaitTimeMin = 3000000000;
  m_RasterStatusWaitTimeMax = 0;
  m_RasterStatusWaitTimeMaxCalc = 0;

  m_ClockDiff = 0.0;
  m_ClockDiffPrim = 0.0;
  m_ClockDiffCalc = 0.0;

  m_ModeratedTimeSpeed = 1.0;
  m_ModeratedTimeSpeedDiff = 0.0;
  m_ModeratedTimeSpeedPrim = 0;
  ZeroMemory(m_TimeChangeHistory, sizeof(m_TimeChangeHistory));
  ZeroMemory(m_ClockChangeHistory, sizeof(m_ClockChangeHistory));
  m_ClockTimeChangeHistoryPos = 0;
  //todo dx11
  /*
  m_pD3DDev = g_Windowing.Get3DDevice();
  m_pD3D = g_Windowing.Get3DObject();
  */

  m_pResizerPixelShader[0] = 0;
  m_pResizerPixelShader[1] = 0;
  m_pResizerPixelShader[2] = 0;
  m_pResizerPixelShader[3] = 0;

  D3DDISPLAYMODE d3ddm;
  HRESULT hr = S_OK;
  if (FAILED(m_pD3D->GetAdapterDisplayMode(GetAdapter(m_pD3D), &d3ddm)))
  {
    _Error += L"GetAdapterDisplayMode failed\n";
    return E_UNEXPECTED;
  }

  m_RefreshRate = d3ddm.RefreshRate;
  m_DisplayType = d3ddm.Format;
  m_ScreenSize.SetSize(d3ddm.Width, d3ddm.Height);

  D3DPRESENT_PARAMETERS pp;
  ZeroMemory(&pp, sizeof(pp));

  BOOL bCompositionEnabled = false;
  if (g_dsSettings.m_pDwmIsCompositionEnabled)
    g_dsSettings.m_pDwmIsCompositionEnabled(&bCompositionEnabled);

  m_bCompositionEnabled = bCompositionEnabled != 0;

  m_bAlternativeVSync = g_dsSettings.pRendererSettings->alterativeVSync;
  m_bHighColorResolution = m_bIsEVR && ((CEVRRendererSettings *)g_dsSettings.pRendererSettings)->highColorResolution;

  if (FAILED(hr))
  {
    _Error += "CreateDevice failed\n";

    return hr;
  }

  //
  m_filter = D3DTEXF_NONE;
  //testing here


  ZeroMemory(&m_caps, sizeof(m_caps));
  m_pD3DDev->GetDeviceCaps(&m_caps);

  if ((m_caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MINFLINEAR)
    && (m_caps.StretchRectFilterCaps&D3DPTFILTERCAPS_MAGFLINEAR))
    m_filter = D3DTEXF_LINEAR;

  m_bicubicA = 0;

  m_pFont = NULL;
  if (m_pD3DXCreateFont)
  {
    int MinSize = 1600;
    int CurrentSize = std::min((int)m_ScreenSize.cx, MinSize);
    double Scale = double(CurrentSize) / double(MinSize);
    m_TextScale = Scale;
    m_pD3DXCreateFont(m_pD3DDev,            // D3D device
      (int)(-24.0*Scale),               // Height
      (UINT)(-11.0*Scale),                     // Width
      CurrentSize < 800 ? FW_NORMAL : FW_BOLD,               // Weight
      0,                     // MipLevels, 0 = autogen mipmaps
      FALSE,                 // Italic
      DEFAULT_CHARSET,       // CharSet
      OUT_DEFAULT_PRECIS,    // OutputPrecision
      ANTIALIASED_QUALITY,       // Quality
      FIXED_PITCH | FF_DONTCARE, // PitchAndFamily
      L"Lucida Console",              // pFaceName
      &m_pFont);              // ppFont
  }

  m_pSprite = NULL;

  if (m_pD3DXCreateSprite)
  {
    m_pD3DXCreateSprite(m_pD3DDev,            // D3D device
      &m_pSprite);
  }

  m_pLine = NULL;
  if (m_pD3DXCreateLine)
    m_pD3DXCreateLine(m_pD3DDev, &m_pLine);

  StartWorkerThreads();

  return S_OK;
}

HRESULT CDX9AllocatorPresenter::AllocSurfaces(D3DFORMAT Format)
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);

  for (size_t i = 0; i < m_nNbDXSurface + 3; i++)
  {
    m_pVideoTexture[i] = NULL;
    m_pVideoSurface[i] = NULL;
  }

  m_pScreenSizeTemporaryTexture[0] = NULL;
  m_pScreenSizeTemporaryTexture[1] = NULL;

  m_SurfaceType = Format;

  HRESULT hr;
  if (g_dsSettings.pRendererSettings->apSurfaceUsage == VIDRNDT_AP_TEXTURE2D || g_dsSettings.pRendererSettings->apSurfaceUsage == VIDRNDT_AP_TEXTURE3D)
  {
    int nTexturesNeeded = g_dsSettings.pRendererSettings->apSurfaceUsage == VIDRNDT_AP_TEXTURE3D ? m_nNbDXSurface + 2 : 1;

    for (int i = 0; i < nTexturesNeeded; i++)
    {
      if (FAILED(hr = m_pD3DDev->CreateTexture(
        m_NativeVideoSize.cx, m_NativeVideoSize.cy, 1,
        D3DUSAGE_RENDERTARGET, Format/*D3DFMT_X8R8G8B8 D3DFMT_A8R8G8B8*/,
        D3DPOOL_DEFAULT, &m_pVideoTexture[i], NULL)))
        return hr;

      if (FAILED(hr = m_pVideoTexture[i]->GetSurfaceLevel(0, &m_pVideoSurface[i])))
        return hr;
    }

    // Rendering target
    uint32_t height = 0, width = 0;
    //todo dx11
    //g_Windowing.GetBackbufferSize(width, height);
    if (FAILED(hr = m_pD3DDev->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, Format,
      D3DPOOL_DEFAULT, &m_pVideoTexture[m_nNbDXSurface + 2], NULL)))
      return hr;

    if (FAILED(hr = m_pVideoTexture[m_nNbDXSurface + 2]->GetSurfaceLevel(0, &m_pVideoSurface[m_nNbDXSurface + 2])))
      return hr;

    if (g_dsSettings.pRendererSettings->apSurfaceUsage == VIDRNDT_AP_TEXTURE2D)
    {
      for (size_t i = 0; i < m_nNbDXSurface + 3; i++)
      {
        m_pVideoTexture[i] = NULL;
      }
    }
  }
  else
  {
    if (FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(
      m_NativeVideoSize.cx, m_NativeVideoSize.cy,
      D3DFMT_X8R8G8B8/*D3DFMT_A8R8G8B8*/,
      D3DPOOL_DEFAULT, &m_pVideoSurface[m_nCurSurface], NULL)))
      return hr;
  }

  hr = m_pD3DDev->ColorFill(m_pVideoSurface[m_nCurSurface], NULL, 0);

  return S_OK;
}

void CDX9AllocatorPresenter::DeleteSurfaces()
{
  CAutoLock cAutoLock(this);
  CAutoLock cRenderLock(&m_RenderLock);

  m_pScreenSizeTemporaryTexture[0].Release();
  m_pScreenSizeTemporaryTexture[1].Release();

  for (size_t i = 0; i < m_nNbDXSurface + 2; i++)
  {
    m_pVideoTexture[i].Release();
    m_pVideoSurface[i].Release();
  }
}

uint32_t CDX9AllocatorPresenter::GetAdapter(IDirect3D9* pD3D, bool CreateDevice)
{
  if (m_hWnd == NULL || pD3D == NULL)
    return D3DADAPTER_DEFAULT;

  if (CreateDevice && (pD3D->GetAdapterCount() > 1) && (g_dsSettings.D3D9RenderDevice != _T(L"")))
  {
    WCHAR    strGUID[50];
    D3DADAPTER_IDENTIFIER9 adapterIdentifier;
    m_D3D9Device = _T("");

    for (UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp)
    {
      if (pD3D->GetAdapterIdentifier(adp, 0, &adapterIdentifier) == S_OK)
      {
        if ((::StringFromGUID2(adapterIdentifier.DeviceIdentifier, strGUID, 50) > 0) && (g_dsSettings.D3D9RenderDevice == strGUID))
        {
          m_D3D9Device = adapterIdentifier.Description;
          return  adp;
        }
      }
    }
  }

  HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
  if (hMonitor == NULL) return D3DADAPTER_DEFAULT;

  for (UINT adp = 0, num_adp = pD3D->GetAdapterCount(); adp < num_adp; ++adp)
  {
    HMONITOR hAdpMon = pD3D->GetAdapterMonitor(adp);
    if (hAdpMon == hMonitor)
    {
      if (CreateDevice)
      {
        D3DADAPTER_IDENTIFIER9 adapterIdentifier;
        if (pD3D->GetAdapterIdentifier(adp, 0, &adapterIdentifier) == S_OK)
          m_D3D9Device = adapterIdentifier.Description;
      }
      return adp;
    }
  }

  return D3DADAPTER_DEFAULT;
}

// ISubPicAllocatorPresenter

STDMETHODIMP CDX9AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
  return E_NOTIMPL;
}

HRESULT CDX9AllocatorPresenter::InitResizers(float bicubicA, bool bNeedScreenSizeTexture)
{
  HRESULT hr;

  do
  {
    if (bicubicA)
    {
      if (!m_pResizerPixelShader[0])
        break;
      if (!m_pResizerPixelShader[1])
        break;
      if (!m_pResizerPixelShader[2])
        break;
      if (!m_pResizerPixelShader[3])
        break;
      if (m_bicubicA != bicubicA)
        break;
      if (!m_pScreenSizeTemporaryTexture[0])
        break;
      if (bNeedScreenSizeTexture)
      {
        if (!m_pScreenSizeTemporaryTexture[1])
          break;
      }
    }
    else
    {
      if (!m_pResizerPixelShader[0])
        break;
      if (bNeedScreenSizeTexture)
      {
        if (!m_pScreenSizeTemporaryTexture[0])
          break;
        if (!m_pScreenSizeTemporaryTexture[1])
          break;
      }
    }
    return S_OK;
  } while (0);

  m_bicubicA = bicubicA;
  m_pScreenSizeTemporaryTexture[0] = NULL;
  m_pScreenSizeTemporaryTexture[1] = NULL;

  for (int i = 0; i < countof(m_pResizerPixelShader); i++)
    m_pResizerPixelShader[i] = NULL;

  if (m_caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
    return E_FAIL;

  LPCSTR pProfile = m_caps.PixelShaderVersion >= D3DPS_VERSION(3, 0) ? "ps_3_0" : "ps_2_0";

  CStdString str;
  XFILE::CFileStream file;
  CStdString filename = "special://xbmc/system/players/dsplayer/Shaders/resizer.psh";
  if (!file.Open(filename))
  {
    CLog::Log(LOGERROR, "CWinRenderer::LoadEffect - failed to open file %s", filename.c_str());
    return false;
  }
  getline(file, str, '\0');

  //if(!LoadResource(IDF_SHADER_RESIZER, str, _T("FILE")))
  //  return E_FAIL;

  CStdStringA A;
  A.Format("(%f)", bicubicA);
  str.Replace("_The_Value_Of_A_Is_Set_Here_", A);

  LPCSTR pEntries[] = { "main_bilinear", "main_bicubic1pass", "main_bicubic2pass_pass1", "main_bicubic2pass_pass2" };

  ASSERT(countof(pEntries) == countof(m_pResizerPixelShader));
  for (int i = 0; i < countof(pEntries); i++)
  {
    CStdString ErrorMessage;
    CStdString DissAssembly;
    hr = m_pPSC->CompileShader(str, pEntries[i], pProfile, 0, &m_pResizerPixelShader[i], &DissAssembly, &ErrorMessage);
    if (FAILED(hr))
    {
      //TRACE("%ws", ErrorMessage.GetString());
      ASSERT(0);
      return hr;
    }
  }

  if (m_bicubicA || bNeedScreenSizeTexture)
  {
    if (FAILED(m_pD3DDev->CreateTexture(
      std::min((int)m_ScreenSize.cx, (int)m_caps.MaxTextureWidth), std::min((int)std::max(m_ScreenSize.cy, m_NativeVideoSize.cy), (int)m_caps.MaxTextureHeight), 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
      D3DPOOL_DEFAULT, &m_pScreenSizeTemporaryTexture[0], NULL)))
    {
      ASSERT(0);
      m_pScreenSizeTemporaryTexture[0] = NULL; // will do 1 pass then
    }
  }
  if (m_bicubicA || bNeedScreenSizeTexture)
  {
    if (FAILED(m_pD3DDev->CreateTexture(
      std::min((int)m_ScreenSize.cx, (int)m_caps.MaxTextureWidth), std::min((int)std::max(m_ScreenSize.cy, m_NativeVideoSize.cy), (int)m_caps.MaxTextureHeight), 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
      D3DPOOL_DEFAULT, &m_pScreenSizeTemporaryTexture[1], NULL)))
    {
      ASSERT(0);
      m_pScreenSizeTemporaryTexture[1] = NULL; // will do 1 pass then
    }
  }

  return S_OK;
}

static bool ClipToSurface(IDirect3DSurface9* pSurface, Com::SmartRect& s, Com::SmartRect& d)
{
  D3DSURFACE_DESC d3dsd;
  ZeroMemory(&d3dsd, sizeof(d3dsd));
  if (FAILED(pSurface->GetDesc(&d3dsd)))
    return(false);

  int w = d3dsd.Width, h = d3dsd.Height;
  int sw = s.Width(), sh = s.Height();
  int dw = d.Width(), dh = d.Height();

  if (d.left >= w || d.right < 0 || d.top >= h || d.bottom < 0
    || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
  {
    s.SetRectEmpty();
    d.SetRectEmpty();
    return(true);
  }

  if (d.right > w) { s.right -= (d.right - w)*sw / dw; d.right = w; }
  if (d.bottom > h) { s.bottom -= (d.bottom - h)*sh / dh; d.bottom = h; }
  if (d.left < 0) { s.left += (0 - d.left)*sw / dw; d.left = 0; }
  if (d.top < 0) { s.top += (0 - d.top)*sh / dh; d.top = 0; }

  return(true);
}

HRESULT CDX9AllocatorPresenter::TextureCopy(Com::SmartPtr<IDirect3DTexture9> pTexture)
{
  HRESULT hr;

  D3DSURFACE_DESC desc;
  if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
    return E_FAIL;

  float w = (float)desc.Width;
  float h = (float)desc.Height;

  MYD3DVERTEX<1> v[] =
  {
    { 0, 0, 0.5f, 2.0f, 0, 0 },
    { w, 0, 0.5f, 2.0f, 1, 0 },
    { 0, h, 0.5f, 2.0f, 0, 1 },
    { w, h, 0.5f, 2.0f, 1, 1 },
  };

  for (int i = 0; i < countof(v); i++)
  {
    v[i].x -= 0.5;
    v[i].y -= 0.5;
  }

  hr = m_pD3DDev->SetTexture(0, pTexture);

  return TextureBlt(m_pD3DDev, v, D3DTEXF_LINEAR);
}

HRESULT CDX9AllocatorPresenter::DrawRect(DWORD _Color, DWORD _Alpha, const Com::SmartRect &_Rect)
{
  DWORD Color = D3DCOLOR_ARGB(_Alpha, GetRValue(_Color), GetGValue(_Color), GetBValue(_Color));
  MYD3DVERTEX<0> v[] =
  {
    { float(_Rect.left), float(_Rect.top), 0.5f, 2.0f, Color },
    { float(_Rect.right), float(_Rect.top), 0.5f, 2.0f, Color },
    { float(_Rect.left), float(_Rect.bottom), 0.5f, 2.0f, Color },
    { float(_Rect.right), float(_Rect.bottom), 0.5f, 2.0f, Color },
  };

  for (int i = 0; i < countof(v); i++)
  {
    v[i].x -= 0.5;
    v[i].y -= 0.5;
  }

  return ::DrawRect(m_pD3DDev, v);
}

HRESULT CDX9AllocatorPresenter::TextureResize(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], D3DTEXTUREFILTERTYPE filter, const Com::SmartRect &SrcRect)
{
  HRESULT hr;

  D3DSURFACE_DESC desc;
  if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
    return E_FAIL;

  float w = (float)desc.Width;
  float h = (float)desc.Height;

  float dx2 = 1.0f / w;
  float dy2 = 1.0f / h;

  MYD3DVERTEX<1> v[] =
  {
    { dst[0].x, dst[0].y, dst[0].z, 1.0f / dst[0].z, SrcRect.left * dx2, SrcRect.top * dy2 },
    { dst[1].x, dst[1].y, dst[1].z, 1.0f / dst[1].z, SrcRect.right * dx2, SrcRect.top * dy2 },
    { dst[2].x, dst[2].y, dst[2].z, 1.0f / dst[2].z, SrcRect.left * dx2, SrcRect.bottom * dy2 },
    { dst[3].x, dst[3].y, dst[3].z, 1.0f / dst[3].z, SrcRect.right * dx2, SrcRect.bottom * dy2 },
  };

  AdjustQuad(v, 0, 0);

  hr = m_pD3DDev->SetTexture(0, pTexture);
  hr = m_pD3DDev->SetPixelShader(NULL);

  hr = TextureBlt(m_pD3DDev, v, filter);

  return hr;
}

HRESULT CDX9AllocatorPresenter::TextureResizeBilinear(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect)
{
  HRESULT hr;

  D3DSURFACE_DESC desc;
  if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
    return E_FAIL;

  // make const to give compiler a chance of optimising, also float faster than double and converted to float to sent to PS anyway
  const float dx = 1.0f / (float)desc.Width;
  const float dy = 1.0f / (float)desc.Height;
  const float tx0 = (float)SrcRect.left;
  const float tx1 = (float)SrcRect.right;
  const float ty0 = (float)SrcRect.top;
  const float ty1 = (float)SrcRect.bottom;

  MYD3DVERTEX<1> v[] =
  {
    { dst[0].x, dst[0].y, dst[0].z, 1.0f / dst[0].z, tx0, ty0 },
    { dst[1].x, dst[1].y, dst[1].z, 1.0f / dst[1].z, tx1, ty0 },
    { dst[2].x, dst[2].y, dst[2].z, 1.0f / dst[2].z, tx0, ty1 },
    { dst[3].x, dst[3].y, dst[3].z, 1.0f / dst[3].z, tx1, ty1 },
  };

  AdjustQuad(v, 1.0, 1.0);

  hr = m_pD3DDev->SetTexture(0, pTexture);

  float fConstData[][4] = { { dx*0.5f, dy*0.5f, 0, 0 }, { dx, dy, 0, 0 }, { dx, 0, 0, 0 }, { 0, dy, 0, 0 } };
  hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

  hr = m_pD3DDev->SetTexture(0, pTexture);
  hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShader[0]);

  hr = TextureBlt(m_pD3DDev, v, D3DTEXF_POINT);

  m_pD3DDev->SetPixelShader(NULL);

  return hr;
}

HRESULT CDX9AllocatorPresenter::TextureResizeBicubic1pass(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect)
{
  HRESULT hr;

  D3DSURFACE_DESC desc;
  if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
    return E_FAIL;

  // make const to give compiler a chance of optimising, also float faster than double and converted to float to sent to PS anyway
  const float dx = 1.0f / (float)desc.Width;
  const float dy = 1.0f / (float)desc.Height;
  const float tx0 = (float)SrcRect.left;
  const float tx1 = (float)SrcRect.right;
  const float ty0 = (float)SrcRect.top;
  const float ty1 = (float)SrcRect.bottom;

  MYD3DVERTEX<1> v[] =
  {
    { dst[0].x, dst[0].y, dst[0].z, 1.0f / dst[0].z, tx0, ty0 },
    { dst[1].x, dst[1].y, dst[1].z, 1.0f / dst[1].z, tx1, ty0 },
    { dst[2].x, dst[2].y, dst[2].z, 1.0f / dst[2].z, tx0, ty1 },
    { dst[3].x, dst[3].y, dst[3].z, 1.0f / dst[3].z, tx1, ty1 },
  };

  AdjustQuad(v, 1.0, 1.0);

  hr = m_pD3DDev->SetTexture(0, pTexture);

  float fConstData[][4] = { { dx*0.5f, dy*0.5f, 0, 0 }, { dx, dy, 0, 0 }, { dx, 0, 0, 0 }, { 0, dy, 0, 0 } };
  hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

  hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShader[1]);

  hr = TextureBlt(m_pD3DDev, v, D3DTEXF_POINT);

  m_pD3DDev->SetPixelShader(NULL);

  return hr;
}

HRESULT CDX9AllocatorPresenter::TextureResizeBicubic2pass(Com::SmartPtr<IDirect3DTexture9> pTexture, Vector dst[4], const Com::SmartRect &SrcRect)
{
  // The 2 pass sampler is incorrect in that it only does bilinear resampling in the y direction.
  return TextureResizeBicubic1pass(pTexture, dst, SrcRect);

  /*HRESULT hr;

  // rotated?
  if(dst[0].z != dst[1].z || dst[2].z != dst[3].z || dst[0].z != dst[3].z
  || dst[0].y != dst[1].y || dst[0].x != dst[2].x || dst[2].y != dst[3].y || dst[1].x != dst[3].x)
  return TextureResizeBicubic1pass(pTexture, dst, SrcRect);

  D3DSURFACE_DESC desc;
  if(!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc)))
  return E_FAIL;

  float Tex0_Width = (float) desc.Width;
  float Tex0_Height = (float) desc.Height;

  double dx0 = 1.0/desc.Width;
  double dy0 = 1.0/desc.Height;

  Com::SmartSize SrcTextSize = Com::SmartSize(desc.Width, desc.Height);
  double w = (double)SrcRect.Width();
  double h = (double)SrcRect.Height();

  Com::SmartRect dst1(0, 0, (int)(dst[3].x - dst[0].x), (int)h);

  if(!m_pScreenSizeTemporaryTexture[0] || FAILED(m_pScreenSizeTemporaryTexture[0]->GetLevelDesc(0, &desc)))
  return TextureResizeBicubic1pass(pTexture, dst, SrcRect);

  float Tex1_Width = (float) desc.Width;
  float Tex1_Height = (float) desc.Height;

  double dx1 = 1.0/desc.Width;
  double dy1 = 1.0/desc.Height;

  double dw = (double)dst1.Width() / desc.Width;
  double dh = (double)dst1.Height() / desc.Height;

  float dx2 = 1.0f/SrcTextSize.cx;
  float dy2 = 1.0f/SrcTextSize.cy;
  float tx0 = (float) SrcRect.left;
  float tx1 = (float) SrcRect.right;
  float ty0 = (float) SrcRect.top;
  float ty1 = (float) SrcRect.bottom;

  float tx0_2 = 0;
  float tx1_2 = (float) dst1.Width();
  float ty0_2 = 0;
  float ty1_2 = (float) h;

  //  ASSERT(dst1.Height() == desc.Height);

  if(dst1.Width() > (int)desc.Width || dst1.Height() > (int)desc.Height)
  // if(dst1.Width() != desc.Width || dst1.Height() != desc.Height)
  return TextureResizeBicubic1pass(pTexture, dst, SrcRect);

  MYD3DVERTEX<1> vx[] =
  {
  {(float)dst1.left, (float)dst1.top,    0.5f, 2.0f, tx0, ty0},
  {(float)dst1.right, (float)dst1.top,  0.5f, 2.0f, tx1, ty0},
  {(float)dst1.left, (float)dst1.bottom,  0.5f, 2.0f, tx0, ty1},
  {(float)dst1.right, (float)dst1.bottom, 0.5f, 2.0f, tx1, ty1},
  };

  AdjustQuad(vx, 1.0, 0.0);

  MYD3DVERTEX<1> vy[] =
  {
  {dst[0].x, dst[0].y, dst[0].z, 1.0f/dst[0].z, tx0_2, ty0_2},
  {dst[1].x, dst[1].y, dst[1].z, 1.0f/dst[1].z, tx1_2, ty0_2},
  {dst[2].x, dst[2].y, dst[2].z, 1.0f/dst[2].z, tx0_2, ty1_2},
  {dst[3].x, dst[3].y, dst[3].z, 1.0f/dst[3].z, tx1_2, ty1_2},
  };


  AdjustQuad(vy, 0.0, 1.0);

  hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShader[2]);
  {
  float fConstData[][4] = {{0.5f / Tex0_Width, 0.5f / Tex0_Height, 0, 0}, {1.0f / Tex0_Width, 1.0f / Tex0_Height, 0, 0}, {1.0f / Tex0_Width, 0, 0, 0}, {0, 1.0f / Tex0_Height, 0, 0}, {Tex0_Width, Tex0_Height, 0, 0}};
  hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));
  }

  hr = m_pD3DDev->SetTexture(0, pTexture);

  Com::SmartPtr<IDirect3DSurface9> pRTOld;
  hr = m_pD3DDev->GetRenderTarget(0, &pRTOld);

  Com::SmartPtr<IDirect3DSurface9> pRT;
  hr = m_pScreenSizeTemporaryTexture[0]->GetSurfaceLevel(0, &pRT);
  hr = m_pD3DDev->SetRenderTarget(0, pRT);

  hr = TextureBlt(m_pD3DDev, vx, D3DTEXF_POINT);

  hr = m_pD3DDev->SetPixelShader(m_pResizerPixelShader[3]);
  {
  float fConstData[][4] = {{0.5f / Tex1_Width, 0.5f / Tex1_Height, 0, 0}, {1.0f / Tex1_Width, 1.0f / Tex1_Height, 0, 0}, {1.0f / Tex1_Width, 0, 0, 0}, {0, 1.0f / Tex1_Height, 0, 0}, {Tex1_Width, Tex1_Height, 0, 0}};
  hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));
  }

  hr = m_pD3DDev->SetTexture(0, m_pScreenSizeTemporaryTexture[0]);

  hr = m_pD3DDev->SetRenderTarget(0, pRTOld);

  hr = TextureBlt(m_pD3DDev, vy, D3DTEXF_POINT);

  m_pD3DDev->SetPixelShader(NULL);

  return hr;*/
}

HRESULT CDX9AllocatorPresenter::AlphaBlt(RECT* pSrc, RECT* pDst, Com::SmartPtr<IDirect3DTexture9> pTexture)
{
  if (!pSrc || !pDst)
    return E_POINTER;

  Com::SmartRect src(*pSrc), dst(*pDst);

  HRESULT hr;

  D3DSURFACE_DESC d3dsd;
  ZeroMemory(&d3dsd, sizeof(d3dsd));
  if (FAILED(pTexture->GetLevelDesc(0, &d3dsd)) /*|| d3dsd.Type != D3DRTYPE_TEXTURE*/)
    return E_FAIL;

  float w = (float)d3dsd.Width;
  float h = (float)d3dsd.Height;

  MYD3DVERTEX<1> pVertices[] =
  {
    { (float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / w, (float)src.top / h },
    { (float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / w, (float)src.top / h },
    { (float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / w, (float)src.bottom / h },
    { (float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / w, (float)src.bottom / h },
  };
  AdjustQuad(pVertices, 0, 0);

  hr = m_pD3DDev->SetTexture(0, pTexture);
  hr = m_pD3DDev->SetPixelShader(NULL);

  DWORD abe, sb, db;
  hr = m_pD3DDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &abe);
  hr = m_pD3DDev->GetRenderState(D3DRS_SRCBLEND, &sb);
  hr = m_pD3DDev->GetRenderState(D3DRS_DESTBLEND, &db);

  hr = m_pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  hr = m_pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
  hr = m_pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
  hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
  hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
  hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
  hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

  hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

  hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
  hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

  MYD3DVERTEX<1> tmp = pVertices[2];
  pVertices[2] = pVertices[3];
  pVertices[3] = tmp;
  hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, pVertices, sizeof(pVertices[0]));

  m_pD3DDev->SetTexture(0, NULL);

  m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
  m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, sb);
  m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, db);

  return S_OK;
}

void CDX9AllocatorPresenter::CalculateJitter(int64_t PerfCounter)
{
  // Calculate the jitter!
  int64_t  llPerf = PerfCounter;
  if ((m_rtTimePerFrame != 0) && (labs((long)(llPerf - m_llLastPerf)) < m_rtTimePerFrame * 3))
  {
    m_nNextJitter = (m_nNextJitter + 1) % NB_JITTER;
    m_pllJitter[m_nNextJitter] = llPerf - m_llLastPerf;

    m_MaxJitter = MINLONG64;
    m_MinJitter = MAXLONG64;

    // Calculate the real FPS
    int64_t    llJitterSum = 0;
    int64_t    llJitterSumAvg = 0;
    for (int i = 0; i < NB_JITTER; i++)
    {
      int64_t Jitter = m_pllJitter[i];
      llJitterSum += Jitter;
      llJitterSumAvg += Jitter;
    }
    double FrameTimeMean = double(llJitterSumAvg) / NB_JITTER;
    m_fJitterMean = FrameTimeMean;
    double DeviationSum = 0;
    for (int i = 0; i < NB_JITTER; i++)
    {
      __int64 DevInt = (__int64)(m_pllJitter[i] - FrameTimeMean);
      double Deviation = (double)DevInt;
      DeviationSum += Deviation*Deviation;
      m_MaxJitter = std::max(m_MaxJitter, DevInt);
      m_MinJitter = std::min(m_MinJitter, DevInt);
    }
    double StdDev = sqrt(DeviationSum / NB_JITTER);

    m_fJitterStdDev = StdDev;

    m_fAvrFps = 10000000.0 / (double(llJitterSum) / NB_JITTER);
  }

  m_llLastPerf = llPerf;
}

bool CDX9AllocatorPresenter::GetVBlank(int &_ScanLine, int &_bInVBlank, bool _bMeasureTime)
{

  if (m_bPendingResetDevice)
    return false;

  int64_t llPerf = 0;
  if (_bMeasureTime)
    llPerf = CTimeUtils::GetPerfCounter();

  int ScanLine = 0;
  _ScanLine = 0;
  _bInVBlank = 0;
  D3DRASTER_STATUS RasterStatus;
  if (m_pD3DDev->GetRasterStatus(0, &RasterStatus) != S_OK)
    return false;;
  ScanLine = RasterStatus.ScanLine;
  _bInVBlank = RasterStatus.InVBlank;

  if (_bMeasureTime)
  {
    m_VBlankMax = std::max(m_VBlankMax, ScanLine);
    if (ScanLine != 0 && !_bInVBlank)
      m_VBlankMinCalc = std::min(m_VBlankMinCalc, ScanLine);
    m_VBlankMin = m_VBlankMax - m_ScreenSize.cy;
  }
  if (_bInVBlank)
    _ScanLine = 0;
  else if (m_VBlankMin != 300000)
    _ScanLine = ScanLine - m_VBlankMin;
  else
    _ScanLine = ScanLine;

  if (_bMeasureTime)
  {
    int64_t Time = CTimeUtils::GetPerfCounter() - llPerf;
    m_RasterStatusWaitTimeMaxCalc = std::max(m_RasterStatusWaitTimeMaxCalc, Time);
  }

  return true;
}

bool CDX9AllocatorPresenter::WaitForVBlankRange(int &_RasterStart, int _RasterSize, bool _bWaitIfInside, bool _bNeedAccurate, bool _bMeasure, bool &_bTakenLock)
{
  if (_bMeasure)
    m_RasterStatusWaitTimeMaxCalc = 0;
  bool bWaited = false;
  int ScanLine = 0;
  int InVBlank = 0;
  int64_t llPerf;
  if (_bMeasure)
    llPerf = CTimeUtils::GetPerfCounter();
  GetVBlank(ScanLine, InVBlank, _bMeasure);
  if (_bMeasure)
    m_VBlankStartWait = ScanLine;

  static bool bOneWait = true;
  if (bOneWait && _bMeasure)
  {
    bOneWait = false;
    // If we are already in the wanted interval we need to wait until we aren't, this improves sync when for example you are playing 23.976 Hz material on a 24 Hz refresh rate
    int nInVBlank = 0;
    while (1)
    {
      if (!GetVBlank(ScanLine, InVBlank, _bMeasure))
        break;

      if (InVBlank && nInVBlank == 0)
      {
        nInVBlank = 1;
      }
      else if (!InVBlank && nInVBlank == 1)
      {
        nInVBlank = 2;
      }
      else if (InVBlank && nInVBlank == 2)
      {
        nInVBlank = 3;
      }
      else if (!InVBlank && nInVBlank == 3)
      {
        break;
      }
    }
  }
  if (_bWaitIfInside)
  {
    int ScanLineDiff = long(ScanLine) - _RasterStart;
    if (ScanLineDiff > m_ScreenSize.cy / 2)
      ScanLineDiff -= m_ScreenSize.cy;
    else if (ScanLineDiff < -m_ScreenSize.cy / 2)
      ScanLineDiff += m_ScreenSize.cy;

    if (ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize)
    {
      bWaited = true;
      // If we are already in the wanted interval we need to wait until we aren't, this improves sync when for example you are playing 23.976 Hz material on a 24 Hz refresh rate
      int LastLineDiff = ScanLineDiff;
      while (1)
      {
        if (!GetVBlank(ScanLine, InVBlank, _bMeasure))
          break;
        int ScanLineDiff = long(ScanLine) - _RasterStart;
        if (ScanLineDiff > m_ScreenSize.cy / 2)
          ScanLineDiff -= m_ScreenSize.cy;
        else if (ScanLineDiff < -m_ScreenSize.cy / 2)
          ScanLineDiff += m_ScreenSize.cy;
        if (!(ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize) || (LastLineDiff < 0 && ScanLineDiff > 0))
          break;
        LastLineDiff = ScanLineDiff;
        Sleep(1); // Just sleep
      }
    }
  }
  double RefreshRate = GetRefreshRate();
  LONG ScanLines = GetScanLines();
  int MinRange = std::max(std::min(int(0.0015 * double(ScanLines) * RefreshRate + 0.5),
    int(ScanLines / 3)), 5); // 1.5 ms or max 33 % of Time
  int NoSleepStart = _RasterStart - MinRange;
  int NoSleepRange = MinRange;
  if (NoSleepStart < 0)
    NoSleepStart += m_ScreenSize.cy;

  int MinRange2 = std::max(std::min(int(0.0050 * double(ScanLines) * RefreshRate + 0.5),
    int(ScanLines / 3)), 5); // 5 ms or max 33 % of Time
  int D3DDevLockStart = _RasterStart - MinRange2;
  int D3DDevLockRange = MinRange2;
  if (D3DDevLockStart < 0)
    D3DDevLockStart += m_ScreenSize.cy;

  int ScanLineDiff = ScanLine - _RasterStart;
  if (ScanLineDiff > m_ScreenSize.cy / 2)
    ScanLineDiff -= m_ScreenSize.cy;
  else if (ScanLineDiff < -m_ScreenSize.cy / 2)
    ScanLineDiff += m_ScreenSize.cy;
  int LastLineDiff = ScanLineDiff;


  int ScanLineDiffSleep = long(ScanLine) - NoSleepStart;
  if (ScanLineDiffSleep > m_ScreenSize.cy / 2)
    ScanLineDiffSleep -= m_ScreenSize.cy;
  else if (ScanLineDiffSleep < -m_ScreenSize.cy / 2)
    ScanLineDiffSleep += m_ScreenSize.cy;
  int LastLineDiffSleep = ScanLineDiffSleep;


  int ScanLineDiffLock = long(ScanLine) - D3DDevLockStart;
  if (ScanLineDiffLock > m_ScreenSize.cy / 2)
    ScanLineDiffLock -= m_ScreenSize.cy;
  else if (ScanLineDiffLock < -m_ScreenSize.cy / 2)
    ScanLineDiffLock += m_ScreenSize.cy;
  int LastLineDiffLock = ScanLineDiffLock;

  int64_t llPerfLock = 0;

  while (1)
  {
    if (!GetVBlank(ScanLine, InVBlank, _bMeasure))
      break;
    int ScanLineDiff = long(ScanLine) - _RasterStart;
    if (ScanLineDiff > m_ScreenSize.cy / 2)
      ScanLineDiff -= m_ScreenSize.cy;
    else if (ScanLineDiff < -m_ScreenSize.cy / 2)
      ScanLineDiff += m_ScreenSize.cy;
    if ((ScanLineDiff >= 0 && ScanLineDiff <= _RasterSize) || (LastLineDiff < 0 && ScanLineDiff > 0))
      break;

    LastLineDiff = ScanLineDiff;

    bWaited = true;

    int ScanLineDiffLock = long(ScanLine) - D3DDevLockStart;
    if (ScanLineDiffLock > m_ScreenSize.cy / 2)
      ScanLineDiffLock -= m_ScreenSize.cy;
    else if (ScanLineDiffLock < -m_ScreenSize.cy / 2)
      ScanLineDiffLock += m_ScreenSize.cy;

    if (((ScanLineDiffLock >= 0 && ScanLineDiffLock <= D3DDevLockRange) || (LastLineDiffLock < 0 && ScanLineDiffLock > 0)))
    {
      if (!_bTakenLock && _bMeasure)
      {
        _bTakenLock = true;
        llPerfLock = CTimeUtils::GetPerfCounter();
        LockD3DDevice();
      }
    }
    LastLineDiffLock = ScanLineDiffLock;


    int ScanLineDiffSleep = long(ScanLine) - NoSleepStart;
    if (ScanLineDiffSleep > m_ScreenSize.cy / 2)
      ScanLineDiffSleep -= m_ScreenSize.cy;
    else if (ScanLineDiffSleep < -m_ScreenSize.cy / 2)
      ScanLineDiffSleep += m_ScreenSize.cy;

    if (!((ScanLineDiffSleep >= 0 && ScanLineDiffSleep <= NoSleepRange) || (LastLineDiffSleep < 0 && ScanLineDiffSleep > 0)) || !_bNeedAccurate)
    {
      //TRACE("%d\n", RasterStatus.ScanLine);
      Sleep(1); // Don't sleep for the last 1.5 ms scan lines, so we get maximum precision
    }
    LastLineDiffSleep = ScanLineDiffSleep;
  }
  _RasterStart = ScanLine;
  if (_bMeasure)
  {
    m_VBlankEndWait = ScanLine;
    m_VBlankWaitTime = CTimeUtils::GetPerfCounter() - llPerf;

    if (_bTakenLock)
    {
      m_VBlankLockTime = CTimeUtils::GetPerfCounter() - llPerfLock;
    }
    else
      m_VBlankLockTime = 0;

    m_RasterStatusWaitTime = m_RasterStatusWaitTimeMaxCalc;
    m_RasterStatusWaitTimeMin = std::min(m_RasterStatusWaitTimeMin, m_RasterStatusWaitTime);
    m_RasterStatusWaitTimeMax = std::max(m_RasterStatusWaitTimeMax, m_RasterStatusWaitTime);
  }

  return bWaited;
}

int CDX9AllocatorPresenter::GetVBlackPos()
{
  BOOL bCompositionEnabled = m_bCompositionEnabled;

  int WaitRange = std::max(int(m_ScreenSize.cy / 40), 5);
  if (!bCompositionEnabled)
  {
    if (m_bAlternativeVSync)
    {
      return g_dsSettings.pRendererSettings->vSyncOffset;
    }
    else
    {
      int MinRange = std::max(std::min(int(0.005 * double(m_ScreenSize.cy) * GetRefreshRate() + 0.5),
        int(m_ScreenSize.cy / 3)), 5); // 5  ms or max 33 % of Time
      int WaitFor = m_ScreenSize.cy - (MinRange + WaitRange);
      return WaitFor;
    }
  }
  else
  {
    int WaitFor = m_ScreenSize.cy / 2;
    return WaitFor;
  }
}


bool CDX9AllocatorPresenter::WaitForVBlank(bool &_Waited, bool &_bTakenLock)
{
  if (!g_dsSettings.pRendererSettings->vSync)
  {
    _Waited = true;
    m_VBlankWaitTime = 0;
    m_VBlankLockTime = 0;
    m_VBlankEndWait = 0;
    m_VBlankStartWait = 0;
    return true;
  }
  //  _Waited = true;
  //  return false;

  BOOL bCompositionEnabled = m_bCompositionEnabled;
  int WaitFor = GetVBlackPos();

  if (!bCompositionEnabled)
  {
    if (m_bAlternativeVSync)
    {
      _Waited = WaitForVBlankRange(WaitFor, 0, false, true, true, _bTakenLock);
      return false;
    }
    else
    {
      _Waited = WaitForVBlankRange(WaitFor, 0, false, g_dsSettings.pRendererSettings->vSyncAccurate, true, _bTakenLock);
      return true;
    }
  }
  else
  {
    // Instead we wait for VBlack after the present, this seems to fix the stuttering problem. It's also possible to fix by removing the Sleep above, but that isn't an option.
    WaitForVBlankRange(WaitFor, 0, false, g_dsSettings.pRendererSettings->vSyncAccurate, true, _bTakenLock);

    return false;
  }
}

void CDX9AllocatorPresenter::UpdateAlphaBitmap()
{
  m_VMR9AlphaBitmapData.Free();

  if ((m_VMR9AlphaBitmap.dwFlags & VMRBITMAP_DISABLE) == 0)
  {
    HBITMAP      hBitmap = (HBITMAP)GetCurrentObject(m_VMR9AlphaBitmap.hdc, OBJ_BITMAP);
    if (!hBitmap)
      return;
    DIBSECTION    info = { 0 };
    if (!::GetObject(hBitmap, sizeof(DIBSECTION), &info))
      return;

    m_VMR9AlphaBitmapRect = Com::SmartRect(0, 0, info.dsBm.bmWidth, info.dsBm.bmHeight);
    m_VMR9AlphaBitmapWidthBytes = info.dsBm.bmWidthBytes;

    if (m_VMR9AlphaBitmapData.Allocate(info.dsBm.bmWidthBytes * info.dsBm.bmHeight))
    {
      memcpy((BYTE *)m_VMR9AlphaBitmapData, info.dsBm.bmBits, info.dsBm.bmWidthBytes * info.dsBm.bmHeight);
    }
  }
}

STDMETHODIMP_(bool) CDX9AllocatorPresenter::Paint(bool fAll)
{
  CAutoSetEvent autoEvent(&m_drawingIsDone);

  if (!g_renderManager.IsStarted() || m_bPendingResetDevice)
    return false;

  if (CDSPlayer::PlayerState == DSPLAYER_CLOSING || CDSPlayer::PlayerState == DSPLAYER_CLOSED)
    return false;

  int64_t StartPaint = CTimeUtils::GetPerfCounter();
  CAutoLock cRenderLock(&m_RenderLock);

  if (m_WindowRect.right <= m_WindowRect.left || m_WindowRect.bottom <= m_WindowRect.top
    || m_NativeVideoSize.cx <= 0 || m_NativeVideoSize.cy <= 0
    || !m_pVideoSurface)
  {
    if (m_OrderedPaint)
      --m_OrderedPaint;
    else
    {
      //      TRACE("UNORDERED PAINT!!!!!!\n");
    }


    return(false);
  }

  HRESULT hr;

  Com::SmartRect rSrcVid(Com::SmartPoint(0, 0), m_NativeVideoSize);
  Com::SmartRect rDstVid(m_VideoRect);

  Com::SmartRect rSrcPri(Com::SmartPoint(0, 0), m_WindowRect.Size());
  Com::SmartRect rDstPri(m_WindowRect);

  Com::SmartPtr<IDirect3DSurface9> pBackBuffer;
  m_pD3DDev->GetRenderTarget(0, &pBackBuffer);

  /*m_pD3DDev->SetRenderTarget(0, m_pVideoSurface[m_nNbDXSurface + 2]);
  hr = m_pD3DDev->ColorFill(m_pVideoSurface[m_nNbDXSurface + 2], NULL, 0);*/


  if (g_renderManager.IsConfigured() && !rDstVid.IsRectEmpty())
  {

    if (m_pVideoTexture[m_nCurSurface])
    {
      Com::SmartPtr<IDirect3DTexture9> pVideoTexture = m_pVideoTexture[m_nCurSurface];

      if (m_pVideoTexture[m_nNbDXSurface] && m_pVideoTexture[m_nNbDXSurface + 1])
      {

        // Brightness, contrast and saturation
        size_t src = m_nCurSurface, dst = m_nNbDXSurface;

        float contrast = 0.f, brightness = 0.f;
        // Map brightness and contrast settings

        // Range from 0 and 2
        contrast = (float)CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast / 50.f;

        // Range from -1 and 1
        brightness = (float)(CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness - 50.f) / 50.f;

        if (m_bscShader.IsValid()
          && ((abs(CMediaSettings::Get().GetCurrentVideoSettings().m_Contrast - 50) >= 1)
          || (abs(CMediaSettings::Get().GetCurrentVideoSettings().m_Brightness - 50) >= 1)))
        {
          Com::SmartPtr<IDirect3DSurface9> pRT;
          hr = m_pD3DDev->GetRenderTarget(0, &pRT);

          float fConstData[][4] =
          {
            { brightness, contrast, 0, 0 }
          };
          hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

          if (!m_bscShader.m_pPixelShader)
            m_bscShader.Compile(m_pPSC.get());

          hr = m_pD3DDev->SetRenderTarget(0, m_pVideoSurface[dst]);
          hr = m_pD3DDev->SetPixelShader(m_bscShader.m_pPixelShader);

          TextureCopy(m_pVideoTexture[src]);

          pVideoTexture = m_pVideoTexture[dst];
          src = dst;

          if (++dst >= m_nNbDXSurface + 2)
            dst = m_nNbDXSurface;

          hr = m_pD3DDev->SetRenderTarget(0, pRT);
          hr = m_pD3DDev->SetPixelShader(NULL);
        }

        if (!g_dsSettings.pixelShaderList->HasEnabledShaders())
        {
          static __int64 counter = 0;
          static long start = clock();

          long stop = clock();
          long diff = stop - start;

          if (diff >= 10 * 60 * CLOCKS_PER_SEC)
            start = stop; // reset after 10 min (ps float has its limits in both range and accuracy)

          D3DSURFACE_DESC desc;
          m_pVideoTexture[src]->GetLevelDesc(0, &desc);

          float fConstData[][4] =
          {
            { (float)desc.Width, (float)desc.Height, (float)(counter++), (float)diff / CLOCKS_PER_SEC },
            { 1.0f / desc.Width, 1.0f / desc.Height, 0, 0 },
          };
          hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

          Com::SmartPtr<IDirect3DSurface9> pRT;
          hr = m_pD3DDev->GetRenderTarget(0, &pRT);

          {
            CSingleLock lock(g_dsSettings.pixelShaderList->m_accessLock);
            g_dsSettings.pixelShaderList->UpdateActivatedList();
            PixelShaderVector& psVec = g_dsSettings.pixelShaderList->GetActivatedPixelShaders();

            for (PixelShaderVector::iterator it = psVec.begin();
              it != psVec.end(); it++)
            {
              pVideoTexture = m_pVideoTexture[dst];

              hr = m_pD3DDev->SetRenderTarget(0, m_pVideoSurface[dst]);
              CExternalPixelShader *Shader = *it;

              if (!Shader->m_pPixelShader)
                Shader->Compile(m_pPSC.get());

              hr = m_pD3DDev->SetPixelShader(Shader->m_pPixelShader);

              TextureCopy(m_pVideoTexture[src]);

              src = dst;
              if (++dst >= m_nNbDXSurface + 2)
                dst = m_nNbDXSurface;
            }
          }

          hr = m_pD3DDev->SetRenderTarget(0, pRT);
          hr = m_pD3DDev->SetPixelShader(NULL);
        }
      }

      Vector dst[4];
      Transform(rDstVid, dst);

      EDSSCALINGMETHOD iDX9Resizer = CMediaSettings::Get().GetCurrentVideoSettings().GetDSPlayerScalingMethod();

      float A = 0;

      switch (iDX9Resizer)
      {
      case DS_SCALINGMETHOD_BILINEAR_2_60: A = -0.60f; break;
      case DS_SCALINGMETHOD_BILINEAR_2_75: A = -0.751f; break;  // FIXME : 0.75 crash recent D3D, or eat CPU 
      case DS_SCALINGMETHOD_BILINEAR_2_100: A = -1.00f; break;
      }
      bool bScreenSpacePixelShaders = false;// = !m_pPixelShadersScreenSpace.IsEmpty();

      hr = InitResizers(A, bScreenSpacePixelShaders);

#if 0
      if (!m_pScreenSizeTemporaryTexture[0] || !m_pScreenSizeTemporaryTexture[1])
        bScreenSpacePixelShaders = false;

      if (bScreenSpacePixelShaders)
      {
        Com::SmartPtr<IDirect3DSurface9> pRT;
        hr = m_pScreenSizeTemporaryTexture[1]->GetSurfaceLevel(0, &pRT);
        if (hr != S_OK)
          bScreenSpacePixelShaders = false;
        if (bScreenSpacePixelShaders)
        {
          hr = m_pD3DDev->SetRenderTarget(0, pRT);
          if (hr != S_OK)
            bScreenSpacePixelShaders = false;
          hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        }
      }
#endif

      if (rSrcVid.Size() != rDstVid.Size())
      {
        if (iDX9Resizer == 0 || iDX9Resizer == 1)
        {
          D3DTEXTUREFILTERTYPE Filter = iDX9Resizer == 0 ? D3DTEXF_POINT : D3DTEXF_LINEAR;
          hr = TextureResize(pVideoTexture, dst, Filter, rSrcVid);
        }
        else if (iDX9Resizer == 2)
        {
          hr = TextureResizeBilinear(pVideoTexture, dst, rSrcVid);
        }
        else if (iDX9Resizer >= 3)
        {
          hr = TextureResizeBicubic2pass(pVideoTexture, dst, rSrcVid);
        }
      }
      else hr = TextureResize(pVideoTexture, dst, D3DTEXF_POINT, rSrcVid);

#if 0
      if (bScreenSpacePixelShaders)
      {
        static __int64 counter = 555;
        static long start = clock() + 333;

        long stop = clock() + 333;
        long diff = stop - start;

        if (diff >= 10 * 60 * CLOCKS_PER_SEC) start = stop; // reset after 10 min (ps float has its limits in both range and accuracy)

        D3DSURFACE_DESC desc;
        m_pScreenSizeTemporaryTexture[0]->GetLevelDesc(0, &desc);
        float fConstData[][4] =
        {
          { (float)desc.Width, (float)desc.Height, (float)(counter++), (float)diff / CLOCKS_PER_SEC },
          { 1.0f / desc.Width, 1.0f / desc.Height, 0, 0 },
        };

        hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

        int src = 1, dst = 0;

        POSITION pos = m_pPixelShadersScreenSpace.GetHeadPosition();
        while (pos)
        {
          if (m_pPixelShadersScreenSpace.GetTailPosition() == pos)
          {
            m_pD3DDev->SetRenderTarget(0, pBackBuffer);
          }
          else
          {
            CComPtr<IDirect3DSurface9> pRT;
            hr = m_pScreenSizeTemporaryTexture[dst]->GetSurfaceLevel(0, &pRT);
            m_pD3DDev->SetRenderTarget(0, pRT);
          }

          CExternalPixelShader &Shader = m_pPixelShadersScreenSpace.GetNext(pos);
          if (!Shader.m_pPixelShader)
            Shader.Compile(m_pPSC);
          hr = m_pD3DDev->SetPixelShader(Shader.m_pPixelShader);
          TextureCopy(m_pScreenSizeTemporaryTexture[src]);

          swap(src, dst);
        }
        hr = m_pD3DDev->SetPixelShader(NULL);
      }
#endif
    }
  }
  else
  {
    if (pBackBuffer)
    {
      ClipToSurface(pBackBuffer, rSrcVid, rDstVid); // grrr
      // IMPORTANT: rSrcVid has to be aligned on mod2 for yuy2->rgb conversion with StretchRect!!!
      rSrcVid.left &= ~1;
      rSrcVid.right &= ~1;
      rSrcVid.top &= ~1;
      rSrcVid.bottom &= ~1;
      hr = m_pD3DDev->StretchRect(m_pVideoSurface[m_nCurSurface], rSrcVid, pBackBuffer, rDstVid, m_filter);

      // Support ffdshow queueing
      // m_pD3DDev->StretchRect may fail if ffdshow is using queue output samples.
      // Here we don't want to show the black buffer.
      if (FAILED(hr))
      {
        if (m_OrderedPaint)
          --m_OrderedPaint;
        return false;
      }
    }
  }

  // Subtitle drawing
  if (CStreamsManager::Get()->SubtitleManager)
  {

    Com::SmartPtr<IDirect3DTexture9> pTexture;
    Com::SmartRect pSrc, pDst;

    if (SUCCEEDED(CStreamsManager::Get()->SubtitleManager->GetTexture(pTexture, pSrc, pDst, rSrcPri)))
    {
      AlphaBlt(&pSrc, &pDst, pTexture);
    }
  }

  if (g_dsSettings.pRendererSettings->displayStats)
    DrawStats();

  /*m_pD3DDev->SetRenderTarget(0, pBackBuffer);
  TextureCopy(m_pVideoTexture[m_nNbDXSurface + 2]);*/

  BOOL bCompositionEnabled = m_bCompositionEnabled;

  bool bDoVSyncInPresent = (!bCompositionEnabled && !m_bAlternativeVSync) || !g_dsSettings.pRendererSettings->vSync;

  int64_t PresentWaitTime = 0;

  Com::SmartPtr<IDirect3DQuery9> pEventQuery;

  m_pD3DDev->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);
  if (pEventQuery)
    pEventQuery->Issue(D3DISSUE_END);

  if (g_dsSettings.pRendererSettings->flushGPUBeforeVSync && pEventQuery)
  {
    int64_t llPerf = CTimeUtils::GetPerfCounter();
    BOOL Data;
    //Sleep(5);
    int64_t FlushStartTime = CTimeUtils::GetPerfCounter();
    while (S_FALSE == pEventQuery->GetData(&Data, sizeof(Data), D3DGETDATA_FLUSH))
    {
      if (!g_dsSettings.pRendererSettings->flushGPUWait)
        break;
      Sleep(1);
      if (CTimeUtils::GetPerfCounter() - FlushStartTime > 500000)
        break; // timeout after 50 ms
    }
    if (g_dsSettings.pRendererSettings->flushGPUWait)
      m_WaitForGPUTime = CTimeUtils::GetPerfCounter() - llPerf;
    else
      m_WaitForGPUTime = 0;
  }
  else
    m_WaitForGPUTime = 0;
  if (fAll)
  {
    m_PaintTime = (CTimeUtils::GetPerfCounter() - StartPaint);
    m_PaintTimeMin = std::min(m_PaintTimeMin, m_PaintTime);
    m_PaintTimeMax = std::max(m_PaintTimeMax, m_PaintTime);
  }

  bool bWaited = false;
  m_bTakenLock = false;
  if (fAll)
  {
    // Only sync to refresh when redrawing all
    bool bTest = WaitForVBlank(bWaited, m_bTakenLock);
    ASSERT(bTest == bDoVSyncInPresent);
    if (!bDoVSyncInPresent)
    {
      int64_t Time = CTimeUtils::GetPerfCounter();
      OnVBlankFinished(fAll, Time);
      if (!m_bIsEVR || m_OrderedPaint)
        CalculateJitter(Time);
    }
  }

  if (m_bTakenLock)
    UnlockD3DDevice();

  m_bPaintWasCalled = true;
  m_beforePresentTime = CTimeUtils::GetPerfCounter();
  return(true);

  // We have some code that need to be runned after the D3D Present method was called.
  // That code is now on the OnAfterPresent method.
}

double CDX9AllocatorPresenter::GetFrameTime()
{
  if (m_DetectedLock)
    return m_DetectedFrameTime;

  return m_rtTimePerFrame / 10000000.0;
}

double CDX9AllocatorPresenter::GetFrameRate()
{
  if (m_DetectedLock)
    return m_DetectedFrameRate;

  return 10000000.0 / m_rtTimePerFrame;
}

void CDX9AllocatorPresenter::DrawText(const RECT &rc, const CStdString &strText, int _Priority)
{
  if (_Priority < 1)
    return;
  int Quality = 1;
  D3DXCOLOR Color1(1.0f, 0.2f, 0.2f, 1.0f);
  D3DXCOLOR Color0(0.0f, 0.0f, 0.0f, 1.0f);
  RECT Rect1 = rc;
  RECT Rect2 = rc;
  if (Quality == 1)
    OffsetRect(&Rect2, 2, 2);
  else
    OffsetRect(&Rect2, -1, -1);
  if (Quality > 0)
    m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
  OffsetRect(&Rect2, 1, 0);
  if (Quality > 3)
    m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
  OffsetRect(&Rect2, 1, 0);
  if (Quality > 2)
    m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
  OffsetRect(&Rect2, 0, 1);
  if (Quality > 3)
    m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
  OffsetRect(&Rect2, 0, 1);
  if (Quality > 1)
    m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
  OffsetRect(&Rect2, -1, 0);
  if (Quality > 3)
    m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
  OffsetRect(&Rect2, -1, 0);
  if (Quality > 2)
    m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
  OffsetRect(&Rect2, 0, -1);
  if (Quality > 3)
    m_pFont->DrawText(m_pSprite, strText, -1, &Rect2, DT_NOCLIP, Color0);
  m_pFont->DrawText(m_pSprite, strText, -1, &Rect1, DT_NOCLIP, Color1);
}


void CDX9AllocatorPresenter::DrawStats()
{
  int bDetailedStats = 2;
  switch (g_dsSettings.pRendererSettings->displayStats)
  {
  case 1: bDetailedStats = 2; break;
  case 2: bDetailedStats = 1; break;
  case 3: bDetailedStats = 0; break;
  }

  int64_t    llMaxJitter = m_MaxJitter;
  int64_t    llMinJitter = m_MinJitter;
  int64_t    llMaxSyncOffset = m_MaxSyncOffset;
  int64_t    llMinSyncOffset = m_MinSyncOffset;
  if (m_pFont && m_pSprite)
  {
    m_pSprite->Begin(D3DXSPRITE_ALPHABLEND);
    RECT      rc = { 700, 40, 0, 0 };
    rc.left = 40;
    CStdString    strText;
    int TextHeight = lrint(25.0*m_TextScale);
    //    strText.Format("Frame rate   : %7.03f   (%7.3f ms = %.03f, %s)   (%7.3f ms = %.03f%s)    Clock: %7.3f ms %+1.4f %%  %+1.9f  %+1.9f", m_fAvrFps, double(m_rtTimePerFrame) / 10000.0, 10000000.0 / (double)(m_rtTimePerFrame), m_bInterlaced ? L"I" : L"P", GetFrameTime() * 1000.0, GetFrameRate(), m_DetectedLock ? L" L" : L"", m_ClockDiff/10000.0, m_ModeratedTimeSpeed*100.0 - 100.0, m_ModeratedTimeSpeedDiff, m_ClockDiffCalc/10000.0);
    if (bDetailedStats > 1)
    {
      if (m_bIsEVR)
        strText.Format("Frame rate   : %7.03f   (%7.3f ms = %.03f, %s)   (%7.3f ms = %.03f%s, %2.03f StdDev)  Clock: %1.4f %%", m_fAvrFps, double(m_rtTimePerFrame) / 10000.0, 10000000.0 / (double)(m_rtTimePerFrame), m_bInterlaced ? L"I" : L"P", GetFrameTime() * 1000.0, GetFrameRate(), m_DetectedLock ? L" L" : L"", m_DetectedFrameTimeStdDev / 10000.0, m_ModeratedTimeSpeed*100.0);
      else
        strText.Format("Frame rate   : %7.03f   (%7.3f ms = %.03f, %s)", m_fAvrFps, double(m_rtTimePerFrame) / 10000.0, 10000000.0 / (double)(m_rtTimePerFrame), m_bInterlaced ? L"I" : L"P");
    }
    else
      strText.Format("Frame rate   : %7.03f   (%.03f%s)", m_fAvrFps, GetFrameRate(), m_DetectedLock ? L" L" : L"");
    DrawText(rc, strText, 1);
    OffsetRect(&rc, 0, TextHeight);

    if (bDetailedStats > 1)
    {
      strText.Format("Settings     : ");

      if (m_bIsEVR)
        strText += "EVR ";
      else
        strText += "VMR9 ";

      if (g_dsSettings.pRendererSettings->d3dFullscreen)
        strText += "FS ";
      if (g_dsSettings.pRendererSettings->fullscreenGUISupport)
        strText += "FSGui ";

      if (g_dsSettings.pRendererSettings->disableDesktopComposition)
        strText += "DisDC ";

      if (g_dsSettings.pRendererSettings->flushGPUBeforeVSync)
        strText += "GPUFlushBV ";
      if (g_dsSettings.pRendererSettings->flushGPUAfterPresent)
        strText += "GPUFlushAP ";

      if (g_dsSettings.pRendererSettings->flushGPUWait)
        strText += "GPUFlushWt ";

      if (g_dsSettings.pRendererSettings->vSync)
        strText += "VS ";
      if (g_dsSettings.pRendererSettings->alterativeVSync)
        strText += "AltVS ";
      if (g_dsSettings.pRendererSettings->vSyncAccurate)
        strText += "AccVS ";
      if (g_dsSettings.pRendererSettings->vSyncOffset)
        strText.AppendFormat("VSOfst(%d)", g_dsSettings.pRendererSettings->vSyncOffset);

      if (m_bIsEVR)
      {
        CEVRRendererSettings *pSettings = (CEVRRendererSettings *)g_dsSettings.pRendererSettings;
        if (pSettings->highColorResolution)
          strText += "10bit ";
        if (pSettings->enableFrameTimeCorrection)
          strText += "FTC ";
        if (pSettings->outputRange == 0)
          strText += "0-255 ";
        else if (pSettings->outputRange == 1)
          strText += "16-235 ";
      }


      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);

    }

    if (bDetailedStats > 1)
    {
      strText.Format("Formats      : Surface %s    Backbuffer %s    Display %s     Device D3DDev", GetD3DFormatStr(m_SurfaceType), GetD3DFormatStr(D3DFMT_X8R8G8B8), GetD3DFormatStr(m_DisplayType));
      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);

      if (m_bIsEVR)
      {
        strText.Format("Refresh rate : %.05f Hz    SL: %4d     (%3d Hz)      Last Duration: %10.6f      Corrected Frame Time: %s", m_DetectedRefreshRate, int(m_DetectedScanlinesPerFrame + 0.5), m_RefreshRate, double(m_LastFrameDuration) / 10000.0, m_bCorrectedFrameTime ? "Yes" : "No");
        DrawText(rc, strText, 1);
        OffsetRect(&rc, 0, TextHeight);
      }
    }

    if (m_bSyncStatsAvailable)
    {
      if (bDetailedStats > 1)
        strText.Format("Sync offset  : min = %+8.3f ms, max = %+8.3f ms, StdDev = %7.3f ms, Avr = %7.3f ms, Mode = %d", (double(llMinSyncOffset) / 10000.0), (double(llMaxSyncOffset) / 10000.0), m_fSyncOffsetStdDev / 10000.0, m_fSyncOffsetAvr / 10000.0, m_VSyncMode);
      else
        strText.Format("Sync offset  : Mode = %d", m_VSyncMode);
      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);
    }

    if (bDetailedStats > 1)
    {
      strText.Format("Jitter       : min = %+8.3f ms, max = %+8.3f ms, StdDev = %7.3f ms", (double(llMinJitter) / 10000.0), (double(llMaxJitter) / 10000.0), m_fJitterStdDev / 10000.0);
      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);
    }

    /*if (m_pAllocator && bDetailedStats > 1)
    {
    CDX9SubPicAllocator *pAlloc = (CDX9SubPicAllocator *)m_pAllocator.m_ptr;

    //Com::SmartPtr<ISubPicAllocator> pAlloc;
    //pAlloc = m_pAllocator;
    int nFree = 0;
    int nAlloc = 0;
    int nSubPic = 0;
    REFERENCE_TIME QueueNow = 0;
    REFERENCE_TIME QueueStart = 0;
    REFERENCE_TIME QueueEnd = 0;
    if (m_pSubPicQueue)
    {
    m_pSubPicQueue->GetStats(nSubPic, QueueNow, QueueStart, QueueEnd);
    if (QueueStart)
    QueueStart -= QueueNow;
    if (QueueEnd)
    QueueEnd -= QueueNow;
    }
    pAlloc->GetStats(nFree, nAlloc);
    strText.Format("Subtitles    : Free %d     Allocated %d     Buffered %d     QueueStart %7.3f     QueueEnd %7.3f", nFree, nAlloc, nSubPic, (double(QueueStart)/10000000.0), (double(QueueEnd)/10000000.0));
    DrawText(rc, strText, 1);
    OffsetRect (&rc, 0, TextHeight);
    }*/ // No available right now

    if (bDetailedStats > 1)
    {
      if (m_VBlankEndPresent == -100000)
        strText.Format("VBlank Wait  : Start %4d   End %4d   Wait %7.3f ms   Lock %7.3f ms   Offset %4d   max %4d", m_VBlankStartWait, m_VBlankEndWait, (double(m_VBlankWaitTime) / 10000.0), (double(m_VBlankLockTime) / 10000.0), m_VBlankMin, m_VBlankMax - m_VBlankMin);
      else
        strText.Format("VBlank Wait  : Start %4d   End %4d   Wait %7.3f ms   Lock %7.3f ms   Offset %4d   max %4d   EndPresent %4d", m_VBlankStartWait, m_VBlankEndWait, (double(m_VBlankWaitTime) / 10000.0), (double(m_VBlankLockTime) / 10000.0), m_VBlankMin, m_VBlankMax - m_VBlankMin, m_VBlankEndPresent);
    }
    else
    {
      if (m_VBlankEndPresent == -100000)
        strText.Format("VBlank Wait  : Start %4d   End %4d", m_VBlankStartWait, m_VBlankEndWait);
      else
        strText.Format("VBlank Wait  : Start %4d   End %4d   EP %4d", m_VBlankStartWait, m_VBlankEndWait, m_VBlankEndPresent);
    }
    DrawText(rc, strText, 1);
    OffsetRect(&rc, 0, TextHeight);

    BOOL bCompositionEnabled = m_bCompositionEnabled;

    bool bDoVSyncInPresent = (!bCompositionEnabled && !m_bAlternativeVSync) || !g_dsSettings.pRendererSettings->vSync;

    if (bDetailedStats > 1 && bDoVSyncInPresent)
    {
      strText.Format("Present Wait : Wait %7.3f ms   min %7.3f ms   max %7.3f ms", (double(m_PresentWaitTime) / 10000.0), (double(m_PresentWaitTimeMin) / 10000.0), (double(m_PresentWaitTimeMax) / 10000.0));
      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);
    }

    if (bDetailedStats > 1)
    {
      if (m_WaitForGPUTime)
        strText.Format("Paint Time   : Draw %7.3f ms   min %7.3f ms   max %7.3f ms   GPU %7.3f ms", (double(m_PaintTime - m_WaitForGPUTime) / 10000.0), (double(m_PaintTimeMin) / 10000.0), (double(m_PaintTimeMax) / 10000.0), (double(m_WaitForGPUTime) / 10000.0));
      else
        strText.Format("Paint Time   : Draw %7.3f ms   min %7.3f ms   max %7.3f ms", (double(m_PaintTime - m_WaitForGPUTime) / 10000.0), (double(m_PaintTimeMin) / 10000.0), (double(m_PaintTimeMax) / 10000.0));
    }
    else
    {
      if (m_WaitForGPUTime)
        strText.Format("Paint Time   : Draw %7.3f ms   GPU %7.3f ms", (double(m_PaintTime - m_WaitForGPUTime) / 10000.0), (double(m_WaitForGPUTime) / 10000.0));
      else
        strText.Format("Paint Time   : Draw %7.3f ms", (double(m_PaintTime - m_WaitForGPUTime) / 10000.0));
    }
    DrawText(rc, strText, 2);
    OffsetRect(&rc, 0, TextHeight);

    if (bDetailedStats > 1)
    {
      strText.Format("Raster Status: Wait %7.3f ms   min %7.3f ms   max %7.3f ms", (double(m_RasterStatusWaitTime) / 10000.0), (double(m_RasterStatusWaitTimeMin) / 10000.0), (double(m_RasterStatusWaitTimeMax) / 10000.0));
      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);
    }

    if (bDetailedStats > 1)
    {
      if (m_bIsEVR)
        strText.Format("Buffering    : Buffered %3d    Free %3d    Current Surface %3d", m_nUsedBuffer, m_nNbDXSurface - m_nUsedBuffer, m_nCurSurface, m_nVMR9Surfaces, m_iVMR9Surface);
      else
        strText.Format("Buffering    : VMR9Surfaces %3d   VMR9Surface %3d", m_nVMR9Surfaces, m_iVMR9Surface);
    }
    else
      strText.Format("Buffered     : %3d", m_nUsedBuffer);
    DrawText(rc, strText, 1);
    OffsetRect(&rc, 0, TextHeight);

    if (bDetailedStats > 1)
    {
      strText.Format("Video size   : %d x %d  (AR = %d x %d)", m_NativeVideoSize.cx, m_NativeVideoSize.cy, m_AspectRatio.cx, m_AspectRatio.cy);
      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);
      if (m_pVideoTexture[0] || m_pVideoSurface[0])
      {
        D3DSURFACE_DESC desc;
        if (m_pVideoTexture[0])
          m_pVideoTexture[0]->GetLevelDesc(0, &desc);
        else if (m_pVideoSurface[0])
          m_pVideoSurface[0]->GetDesc(&desc);

        if (desc.Width != m_NativeVideoSize.cx || desc.Height != m_NativeVideoSize.cy)
        {
          strText.Format("Texture size : %d x %d", desc.Width, desc.Height);
          DrawText(rc, strText, 1);
          OffsetRect(&rc, 0, TextHeight);
        }
      }


      strText.Format("%-13s: %s", GetDXVAVersion(), GetDXVADecoderDescription());
      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);

      if (m_D3D9Device != _T(""))
      {
        strText = "Render device: " + m_D3D9Device;
        DrawText(rc, strText, 1);
        OffsetRect(&rc, 0, TextHeight);
      }

      strText.Format("DirectX SDK  : %d", g_dsSettings.GetDXSdkRelease());
      DrawText(rc, strText, 1);
      OffsetRect(&rc, 0, TextHeight);

      for (int i = 0; i < 6; i++)
      {
        if (!m_strStatsMsg[i].empty())
        {
          DrawText(rc, m_strStatsMsg[i], 1);
          OffsetRect(&rc, 0, TextHeight);
        }
      }
    }
    m_pSprite->End();
  }

  if (m_pLine && bDetailedStats)
  {
    D3DXVECTOR2    Points[NB_JITTER];
    int        nIndex;

    int StartX = 0;
    int StartY = 0;
    int ScaleX = 1;
    int ScaleY = 1;
    int DrawWidth = 625 * ScaleX + 50;
    int DrawHeight = 500 * ScaleY;
    int Alpha = 80;
    StartX = m_WindowRect.Width() - (DrawWidth + 20);
    StartY = m_WindowRect.Height() - (DrawHeight + 20);

    DrawRect(RGB(0, 0, 0), Alpha, Com::SmartRect(StartX, StartY, StartX + DrawWidth, StartY + DrawHeight));
    // === Jitter Graduation
    //    m_pLine->SetWidth(2.2);          // Width 
    //    m_pLine->SetAntialias(1);
    m_pLine->SetWidth(2.5);          // Width 
    m_pLine->SetAntialias(1);
    //    m_pLine->SetGLLines(1);
    m_pLine->Begin();

    for (int i = 10; i < 500 * ScaleY; i += 20 * ScaleY)
    {
      Points[0].x = (FLOAT)StartX;
      Points[0].y = (FLOAT)(StartY + i);
      Points[1].x = (FLOAT)(StartX + ((i - 10) % 80 ? 50 : 625 * ScaleX));
      Points[1].y = (FLOAT)(StartY + i);
      if (i == 250) Points[1].x += 50;
      m_pLine->Draw(Points, 2, D3DCOLOR_XRGB(100, 100, 255));
    }

    // === Jitter curve
    if (m_rtTimePerFrame)
    {
      for (int i = 0; i < NB_JITTER; i++)
      {
        nIndex = (m_nNextJitter + 1 + i) % NB_JITTER;
        if (nIndex < 0)
          nIndex += NB_JITTER;
        double Jitter = m_pllJitter[nIndex] - m_fJitterMean;
        Points[i].x = (FLOAT)(StartX + (i * 5 * ScaleX + 5));
        Points[i].y = (FLOAT)(StartY + ((Jitter*ScaleY) / 5000.0 + 250.0* ScaleY));
      }
      m_pLine->Draw(Points, NB_JITTER, D3DCOLOR_XRGB(255, 100, 100));

      if (m_bSyncStatsAvailable)
      {
        for (int i = 0; i < NB_JITTER; i++)
        {
          nIndex = (m_nNextSyncOffset + 1 + i) % NB_JITTER;
          if (nIndex < 0)
            nIndex += NB_JITTER;
          Points[i].x = (FLOAT)(StartX + (i * 5 * ScaleX + 5));
          Points[i].y = (FLOAT)(StartY + ((m_pllSyncOffset[nIndex] * ScaleY) / 5000 + 250 * ScaleY));
        }
        m_pLine->Draw(Points, NB_JITTER, D3DCOLOR_XRGB(100, 200, 100));
      }
    }
    m_pLine->End();
  }

  // === Text

}

STDMETHODIMP CDX9AllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
  CheckPointer(size, E_POINTER);

  HRESULT hr;

  D3DSURFACE_DESC desc;
  memset(&desc, 0, sizeof(desc));
  m_pVideoSurface[m_nCurSurface]->GetDesc(&desc);

  DWORD required = sizeof(BITMAPINFOHEADER) + (desc.Width * desc.Height * 32 >> 3);
  if (!lpDib) { *size = required; return S_OK; }
  if (*size < required) return E_OUTOFMEMORY;
  *size = required;

  Com::SmartPtr<IDirect3DSurface9> pSurface = m_pVideoSurface[m_nCurSurface];
  D3DLOCKED_RECT r;
  if (FAILED(hr = pSurface->LockRect(&r, NULL, D3DLOCK_READONLY)))
  {
    pSurface = NULL;
    if (FAILED(hr = m_pD3DDev->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pSurface, NULL))
      || FAILED(hr = m_pD3DDev->GetRenderTargetData(m_pVideoSurface[m_nCurSurface], pSurface))
      || FAILED(hr = pSurface->LockRect(&r, NULL, D3DLOCK_READONLY)))
      return hr;
  }

  BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)lpDib;
  memset(bih, 0, sizeof(BITMAPINFOHEADER));
  bih->biSize = sizeof(BITMAPINFOHEADER);
  bih->biWidth = desc.Width;
  bih->biHeight = desc.Height;
  bih->biBitCount = 32;
  bih->biPlanes = 1;
  bih->biSizeImage = bih->biWidth * bih->biHeight * bih->biBitCount >> 3;

  BitBltFromRGBToRGB(
    bih->biWidth, bih->biHeight,
    (BYTE*)(bih + 1), bih->biWidth*bih->biBitCount >> 3, bih->biBitCount,
    (BYTE*)r.pBits + r.Pitch*(desc.Height - 1), -(int)r.Pitch, 32);

  pSurface->UnlockRect();

  return S_OK;
}

void CDX9AllocatorPresenter::BeforeDeviceReset()
{
  m_bPendingResetDevice = true;

  StopWorkerThreads();
  if (CStreamsManager::Get()->SubtitleManager)
    CStreamsManager::Get()->SubtitleManager->StopThread();

  DeleteSurfaces();

  m_pD3DDev = NULL;
  m_pD3D = NULL;
}

void CDX9AllocatorPresenter::AfterDeviceReset()
{
  //todo dx11
  /*
  m_pD3DDev = g_Windowing.Get3DDevice();
  m_pD3D = g_Windowing.Get3DObject();
  */
  AllocSurfaces();

  if (CStreamsManager::Get()->SubtitleManager)
    CStreamsManager::Get()->SubtitleManager->StartThread();

  m_bPendingResetDevice = false;
}

void CDX9AllocatorPresenter::OnLostDevice()
{
  // XBMC gets a Reset event. We need to cleanup our ressources
  CLog::Log(LOGDEBUG, "%s Device lost, cleaning ressources", __FUNCTION__);
  BeforeDeviceReset();
}

void CDX9AllocatorPresenter::OnDestroyDevice()
{
  // The device is going to be destroyed. Cleanup!
  CLog::Log(LOGDEBUG, "%s Device destroyed, cleaning ressources", __FUNCTION__);
  BeforeDeviceReset();
}

void CDX9AllocatorPresenter::OnCreateDevice()
{
  // Do nothing!
}

void CDX9AllocatorPresenter::OnResetDevice()
{
  CLog::Log(LOGDEBUG, "%s Device is back, recreating ressources", __FUNCTION__);
  AfterDeviceReset();
}

void CDX9AllocatorPresenter::OnPaint(CRect destRect)
{

  m_VideoRect.bottom = (long)destRect.y2;
  m_VideoRect.top = (long)destRect.y1;
  m_VideoRect.left = (long)destRect.x1;
  m_VideoRect.right = (long)destRect.x2;

  //Need to be true for vsync
  Paint(bPaintAll);
  bPaintAll = true;
}

void CDX9AllocatorPresenter::OnAfterPresent()
{
  if (!m_bPaintWasCalled)
    return;

  m_bPaintWasCalled = false;

  BOOL bCompositionEnabled = m_bCompositionEnabled;
  bool bDoVSyncInPresent = (!bCompositionEnabled && !m_bAlternativeVSync) || !g_dsSettings.pRendererSettings->vSync;
  bool fAll = true; // It seems that Paint is always called with a "true" param

  {
    Com::SmartPtr<IDirect3DQuery9> pEventQuery;
    m_pD3DDev->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);

    // Issue an End event
    if (pEventQuery)
      pEventQuery->Issue(D3DISSUE_END);

    BOOL Data = 0;

    if (g_dsSettings.pRendererSettings->flushGPUAfterPresent && pEventQuery)
    {
      int64_t FlushStartTime = CTimeUtils::GetPerfCounter();
      while (S_FALSE == pEventQuery->GetData(&Data, sizeof(Data), D3DGETDATA_FLUSH))
      {
        if (!g_dsSettings.pRendererSettings->flushGPUWait)
          break;
        if (CTimeUtils::GetPerfCounter() - FlushStartTime > 500000)
          break; // timeout after 50 ms
      }
    }

    int ScanLine;
    int bInVBlank;
    GetVBlank(ScanLine, bInVBlank, false);

    if (fAll && (!m_bIsEVR || m_OrderedPaint))
    {
      m_VBlankEndPresent = ScanLine;
    }

    while (ScanLine == 0 || bInVBlank)
    {
      GetVBlank(ScanLine, bInVBlank, false);

    }
    m_VBlankStartMeasureTime = CTimeUtils::GetPerfCounter();
    m_VBlankStartMeasure = ScanLine;

    int64_t llPerf = CTimeUtils::GetPerfCounter();
    if (fAll && bDoVSyncInPresent)
    {
      // PresentWaitTime is always 0
      m_PresentWaitTime = (CTimeUtils::GetPerfCounter() - m_beforePresentTime);
      m_PresentWaitTimeMin = std::min(m_PresentWaitTimeMin, m_PresentWaitTime);
      m_PresentWaitTimeMax = std::max(m_PresentWaitTimeMax, m_PresentWaitTime);
    }
    else
    {
      m_PresentWaitTime = 0;
      m_PresentWaitTimeMin = std::min(m_PresentWaitTimeMin, m_PresentWaitTime);
      m_PresentWaitTimeMax = std::max(m_PresentWaitTimeMax, m_PresentWaitTime);
    }
  }

  if (bDoVSyncInPresent)
  {
    int64_t Time = CTimeUtils::GetPerfCounter();
    if (!m_bIsEVR || m_OrderedPaint)
      CalculateJitter(Time);
    OnVBlankFinished(fAll, Time);
  }

  if (m_OrderedPaint)
    --m_OrderedPaint;
}

#endif
