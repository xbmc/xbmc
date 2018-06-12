/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <string>
#include <vector>

#include "interfaces/generic/ILanguageInvoker.h"
#include "threads/Thread.h"

class CScriptInvocationManager;

class CLanguageInvokerThread : public ILanguageInvoker, protected CThread
{
public:
  CLanguageInvokerThread(LanguageInvokerPtr invoker, CScriptInvocationManager *invocationManager, bool reuseable);
  ~CLanguageInvokerThread() override;

  virtual InvokerState GetState() const;

  const std::string &GetScript() const { return m_script; };
  LanguageInvokerPtr GetInvoker() const { return m_invoker; };
  bool Reuseable(const std::string &script) const { return !m_bStop && m_reusable && GetState() == InvokerStateScriptDone && m_script == script; };
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

