/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/IStationHUD.h"

#include <memory>

#include <oasis_msgs/msg/conductor_state.hpp>
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
 * \brief ROS 2 subscription to the state of a LEGO train power station
 */
class CRos2StationSubscriber : public IStationHUD
{
public:
  CRos2StationSubscriber(std::shared_ptr<rclcpp::Node> node);
  ~CRos2StationSubscriber();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  // Implementation of IStationHUD
  bool IsActive() const override;
  float SupplyVoltage() const override { return m_supplyVoltage; }
  float MotorVoltage() const override { return m_motorVoltage; }
  float MotorCurrent() const override { return m_motorCurrent; }
  unsigned int CPUPercent() const override { return m_cpuPercent; }
  const std::string& Message() const override { return m_message; }

private:
  // ROS messages
  using ConductorState = oasis_msgs::msg::ConductorState;
  using SystemTelemetry = oasis_msgs::msg::SystemTelemetry;

  // ROS 2 subscriber callbacks
  void OnConductorState(const ConductorState::SharedPtr msg);
  void OnSystemTelemetry(const SystemTelemetry::SharedPtr msg);

  // Construction parameters
  const std::shared_ptr<rclcpp::Node> m_node;

  // ROS parameters
  rclcpp::Subscription<ConductorState>::SharedPtr m_conductorSubscriber;
  rclcpp::Subscription<SystemTelemetry>::SharedPtr m_telemetrySubscriber;

  // GUI parameters
  unsigned int m_lastActiveTime{0};
  float m_supplyVoltage{0.0f};
  float m_motorVoltage{0.0f};
  float m_motorCurrent{0.0f};
  unsigned int m_cpuPercent{0};
  std::string m_message;
};
} // namespace SMART_HOME
} // namespace KODI
