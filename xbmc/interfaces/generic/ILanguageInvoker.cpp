/*
*      Copyright (C) 2015 Team XBMC
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

#include <string>
#include <vector>

#include "ILanguageInvoker.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"

ILanguageInvoker::ILanguageInvoker(ILanguageInvocationHandler *invocationHandler)
  : m_id(-1),
    m_state(InvokerStateUninitialized),
    m_invocationHandler(invocationHandler)
{ }

ILanguageInvoker::~ILanguageInvoker()
{ }

bool ILanguageInvoker::Execute(const std::string &script, const std::vector<std::string> &arguments /* = std::vector<std::string>() */)
{
  if (m_invocationHandler)
    m_invocationHandler->OnScriptStarted(this);

  return execute(script, arguments);
}

bool ILanguageInvoker::Stop(bool abort /* = false */)
{
  return stop(abort);
}

bool ILanguageInvoker::IsActive() const
{
  return GetState() > InvokerStateUninitialized && GetState() < InvokerStateDone;
}

bool ILanguageInvoker::IsRunning() const
{
  return GetState() == InvokerStateRunning;
}

bool ILanguageInvoker::IsStopping() const
{
  return GetState() == InvokerStateStopping;
}

void ILanguageInvoker::pulseGlobalEvent()
{
  if (m_invocationHandler)
    m_invocationHandler->PulseGlobalEvent();
}

bool ILanguageInvoker::onExecutionInitialized()
{
  if (m_invocationHandler == NULL)
    return false;

  return m_invocationHandler->OnScriptInitialized(this);
}

void ILanguageInvoker::onAbortRequested()
{
  if (m_invocationHandler)
    m_invocationHandler->OnScriptAbortRequested(this);
}

void ILanguageInvoker::onExecutionFailed()
{
  if (m_invocationHandler)
    m_invocationHandler->OnScriptEnded(this);
}

void ILanguageInvoker::onExecutionDone()
{
  if (m_invocationHandler)
    m_invocationHandler->OnScriptEnded(this);
}

void ILanguageInvoker::onExecutionFinalized()
{
  if (m_invocationHandler)
    m_invocationHandler->OnScriptFinalized(this);
}

void ILanguageInvoker::setState(InvokerState state)
{
  if (state <= m_state)
    return;

  m_state = state;
}
