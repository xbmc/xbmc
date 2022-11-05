/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace PVR
{
class CPVRTimersPath
{
public:
  static const std::string PATH_ADDTIMER;
  static const std::string PATH_NEW;
  static const std::string PATH_TV_TIMERS;
  static const std::string PATH_TV_TIMER_RULES;
  static const std::string PATH_RADIO_TIMERS;
  static const std::string PATH_RADIO_TIMER_RULES;

  explicit CPVRTimersPath(const std::string& strPath);
  CPVRTimersPath(const std::string& strPath, int iClientId, int iParentId);
  CPVRTimersPath(bool bRadio, bool bTimerRules);

  bool IsValid() const { return m_bValid; }

  const std::string& GetPath() const { return m_path; }
  bool IsTimersRoot() const { return m_bRoot; }
  bool IsTimerRule() const { return !IsTimersRoot(); }
  bool IsRadio() const { return m_bRadio; }
  bool IsRules() const { return m_bTimerRules; }
  int GetClientId() const { return m_iClientId; }
  int GetParentId() const { return m_iParentId; }

private:
  bool Init(const std::string& strPath);

  std::string m_path;
  bool m_bValid = false;
  bool m_bRoot = false;
  bool m_bRadio = false;
  bool m_bTimerRules = false;
  int m_iClientId = -1;
  int m_iParentId = 0;
};
} // namespace PVR
