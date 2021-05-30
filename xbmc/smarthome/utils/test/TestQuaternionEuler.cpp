/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "smarthome/utils/QuaternionEuler.h"

#include <cmath>

#include <gtest/gtest.h>

using namespace KODI::SMART_HOME;

namespace
{
constexpr double DEG_TO_RAD = std::acos(-1.0) / 180.0;

struct Quaternion
{
  double x{0.0};
  double y{0.0};
  double z{0.0};
  double w{1.0};
};

Quaternion Normalize(Quaternion quaternion)
{
  const double norm = std::sqrt(quaternion.x * quaternion.x + quaternion.y * quaternion.y +
                                quaternion.z * quaternion.z + quaternion.w * quaternion.w);
  quaternion.x /= norm;
  quaternion.y /= norm;
  quaternion.z /= norm;
  quaternion.w /= norm;
  return quaternion;
}

Quaternion Multiply(const Quaternion& a, const Quaternion& b)
{
  return {
      a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
      a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
  };
}

Quaternion RollDegrees(double degrees)
{
  const double half = degrees * DEG_TO_RAD * 0.5;
  return {std::sin(half), 0.0, 0.0, std::cos(half)};
}

Quaternion PitchDegrees(double degrees)
{
  const double half = degrees * DEG_TO_RAD * 0.5;
  return {0.0, std::sin(half), 0.0, std::cos(half)};
}

Quaternion YawDegrees(double degrees)
{
  const double half = degrees * DEG_TO_RAD * 0.5;
  return {0.0, 0.0, std::sin(half), std::cos(half)};
}

Quaternion Conjugate(const Quaternion& quaternion)
{
  return {-quaternion.x, -quaternion.y, -quaternion.z, quaternion.w};
}

AttitudeDegrees Extract(const Quaternion& quaternion)
{
  AttitudeDegrees attitude{};
  EXPECT_TRUE(QuaternionToAttitudeDegrees(quaternion.x, quaternion.y, quaternion.z, quaternion.w,
                                          attitude));
  return attitude;
}
} // namespace

TEST(TestQuaternionEuler, RejectsZeroQuaternion)
{
  AttitudeDegrees attitude{};
  EXPECT_FALSE(QuaternionToAttitudeDegrees(0.0, 0.0, 0.0, 0.0, attitude));
}

TEST(TestQuaternionEuler, RejectsNonFiniteQuaternion)
{
  AttitudeDegrees attitude{};
  EXPECT_FALSE(QuaternionToAttitudeDegrees(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 1.0,
                                           attitude));
}

TEST(TestQuaternionEuler, IdentityQuaternionProducesZeroAttitude)
{
  const AttitudeDegrees attitude = Extract({});

  EXPECT_NEAR(attitude.roll, 0.0, 1e-6);
  EXPECT_NEAR(attitude.pitch, 0.0, 1e-6);
  EXPECT_NEAR(attitude.yaw, 0.0, 1e-6);
}

TEST(TestQuaternionEuler, PureRollIsExtractedWithoutCrossCoupling)
{
  const AttitudeDegrees roll = Extract(RollDegrees(20.0));
  EXPECT_NEAR(roll.roll, 20.0, 1e-3);
  EXPECT_NEAR(roll.pitch, 0.0, 1e-3);
  EXPECT_NEAR(roll.yaw, 0.0, 1e-3);
}

TEST(TestQuaternionEuler, PurePitchIsExtractedWithoutCrossCoupling)
{
  const AttitudeDegrees pitch = Extract(PitchDegrees(20.0));
  EXPECT_NEAR(pitch.roll, 0.0, 1e-3);
  EXPECT_NEAR(pitch.pitch, 20.0, 1e-3);
  EXPECT_NEAR(pitch.yaw, 0.0, 1e-3);
}

TEST(TestQuaternionEuler, PureYawWrapsIntoPositiveRange)
{
  const AttitudeDegrees yaw = Extract(YawDegrees(90.0));
  EXPECT_NEAR(yaw.roll, 0.0, 1e-3);
  EXPECT_NEAR(yaw.pitch, 0.0, 1e-3);
  EXPECT_NEAR(yaw.yaw, 90.0, 1e-3);
}

TEST(TestQuaternionEuler, NegativeYawWrapsToThreeSixtyRange)
{
  const AttitudeDegrees yaw = Extract(YawDegrees(-90.0));
  EXPECT_NEAR(yaw.roll, 0.0, 1e-3);
  EXPECT_NEAR(yaw.pitch, 0.0, 1e-3);
  EXPECT_NEAR(yaw.yaw, 270.0, 1e-3);
}

TEST(TestQuaternionEuler, CombinedOrientationMatchesConstructionOrder)
{
  Quaternion orientation =
      Multiply(YawDegrees(30.0), Multiply(PitchDegrees(10.0), RollDegrees(-15.0)));
  orientation = Normalize(orientation);

  const AttitudeDegrees attitude = Extract(orientation);

  EXPECT_NEAR(attitude.roll, -15.0, 1e-3);
  EXPECT_NEAR(attitude.pitch, 10.0, 1e-3);
  EXPECT_NEAR(attitude.yaw, 30.0, 1e-3);
}

TEST(TestQuaternionEuler, NonNormalizedQuaternionIsHandled)
{
  Quaternion orientation = Multiply(YawDegrees(45.0), RollDegrees(10.0));
  orientation.x *= 5.0;
  orientation.y *= 5.0;
  orientation.z *= 5.0;
  orientation.w *= 5.0;

  const AttitudeDegrees attitude = Extract(orientation);

  EXPECT_NEAR(attitude.roll, 10.0, 1e-3);
  EXPECT_NEAR(attitude.pitch, 0.0, 1e-3);
  EXPECT_NEAR(attitude.yaw, 45.0, 1e-3);
}

TEST(TestQuaternionEuler, NearHalfTurnBodyRollStaysVehicleFacing)
{
  Quaternion orientation = Multiply(YawDegrees(170.0), RollDegrees(8.0));
  orientation = Normalize(orientation);

  const AttitudeDegrees direct = Extract(orientation);
  const AttitudeDegrees conjugated = Extract(Conjugate(orientation));

  EXPECT_NEAR(direct.roll, 8.0, 1e-3);
  EXPECT_NEAR(direct.pitch, 0.0, 1e-3);
  EXPECT_NEAR(direct.yaw, 170.0, 1e-3);

  EXPECT_GT(std::abs(conjugated.pitch), 3.0);
}
