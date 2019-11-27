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
#include <vector>

namespace PVR
{
class CPVREpgChannelData;
class CPVREpgDatabase;
class CPVREpgInfoTag;

class CPVREpgTagsContainer
{
public:
  CPVREpgTagsContainer() = delete;
  CPVREpgTagsContainer(int iEpgID, const std::shared_ptr<CPVREpgChannelData>& channelData)
    : m_iEpgID(iEpgID), m_channelData(channelData)
  {
  }

  void SetEpgID(int iEpgID);

  void SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data);

  void AddEntry(const CPVREpgInfoTag& tag);

  /*!
   * @brief Update an entry in this EPG.
   * @param tag The tag to update.
   * @param bUpdateDatabase If set to true, this event will be persisted in the database.
   * @param channelData ...
   * @param iEpgID
   * @return True if it was updated successfully, false otherwise.
   */
  bool UpdateEntry(const std::shared_ptr<CPVREpgInfoTag>& tag, bool bUpdateDatabase);

  bool DeleteEntry(const std::shared_ptr<CPVREpgInfoTag>& tag, bool bUpdateDatabase);

  bool UpdateEntries(const CPVREpgTagsContainer& tags, bool bUpdateDatabase);

  void Clear();

  void Cleanup(const CDateTime& time);

  bool IsEmpty() const;

  std::shared_ptr<CPVREpgInfoTag> GetTag(const CDateTime& time) const;

  std::shared_ptr<CPVREpgInfoTag> GetTag(unsigned int iUniqueBroadcastId) const;

  std::shared_ptr<CPVREpgInfoTag> GetTagBetween(const CDateTime& start, const CDateTime& end) const;

  std::shared_ptr<CPVREpgInfoTag> GetActiveTag(bool bUpdateIfNeeded) const;

  std::shared_ptr<CPVREpgInfoTag> GetFirstUpcomingTag() const;

  std::shared_ptr<CPVREpgInfoTag> GetLastEndedTag() const;

  std::shared_ptr<CPVREpgInfoTag> GetPredecessor(const std::shared_ptr<CPVREpgInfoTag>& tag) const;

  std::shared_ptr<CPVREpgInfoTag> GetSuccessor(const std::shared_ptr<CPVREpgInfoTag>& tag) const;

  std::shared_ptr<CPVREpgInfoTag> GetFirstTag() const;
  std::shared_ptr<CPVREpgInfoTag> GetLastTag() const;

  std::vector<std::shared_ptr<CPVREpgInfoTag>> GetTimeline(const CDateTime& timelineStart,
                                                           const CDateTime& timelineEnd,
                                                           const CDateTime& minEventEnd,
                                                           const CDateTime& maxEventStart) const;

  std::vector<std::shared_ptr<CPVREpgInfoTag>> GetAllTags() const;

  bool NeedsSave() const;

  void Persist(const std::shared_ptr<CPVREpgDatabase>& database);

private:
  /*!
   * @brief Create a "gap" tag
   * @param start The start time of the gap.
   * @param end The end time of the gap.
   * @return The tag.
   */
  std::shared_ptr<CPVREpgInfoTag> CreateGapTag(const CDateTime& start, const CDateTime& end) const;

  /*!
   * @brief Fix overlapping events from the tables.
   * @param bUpdateDatabase True, to persist any changes to tags during fixup
   * @return True if anything changed, false otherwise.
   */
  bool FixOverlappingEvents(bool bUpdateDatabase);

  int m_iEpgID = 0;
  std::shared_ptr<CPVREpgChannelData> m_channelData;
  mutable CDateTime m_nowActiveStart;

  std::map<CDateTime, std::shared_ptr<CPVREpgInfoTag>> m_tags;

  std::map<int, std::shared_ptr<CPVREpgInfoTag>> m_changedTags;
  std::map<int, std::shared_ptr<CPVREpgInfoTag>> m_deletedTags;
};

} // namespace PVR
