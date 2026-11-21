/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "smarthome/guiinfo/IPowerMeterHUD.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace rclcpp
{
class Node;
}

namespace KODI::SMART_HOME
{
class CRos2PowerMeterSubscriber
{
public:
  using ActivityCallback = std::function<void()>;

  CRos2PowerMeterSubscriber(std::string topic, ActivityCallback activityCallback);
  ~CRos2PowerMeterSubscriber();

  void Initialize(const std::shared_ptr<rclcpp::Node>& node);
  void Deinitialize();
  std::optional<PowerMeterReading> Reading() const;

private:
  struct State;
  const std::string m_topic;
  const std::shared_ptr<State> m_state;
};
} // namespace KODI::SMART_HOME
