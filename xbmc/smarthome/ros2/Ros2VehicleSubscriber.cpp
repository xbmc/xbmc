
/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2VehicleSubscriber.h"

#include "smarthome/utils/QuaternionEuler.h"
#include "smarthome/utils/RosNameUtils.h"
#include "utils/log.h"

#include <cmath>

#include <rclcpp/rclcpp.hpp>

using namespace KODI;
using namespace SMART_HOME;
using std::placeholders::_1;

namespace
{
// ROS topics
constexpr const char* SUBSCRIBE_FORWARD_TWIST_TOPIC = "ahrs/forward_twist";
constexpr const char* SUBSCRIBE_IMU_TOPIC = "ahrs/imu";
} // namespace

CRos2VehicleSubscriber::CRos2VehicleSubscriber(std::string rosNamespace)
  : m_rosNamespace(std::move(rosNamespace))
{
}

void CRos2VehicleSubscriber::Initialize(std::shared_ptr<rclcpp::Node> node,
                                        const std::string& vehicleName)
{
  // Calculate topic names
  const std::string subscribeForwardTwistTopic =
      BuildRosName(m_rosNamespace, {vehicleName, SUBSCRIBE_FORWARD_TWIST_TOPIC});
  const std::string subscribeImuTopic =
      BuildRosName(m_rosNamespace, {vehicleName, SUBSCRIBE_IMU_TOPIC});

  // Initialize ROS
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeForwardTwistTopic);
  CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", subscribeImuTopic);

  // Subscribers
  m_forwardTwistSubscriber = node->create_subscription<TwistWithCovarianceStamped>(
      subscribeForwardTwistTopic, rclcpp::SensorDataQoS{},
      std::bind(&CRos2VehicleSubscriber::OnForwardTwist, this, _1));
  m_imuSubscriber =
      node->create_subscription<Imu>(subscribeImuTopic, rclcpp::SensorDataQoS{},
                                     std::bind(&CRos2VehicleSubscriber::OnImu, this, _1));
}

void CRos2VehicleSubscriber::Deinitialize()
{
  m_forwardTwistSubscriber.reset();
  m_imuSubscriber.reset();
}

float CRos2VehicleSubscriber::ForwardSpeed() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_forwardSpeed;
}

float CRos2VehicleSubscriber::ForwardSpeedStdDev() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_forwardSpeedStdDev;
}

float CRos2VehicleSubscriber::Roll() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_roll;
}

float CRos2VehicleSubscriber::RollStdDev() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_rollStdDev;
}

float CRos2VehicleSubscriber::Pitch() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_pitch;
}

float CRos2VehicleSubscriber::PitchStdDev() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_pitchStdDev;
}

float CRos2VehicleSubscriber::Yaw() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_yaw;
}

float CRos2VehicleSubscriber::YawStdDev() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_yawStdDev;
}

float CRos2VehicleSubscriber::Tilt() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_tilt;
}

float CRos2VehicleSubscriber::TiltStdDev() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return m_tiltStdDev;
}

void CRos2VehicleSubscriber::OnForwardTwist(const TwistWithCovarianceStamped::SharedPtr msg)
{
  if (!msg)
    return;

  const double velocity = msg->twist.twist.linear.x;

  // Covariance is a 6x6 row-major matrix for twist:
  // indices: 0..5 linear (x,y,z) then angular (x,y,z)
  // variance of linear.x is covariance[0]
  double variance = 0.0;
  if (msg->twist.covariance.size() >= 1)
    variance = msg->twist.covariance[0];

  float speed = 0.0f;
  float stddev = 0.0f;

  // Validate speed
  if (std::isfinite(velocity))
    speed = static_cast<float>(velocity);

  // Validate variance and compute std dev
  if (std::isfinite(variance) && variance >= 0.0)
    stddev = static_cast<float>(std::sqrt(variance));
  else
    stddev = 0.0f; // unknown/invalid covariance

  std::lock_guard<std::mutex> lock(m_mutex);

  m_forwardSpeed = speed;
  m_forwardSpeedStdDev = stddev;
}

void CRos2VehicleSubscriber::OnImu(const Imu::SharedPtr msg)
{
  if (!msg)
    return;

  AttitudeDegrees attitude{};
  QuaternionToAttitudeDegrees(msg->orientation.x, msg->orientation.y, msg->orientation.z,
                              msg->orientation.w, attitude);

  float rollStdDev = 0.0f;
  float pitchStdDev = 0.0f;
  float yawStdDev = 0.0f;
  float tiltStdDev = 0.0f;

  // sensor_msgs/Imu.orientation_covariance is 3x3 row-major:
  // [0]=var(roll), [4]=var(pitch), [8]=var(yaw), units are rad^2.
  if (msg->orientation_covariance.size() >= 9 && msg->orientation_covariance[0] >= 0.0)
  {
    constexpr double kRadToDeg = 180.0 / std::acos(-1.0);

    const double rollVariance = msg->orientation_covariance[0];
    const double pitchVariance = msg->orientation_covariance[4];
    const double yawVariance = msg->orientation_covariance[8];

    if (std::isfinite(rollVariance) && rollVariance >= 0.0)
      rollStdDev = static_cast<float>(std::sqrt(rollVariance) * kRadToDeg);

    if (std::isfinite(pitchVariance) && pitchVariance >= 0.0)
      pitchStdDev = static_cast<float>(std::sqrt(pitchVariance) * kRadToDeg);

    if (std::isfinite(yawVariance) && yawVariance >= 0.0)
      yawStdDev = static_cast<float>(std::sqrt(yawVariance) * kRadToDeg);
  }

  const double rollDegrees = static_cast<double>(attitude.roll);
  const double pitchDegrees = static_cast<double>(attitude.pitch);
  float tilt = 0.0f;
  if (std::isfinite(rollDegrees) && std::isfinite(pitchDegrees))
    tilt = static_cast<float>(std::hypot(rollDegrees, pitchDegrees));

  if (msg->orientation_covariance.size() >= 9 && msg->orientation_covariance[0] >= 0.0)
  {
    const double rollVariance = msg->orientation_covariance[0];
    const double pitchVariance = msg->orientation_covariance[4];
    const double rollPitchCovarianceA = msg->orientation_covariance[1];
    const double rollPitchCovarianceB = msg->orientation_covariance[3];

    double rollPitchCovariance = 0.0;
    if (std::isfinite(rollPitchCovarianceA) && std::isfinite(rollPitchCovarianceB))
      rollPitchCovariance = 0.5 * (rollPitchCovarianceA + rollPitchCovarianceB);
    else if (std::isfinite(rollPitchCovarianceA))
      rollPitchCovariance = rollPitchCovarianceA;
    else if (std::isfinite(rollPitchCovarianceB))
      rollPitchCovariance = rollPitchCovarianceB;

    const double tiltDegrees = static_cast<double>(tilt);
    if (std::isfinite(rollVariance) && rollVariance >= 0.0 && std::isfinite(pitchVariance) &&
        pitchVariance >= 0.0)
    {
      double tiltVariance = 0.0;

      if (tiltDegrees > 1e-6)
      {
        const double dTiltDRoll = rollDegrees / tiltDegrees;
        const double dTiltDPitch = pitchDegrees / tiltDegrees;
        tiltVariance = dTiltDRoll * dTiltDRoll * rollVariance +
                       dTiltDPitch * dTiltDPitch * pitchVariance +
                       2.0 * dTiltDRoll * dTiltDPitch * rollPitchCovariance;
      }
      else
      {
        // The norm is not differentiable at zero tilt; fall back to the
        // dominant axis variance instead of dividing by a near-zero magnitude.
        tiltVariance = std::max(rollVariance, pitchVariance);
      }

      if (std::isfinite(tiltVariance) && tiltVariance >= 0.0)
        tiltStdDev = static_cast<float>(std::sqrt(tiltVariance) * (180.0 / std::acos(-1.0)));
    }
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  m_roll = attitude.roll;
  m_rollStdDev = rollStdDev;
  m_pitch = attitude.pitch;
  m_pitchStdDev = pitchStdDev;
  m_yaw = attitude.yaw;
  m_yawStdDev = yawStdDev;
  m_tilt = tilt;
  m_tiltStdDev = tiltStdDev;
}
