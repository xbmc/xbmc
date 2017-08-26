/*
 *      Copyright (C) 2016-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "EventPollHandle.h"

#include <assert.h>

using namespace PERIPHERALS;

CEventPollHandle::CEventPollHandle(IEventPollCallback* callback) :
  m_callback(callback)
{
  assert(m_callback != nullptr);
}

CEventPollHandle::~CEventPollHandle(void)
{
  m_callback->Release(this);
}

void CEventPollHandle::Activate()
{
  m_callback->Activate(this);
}

void CEventPollHandle::Deactivate()
{
  m_callback->Deactivate(this);
}

void CEventPollHandle::HandleEvents(bool bWait)
{
  m_callback->HandleEvents(bWait);
}
