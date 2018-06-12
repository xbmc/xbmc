/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "threads/CriticalSection.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace PVR
{
  class CPVRGUITimerInfo
  {
  public:
    CPVRGUITimerInfo();
    virtual ~CPVRGUITimerInfo() = default;

    void ResetProperties();

    void UpdateTimersCache();
    void UpdateTimersToggle();
    void UpdateNextTimer();

    const std::string& GetActiveTimerTitle() const;
    const std::string& GetActiveTimerChannelName() const;
    const std::string& GetActiveTimerChannelIcon() const;
    const std::string& GetActiveTimerDateTime() const;
    const std::string& GetNextTimerTitle() const;
    const std::string& GetNextTimerChannelName() const;
    const std::string& GetNextTimerChannelIcon() const;
    const std::string& GetNextTimerDateTime() const;
    const std::string& GetNextTimer() const;

    bool HasTimers() const { return m_iTimerAmount > 0; }
    bool HasRecordingTimers() const { return m_iRecordingTimerAmount > 0; }
    bool HasNonRecordingTimers() const { return m_iTimerAmount - m_iRecordingTimerAmount > 0; }

  private:
    bool TimerInfoToggle();

    virtual int AmountActiveTimers() = 0;
    virtual int AmountActiveRecordings() = 0;
    virtual std::vector<CFileItemPtr> GetActiveRecordings() = 0;
    virtual CFileItemPtr GetNextActiveTimer() = 0;

    unsigned int m_iTimerAmount;
    unsigned int m_iRecordingTimerAmount;

    std::string m_strActiveTimerTitle;
    std::string m_strActiveTimerChannelName;
    std::string m_strActiveTimerChannelIcon;
    std::string m_strActiveTimerTime;
    std::string m_strNextRecordingTitle;
    std::string m_strNextRecordingChannelName;
    std::string m_strNextRecordingChannelIcon;
    std::string m_strNextRecordingTime;
    std::string m_strNextTimerInfo;

    unsigned int m_iTimerInfoToggleStart;
    unsigned int m_iTimerInfoToggleCurrent;

    CCriticalSection m_critSection;
  };

  class CPVRGUIAnyTimerInfo : public CPVRGUITimerInfo
  {
  public:
    CPVRGUIAnyTimerInfo() = default;

  private:
    int AmountActiveTimers() override;
    int AmountActiveRecordings() override;
    std::vector<CFileItemPtr> GetActiveRecordings() override;
    CFileItemPtr GetNextActiveTimer() override;
  };

  class CPVRGUITVTimerInfo : public CPVRGUITimerInfo
  {
  public:
    CPVRGUITVTimerInfo() = default;

  private:
    int AmountActiveTimers() override;
    int AmountActiveRecordings() override;
    std::vector<CFileItemPtr> GetActiveRecordings() override;
    CFileItemPtr GetNextActiveTimer() override;
  };

  class CPVRGUIRadioTimerInfo : public CPVRGUITimerInfo
  {
  public:
    CPVRGUIRadioTimerInfo() = default;

  private:
    int AmountActiveTimers() override;
    int AmountActiveRecordings() override;
    std::vector<CFileItemPtr> GetActiveRecordings() override;
    CFileItemPtr GetNextActiveTimer() override;
  };

} // namespace PVR
