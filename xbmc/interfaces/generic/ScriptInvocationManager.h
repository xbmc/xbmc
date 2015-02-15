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
#include <set>
#include <memory>

#include "addons/IAddon.h"
#include "threads/CriticalSection.h"

class ILanguageInvocationHandler;
class ILanguageInvoker;
class CLanguageInvokerThread;
typedef std::shared_ptr<CLanguageInvokerThread> CLanguageInvokerThreadPtr;

class CScriptInvocationManager
{
public:
  static CScriptInvocationManager& Get();

  void Process();
  void Uninitialize();

  void RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::string &extension);
  void RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::set<std::string> &extensions);
  void UnregisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler);
  bool HasLanguageInvoker(const std::string &script) const;
  ILanguageInvoker* GetLanguageInvoker(const std::string &script) const;

  int Execute(const std::string &script, const ADDON::AddonPtr &addon = ADDON::AddonPtr(), const std::vector<std::string> &arguments = std::vector<std::string>());
  bool Stop(int scriptId, bool wait = false);
  bool Stop(const std::string &scriptPath, bool wait = false);

  bool IsRunning(int scriptId) const;

protected:
  friend class CLanguageInvokerThread;

  void OnScriptEnded(int scriptId);

private:
  CScriptInvocationManager();
  CScriptInvocationManager(const CScriptInvocationManager&);
  CScriptInvocationManager const& operator=(CScriptInvocationManager const&);
  virtual ~CScriptInvocationManager();

  typedef struct {
    CLanguageInvokerThreadPtr thread;
    std::string script;
    bool done;
  } LanguageInvokerThread;
  typedef std::map<int, LanguageInvokerThread> LanguageInvokerThreadMap;
  typedef std::map<std::string, ILanguageInvocationHandler*> LanguageInvocationHandlerMap;

  LanguageInvokerThread getInvokerThread(int scriptId) const;

  LanguageInvocationHandlerMap m_invocationHandlers;
  LanguageInvokerThreadMap m_scripts;
  std::map<std::string, int> m_scriptPaths;
  int m_nextId;
  CCriticalSection m_critSection;
};