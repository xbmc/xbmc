/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2StationSubscriber.h"

#include "utils/TimeUtils.h"
#include "utils/log.h"

#include <cmath>

#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;
using std::placeholders::_1;

namespace KODI
{
namespace SMART_HOME
{
constexpr const char* ROS_NAMESPACE = "oasis"; //! @todo

//! @todo Hardware configuration
constexpr const char* STATION_HOSTNAME = "station";

constexpr const char* SUBSCRIBE_CONDUCTOR_TOPIC = "conductor_state";
constexpr const char* SUBSCRIBE_TELEMETRY_TOPIC = "system_telemetry";

constexpr unsigned int ACTIVE_TIMEOUT_SECS = 10;
} // namespace SMART_HOME
} // namespace KODI

CRos2StationSubscriber::CRos2StationSubscriber(std::shared_ptr<rclcpp::Node> node)
  : m_node(std::move(node))
{
}

CRos2StationSubscriber::~CRos2StationSubscriber() = default;

void CRos2StationSubscriber::Initialize()
{
  using ConductorState = oasis_msgs::msg::ConductorState;

  const std::string subscribeConductorTopic =
      std::string("/") + ROS_NAMESPACE + "/" + STATION_HOSTNAME + "/" + SUBSCRIBE_CONDUCTOR_TOPIC;
  const std::string subscribeTelemetryTopic =
      std::string("/") + ROS_NAMESPACE + "/" + STATION_HOSTNAME + "/" + SUBSCRIBE_TELEMETRY_TOPIC;

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeConductorTopic);
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeTelemetryTopic);

  // QoS policy
  rclcpp::SensorDataQoS qos;
  const size_t queueSize = 10;
  qos.keep_last(queueSize);

  // Subscribers
  m_conductorSubscriber = m_node->create_subscription<ConductorState>(
      subscribeConductorTopic, qos, std::bind(&CRos2StationSubscriber::OnConductorState, this, _1));
  m_telemetrySubscriber = m_node->create_subscription<SystemTelemetry>(
      subscribeTelemetryTopic, qos,
      std::bind(&CRos2StationSubscriber::OnSystemTelemetry, this, _1));
}

void CRos2StationSubscriber::Deinitialize()
{
  // Deinitialize ROS
  m_conductorSubscriber.reset();
  m_telemetrySubscriber.reset();
}

bool CRos2StationSubscriber::IsActive() const
{
  if (m_lastActiveTime > 0)
    return (CTimeUtils::GetFrameTime() - m_lastActiveTime) / 1000 < ACTIVE_TIMEOUT_SECS;

  return false;
}

void CRos2StationSubscriber::OnConductorState(const ConductorState::SharedPtr msg)
{
  // Update state
  m_supplyVoltage = msg->supply_voltage;
  m_motorVoltage = msg->motor_voltage;
  m_motorCurrent = msg->motor_current;
  m_message = msg->message;

  if (m_motorVoltage != 0.0f)
    m_lastActiveTime = CTimeUtils::GetFrameTime();
}

void CRos2StationSubscriber::OnSystemTelemetry(const SystemTelemetry::SharedPtr msg)
{
  // Update state
  m_cpuPercent = std::lround(msg->cpu_percent);
}
