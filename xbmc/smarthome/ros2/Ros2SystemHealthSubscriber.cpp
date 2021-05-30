
/*
 *  Copyright (C) 2024-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2SystemHealthSubscriber.h"

#include "smarthome/utils/RosNameUtils.h"
#include "utils/log.h"

#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;
using namespace std::literals::chrono_literals;
using std::placeholders::_1;

namespace
{
constexpr const char* SUBSCRIBE_TELEMETRY_TOPIC = "system_telemetry";
constexpr const char* SUBSCRIBE_UPS_STATUS_TOPIC = "ups_status";
} // namespace

CRos2SystemHealthSubscriber::CRos2SystemHealthSubscriber(std::string rosNamespace)
  : m_rosNamespace(std::move(rosNamespace))
{
}

void CRos2SystemHealthSubscriber::Initialize(std::shared_ptr<rclcpp::Node> node,
                                             const std::string& systemName)
{
  // Calculate topic names
  const std::string subscribeTelemetryTopic =
      BuildRosName(m_rosNamespace, {systemName, SUBSCRIBE_TELEMETRY_TOPIC});
  const std::string subscribeUPSStatusTopic =
      BuildRosName(m_rosNamespace, {systemName, SUBSCRIBE_UPS_STATUS_TOPIC});

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeTelemetryTopic);
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeUPSStatusTopic);

  // Subscribers
  m_telemetrySubscriber = node->create_subscription<SystemTelemetry>(
      subscribeTelemetryTopic, 1,
      std::bind(&CRos2SystemHealthSubscriber::OnSystemTelemetry, this, _1));
  m_upsStatusSubscriber = node->create_subscription<UPSStatus>(
      subscribeUPSStatusTopic, 1, std::bind(&CRos2SystemHealthSubscriber::OnUPSStatus, this, _1));
}

void CRos2SystemHealthSubscriber::Deinitialize()
{
  m_telemetrySubscriber.reset();
}

bool CRos2SystemHealthSubscriber::IsActive(std::chrono::milliseconds timeout) const
{
  bool isActive = false;

  std::lock_guard<std::mutex> lock(m_mutex);

  // Check if m_lastActive is valid
  if (m_lastActive.time_since_epoch().count() > 0)
  {
    // Get current time
    const auto now = std::chrono::steady_clock::now();

    // Check if the system is active
    isActive = (now - m_lastActive) < timeout;
  }

  return isActive;
}

CTemperature CRos2SystemHealthSubscriber::CPUTemperature() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_cpuTemperature;
}

float CRos2SystemHealthSubscriber::CPUUtilization() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_cpuUtilization;
}

double CRos2SystemHealthSubscriber::CPUFrequencyHz() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_cpuFrequencyHz;
}

float CRos2SystemHealthSubscriber::MemoryUtilization() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_memoryUtilization;
}

unsigned int CRos2SystemHealthSubscriber::BatteryCharge() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_batteryCharge;
}

float CRos2SystemHealthSubscriber::BatteryLoad() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_batteryLoadWatts;
}

void CRos2SystemHealthSubscriber::MarkActive()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_lastActive = std::chrono::steady_clock::now();
}

void CRos2SystemHealthSubscriber::OnSystemTelemetry(const SystemTelemetry::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Update the last active time
  m_lastActive = std::chrono::steady_clock::now();

  // Update system health parameters
  m_cpuTemperature = CTemperature::CreateFromCelsius(msg->cpu_temperature);
  m_cpuUtilization = msg->cpu_utilization;
  m_cpuFrequencyHz = msg->cpu_frequency_ghz * 1'000'000'000.0;
  m_memoryUtilization = msg->memory_utilization;
}

void CRos2SystemHealthSubscriber::OnUPSStatus(const UPSStatus::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_lastActive = std::chrono::steady_clock::now();

  m_batteryCharge = static_cast<unsigned int>(msg->battery_charge);
  m_batteryLoadWatts = msg->load;
}
