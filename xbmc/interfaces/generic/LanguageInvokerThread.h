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

#include <string>
#include <vector>

class CScriptInvocationManager;

class CLanguageInvokerThread : public ILanguageInvoker, protected CThread
{
public:
  CLanguageInvokerThread(LanguageInvokerPtr invoker,
                         CScriptInvocationManager* invocationManager,
                         bool reuseable);
  ~CLanguageInvokerThread() override;

  virtual InvokerState GetState() const;

  const std::string& GetScript() const { return m_script; };
  LanguageInvokerPtr GetInvoker() const { return m_invoker; };
  bool Reuseable(const std::string& script) const
  {
    return !m_bStop && m_reusable && GetState() == InvokerStateScriptDone && m_script == script;
  };
  virtual void Release();

protected:
  bool execute(const std::string &script, const std::vector<std::string> &arguments) override;
  bool stop(bool wait) override;

  void OnStartup() override;
  void Process() override;
  void OnExit() override;
  void OnException() override;

private:
  LanguageInvokerPtr m_invoker;
  CScriptInvocationManager *m_invocationManager;
  std::string m_script;
  std::vector<std::string> m_args;

  std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_restart = false;
  bool m_reusable = false;
};
