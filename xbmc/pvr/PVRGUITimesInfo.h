/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "threads/CriticalSection.h"

#include "pvr/PVRTypes.h"

namespace PVR
{
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

    std::string GetEpgEventDuration(const CPVREpgInfoTagPtr& epgTag, TIME_FORMAT format) const;
    std::string GetEpgEventElapsedTime(const CPVREpgInfoTagPtr& epgTag, TIME_FORMAT format) const;
    std::string GetEpgEventRemainingTime(const CPVREpgInfoTagPtr& epgTag, TIME_FORMAT format) const;
    std::string GetEpgEventFinishTime(const CPVREpgInfoTagPtr& epgTag, TIME_FORMAT format) const;
    std::string GetEpgEventSeekTime(int iSeekSize, TIME_FORMAT format) const;

    // GUI info ints
    int GetTimeshiftProgress() const;
    int GetTimeshiftProgressDuration() const;
    int GetTimeshiftProgressPlayPosition() const;
    int GetTimeshiftProgressEpgStart() const;
    int GetTimeshiftProgressEpgEnd() const;
    int GetTimeshiftProgressBufferStart() const;
    int GetTimeshiftProgressBufferEnd() const;

    int GetEpgEventDuration(const CPVREpgInfoTagPtr& epgTag) const;
    int GetEpgEventProgress(const CPVREpgInfoTagPtr& epgTag) const;

    // GUI info bools
    bool IsTimeshifting() const;

  private:
    void UpdatePlayingTag();
    void UpdateTimeshiftData();
    void UpdateTimeshiftProgressData();

    static std::string TimeToTimeString(time_t datetime, TIME_FORMAT format, bool withSeconds);

    int GetElapsedTime() const;
    int GetRemainingTime(const CPVREpgInfoTagPtr& epgTag) const;

    mutable CCriticalSection m_critSection;

    CPVREpgInfoTagPtr m_playingEpgTag;

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
