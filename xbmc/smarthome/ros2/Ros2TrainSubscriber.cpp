/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2TrainSubscriber.h"

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
constexpr const char* TRAIN_HOSTNAME = "jetson"; //! @todo

constexpr const char* SUBSCRIBE_ENGINE_TOPIC = "engine_state";
constexpr const char* SUBSCRIBE_TELEMETRY_TOPIC = "system_telemetry";

constexpr unsigned int ACTIVE_TIMEOUT_SECS = 60;
} // namespace SMART_HOME
} // namespace KODI

CRos2TrainSubscriber::CRos2TrainSubscriber(std::shared_ptr<rclcpp::Node> node)
  : m_node(std::move(node))
{
}

CRos2TrainSubscriber::~CRos2TrainSubscriber() = default;

void CRos2TrainSubscriber::Initialize()
{
  using EngineState = oasis_msgs::msg::EngineState;

  const std::string subscribeEngineTopic =
      std::string("/") + ROS_NAMESPACE + "/" + TRAIN_HOSTNAME + "/" + SUBSCRIBE_ENGINE_TOPIC;
  const std::string subscribeTelemetryTopic =
      std::string("/") + ROS_NAMESPACE + "/" + TRAIN_HOSTNAME + "/" + SUBSCRIBE_TELEMETRY_TOPIC;

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeEngineTopic);
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeTelemetryTopic);

  // QoS policy
  rclcpp::SensorDataQoS qos;
  const size_t queueSize = 10;
  qos.keep_last(queueSize);

  // Subscribers
  m_engineSubscriber = m_node->create_subscription<EngineState>(
      subscribeEngineTopic, qos, std::bind(&CRos2TrainSubscriber::OnEngineState, this, _1));
  m_telemetrySubscriber = m_node->create_subscription<SystemTelemetry>(
      subscribeTelemetryTopic, qos, std::bind(&CRos2TrainSubscriber::OnSystemTelemetry, this, _1));
}

void CRos2TrainSubscriber::Deinitialize()
{
  // Deinitialize ROS
  m_engineSubscriber.reset();
  m_telemetrySubscriber.reset();
}

bool CRos2TrainSubscriber::IsActive() const
{
  if (m_lastActiveTime > 0)
    return (CTimeUtils::GetFrameTime() - m_lastActiveTime) / 1000 < ACTIVE_TIMEOUT_SECS;

  return false;
}

void CRos2TrainSubscriber::OnEngineState(const EngineState::SharedPtr msg)
{
  // Update state
  if (msg->supply_voltage != m_supplyVoltage)
    m_lastActiveTime = CTimeUtils::GetFrameTime();

  m_memoryPercent = std::lround(msg->ram_utilization);
  m_supplyVoltage = msg->supply_voltage;
  m_message = msg->message;
}

void CRos2TrainSubscriber::OnSystemTelemetry(const SystemTelemetry::SharedPtr msg)
{
  // Update state
  m_cpuPercent = std::lround(msg->cpu_percent);
}
