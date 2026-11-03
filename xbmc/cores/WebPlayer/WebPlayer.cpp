/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebPlayer.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "WebPlayerDefs.h"
#include "WebVideoBuffer.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPowerHandling.h"
#include "cores/DataCacheCore.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/keyboard/Key.h"
#include "input/keyboard/XBMC_keyboard.h"
#include "input/touch/generic/GenericTouchInputHandler.h"
#include "kodi-web-video.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include "platform/linux/waylandhost/WaylandBuffer.h"
#include "platform/linux/waylandhost/WaylandSurface.h"

#if defined(HAVE_GBM) && defined(HAS_GLES)
#include "WebLayerBridge.h"
#include "windowing/gbm/WinSystemGbm.h"
#include "windowing/gbm/drm/DRMAtomic.h"
#endif

#include <cmath>
#include <mutex>

#include <linux/input-event-codes.h>

using namespace KODI::WEB;
using namespace KODI::WAYLANDHOST;

namespace
{

constexpr int RENDER_BUFFERS = 3;

// The web window's keymap section claims keys for Kodi outright. Of the
// others, a key bound globally to a Kodi command or to the volume stays
// with Kodi as well, unless it types a character into the page or is one of
// the colour keys, which TV pages use for their own menus.
bool IsKodiKey(const XBMC_keysym& keysym, const CKey& key)
{
  CInputManager& input = CServiceBroker::GetInputManager();
  if (input.GetAction(WINDOW_FULLSCREEN_WEB, key, false).GetID() != ACTION_NONE)
    return true;

  if (keysym.unicode >= 0x20 && keysym.unicode != 0x7f)
    return false;

  switch (keysym.sym)
  {
    case XBMCK_RED:
    case XBMCK_GREEN:
    case XBMCK_YELLOW:
    case XBMCK_BLUE:
      return false;
    default:
      break;
  }

  switch (input.GetAction(WINDOW_FULLSCREEN_WEB, key, true).GetID())
  {
    case ACTION_BUILT_IN_FUNCTION:
    case ACTION_VOLUME_UP:
    case ACTION_VOLUME_DOWN:
    case ACTION_MUTE:
      return true;
    default:
      return false;
  }
}

AVColorSpace ColorSpaceOf(uint32_t matrix)
{
  switch (matrix)
  {
    case KODI_WEB_VIDEO_MATRIX_BT601:
      return AVCOL_SPC_SMPTE170M;
    case KODI_WEB_VIDEO_MATRIX_BT709:
      return AVCOL_SPC_BT709;
    case KODI_WEB_VIDEO_MATRIX_BT2020:
      return AVCOL_SPC_BT2020_NCL;
    default:
      return AVCOL_SPC_UNSPECIFIED;
  }
}

AVColorPrimaries PrimariesOf(uint32_t matrix)
{
  switch (matrix)
  {
    case KODI_WEB_VIDEO_MATRIX_BT601:
      return AVCOL_PRI_SMPTE170M;
    case KODI_WEB_VIDEO_MATRIX_BT709:
      return AVCOL_PRI_BT709;
    case KODI_WEB_VIDEO_MATRIX_BT2020:
      return AVCOL_PRI_BT2020;
    default:
      return AVCOL_PRI_UNSPECIFIED;
  }
}

AVColorTransferCharacteristic TransferOf(uint32_t transfer)
{
  switch (transfer)
  {
    case KODI_WEB_VIDEO_TRANSFER_SDR:
      return AVCOL_TRC_BT709;
    case KODI_WEB_VIDEO_TRANSFER_PQ:
      return AVCOL_TRC_SMPTE2084;
    case KODI_WEB_VIDEO_TRANSFER_HLG:
      return AVCOL_TRC_ARIB_STD_B67;
    default:
      return AVCOL_TRC_UNSPECIFIED;
  }
}

} // unnamed namespace

CWebPlayer::CWebPlayer(IPlayerCallback& callback)
  : IPlayer(callback),
    m_renderManager(m_clock, this)
{
}

CWebPlayer::~CWebPlayer()
{
  CloseFile();
}

bool CWebPlayer::OpenFile(const CFileItem& file, const CPlayerOptions& options)
{
  CloseFile();

  m_manager = CServiceBroker::GetAppComponents().GetComponent<CWebClientManager>();
  if (!m_manager || !m_manager->IsEnabled())
  {
    CLog::Log(LOGERROR, "CWebPlayer::{}: the web client is disabled", __FUNCTION__);
    return false;
  }

  m_item = std::make_unique<CFileItem>(file);
  m_options = options;
  m_stop = false;
  m_configured = false;
  m_started = false;
  m_videoFrames = false;
  m_shown = false;
  m_mediaVideo = false;
  m_mediaAudio = false;
  m_mediaPlaying = false;
  m_surface = nullptr;
  m_url = file.GetDynPath();
  m_profile = file.GetProperty(PROPERTY_PROFILE).asString(DEFAULT_PROFILE);
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();

  m_processInfo.reset(CProcessInfo::CreateInstance());
  m_processInfo->SetDataCache(&CServiceBroker::GetDataCacheCore());
  m_processInfo->SetSpeed(0.0f);
  m_processInfo->SetTempo(1.0f);
  m_processInfo->SetFrameAdvance(false);
  m_processInfo->SetStateRealtime(true);
  m_pool = std::make_shared<CWebVideoBufferPool>();

  m_clock.SetSpeed(DVD_PLAYSPEED_NORMAL);
  m_clock.Discontinuity(0.0);
  m_renderManager.PreInit();

  std::unique_lock playLock(m_playLock);
  m_opening = true;
  m_openFailed = false;
  playLock.unlock();

  if (!m_manager->RequestView(m_profile, *this))
  {
    playLock.lock();
    m_opening = false;
    playLock.unlock();
    CloseFile();
    return false;
  }

#if defined(HAVE_GBM) && defined(HAS_GLES)
  auto* gbm = dynamic_cast<WINDOWING::GBM::CWinSystemGbm*>(CServiceBroker::GetWinSystem());
  auto drm = gbm ? std::dynamic_pointer_cast<WINDOWING::GBM::CDRMAtomic>(gbm->GetDrm()) : nullptr;
  if (drm && m_manager->ShowsVideoOnPlane())
  {
    m_bridge = std::make_shared<CWebLayerBridge>(drm);
    gbm->RegisterVideoLayerBridge(m_bridge);
    m_manager->SetVideoObserver(this);
  }
#endif

  CServiceBroker::GetWinSystem()->RegisterRenderLoop(this);
  CServiceBroker::GetInputManager().RegisterKeyboardEventHandler(this);
  CServiceBroker::GetInputManager().RegisterMouseDriverHandler(this);
  CGenericTouchInputHandler::GetInstance().RegisterEventHandler(this);

  playLock.lock();
  m_opening = false;
  if (m_openFailed)
  {
    playLock.unlock();
    CLog::Log(LOGERROR, "CWebPlayer::{}: the web client failed while opening {}", __FUNCTION__,
              m_url);
    CloseFile();
    return false;
  }
  m_playing = true;
  m_callback.OnPlayBackStarted(*m_item);
  if (!m_mediaPlaying)
    m_callback.OnPlayBackPaused();
  return true;
}

bool CWebPlayer::HasVideo() const
{
  return m_mediaVideo;
}

bool CWebPlayer::HasAudio() const
{
  return m_mediaAudio;
}

bool CWebPlayer::CloseFile(bool reopen)
{
  if (!m_manager)
    return true;

  // before m_manager goes: the touch input thread reaches the client through it
  CGenericTouchInputHandler::GetInstance().UnregisterEventHandler(this);
  m_touchInput = false;
  m_touchPoints.clear();

  const bool wasPlaying = m_playing.exchange(false);
  m_stop = true;
  m_manager->SetVideoObserver(nullptr);
  m_manager->ReleaseView(*this);
  m_surface = nullptr;
  m_manager.reset();

  CServiceBroker::GetInputManager().UnregisterKeyboardEventHandler(this);
  CServiceBroker::GetInputManager().UnregisterMouseDriverHandler(this);
  m_inputActive = false;
  m_pressedKeys.clear();
  CServiceBroker::GetWinSystem()->UnregisterRenderLoop(this);
  m_renderManager.UnInit();
  {
    std::unique_lock lock(m_frameLock);
    if (m_page)
      m_page->Unlock();
    m_page.reset();
    m_videoFrames = false;
  }
  m_bridge.reset();
  m_pool.reset();
  CServiceBroker::GetDataCacheCore().Reset();
  m_processInfo.reset();

  if (wasPlaying)
    m_callback.OnPlayBackStopped();
  return true;
}

void CWebPlayer::Render(bool clear, uint32_t alpha, bool gui)
{
  m_renderManager.Render(clear, 0, alpha, gui);
}

void CWebPlayer::FlushRenderer()
{
  m_renderManager.Flush(true, true);
}

void CWebPlayer::SetRenderViewMode(int mode, float zoom, float par, float shift, bool stretch)
{
  if (!m_processInfo)
    return;

  m_processInfo->GetVideoSettingsLocked().SetViewMode(mode, zoom, par, shift, stretch);
  m_renderManager.SetVideoSettings(m_processInfo->GetVideoSettings());
  m_renderManager.SetViewMode(mode);
}

float CWebPlayer::GetRenderAspectRatio() const
{
  return m_renderManager.GetAspectRatio();
}

void CWebPlayer::TriggerUpdateResolution()
{
  std::string stereoMode;
  m_renderManager.TriggerUpdateResolution(0, 0, 0, stereoMode);
}

bool CWebPlayer::IsRenderingVideo() const
{
  return m_renderManager.IsConfigured();
}

void CWebPlayer::GetRects(CRect& source, CRect& dest, CRect& view) const
{
  m_renderManager.GetVideoRect(source, dest, view);
}

unsigned int CWebPlayer::GetOrientation() const
{
  return m_renderManager.GetOrientation();
}

bool CWebPlayer::Supports(ERENDERFEATURE feature) const
{
  return m_renderManager.Supports(feature);
}

bool CWebPlayer::Supports(ESCALINGMETHOD method) const
{
  return m_renderManager.Supports(method);
}

CVideoSettings CWebPlayer::GetVideoSettings() const
{
  return m_processInfo ? m_processInfo->GetVideoSettings() : CVideoSettings();
}

void CWebPlayer::SetVideoSettings(CVideoSettings& settings)
{
  if (!m_processInfo)
    return;

  m_processInfo->SetVideoSettings(settings);
  m_renderManager.SetVideoSettings(settings);
}

void CWebPlayer::FrameMove()
{
  m_renderManager.FrameMove();
  UpdateInputRouting();
  UpdateVisibility();
}

void CWebPlayer::UpdateVisibility()
{
  const bool shown =
      m_configured && m_manager &&
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_WEB;
  if (shown == m_shown)
    return;

  m_shown = shown;
  m_manager->SetActive(m_profile, shown);
}

void CWebPlayer::UpdateInputRouting()
{
  const bool active = m_configured && m_manager &&
                      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog() ==
                          WINDOW_FULLSCREEN_WEB;
  // a touch on a sleeping display wakes it through Kodi
  m_touchInput = active && !DisplayAsleep();
  if (m_touchActivity.exchange(false))
    InputWakesDisplay();

  if (active == m_inputActive)
    return;

  m_inputActive = active;
  m_pointerX = -1;
  m_pointerY = -1;
  if (active)
    return;

  // a key still held when Kodi takes the input back must not stay down
  // for the client
  for (uint32_t scancode : m_pressedKeys)
    m_manager->GetHost()->KeyboardKey(scancode, false);
  m_pressedKeys.clear();
  m_pressedButtons.clear();

  std::unique_lock lock(m_touchLock);
  if (!m_touchPoints.empty() && !m_touchCancelled)
    m_manager->GetHost()->TouchCancel();
  m_touchCancelled = true;
}

// input the page takes never reaches CInputManager, which counts it as
// activity and lets it wake the display; true when it woke the display
bool CWebPlayer::InputWakesDisplay() const
{
  const auto appPower =
      CServiceBroker::GetAppComponents().GetComponent<CApplicationPowerHandling>();
  appPower->ResetSystemIdleTimer();
  appPower->ResetScreenSaver();
  return appPower->WakeUpScreenSaverAndDPMS();
}

bool CWebPlayer::DisplayAsleep() const
{
  const auto appPower =
      CServiceBroker::GetAppComponents().GetComponent<CApplicationPowerHandling>();
  return appPower->IsDPMSActive() || appPower->IsInScreenSaver();
}

bool CWebPlayer::OnKeyboardEvent(const XBMC_keysym& keysym, const CKey& key, bool pressed)
{
  if (!m_inputActive || IsKodiKey(keysym, key))
    return false;

  // the key that wakes the display does nothing else, its release included
  if (pressed)
  {
    if (InputWakesDisplay())
      return true;
    m_pressedKeys.insert(keysym.scancode);
  }
  else if (m_pressedKeys.erase(keysym.scancode) == 0)
    return true;
  m_manager->GetHost()->KeyboardKey(keysym.scancode, pressed);
  return true;
}

bool CWebPlayer::OnPosition(int x, int y)
{
  // whether motion wakes the display is Kodi's decision
  if (!m_inputActive || DisplayAsleep())
    return false;

  // an air mouse lying still keeps reporting the same position
  if (x == m_pointerX && y == m_pointerY)
    return false;
  m_pointerX = x;
  m_pointerY = y;

  m_manager->GetHost()->PointerMotion(x, y);
  // Kodi keeps drawing the pointer, so the event goes on to the GUI
  return false;
}

bool CWebPlayer::OnButtonPress(MOUSE::BUTTON_ID button)
{
  if (!m_inputActive)
    return false;

  if (InputWakesDisplay())
    return true;
  m_pressedButtons.insert(button);

  switch (button)
  {
    case MOUSE::BUTTON_ID::LEFT:
      m_manager->GetHost()->PointerButton(BTN_LEFT, true);
      return true;
    case MOUSE::BUTTON_ID::RIGHT:
      m_manager->GetHost()->PointerButton(BTN_RIGHT, true);
      return true;
    case MOUSE::BUTTON_ID::MIDDLE:
      m_manager->GetHost()->PointerButton(BTN_MIDDLE, true);
      return true;
    case MOUSE::BUTTON_ID::BUTTON4:
      m_manager->GetHost()->PointerButton(BTN_SIDE, true);
      return true;
    case MOUSE::BUTTON_ID::BUTTON5:
      m_manager->GetHost()->PointerButton(BTN_EXTRA, true);
      return true;
    case MOUSE::BUTTON_ID::WHEEL_UP:
      m_manager->GetHost()->PointerAxis(-120);
      return true;
    case MOUSE::BUTTON_ID::WHEEL_DOWN:
      m_manager->GetHost()->PointerAxis(120);
      return true;
    default:
      return false;
  }
}

void CWebPlayer::OnButtonRelease(MOUSE::BUTTON_ID button)
{
  if (!m_inputActive || m_pressedButtons.erase(button) == 0)
    return;

  switch (button)
  {
    case MOUSE::BUTTON_ID::LEFT:
      m_manager->GetHost()->PointerButton(BTN_LEFT, false);
      break;
    case MOUSE::BUTTON_ID::RIGHT:
      m_manager->GetHost()->PointerButton(BTN_RIGHT, false);
      break;
    case MOUSE::BUTTON_ID::MIDDLE:
      m_manager->GetHost()->PointerButton(BTN_MIDDLE, false);
      break;
    case MOUSE::BUTTON_ID::BUTTON4:
      m_manager->GetHost()->PointerButton(BTN_SIDE, false);
      break;
    case MOUSE::BUTTON_ID::BUTTON5:
      m_manager->GetHost()->PointerButton(BTN_EXTRA, false);
      break;
    default:
      break;
  }
}

bool CWebPlayer::OnTouchEvent(TouchInput event, float x, float y, int64_t time, int32_t pointer)
{
  std::unique_lock lock(m_touchLock);
  CWaylandHost* host = m_manager->GetHost();
  const auto surfaceX = static_cast<int32_t>(std::lround(x));
  const auto surfaceY = static_cast<int32_t>(std::lround(y));

  switch (event)
  {
    case TouchInputDown:
      if (m_touchPoints.empty())
      {
        if (!m_touchInput)
          return false;
        m_touchCancelled = false;
      }
      m_touchPoints.insert(pointer);
      if (m_touchCancelled)
        return true;
      host->TouchDown(pointer, surfaceX, surfaceY);
      break;
    case TouchInputMove:
      if (!m_touchPoints.contains(pointer))
        return false;
      if (m_touchCancelled)
        return true;
      host->TouchMotion(pointer, surfaceX, surfaceY);
      break;
    case TouchInputUp:
      if (m_touchPoints.erase(pointer) == 0)
        return false;
      if (m_touchCancelled)
        return true;
      host->TouchUp(pointer);
      break;
    case TouchInputAbort:
      if (m_touchPoints.empty())
        return false;
      m_touchPoints.clear();
      if (!m_touchCancelled)
        host->TouchCancel();
      return true;
    default:
      return false;
  }

  host->TouchFrame();
  m_touchActivity = true;
  return true;
}

void CWebPlayer::OnWebViewAvailable(const std::string& profile, CWaylandSurface& surface)
{
  CLog::Log(LOGINFO, "CWebPlayer::{}: view for profile \"{}\" loads {}", __FUNCTION__, profile,
            m_url);
  m_surface = &surface;
  m_manager->LoadUrl(profile, m_url);
  m_manager->SetActive(profile, true);
}

void CWebPlayer::OnWebViewUnavailable(const std::string& profile, const std::string& message)
{
  CLog::Log(LOGERROR, "CWebPlayer::{}: no view for profile \"{}\": {}", __FUNCTION__, profile,
            message);
  StopFromClient();
}

void CWebPlayer::OnWebViewGone(const std::string& profile)
{
  CLog::Log(LOGINFO, "CWebPlayer::{}: the view for profile \"{}\" is gone", __FUNCTION__, profile);
  m_surface = nullptr;
  StopFromClient();
}

void CWebPlayer::OnWebViewUnmapped(CWaylandSurface& surface)
{
  CLog::Log(LOGINFO, "CWebPlayer::{}: client surface \"{}\" is gone", __FUNCTION__,
            surface.GetTitle());
}

void CWebPlayer::OnWebViewUrl(const std::string& profile, const std::string& url)
{
  CLog::Log(LOGDEBUG, "CWebPlayer::{}: {}: {}", __FUNCTION__, profile, url);
}

void CWebPlayer::OnWebViewTitle(const std::string& profile, const std::string& title)
{
  CLog::Log(LOGDEBUG, "CWebPlayer::{}: {}: {}", __FUNCTION__, profile, title);
}

void CWebPlayer::OnWebViewProgress(const std::string& profile, uint32_t percent)
{
  CLog::Log(LOGDEBUG, "CWebPlayer::{}: {}: {} percent", __FUNCTION__, profile, percent);
}

void CWebPlayer::OnWebViewNavigationState(const std::string& profile,
                                          bool canGoBack,
                                          bool canGoForward)
{
  CLog::Log(LOGDEBUG, "CWebPlayer::{}: {}: back {} forward {}", __FUNCTION__, profile, canGoBack,
            canGoForward);
}

void CWebPlayer::OnWebViewError(const std::string& profile,
                                uint32_t code,
                                const std::string& message)
{
  CLog::Log(LOGERROR, "CWebPlayer::{}: {}: error {}: {}", __FUNCTION__, profile, code, message);
  if (code == static_cast<uint32_t>(WebViewError::CONFIG_FAILED))
    StopFromClient();
}

void CWebPlayer::OnWebViewExit(const std::string& profile)
{
  CLog::Log(LOGINFO, "CWebPlayer::{}: the page of profile \"{}\" asks to leave", __FUNCTION__,
            profile);
  StopFromClient();
}

void CWebPlayer::OnWebViewMediaState(const std::string& profile,
                                     bool video,
                                     bool audio,
                                     bool paused)
{
  CLog::Log(LOGDEBUG, "CWebPlayer::{}: {}: video {} audio {} paused {}", __FUNCTION__, profile,
            video, audio, paused);
  const bool playing = (video || audio) && !paused;

  std::unique_lock lock(m_playLock);
  const bool streamsChanged = m_mediaVideo != video || m_mediaAudio != audio;
  const bool playingChanged = m_mediaPlaying != playing;
  m_mediaVideo = video;
  m_mediaAudio = audio;
  m_mediaPlaying = playing;
  if (!m_playing)
    return;

  CServiceBroker::GetDataCacheCore().SetSpeed(1.0f, playing ? 1.0f : 0.0f);
  if (streamsChanged)
    m_callback.OnAVChange();
  if (!playingChanged)
    return;
  if (playing)
    m_callback.OnPlayBackResumed();
  else
    m_callback.OnPlayBackPaused();
}

void CWebPlayer::OnWebViewFrame(CWaylandSurface& surface, std::shared_ptr<CWaylandBuffer> buffer)
{
  if (&surface != m_surface || m_stop || !m_pool)
  {
    buffer->Unlock();
    return;
  }

  std::unique_lock lock(m_frameLock);
  KeepPage(buffer);
  if (m_videoFrames && m_bridge)
  {
    auto* page = static_cast<CWebVideoBuffer*>(m_pool->Get());
    page->SetBuffer(std::move(buffer));
    m_bridge->SetPage(page);
    return;
  }

  ShowPage(std::move(buffer));
}

// the last page stays locked, to be shown again when its video leaves the video plane
void CWebPlayer::KeepPage(const std::shared_ptr<CWaylandBuffer>& buffer)
{
  if (m_page)
    m_page->Unlock();
  buffer->Lock();
  m_page = buffer;
}

void CWebPlayer::ShowPage(std::shared_ptr<CWaylandBuffer> buffer)
{
  auto* videoBuffer = static_cast<CWebVideoBuffer*>(m_pool->Get());
  videoBuffer->SetBuffer(buffer);

  VideoPicture picture;
  picture.Reset();
  picture.videoBuffer = videoBuffer;
  picture.iWidth = buffer->GetWidth();
  picture.iHeight = buffer->GetHeight();
  picture.iDisplayWidth = picture.iWidth;
  picture.iDisplayHeight = picture.iHeight;
  picture.pixelFormat = AV_PIX_FMT_DRM_PRIME;
  picture.color_space = AVCOL_SPC_BT709;
  picture.color_range = 1;
  picture.color_primaries = AVCOL_PRI_BT709;
  picture.color_transfer = AVCOL_TRC_BT709;
  picture.chroma_position = AVCHROMA_LOC_UNSPECIFIED;
  picture.pts = m_clock.GetClock();
  picture.dts = DVD_NOPTS_VALUE;
  videoBuffer->SetPictureParams(picture);
  ShowPicture(picture);
}

void CWebPlayer::OnWebVideoFrame(CWebVideoFrame* frame)
{
  std::unique_lock lock(m_frameLock);
  if (m_stop || !m_pool || !m_bridge)
  {
    frame->Release();
    return;
  }

  if (!m_videoFrames)
  {
    CLog::Log(LOGINFO, "CWebPlayer::{}: the page's video goes to the video plane", __FUNCTION__);
    m_videoFrames = true;
    if (m_page)
    {
      m_page->Lock();
      auto* page = static_cast<CWebVideoBuffer*>(m_pool->Get());
      page->SetBuffer(m_page);
      m_bridge->SetPage(page);
    }
  }

  const uint64_t displayWidth =
      static_cast<uint64_t>(frame->GetWidth()) * frame->GetParN() / frame->GetParD();

  VideoPicture picture;
  picture.Reset();
  picture.videoBuffer = frame;
  picture.iWidth = frame->GetWidth();
  picture.iHeight = frame->GetHeight();
  picture.iDisplayWidth = static_cast<unsigned int>(displayWidth);
  picture.iDisplayHeight = picture.iHeight;
  picture.pixelFormat = AV_PIX_FMT_DRM_PRIME;
  picture.color_space = ColorSpaceOf(frame->GetMatrix());
  picture.color_range = frame->GetRange() == KODI_WEB_VIDEO_RANGE_FULL ? 1 : 0;
  picture.color_primaries = PrimariesOf(frame->GetMatrix());
  picture.color_transfer = TransferOf(frame->GetTransfer());
  picture.chroma_position = AVCHROMA_LOC_UNSPECIFIED;
  picture.pts = m_clock.GetClock();
  picture.dts = DVD_NOPTS_VALUE;
  frame->SetPictureParams(picture);
  ShowPicture(picture);
}

void CWebPlayer::OnWebVideoClear()
{
  std::unique_lock lock(m_frameLock);
  if (!m_videoFrames)
    return;

  CLog::Log(LOGINFO, "CWebPlayer::{}: the page's video leaves the video plane", __FUNCTION__);
  m_videoFrames = false;
  m_bridge->ClearPage();
  if (m_page && !m_stop && m_pool)
  {
    m_page->Lock();
    ShowPage(m_page);
  }
}

void CWebPlayer::OnWebVideoRect(const CRect& rect)
{
  std::unique_lock lock(m_frameLock);
  if (m_bridge)
    m_bridge->SetVideoRect(rect);
}

void CWebPlayer::ShowPicture(VideoPicture& picture)
{
  auto* buffer = static_cast<CVideoBufferDRMPRIME*>(picture.videoBuffer);
  const AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  const uint32_t format = descriptor->layers[0].format;

  if (!m_configured || format != m_configuredFormat || picture.iWidth != m_configuredWidth ||
      picture.iHeight != m_configuredHeight)
  {
    CLog::Log(LOGINFO, "CWebPlayer::{}: frames {}x{} format {:#x} modifier {:#x} planes {}",
              __FUNCTION__, picture.iWidth, picture.iHeight, format,
              descriptor->objects[0].format_modifier, descriptor->layers[0].nb_planes);
    m_processInfo->SetVideoDimensions(picture.iWidth, picture.iHeight);
    m_processInfo->SetVideoDAR(static_cast<float>(picture.iWidth) / picture.iHeight);
    m_processInfo->SetVideoDecoderName("wayland", true);
    m_processInfo->SetVideoPixelFormat("drm_prime");
    m_processInfo->SetVideoFps(m_fps);
    m_processInfo->SetVideoInterlaced(false);
    m_processInfo->SetVideoDeintMethod("none");
    m_configuredFormat = format;
    m_configuredWidth = picture.iWidth;
    m_configuredHeight = picture.iHeight;
  }

  // the renderer may also give way to another one for pictures of the same kind
  if (!ConfigureRenderer(picture))
    return;

  if (!m_renderManager.AddVideoPicture(picture, m_stop, VS_INTERLACEMETHOD_NONE, false))
    CLog::Log(LOGDEBUG, "CWebPlayer::{}: frame dropped, no free render buffer", __FUNCTION__);
}

bool CWebPlayer::ConfigureRenderer(const VideoPicture& picture)
{
  if (!m_renderManager.Configure(picture, m_fps, 0, RENDER_BUFFERS))
  {
    CLog::Log(LOGERROR, "CWebPlayer::{}: no renderer for the client's {}x{} buffers", __FUNCTION__,
              picture.iWidth, picture.iHeight);
    m_stop = true;
    StopFromClient(true);
    return false;
  }

  m_configured = true;
  if (m_started)
    return true;

  m_started = true;
  m_renderManager.ShowVideo(true);
  if (m_options.fullscreen)
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SWITCHTOFULLSCREEN);
  m_callback.OnAVStarted(*m_item);
  return true;
}

void CWebPlayer::StopFromClient(bool error)
{
  std::unique_lock lock(m_playLock);
  if (m_opening)
  {
    m_openFailed = true;
    return;
  }
  if (!m_playing.exchange(false))
    return;

  if (error)
    m_callback.OnPlayBackError();
  else
    m_callback.OnPlayBackEnded();
}

void CWebPlayer::GetDebugInfo(std::string& audio, std::string& video, std::string& general)
{
  audio.clear();
  video = "web client frames through the Wayland host";
  general.clear();
}

void CWebPlayer::UpdateClockSync(bool enabled)
{
  m_processInfo->SetRenderClockSync(enabled);
}

void CWebPlayer::UpdateRenderInfo(CRenderInfo& info)
{
  m_processInfo->UpdateRenderInfo(info);
}

void CWebPlayer::UpdateRenderBuffers(int queued, int discard, int free)
{
  m_processInfo->UpdateRenderBuffers(queued, discard, free);
}

void CWebPlayer::UpdateGuiRender(bool gui)
{
  m_processInfo->SetGuiRender(gui);
}

void CWebPlayer::UpdateVideoRender(bool video)
{
  m_processInfo->SetVideoRender(video);
}
