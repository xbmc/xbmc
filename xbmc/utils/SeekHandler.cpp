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

#include <stdlib.h>
#include "guilib/LocalizeStrings.h"
#include "GUIInfoManager.h"
#include "Application.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

CSeekHandler::CSeekHandler()
: m_seekDelay(500),
  m_requireSeek(false),
  m_percent(0.0f),
  m_percentPlayTime(0.0f),
  m_analogSeek(false),
  m_seekStep(0)
{
}

CSeekHandler::~CSeekHandler()
{
  m_seekDelays.clear();
  m_forwardSeekSteps.clear();
  m_backwardSeekSteps.clear();
}

CSeekHandler& CSeekHandler::Get()
{
  static CSeekHandler instance;
  return instance;
}

void CSeekHandler::Reset()
{
  m_requireSeek = false;
  m_percent = 0;
  m_seekStep = 0;

  m_seekDelays.clear();
  m_seekDelays.insert(std::make_pair(SEEK_TYPE_VIDEO, CSettings::Get().GetInt("videoplayer.seekdelay")));
  m_seekDelays.insert(std::make_pair(SEEK_TYPE_MUSIC, CSettings::Get().GetInt("musicplayer.seekdelay")));

  m_forwardSeekSteps.clear();
  m_backwardSeekSteps.clear();

  std::map<SeekType, std::string> seekTypeSettingMap;
  seekTypeSettingMap.insert(std::make_pair(SEEK_TYPE_VIDEO, "videoplayer.seeksteps"));
  seekTypeSettingMap.insert(std::make_pair(SEEK_TYPE_MUSIC, "musicplayer.seeksteps"));

  for (std::map<SeekType, std::string>::iterator it = seekTypeSettingMap.begin(); it!=seekTypeSettingMap.end(); ++it)
  {
    std::vector<int> forwardSeekSteps;
    std::vector<int> backwardSeekSteps;

    std::vector<CVariant> seekSteps = CSettings::Get().GetList(it->second);
    for (std::vector<CVariant>::iterator it = seekSteps.begin(); it != seekSteps.end(); ++it)
    {
      int stepSeconds = (*it).asInteger();
      if (stepSeconds < 0)
        backwardSeekSteps.insert(backwardSeekSteps.begin(), stepSeconds);
      else
        forwardSeekSteps.push_back(stepSeconds);
    }

    m_forwardSeekSteps.insert(std::make_pair(it->first, forwardSeekSteps));
    m_backwardSeekSteps.insert(std::make_pair(it->first, backwardSeekSteps));
  }
}

int CSeekHandler::GetSeekSeconds(bool forward, SeekType type)
{
  m_seekStep = m_seekStep + (forward ? 1 : -1);

  if (m_seekStep == 0)
    return 0;

  std::vector<int> seekSteps(m_seekStep > 0 ? m_forwardSeekSteps.at(type) : m_backwardSeekSteps.at(type));

  if (seekSteps.empty())
  {
    CLog::Log(LOGERROR, "SeekHandler - %s - No %s %s seek steps configured.", __FUNCTION__,
              (type == SeekType::SEEK_TYPE_VIDEO ? "video" : "music"), (m_seekStep > 0 ? "forward" : "backward"));
    return 0;
  }

  int seconds = 0;

  // when exceeding the selected amount of steps repeat/sum up the last step size
  if (static_cast<size_t>(abs(m_seekStep)) <= seekSteps.size())
    seconds = seekSteps.at(abs(m_seekStep) - 1);
  else
    seconds = seekSteps.back() * (abs(m_seekStep) - seekSteps.size() + 1);

  return seconds;
}

void CSeekHandler::Seek(bool forward, float amount, float duration /* = 0 */, bool analogSeek /* = false */, SeekType type /* = SEEK_TYPE_VIDEO */)
{
  // not yet seeking
  if (!m_requireSeek)
  {
    if (g_infoManager.GetTotalPlayTime())
      m_percent = static_cast<float>(g_infoManager.GetPlayTime()) / g_infoManager.GetTotalPlayTime() * 0.1f;
    else
      m_percent = 0.0f;
    m_percentPlayTime = m_percent;

    // tell info manager that we have started a seek operation
    m_requireSeek = true;
    g_infoManager.SetSeeking(true);
    m_seekStep = 0;
    m_analogSeek = analogSeek;
    m_seekDelay = analogSeek ? analogSeekDelay : m_seekDelays.at(type);
  }

  // calculate our seek amount
  if (!g_infoManager.m_performingSeek)
  {
    if (analogSeek)
    {
      //100% over 1 second.
      float speed = 100.0f;
      if( duration )
        speed *= duration;
      else
        speed /= g_infoManager.GetFPS();

      if (forward)
        m_percent += amount * amount * speed;
      else
        m_percent -= amount * amount * speed;
    }
    else
    {
      int seekSeconds = GetSeekSeconds(forward, type);
      if (seekSeconds != 0)
      {
        float percentPerSecond = 0.0f;
        if (g_infoManager.GetTotalPlayTime())
          percentPerSecond = 100.0f / static_cast<float>(g_infoManager.GetTotalPlayTime());

        m_percent = m_percentPlayTime + percentPerSecond * seekSeconds;

        g_infoManager.SetSeekStepSize(seekSeconds);
      }
      else
      {
        // nothing to do, abort seeking
        m_requireSeek = false;
        g_infoManager.SetSeeking(false);
      }
    }

    if (m_percent > 100.0f)
      m_percent = 100.0f;
    if (m_percent < 0.0f)
      m_percent = 0.0f;
  }

  m_timer.StartZero();
}

void CSeekHandler::SeekSeconds(int seconds)
{
  if (seconds == 0 || g_infoManager.GetTotalPlayTime() == 0)
    return;

  CSingleLock lock(m_critSection);

  m_requireSeek = true;
  m_seekDelay = 0;

  g_infoManager.SetSeeking(true);
  g_infoManager.SetSeekStepSize(seconds);

  float percentPlayTime = static_cast<float>(g_infoManager.GetPlayTime()) / g_infoManager.GetTotalPlayTime() * 0.1f;
  float percentPerSecond = 100.0f / static_cast<float>(g_infoManager.GetTotalPlayTime());

  m_percent = percentPlayTime + percentPerSecond * seconds;

  if (m_percent > 100.0f)
    m_percent = 100.0f;
  if (m_percent < 0.0f)
    m_percent = 0.0f;

  m_timer.StartZero();
}

float CSeekHandler::GetPercent() const
{
  return m_percent;
}

bool CSeekHandler::InProgress() const
{
  return m_requireSeek;
}

void CSeekHandler::Process()
{
  if (m_timer.GetElapsedMilliseconds() > m_seekDelay && m_requireSeek)
  {
    g_infoManager.m_performingSeek = true;

    // reset seek step size
    g_infoManager.SetSeekStepSize(0);

    // calculate the seek time
    double time = g_infoManager.GetTotalPlayTime() * m_percent * 0.01;

    g_application.SeekTime(time);
    m_requireSeek = false;
    g_infoManager.SetSeeking(false);
  }
}

void CSeekHandler::SettingOptionsSeekStepsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
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

void CSeekHandler::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  if (setting->GetId() == "videoplayer.seekdelay" ||
      setting->GetId() == "videoplayer.seeksteps" ||
      setting->GetId() == "musicplayer.seekdelay" ||
      setting->GetId() == "musicplayer.seeksteps")
    Reset();
}

bool CSeekHandler::OnAction(const CAction &action)
{
  if (!g_application.m_pPlayer->IsPlaying() || !g_application.m_pPlayer->CanSeek())
    return false;

  SeekType type = g_application.CurrentFileItem().IsAudio() ? SEEK_TYPE_MUSIC : SEEK_TYPE_VIDEO;

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
      if (g_application.m_pPlayer->SeekScene(true))
        g_infoManager.SetDisplayAfterSeek();
      return true;
    }
    case ACTION_PREV_SCENE:
    {
      if (g_application.m_pPlayer->SeekScene(false))
        g_infoManager.SetDisplayAfterSeek();
      return true;
    }
    case ACTION_ANALOG_SEEK_FORWARD:
    case ACTION_ANALOG_SEEK_BACK:
    {
      if (action.GetAmount())
        Seek(action.GetID() == ACTION_ANALOG_SEEK_FORWARD, action.GetAmount(), action.GetRepeat(), true);
      return true;
    }
    default:
      break;
  }

  return false;
}
