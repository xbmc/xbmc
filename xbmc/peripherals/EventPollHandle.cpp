/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventPollHandle.h"

using namespace PERIPHERALS;

CEventPollHandle::CEventPollHandle(IEventPollCallback& callback) : m_callback(callback)
{
}

CEventPollHandle::~CEventPollHandle(void)
{
  m_callback.Release(*this);
}

void CEventPollHandle::Activate()
{
  m_callback.Activate(*this);
}

void CEventPollHandle::Deactivate()
{
  m_callback.Deactivate(*this);
}

void CEventPollHandle::HandleEvents(bool bWait)
{
  m_callback.HandleEvents(bWait);
}
