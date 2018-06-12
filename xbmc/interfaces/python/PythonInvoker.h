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

#include <map>
#include <string>
#include <vector>

#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/python/LanguageHook.h"
#include "interfaces/legacy/Addon.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

struct _ts;

class CPythonInvoker : public ILanguageInvoker
{
public:
  explicit CPythonInvoker(ILanguageInvocationHandler *invocationHandler);
  ~CPythonInvoker() override;

  bool Execute(const std::string &script, const std::vector<std::string> &arguments = std::vector<std::string>()) override;

  bool IsStopping() const override { return m_stop || ILanguageInvoker::IsStopping(); }

  typedef void (*PythonModuleInitialization)();

protected:
  // implementation of ILanguageInvoker
  bool execute(const std::string &script, const std::vector<std::string> &arguments) override;
  virtual void executeScript(void *fp, const std::string &script, void *module, void *moduleDict);
  bool stop(bool abort) override;
  void onExecutionDone() override;
  void onExecutionFailed() override;

  // custom virtual methods
  virtual std::map<std::string, PythonModuleInitialization> getModules() const;
  virtual const char* getInitializationScript() const;
  virtual void onInitialization();
  // actually a PyObject* but don't wanna draw Python.h include into the header
  virtual void onPythonModuleInitialization(void* moduleDict);
  virtual void onDeinitialization();

  virtual void onSuccess() { }
  virtual void onAbort() { }
  virtual void onError(const std::string &exceptionType = "", const std::string &exceptionValue = "", const std::string &exceptionTraceback = "");

  std::string m_sourceFile;
  CCriticalSection m_critical;

private:
  void initializeModules(const std::map<std::string, PythonModuleInitialization> &modules);
  bool initializeModule(PythonModuleInitialization module);
  void addPath(const std::string& path); // add path in UTF-8 encoding
  void addNativePath(const std::string& path); // add path in system/Python encoding
  void getAddonModuleDeps(const ADDON::AddonPtr& addon, std::set<std::string>& paths);

  std::string m_pythonPath;
  _ts *m_threadState;
  bool m_stop;
  CEvent m_stoppedEvent;

  XBMCAddon::AddonClass::Ref<XBMCAddon::Python::PythonLanguageHook> m_languageHook;
  bool m_systemExitThrown = false;

  static CCriticalSection s_critical;
};
