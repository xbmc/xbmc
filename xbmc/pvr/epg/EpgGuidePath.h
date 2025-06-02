/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include <string>

namespace PVR
{
class CPVREpgGuidePath
{
public:
  CPVREpgGuidePath(int epgId, const CDateTime& startDateTime);
  explicit CPVREpgGuidePath(const std::string& path);

  bool IsValid() const { return m_isValid; }

  const std::string& AsString() const& { return m_path; }
  std::string AsString() && { return std::move(m_path); }

  int GetEpgId() const { return m_epgId; }
  CDateTime GetStartDateTime() const { return m_startDateTime; }

private:
  std::string m_path;
  bool m_isValid{false};
  int m_epgId{0};
  CDateTime m_startDateTime;
};
} // namespace PVR
