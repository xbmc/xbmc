/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"

#include <map>
#include <memory>

namespace PVR
{
class CPVREpgChannelData;
class CPVREpgDatabase;
class CPVREpgInfoTag;

class CPVREpgTagsCache
{
public:
  CPVREpgTagsCache() = delete;
  CPVREpgTagsCache(int iEpgID,
                   const std::shared_ptr<CPVREpgChannelData>& channelData,
                   const std::shared_ptr<CPVREpgDatabase>& database,
                   const std::map<CDateTime, std::shared_ptr<CPVREpgInfoTag>>& changedTags)
    : m_iEpgID(iEpgID), m_channelData(channelData), m_database(database), m_changedTags(changedTags)
  {
  }

  void SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data);

  void Reset();

  std::shared_ptr<CPVREpgInfoTag> GetLastEndedTag();
  std::shared_ptr<CPVREpgInfoTag> GetNowActiveTag(bool bUpdateIfNeeded);
  std::shared_ptr<CPVREpgInfoTag> GetNextStartingTag();

private:
  void Refresh(bool bUpdateIfNeeded);
  void RefreshLastEndedTag(const CDateTime& activeTime);
  void RefreshNextStartingTag(const CDateTime& activeTime);

  int m_iEpgID;
  std::shared_ptr<CPVREpgChannelData> m_channelData;
  std::shared_ptr<CPVREpgDatabase> m_database;
  const std::map<CDateTime, std::shared_ptr<CPVREpgInfoTag>>& m_changedTags;

  std::shared_ptr<CPVREpgInfoTag> m_lastEndedTag;
  std::shared_ptr<CPVREpgInfoTag> m_nowActiveTag;
  std::shared_ptr<CPVREpgInfoTag> m_nextStartingTag;

  CDateTime m_nowActiveStart;
  CDateTime m_nowActiveEnd;
};

} // namespace PVR
