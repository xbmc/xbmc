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
class CPVREpgTagsCache;
class CPVREpgChannelData;
class CPVREpgDatabase;
class CPVREpgInfoTag;

class CPVREpgTagsContainer
{
public:
  CPVREpgTagsContainer() = delete;
  CPVREpgTagsContainer(int iEpgID,
                       const std::shared_ptr<CPVREpgChannelData>& channelData,
                       const std::shared_ptr<CPVREpgDatabase>& database);
  virtual ~CPVREpgTagsContainer();

  /*!
   * @brief Set the EPG id for this EPG.
   * @param iEpgID The ID.
   */
  void SetEpgID(int iEpgID);

  /*!
   * @brief Set the channel data for this EPG.
   * @param data The channel data.
   */
  void SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data);

  /*!
   * @brief Update an entry.
   * @param tag The tag to update.
   * @return True if it was updated successfully, false otherwise.
   */
  bool UpdateEntry(const std::shared_ptr<CPVREpgInfoTag>& tag);

  /*!
   * @brief Delete an entry.
   * @param tag The tag to delete.
   * @return True if it was deleted successfully, false otherwise.
   */
  bool DeleteEntry(const std::shared_ptr<CPVREpgInfoTag>& tag);

  /*!
   * @brief Update all entries with the provided tags.
   * @param tags The  updated tags.
   * @return True if the update was successful, false otherwise.
   */
  bool UpdateEntries(const CPVREpgTagsContainer& tags);

  /*!
   * @brief Release all entries.
   */
  void Clear();

  /*!
   * @brief Remove all entries which were finished before the given time.
   * @param time Delete entries with an end time before this time.
   */
  void Cleanup(const CDateTime& time);

  /*!
   * @brief Check whether this container is empty.
   * @return True if the container does not contain any entries, false otherwise.
   */
  bool IsEmpty() const;

  /*!
   * @brief Get an EPG tag given its start time.
   * @param startTime The start time
   * @return The tag or nullptr if no tag was found.
   */
  std::shared_ptr<CPVREpgInfoTag> GetTag(const CDateTime& startTime) const;

  /*!
   * @brief Get an EPG tag given its unique broadcast ID.
   * @param iUniqueBroadcastID The ID.
   * @return The tag or nullptr if no tag was found.
   */
  std::shared_ptr<CPVREpgInfoTag> GetTag(unsigned int iUniqueBroadcastID) const;

  /*!
   * @brief Get the event that occurs between the given begin and end time.
   * @param start The start of the time interval.
   * @param end The end of the time interval.
   * @return The tag or nullptr if no tag was found.
   */
  std::shared_ptr<CPVREpgInfoTag> GetTagBetween(const CDateTime& start, const CDateTime& end) const;

  /*!
   * @brief Get the event that is occurring now
   * @param bUpdateIfNeeded Whether the tag should be obtained if no one was cached before.
   * @return The tag or nullptr if no tag was found.
   */
  std::shared_ptr<CPVREpgInfoTag> GetActiveTag(bool bUpdateIfNeeded) const;

  /*!
   * @brief Get the event that will occur next
   * @return The tag or nullptr if no tag was found.
   */
  std::shared_ptr<CPVREpgInfoTag> GetNextStartingTag() const;

  /*!
   * @brief Get the event that occurred previously
   * @return The tag or nullptr if no tag was found.
   */
  std::shared_ptr<CPVREpgInfoTag> GetLastEndedTag() const;

  /*!
   * @brief Get all EPG tags for the given time frame, including "gap" tags.
   * @param timelineStart Start of time line
   * @param timelineEnd End of time line
   * @param minEventEnd The minimum end time of the events to return
   * @param maxEventStart The maximum start time of the events to return
   * @return The matching tags.
   */
  std::vector<std::shared_ptr<CPVREpgInfoTag>> GetTimeline(const CDateTime& timelineStart,
                                                           const CDateTime& timelineEnd,
                                                           const CDateTime& minEventEnd,
                                                           const CDateTime& maxEventStart) const;

  /*!
   * @brief Get all EPG tags.
   * @return The tags.
   */
  std::vector<std::shared_ptr<CPVREpgInfoTag>> GetAllTags() const;

  /*!
   * @brief Get the start time of the first tag in this EPG.
   * @return The time.
   */
  CDateTime GetFirstStartTime() const;

  /*!
   * @brief Get the end time of the last tag in this EPG.
   * @return The time.
   */
  CDateTime GetLastEndTime() const;

  /*!
   * @brief Check whether this container has unsaved data.
   * @return True if this container contains unsaved data, false otherwise.
   */
  bool NeedsSave() const;

  /*!
   * @brief Persist this container in its database.
   * @param bCommit Whether to commit the data.
   */
  void Persist(bool bCommit);

  /*!
   * @brief Delete this container from its database.
   */
  void Delete();

private:
  /*!
   * @brief Complete the instance data for the given tags.
   * @param tags The tags to complete.
   * @return The completed tags.
   */
  std::vector<std::shared_ptr<CPVREpgInfoTag>> CreateEntries(
      const std::vector<std::shared_ptr<CPVREpgInfoTag>>& tags) const;

  /*!
   * @brief Complete the instance data for the given tag.
   * @param tags The tag to complete.
   * @return The completed tag.
   */
  std::shared_ptr<CPVREpgInfoTag> CreateEntry(const std::shared_ptr<CPVREpgInfoTag>& tag) const;

  /*!
   * @brief Create a "gap" tag
   * @param start The start time of the gap.
   * @param end The end time of the gap.
   * @return The tag.
   */
  std::shared_ptr<CPVREpgInfoTag> CreateGapTag(const CDateTime& start, const CDateTime& end) const;

  /*!
   * @brief Fix overlapping events.
   * @param tags The events to check/fix.
   */
  void FixOverlappingEvents(std::vector<std::shared_ptr<CPVREpgInfoTag>>& tags) const;
  void FixOverlappingEvents(std::map<CDateTime, std::shared_ptr<CPVREpgInfoTag>>& tags) const;

  int m_iEpgID = 0;
  std::shared_ptr<CPVREpgChannelData> m_channelData;
  const std::shared_ptr<CPVREpgDatabase> m_database;
  const std::unique_ptr<CPVREpgTagsCache> m_tagsCache;

  std::map<CDateTime, std::shared_ptr<CPVREpgInfoTag>> m_changedTags;
  std::map<CDateTime, std::shared_ptr<CPVREpgInfoTag>> m_deletedTags;
};

} // namespace PVR
