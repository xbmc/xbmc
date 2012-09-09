/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

CSeekHandler::CSeekHandler()
: m_requireSeek(false),
  m_percent(0.0f)
{
}

void CSeekHandler::Reset()
{
  m_requireSeek = false;
  m_percent = 0;
}

void CSeekHandler::Seek(bool forward, float amount, float duration)
{
  if (!m_requireSeek)
  { // not yet seeking
    if (g_infoManager.GetTotalPlayTime())
      m_percent = (float)g_infoManager.GetPlayTime() / g_infoManager.GetTotalPlayTime() * 0.1f;
    else
      m_percent = 0.0f;

    // tell info manager that we have started a seek operation
    m_requireSeek = true;
    g_infoManager.SetSeeking(true);
  }
  // calculate our seek amount
  if (!g_infoManager.m_performingSeek)
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
  if (m_timer.GetElapsedMilliseconds() > time_before_seek)
  {
    if (!g_infoManager.m_performingSeek && m_timer.GetElapsedMilliseconds() > time_for_display) // TODO: Why?
      g_infoManager.SetSeeking(false);
    if (m_requireSeek)
    {
      g_infoManager.m_performingSeek = true;
      double time = g_infoManager.GetTotalPlayTime() * m_percent * 0.01;
      g_application.SeekTime(time);
      m_requireSeek = false;
    }
  }
}
