/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2PropertyManager.h"

#include "Ros2PropertySubscriber.h"
#include "smarthome/utils/RosNameUtils.h"
#include "utils/log.h"

#include <rclcpp/rclcpp.hpp>

using namespace KODI::SMART_HOME;

CRos2PropertyManager::CRos2PropertyManager(std::string rosNamespace,
                                           ActivityCallback activityCallback)
  : m_rosNamespace(std::move(rosNamespace)),
    m_activityCallback(std::move(activityCallback))
{
}

CRos2PropertyManager::~CRos2PropertyManager()
{
  Deinitialize();
}

void CRos2PropertyManager::Initialize(std::shared_ptr<rclcpp::Node> node)
{
  std::lock_guard lock(m_mutex);
  m_node = std::move(node);
}

void CRos2PropertyManager::Deinitialize()
{
  std::map<PropertyNameKey, std::unique_ptr<CRos2PropertySubscriber>> subscribers;
  {
    std::lock_guard lock(m_mutex);
    subscribers.swap(m_subscribers);
    m_propertyTypes.clear();
    m_failedRegistrations.clear();
    m_node.reset();
  }

  for (auto& subscriber : subscribers)
    subscriber.second->Deinitialize();
}

std::optional<SmartHomePropertyValue> CRos2PropertyManager::Property(
    const std::string& systemName, const std::string& propertyName, SmartHomePropertyType type)
{
  std::lock_guard lock(m_mutex);
  const PropertyNameKey key{systemName, propertyName};
  const PropertyRequestKey requestKey{systemName, propertyName, type};

  if (m_failedRegistrations.contains(requestKey))
    return std::nullopt;

  const auto typeIt = m_propertyTypes.find(key);
  if (typeIt != m_propertyTypes.end() && typeIt->second != type)
  {
    m_failedRegistrations.emplace(requestKey);
    CLog::Log(
        LOGERROR,
        "ROS2: Rejecting conflicting type '{}' for property {}/{}; already registered as '{}'",
        SmartHomePropertyTypeToString(type), systemName, propertyName,
        SmartHomePropertyTypeToString(typeIt->second));
    return std::nullopt;
  }

  const auto subscriberIt = m_subscribers.find(key);
  if (subscriberIt != m_subscribers.end())
    return subscriberIt->second->Value();

  // Initialization is transient, so do not cache this as a permanent registration failure.
  if (!m_node)
    return std::nullopt;

  const std::string topic = BuildRosName(m_rosNamespace, {systemName, propertyName});
  auto subscriber = std::make_unique<CRos2PropertySubscriber>(
      topic, type,
      [activityCallback = m_activityCallback, systemName]()
      {
        if (activityCallback)
          activityCallback(systemName);
      });

  try
  {
    CLog::Log(LOGDEBUG, "ROS2: Subscribing to {} as {}", topic,
              CRos2PropertySubscriber::MessageTypeName(type));
    subscriber->Initialize(m_node);
  }
  catch (const std::exception& error)
  {
    m_failedRegistrations.emplace(requestKey);
    CLog::Log(LOGERROR,
              "ROS2: Failed to subscribe to system '{}' property '{}' as type '{}' on topic '{}': "
              "{}",
              systemName, propertyName, SmartHomePropertyTypeToString(type), topic, error.what());
    return std::nullopt;
  }

  const auto value = subscriber->Value();
  m_propertyTypes.emplace(key, type);
  m_subscribers.emplace(key, std::move(subscriber));
  return value;
}

size_t CRos2PropertyManager::SubscriptionCount() const
{
  std::lock_guard lock(m_mutex);
  return m_subscribers.size();
}
