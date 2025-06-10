/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LockMode.h"
#include "media/MediaLockState.h"

#include <string>
#include <string_view>

namespace KODI::UTILS
{
class CLockInfo
{
public:
  CLockInfo() = default;

  /*!
  \brief The type of Lock UI to show when accessing the media source.

  Value can be:
  - LockMode::EVERYONE \n
  Default value.  No lock UI is shown, user can freely access the source.
  - LockMode::NUMERIC \n
  Lock code is entered via OSD numpad or IrDA remote buttons.
  - LockMode::GAMEPAD \n
  Lock code is entered via XBOX gamepad buttons.
  - LockMode::QWERTY \n
  Lock code is entered via OSD keyboard or PC USB keyboard.
  - LockMode::SAMBA \n
  Lock code is entered via OSD keyboard or PC USB keyboard and passed directly to SMB for authentication.
  - LockMode::EEPROM_PARENTAL \n
  Lock code is retrieved from XBOX EEPROM and entered via XBOX gamepad or remote.
  - LockMode::UNKNOWN \n
  Value is unknown or unspecified.
  */
  LockMode GetMode() const { return m_mode; }
  void SetMode(LockMode mode) { m_mode = mode; }

  const std::string& GetCode() const { return m_code; }
  void SetCode(std::string_view code) { m_code = code; }

  MediaLockState GetState() const { return m_state; }
  void SetState(MediaLockState state) { m_state = state; }

  int GetBadPasswordCount() const { return m_badPasswordCount; }
  void SetBadPasswordCount(int count) { m_badPasswordCount = count; }
  void ResetBadPasswordCount() { m_badPasswordCount = 0; }
  void IncrementBadPasswordCount() { m_badPasswordCount++; }

  void Set(LockMode mode, std::string_view code, MediaLockState state)
  {
    m_mode = mode;
    m_code = code;
    m_state = state;
  }

  bool IsLocked() const { return m_state == LOCK_STATE_LOCKED; }

private:
  LockMode m_mode{LockMode::EVERYONE};
  std::string m_code;
  MediaLockState m_state{LOCK_STATE_NO_LOCK};
  int m_badPasswordCount{0};
};
} // namespace KODI::UTILS
