/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "utils/TimeFormat.h"

#include <memory>

namespace PVR
{
  class CPVRChannel;
  class CPVREpgInfoTag;

  class CPVRGUITimesInfo
  {
  public:
    CPVRGUITimesInfo();
    virtual ~CPVRGUITimesInfo() = default;

    void Reset();
    void Update();

    // GUI info labels
    std::string GetTimeshiftStartTime(TIME_FORMAT format) const;
    std::string GetTimeshiftEndTime(TIME_FORMAT format) const;
    std::string GetTimeshiftPlayTime(TIME_FORMAT format) const;
    std::string GetTimeshiftOffset(TIME_FORMAT format) const;
    std::string GetTimeshiftProgressDuration(TIME_FORMAT format) const;
    std::string GetTimeshiftProgressStartTime(TIME_FORMAT format) const;
    std::string GetTimeshiftProgressEndTime(TIME_FORMAT format) const;

    std::string GetEpgEventDuration(const std::shared_ptr<const CPVREpgInfoTag>& epgTag,
                                    TIME_FORMAT format) const;
    std::string GetEpgEventElapsedTime(const std::shared_ptr<const CPVREpgInfoTag>& epgTag,
                                       TIME_FORMAT format) const;
    std::string GetEpgEventRemainingTime(const std::shared_ptr<const CPVREpgInfoTag>& epgTag,
                                         TIME_FORMAT format) const;
    std::string GetEpgEventFinishTime(const std::shared_ptr<const CPVREpgInfoTag>& epgTag,
                                      TIME_FORMAT format) const;
    std::string GetEpgEventSeekTime(int iSeekSize, TIME_FORMAT format) const;

    // GUI info ints
    int GetTimeshiftProgress() const;
    int GetTimeshiftProgressDuration() const;
    int GetTimeshiftProgressPlayPosition() const;
    int GetTimeshiftProgressEpgStart() const;
    int GetTimeshiftProgressEpgEnd() const;
    int GetTimeshiftProgressBufferStart() const;
    int GetTimeshiftProgressBufferEnd() const;

    int GetEpgEventDuration(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;
    int GetEpgEventProgress(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;

    // GUI info bools
    bool IsTimeshifting() const;

  private:
    void UpdatePlayingTag();
    void UpdateTimeshiftData();
    void UpdateTimeshiftProgressData();

    static std::string TimeToTimeString(time_t datetime, TIME_FORMAT format, bool withSeconds);

    int GetElapsedTime() const;
    int GetRemainingTime(const std::shared_ptr<const CPVREpgInfoTag>& epgTag) const;

    mutable CCriticalSection m_critSection;

    std::shared_ptr<const CPVREpgInfoTag> m_playingEpgTag;
    std::shared_ptr<const CPVRChannel> m_playingChannel;

    time_t m_iStartTime;
    unsigned int m_iDuration;
    time_t m_iTimeshiftStartTime;
    time_t m_iTimeshiftEndTime;
    time_t m_iTimeshiftPlayTime;
    unsigned int m_iTimeshiftOffset;

    time_t m_iTimeshiftProgressStartTime;
    time_t m_iTimeshiftProgressEndTime;
    unsigned int m_iTimeshiftProgressDuration;
  };

} // namespace PVR
