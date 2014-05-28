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

#include <map>
#include <string>

#ifndef PYTHON_INTERFACES_GENERIC_ILANGUAGEINVOKER_H_INCLUDED
#define PYTHON_INTERFACES_GENERIC_ILANGUAGEINVOKER_H_INCLUDED
#include "interfaces/generic/ILanguageInvoker.h"
#endif

#ifndef PYTHON_THREADS_CRITICALSECTION_H_INCLUDED
#define PYTHON_THREADS_CRITICALSECTION_H_INCLUDED
#include "threads/CriticalSection.h"
#endif

#ifndef PYTHON_THREADS_EVENT_H_INCLUDED
#define PYTHON_THREADS_EVENT_H_INCLUDED
#include "threads/Event.h"
#endif


class CPythonInvoker : public ILanguageInvoker
{
public:
  CPythonInvoker(ILanguageInvocationHandler *invocationHandler);
  virtual ~CPythonInvoker();

  virtual bool Execute(const std::string &script, const std::vector<std::string> &arguments = std::vector<std::string>());

  virtual bool IsStopping() const { return m_stop || ILanguageInvoker::IsStopping(); }

  typedef void (*PythonModuleInitialization)();
  
protected:
  // implementation of ILanguageInvoker
  virtual bool execute(const std::string &script, const std::vector<std::string> &arguments);
  virtual bool stop(bool abort);
  virtual void onExecutionFailed();

  // custom virtual methods
  virtual std::map<std::string, PythonModuleInitialization> getModules() const;
  virtual const char* getInitializationScript() const;
  virtual void onInitialization();
  // actually a PyObject* but don't wanna draw Python.h include into the header
  virtual void onPythonModuleInitialization(void* moduleDict);
  virtual void onDeinitialization();

  virtual void onSuccess() { }
  virtual void onAbort() { }
  virtual void onError();

  char *m_source;
  unsigned int  m_argc;
  char **m_argv;
  CCriticalSection m_critical;

private:
  void initializeModules(const std::map<std::string, PythonModuleInitialization> &modules);
  bool initializeModule(PythonModuleInitialization module);
  void addPath(const std::string& path);

  std::string m_pythonPath;
  void *m_threadState;
  bool m_stop;
  CEvent m_stoppedEvent;

  static CCriticalSection s_critical;
};
