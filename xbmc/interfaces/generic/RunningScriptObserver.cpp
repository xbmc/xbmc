/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RunningScriptObserver.h"

#include "interfaces/generic/ScriptInvocationManager.h"

using namespace std::chrono_literals;

CRunningScriptObserver::CRunningScriptObserver(int scriptId, CEvent& evt)
  : m_scriptId(scriptId), m_event(evt), m_stopEvent(true), m_thread(this, "ScriptObs")
{
  m_thread.Create();
}

CRunningScriptObserver::~CRunningScriptObserver()
{
  Abort();
}

void CRunningScriptObserver::Run()
{
  do
  {
    if (!CScriptInvocationManager::GetInstance().IsRunning(m_scriptId))
    {
      m_event.Set();
      break;
    }
  } while (!m_stopEvent.Wait(20ms));
}

void CRunningScriptObserver::Abort()
{
  m_stopEvent.Set();
  m_thread.StopThread();
}
