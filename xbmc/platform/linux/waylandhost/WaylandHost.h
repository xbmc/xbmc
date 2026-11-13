/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <cstdint>
#include <ctime>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <sys/types.h>

struct wl_client;
struct wl_display;
struct wl_event_loop;
struct wl_event_source;
struct wl_global;
struct wl_resource;
struct xkb_context;
struct xkb_keymap;
struct xkb_state;

namespace KODI
{
namespace WAYLANDHOST
{

class CWaylandBuffer;
class CWaylandSurface;

/*!
 * \brief What the focused page tells about its text input: the text around
 * the caret with the caret's byte offset, the content type and the caret
 * rectangle in surface coordinates
 */
struct TextInputState
{
  bool enabled{false};
  std::string surrounding;
  int32_t cursor{0};
  uint32_t hint{0};
  uint32_t purpose{0};
  int32_t caretX{0};
  int32_t caretY{0};
  int32_t caretWidth{0};
  int32_t caretHeight{0};

  bool IsHidden() const;
};

/*!
 * \brief Receives what the clients do. All methods run on the host thread.
 */
class IWaylandHostObserver
{
public:
  virtual ~IWaylandHostObserver() = default;

  virtual void OnSurfaceMapped(CWaylandSurface& surface) = 0;
  virtual void OnSurfaceUnmapped(CWaylandSurface& surface) = 0;
  virtual void OnSurfaceDestroyed(CWaylandSurface& surface) = 0;

  /*!
   * \brief The surface shows new content; the buffer is locked for the
   * observer, which unlocks it when it no longer needs the pixels
   */
  virtual void OnSurfaceFrame(CWaylandSurface& surface, std::shared_ptr<CWaylandBuffer> buffer) = 0;

  /*!
   * \brief A client ended, or could not be started (pid -1); status is -1
   * when it is unknown
   */
  virtual void OnClientExited(pid_t pid, int status) = 0;

  // kodi_web_v1, what the web client reports; an observer may ignore any of it
  virtual void OnWebClientReady() {}
  virtual void OnWebViewCreated(const std::string& profile, CWaylandSurface& surface) {}
  virtual void OnWebViewUnavailable(const std::string& profile, const std::string& message) {}
  virtual void OnWebFilterListPrepared(const std::string& name, bool prepared) {}
  virtual void OnWebViewUrl(const std::string& profile, const std::string& url) {}
  virtual void OnWebViewTitle(const std::string& profile, const std::string& title) {}
  virtual void OnWebViewProgress(const std::string& profile, uint32_t percent) {}
  virtual void OnWebViewNavigationState(const std::string& profile,
                                        bool canGoBack,
                                        bool canGoForward)
  {
  }
  virtual void OnWebViewError(const std::string& profile, uint32_t code, const std::string& message)
  {
  }
  virtual void OnWebViewExit(const std::string& profile) {}
  virtual void OnWebViewMediaState(const std::string& profile, bool video, bool audio, bool paused)
  {
  }
  virtual void OnWebViewDestroyed(const std::string& profile) {}

  // zwp_text_input_v3: the focused page asks for an on-screen keyboard,
  // then reports its text and caret after every change
  virtual void OnTextInputEnabled(const TextInputState& state) {}
  virtual void OnTextInputUpdated(const TextInputState& state) {}
  virtual void OnTextInputDisabled() {}
};

enum class WebNavigation
{
  BACK,
  FORWARD,
  RELOAD,
  STOP,
};

enum class WebViewError
{
  LOAD_FAILED,
  TLS_FAILED,
  ENGINE_CRASHED,
  UNRESPONSIVE,
  CONFIG_FAILED,
};

struct WaylandHostConfig
{
  int32_t width{0};
  int32_t height{0};
  int32_t refreshMilliHz{0};
  // the screen's size in millimetres, 0 when not known
  int32_t physicalWidth{0};
  int32_t physicalHeight{0};
  std::string renderDevicePath;
  std::string keyboardLayout;
};

/*!
 * \brief Functions posted from any thread for the host thread to run. A
 * consumer may hold the queue beyond the host's life; once the host has
 * closed it, posted functions are dropped.
 */
class CWaylandTaskQueue
{
public:
  CWaylandTaskQueue() = default;
  ~CWaylandTaskQueue();

  CWaylandTaskQueue(const CWaylandTaskQueue&) = delete;
  CWaylandTaskQueue& operator=(const CWaylandTaskQueue&) = delete;

  /*!
   * \brief Accept functions until Close(); returns an eventfd that becomes
   * readable when one is posted, or -1
   */
  int Open();
  void Close();

  // any thread
  void Post(std::function<void()> task);

  // host thread only
  void Run();

private:
  CCriticalSection m_lock;
  std::list<std::function<void()>> m_tasks;
  int m_wakeFd{-1};
};

/*!
 * \brief A Wayland compositor without a screen of its own: clients connect
 * through socket pairs handed to processes the host spawns, their buffers
 * are passed to the observer, and input comes from Kodi.
 */
class CWaylandHost : private CThread
{
public:
  CWaylandHost(IWaylandHostObserver& observer, WaylandHostConfig config);
  ~CWaylandHost() override;

  bool Start();
  void Stop();

  /*!
   * \brief Run a function on the host thread; any thread
   */
  void Post(std::function<void()> task);

  /*!
   * \brief Start a client process connected to this host; any thread
   */
  void SpawnClient(std::vector<std::string> argv, std::map<std::string, std::string> env);

  /*!
   * \brief Ask every client process to terminate; any thread
   */
  void TerminateClients();

  /*!
   * \brief Tell the clients the output's new mode and resize their surfaces
   * to it; any thread
   */
  void SetOutputMode(int32_t width, int32_t height, int32_t refreshMilliHz);

  // input injection; any thread
  void KeyboardKey(uint32_t evdevCode, bool pressed);
  void PointerMotion(int32_t x, int32_t y);
  void PointerButton(uint32_t evdevButton, bool pressed);
  void PointerAxis(int32_t value120);

  /*!
   * \brief Scroll the content under a point of the focused surface by a
   * number of pixels, as a touchpad would; any thread
   */
  void PointerScroll(int32_t x, int32_t y, int32_t pixels);

  /*!
   * \brief Touch points of the focused surface in surface coordinates; a
   * touch stays with the surface it went down on until its last point is
   * up; TouchFrame() ends a set of changes; any thread
   */
  void TouchDown(int32_t id, int32_t x, int32_t y);
  void TouchMotion(int32_t id, int32_t x, int32_t y);
  void TouchUp(int32_t id);
  void TouchFrame();
  void TouchCancel();

  /*!
   * \brief A display refresh has passed; any thread
   */
  void FrameDone();

  // kodi_web_v1, what Kodi asks of the web client; any thread
  void WebWantView(std::string profile, std::string uplink);
  void WebViewLoadUrl(std::string profile, std::string url);
  void WebViewNavigate(std::string profile, WebNavigation action);
  void WebViewSetActive(std::string profile, bool active);
  void WebViewClose(std::string profile);
  void WebViewSetUserAgent(std::string profile, std::string userAgent);
  void WebViewSetViewport(std::string profile, uint32_t width, uint32_t height);
  void WebViewSetSoftwareVideoDecoding(std::string profile, bool allowed);

  /*!
   * \brief Replace the text around the cursor of the enabled text input;
   * any thread
   */
  void TextInputCommit(std::string text, uint32_t deleteBefore, uint32_t deleteAfter);

  /*!
   * \brief Hand the client a script or filter; the host owns the descriptor
   * from here on and closes it once sent
   */
  void WebViewAddUserScript(std::string profile, std::string name, int fd, uint32_t size);
  void WebViewAddContentFilter(std::string profile, std::string name, int fd, uint32_t size);

  /*!
   * \brief Apply a shared content filter list to a view; any thread
   */
  void WebViewAddContentFilterList(std::string profile, std::string name);

  /*!
   * \brief Keep a shared content filter list, in Adblock Plus syntax, for
   * the web client, which gets it now and whenever it binds again; an empty
   * path drops the list; any thread
   */
  void WebSetContentFilterList(std::string name, std::string path);

  /*!
   * \brief Tell the web client the direction the user's language is written
   * in, now and whenever it binds again; any thread
   */
  void WebSetLayoutDirection(bool rightToLeft);

  static bool IsValidWebProfile(const std::string& profile);

  struct WebView
  {
    CWaylandHost* host{nullptr};
    wl_resource* resource{nullptr};
    std::string profile;
    CWaylandSurface* surface{nullptr};
    bool active{false};
  };

  struct TextInput
  {
    CWaylandHost* host{nullptr};
    wl_resource* resource{nullptr};
    TextInputState current;
    TextInputState pending;
    uint32_t serial{0};
    uint32_t enableRequests{0};
    bool keyboardShown{false};
  };

  // host thread only
  wl_display* GetDisplay() const { return m_display; }
  void OnWebManagerBound(wl_resource* manager);
  void OnWebManagerUnbound(wl_resource* manager);
  void OnWebClientReady();
  void OnWebViewCreate(wl_client* client,
                       wl_resource* manager,
                       uint32_t id,
                       const char* profile,
                       wl_resource* surfaceResource);
  void OnWebViewUnavailable(const char* profile, const char* message);
  void OnWebFilterListPrepared(const char* name, bool prepared);
  void OnWebViewUrl(WebView& view, const char* url);
  void OnWebViewTitle(WebView& view, const char* title);
  void OnWebViewFavicon(WebView& view, wl_resource* buffer);
  void OnWebViewProgress(WebView& view, uint32_t percent);
  void OnWebViewNavigationState(WebView& view, uint32_t canGoBack, uint32_t canGoForward);
  void OnWebViewError(WebView& view, uint32_t code, const char* message);
  void OnWebViewExit(WebView& view);
  void OnWebViewMediaState(WebView& view, bool video, bool audio, bool paused);
  void OnWebViewResourceDestroyed(WebView& view);
  void AddTextInput(wl_client* client, uint32_t version, uint32_t id);
  void OnTextInputEnable(TextInput& input, bool enable);
  void OnTextInputSurroundingText(TextInput& input, const char* text, int32_t cursor);
  void OnTextInputContentType(TextInput& input, uint32_t hint, uint32_t purpose);
  void OnTextInputCursorRectangle(
      TextInput& input, int32_t x, int32_t y, int32_t width, int32_t height);
  void OnTextInputCommit(TextInput& input);
  void OnTextInputResourceDestroyed(TextInput& input);
  const WaylandHostConfig& GetConfig() const { return m_config; }
  const std::shared_ptr<CWaylandTaskQueue>& GetTaskQueue() const { return m_tasks; }
  uint32_t NextSerial();
  void OnSurfaceCreated(CWaylandSurface* surface);
  void OnSurfaceDestroyed(CWaylandSurface* surface);
  void OnSurfaceMapped(CWaylandSurface& surface);
  void OnSurfaceUnmapped(CWaylandSurface& surface);
  void OnSurfaceReleased(CWaylandSurface& surface);
  void OnSurfaceFrame(CWaylandSurface& surface, std::shared_ptr<CWaylandBuffer> buffer);
  void SendDmabufFeedback(wl_resource* feedback);
  void AddKeyboard(wl_client* client, uint32_t version, uint32_t id);
  void RemoveKeyboard(wl_resource* keyboard);
  void AddPointer(wl_client* client, uint32_t version, uint32_t id);
  void RemovePointer(wl_resource* pointer);
  void AddTouch(wl_client* client, uint32_t version, uint32_t id);
  void RemoveTouch(wl_resource* touch);

protected:
  void Process() override;

private:
  struct Client
  {
    CWaylandHost* host{nullptr};
    pid_t pid{-1};
    int pidfd{-1};
    wl_client* client{nullptr};
    wl_event_source* exitSource{nullptr};
  };

  bool CreateGlobals();
  bool CreateKeymap();
  bool CreateFormatTable();
  static int SealedMemfd(const char* name, const void* data, size_t size);
  void DestroyAll();
  void DoSpawnClient(const std::vector<std::string>& argv,
                     const std::map<std::string, std::string>& env);
  void DoTerminateClients();
  static void ReapClient(const Client& client);
  void DoKeyboardKey(uint32_t evdevCode, bool pressed);
  void DoPointerMotion(int32_t x, int32_t y);
  void DoPointerButton(uint32_t evdevButton, bool pressed);
  void DoPointerAxis(int32_t value120);
  void DoPointerScroll(int32_t x, int32_t y, int32_t pixels);
  void DoTouchDown(int32_t id, int32_t x, int32_t y);
  void DoTouchMotion(int32_t id, int32_t x, int32_t y);
  void DoTouchUp(int32_t id);
  void DoTouchFrame();
  void DoTouchCancel();
  void DoFrameDone();
  void DoSetOutputMode(int32_t width, int32_t height, int32_t refreshMilliHz);
  void SendOutputMode(wl_resource* output) const;
  void SetKeyboardFocus(CWaylandSurface* surface);
  void SendKeyboardEnter(CWaylandSurface& surface);
  void SendKeyboardEnterTo(wl_resource* keyboard, CWaylandSurface& surface);
  void SendKeyboardLeave(CWaylandSurface& surface);
  void SendPointerEnter(CWaylandSurface& surface);
  void SendPointerEnterTo(wl_resource* pointer, CWaylandSurface& surface);
  void SendPointerLeave();
  std::vector<wl_resource*> TouchesOfFocus() const;
  void SendModifiers();
  void OnClientExit(Client& client);

  static int OnWake(int fd, uint32_t mask, void* data);
  static int OnClientExitStatic(int fd, uint32_t mask, void* data);

  // globals
  static void BindCompositor(wl_client* client, void* data, uint32_t version, uint32_t id);
  static void BindOutput(wl_client* client, void* data, uint32_t version, uint32_t id);
  static void OutputDestroyed(wl_resource* resource);
  static void BindSeat(wl_client* client, void* data, uint32_t version, uint32_t id);
  static void BindXdgWmBase(wl_client* client, void* data, uint32_t version, uint32_t id);
  static void BindLinuxDmabuf(wl_client* client, void* data, uint32_t version, uint32_t id);
  static void BindPresentation(wl_client* client, void* data, uint32_t version, uint32_t id);
  static void BindTextInputManager(wl_client* client, void* data, uint32_t version, uint32_t id);
  static void BindWebManager(wl_client* client, void* data, uint32_t version, uint32_t id);

  WebView* FindWebView(const std::string& profile);
  WebView* WebViewOf(const CWaylandSurface* surface);
  void DoWebViewSetActive(WebView& view, bool active);
  void SendTextInputFocus(CWaylandSurface* surface, bool entered);
  void SendWebViewFile(
      const std::string& profile, const std::string& name, int fd, uint32_t size, bool script);
  void SendContentFilterList(const std::string& name, const std::string& path);
  void SendLayoutDirection();

  IWaylandHostObserver& m_observer;
  // changed only on the host thread
  WaylandHostConfig m_config;

  wl_display* m_display{nullptr};
  wl_event_loop* m_loop{nullptr};
  wl_event_source* m_wakeSource{nullptr};
  std::vector<wl_global*> m_globals;

  const std::shared_ptr<CWaylandTaskQueue> m_tasks{std::make_shared<CWaylandTaskQueue>()};

  std::list<Client> m_clients;
  std::list<CWaylandSurface*> m_surfaces;
  wl_resource* m_webManager{nullptr};
  std::list<WebView> m_views;
  std::map<std::string, std::string> m_filterLists;
  bool m_rightToLeft{false};
  std::list<TextInput> m_textInputs;
  std::list<wl_resource*> m_outputs;
  std::list<wl_resource*> m_keyboards;
  std::list<wl_resource*> m_pointers;
  std::list<wl_resource*> m_touches;
  CWaylandSurface* m_keyboardFocus{nullptr};
  CWaylandSurface* m_pointerFocus{nullptr};
  int32_t m_pointerX{0};
  int32_t m_pointerY{0};
  CWaylandSurface* m_touchFocus{nullptr};
  std::set<int32_t> m_touchIds;

  xkb_context* m_xkbContext{nullptr};
  xkb_keymap* m_xkbKeymap{nullptr};
  xkb_state* m_xkbState{nullptr};
  int m_keymapFd{-1};
  size_t m_keymapSize{0};
  uint32_t m_modsDepressed{0};
  uint32_t m_modsLatched{0};
  uint32_t m_modsLocked{0};
  uint32_t m_modsGroup{0};

  int m_formatTableFd{-1};
  size_t m_formatTableSize{0};
  dev_t m_renderDevice{0};

  uint32_t m_serial{0};
};

} // namespace WAYLANDHOST
} // namespace KODI
