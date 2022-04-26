/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "SkinTimer.h"

#include "interfaces/builtins/Builtins.h"
#include "interfaces/info/Info.h"

CSkinTimer::CSkinTimer(const std::string& name,
                       const INFO::InfoPtr startCondition,
                       const INFO::InfoPtr resetCondition,
                       const INFO::InfoPtr stopCondition,
                       const std::string& startAction,
                       const std::string& stopAction,
                       bool resetOnStart)
  : m_name{name},
    m_startCondition{startCondition},
    m_resetCondition{resetCondition},
    m_stopCondition{stopCondition},
    m_startAction{startAction},
    m_stopAction{stopAction},
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

void CSkinTimer::OnStart()
{
  if (!m_startAction.empty())
  {
    CBuiltins::GetInstance().Execute(m_startAction);
  }
}

void CSkinTimer::OnStop()
{
  if (!m_stopAction.empty())
  {
    CBuiltins::GetInstance().Execute(m_stopAction);
  }
}
