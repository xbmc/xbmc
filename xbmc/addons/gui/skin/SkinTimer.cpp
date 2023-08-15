/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "SkinTimer.h"

#include "interfaces/info/Info.h"

CSkinTimer::CSkinTimer(const std::string& name,
                       const INFO::InfoPtr& startCondition,
                       const INFO::InfoPtr& resetCondition,
                       const INFO::InfoPtr& stopCondition,
                       const CGUIAction& startActions,
                       const CGUIAction& stopActions,
                       bool resetOnStart)
  : m_name{name},
    m_startCondition{startCondition},
    m_resetCondition{resetCondition},
    m_stopCondition{stopCondition},
    m_startActions{startActions},
    m_stopActions{stopActions},
    m_resetOnStart{resetOnStart}
{
}

void CSkinTimer::Start()
{
  if (m_resetOnStart)
  {
    CStopWatch::StartZero();
  }
  else
  {
    CStopWatch::Start();
  }
  OnStart();
}

void CSkinTimer::Reset()
{
  CStopWatch::Reset();
}

void CSkinTimer::Stop()
{
  CStopWatch::Stop();
  OnStop();
}

bool CSkinTimer::VerifyStartCondition() const
{
  return m_startCondition && m_startCondition->Get(INFO::DEFAULT_CONTEXT);
}

bool CSkinTimer::VerifyResetCondition() const
{
  return m_resetCondition && m_resetCondition->Get(INFO::DEFAULT_CONTEXT);
}

bool CSkinTimer::VerifyStopCondition() const
{
  return m_stopCondition && m_stopCondition->Get(INFO::DEFAULT_CONTEXT);
}

const std::string& CSkinTimer::GetName() const
{
  return m_name;
}

INFO::InfoPtr CSkinTimer::GetStartCondition() const
{
  return m_startCondition;
}

INFO::InfoPtr CSkinTimer::GetResetCondition() const
{
  return m_resetCondition;
}

INFO::InfoPtr CSkinTimer::GetStopCondition() const
{
  return m_stopCondition;
}

const CGUIAction& CSkinTimer::GetStartActions() const
{
  return m_startActions;
}

const CGUIAction& CSkinTimer::GetStopActions() const
{
  return m_stopActions;
}

bool CSkinTimer::ResetsOnStart() const
{
  return m_resetOnStart;
}

void CSkinTimer::OnStart()
{
  if (m_startActions.HasAnyActions())
  {
    m_startActions.ExecuteActions();
  }
}

void CSkinTimer::OnStop()
{
  if (m_stopActions.HasAnyActions())
  {
    m_stopActions.ExecuteActions();
  }
}
