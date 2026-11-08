/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WebAudioServer.h"
#include "WebFilterLists.h"
#include "WebVideoServer.h"
#include "application/IApplicationComponent.h"
#include "guilib/DispResource.h"
#include "jobs/Job.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/Condition.h"
#include "threads/CriticalSection.h"

#include "platform/linux/waylandhost/WaylandHost.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace ADDON
{
struct AddonEvent;
}

namespace KODI
{
namespace WEB
{

/*!
 * \brief What a player wants to know about the view it asked for. All
 * methods run on the host thread.
 */
class IWebViewObserver
{
public:
  virtual ~IWebViewObserver() = default;

  virtual void OnWebViewAvailable(const std::string& profile,
                                  WAYLANDHOST::CWaylandSurface& surface) = 0;
  virtual void OnWebViewUnavailable(const std::string& profile, const std::string& message) = 0;
  virtual void OnWebViewGone(const std::string& profile) = 0;
  virtual void OnWebViewFrame(WAYLANDHOST::CWaylandSurface& surface,
                              std::shared_ptr<WAYLANDHOST::CWaylandBuffer> buffer) = 0;
  virtual void OnWebViewUnmapped(WAYLANDHOST::CWaylandSurface& surface) = 0;
  virtual void OnWebViewUrl(const std::string& profile, const std::string& url) = 0;
  virtual void OnWebViewTitle(const std::string& profile, const std::string& title) = 0;
  virtual void OnWebViewProgress(const std::string& profile, uint32_t percent) = 0;
  virtual void OnWebViewNavigationState(const std::string& profile,
                                        bool canGoBack,
                                        bool canGoForward) = 0;
  virtual void OnWebViewError(const std::string& profile,
                              uint32_t code,
                              const std::string& message) = 0;
  virtual void OnWebViewExit(const std::string& profile) = 0;
  virtual void OnWebViewMediaState(const std::string& profile,
                                   bool video,
                                   bool audio,
                                   bool paused) = 0;
};

/*!
 * \brief Runs the Wayland host for Kodi's lifetime and the web client
 * alongside: the client is started once the GUI is up, a view lives as long
 * as the player that shows it, and a crashed client is started again once.
 */
class CWebClientManager : public IApplicationComponent,
                          public WAYLANDHOST::IWaylandHostObserver,
                          public IRenderLoop,
                          public IDispResource,
                          public ISettingCallback
{
public:
  enum class State
  {
    ABSENT,
    STARTING,
    READY,
    FAILED,
  };

  struct ViewState
  {
    std::string profile;
    std::string url;
    std::string title;
    bool canGoBack{false};
    bool canGoForward{false};
  };

  CWebClientManager() = default;
  ~CWebClientManager() override;

  void Start();
  void Stop();

  bool IsEnabled() const { return m_host != nullptr; }
  State GetState() const;
  WAYLANDHOST::CWaylandHost* GetHost() const { return m_host.get(); }

  /*!
   * \brief Ask for the view of a profile on behalf of an observer; the
   * answer comes through the observer, at once if the view exists
   */
  bool RequestView(const std::string& profile, IWebViewObserver& observer);

  /*!
   * \brief The observer is done with its view, which closes; returns once no
   * call into the observer runs any more, so never from such a call
   */
  void ReleaseView(IWebViewObserver& observer);

  /*!
   * \brief What the view a player asked for reports; false without one
   */
  bool GetShownView(ViewState& state) const;

  /*!
   * \brief Whether the running client shows pages' video on a display plane
   */
  bool ShowsVideoOnPlane() const;

  /*!
   * \brief Send the frames of pages' video to the observer, none with nullptr
   */
  void SetVideoObserver(IWebVideoObserver* observer);

  /*!
   * \brief The shared content filter lists a web app uses, fetched now if
   * they are not yet
   */
  std::set<std::string> WantFilterLists(const std::string& profile);
  CWebFilterLists::ListState FilterListState(const std::string& name) const;

  /*!
   * \brief Set whenever a filter list's state changes
   */
  CEvent& FilterListsChanged() { return m_filterListsChanged; }

  /*!
   * \brief Tell the user that a web app cannot open without a filter list
   */
  void NotifyFilterListMissing(const std::string& profile, const std::string& list);

  void LoadUrl(const std::string& profile, const std::string& url);
  void Navigate(const std::string& profile, WAYLANDHOST::WebNavigation action);
  void SetActive(const std::string& profile, bool active);

  // implementation of IRenderLoop
  void FrameMove() override;

  // implementation of IDispResource
  void OnResetDisplay() override;

  // implementation of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  // implementation of IWaylandHostObserver
  void OnSurfaceMapped(WAYLANDHOST::CWaylandSurface& surface) override;
  void OnSurfaceUnmapped(WAYLANDHOST::CWaylandSurface& surface) override;
  void OnSurfaceDestroyed(WAYLANDHOST::CWaylandSurface& surface) override;
  void OnSurfaceFrame(WAYLANDHOST::CWaylandSurface& surface,
                      std::shared_ptr<WAYLANDHOST::CWaylandBuffer> buffer) override;
  void OnClientExited(pid_t pid, int status) override;
  void OnWebClientReady() override;
  void OnWebViewCreated(const std::string& profile, WAYLANDHOST::CWaylandSurface& surface) override;
  void OnWebViewUnavailable(const std::string& profile, const std::string& message) override;
  void OnWebFilterListPrepared(const std::string& name, bool prepared) override;
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
  void OnWebViewDestroyed(const std::string& profile) override;
  void OnTextInputEnabled(const WAYLANDHOST::TextInputState& state) override;
  void OnTextInputUpdated(const WAYLANDHOST::TextInputState& state) override;
  void OnTextInputDisabled() override;

  // the keyboard dialog's life, off the host thread
  void OnKeyboardOpening(int coveredTop);
  void OnKeyboardText(const std::string& text);
  void OnKeyboardDone();

private:
  // a call into the observer in progress, counted for ReleaseView(); made
  // under m_lock
  class CObserverCall
  {
  public:
    CObserverCall(CWebClientManager& manager, IWebViewObserver* observer);
    ~CObserverCall();
    CObserverCall(const CObserverCall&) = delete;
    CObserverCall& operator=(const CObserverCall&) = delete;

    explicit operator bool() const { return m_observer != nullptr; }
    IWebViewObserver* operator->() const { return m_observer; }

  private:
    CWebClientManager& m_manager;
    IWebViewObserver* m_observer;
  };

  void OnAddonEvent(const ADDON::AddonEvent& event);
  bool StartHost();
  void UpdateVideo();
  void Spawn();
  void SpawnIfPreloaded();
  void RestartClientForVideo();
  void ConfigureView(const std::string& profile);
  bool ConfigureViewFromAddon(const std::string& profile);
  std::string UplinkOf(const std::string& profile) const;
  void SendSoftwareVideoDecoding(const std::string& profile);
  void SendProfileFiles(const std::string& profile,
                        const std::string& path,
                        const std::string& extension,
                        bool scripts);
  void SendProfileFile(const std::string& profile,
                       const std::string& file,
                       const std::string& name,
                       bool script);
  CObserverCall ObserverOf(const std::string& profile);
  void CloseView(const std::string& profile);
  void EnsureCaretVisible();

  std::unique_ptr<WAYLANDHOST::CWaylandHost> m_host;
  std::unique_ptr<CWebAudioServer> m_audio;
  std::unique_ptr<CWebVideoServer> m_video;
  IWebVideoObserver* m_videoObserver{nullptr};
  std::unique_ptr<CWebFilterLists> m_filterLists;
  CEvent m_filterListsChanged;
  mutable CCriticalSection m_lock;
  State m_state{State::ABSENT};
  bool m_respawned{false};
  bool m_restarting{false};
  bool m_restartWhenIdle{false};
  IWebViewObserver* m_observer{nullptr};
  int m_observerCalls{0};
  XbmcThreads::ConditionVariable m_observerCallsDone;
  std::string m_observedProfile;
  ViewState m_shown;
  bool m_wantPending{false};
  std::map<std::string, WAYLANDHOST::CWaylandSurface*> m_views;
  std::map<WAYLANDHOST::CWaylandSurface*, std::string> m_surfaces;
  std::map<std::string, std::string> m_uplinks;
  std::set<std::string> m_closing;
  bool m_keyboardOpen{false};
  bool m_textInputActive{false};
  WAYLANDHOST::TextInputState m_textInput;
  std::string m_keyboardText;
  uint32_t m_replaceBefore{0};
  uint32_t m_replaceAfter{0};
  int32_t m_coveredTop{0};
  int32_t m_scrolledCaretY{0};
};

/*!
 * \brief Shows Kodi's keyboard dialog for a page's text input on a job
 * thread, so neither the host nor the GUI thread waits on it
 */
class CWebKeyboardJob : public CJob
{
public:
  CWebKeyboardJob(CWebClientManager& manager, std::string text, bool hidden)
    : m_manager(manager),
      m_text(std::move(text)),
      m_hidden(hidden)
  {
  }

  bool DoWork() override;
  const char* GetType() const override { return "webkeyboard"; }

private:
  CWebClientManager& m_manager;
  std::string m_text;
  const bool m_hidden;
};

} // namespace WEB
} // namespace KODI
