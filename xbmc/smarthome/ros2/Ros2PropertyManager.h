/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/SmartHomeProperty.h"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace rclcpp
{
class Node;
}

namespace KODI::SMART_HOME
{
class CRos2PropertySubscriber;

class CRos2PropertyManager
{
public:
  using ActivityCallback = std::function<void(const std::string&)>;

  CRos2PropertyManager(std::string rosNamespace, ActivityCallback activityCallback);
  ~CRos2PropertyManager();

  void Initialize(std::shared_ptr<rclcpp::Node> node);
  void Deinitialize();

  std::optional<SmartHomePropertyValue> Property(const std::string& systemName,
                                                 const std::string& propertyName,
                                                 SmartHomePropertyType type);

  size_t SubscriptionCount() const;

private:
  using PropertyNameKey = std::pair<std::string, std::string>;
  using PropertyRequestKey = std::tuple<std::string, std::string, SmartHomePropertyType>;

  const std::string m_rosNamespace;
  const ActivityCallback m_activityCallback;
  std::shared_ptr<rclcpp::Node> m_node;
  std::map<PropertyNameKey, std::unique_ptr<CRos2PropertySubscriber>> m_subscribers;
  std::map<PropertyNameKey, SmartHomePropertyType> m_propertyTypes;
  std::set<PropertyRequestKey> m_failedRegistrations;
  mutable std::mutex m_mutex;
};
} // namespace KODI::SMART_HOME
