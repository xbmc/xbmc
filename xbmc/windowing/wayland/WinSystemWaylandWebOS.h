/*
*  Copyright (C) 2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#pragma once

#include "Registry.h"
#include "WinSystemWayland.h"

#include <wayland-webos-protocols.hpp>

namespace KODI::WINDOWING::WAYLAND
{

class CWinSystemWaylandWebOS : public CWinSystemWayland
{

public:
  bool InitWindowSystem() override;

  /**
   * Gets the exported window name. May return an empty string on non wayland-webos-foreign devices (pre webOS 5)
   * @return Exported window name
   */
  std::string GetExportedWindowName();

  /**
   * Sets up the an exported window for display. The display engine will merge the exported window with the UI layer.
   * Therefore any UI element must be transparent for the exported window to punch through. Not available on non
   * wayland-webos-foreign devices (pre webOS 5)
   * @param srcWidth Exported window width
   * @param srcHeight Exported window height
   * @param dstWidth Destination width
   * @param dstHeight Destination width
   * @return True on success, else false
   */
  bool SetExportedWindow(int32_t srcWidth, int32_t srcHeight, int32_t dstWidth, int32_t dstHeight);

  IShellSurface* CreateShellSurface(const std::string& name) override;
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  ~CWinSystemWaylandWebOS() noexcept override;
  bool HasCursor() override;

private:
  std::unique_ptr<CRegistry> m_webosRegistry;

  // WebOS foreign surface
  std::string m_exportedWindowName;
  wayland::compositor_t m_compositor;
  wayland::webos_exported_t m_exportedSurface;
  wayland::webos_foreign_t m_webosForeign;
};

} // namespace KODI::WINDOWING::WAYLAND
