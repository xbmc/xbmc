/*
 * (C) 2006-2014 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "madVRAllocatorPresenter.h"
#include "windowing/WindowingFactory.h"
#include <moreuuids.h>
#include "RendererSettings.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/CharsetConverter.h"
#include "cores/DSPlayer/Filters/MadvrSettings.h"
#include "settings/MediaSettings.h"
#include "settings/DisplaySettings.h"
#include "PixelShaderList.h"
#include "DSPlayer.h"

#define ShaderStage_PreScale 0
#define ShaderStage_PostScale 1

extern bool g_bExternalSubtitleTime;
ThreadIdentifier CmadVRAllocatorPresenter::m_threadID = 0;

//
// CmadVRAllocatorPresenter
//

CmadVRAllocatorPresenter::CmadVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CStdString& _Error)
  : ISubPicAllocatorPresenterImpl(hWnd, hr)
  , m_ScreenSize(0, 0)
  , m_bIsFullscreen(false)
{

  //Init Variable
  g_renderManager.PreInit(RENDERER_DSHOW);
  m_exclusiveCallback = ExclusiveCallback;
  m_firstBoot = true;
  m_isEnteringExclusive = false;
  m_updateDisplayLatencyForMadvr = false;
  m_threadID = 0;
  m_pMadvrShared = DNew CMadvrSharedRender();
  
  if (FAILED(hr)) {
    _Error += L"ISubPicAllocatorPresenterImpl failed\n";
    return;
  }

  hr = S_OK;
}

CmadVRAllocatorPresenter::~CmadVRAllocatorPresenter()
{
  if (m_pSRCB) {
    // nasty, but we have to let it know about our death somehow
    ((CSubRenderCallback*)(ISubRenderCallback2*)m_pSRCB)->SetDXRAP(nullptr);
  }

  // Unregister madVR Exclusive Callback
  if (Com::SmartQIPtr<IMadVRExclusiveModeCallback> pEXL = m_pDXR)
    pEXL->Unregister(m_exclusiveCallback, this);

  // Let's madVR restore original display mode (when adjust refresh it's handled by madVR)
  if (Com::SmartQIPtr<IMadVRCommand> pMadVrCmd = m_pDXR)
    pMadVrCmd->SendCommand("restoreDisplayModeNow");

  g_renderManager.UnInit();
  
  // the order is important here
  CMadvrCallback::Destroy();
  SAFE_DELETE(m_pMadvrShared);
  m_pSubPicQueue = nullptr;
  m_pAllocator = nullptr;
  m_pDXR = nullptr;
  m_pORCB = nullptr;
  m_pSRCB = nullptr;

  CLog::Log(LOGDEBUG, "%s Resources released", __FUNCTION__);
}

STDMETHODIMP CmadVRAllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  if (riid != IID_IUnknown && m_pDXR) {
    if (SUCCEEDED(m_pDXR->QueryInterface(riid, ppv))) {
      return S_OK;
    }
  }

  return __super::NonDelegatingQueryInterface(riid, ppv);
}

void CmadVRAllocatorPresenter::SetResolution()
{
  ULONGLONG frameRate;
  float fps;

  CMadvrCallback::Get()->SetInitMadvr(true);

  // Set the context in FullScreenVideo
  g_graphicsContext.SetFullScreenVideo(true);

  if (Com::SmartQIPtr<IMadVRInfo> pInfo = m_pDXR)
  {
    pInfo->GetUlonglong("frameRate", &frameRate);
    fps = 10000000.0 / frameRate;
  }

  if (CSettings::Get().GetInt("videoplayer.adjustrefreshrate") != ADJUST_REFRESHRATE_OFF
    && (CSettings::Get().GetInt("videoplayer.changerefreshwith") == ADJUST_REFRESHRATE_WITH_BOTH || CSettings::Get().GetInt("videoplayer.changerefreshwith") == ADJUST_REFRESHRATE_WITH_DSPLAYER)
    && g_graphicsContext.IsFullScreenRoot())
  {
    RESOLUTION bestRes = g_renderManager.m_pRenderer->ChooseBestMadvrResolution(fps);
    CDSPlayer::SetDsWndVisible(true);
    g_graphicsContext.SetVideoResolution(bestRes);
  }
  else
    m_updateDisplayLatencyForMadvr = true;

  CMadvrCallback::Get()->SetInitMadvr(false);
}

void CmadVRAllocatorPresenter::ExclusiveCallback(LPVOID context, int event)
{
  CmadVRAllocatorPresenter *pThis = (CmadVRAllocatorPresenter*)context;

  std::vector<std::string> strEvent = { "IsAboutToBeEntered", "WasJustEntered", "IsAboutToBeLeft", "WasJustLeft" };

  if (event == ExclusiveModeIsAboutToBeEntered || event == ExclusiveModeIsAboutToBeLeft)
    pThis->m_isEnteringExclusive = true;

  if (event == ExclusiveModeWasJustEntered || event == ExclusiveModeWasJustLeft)
    pThis->m_isEnteringExclusive = false;

  CLog::Log(LOGDEBUG, "%s madVR %s in Fullscreen Exclusive-Mode", __FUNCTION__, strEvent[event - 1].c_str());
}

void CmadVRAllocatorPresenter::ConfigureMadvr()
{

  if (Com::SmartQIPtr<IMadVRCommand> pMadVrCmd = m_pDXR)
    pMadVrCmd->SendCommandBool("disableSeekbar", true);

  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
    pMadvrSettings->SettingsSetBoolean(L"delayPlaybackStart2", CSettings::Get().GetBool("dsplayer.delaymadvrplayback"));

  if (Com::SmartQIPtr<IMadVRExclusiveModeCallback> pEXL = m_pDXR)
    pEXL->Register(m_exclusiveCallback, this);

  if (CSettings::Get().GetBool("dsplayer.madvrexclusivemode"))
  {
    if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
    {
      pMadvrSettings->SettingsSetBoolean(L"exclusiveDelay", true);
      pMadvrSettings->SettingsSetBoolean(L"enableExclusive", true);
    }
  }
  else
  {
    if (Com::SmartQIPtr<IMadVRCommand> pMadVrCmd = m_pDXR)
      pMadVrCmd->SendCommandBool("disableExclusiveMode", true);
  }
}

bool CmadVRAllocatorPresenter::ParentWindowProc(HWND hWnd, UINT uMsg, WPARAM *wParam, LPARAM *lParam, LRESULT *ret)
{
  if (Com::SmartQIPtr<IMadVRSubclassReplacement> pMVRSR = m_pDXR)
    return pMVRSR->ParentWindowProc(hWnd, uMsg, wParam, lParam, ret);
  else
    return false;
}

bool CmadVRAllocatorPresenter::IsCurrentThreadId()
{
  return CThread::IsCurrentThread(m_threadID);
}

STDMETHODIMP CmadVRAllocatorPresenter::ClearBackground(LPCSTR name, REFERENCE_TIME frameStart, RECT *fullOutputRect, RECT *activeVideoRect)
{
  if (!g_graphicsContext.IsFullScreenVideo())
    return m_pMadvrShared->RenderMadvr(RENDER_LAYER_UNDER, fullOutputRect->right - fullOutputRect->left, fullOutputRect->bottom - fullOutputRect->top);
  else
    return CALLBACK_INFO_DISPLAY;
}

STDMETHODIMP CmadVRAllocatorPresenter::RenderOsd(LPCSTR name, REFERENCE_TIME frameStart, RECT *fullOutputRect, RECT *activeVideoRect)
{
  return m_pMadvrShared->RenderMadvr(RENDER_LAYER_OVER, fullOutputRect->right - fullOutputRect->left, fullOutputRect->bottom - fullOutputRect->top);
}

STDMETHODIMP CmadVRAllocatorPresenter::SetDeviceOsd(IDirect3DDevice9* pD3DDev)
{
  if (!pD3DDev)
  {
    // release all resources
    m_pSubPicQueue = nullptr;
    m_pAllocator = nullptr;
  }
  return S_OK;
}

HRESULT CmadVRAllocatorPresenter::SetDevice(IDirect3DDevice9* pD3DDev)
{
  CLog::Log(LOGDEBUG, "%s madVR's device it's ready", __FUNCTION__);

  if (!pD3DDev)
  {
    // release all resources
    m_pSubPicQueue = nullptr;
    m_pAllocator = nullptr;
    return S_OK;
  }

  if (m_firstBoot)
  { 
    m_pMadvrShared->CreateTextures((IDirect3DDevice9Ex*)g_Windowing.Get3DDevice(), (IDirect3DDevice9Ex*)pD3DDev, (int)m_ScreenSize.cx, (int)m_ScreenSize.cy);

    m_firstBoot = false;
    m_threadID = CThread::GetCurrentThreadId();
  }

  Com::SmartSize size;

  if (m_pAllocator) {
    m_pAllocator->ChangeDevice(pD3DDev);
  }
  else
  {
    m_pAllocator = DNew CDX9SubPicAllocator(pD3DDev, size, true);
    if (!m_pAllocator) {
      return E_FAIL;
    }
  }

  HRESULT hr = S_OK;

  if (!m_pSubPicQueue) {
    CAutoLock cAutoLock(this);
    m_pSubPicQueue = g_dsSettings.pRendererSettings->subtitlesSettings.bufferAhead > 0
      ? (ISubPicQueue*)DNew CSubPicQueue(g_dsSettings.pRendererSettings->subtitlesSettings.bufferAhead, g_dsSettings.pRendererSettings->subtitlesSettings.disableAnimations, m_pAllocator, &hr)
      : (ISubPicQueue*)DNew CSubPicQueueNoThread(m_pAllocator, &hr);
  }
  else {
    m_pSubPicQueue->Invalidate();
  }

  if (SUCCEEDED(hr) && (m_SubPicProvider)) {
    m_pSubPicQueue->SetSubPicProvider(m_SubPicProvider);
  }

  return hr;
}

HRESULT CmadVRAllocatorPresenter::Render( REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME atpf, int left, int top, int right, int bottom, int width, int height)
{
  Com::SmartRect wndRect(0, 0, width, height);
  Com::SmartRect videoRect(left, top, right, bottom);

  __super::SetPosition(wndRect, videoRect);
  if (!g_bExternalSubtitleTime) {
    SetTime(rtStart);
  }
  if (atpf > 0 && m_pSubPicQueue) {
    m_fps = 10000000.0 / atpf;
    m_pSubPicQueue->SetFPS(m_fps);
  }

  if (!g_renderManager.IsConfigured())
  {
    m_NativeVideoSize = GetVideoSize(false);
    m_AspectRatio = GetVideoSize(true);

    // Configure Render Manager
    g_renderManager.Configure(m_NativeVideoSize.cx, m_NativeVideoSize.cy, m_AspectRatio.cx, m_AspectRatio.cy, m_fps, CONF_FLAGS_FULLSCREEN , RENDER_FMT_NONE, 0, 0);
    CLog::Log(LOGDEBUG, "%s Render manager configured (FPS: %f) %i %i %i %i", __FUNCTION__, m_fps, m_NativeVideoSize.cx, m_NativeVideoSize.cy, m_AspectRatio.cx, m_AspectRatio.cy);

    // Begin Render Kodi 
    CDSPlayer::SetDsWndVisible(true);
    CMadvrCallback::Get()->SetRenderOnMadvr(true);

    // Update Display Latency for madVR (sets differents delay for each refresh as configured in advancedsettings)
    if (m_updateDisplayLatencyForMadvr)
    { 
      if (Com::SmartQIPtr<IMadVRInfo> pInfo = m_pDXR)
      { 
        double refreshRate;
        pInfo->GetDouble("refreshRate", &refreshRate);
        g_renderManager.UpdateDisplayLatencyForMadvr(refreshRate);
      }
    }
  }

  AlphaBltSubPic(Com::SmartSize(width, height));

  return S_OK;
}

// ISubPicAllocatorPresenter
STDMETHODIMP CmadVRAllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
  CheckPointer(ppRenderer, E_POINTER);

  if (m_pDXR) {
    return E_UNEXPECTED;
  }

  m_pDXR.CoCreateInstance(CLSID_madVR, GetOwner());
  if (!m_pDXR) {
    return E_FAIL;
  }

  Com::SmartQIPtr<ISubRender> pSR = m_pDXR;
  if (!pSR) {
    m_pDXR = nullptr;
    return E_FAIL;
  }

  m_pSRCB = DNew CSubRenderCallback(this);
  if (FAILED(pSR->SetCallback(m_pSRCB))) {
    m_pDXR = nullptr;
    return E_FAIL;
  }

  // IOsdRenderCallback
  Com::SmartQIPtr<IMadVROsdServices> pOR = m_pDXR;
  if (!pOR) {
    m_pDXR = nullptr;
    return E_FAIL;
  }

  m_pORCB = DNew COsdRenderCallback(this);
  if (FAILED(pOR->OsdSetRenderCallback("Kodi.Gui", m_pORCB))) {
    m_pDXR = nullptr;
    return E_FAIL;
  }

  // Configure initial Madvr Settings
  ConfigureMadvr();

  CMadvrCallback::Get()->SetCallback(this);

  (*ppRenderer = (IUnknown*)(INonDelegatingUnknown*)(this))->AddRef();

  MONITORINFO mi;
  mi.cbSize = sizeof(MONITORINFO);
  if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi)) {
    m_ScreenSize.SetSize(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
  }

  return S_OK;
}

void CmadVRAllocatorPresenter::SetMadvrPosition(CRect wndRect, CRect videoRect)
{

  Com::SmartRect wndR(wndRect.x1, wndRect.y1, wndRect.x2, wndRect.y2);
  Com::SmartRect videoR(videoRect.x1, videoRect.y1, videoRect.x2, videoRect.y2);
  SetPosition(wndR, videoR);
  //CLog::Log(0, "wndR x1: %g   y1: %g   x2: %g   y2: %g - videoR x1: %g   y1: %g   x2: %g   y2: %g", wndRect.x1, wndRect.y1, wndRect.x2, wndRect.y2, videoRect.x1, videoRect.y1, videoRect.x2, videoRect.y2);
}

STDMETHODIMP_(void) CmadVRAllocatorPresenter::SetPosition(RECT w, RECT v)
{
  if (Com::SmartQIPtr<IBasicVideo> pBV = m_pDXR) {
    pBV->SetDefaultSourcePosition();
    pBV->SetDestinationPosition(v.left, v.top, v.right - v.left, v.bottom - v.top);
  }

  if (Com::SmartQIPtr<IVideoWindow> pVW = m_pDXR) {
    if (!g_graphicsContext.IsFullScreenVideo())
    {
      w.left = 0;
      w.top = 0;
      w.right = g_graphicsContext.GetWidth();
      w.bottom = g_graphicsContext.GetHeight();
    }
    pVW->SetWindowPosition(w.left, w.top, w.right - w.left, w.bottom - w.top);
  }
}

STDMETHODIMP_(SIZE) CmadVRAllocatorPresenter::GetVideoSize(bool fCorrectAR)
{
  SIZE size = { 0, 0 };

  if (!fCorrectAR) {
    if (Com::SmartQIPtr<IBasicVideo> pBV = m_pDXR) {
      pBV->GetVideoSize(&size.cx, &size.cy);
    }
  }
  else {
    if (Com::SmartQIPtr<IBasicVideo2> pBV2 = m_pDXR) {
      pBV2->GetPreferredAspectRatio(&size.cx, &size.cy);
    }
  }

  return size;
}

STDMETHODIMP CmadVRAllocatorPresenter::GetDIB(BYTE* lpDib, DWORD* size)
{
  HRESULT hr = E_NOTIMPL;
  if (Com::SmartQIPtr<IBasicVideo> pBV = m_pDXR) {
    hr = pBV->GetCurrentImage((long*)size, (long*)lpDib);
  }
  return hr;
}

STDMETHODIMP_(bool) CmadVRAllocatorPresenter::Paint(bool fAll)
{
  return false;
}

void CmadVRAllocatorPresenter::SetMadvrPixelShader()
{
  g_dsSettings.pixelShaderList->UpdateActivatedList();
  m_shaderStage = 0;
  CStdString strStage;
  PixelShaderVector& psVec = g_dsSettings.pixelShaderList->GetActivatedPixelShaders();

  for (PixelShaderVector::iterator it = psVec.begin();
    it != psVec.end(); it++)
  {
    CExternalPixelShader *Shader = *it;
    Shader->Load();
    m_shaderStage = Shader->GetStage();
    m_shaderStage == 0 ? strStage = "Pre-Resize" : strStage = "Post-Resize";
    SetPixelShader(Shader->GetSourceData(), nullptr);
    Shader->DeleteSourceData();

    CLog::Log(LOGDEBUG, "%s Set PixelShader: %s applied: %s", __FUNCTION__, Shader->GetName().c_str(), strStage.c_str());
  }
};

STDMETHODIMP CmadVRAllocatorPresenter::SetPixelShader(LPCSTR pSrcData, LPCSTR pTarget)
{
  HRESULT hr = E_NOTIMPL;
  if (Com::SmartQIPtr<IMadVRExternalPixelShaders> pEPS = m_pDXR) {
    if (!pSrcData && !pTarget) {
      hr = pEPS->ClearPixelShaders(false);
    }
    else {
      hr = pEPS->AddPixelShader(pSrcData, pTarget, m_shaderStage, nullptr);
    }
  }
  return hr;
}

//IPaintCallbackMadvr

void CmadVRAllocatorPresenter::SettingSetScaling(CStdStringW path, int scaling)
{
  std::vector<std::wstring> vecMadvrScaling =
  {
    L"Nearest Neighbor",
    L"Bilinear",
    L"Dvxa",
    L"Mitchell-Netravali",
    L"Catmull-Rom",
    L"Bicubic50", L"Bicubic60", L"Bicubic75", L"Bicubic100",
    L"SoftCubic50", L"SoftCubic60", L"SoftCubic70", L"SoftCubic80", L"SoftCubic100",
    L"Lanczos3", L"Lanczos4", L"Lanczos8",
    L"Spline36", L"Spline64",
    L"Jinc3", L"Jinc4", L"Jinc8",
    L"Nnedi16", L"Nnedi32", L"Nnedi64", L"Nnedi128", L"Nnedi256"
  };

  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
    pMadvrSettings->SettingsSetString(path, vecMadvrScaling[scaling].c_str());
}

void CmadVRAllocatorPresenter::SettingSetDoubling(CStdStringW path, int iValue)
{
  CStdStringW strBool, strInt;
  strBool = path + "Enable";
  strInt = path + "Quality";

  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
  { 
    pMadvrSettings->SettingsSetBoolean(strBool, (iValue>-1));
    if (iValue>-1)
      pMadvrSettings->SettingsSetInteger(strInt, iValue);
  }
}

void CmadVRAllocatorPresenter::SettingSetDoublingCondition(CStdStringW path, int condition)
{
  std::vector<std::wstring> vecMadvrCondition = { L"2.0x", L"1.5x", L"1.2x", L"always" };
  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
    pMadvrSettings->SettingsSetString(path, vecMadvrCondition[condition].c_str());
}

void CmadVRAllocatorPresenter::SettingSetQuadrupleCondition(CStdStringW path, int condition)
{
  std::vector<std::wstring> vecMadvrCondition = { L"4.0x", L"3.0x", L"2.4x", L"always" };
  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
    pMadvrSettings->SettingsSetString(path, vecMadvrCondition[condition].c_str());
}

void CmadVRAllocatorPresenter::SettingSetDeintActive(CStdStringW path, int iValue)
{
  CStdStringW strAuto = "autoActivateDeinterlacing";
  CStdStringW strIfDoubt = "ifInDoubtDeinterlace";

  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
  {
    pMadvrSettings->SettingsSetBoolean(strAuto, (iValue > -1));
    pMadvrSettings->SettingsSetBoolean(strIfDoubt, (iValue == MADVR_DEINT_IFDOUBT_ACTIVE));
  }
}

void CmadVRAllocatorPresenter::SettingSetDeintForce(CStdStringW path, int iValue)
{
  std::vector<std::wstring> vecMadvrCondition = { L"auto", L"film", L"video" };
  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
    pMadvrSettings->SettingsSetString(path, vecMadvrCondition[iValue].c_str());
}

void CmadVRAllocatorPresenter::SettingSetSmoothmotion(CStdStringW path, int iValue)
{
  CStdStringW stEnabled = "smoothMotionEnabled";
  CStdStringW strMode = "smoothMotionMode";
  std::vector<std::wstring> vecMadvrMode = { L"avoidJudder", L"almostAlways", L"always" };

  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
  {
    pMadvrSettings->SettingsSetBoolean(stEnabled, (iValue > -1));
    if (iValue > -1)
      pMadvrSettings->SettingsSetString(strMode, vecMadvrMode[iValue].c_str());
  }
}

void CmadVRAllocatorPresenter::SettingSetDithering(CStdStringW path, int iValue)
{
  CStdStringW stDisable = "dontDither";
  CStdStringW strMode = "ditheringAlgo";
  std::vector<std::wstring> vecMadvrMode = { L"random", L"ordered", L"errorDifMedNoise", L"errorDifLowNoise" };

  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
  {
    pMadvrSettings->SettingsSetBoolean(stDisable, (iValue == -1));
    if (iValue > -1)
      pMadvrSettings->SettingsSetString(strMode, vecMadvrMode[iValue].c_str());
  }
}

void CmadVRAllocatorPresenter::SettingSetBool(CStdStringW path, BOOL bValue)
{
  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
    pMadvrSettings->SettingsSetBoolean(path, bValue);
}

void CmadVRAllocatorPresenter::SettingSetInt(CStdStringW path, int iValue)
{
  if (Com::SmartQIPtr<IMadVRSettings> pMadvrSettings = m_pDXR)
    pMadvrSettings->SettingsSetInteger(path, iValue);
}

CStdString CmadVRAllocatorPresenter::GetDXVADecoderDescription()
{
  CStdString strDXVA, strDecoding;
  bool bDecoding, bDeinterlace, bScaling;

  if (Com::SmartQIPtr<IMadVRInfo> pMadvrInfo = m_pDXR)
  {
    pMadvrInfo->GetBool("dxvaDecodingActive", &bDecoding);
    pMadvrInfo->GetBool("dxvaDeinterlacingActive", &bDeinterlace);
    pMadvrInfo->GetBool("dxvaScalingActive", &bScaling);
  }
  bDecoding ? strDecoding = "DXVA Decoding" : strDecoding = "Not using DXVA";

  strDXVA = strDecoding;
  if (bDeinterlace)
    strDXVA += ", DXVA Deinterlacing";
  if (bScaling)
    strDXVA += ", DXVA Scaling";

  return strDXVA;
};

void CmadVRAllocatorPresenter::RestoreMadvrSettings()
{

  if (!CSettings::Get().GetBool("dsplayer.managemadvrsettings"))
    return;

  CMadvrSettings &madvrSettings = CMediaSettings::Get().GetCurrentMadvrSettings();

  SettingSetScaling("chromaUp", madvrSettings.m_ChromaUpscaling);
  SettingSetBool("chromaAntiRinging", madvrSettings.m_ChromaAntiRing);
  SettingSetScaling("LumaUp", madvrSettings.m_ImageUpscaling);
  SettingSetBool("lumaUpAntiRinging", madvrSettings.m_ImageUpAntiRing);
  SettingSetBool("lumaUpLinear", madvrSettings.m_ImageUpLinear);
  SettingSetScaling("LumaDown", madvrSettings.m_ImageDownscaling);
  SettingSetBool("lumaDownAntiRinging", madvrSettings.m_ImageDownAntiRing);
  SettingSetBool("lumaDownLinear", madvrSettings.m_ImageDownLinear);
  SettingSetDoubling("nnediDL", madvrSettings.m_ImageDoubleLuma);
  SettingSetDoublingCondition("nnediDLScalingFactor", madvrSettings.m_ImageDoubleLumaFactor);
  SettingSetDoubling("nnediDC", madvrSettings.m_ImageDoubleChroma);
  SettingSetDoublingCondition("nnediDCScalingFactor", madvrSettings.m_ImageDoubleChromaFactor);
  SettingSetDoubling("nnediQL", madvrSettings.m_ImageQuadrupleLuma);
  SettingSetQuadrupleCondition("nnediQLScalingFactor", madvrSettings.m_ImageQuadrupleLumaFactor);
  SettingSetDoubling("nnediQC", madvrSettings.m_ImageQuadrupleChroma);
  SettingSetQuadrupleCondition("nnediQCScalingFactor", madvrSettings.m_ImageQuadrupleChromaFactor);
  SettingSetDeintActive("", madvrSettings.m_deintactive);
  SettingSetDeintForce("contentType", madvrSettings.m_deintforce);
  SettingSetBool("scanPartialFrame", madvrSettings.m_deintlookpixels);
  SettingSetBool("debandActive", madvrSettings.m_deband);
  SettingSetInt("debandLevel", madvrSettings.m_debandLevel);
  SettingSetInt("debandFadeLevel", madvrSettings.m_debandFadeLevel);
  SettingSetDithering("", madvrSettings.m_dithering);
  SettingSetBool("coloredDither", madvrSettings.m_ditheringColoredNoise);
  SettingSetBool("dynamicDither", madvrSettings.m_ditheringEveryFrame);
  SettingSetSmoothmotion("", madvrSettings.m_smoothMotion);
}

