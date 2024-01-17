/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

#include <string>
#include <vector>

namespace KODI
{
namespace JOYSTICK
{
class IInputReceiver;

/*!
 * \ingroup joystick
 */
class CRumbleGenerator : public CThread
{
public:
  CRumbleGenerator();

  ~CRumbleGenerator() override { AbortRumble(); }

  std::string ControllerID() const;

  void NotifyUser(IInputReceiver* receiver);
  bool DoTest(IInputReceiver* receiver);

  void AbortRumble(void) { StopThread(); }

protected:
  // implementation of CThread
  void Process() override;

private:
  enum RUMBLE_TYPE
  {
    RUMBLE_UNKNOWN,
    RUMBLE_NOTIFICATION,
    RUMBLE_TEST,
  };

  static std::vector<std::string> GetMotors(const std::string& controllerId);

  // Construction param
  const std::vector<std::string> m_motors;

  // Test param
  IInputReceiver* m_receiver = nullptr;
  RUMBLE_TYPE m_type = RUMBLE_UNKNOWN;
};
} // namespace JOYSTICK
} // namespace KODI
