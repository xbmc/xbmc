/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WebClientManager.h"
#include "cores/IPlayer.h"
#include "cores/VideoPlayer/DVDClock.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "guilib/DispResource.h"
#include "input/keyboard/interfaces/IKeyboardEventHandler.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "input/touch/ITouchEventHandler.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <memory>
#include <set>

class CFileItem;
class CProcessInfo;

namespace KODI
{
namespace WEB
{

class CWebLayerBridge;
class CWebVideoBufferPool;

/*!
 * \brief Plays a web page: the view the client manager provides for the
 * item's profile loads the URL, and the frames the client commits go
 * through Kodi's video renderers, so the GUI composes over them like over
 * any other video. While a page's video hands its frames to Kodi, those go
 * to the video plane instead and the page to a plane above it. While the
 * web window is up, keys the keymap does not claim, the pointer and touch
 * go to the client.
 */
class CWebPlayer : public IPlayer,
                   public IRenderLoop,
                   public IRenderMsg,
                   public IWebViewObserver,
                   public IWebVideoObserver,
                   public KEYBOARD::IKeyboardEventHandler,
                   public MOUSE::IMouseDriverHandler,
                   public ITouchEventHandler
{
public:
  explicit CWebPlayer(IPlayerCallback& callback);
  ~CWebPlayer() override;

  // implementation of IPlayer
  bool OpenFile(const CFileItem& file, const CPlayerOptions& options) override;
  bool CloseFile(bool reopen = false) override;
  bool IsPlaying() const override { return m_playing; }
  bool CanPause() const override { return false; }
  void Pause() override {}
  bool HasVideo() const override;
  bool HasAudio() const override;
  bool CanSeek() const override { return false; }
  void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) override {}
  void SetSpeed(float speed) override {}
  void Render(bool clear, uint32_t alpha = 255, bool gui = true) override;
  void FlushRenderer() override;
  void SetRenderViewMode(int mode, float zoom, float par, float shift, bool stretch) override;
  float GetRenderAspectRatio() const override;
  void TriggerUpdateResolution() override;
  bool IsRenderingVideo() const override;
  void GetRects(CRect& source, CRect& dest, CRect& view) const override;
  unsigned int GetOrientation() const override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;
  CVideoSettings GetVideoSettings() const override;
  void SetVideoSettings(CVideoSettings& settings) override;

  // implementation of IRenderLoop
  void FrameMove() override;

  // implementation of IWebViewObserver
  void OnWebViewAvailable(const std::string& profile,
                          WAYLANDHOST::CWaylandSurface& surface) override;
  void OnWebViewUnavailable(const std::string& profile, const std::string& message) override;
  void OnWebViewGone(const std::string& profile) override;
  void OnWebViewFrame(WAYLANDHOST::CWaylandSurface& surface,
                      std::shared_ptr<WAYLANDHOST::CWaylandBuffer> buffer) override;
  void OnWebViewUnmapped(WAYLANDHOST::CWaylandSurface& surface) override;
  void OnWebViewUrl(const std::string& profile, const std::string& url) override;
  void OnWebViewTitle(const std::string& profile, const std::string& title) override;
  void OnWebViewProgress(const std::string& profile, uint32_t percent) override;
  void OnWebViewNavigationState(const std::string& profile,
                                bool canGoBack,
                                bool canGoForward) override;
  void OnWebViewError(const std::string& profile,
                      uint32_t code,
                      const std::string& message) override;
  void OnWebViewExit(const std::string& profile) override;
  void OnWebViewMediaState(const std::string& profile,
                           bool video,
                           bool audio,
                           bool paused) override;

  // implementation of IWebVideoObserver
  void OnWebVideoFrame(CWebVideoFrame* frame) override;
  void OnWebVideoClear() override;
  void OnWebVideoRect(const CRect& rect) override;

  // implementation of IKeyboardEventHandler
  bool OnKeyboardEvent(const XBMC_keysym& keysym, const CKey& key, bool pressed) override;

  // implementation of IMouseDriverHandler
  bool OnPosition(int x, int y) override;
  bool OnButtonPress(MOUSE::BUTTON_ID button) override;
  void OnButtonRelease(MOUSE::BUTTON_ID button) override;

  // implementation of ITouchEventHandler
  bool OnTouchEvent(TouchInput event, float x, float y, int64_t time, int32_t pointer) override;

protected:
  // implementation of IRenderMsg
  void VideoParamsChange() override {}
  void GetDebugInfo(std::string& audio, std::string& video, std::string& general) override;
  void UpdateClockSync(bool enabled) override;
  void UpdateRenderInfo(CRenderInfo& info) override;
  void UpdateRenderBuffers(int queued, int discard, int free) override;
  void UpdateGuiRender(bool gui) override;
  void UpdateVideoRender(bool video) override;

private:
  bool ConfigureRenderer(const VideoPicture& picture);
  void ShowPicture(VideoPicture& picture);
  void ShowPage(std::shared_ptr<WAYLANDHOST::CWaylandBuffer> buffer);
  void KeepPage(const std::shared_ptr<WAYLANDHOST::CWaylandBuffer>& buffer);
  void UpdateInputRouting();
  void UpdateVisibility();
  bool InputWakesDisplay() const;
  bool DisplayAsleep() const;
  void StopFromClient(bool error = false);
  bool AwaitFilterLists();

  std::unique_ptr<CProcessInfo> m_processInfo;
  CDVDClock m_clock;
  CRenderManager m_renderManager;
  std::shared_ptr<CWebClientManager> m_manager;
  std::string m_profile;
  std::string m_url;
  std::atomic<WAYLANDHOST::CWaylandSurface*> m_surface{nullptr};
  std::shared_ptr<CWebVideoBufferPool> m_pool;
  std::shared_ptr<CWebLayerBridge> m_bridge;
  std::unique_ptr<CFileItem> m_item;
  CPlayerOptions m_options;
  std::atomic<bool> m_playing{false};
  // a stop from the client while OpenFile() runs makes the open fail
  CCriticalSection m_playLock;
  bool m_opening{false};
  bool m_openFailed{false};
  volatile std::atomic_bool m_stop{false};
  CCriticalSection m_frameLock;
  std::shared_ptr<WAYLANDHOST::CWaylandBuffer> m_page;
  bool m_videoFrames{false};
  bool m_configured{false};
  bool m_started{false};
  uint32_t m_configuredFormat{0};
  unsigned int m_configuredWidth{0};
  unsigned int m_configuredHeight{0};
  float m_fps{0.0f};
  bool m_inputActive{false};
  bool m_shown{false};
  std::atomic<bool> m_mediaVideo{false};
  std::atomic<bool> m_mediaAudio{false};
  bool m_mediaPlaying{false};
  std::set<uint32_t> m_pressedKeys;
  std::set<MOUSE::BUTTON_ID> m_pressedButtons;
  int m_pointerX{-1};
  int m_pointerY{-1};
  // touch arrives on the input thread; a touch the page took stays with it
  // until its last point is up
  std::atomic<bool> m_touchInput{false};
  std::atomic<bool> m_touchActivity{false};
  CCriticalSection m_touchLock;
  std::set<int32_t> m_touchPoints;
  bool m_touchCancelled{false};
};

} // namespace WEB
} // namespace KODI
