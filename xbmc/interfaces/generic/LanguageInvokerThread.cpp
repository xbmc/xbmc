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

void CLanguageInvokerThread::Release()
{
  std::unique_lock<std::mutex> lck(m_mutex);
  m_bStop = true;
  m_condition.notify_one();
}

bool CLanguageInvokerThread::Claim()
{
  bool unclaimed = false;
  return m_claimed.compare_exchange_strong(unclaimed, true);
}

void CLanguageInvokerThread::ReleaseClaim()
{
  m_claimed = false;
}

bool CLanguageInvokerThread::execute(const std::string &script, const std::vector<std::string> &arguments)
{
  if (m_invoker == NULL || script.empty())
    return false;

  std::unique_lock<std::mutex> lck(m_mutex);

  // The Process() loop has gone but the thread has not: CThread::IsRunning()
  // stays true all through OnExit()/Py_EndInterpreter, so a restart set here
  // would notify a condition variable nobody waits on and the script would
  // silently never run. Create() is not an alternative either - it calls
  // exit(1) while the previous thread has not fulfilled its promise. Refuse,
  // and let the caller start a fresh thread.
  if (m_processDone)
    return false;

  m_script = script;
  m_args = arguments;

  if (CThread::IsRunning())
  {
    m_restart = true;
    m_condition.notify_one();
    return true;
  }

  lck.unlock();
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
  bool keepGoing = true;
  while (keepGoing)
  {
    m_restart = false;
    const std::string script = m_script;
    const std::vector<std::string> args = m_args;
    // Execute() talks to the GUI and to other invokers. Holding m_mutex
    // across it deadlocks Home.xml load: a JobWorker in Release()/restart
    // waits for this lock, the app thread waits for that job, and this
    // script waits for the GUI.
    lckdl.unlock();
    m_invoker->Execute(script, args);
    lckdl.lock();

    // A claimed invoker was Reset() on purpose by whoever claimed it, so its
    // state says nothing about whether this thread is still worth keeping.
    // Reading it unconditionally raced that Reset() and retired the very
    // thread the claimer was about to run its script on.
    if (!IsClaimed() && m_invoker->GetState() != InvokerStateScriptDone)
      m_reusable = false;

    m_condition.wait(lckdl,
                     [this] { return m_bStop || m_restart || (!m_reusable && !IsClaimed()); });

    // A claim keeps the loop open, so a stop request does not have to be
    // ignored to protect a script that has just been handed to this thread.
    keepGoing = (m_restart || m_reusable || IsClaimed()) && !m_bStop;
    if (!keepGoing)
      m_processDone = true; // set under m_mutex, before execute() can look again
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
