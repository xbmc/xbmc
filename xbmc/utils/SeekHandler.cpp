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
#include "GUIInfoManager.h"
#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"

CSeekHandler::CSeekHandler()
  : m_requireSeek(false),
  m_percent(0.0f),
  m_seek_step(0),
  m_percents_pro_sec(0.1f),
  m_time_before_seek(500),
  m_time_for_display(2000)
{

}

void CSeekHandler::Reset()
{
  m_requireSeek = false;
  m_percent = 0;
  m_seek_step = 0;
  m_percents_pro_sec = 1.0f;
  m_time_before_seek = g_advancedSettings.m_videoAdditiveTimeSeekWaitForSeekingStart;
  m_time_for_display = g_advancedSettings.m_videoAdditiveTimeSeekWaitForDisplayHide;
}

int CSeekHandler::GetSeekSeconds(bool forward)
{
  if (forward)
  {
    m_seek_step++;
  }
  else
  {
    m_seek_step--;
  }

  int seconds = 0;
  switch (abs(m_seek_step))
  {
  case 1: seconds = g_advancedSettings.m_videoAdditiveTimeSeekStepOne; break;
  case 2: seconds = g_advancedSettings.m_videoAdditiveTimeSeekStepTwo; break;
  case 3: seconds = g_advancedSettings.m_videoAdditiveTimeSeekStepThree; break;
  case 4: seconds = g_advancedSettings.m_videoAdditiveTimeSeekStepFour; break;
  case 5: seconds = g_advancedSettings.m_videoAdditiveTimeSeekStepFive; break;
  case 6: seconds = g_advancedSettings.m_videoAdditiveTimeSeekStepSix; break;
  default:
    seconds = (abs(m_seek_step) - 6) * g_advancedSettings.m_videoAdditiveTimeSeekStepOther; break;
  }
  if (m_seek_step < 0)
  {
    seconds = seconds * -1.0f;
  }
  return seconds;
}

void CSeekHandler::Seek(bool forward, float amount, float duration)
{
  if (!m_requireSeek)
  { // not yet seeking
    if (CSettings::Get().GetBool("videoplayer.useadditiveseeking"))
    {
      if (g_infoManager.GetTotalPlayTime())
      {
        m_percents_pro_sec = 100.0f / (float)g_infoManager.GetTotalPlayTime();
      }
      m_seek_step = 0;
      m_percent = GetSeekSeconds(forward)*m_percents_pro_sec;
    }
    else
    {
      if (g_infoManager.GetTotalPlayTime())
        m_percent = (float)g_infoManager.GetPlayTime() / g_infoManager.GetTotalPlayTime() * 0.1f;
      else
        m_percent = 0.0f;
    }
    // tell info manager that we have started a seek operation
    m_requireSeek = true;
    g_infoManager.SetSeeking(true);
  }
  // calculate our seek amount
  if (!g_infoManager.m_performingSeek)
  {
    if (CSettings::Get().GetBool("videoplayer.useadditiveseeking"))
    {
      m_percent = GetSeekSeconds(forward)*m_percents_pro_sec;
    }
    else
    {
      //100% over 1 second.
      float speed = 100.0f;
      if (duration)
        speed *= duration;
      else
        speed /= g_infoManager.GetFPS();

      if (forward)
        m_percent += amount * amount * speed;
      else
        m_percent -= amount * amount * speed;
      if (m_percent > 100.0f) m_percent = 100.0f;
      if (m_percent < 0.0f)   m_percent = 0.0f;
    }
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
  if (m_timer.GetElapsedMilliseconds() > m_time_before_seek)
  {
    if (!g_infoManager.m_performingSeek && m_timer.GetElapsedMilliseconds() > m_time_for_display) // TODO: Why?
      g_infoManager.SetSeeking(false);
    if (m_requireSeek)
    {
      g_infoManager.m_performingSeek = true;
      double time;
      if (CSettings::Get().GetBool("videoplayer.useadditiveseeking"))
      {
        time = (g_infoManager.GetTotalPlayTime() * m_percent / 100.0f + g_infoManager.GetPlayTime() / 1000.0f);
      }
      else
      {
        time = g_infoManager.GetTotalPlayTime() * m_percent * 0.01;
      }
      g_application.SeekTime(time);
      m_requireSeek = false;
    }
  }
}
