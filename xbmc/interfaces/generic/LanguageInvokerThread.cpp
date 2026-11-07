/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LanguageInvokerThread.h"

#include "ScriptInvocationManager.h"

#include <utility>

CLanguageInvokerThread::CLanguageInvokerThread(std::shared_ptr<ILanguageInvoker> invoker,
                                               CScriptInvocationManager* invocationManager,
                                               bool reuseable)
  : ILanguageInvoker(NULL),
    CThread("LanguageInvoker"),
    m_invoker(std::move(invoker)),
    m_invocationManager(invocationManager),
    m_reusable(reuseable)
{ }

CLanguageInvokerThread::~CLanguageInvokerThread()
{
  Stop(true);
}

InvokerState CLanguageInvokerThread::GetState() const
{
  if (m_invoker == NULL)
    return InvokerStateFailed;

  return m_invoker->GetState();
}

void CLanguageInvokerThread::SetReserved(bool reserved)
{
  bool shouldWake = false;
  {
    std::unique_lock<std::mutex> lck(m_mutex);
    m_reserved = reserved;
    if (!m_reserved && m_releasePending)
    {
      m_releasePending = false;
      m_released = true;
      m_bStop = true;
      shouldWake = true;
    }
  }
  if (shouldWake)
    m_condition.notify_one();
}

void CLanguageInvokerThread::Release()
{
  {
    std::unique_lock<std::mutex> lck(m_mutex);
    if (m_reserved)
    {
      m_releasePending = true;
      return;
    }
    m_released = true;
    m_bStop = true;
  }
  m_condition.notify_one();
}

bool CLanguageInvokerThread::execute(const std::string &script, const std::vector<std::string> &arguments)
{
  if (m_invoker == NULL || script.empty())
  {
    SetReserved(false);
    return false;
  }

  if (CThread::IsRunning())
  {
    {
      std::unique_lock<std::mutex> lck(m_mutex);
      m_script = script;
      m_args = arguments;
      m_restart = true;
    }
    m_condition.notify_one();
  }
  else
  {
    {
      std::unique_lock<std::mutex> lck(m_mutex);
      m_script = script;
      m_args = arguments;
    }
    Create();
  }

  //Todo wait until running

  return true;
}

bool CLanguageInvokerThread::stop(bool wait)
{
  if (m_invoker == NULL)
    return false;

  if (!CThread::IsRunning())
    return false;

  {
    std::unique_lock<std::mutex> lck(m_mutex);
    m_releasePending = false;
    m_released = true;
    m_bStop = true;
  }
  m_condition.notify_one();

  bool result = true;
  if (m_invoker->GetState() < InvokerStateExecutionDone)
  {
    // stop the language-specific invoker
    result = m_invoker->Stop(wait);
  }
  // stop the thread
  CThread::StopThread(wait);

  return result;
}

void CLanguageInvokerThread::OnStartup()
{
  if (m_invoker == NULL)
    return;

  m_invoker->SetId(GetId());
  if (m_addon != NULL)
    m_invoker->SetAddon(m_addon);
}

void CLanguageInvokerThread::Process()
{
  if (m_invoker == NULL)
    return;

  while (true)
  {
    std::string script;
    std::vector<std::string> args;
    {
      std::unique_lock<std::mutex> lck(m_mutex);
      if (m_bStop || m_released)
        break;

      script = m_script;
      args = m_args;
    }

    m_invoker->Execute(script, args);

    std::unique_lock<std::mutex> lck(m_mutex);
    if (m_invoker->GetState() != InvokerStateScriptDone)
      m_reusable = false;

    m_reserved = false;
    if (m_releasePending && !m_restart)
    {
      m_releasePending = false;
      m_released = true;
      m_bStop = true;
    }

    m_condition.wait(lck, [this] { return m_bStop || m_released || m_restart || !m_reusable; });

    if (m_bStop || m_released || !m_reusable || !m_restart)
      break;

    m_restart = false;
  }
}

void CLanguageInvokerThread::OnExit()
{
  if (m_invoker == NULL)
    return;

  m_invoker->onExecutionDone();
  m_invocationManager->OnExecutionDone(GetId());
}

void CLanguageInvokerThread::OnException()
{
  if (m_invoker == NULL)
    return;

  m_invoker->onExecutionFailed();
  m_invocationManager->OnExecutionDone(GetId());
}
