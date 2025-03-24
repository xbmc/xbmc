/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>

class CPowerState
{
public:
  CPowerState() = default;
  virtual ~CPowerState() = default;

  /**
   * Called when system is going to sleep.
   */
  virtual void OnSleep() { m_sleeping.test_and_set(); }

  /**
   * Called when system is awakened from sleep.
   */
  virtual void OnWake() { m_sleeping.clear(); }

  /**
   * Test whether system is sleeping.
   */
  bool IsSleeping() const { return m_sleeping.test(); }

  /**
   * Test whether system is awakened from sleep.
   */
  bool IsAwake() const { return !IsSleeping(); }

private:
  std::atomic_flag m_sleeping{};
};
