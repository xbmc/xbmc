/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Connection.h"
#include "utils/HDRCapabilities.h"

#include <wayland-client-protocol.hpp>
#include <wayland-extra-protocols.hpp>

struct VideoPicture;

namespace KODI::WINDOWING::WAYLAND
{

class CColorManager
{
public:
  explicit CColorManager(CConnection& connection);
  ~CColorManager();

  void SetSurface(const wayland::surface_t& surface);
  void UnsetSurface();

  bool SetHDR(const VideoPicture* videoPicture);
  bool IsHDRDisplay() const;
  CHDRCapabilities GetDisplayHDRCapabilities() const;

#ifdef HAS_WAYLAND_COLOR_MANAGEMENT
private:
  void SetDisplayMetadata(wayland::image_description_creator_params_v1_t& descriptionCreator,
                          const VideoPicture* videoPicture);
  void SetLightMetadata(wayland::image_description_creator_params_v1_t& descriptionCreator,
                        const VideoPicture* videoPicture);

  template<typename T>
  class Supported
  {
  public:
    void SetSupported(T t) { flags |= 1 << static_cast<uint32_t>(t); }
    bool IsSupported(T t) const { return flags & 1 << static_cast<uint32_t>(t); }

  private:
    uint32_t flags{};
  };

  Supported<wayland::color_manager_v1_render_intent> m_compositorIntents;
  Supported<wayland::color_manager_v1_feature> m_compositorFeatures;
  Supported<wayland::color_manager_v1_transfer_function> m_compositorTFs;
  Supported<wayland::color_manager_v1_primaries> m_compositorPrimaries;

  wayland::color_manager_v1_t m_colorManager;
  wayland::color_management_surface_v1_t m_colorManagementSurface;
  wayland::color_management_output_v1_t m_output;
#endif
};

} // namespace KODI::WINDOWING::WAYLAND
