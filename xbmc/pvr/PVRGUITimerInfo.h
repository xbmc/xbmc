/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "threads/CriticalSection.h"

namespace PVR
{
  class CPVRTimerInfoTag;

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
    virtual std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveRecordings() = 0;
    virtual std::shared_ptr<CPVRTimerInfoTag> GetNextActiveTimer() = 0;

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

    mutable CCriticalSection m_critSection;
  };

  class CPVRGUIAnyTimerInfo : public CPVRGUITimerInfo
  {
  public:
    CPVRGUIAnyTimerInfo() = default;

  private:
    int AmountActiveTimers() override;
    int AmountActiveRecordings() override;
    std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveRecordings() override;
    std::shared_ptr<CPVRTimerInfoTag> GetNextActiveTimer() override;
  };

  class CPVRGUITVTimerInfo : public CPVRGUITimerInfo
  {
  public:
    CPVRGUITVTimerInfo() = default;

  private:
    int AmountActiveTimers() override;
    int AmountActiveRecordings() override;
    std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveRecordings() override;
    std::shared_ptr<CPVRTimerInfoTag> GetNextActiveTimer() override;
  };

  class CPVRGUIRadioTimerInfo : public CPVRGUITimerInfo
  {
  public:
    CPVRGUIRadioTimerInfo() = default;

  private:
    int AmountActiveTimers() override;
    int AmountActiveRecordings() override;
    std::vector<std::shared_ptr<CPVRTimerInfoTag>> GetActiveRecordings() override;
    std::shared_ptr<CPVRTimerInfoTag> GetNextActiveTimer() override;
  };

} // namespace PVR
