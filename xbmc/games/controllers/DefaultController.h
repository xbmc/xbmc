/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace GAME
{
class CDefaultController
{
public:
  // Face buttons
  static const char* FEATURE_A;
  static const char* FEATURE_B;
  static const char* FEATURE_X;
  static const char* FEATURE_Y;
  static const char* FEATURE_START;
  static const char* FEATURE_BACK;
  static const char* FEATURE_GUIDE;
  static const char* FEATURE_UP;
  static const char* FEATURE_RIGHT;
  static const char* FEATURE_DOWN;
  static const char* FEATURE_LEFT;
  static const char* FEATURE_LEFT_THUMB;
  static const char* FEATURE_RIGHT_THUMB;

  // Shoulder buttons
  static const char* FEATURE_LEFT_BUMPER;
  static const char* FEATURE_RIGHT_BUMPER;

  // Triggers
  static const char* FEATURE_LEFT_TRIGGER;
  static const char* FEATURE_RIGHT_TRIGGER;

  // Analog sticks
  static const char* FEATURE_LEFT_STICK;
  static const char* FEATURE_RIGHT_STICK;

  // Haptics
  static const char* FEATURE_LEFT_MOTOR;
  static const char* FEATURE_RIGHT_MOTOR;
};
} // namespace GAME
} // namespace KODI
