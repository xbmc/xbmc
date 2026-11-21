/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PipewireProxy.h"

#include <cstdint>
#include <map>
#include <vector>

#include <pipewire/device.h>

namespace KODI
{
namespace PIPEWIRE
{

class CPipewireRegistry;

/*!
 * \brief An audio card, tracked for the routes its profile devices are
 * connected to, which is where the card's sinks have their volume
 */
class CPipewireDevice : public CPipewireProxy
{
public:
  explicit CPipewireDevice(CPipewireRegistry& registry, uint32_t id, const char* type);
  CPipewireDevice() = delete;
  ~CPipewireDevice() override;

  /*!
   * \brief Set the volumes and mute of the route a profile device is on;
   * false when it has none. The thread loop must be locked.
   */
  bool SetVolume(uint32_t profileDevice, const std::vector<float>& volumes, bool mute);

private:
  static void Info(void* userdata, const struct pw_device_info* info);
  static void Param(void* userdata,
                    int seq,
                    uint32_t id,
                    uint32_t index,
                    uint32_t next,
                    const struct spa_pod* param);

  static pw_device_events CreateDeviceEvents();

  const pw_device_events m_deviceEvents;

  spa_hook m_objectListener;

  std::map<uint32_t, uint32_t> m_routes;
};

} // namespace PIPEWIRE
} // namespace KODI
