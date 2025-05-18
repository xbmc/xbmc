/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/ITrainHUD.h"

#include <memory>

#include <oasis_msgs/msg/engine_state.hpp>
#include <oasis_msgs/msg/system_telemetry.hpp>
#include <rclcpp/subscription.hpp>

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief ROS 2 subscription to the state of a LEGO train
 */
class CRos2TrainSubscriber : public ITrainHUD
{
public:
  CRos2TrainSubscriber(std::shared_ptr<rclcpp::Node> node);
  ~CRos2TrainSubscriber();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  // Implementation of ITrainHUD
  bool IsActive() const override;
  unsigned int CPUPercent() const override { return m_cpuPercent; }
  unsigned int MemoryPercent() const override { return m_memoryPercent; }
  float SupplyVoltage() const override { return m_supplyVoltage; }
  const std::string& Message() const override { return m_message; }

private:
  // ROS messages
  using EngineState = oasis_msgs::msg::EngineState;
  using SystemTelemetry = oasis_msgs::msg::SystemTelemetry;

  // ROS 2 subscriber callbacks
  void OnEngineState(const EngineState::SharedPtr msg);
  void OnSystemTelemetry(const SystemTelemetry::SharedPtr msg);

  // Construction parameters
  const std::shared_ptr<rclcpp::Node> m_node;

  // ROS parameters
  rclcpp::Subscription<EngineState>::SharedPtr m_engineSubscriber;
  rclcpp::Subscription<SystemTelemetry>::SharedPtr m_telemetrySubscriber;

  // GUI parameters
  unsigned int m_lastActiveTime{0};
  unsigned int m_cpuPercent{0};
  unsigned int m_memoryPercent{0};
  float m_supplyVoltage{0.0f};
  std::string m_message;
};
} // namespace SMART_HOME
} // namespace KODI
