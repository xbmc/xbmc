/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <cmath>

namespace KODI
{
namespace SMART_HOME
{
struct AttitudeDegrees
{
  float roll{0.0f};
  float pitch{0.0f};
  float yaw{0.0f};
};

/*!
 * \brief Convert a quaternion orientation into roll, pitch, and yaw angles
 *
 * \ingroup smarthome
 *
 * The input quaternion is normalized before extracting Euler angles. Roll and
 * pitch are returned in degrees, and yaw is wrapped into the range [0, 360).
 *
 * \param qx Quaternion x component
 * \param qy Quaternion y component
 * \param qz Quaternion z component
 * \param qw Quaternion w component
 * \param attitude Output roll, pitch, and yaw angles in degrees
 *
 * \return True if the quaternion is valid and the extracted attitude is finite,
 *         false otherwise
 */
inline bool QuaternionToAttitudeDegrees(
    double qx, double qy, double qz, double qw, AttitudeDegrees& attitude)
{
  const double norm2 = qx * qx + qy * qy + qz * qz + qw * qw;
  if (!std::isfinite(norm2) || norm2 <= 0.0)
    return false;

  const double invNorm = 1.0 / std::sqrt(norm2);
  const double x = qx * invNorm;
  const double y = qy * invNorm;
  const double z = qz * invNorm;
  const double w = qw * invNorm;

  constexpr double kRadToDeg = 180.0 / std::acos(-1.0);

  // REP 145 defines sensor_msgs/Imu.orientation as the sensor frame orientation
  // with respect to the world frame. Extract RPY directly from that quaternion;
  // conjugating here would instead interpret a world-to-sensor quaternion and
  // introduces cross-coupling for vehicle-facing roll/pitch displays.
  const double sinrCosp = 2.0 * (w * x + y * z);
  const double cosrCosp = 1.0 - 2.0 * (x * x + y * y);
  const double sinp = std::clamp(2.0 * (w * y - z * x), -1.0, 1.0);
  const double sinyCosp = 2.0 * (w * z + x * y);
  const double cosyCosp = 1.0 - 2.0 * (y * y + z * z);

  attitude.roll = static_cast<float>(std::atan2(sinrCosp, cosrCosp) * kRadToDeg);
  attitude.pitch = static_cast<float>(std::asin(sinp) * kRadToDeg);

  const double yawDegrees = std::atan2(sinyCosp, cosyCosp) * kRadToDeg;
  attitude.yaw = static_cast<float>(yawDegrees < 0.0 ? yawDegrees + 360.0 : yawDegrees);

  return std::isfinite(attitude.roll) && std::isfinite(attitude.pitch) &&
         std::isfinite(attitude.yaw);
}
} // namespace SMART_HOME
} // namespace KODI
