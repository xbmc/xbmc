/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/ILabHUD.h"

#include <memory>

#include <oasis_msgs/msg/lab_state.hpp>
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
 * \brief ROS 2 subscription to the state of a LEGO lab
 */
class CRos2LabSubscriber : public ILabHUD
{
public:
  CRos2LabSubscriber(std::shared_ptr<rclcpp::Node> node);
  ~CRos2LabSubscriber();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  // Implementation of ILabHUD
  bool IsActive() const override;
  unsigned int CPUPercent() const override { return m_cpuPercent; }
  unsigned int MemoryPercent() const override { return m_memoryPercent; }
  float ShuntCurrent() const override { return m_shuntCurrent; }
  float IRVoltage() const override { return m_irVoltage; }

private:
  // ROS messages
  using LabState = oasis_msgs::msg::LabState;
  using SystemTelemetry = oasis_msgs::msg::SystemTelemetry;

  // ROS 2 subscriber callbacks
  void OnLabState(const LabState::SharedPtr msg);
  void OnSystemTelemetry(const SystemTelemetry::SharedPtr msg);

  // Construction parameters
  const std::shared_ptr<rclcpp::Node> m_node;

  // ROS parameters
  rclcpp::Subscription<LabState>::SharedPtr m_labSubscriber;
  rclcpp::Subscription<SystemTelemetry>::SharedPtr m_telemetrySubscriber;

  // GUI parameters
  unsigned int m_lastActiveTime{0};
  unsigned int m_cpuPercent{0};
  unsigned int m_memoryPercent{0};
  float m_shuntCurrent{0.0f};
  float m_irVoltage{0.0f};
};
} // namespace SMART_HOME
} // namespace KODI
