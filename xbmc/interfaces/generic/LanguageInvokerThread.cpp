/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LanguageInvokerThread.h"

#include "ScriptInvocationManager.h"

CLanguageInvokerThread::CLanguageInvokerThread(LanguageInvokerPtr invoker,
                                               CScriptInvocationManager* invocationManager,
                                               bool reuseable)
  : ILanguageInvoker(NULL),
    CThread("LanguageInvoker"),
    m_invoker(invoker),
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

void CLanguageInvokerThread::Release()
{
  m_bStop = true;
  m_condition.notify_one();
}

bool CLanguageInvokerThread::execute(const std::string &script, const std::vector<std::string> &arguments)
{
  if (m_invoker == NULL || script.empty())
    return false;

  m_script = script;
  m_args = arguments;

  if (CThread::IsRunning())
  {
    std::unique_lock<std::mutex> lck(m_mutex);
    m_restart = true;
    m_condition.notify_one();
  }
  else
    Create();

  //Todo wait until running

  return true;
}

bool CLanguageInvokerThread::stop(bool wait)
{
  if (m_invoker == NULL)
    return false;

  if (!CThread::IsRunning())
    return false;

  Release();

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

  std::unique_lock<std::mutex> lckdl(m_mutex);
  do
  {
    m_restart = false;
    m_invoker->Execute(m_script, m_args);

    if (m_invoker->GetState() != InvokerStateScriptDone)
      m_reusable = false;

    m_condition.wait(lckdl, [this] { return m_bStop || m_restart || !m_reusable; });

  } while (m_reusable && !m_bStop);
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