/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include <stdexcept>

#include "StubEventListener.h"

StubEventListener::StubEventListener() :
  m_focused(false)
{
}

XBMC_Event
StubEventListener::FetchLastEvent()
{
  if (m_events.empty())
    throw std::logic_error("No events left to get!");
  
  XBMC_Event ev = m_events.front();
  m_events.pop();
  return ev;
}

bool
StubEventListener::Focused()
{
  return m_focused;
}

void
StubEventListener::OnFocused()
{
  m_focused = true;
}

void
StubEventListener::OnUnfocused()
{
  m_focused = false;
}

void
StubEventListener::OnEvent(XBMC_Event &ev)
{
  m_events.push(ev);
}
