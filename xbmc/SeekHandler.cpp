/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "SeekHandler.h"

#include <cmath>
#include <stdlib.h>

#include "Application.h"
#include "cores/DataCacheCore.h"
#include "FileItem.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

CSeekHandler::CSeekHandler()
: m_seekDelay(500),
  m_requireSeek(false),
  m_analogSeek(false),
  m_seekSize(0),
  m_seekStep(0)
{
}

CSeekHandler::~CSeekHandler()
{
  m_seekDelays.clear();
  m_forwardSeekSteps.clear();
  m_backwardSeekSteps.clear();
}

CSeekHandler& CSeekHandler::GetInstance()
{
  static CSeekHandler instance;
  return instance;
}

void CSeekHandler::Configure()
{
  Reset();

  m_seekDelays.clear();
  m_seekDelays.insert(std::make_pair(SEEK_TYPE_VIDEO, CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOPLAYER_SEEKDELAY)));
  m_seekDelays.insert(std::make_pair(SEEK_TYPE_MUSIC, CServiceBroker::GetSettings().GetInt(CSettings::SETTING_MUSICPLAYER_SEEKDELAY)));

  m_forwardSeekSteps.clear();
  m_backwardSeekSteps.clear();

  std::map<SeekType, std::string> seekTypeSettingMap;
  seekTypeSettingMap.insert(std::make_pair(SEEK_TYPE_VIDEO, CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS));
  seekTypeSettingMap.insert(std::make_pair(SEEK_TYPE_MUSIC, CSettings::SETTING_MUSICPLAYER_SEEKSTEPS));

  for (std::map<SeekType, std::string>::iterator it = seekTypeSettingMap.begin(); it!=seekTypeSettingMap.end(); ++it)
  {
    std::vector<int> forwardSeekSteps;
    std::vector<int> backwardSeekSteps;

    std::vector<CVariant> seekSteps = CServiceBroker::GetSettings().GetList(it->second);
    for (std::vector<CVariant>::iterator it = seekSteps.begin(); it != seekSteps.end(); ++it)
    {
      int stepSeconds = static_cast<int>((*it).asInteger());
      if (stepSeconds < 0)
        backwardSeekSteps.insert(backwardSeekSteps.begin(), stepSeconds);
      else
        forwardSeekSteps.push_back(stepSeconds);
    }

    m_forwardSeekSteps.insert(std::make_pair(it->first, forwardSeekSteps));
    m_backwardSeekSteps.insert(std::make_pair(it->first, backwardSeekSteps));
  }
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

  std::vector<int> seekSteps(step > 0 ? m_forwardSeekSteps.at(type) : m_backwardSeekSteps.at(type));

  if (seekSteps.empty())
  {
    CLog::Log(LOGERROR, "SeekHandler - %s - No %s %s seek steps configured.", __FUNCTION__,
              (type == SeekType::SEEK_TYPE_VIDEO ? "video" : "music"), (step > 0 ? "forward" : "backward"));
    return 0;
  }

  int seconds = 0;

  // when exceeding the selected amount of steps repeat/sum up the last step size
  if (static_cast<size_t>(abs(step)) <= seekSteps.size())
    seconds = seekSteps.at(abs(step) - 1);
  else
    seconds = seekSteps.back() * (abs(step) - seekSteps.size() + 1);

  return seconds;
}

void CSeekHandler::Seek(bool forward, float amount, float duration /* = 0 */, bool analogSeek /* = false */, SeekType type /* = SEEK_TYPE_VIDEO */)
{
  CSingleLock lock(m_critSection);

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
      speed /= g_graphicsContext.GetFPS();

    double totalTime = g_application.GetTotalTime();
    if (totalTime < 0)
      totalTime = 0;

    double seekSize = (amount * amount * speed) * totalTime / 100;
    if (forward)
      m_seekSize += seekSize;
    else
      m_seekSize -= seekSize;
  }
  else
  {
    m_seekStep += forward ? 1 : -1;
    int seekSeconds = GetSeekStepSize(type, m_seekStep);
    if (seekSeconds != 0)
    {
      m_seekSize = seekSeconds;
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

  CSingleLock lock(m_critSection);
  m_seekSize = seconds;

  // perform relative seek
  g_application.m_pPlayer->SeekTimeRelative(static_cast<int64_t>(seconds * 1000));

  Reset();
}

int CSeekHandler::GetSeekSize() const
{
  return MathUtils::round_int(m_seekSize);
}

bool CSeekHandler::InProgress() const
{
  return m_requireSeek || CServiceBroker::GetDataCacheCore().IsSeeking();
}

void CSeekHandler::FrameMove()
{
  if (m_timer.GetElapsedMilliseconds() >= m_seekDelay && m_requireSeek)
  {
    CSingleLock lock(m_critSection);

    // perform relative seek
    g_application.m_pPlayer->SeekTimeRelative(static_cast<int64_t>(m_seekSize * 1000));

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
    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_STATE_CHANGED);
  }
}

void CSeekHandler::SettingOptionsSeekStepsFiller(SettingConstPtr setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  std::string label;
  for (std::vector<int>::iterator it = g_advancedSettings.m_seekSteps.begin(); it != g_advancedSettings.m_seekSteps.end(); ++it) {
    int seconds = *it;
    if (seconds > 60)
      label = StringUtils::Format(g_localizeStrings.Get(14044).c_str(), seconds / 60);
    else
      label = StringUtils::Format(g_localizeStrings.Get(14045).c_str(), seconds);

    list.insert(list.begin(), make_pair("-" + label, seconds*-1));
    list.push_back(make_pair(label, seconds));
  }
}

void CSeekHandler::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  if (setting->GetId() == CSettings::SETTING_VIDEOPLAYER_SEEKDELAY ||
      setting->GetId() == CSettings::SETTING_VIDEOPLAYER_SEEKSTEPS ||
      setting->GetId() == CSettings::SETTING_MUSICPLAYER_SEEKDELAY ||
      setting->GetId() == CSettings::SETTING_MUSICPLAYER_SEEKSTEPS)
    Configure();
}

bool CSeekHandler::OnAction(const CAction &action)
{
  if (!g_application.m_pPlayer->IsPlaying() || !g_application.m_pPlayer->CanSeek())
    return false;

  SeekType type = g_application.CurrentFileItem().IsAudio() ? SEEK_TYPE_MUSIC : SEEK_TYPE_VIDEO;

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
      g_application.m_pPlayer->Seek(false, true, action.GetID() == ACTION_CHAPTER_OR_BIG_STEP_BACK);
      return true;
    }
    case ACTION_BIG_STEP_FORWARD:
    case ACTION_CHAPTER_OR_BIG_STEP_FORWARD:
    {
      g_application.m_pPlayer->Seek(true, true, action.GetID() == ACTION_CHAPTER_OR_BIG_STEP_FORWARD);
      return true;
    }
    case ACTION_NEXT_SCENE:
    {
      g_application.m_pPlayer->SeekScene(true);
      return true;
    }
    case ACTION_PREV_SCENE:
    {
      g_application.m_pPlayer->SeekScene(false);
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
      CSingleLock lock(m_critSection);

      g_application.m_pPlayer->SeekTime(GetTimeCodeSeconds() * 1000);
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
      m_timeCodeStamp[m_timeCodePosition++] = remote - REMOTE_0;
    else
    {
      // rotate around
      for (int i = 0; i < 5; i++)
        m_timeCodeStamp[i] = m_timeCodeStamp[i + 1];
      m_timeCodeStamp[5] = remote - REMOTE_0;
    }
   }
 }

int CSeekHandler::GetTimeCodeSeconds() const
{
  if (m_timeCodePosition > 0)
  {
    // Convert the timestamp into an integer
    int tot = 0;
    for (int i = 0; i < m_timeCodePosition; i++)
      tot = tot * 10 + m_timeCodeStamp[i];

    // Interpret result as HHMMSS
    int s = tot % 100; tot /= 100;
    int m = tot % 100; tot /= 100;
    int h = tot % 100;

    return h * 3600 + m * 60 + s;
  }
  return 0;
}
