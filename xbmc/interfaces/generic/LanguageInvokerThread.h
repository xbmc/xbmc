#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include "interfaces/generic/ILanguageInvoker.h"
#include "threads/Thread.h"

class CScriptInvocationManager;

class CLanguageInvokerThread : public ILanguageInvoker, protected CThread
{
public:
  CLanguageInvokerThread(LanguageInvokerPtr invoker, CScriptInvocationManager *invocationManager);
  ~CLanguageInvokerThread();

  virtual InvokerState GetState();

protected:
  virtual bool execute(const std::string &script, const std::vector<std::string> &arguments);
  virtual bool stop(bool wait);

  virtual void OnStartup();
  virtual void Process();
  virtual void OnExit();
  virtual void OnException();

private:
  LanguageInvokerPtr m_invoker;
  CScriptInvocationManager *m_invocationManager;
  std::string m_script;
  std::vector<std::string> m_args;
};