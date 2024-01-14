/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/actions/interfaces/IActionListener.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/Stopwatch.h"

#include <map>
#include <utility>
#include <vector>

struct IntegerSettingOption;

enum SeekType
{
  SEEK_TYPE_VIDEO = 0,
  SEEK_TYPE_MUSIC = 1
};

class CSeekHandler : public ISettingCallback, public KODI::ACTION::IActionListener
{
public:
  CSeekHandler() = default;
  ~CSeekHandler() override;

  static void SettingOptionsSeekStepsFiller(const std::shared_ptr<const CSetting>& setting,
                                            std::vector<IntegerSettingOption>& list,
                                            int& current,
                                            void* data);

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  bool OnAction(const CAction &action) override;

  void Seek(bool forward, float amount, float duration = 0, bool analogSeek = false, SeekType type = SEEK_TYPE_VIDEO);
  void SeekSeconds(int seconds);
  void FrameMove();
  void Reset();
  void Configure();

  int GetSeekSize() const;
  bool InProgress() const;

  bool HasTimeCode() const { return m_timeCodePosition > 0; }
  int GetTimeCodeSeconds() const;

protected:
  CSeekHandler(const CSeekHandler&) = delete;
  CSeekHandler& operator=(CSeekHandler const&) = delete;
  bool SeekTimeCode(const CAction &action);
  void ChangeTimeCode(int remote);

private:
  static const int analogSeekDelay = 500;

  void SetSeekSize(double seekSize);
  int GetSeekStepSize(SeekType type, int step);

  int m_seekDelay = 500;
  std::map<SeekType, int > m_seekDelays;
  bool m_requireSeek = false;
  bool m_seekChanged = false;
  bool m_analogSeek = false;
  double m_seekSize = 0;
  int m_seekStep = 0;
  std::map<SeekType, std::vector<int> > m_forwardSeekSteps;
  std::map<SeekType, std::vector<int> > m_backwardSeekSteps;
  CStopWatch m_timer;
  CStopWatch m_timerTimeCode;
  int m_timeCodeStamp[6];
  int m_timeCodePosition;

  CCriticalSection m_critSection;
};
