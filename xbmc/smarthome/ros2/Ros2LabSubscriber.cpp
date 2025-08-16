/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2LabSubscriber.h"

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
constexpr const char* LAB_HOSTNAME = "starship"; //! @todo

constexpr const char* SUBSCRIBE_LAB_TOPIC = "lab_state";
constexpr const char* SUBSCRIBE_TELEMETRY_TOPIC = "system_telemetry";

constexpr unsigned int ACTIVE_TIMEOUT_SECS = 10;
} // namespace SMART_HOME
} // namespace KODI

CRos2LabSubscriber::CRos2LabSubscriber(std::shared_ptr<rclcpp::Node> node) : m_node(std::move(node))
{
}

CRos2LabSubscriber::~CRos2LabSubscriber() = default;

void CRos2LabSubscriber::Initialize()
{
  using LabState = oasis_msgs::msg::LabState;

  const std::string subscribeLabTopic =
      std::string("/") + ROS_NAMESPACE + "/" + LAB_HOSTNAME + "/" + SUBSCRIBE_LAB_TOPIC;
  const std::string subscribeTelemetryTopic =
      std::string("/") + ROS_NAMESPACE + "/" + LAB_HOSTNAME + "/" + SUBSCRIBE_TELEMETRY_TOPIC;

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeLabTopic);
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeTelemetryTopic);

  // QoS policy
  rclcpp::SensorDataQoS qos;
  const size_t queueSize = 10;
  qos.keep_last(queueSize);

  // Subscribers
  m_labSubscriber = m_node->create_subscription<LabState>(
      subscribeLabTopic, qos, std::bind(&CRos2LabSubscriber::OnLabState, this, _1));
  m_telemetrySubscriber = m_node->create_subscription<SystemTelemetry>(
      subscribeTelemetryTopic, qos, std::bind(&CRos2LabSubscriber::OnSystemTelemetry, this, _1));
}

void CRos2LabSubscriber::Deinitialize()
{
  // Deinitialize ROS
  m_labSubscriber.reset();
  m_telemetrySubscriber.reset();
}

bool CRos2LabSubscriber::IsActive() const
{
  if (m_lastActiveTime > 0)
    return (CTimeUtils::GetFrameTime() - m_lastActiveTime) / 1000 < ACTIVE_TIMEOUT_SECS;

  return false;
}

void CRos2LabSubscriber::OnLabState(const LabState::SharedPtr msg)
{
  // Update state
  if (msg->shunt_current != m_shuntCurrent)
    m_lastActiveTime = CTimeUtils::GetFrameTime();

  m_memoryPercent = msg->ram_utilization;
  m_shuntCurrent = msg->shunt_current;
  m_irVoltage = msg->ir_vout;
}

void CRos2LabSubscriber::OnSystemTelemetry(const SystemTelemetry::SharedPtr msg)
{
  // Update state
  m_cpuPercent = std::lround(msg->cpu_percent);
}
