/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "smarthome/guiinfo/IPowerMeterHUD.h"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>

namespace rclcpp
{
class Node;
}

namespace KODI::SMART_HOME
{
class CRos2PowerMeterSubscriber;

class CRos2PowerMeterManager : public IPowerMeterHUD
{
public:
  using ActivityCallback = std::function<void(const std::string&)>;
  using PowerMeterKey = std::pair<std::string, std::string>;

  CRos2PowerMeterManager(std::string rosNamespace, ActivityCallback activityCallback);
  ~CRos2PowerMeterManager();
  void Initialize(std::shared_ptr<rclcpp::Node> node);
  void Deinitialize();
  std::optional<PowerMeterReading> PowerMeter(const std::string& systemName,
                                              const std::string& powerMeterName) override;
  std::optional<double> CurrentShare(const std::string& systemName,
                                     const std::string& powerMeterName,
                                     const std::string& otherPowerMeterName) override;
  size_t SubscriptionCount() const;

private:
  std::optional<PowerMeterReading> PowerMeterLocked(const std::string& systemName,
                                                    const std::string& powerMeterName);

  const std::string m_rosNamespace;
  const ActivityCallback m_activityCallback;
  std::shared_ptr<rclcpp::Node> m_node;
  std::map<PowerMeterKey, std::unique_ptr<CRos2PowerMeterSubscriber>> m_subscribers;
  std::set<PowerMeterKey> m_failedRegistrations;
  mutable std::mutex m_mutex;
};
} // namespace KODI::SMART_HOME
