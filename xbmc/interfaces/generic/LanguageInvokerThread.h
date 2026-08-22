/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/generic/ILanguageInvoker.h"
#include "threads/Thread.h"

#include <mutex>
#include <string>
#include <vector>

class CScriptInvocationManager;

class CLanguageInvokerThread : public ILanguageInvoker, protected CThread
{
public:
  CLanguageInvokerThread(std::shared_ptr<ILanguageInvoker> invoker,
                         CScriptInvocationManager* invocationManager,
                         bool reuseable);
  ~CLanguageInvokerThread() override;

  virtual InvokerState GetState() const;

  std::shared_ptr<ILanguageInvoker> GetInvoker() const { return m_invoker; }
  bool Reuseable(const std::string& script) const
  {
    // Cheap, independently-synchronised checks first, so the common "not
    // reusable" answer does not contend with a running script for m_mutex.
    if (m_bStop || GetState() != InvokerStateScriptDone)
      return false;

    // m_script and m_reusable are written by execute() and Process() under
    // m_mutex. Reading them unlocked from the manager thread was a data race on
    // a std::string: a concurrent assignment can reallocate the buffer while
    // the comparison walks it.
    std::unique_lock<std::mutex> lck(m_mutex);
    return m_reusable && m_script == script;
  }
  virtual void Release();

protected:
  bool execute(const std::string &script, const std::vector<std::string> &arguments) override;
  bool stop(bool wait) override;

  void OnStartup() override;
  void Process() override;
  void OnExit() override;
  void OnException() override;

private:
  std::shared_ptr<ILanguageInvoker> m_invoker;
  CScriptInvocationManager *m_invocationManager;
  std::string m_script;
  std::vector<std::string> m_args;

  //! Guards m_script, m_args and m_reusable. Taken while m_critSection is held
  //! (CScriptInvocationManager -> Reuseable()/Release()); never the other way
  //! round, which is why Process() drops it across Execute().
  mutable std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_restart = false;
  bool m_reusable = false;
};
