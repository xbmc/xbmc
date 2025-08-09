/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SeekHandler.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/DataCacheCore.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "music/MusicFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <numeric>
#include <stdlib.h>

using namespace KODI;

void CSeekHandler::Configure()
{
  Reset();

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_seekDelays.clear();
  m_seekDelays.emplace(SeekType::VIDEO, settings->GetInt(CSettings::SETTING_VIDEOPLAYER_SEEKDELAY));
  m_seekDelays.emplace(SeekType::MUSIC, settings->GetInt(CSettings::SETTING_MUSICPLAYER_SEEKDELAY));
  m_forwardSeekSteps.clear();
  m_backwardSeekSteps.clear();

  std::map<SeekType, std::string> seekTypeSettingMap;
  seekTypeSettingMap.try_emplace(SeekType::VIDEO, CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS);
  seekTypeSettingMap.try_emplace(SeekType::MUSIC, CSettings::SETTING_MUSICPLAYER_SEEKSTEPS);

  std::ranges::for_each(seekTypeSettingMap,
                        [&settings, &forwardSteps = m_forwardSeekSteps,
                         &backwardSteps = m_backwardSeekSteps](const auto& seekDef)
                        {
                          auto& forward = forwardSteps[seekDef.first];
                          auto& backward = backwardSteps[seekDef.first];

                          std::ranges::for_each(settings->GetList(seekDef.second),
                                                [&backward, &forward](const auto& seekStep)
                                                {
                                                  const auto stepSeconds{
                                                      static_cast<int>(seekStep.asInteger())};
                                                  if (stepSeconds < 0)
                                                    backward.insert(backward.begin(), stepSeconds);
                                                  else
                                                    forward.push_back(stepSeconds);
                                                });
                        });
}

void CSeekHandler::Reset()
{
  m_requireSeek = false;
  m_analogSeek = false;
  m_seekStep = 0;
  m_seekSize = 0;
  m_timeCodePosition = 0;
}

int CSeekHandler::GetSeekStepSize(SeekType type, int step)
{
  if (step == 0)
    return 0;

  const std::vector<int>& seekSteps(step > 0 ? m_forwardSeekSteps.at(type)
                                             : m_backwardSeekSteps.at(type));

  if (seekSteps.empty())
  {
    CLog::LogF(LOGERROR, "No {} {} seek steps configured.",
               (type == SeekType::VIDEO ? "video" : "music"), (step > 0 ? "forward" : "backward"));
    return 0;
  }

  int seconds = 0;

  // when exceeding the selected amount of steps repeat/sum up the last step size
  if (static_cast<size_t>(std::abs(step)) <= seekSteps.size())
    seconds = seekSteps.at(std::abs(step) - 1);
  else
    seconds = static_cast<int>(seekSteps.back() * (std::abs(step) - seekSteps.size() + 1));

  return seconds;
}

void CSeekHandler::Seek(bool forward, float amount, float duration /* = 0 */, bool analogSeek /* = false */, SeekType type /* = SEEK_TYPE_VIDEO */)
{
  std::unique_lock lock(m_critSection);

  // not yet seeking
  if (!m_requireSeek)
  {
    // use only the first step forward/backward for a seek without a delay
    if (!analogSeek && m_seekDelays.at(type) == 0)
    {
      SeekSeconds(GetSeekStepSize(type, forward ? 1 : -1));
      return;
    }

    m_requireSeek = true;
    m_analogSeek = analogSeek;
    m_seekDelay = analogSeek ? analogSeekDelay : m_seekDelays.at(type);
  }

  // calculate our seek amount
  if (analogSeek)
  {
    //100% over 1 second.
    float speed = 100.0f;
    if( duration )
      speed *= duration;
    else
      speed /= CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();

    double totalTime = g_application.GetTotalTime();
    if (totalTime < 0)
      totalTime = 0;

    const double seekSize = static_cast<double>(amount * amount * speed) * totalTime / 100.0;
    if (forward)
      SetSeekSize(m_seekSize + seekSize);
    else
      SetSeekSize(m_seekSize - seekSize);
  }
  else
  {
    m_seekStep += forward ? 1 : -1;
    const int seekSeconds = GetSeekStepSize(type, m_seekStep);
    if (seekSeconds != 0)
    {
      SetSeekSize(seekSeconds);
    }
    else
    {
      // nothing to do, abort seeking
      Reset();
    }
  }
  m_seekChanged = true;
  m_timer.StartZero();
}

void CSeekHandler::SeekSeconds(int seconds)
{
  // abort if we do not have a play time or already perform a seek
  if (seconds == 0)
    return;

  std::unique_lock lock(m_critSection);
  SetSeekSize(seconds);

  // perform relative seek
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  appPlayer->SeekTimeRelative(static_cast<int64_t>(seconds * 1000));

  Reset();
}

int CSeekHandler::GetSeekSize() const
{
  return MathUtils::round_int(m_seekSize);
}

void CSeekHandler::SetSeekSize(double seekSize)
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  const int64_t playTime = appPlayer->GetTime();
  const double minSeekSize{static_cast<double>(appPlayer->GetMinTime() - playTime) / 1000.0};
  const double maxSeekSize{static_cast<double>(appPlayer->GetMaxTime() - playTime) / 1000.0};

  m_seekSize = seekSize > 0
    ? std::min(seekSize, maxSeekSize)
    : std::max(seekSize, minSeekSize);
}

bool CSeekHandler::InProgress() const
{
  return m_requireSeek || CServiceBroker::GetDataCacheCore().IsSeeking();
}

void CSeekHandler::FrameMove()
{
  if (m_timer.GetElapsedMilliseconds() >= m_seekDelay && m_requireSeek)
  {
    std::unique_lock lock(m_critSection);

    // perform relative seek
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    appPlayer->SeekTimeRelative(static_cast<int64_t>(m_seekSize * 1000));

    m_seekChanged = true;

    Reset();
  }

  if (m_timeCodePosition > 0 && m_timerTimeCode.GetElapsedMilliseconds() >= 2500)
  {
    m_timeCodePosition = 0;
  }

  if (m_seekChanged)
  {
    m_seekChanged = false;
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_STATE_CHANGED);
  }
}

void CSeekHandler::SettingOptionsSeekStepsFiller(const SettingConstPtr& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current)
{
  std::string label;
  for (int seconds : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_seekSteps)
  {
    if (seconds > 60)
      label = StringUtils::Format(g_localizeStrings.Get(14044), seconds / 60);
    else
      label = StringUtils::Format(g_localizeStrings.Get(14045), seconds);

    list.insert(list.begin(), IntegerSettingOption("-" + label, seconds * -1));
    list.emplace_back(label, seconds);
  }
}

void CSeekHandler::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  if (setting->GetId() == CSettings::SETTING_VIDEOPLAYER_SEEKDELAY ||
      setting->GetId() == CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS ||
      setting->GetId() == CSettings::SETTING_MUSICPLAYER_SEEKDELAY ||
      setting->GetId() == CSettings::SETTING_MUSICPLAYER_SEEKSTEPS)
    Configure();
}

bool CSeekHandler::OnAction(const CAction &action)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlaying() || !appPlayer->CanSeek())
    return false;

  const SeekType type =
      MUSIC::IsAudio(g_application.CurrentFileItem()) ? SeekType::MUSIC : SeekType::VIDEO;

  if (SeekTimeCode(action))
    return true;

  switch (action.GetID())
  {
    case ACTION_SMALL_STEP_BACK:
    case ACTION_STEP_BACK:
    {
      Seek(false, action.GetAmount(), action.GetRepeat(), false, type);
      return true;
    }
    case ACTION_STEP_FORWARD:
    {
      Seek(true, action.GetAmount(), action.GetRepeat(), false, type);
      return true;
    }
    case ACTION_BIG_STEP_BACK:
    case ACTION_CHAPTER_OR_BIG_STEP_BACK:
    {
      appPlayer->Seek(false, true, action.GetID() == ACTION_CHAPTER_OR_BIG_STEP_BACK);
      return true;
    }
    case ACTION_BIG_STEP_FORWARD:
    case ACTION_CHAPTER_OR_BIG_STEP_FORWARD:
    {
      appPlayer->Seek(true, true, action.GetID() == ACTION_CHAPTER_OR_BIG_STEP_FORWARD);
      return true;
    }
    case ACTION_NEXT_SCENE:
    {
      appPlayer->SeekScene(Direction::FORWARD);
      return true;
    }
    case ACTION_PREV_SCENE:
    {
      appPlayer->SeekScene(Direction::BACKWARD);
      return true;
    }
    case ACTION_ANALOG_SEEK_FORWARD:
    case ACTION_ANALOG_SEEK_BACK:
    {
      if (action.GetAmount())
        Seek(action.GetID() == ACTION_ANALOG_SEEK_FORWARD, action.GetAmount(), action.GetRepeat(), true);
      return true;
    }
    case REMOTE_0:
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
    case ACTION_JUMP_SMS2:
    case ACTION_JUMP_SMS3:
    case ACTION_JUMP_SMS4:
    case ACTION_JUMP_SMS5:
    case ACTION_JUMP_SMS6:
    case ACTION_JUMP_SMS7:
    case ACTION_JUMP_SMS8:
    case ACTION_JUMP_SMS9:
    {
      if (!g_application.CurrentFileItem().IsLiveTV())
      {
        ChangeTimeCode(action.GetID());
        return true;
      }
    }
    break;
    default:
      break;
  }

  return false;
}

bool CSeekHandler::SeekTimeCode(const CAction &action)
{
  if (m_timeCodePosition <= 0)
    return false;

  switch (action.GetID())
  {
    case ACTION_SELECT_ITEM:
    case ACTION_PLAYER_PLAY:
    case ACTION_PAUSE:
    {
      std::unique_lock lock(m_critSection);

      g_application.SeekTime(GetTimeCodeSeconds());
      Reset();
      return true;
    }
    case ACTION_SMALL_STEP_BACK:
    case ACTION_STEP_BACK:
    case ACTION_BIG_STEP_BACK:
    case ACTION_CHAPTER_OR_BIG_STEP_BACK:
    case ACTION_MOVE_LEFT:
    {
      SeekSeconds(-GetTimeCodeSeconds());
      return true;
    }
    case ACTION_STEP_FORWARD:
    case ACTION_BIG_STEP_FORWARD:
    case ACTION_CHAPTER_OR_BIG_STEP_FORWARD:
    case ACTION_MOVE_RIGHT:
    {
      SeekSeconds(GetTimeCodeSeconds());
      return true;
    }
    default:
      break;
  }
  return false;
}

void CSeekHandler::ChangeTimeCode(int remote)
{
  if (remote >= ACTION_JUMP_SMS2 && remote <= ACTION_JUMP_SMS9)
  {
    // cast to REMOTE_X
    remote -= (ACTION_JUMP_SMS2 - REMOTE_2);
  }
  if (remote >= REMOTE_0 && remote <= REMOTE_9)
  {
    m_timerTimeCode.StartZero();

    if (m_timeCodePosition < 6)
    {
      m_timeCodeStamp[m_timeCodePosition] = remote - REMOTE_0;
      m_timeCodePosition++;
    }
    else
    {
      // rotate around
      std::shift_left(m_timeCodeStamp.begin(), m_timeCodeStamp.end(), 1);
      m_timeCodeStamp[5] = remote - REMOTE_0;
    }
   }
 }

int CSeekHandler::GetTimeCodeSeconds() const
{
  if (m_timeCodePosition > 0)
  {
    // Convert the timestamp into an integer
    int tot =
        std::accumulate(m_timeCodeStamp.begin(), m_timeCodeStamp.begin() + m_timeCodePosition, 0,
                        [](const int acc, const int timeCode) { return acc * 10 + timeCode; });

    // Interpret result as HHMMSS
    const int s{tot % 100};
    tot /= 100;
    const int m{tot % 100};
    tot /= 100;
    const int h{tot % 100};

    return h * 3600 + m * 60 + s;
  }
  return 0;
}
