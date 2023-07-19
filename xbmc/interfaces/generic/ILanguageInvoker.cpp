/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ILanguageInvoker.h"

#include "interfaces/generic/ILanguageInvocationHandler.h"

#include <string>
#include <vector>

ILanguageInvoker::ILanguageInvoker(ILanguageInvocationHandler* invocationHandler)
  : m_invocationHandler(invocationHandler)
{ }

ILanguageInvoker::~ILanguageInvoker() = default;

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
  return GetState() > InvokerStateUninitialized && GetState() < InvokerStateScriptDone;
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

void ILanguageInvoker::AbortNotification()
{
  if (m_invocationHandler)
    m_invocationHandler->NotifyScriptAborting(this);
}

void ILanguageInvoker::onExecutionFailed()
{
  if (m_invocationHandler)
    m_invocationHandler->OnExecutionEnded(this);
}

void ILanguageInvoker::onExecutionDone()
{
  if (m_invocationHandler)
    m_invocationHandler->OnExecutionEnded(this);
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
