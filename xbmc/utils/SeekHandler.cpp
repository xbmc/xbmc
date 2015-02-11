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
#include "guilib/LocalizeStrings.h"
#include "GUIInfoManager.h"
#include "Application.h"
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
  m_seekDelay = CSettings::Get().GetInt("videoplayer.seekdelay");

  std::vector<CVariant> seekSteps = CSettings::Get().GetList("videoplayer.seeksteps");
  m_forwardSeekSteps.clear();
  m_backwardSeekSteps.clear();
  for (std::vector<CVariant>::iterator it = seekSteps.begin(); it != seekSteps.end(); ++it) {
    int stepSeconds = (*it).asInteger();
    if (stepSeconds < 0)
      m_backwardSeekSteps.insert(m_backwardSeekSteps.begin(), stepSeconds);
    else
      m_forwardSeekSteps.push_back(stepSeconds);
  }
}

int CSeekHandler::GetSeekSeconds(bool forward)
{
  m_seekStep = m_seekStep + (forward ? 1 : -1);

  int seconds = 0;
  if (m_seekStep > 0)
  {
    // when exceeding the selected amount of steps repeat/sum up the last step size
    if ((size_t)m_seekStep <= m_forwardSeekSteps.size())
      seconds = m_forwardSeekSteps.at(m_seekStep - 1);
    else
      seconds = m_forwardSeekSteps.back() * (m_seekStep - m_forwardSeekSteps.size() + 1);
  }
  else if (m_seekStep < 0)
  {
    // when exceeding the selected amount of steps repeat/sum up the last step size
    if ((size_t)m_seekStep*-1 <= m_backwardSeekSteps.size())
      seconds = m_backwardSeekSteps.at((m_seekStep*-1) - 1);
    else
      seconds = m_backwardSeekSteps.back() * ((m_seekStep*-1) - m_backwardSeekSteps.size() + 1);
  }

  return seconds;
}

void CSeekHandler::Seek(bool forward, float amount, float duration /* = 0 */, bool analogSeek /* = false */)
{
  // not yet seeking
  if (!m_requireSeek)
  {
    if (g_infoManager.GetTotalPlayTime())
      m_percent = (float)g_infoManager.GetPlayTime() / g_infoManager.GetTotalPlayTime() * 0.1f;
    else
      m_percent = 0.0f;
    m_percentPlayTime = m_percent;

    // tell info manager that we have started a seek operation
    m_requireSeek = true;
    g_infoManager.SetSeeking(true);
    m_seekStep = 0;
    m_analogSeek = analogSeek;

    if (!analogSeek)
    {
      // don't apply a seek delay if only one seek step for the given direction exists
      if ((m_backwardSeekSteps.size() == 1 && forward == false) ||
          (m_forwardSeekSteps.size() == 1 && forward == true))
        m_seekDelay = 0;
    }
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
      float percentPerSecond = 0.0f;
      if (g_infoManager.GetTotalPlayTime())
        percentPerSecond = 100.0f / (float)g_infoManager.GetTotalPlayTime();

      int seekSeconds = GetSeekSeconds(forward);
      g_infoManager.SetSeekStepSize(seekSeconds);

      m_percent = m_percentPlayTime + percentPerSecond * seekSeconds;
    }

    if (m_percent > 100.0f) m_percent = 100.0f;
    if (m_percent < 0.0f)   m_percent = 0.0f;
  }

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
  if (m_timer.GetElapsedMilliseconds() > m_seekDelay)
  {
    if (!g_infoManager.m_performingSeek && m_timer.GetElapsedMilliseconds() > time_for_display) // TODO: Why?
      g_infoManager.SetSeeking(false);

    if (m_requireSeek)
    {
      g_infoManager.m_performingSeek = true;

      // reset seek step size
      g_infoManager.SetSeekStepSize(0);

      // calculate the seek time
      double time = g_infoManager.GetTotalPlayTime() * m_percent * 0.01;

      g_application.SeekTime(time);
      m_requireSeek = false;
    }
  }
}

void CSeekHandler::SettingOptionsSeekStepsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  std::string label;
  for (std::vector<int>::iterator it = g_advancedSettings.m_videoSeekSteps.begin(); it != g_advancedSettings.m_videoSeekSteps.end(); ++it) {
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
      setting->GetId() == "videoplayer.seeksteps")
    Reset();
}

bool CSeekHandler::OnAction(const CAction &action)
{
  if (!g_application.m_pPlayer->IsPlaying() || !g_application.m_pPlayer->CanSeek())
    return false;

  switch (action.GetID())
  {
    case ACTION_STEP_BACK:
    {
      Seek(false, action.GetAmount(), action.GetRepeat());
      return true;
    }
    case ACTION_STEP_FORWARD:
    {
      Seek(true, action.GetAmount(), action.GetRepeat());
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
    case ACTION_SMALL_STEP_BACK:
    {
      int orgpos = (int)g_application.GetTime();
      int jumpsize = g_advancedSettings.m_videoSmallStepBackSeconds; // secs
      int setpos = (orgpos > jumpsize) ? orgpos - jumpsize : 0;
      g_application.SeekTime((double)setpos);
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
