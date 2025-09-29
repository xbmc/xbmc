/*
*  Copyright (C) 2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "WinSystemWaylandWebOS.h"

#include "Connection.h"
#include "OSScreenSaverWebOS.h"
#include "Registry.h"
#include "SeatWebOS.h"
#include "ShellSurfaceWebOSShell.h"
#include "VideoRenderers/HwDecRender/RendererStarfish.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/log.h"

#include "platform/linux/WebOSTVPlatformConfig.h"

#include <CompileInfo.h>

namespace
{
constexpr const char* LUNA_REGISTER_APP = "luna://com.webos.service.applicationmanager/registerApp";

constexpr unsigned int WIDTH_1080P = 1920;
constexpr unsigned int HEIGHT_1080P = 1080;
constexpr unsigned int SCREEN_WIDTH_4K = 3840;
constexpr unsigned int SCREEN_HEIGHT_4K = 2160;

constexpr unsigned int WIDTH_720P = 1280;
constexpr unsigned int HEIGHT_720P = 720;
constexpr unsigned int SCREEN_WIDTH_1080P = 1080;
constexpr unsigned int SCREEN_HEIGHT_1080P = 1920;
} // namespace

namespace KODI::WINDOWING::WAYLAND
{

bool CWinSystemWaylandWebOS::InitWindowSystem()
{
  if (!CWinSystemWayland::InitWindowSystem())
    return false;

  CRendererStarfish::Register();

  m_webosRegistry = std::make_unique<CRegistry>(*GetConnection());
  m_webosRegistry->RequestSingleton(m_compositor, 1, 4);
  // available since webOS 5.0
  m_webosRegistry->RequestSingleton(m_webosForeign, 1, 2, false);
  m_webosRegistry->Bind();

  m_requestContext->pub = true;
  m_requestContext->multiple = true;
  m_requestContext->callback = &OnAppLifecycleEventWrapper;
  m_requestContext->userdata = this;
  if (HLunaServiceCall(LUNA_REGISTER_APP, "{}", m_requestContext.get()))
  {
    CLog::LogF(LOGWARNING, "Luna request call failed");
    m_requestContext = nullptr;
  }

  return true;
}

bool CWinSystemWaylandWebOS::DestroyWindowSystem()
{
  m_exportedSurface = wayland::webos_exported_t{};
  m_webosForeign = wayland::webos_foreign_t{};

  if (m_webosRegistry)
  {
    m_webosRegistry->UnbindSingletons();
  }
  m_webosRegistry.reset();
  return CWinSystemWayland::DestroyWindowSystem();
}

bool CWinSystemWaylandWebOS::CreateNewWindow(const std::string& name,
                                             bool fullScreen,
                                             RESOLUTION_INFO& res)
{
  if (!CWinSystemWayland::CreateNewWindow(name, fullScreen, res))
    return false;

  if (m_webosForeign)
  {
    m_exportedSurface = m_webosForeign.export_element(
        GetMainSurface(),
        static_cast<uint32_t>(wayland::webos_foreign_webos_exported_type::video_object));
    m_exportedSurface.on_window_id_assigned() =
        [this](std::string window_id, uint32_t exported_type)
    {
      CLog::LogF(LOGDEBUG, "Foreign video surface exported {}", window_id);
      this->m_exportedWindowName = window_id;
    };
  }

  return true;
}

bool CWinSystemWaylandWebOS::HasCursor()
{
  return false;
}

std::string CWinSystemWaylandWebOS::GetExportedWindowName()
{
  return m_exportedWindowName;
}

bool CWinSystemWaylandWebOS::SetExportedWindow(CRect orig, CRect src, CRect dest)
{
  if (m_webosForeign)
  {
    CLog::LogF(LOGINFO, "orig {} {} {} {} src {} {} {} {} -> dest {} {} {} {}", orig.x1, orig.y1,
               orig.x2, orig.y2, src.x1, src.y1, src.x2, src.y2, dest.x1, dest.y1, dest.x2,
               dest.y2);
    wayland::region_t origRegion = m_compositor.create_region();
    wayland::region_t srcRegion = m_compositor.create_region();
    wayland::region_t dstRegion = m_compositor.create_region();
    origRegion.add(static_cast<int>(orig.x1), static_cast<int>(orig.y1),
                   static_cast<int>(orig.Width()), static_cast<int>(orig.Height()));
    srcRegion.add(static_cast<int>(src.x1), static_cast<int>(src.y1), static_cast<int>(src.Width()),
                  static_cast<int>(src.Height()));
    dstRegion.add(static_cast<int>(dest.x1), static_cast<int>(dest.y1),
                  static_cast<int>(dest.Width()), static_cast<int>(dest.Height()));
    m_exportedSurface.set_crop_region(origRegion, srcRegion, dstRegion);

    return true;
  }

  return false;
}

bool CWinSystemWaylandWebOS::SupportsExportedWindow()
{
  return m_webosForeign;
}

IShellSurface* CWinSystemWaylandWebOS::CreateShellSurface(const std::string& name)
{
  return new CShellSurfaceWebOSShell(*this, *GetConnection(), GetMainSurface(), name,
                                     std::string(CCompileInfo::GetAppName()));
}

std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> CWinSystemWaylandWebOS::GetOSScreenSaverImpl()
{
  return std::make_unique<COSScreenSaverWebOS>();
}

std::unique_ptr<CSeat> CWinSystemWaylandWebOS::CreateSeat(std::uint32_t name, wayland::seat_t& seat)
{
  return std::make_unique<CSeatWebOS>(name, seat, *GetConnection());
}

bool CWinSystemWaylandWebOS::OnAppLifecycleEventWrapper(LSHandle* sh, LSMessage* reply, void* ctx)
{
  HContext* context = static_cast<HContext*>(ctx);
  return static_cast<CWinSystemWaylandWebOS*>(context->userdata)->OnAppLifecycleEvent(sh, reply);
}

void CWinSystemWaylandWebOS::OnConfigure(std::uint32_t serial,
                                         CSizeInt size,
                                         IShellSurface::StateBitset state)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto player = components.GetComponent<CApplicationPlayer>();

  // intercept minimized event, passing the minimized event causes a weird animation
  if (state.none())
  {
    if (player)
    {
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1,
                                                 static_cast<void*>(new CAction(ACTION_STOP)));
    }
  }
  else
  {
    CWinSystemWayland::OnConfigure(serial, size, state);
  }
}

bool CWinSystemWaylandWebOS::OnAppLifecycleEvent(LSHandle* sh, LSMessage* reply)
{
  const char* msg = HLunaServiceMessage(reply);
  CLog::Log(LOGDEBUG, "Got lifecycle event: {}", msg);

  CVariant event;
  CJSONVariantParser::Parse(msg, event);

  IShellSurface* shellSurface = GetShellSurface();
  if (event["event"] == "relaunch" && shellSurface)
    shellSurface->SetFullScreen(nullptr, 60.0f);

  return true;
}

// The reported resolution is always 1080p even for 4K devices and 720p for HD devices
// See: https://webostv.developer.lge.com/develop/specifications/app-resolution
// So we need to adjust the reported resolution to match the actual screen resolution
// Note: Should webOS ever change to support more resolutions we need to update this code
void CWinSystemWaylandWebOS::UpdateResolutions()
{
  CWinSystemWayland::UpdateResolutions();

  RESOLUTION_INFO& res = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
  if (res.iWidth == WIDTH_1080P && res.iHeight == HEIGHT_1080P)
  {
    // set supported video resolution to 4K for 1080p GUI resolution device
    res.iScreenHeight = SCREEN_HEIGHT_4K;
    res.iScreenWidth = SCREEN_WIDTH_4K;
  }
  else if (res.iWidth == WIDTH_720P && res.iHeight == HEIGHT_720P)
  {
    // set supported video resolution to 1080p for 720p GUI resolution device
    res.iScreenHeight = SCREEN_HEIGHT_1080P;
    res.iScreenWidth = SCREEN_WIDTH_1080P;
  }
  else
    CLog::Log(LOGWARNING, "Cannot adjust resolution, due to unmapped w x h values: {} x {}",
              res.iWidth, res.iHeight);
}

float CWinSystemWaylandWebOS::GetGuiSdrPeakLuminance() const
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const int guiSdrPeak = settings->GetInt(CSettings::SETTING_VIDEOSCREEN_GUISDRPEAKLUMINANCE);

  return (0.7f * guiSdrPeak + 30.0f) / 100.0f;
}

bool CWinSystemWaylandWebOS::IsHDRDisplay()
{
  return WebOSTVPlatformConfig::SupportsHDR();
}

} // namespace KODI::WINDOWING::WAYLAND
