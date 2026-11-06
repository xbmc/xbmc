/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/SmartHomeProperty.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace rclcpp
{
class Node;
}

namespace KODI::SMART_HOME
{
class CRos2PropertySubscriber
{
public:
  using ActivityCallback = std::function<void()>;

  CRos2PropertySubscriber(std::string topic,
                          SmartHomePropertyType type,
                          ActivityCallback activityCallback);
  ~CRos2PropertySubscriber();

  void Initialize(const std::shared_ptr<rclcpp::Node>& node);
  void Deinitialize();

  SmartHomePropertyType Type() const { return m_type; }
  std::optional<SmartHomePropertyValue> Value() const;

  static std::string_view MessageTypeName(SmartHomePropertyType type);

private:
  struct State;

  const std::string m_topic;
  const SmartHomePropertyType m_type;
  const std::shared_ptr<State> m_state;
};
} // namespace KODI::SMART_HOME
