/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebClientManager.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIDialogWebKeyboard.h"
#include "ServiceBroker.h"
#include "WebApp.h"
#include "addons/Addon.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/WebApp.h"
#include "addons/addoninfo/AddonType.h"
#include "application/ApplicationComponents.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "jobs/JobManager.h"
#include "kodi-web-audio.h"
#include "kodi-web-video.h"
#include "language/LangInfo.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include "platform/linux/waylandhost/WaylandBuffer.h"
#include "platform/linux/waylandhost/WaylandSurface.h"

#if defined(HAVE_GBM)
#include "windowing/gbm/WinSystemGbm.h"
#include "windowing/gbm/drm/DRMAtomic.h"
#include "windowing/gbm/drm/DRMUtils.h"

#include "platform/linux/input/LibInputSettings.h"
#endif

#if defined(HAVE_GBM) && defined(HAS_GLES) && defined(HAVE_LINUX_DMA_HEAP)
#include "utils/DMAHeapBufferObject.h"

#include <drm_fourcc.h>
#include <xf86drmMode.h>
#endif

#if defined(HAVE_WAYLAND)
#include "windowing/wayland/WinSystemWayland.h"
#endif

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <mutex>
#include <tuple>
#include <typeinfo>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xf86drm.h>

using namespace KODI::WEB;
using namespace KODI::WAYLANDHOST;

namespace
{

constexpr const char* PROFILE_ROOT = "special://profile/webprofiles/";
constexpr const char* AUDIO_SOCKET = "special://temp/webaudio.sock";
constexpr const char* VIDEO_SOCKET = "special://temp/webvideo.sock";
constexpr const char* CACHE_TEMP = "special://temp/webcache";
constexpr const char* CACHE_KEPT = "special://profile/webcache";

void OnWebKeyboardText(CGUIKeyboard*, const std::string& text)
{
  if (auto manager = CServiceBroker::GetAppComponents().GetComponent<CWebClientManager>())
    manager->OnKeyboardText(text);
}

bool HasPhysicalKeyboard()
{
#if defined(HAVE_GBM)
  if (auto* gbm =
          dynamic_cast<KODI::WINDOWING::GBM::CWinSystemGbm*>(CServiceBroker::GetWinSystem()))
    return gbm->HasKeyboard();
#endif
  return false;
}

// web frames are DRM PRIME buffers, which only the GBM and Wayland renderers take
bool WindowingShowsWebFrames()
{
#if defined(HAVE_GBM)
  if (dynamic_cast<KODI::WINDOWING::GBM::CWinSystemGbm*>(CServiceBroker::GetWinSystem()))
    return true;
#endif
#if defined(HAVE_WAYLAND)
  if (dynamic_cast<KODI::WINDOWING::WAYLAND::CWinSystemWayland*>(CServiceBroker::GetWinSystem()))
    return true;
#endif
  return false;
}

std::string FirstRenderNode()
{
  std::string path;
  drmDevicePtr devices[16] = {};
  const int count = drmGetDevices2(0, devices, std::size(devices));
  for (int i = 0; i < count && path.empty(); ++i)
  {
    if (devices[i]->available_nodes & (1 << DRM_NODE_RENDER))
      path = devices[i]->nodes[DRM_NODE_RENDER];
  }
  if (count > 0)
    drmFreeDevices(devices, count);
  return path;
}

std::string CdmDirectory()
{
  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon("inputstream.adaptive", addon,
                                              ADDON::AddonType::INPUTSTREAM,
                                              ADDON::OnlyEnabled::CHOICE_YES))
    return {};

  const std::string path = addon->GetSetting("DECRYPTERPATH");
  return path.empty() ? path : CSpecialProtocol::TranslatePath(path);
}

void NotifyOpenError(const std::string& profile, const std::string& reason)
{
  ADDON::AddonPtr addon;
  const std::string name =
      CServiceBroker::GetAddonMgr().GetAddon(profile, addon, ADDON::OnlyEnabled::CHOICE_YES)
          ? addon->Name()
          : profile;
  const auto& localizeStrings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
  // "Error opening {0:s}"
  CGUIDialogKaiToast::QueueNotification(
      CGUIDialogKaiToast::Error, StringUtils::Format(localizeStrings.Get(13329), name), reason);
}

bool WebAppSettingsLoad(const std::string& profile)
{
  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(profile, addon, ADDON::AddonType::WEBAPP,
                                              ADDON::OnlyEnabled::CHOICE_YES))
    return true;

  const auto webapp = std::dynamic_pointer_cast<ADDON::CWebApp>(addon);
  return !webapp || !webapp->LeavesKodiSettingsToKodi() || webapp->HasSettings();
}

const ADDON::CAddonType* WebAppType(const std::shared_ptr<ADDON::CAddon>& webapp)
{
  return webapp ? webapp->Type(ADDON::AddonType::WEBAPP) : nullptr;
}

void AddFilterListsOf(const ADDON::AddonPtr& addon, std::set<std::string>& lists)
{
  const ADDON::CAddonType* type = WebAppType(std::dynamic_pointer_cast<ADDON::CAddon>(addon));
  if (!type)
    return;

  for (const auto& [element, values] : type->GetValues())
  {
    const std::string list = values.GetValue("contentfilter@list").asString();
    if (element == "contentfilter" && !list.empty())
      lists.insert(list);
  }
}

std::set<std::string> FilterListsOfInstalledWebApps()
{
  std::set<std::string> lists;
  ADDON::VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetInstalledAddons(addons, ADDON::AddonType::WEBAPP);
  for (const auto& addon : addons)
    AddFilterListsOf(addon, lists);
  return lists;
}

std::set<std::string> FilterListsOfWebApp(const std::string& id)
{
  std::set<std::string> lists;
  ADDON::AddonPtr addon;
  if (CServiceBroker::GetAddonMgr().GetAddon(id, addon, ADDON::AddonType::WEBAPP,
                                             ADDON::OnlyEnabled::CHOICE_YES))
    AddFilterListsOf(addon, lists);
  return lists;
}

// the command is run as execvpe() runs it: a name with a slash as a path,
// any other through PATH
bool ClientCommandExists(const std::string& command)
{
  if (command.empty())
    return false;
  if (command.find('/') != std::string::npos)
    return access(command.c_str(), X_OK) == 0;

  // without PATH, execvpe() searches a default of the C library's own
  const char* path = getenv("PATH");
  if (!path)
    return true;
  for (const auto& dir : StringUtils::Split(path, ':'))
  {
    if (!dir.empty() && access((dir + "/" + command).c_str(), X_OK) == 0)
      return true;
  }
  return false;
}

bool WebAppsInstalled()
{
  ADDON::VECADDONS addons;
  return CServiceBroker::GetAddonMgr().GetInstalledAddons(addons, ADDON::AddonType::WEBAPP) &&
         !addons.empty();
}

// pages show their video on a display plane where Kodi shows its own video on one
bool VideoOnPlane()
{
  constexpr int PRIME_RENDER_DIRECT = 0;
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
             CSettings::SETTING_VIDEOPLAYER_USEPRIMERENDERER) == PRIME_RENDER_DIRECT;
}

#if defined(HAVE_GBM) && defined(HAS_GLES) && defined(HAVE_LINUX_DMA_HEAP)
constexpr const char* DMA_HEAP_DIR = "/dev/dma_heap/";
constexpr const char* SYSTEM_DMA_HEAP = "system";

// the system heap first, which leaves contiguous memory to the devices that need it
std::vector<std::string> DmaHeaps()
{
  CFileItemList items;
  std::vector<std::string> heaps;
  if (!XFILE::CDirectory::GetDirectory(DMA_HEAP_DIR, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS))
    return heaps;

  for (const auto& item : items)
    heaps.push_back(URIUtils::GetFileName(item->GetPath()));
  std::ranges::sort(heaps, {}, [](const std::string& heap)
                    { return std::make_pair(heap != SYSTEM_DMA_HEAP, heap); });
  return heaps;
}

// a screen-sized NV12 frame, as web video brings: a smaller one may be contiguous from any heap
bool HeapScansOut(int drmFd, const std::string& heap, uint32_t width, uint32_t height)
{
  CDMAHeapBufferObject buffer(DMA_HEAP_DIR + heap);
  const uint32_t lumaSize = width * height;
  if (!buffer.CreateBufferObject(static_cast<uint64_t>(lumaSize) + lumaSize / 2))
    return false;

  uint32_t handle = 0;
  if (drmPrimeFDToHandle(drmFd, buffer.GetFd(), &handle) < 0)
  {
    CLog::Log(LOGDEBUG, "CWebClientManager::{}: cannot import from {}: {}", __FUNCTION__, heap,
              strerror(errno));
    return false;
  }

  const uint32_t handles[4] = {handle, handle};
  const uint32_t pitches[4] = {width, width};
  const uint32_t offsets[4] = {0, lumaSize};
  uint32_t fbId = 0;
  const bool added = drmModeAddFB2(drmFd, width, height, DRM_FORMAT_NV12, handles, pitches, offsets,
                                   &fbId, 0) == 0;
  if (added)
    drmModeRmFB(drmFd, fbId);
  else
    CLog::Log(LOGDEBUG, "CWebClientManager::{}: no framebuffer from {}: {}", __FUNCTION__, heap,
              strerror(errno));

  drm_gem_close close{};
  close.handle = handle;
  drmIoctl(drmFd, DRM_IOCTL_GEM_CLOSE, &close);
  return added;
}

std::string ScanoutDmaHeap(int drmFd, uint32_t width, uint32_t height)
{
  for (const std::string& heap : DmaHeaps())
  {
    if (HeapScansOut(drmFd, heap, width, height))
      return heap;
  }
  return {};
}
#endif

// web apps run at the screen's resolution, whatever size the GUI renders at
WaylandHostConfig OutputMode()
{
  CGraphicContext& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  const RESOLUTION_INFO res = gfx.GetResInfo();
  WaylandHostConfig mode;
  mode.width = res.iScreenWidth;
  mode.height = res.iScreenHeight;
  mode.refreshMilliHz = static_cast<int32_t>(std::lround(gfx.GetFPS() * 1000.0f));
  return mode;
}

} // unnamed namespace

CWebClientManager::~CWebClientManager()
{
  Stop();
}

void CWebClientManager::Start()
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (!settings->GetBool(CSettings::SETTING_WEB_ENABLED) || m_host)
    return;

  if (!WindowingShowsWebFrames())
  {
    CLog::Log(LOGINFO, "CWebClientManager::{}: web apps need the GBM or Wayland windowing",
              __FUNCTION__);
    return;
  }

  if (!StartHost())
    return;

  m_audio = std::make_unique<CWebAudioServer>();
  if (!m_audio->Start(CSpecialProtocol::TranslatePath(AUDIO_SOCKET)))
    m_audio.reset();

  m_filterLists = std::make_unique<CWebFilterLists>(*m_host, m_filterListsChanged);
  m_filterLists->Start(FilterListsOfInstalledWebApps());
  // a web app's lists are fetched when it is installed, not when it opens
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, [this](const ADDON::AddonEvent& event)
                                                   { OnAddonEvent(event); });

  CServiceBroker::GetWinSystem()->RegisterRenderLoop(this);
  CServiceBroker::GetWinSystem()->Register(this);
  settings->RegisterCallback(
      this, {CSettings::SETTING_VIDEOPLAYER_USEPRIMERENDERER, CSettings::SETTING_LOCALE_LANGUAGE});
  SpawnIfPreloaded();
}

void CWebClientManager::SpawnIfPreloaded()
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  // a preloaded client is wasted memory where no web app could use it
  if (settings->GetBool(CSettings::SETTING_WEB_PRELOAD) && WebAppsInstalled() &&
      ClientCommandExists(settings->GetString(CSettings::SETTING_WEB_CLIENTCOMMAND)))
    Spawn();
}

void CWebClientManager::Stop()
{
  if (!m_host)
    return;

  if (const auto settingsComponent = CServiceBroker::GetSettingsComponent())
    settingsComponent->GetSettings()->UnregisterCallback(this);
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  CServiceBroker::GetWinSystem()->Unregister(this);
  CServiceBroker::GetWinSystem()->UnregisterRenderLoop(this);
  std::unique_lock lock(m_lock);
  std::unique_ptr<CWebFilterLists> filterLists = std::move(m_filterLists);
  lock.unlock();
  // destroyed while the host still runs: its fetch thread posts to the host
  filterLists.reset();
  m_host->Stop();
  m_host.reset();
  m_audio.reset();
  m_video.reset();

  lock.lock();
  m_state = State::ABSENT;
  m_views.clear();
  m_surfaces.clear();
  m_closing.clear();
  m_observer = nullptr;
}

void CWebClientManager::OnAddonEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) != typeid(ADDON::AddonEvents::ReInstalled) &&
      typeid(event) != typeid(ADDON::AddonEvents::Enabled))
    return;

  const std::set<std::string> lists = FilterListsOfWebApp(event.addonId);
  std::unique_lock lock(m_lock);
  if (!m_filterLists)
    return;
  for (const auto& list : lists)
    m_filterLists->Want(list);
}

std::set<std::string> CWebClientManager::WantFilterLists(const std::string& profile)
{
  const std::set<std::string> lists = FilterListsOfWebApp(profile);
  std::unique_lock lock(m_lock);
  if (m_filterLists)
  {
    for (const auto& list : lists)
      m_filterLists->Want(list);
  }
  return lists;
}

CWebFilterLists::ListState CWebClientManager::FilterListState(const std::string& name) const
{
  std::unique_lock lock(m_lock);
  return m_filterLists ? m_filterLists->State(name) : CWebFilterLists::ListState::UNKNOWN;
}

void CWebClientManager::NotifyFilterListMissing(const std::string& profile, const std::string& list)
{
  const auto& localizeStrings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
  NotifyOpenError(profile, StringUtils::Format(localizeStrings.Get(40253), list));
}

void CWebClientManager::OnWebFilterListPrepared(const std::string& name, bool prepared)
{
  std::unique_lock lock(m_lock);
  if (m_filterLists)
    m_filterLists->OnPrepared(name, prepared);
}

// the client finds a video socket only while pages' video goes to a display plane
void CWebClientManager::UpdateVideo()
{
  if (!VideoOnPlane())
  {
    m_video.reset();
    return;
  }
  if (m_video)
    return;

#if defined(HAVE_GBM) && defined(HAS_GLES)
  // display planes take part in atomic commits only
  auto* gbm = dynamic_cast<WINDOWING::GBM::CWinSystemGbm*>(CServiceBroker::GetWinSystem());
  if (!gbm || !std::dynamic_pointer_cast<WINDOWING::GBM::CDRMAtomic>(gbm->GetDrm()))
    return;

  std::function<std::string()> dmaHeap;
#if defined(HAVE_LINUX_DMA_HEAP)
  const RESOLUTION_INFO mode = gbm->GetDrm()->GetCurrentMode();
  dmaHeap = [drm = gbm->GetDrm(), width = static_cast<uint32_t>(mode.iScreenWidth),
             height = static_cast<uint32_t>(mode.iScreenHeight)]
  { return ScanoutDmaHeap(drm->GetFileDescriptor(), width, height); };
#endif

  m_video = std::make_unique<CWebVideoServer>();
  if (!m_video->Start(CSpecialProtocol::TranslatePath(VIDEO_SOCKET),
                      gbm->GetDrm()->GetVideoFormats(), std::move(dmaHeap)))
  {
    m_video.reset();
    return;
  }
  m_video->SetObserver(m_videoObserver);
#endif
}

bool CWebClientManager::ShowsVideoOnPlane() const
{
  std::unique_lock lock(m_lock);
  return m_video != nullptr;
}

void CWebClientManager::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  // CLangInfo may learn of the new language after this callback, so ask for that language
  if (setting->GetId() == CSettings::SETTING_LOCALE_LANGUAGE)
  {
    const std::string& language =
        std::static_pointer_cast<const CSettingString>(setting)->GetValue();
    std::unique_lock lock(m_lock);
    if (m_host)
      m_host->WebSetLayoutDirection(g_langInfo.IsRightToLeft(language));
    return;
  }

  if (setting->GetId() != CSettings::SETTING_VIDEOPLAYER_USEPRIMERENDERER)
    return;

  std::unique_lock lock(m_lock);
  // a client not running picks the setting up when it starts
  if (!m_host || (m_state != State::READY && m_state != State::STARTING) ||
      (m_video != nullptr) == VideoOnPlane())
    return;

  if (m_observer)
  {
    m_restartWhenIdle = true;
    return;
  }
  RestartClientForVideo();
}

// m_lock held
void CWebClientManager::RestartClientForVideo()
{
  CLog::Log(LOGINFO, "CWebClientManager::{}: restarting the web client for the video setting",
            __FUNCTION__);
  m_restartWhenIdle = false;
  m_restarting = true;
  m_host->TerminateClients();
}

bool CWebClientManager::StartHost()
{
  WaylandHostConfig config = OutputMode();

#if defined(HAVE_GBM)
  if (auto* gbm = dynamic_cast<WINDOWING::GBM::CWinSystemGbm*>(CServiceBroker::GetWinSystem()))
  {
    if (auto drm = gbm->GetDrm())
      config.renderDevicePath = drm->GetRenderDevicePath();
    config.keyboardLayout = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
        CLibInputSettings::SETTING_INPUT_LIBINPUTKEYBOARDLAYOUT);
    std::tie(config.physicalWidth, config.physicalHeight) = gbm->GetPhysicalSize();
  }
#endif
  if (config.renderDevicePath.empty())
    config.renderDevicePath = FirstRenderNode();
  if (config.renderDevicePath.empty())
  {
    CLog::Log(LOGERROR, "CWebClientManager::{}: no render device for the clients", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGINFO, "CWebClientManager::{}: hosting {}x{} ({}x{} mm) at {} mHz on {}",
            __FUNCTION__, config.width, config.height, config.physicalWidth, config.physicalHeight,
            config.refreshMilliHz, config.renderDevicePath);

  m_host = std::make_unique<CWaylandHost>(*this, std::move(config));
  if (m_host->Start())
  {
    m_host->WebSetLayoutDirection(g_langInfo.IsRightToLeft());
    return true;
  }

  m_host.reset();
  return false;
}

void CWebClientManager::Spawn()
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string command = settings->GetString(CSettings::SETTING_WEB_CLIENTCOMMAND);
  CLog::Log(LOGINFO, "CWebClientManager::{}: starting {}", __FUNCTION__, command);
  m_state = State::STARTING;
  UpdateVideo();
  // a disk cache in Kodi's profile writes to its storage as pages load, so
  // it is kept only while the user asks for one
  const bool diskCache = settings->GetBool(CSettings::SETTING_WEB_DISKCACHE);
  if (!diskCache && XFILE::CDirectory::Exists(CACHE_KEPT))
    XFILE::CDirectory::RemoveRecursive(CACHE_KEPT);
  // the client keeps profile data under XDG_DATA_HOME; Kodi's profile is
  // where it survives restarts and follows the Kodi profile in use
  std::map<std::string, std::string> env{
      {"WPE_PLATFORM", "wayland"},
      {"XDG_DATA_HOME", CSpecialProtocol::TranslatePath("special://profile/webdata")},
      {"XDG_CACHE_HOME", CSpecialProtocol::TranslatePath(diskCache ? CACHE_KEPT : CACHE_TEMP)},
      {"KODI_WEBVIEW_DISK_CACHE", diskCache ? "1" : "0"}};
  // the client's audio sinks play through Kodi's audio engine
  if (m_audio)
    env.emplace(KODI_WEB_AUDIO_ENV, m_audio->GetPath());
  // the client's video sinks show their frames on a display plane
  if (m_video)
    env.emplace(KODI_WEB_VIDEO_ENV, m_video->GetPath());
  // pages decrypt with the CDM that inputstream.adaptive installed
  if (const std::string cdm = CdmDirectory(); !cdm.empty())
    env.emplace("KODI_WEB_CDM_DIR", cdm);
  m_host->SpawnClient({command}, std::move(env));
}

CWebClientManager::State CWebClientManager::GetState() const
{
  std::unique_lock lock(m_lock);
  return m_state;
}

bool CWebClientManager::RequestView(const std::string& profile, IWebViewObserver& observer)
{
  if (!m_host)
    return false;

  if (!CWaylandHost::IsValidWebProfile(profile))
  {
    CLog::Log(LOGERROR, "CWebClientManager::{}: \"{}\" cannot name a web profile", __FUNCTION__,
              profile);
    return false;
  }

  // a web app opened without its settings would ignore the uplink it is meant to use
  if (!WebAppSettingsLoad(profile))
  {
    const auto& localizeStrings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
    NotifyOpenError(profile, localizeStrings.Get(40258));
    return false;
  }

  const std::string uplink = UplinkOf(profile);

  std::unique_lock lock(m_lock);
  m_observer = &observer;
  m_observedProfile = profile;
  m_shown = ViewState{};
  m_shown.profile = profile;
  m_wantPending = false;

  const auto view = m_views.find(profile);
  if (view != m_views.end() && view->second && m_uplinks[profile] == uplink)
  {
    CWaylandSurface& surface = *view->second;
    lock.unlock();
    observer.OnWebViewAvailable(profile, surface);
    return true;
  }

  m_uplinks[profile] = uplink;
  if (m_closing.count(profile))
    return true;

  // the uplink is fixed when the client creates the view's session
  if (view != m_views.end())
  {
    CloseView(profile);
    return true;
  }

  switch (m_state)
  {
    case State::READY:
      m_host->WebWantView(profile, uplink);
      break;
    case State::STARTING:
      m_wantPending = true;
      break;
    case State::ABSENT:
    case State::FAILED:
      m_respawned = false;
      m_wantPending = true;
      Spawn();
      break;
  }
  return true;
}

void CWebClientManager::ReleaseView(IWebViewObserver& observer)
{
  std::unique_lock lock(m_lock);
  if (m_observer != &observer)
    return;

  m_observer = nullptr;
  m_wantPending = false;
  // the observer is torn down next: calls into it still running finish first
  m_observerCallsDone.wait(lock, [this] { return m_observerCalls == 0; });
  CloseView(m_observedProfile);
  if (m_restartWhenIdle && m_state == State::READY && (m_video != nullptr) != VideoOnPlane())
    RestartClientForVideo();
}

// the client destroys the view later; a request for the profile waits for that
void CWebClientManager::CloseView(const std::string& profile)
{
  const auto view = m_views.find(profile);
  if (view == m_views.end())
    return;

  m_closing.insert(profile);
  m_surfaces.erase(view->second);
  m_views.erase(view);
  m_host->WebViewClose(profile);
}

bool CWebClientManager::GetShownView(ViewState& state) const
{
  std::unique_lock lock(m_lock);
  if (!m_observer)
    return false;

  state = m_shown;
  return true;
}

void CWebClientManager::SetVideoObserver(IWebVideoObserver* observer)
{
  std::unique_lock lock(m_lock);
  m_videoObserver = observer;
  if (m_video)
    m_video->SetObserver(observer);
}

void CWebClientManager::LoadUrl(const std::string& profile, const std::string& url)
{
  if (m_host)
    m_host->WebViewLoadUrl(profile, url);
}

void CWebClientManager::Navigate(const std::string& profile, WebNavigation action)
{
  if (m_host)
    m_host->WebViewNavigate(profile, action);
}

void CWebClientManager::SetActive(const std::string& profile, bool active)
{
  if (m_host)
    m_host->WebViewSetActive(profile, active);
}

void CWebClientManager::FrameMove()
{
  if (m_host)
    m_host->FrameDone();
}

void CWebClientManager::OnResetDisplay()
{
  if (!m_host)
    return;

  const WaylandHostConfig mode = OutputMode();
  m_host->SetOutputMode(mode.width, mode.height, mode.refreshMilliHz);
}

CWebClientManager::CObserverCall::CObserverCall(CWebClientManager& manager,
                                                IWebViewObserver* observer)
  : m_manager(manager),
    m_observer(observer)
{
  if (m_observer)
    ++m_manager.m_observerCalls;
}

CWebClientManager::CObserverCall::~CObserverCall()
{
  if (!m_observer)
    return;

  std::unique_lock lock(m_manager.m_lock);
  if (--m_manager.m_observerCalls == 0)
    m_manager.m_observerCallsDone.notifyAll();
}

CWebClientManager::CObserverCall CWebClientManager::ObserverOf(const std::string& profile)
{
  return CObserverCall(*this, m_observer && m_observedProfile == profile ? m_observer : nullptr);
}

void CWebClientManager::OnSurfaceMapped(CWaylandSurface& surface)
{
  CLog::Log(LOGINFO, "CWebClientManager::{}: client surface \"{}\" is up", __FUNCTION__,
            surface.GetTitle());
}

void CWebClientManager::OnSurfaceDestroyed(CWaylandSurface& surface)
{
  std::unique_lock lock(m_lock);
  m_surfaces.erase(&surface);
  for (auto& [profile, viewSurface] : m_views)
  {
    if (viewSurface == &surface)
      viewSurface = nullptr;
  }
}

void CWebClientManager::OnSurfaceUnmapped(CWaylandSurface& surface)
{
  std::unique_lock lock(m_lock);
  const auto entry = m_surfaces.find(&surface);
  CObserverCall observer =
      entry == m_surfaces.end() ? CObserverCall(*this, nullptr) : ObserverOf(entry->second);
  lock.unlock();
  if (observer)
    observer->OnWebViewUnmapped(surface);
}

void CWebClientManager::OnSurfaceFrame(CWaylandSurface& surface,
                                       std::shared_ptr<CWaylandBuffer> buffer)
{
  std::unique_lock lock(m_lock);
  const auto entry = m_surfaces.find(&surface);
  CObserverCall observer =
      entry == m_surfaces.end() ? CObserverCall(*this, nullptr) : ObserverOf(entry->second);
  lock.unlock();
  if (observer)
    observer->OnWebViewFrame(surface, std::move(buffer));
  else
    buffer->Unlock();
}

void CWebClientManager::OnClientExited(pid_t pid, int status)
{
  std::unique_lock lock(m_lock);
  if (pid < 0)
    CLog::Log(LOGERROR, "CWebClientManager::{}: the web client could not be started", __FUNCTION__);
  else
    CLog::Log(m_restarting ? LOGINFO : LOGWARNING,
              "CWebClientManager::{}: the web client (pid {}) exited with status {}", __FUNCTION__,
              pid, status);
  m_views.clear();
  m_surfaces.clear();
  m_closing.clear();
  if (m_filterLists)
    m_filterLists->ClientGone();
  if (m_restarting)
  {
    m_restarting = false;
    m_state = State::ABSENT;
    m_wantPending = false;
    SpawnIfPreloaded();
    return;
  }

  CObserverCall observer(*this, m_observer);
  const std::string profile = m_observedProfile;

  // one restart per request; a client that fails again stays down until
  // the next web request
  if (m_state == State::READY && !m_respawned)
  {
    m_respawned = true;
    m_wantPending = static_cast<bool>(observer);
    Spawn();
  }
  else
  {
    m_state = State::FAILED;
    m_wantPending = false;
  }
  lock.unlock();

  if (observer)
    observer->OnWebViewGone(profile);
}

void CWebClientManager::OnWebClientReady()
{
  std::unique_lock lock(m_lock);
  m_state = State::READY;
  // a view the client created on its own before ready answers the request
  const bool want = m_wantPending && m_views.find(m_observedProfile) == m_views.end();
  m_wantPending = false;
  const std::string profile = m_observedProfile;
  const std::string uplink = m_uplinks[profile];
  lock.unlock();
  if (want)
    m_host->WebWantView(profile, uplink);
}

std::string CWebClientManager::UplinkOf(const std::string& profile) const
{
  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(profile, addon, ADDON::AddonType::WEBAPP,
                                              ADDON::OnlyEnabled::CHOICE_YES))
    return {};

  return addon->GetSetting("uplinkaddress");
}

bool CWebClientManager::ConfigureViewFromAddon(const std::string& profile)
{
  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(profile, addon, ADDON::AddonType::WEBAPP,
                                              ADDON::OnlyEnabled::CHOICE_YES))
    return false;

  const auto webapp = std::dynamic_pointer_cast<ADDON::CAddon>(addon);
  const ADDON::CAddonType* type = WebAppType(webapp);
  if (!type)
    return false;

  const std::string identity = webapp->GetSetting("identity");
  const std::string userAgent = identity == "custom"
                                    ? webapp->GetSetting("useragent")
                                    : KODI::WEB::WebAppElement(*type, "useragent", identity);
  if (!userAgent.empty())
    m_host->WebViewSetUserAgent(profile, userAgent);

  for (const auto& [element, values] : type->GetValues())
  {
    if (element == "userscript")
      SendProfileFile(
          profile,
          URIUtils::AddFileToFolder(webapp->Path(), values.GetValue("userscript").asString()),
          true);
    else if (element == "viewport")
      m_host->WebViewSetViewport(profile,
                                 std::max(values.GetValue("viewport@width").asInteger(), 0),
                                 std::max(values.GetValue("viewport@height").asInteger(), 0));
    else if (element == "contentfilter")
    {
      const std::string list = values.GetValue("contentfilter@list").asString();
      if (!list.empty())
      {
        std::unique_lock lock(m_lock);
        if (m_filterLists)
          m_filterLists->Want(list);
        lock.unlock();
        m_host->WebViewAddContentFilterList(profile, list);
      }
      else
        SendProfileFile(
            profile,
            URIUtils::AddFileToFolder(webapp->Path(), values.GetValue("contentfilter").asString()),
            false);
    }
  }
  return true;
}

void CWebClientManager::ConfigureView(const std::string& profile)
{
  if (ConfigureViewFromAddon(profile))
    return;

  const std::string root = PROFILE_ROOT + profile + "/";
  if (!XFILE::CDirectory::Exists(root))
    return;

  std::vector<uint8_t> userAgent;
  if (XFILE::CFile().LoadFile(root + "user-agent", userAgent) > 0)
  {
    std::string value(userAgent.begin(), userAgent.end());
    m_host->WebViewSetUserAgent(profile, StringUtils::Trim(value));
  }
  SendProfileFiles(profile, root + "scripts/", ".js", true);
  SendProfileFiles(profile, root + "filters/", ".txt", false);
}

void CWebClientManager::SendProfileFiles(const std::string& profile,
                                         const std::string& path,
                                         const std::string& extension,
                                         bool scripts)
{
  CFileItemList items;
  if (!XFILE::CDirectory::GetDirectory(path, items, extension, XFILE::DIR_FLAG_NO_FILE_DIRS))
    return;

  std::vector<std::string> files;
  for (const auto& item : items)
  {
    if (!item->IsFolder())
      files.push_back(item->GetPath());
  }
  std::sort(files.begin(), files.end());

  for (const std::string& file : files)
    SendProfileFile(profile, file, scripts);
}

void CWebClientManager::SendProfileFile(const std::string& profile,
                                        const std::string& file,
                                        bool script)
{
  const int fd = open(CSpecialProtocol::TranslatePath(file).c_str(), O_RDONLY | O_CLOEXEC);
  struct stat st = {};
  if (fd < 0 || fstat(fd, &st) != 0)
  {
    CLog::Log(LOGERROR, "CWebClientManager::{}: cannot read {}", __FUNCTION__, file);
    if (fd >= 0)
      close(fd);
    return;
  }

  const std::string name = URIUtils::GetFileName(file);
  const auto size = static_cast<uint32_t>(st.st_size);
  if (script)
    m_host->WebViewAddUserScript(profile, name, fd, size);
  else
    m_host->WebViewAddContentFilter(profile, name, fd, size);
}

void CWebClientManager::OnWebViewCreated(const std::string& profile, CWaylandSurface& surface)
{
  std::unique_lock lock(m_lock);
  m_views[profile] = &surface;
  m_surfaces[&surface] = profile;
  CObserverCall observer = ObserverOf(profile);
  // the player that asked for the view has stopped since
  if (!observer)
  {
    CloseView(profile);
    return;
  }
  m_wantPending = false;
  lock.unlock();

  ConfigureView(profile);
  observer->OnWebViewAvailable(profile, surface);
}

void CWebClientManager::OnWebViewUnavailable(const std::string& profile, const std::string& message)
{
  std::unique_lock lock(m_lock);
  CObserverCall observer = ObserverOf(profile);
  lock.unlock();
  if (observer)
    observer->OnWebViewUnavailable(profile, message);
}

void CWebClientManager::OnWebViewUrl(const std::string& profile, const std::string& url)
{
  std::unique_lock lock(m_lock);
  CObserverCall observer = ObserverOf(profile);
  if (observer)
    m_shown.url = url;
  lock.unlock();
  if (observer)
    observer->OnWebViewUrl(profile, url);
}

void CWebClientManager::OnWebViewTitle(const std::string& profile, const std::string& title)
{
  std::unique_lock lock(m_lock);
  CObserverCall observer = ObserverOf(profile);
  if (observer)
    m_shown.title = title;
  lock.unlock();
  if (observer)
    observer->OnWebViewTitle(profile, title);
}

void CWebClientManager::OnWebViewProgress(const std::string& profile, uint32_t percent)
{
  std::unique_lock lock(m_lock);
  CObserverCall observer = ObserverOf(profile);
  lock.unlock();
  if (observer)
    observer->OnWebViewProgress(profile, percent);
}

void CWebClientManager::OnWebViewNavigationState(const std::string& profile,
                                                 bool canGoBack,
                                                 bool canGoForward)
{
  std::unique_lock lock(m_lock);
  CObserverCall observer = ObserverOf(profile);
  if (observer)
  {
    m_shown.canGoBack = canGoBack;
    m_shown.canGoForward = canGoForward;
  }
  lock.unlock();
  if (observer)
    observer->OnWebViewNavigationState(profile, canGoBack, canGoForward);
}

void CWebClientManager::OnWebViewError(const std::string& profile,
                                       uint32_t code,
                                       const std::string& message)
{
  if (code == static_cast<uint32_t>(WebViewError::CONFIG_FAILED))
    NotifyOpenError(profile, message);

  std::unique_lock lock(m_lock);
  CObserverCall observer = ObserverOf(profile);
  lock.unlock();
  if (observer)
    observer->OnWebViewError(profile, code, message);
}

void CWebClientManager::OnWebViewExit(const std::string& profile)
{
  std::unique_lock lock(m_lock);
  CObserverCall observer = ObserverOf(profile);
  lock.unlock();
  if (observer)
    observer->OnWebViewExit(profile);
}

void CWebClientManager::OnWebViewMediaState(const std::string& profile,
                                            bool video,
                                            bool audio,
                                            bool paused)
{
  std::unique_lock lock(m_lock);
  CObserverCall observer = ObserverOf(profile);
  lock.unlock();
  if (observer)
    observer->OnWebViewMediaState(profile, video, audio, paused);
}

void CWebClientManager::OnWebViewDestroyed(const std::string& profile)
{
  std::unique_lock lock(m_lock);
  const auto view = m_views.find(profile);
  if (view != m_views.end())
  {
    m_surfaces.erase(view->second);
    m_views.erase(view);
  }
  const bool closed = m_closing.erase(profile) > 0;
  CObserverCall observer = ObserverOf(profile);
  const std::string uplink = m_uplinks[profile];
  lock.unlock();
  if (closed && observer)
    m_host->WebWantView(profile, uplink);
  else if (observer)
    observer->OnWebViewGone(profile);
}

void CWebClientManager::OnTextInputEnabled(const TextInputState& state)
{
  std::unique_lock lock(m_lock);
  m_textInputActive = true;
  m_textInput = state;
  // a typist with a real keyboard types into the page directly
  if (m_keyboardOpen || HasPhysicalKeyboard())
    return;
  m_keyboardOpen = true;
  m_keyboardText = state.surrounding;
  m_replaceBefore = static_cast<uint32_t>(std::max<int32_t>(state.cursor, 0));
  m_replaceAfter = static_cast<uint32_t>(
      state.surrounding.size() > m_replaceBefore ? state.surrounding.size() - m_replaceBefore : 0);
  m_coveredTop = std::numeric_limits<int32_t>::max();
  m_scrolledCaretY = std::numeric_limits<int32_t>::min();
  lock.unlock();

  CServiceBroker::GetJobManager()->AddJob(
      new CWebKeyboardJob(*this, state.surrounding, state.IsHidden()), nullptr);
}

void CWebClientManager::OnTextInputUpdated(const TextInputState& state)
{
  std::unique_lock lock(m_lock);
  m_textInput = state;
  lock.unlock();
  EnsureCaretVisible();
}

void CWebClientManager::OnTextInputDisabled()
{
  std::unique_lock lock(m_lock);
  m_textInputActive = false;
  const bool open = m_keyboardOpen;
  lock.unlock();
  if (!open)
    return;

  auto* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogWebKeyboard>(
      WINDOW_DIALOG_WEB_KEYBOARD);
  if (dialog)
    dialog->Close(false, 0, true, false);
}

void CWebClientManager::OnKeyboardOpening(int coveredTop)
{
  std::unique_lock lock(m_lock);
  m_coveredTop = coveredTop;
  lock.unlock();
  EnsureCaretVisible();
}

void CWebClientManager::OnKeyboardText(const std::string& text)
{
  std::unique_lock lock(m_lock);
  if (!m_keyboardOpen || text == m_keyboardText)
    return;
  const uint32_t before = m_replaceBefore;
  const uint32_t after = m_replaceAfter;
  m_keyboardText = text;
  m_replaceBefore = static_cast<uint32_t>(text.size());
  m_replaceAfter = 0;
  lock.unlock();

  if (m_host)
    m_host->TextInputCommit(text, before, after);
}

void CWebClientManager::OnKeyboardDone()
{
  std::unique_lock lock(m_lock);
  m_keyboardOpen = false;
}

void CWebClientManager::EnsureCaretVisible()
{
  std::unique_lock lock(m_lock);
  if (!m_keyboardOpen || !m_textInputActive || m_textInput.caretHeight <= 0)
    return;

  // keep one caret height of the page visible above the keys; scroll once
  // per caret position, as a page that cannot scroll reports the same one
  const int32_t limit = m_coveredTop - m_textInput.caretHeight;
  const int32_t caretBottom = m_textInput.caretY + m_textInput.caretHeight;
  if (caretBottom <= limit || m_textInput.caretY == m_scrolledCaretY)
    return;
  m_scrolledCaretY = m_textInput.caretY;
  const int32_t x = m_textInput.caretX;
  const int32_t y = m_textInput.caretY;
  lock.unlock();

  if (m_host)
    m_host->PointerScroll(x, y, caretBottom - limit);
}

bool CWebKeyboardJob::DoWork()
{
  auto* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogWebKeyboard>(
      WINDOW_DIALOG_WEB_KEYBOARD);
  if (dialog && dialog->Initialize())
  {
    m_manager.OnKeyboardOpening(dialog->GetCoveredTop());
    std::string typed;
    dialog->ShowAndGetInput(OnWebKeyboardText, m_text, typed, "", m_hidden);
  }
  m_manager.OnKeyboardDone();
  return true;
}
