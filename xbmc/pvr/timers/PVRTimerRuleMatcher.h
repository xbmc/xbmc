/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include <memory>

class CRegExp;

namespace PVR
{
class CPVRChannel;
class CPVRTimerInfoTag;
class CPVREpgInfoTag;

class CPVRTimerRuleMatcher
{
public:
  CPVRTimerRuleMatcher(const std::shared_ptr<CPVRTimerInfoTag>& timerRule, const CDateTime& start);
  virtual ~CPVRTimerRuleMatcher();

  std::shared_ptr<CPVRTimerInfoTag> GetTimerRule() const { return m_timerRule; }

  std::shared_ptr<const CPVRChannel> GetChannel() const;
  CDateTime GetNextTimerStart() const;
  bool Matches(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;

private:
  bool MatchSeriesLink(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;
  bool MatchChannel(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;
  bool MatchStart(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;
  bool MatchEnd(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;
  bool MatchDayOfWeek(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;
  bool MatchSearchText(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;

  const std::shared_ptr<CPVRTimerInfoTag> m_timerRule;
  CDateTime m_start;
  mutable std::unique_ptr<CRegExp> m_textSearch;
};
} // namespace PVR
