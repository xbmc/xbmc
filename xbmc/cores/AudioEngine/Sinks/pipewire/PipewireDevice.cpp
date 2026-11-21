/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireDevice.h"

#include <array>
#include <span>

#include <spa/param/props.h>
#include <spa/param/route.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>

using namespace KODI;
using namespace PIPEWIRE;

CPipewireDevice::CPipewireDevice(CPipewireRegistry& registry, uint32_t id, const char* type)
  : CPipewireProxy(registry, id, type, PW_VERSION_DEVICE),
    m_deviceEvents(CreateDeviceEvents())
{
  pw_proxy_add_object_listener(m_proxy.get(), &m_objectListener, &m_deviceEvents, this);
}

CPipewireDevice::~CPipewireDevice()
{
  spa_hook_remove(&m_objectListener);
}

bool CPipewireDevice::SetVolume(uint32_t profileDevice,
                                const std::vector<float>& volumes,
                                bool mute)
{
  const auto route = m_routes.find(profileDevice);
  if (route == m_routes.end())
    return false;

  std::array<uint8_t, 1024> buffer;
  auto builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());
  std::array<spa_pod_frame, 2> frames;

  spa_pod_builder_push_object(&builder, &frames[0], SPA_TYPE_OBJECT_ParamRoute, SPA_PARAM_Route);
  spa_pod_builder_add(&builder, SPA_PARAM_ROUTE_index, SPA_POD_Int(route->second),
                      SPA_PARAM_ROUTE_device, SPA_POD_Int(profileDevice), 0);
  spa_pod_builder_prop(&builder, SPA_PARAM_ROUTE_props, 0);
  spa_pod_builder_push_object(&builder, &frames[1], SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
  spa_pod_builder_add(&builder, SPA_PROP_channelVolumes,
                      SPA_POD_Array(sizeof(float), SPA_TYPE_Float, volumes.size(), volumes.data()),
                      SPA_PROP_mute, SPA_POD_Bool(mute), 0);
  spa_pod_builder_pop(&builder, &frames[1]);
  spa_pod_builder_prop(&builder, SPA_PARAM_ROUTE_save, 0);
  spa_pod_builder_bool(&builder, true);
  const auto* param = static_cast<const spa_pod*>(spa_pod_builder_pop(&builder, &frames[0]));

  pw_device_set_param(reinterpret_cast<pw_device*>(m_proxy.get()), SPA_PARAM_Route, 0, param);
  return true;
}

void CPipewireDevice::Info(void* userdata, const struct pw_device_info* info)
{
  auto& device = *reinterpret_cast<CPipewireDevice*>(userdata);

  if ((info->change_mask & PW_DEVICE_CHANGE_MASK_PARAMS) == 0)
    return;

  for (const auto& param : std::span(info->params, info->n_params))
  {
    if (param.id != SPA_PARAM_Route || (param.flags & SPA_PARAM_INFO_READ) == 0)
      continue;

    device.m_routes.clear();
    pw_device_enum_params(reinterpret_cast<pw_device*>(device.m_proxy.get()), 0, param.id, 0, 0,
                          nullptr);
  }
}

void CPipewireDevice::Param(void* userdata,
                            int seq,
                            uint32_t id,
                            uint32_t index,
                            uint32_t next,
                            const struct spa_pod* param)
{
  auto& device = *reinterpret_cast<CPipewireDevice*>(userdata);

  int32_t routeIndex = 0;
  int32_t profileDevice = 0;
  if (id != SPA_PARAM_Route ||
      spa_pod_parse_object(param, SPA_TYPE_OBJECT_ParamRoute, nullptr, SPA_PARAM_ROUTE_index,
                           SPA_POD_Int(&routeIndex), SPA_PARAM_ROUTE_device,
                           SPA_POD_Int(&profileDevice)) < 0)
    return;

  device.m_routes[static_cast<uint32_t>(profileDevice)] = static_cast<uint32_t>(routeIndex);
}

pw_device_events CPipewireDevice::CreateDeviceEvents()
{
  pw_device_events deviceEvents = {};
  deviceEvents.version = PW_VERSION_DEVICE_EVENTS;
  deviceEvents.info = Info;
  deviceEvents.param = Param;

  return deviceEvents;
}
