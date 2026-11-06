/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Ros2PowerMeterManager.h"

#include "Ros2PowerMeterSubscriber.h"
#include "smarthome/utils/RosNameUtils.h"
#include "utils/log.h"

#include <rclcpp/rclcpp.hpp>

using namespace KODI::SMART_HOME;

CRos2PowerMeterManager::CRos2PowerMeterManager(std::string rosNamespace,
                                               ActivityCallback activityCallback)
  : m_rosNamespace(std::move(rosNamespace)),
    m_activityCallback(std::move(activityCallback))
{
}

CRos2PowerMeterManager::~CRos2PowerMeterManager()
{
  Deinitialize();
}

void CRos2PowerMeterManager::Initialize(std::shared_ptr<rclcpp::Node> node)
{
  std::lock_guard lock(m_mutex);
  m_node = std::move(node);
}

void CRos2PowerMeterManager::Deinitialize()
{
  std::map<PowerMeterKey, std::unique_ptr<CRos2PowerMeterSubscriber>> subscribers;
  {
    std::lock_guard lock(m_mutex);
    subscribers.swap(m_subscribers);
    m_failedRegistrations.clear();
    m_node.reset();
  }

  for (auto& subscriberEntry : subscribers)
    subscriberEntry.second->Deinitialize();
}

std::optional<PowerMeterReading> CRos2PowerMeterManager::PowerMeter(
    const std::string& systemName, const std::string& powerMeterName)
{
  std::lock_guard lock(m_mutex);
  return PowerMeterLocked(systemName, powerMeterName);
}

std::optional<PowerMeterReading> CRos2PowerMeterManager::PowerMeterLocked(
    const std::string& systemName, const std::string& powerMeterName)
{
  const PowerMeterKey key{systemName, powerMeterName};
  if (m_failedRegistrations.contains(key))
    return std::nullopt;

  const auto subscriberIt = m_subscribers.find(key);
  if (subscriberIt != m_subscribers.end())
    return subscriberIt->second->Reading();

  if (!m_node)
    return std::nullopt;

  const std::string topic = BuildRosName(m_rosNamespace, {systemName, "power", powerMeterName});
  auto subscriber =
      std::make_unique<CRos2PowerMeterSubscriber>(topic,
                                                  [callback = m_activityCallback, systemName]
                                                  {
                                                    if (callback)
                                                      callback(systemName);
                                                  });
  try
  {
    CLog::Log(LOGDEBUG, "ROS2: Subscribing to {} as oasis_msgs/msg/PowerMeter", topic);
    subscriber->Initialize(m_node);
  }
  catch (const std::exception& error)
  {
    m_failedRegistrations.emplace(key);
    CLog::Log(LOGERROR, "ROS2: Failed to subscribe to power meter '{}/{}' on '{}': {}", systemName,
              powerMeterName, topic, error.what());
    return std::nullopt;
  }

  const auto reading = subscriber->Reading();
  m_subscribers.emplace(key, std::move(subscriber));
  return reading;
}

std::optional<double> CRos2PowerMeterManager::CurrentShare(const std::string& systemName,
                                                           const std::string& powerMeterName,
                                                           const std::string& otherPowerMeterName)
{
  std::lock_guard lock(m_mutex);
  const auto reading = PowerMeterLocked(systemName, powerMeterName);
  const auto otherReading = PowerMeterLocked(systemName, otherPowerMeterName);
  if (!reading || !otherReading)
    return std::nullopt;

  if (powerMeterName == otherPowerMeterName)
    return 50.0;

  return CalculateCurrentShare(reading->current, otherReading->current);
}

size_t CRos2PowerMeterManager::SubscriptionCount() const
{
  std::lock_guard lock(m_mutex);
  return m_subscribers.size();
}
