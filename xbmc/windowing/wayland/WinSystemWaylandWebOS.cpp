/*
*  Copyright (C) 2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "WinSystemWaylandWebOS.h"

#include "Connection.h"
#include "Registry.h"
#include "ShellSurfaceWebOSShell.h"
#include "cores/AudioEngine/Sinks/AESinkStarfish.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecStarfish.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererStarfish.h"
#include "utils/log.h"

#include <CompileInfo.h>

namespace KODI::WINDOWING::WAYLAND
{

bool CWinSystemWaylandWebOS::InitWindowSystem()
{
  auto ok = CWinSystemWayland::InitWindowSystem();

  CDVDVideoCodecStarfish::Register();
  CRendererStarfish::Register();
  CAESinkStarfish::Register();

  m_webosRegistry = std::make_unique<CRegistry>(*GetConnection());
  m_webosRegistry->RequestSingleton(m_compositor, 1, 4);
  // available since webOS 5.0
  m_webosRegistry->RequestSingleton(m_webosForeign, 1, 2, false);
  m_webosRegistry->Bind();

  return ok;
}

bool CWinSystemWaylandWebOS::CreateNewWindow(const std::string& name,
                                             bool fullScreen,
                                             RESOLUTION_INFO& res)
{
  auto ok = CWinSystemWayland::CreateNewWindow(name, fullScreen, res);

  if (m_webosForeign)
  {
    m_exportedSurface = m_webosForeign.export_element(
        GetMainSurface(),
        static_cast<uint32_t>(wayland::webos_foreign_webos_exported_type::video_object));
    m_exportedSurface.on_window_id_assigned() = [this](std::string window_id,
                                                       uint32_t exported_type) {
      CLog::Log(LOGDEBUG, "Wayland foreign video surface exported {}", window_id);
      this->m_exportedWindowName = window_id;
    };
  }

  return ok;
}

CWinSystemWaylandWebOS::~CWinSystemWaylandWebOS() noexcept
{
  m_exportedSurface = wayland::webos_exported_t{};
  m_webosForeign = wayland::webos_foreign_t{};

  if (m_webosRegistry)
  {
    m_webosRegistry->UnbindSingletons();
  }
  m_webosRegistry.reset();
}

bool CWinSystemWaylandWebOS::HasCursor()
{
  return false;
}

std::string CWinSystemWaylandWebOS::GetExportedWindowName()
{
  return m_exportedWindowName;
}

bool CWinSystemWaylandWebOS::SetExportedWindow(int32_t srcWidth,
                                               int32_t srcHeight,
                                               int32_t dstWidth,
                                               int32_t dstHeight)
{
  if (m_webosForeign)
  {
    auto srcRegion = m_compositor.create_region();
    auto dstRegion = m_compositor.create_region();
    srcRegion.add(0, 0, srcWidth, srcHeight);
    dstRegion.add(0, 0, dstWidth, dstHeight);
    m_exportedSurface.set_exported_window(srcRegion, dstRegion);

    return true;
  }

  return false;
}

IShellSurface* CWinSystemWaylandWebOS::CreateShellSurface(const std::string& name)
{
  return new CShellSurfaceWebOSShell(*this, *GetConnection(), GetMainSurface(), name,
                                     std::string(CCompileInfo::GetAppName()));
}

} // namespace KODI::WINDOWING::WAYLAND
