/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/IVehicleHUD.h"

#include <mutex>
#include <string>

#include <geometry_msgs/msg/twist_with_covariance_stamped.hpp>
#include <rclcpp/subscription.hpp>
#include <sensor_msgs/msg/imu.hpp>

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief ROS 2 vehicle subscriber
 */
class CRos2VehicleSubscriber
{
public:
  CRos2VehicleSubscriber(std::string rosNamespace);
  CRos2VehicleSubscriber(const CRos2VehicleSubscriber&) = delete;
  CRos2VehicleSubscriber(CRos2VehicleSubscriber&&) = delete;

  // ROS functions
  void Initialize(std::shared_ptr<rclcpp::Node> node, const std::string& vehicleName);
  void Deinitialize();

  // GUI functions
  float ForwardSpeed() const;
  float ForwardSpeedStdDev() const;
  float Roll() const;
  float RollStdDev() const;
  float Pitch() const;
  float PitchStdDev() const;
  float Yaw() const;
  float YawStdDev() const;
  float Tilt() const;
  float TiltStdDev() const;

private:
  // ROS messages
  using TwistWithCovarianceStamped = geometry_msgs::msg::TwistWithCovarianceStamped;
  using Imu = sensor_msgs::msg::Imu;

  // ROS 2 subscriber callbacks
  void OnForwardTwist(const TwistWithCovarianceStamped::SharedPtr msg);
  void OnImu(const Imu::SharedPtr msg);

  // ROS parameters
  const std::string m_rosNamespace;
  rclcpp::Subscription<TwistWithCovarianceStamped>::SharedPtr m_forwardTwistSubscriber;
  rclcpp::Subscription<Imu>::SharedPtr m_imuSubscriber;

  // GUI parameters
  float m_forwardSpeed{0.0f}; // m/s
  float m_forwardSpeedStdDev{0.0f}; // m/s
  float m_roll{0.0f}; // degrees
  float m_rollStdDev{0.0f}; // degrees
  float m_pitch{0.0f}; // degrees
  float m_pitchStdDev{0.0f}; // degrees
  float m_yaw{0.0f}; // degrees
  float m_yawStdDev{0.0f}; // degrees
  float m_tilt{0.0f}; // degrees
  float m_tiltStdDev{0.0f}; // degrees

  // Synchronization parameters
  mutable std::mutex m_mutex;
};
} // namespace SMART_HOME
} // namespace KODI
