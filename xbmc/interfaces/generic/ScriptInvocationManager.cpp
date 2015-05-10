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

#include <errno.h>
#include <vector>

#include "ScriptInvocationManager.h"
#include "filesystem/File.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/LanguageInvokerThread.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace std;
using namespace XFILE;

CScriptInvocationManager::CScriptInvocationManager()
  : m_nextId(0)
{ }

CScriptInvocationManager::~CScriptInvocationManager()
{
  Uninitialize();
}

CScriptInvocationManager& CScriptInvocationManager::Get()
{
  static CScriptInvocationManager s_instance;
  return s_instance;
}

void CScriptInvocationManager::Process()
{
  CSingleLock lock(m_critSection);
  // go through all active threads and find and remove all which are done
  vector<LanguageInvokerThread> tempList;
  for (LanguageInvokerThreadMap::iterator it = m_scripts.begin(); it != m_scripts.end(); )
  {
    if (it->second.done)
    {
      tempList.push_back(it->second);
      m_scripts.erase(it++);
    }
    else
      ++it;
  }

  // remove the finished scripts from the script path map as well
  for (vector<LanguageInvokerThread>::const_iterator it = tempList.begin(); it != tempList.end(); ++it)
    m_scriptPaths.erase(it->script);

  // we can leave the lock now
  lock.Leave();

  // finally remove the finished threads but we do it outside of any locks in
  // case of any callbacks from the destruction of the CLanguageInvokerThread
  tempList.clear();

  // let the invocation handlers do their processing
  for (LanguageInvocationHandlerMap::iterator it = m_invocationHandlers.begin(); it != m_invocationHandlers.end(); ++it)
    it->second->Process();
}

void CScriptInvocationManager::Uninitialize()
{
  CSingleLock lock(m_critSection);

  // execute Process() once more to handle the remaining scripts
  Process();

  // make sure all scripts are done
  vector<LanguageInvokerThread> tempList;
  for (LanguageInvokerThreadMap::iterator script = m_scripts.begin(); script != m_scripts.end(); ++script)
    tempList.push_back(script->second);

  m_scripts.clear();
  m_scriptPaths.clear();

  // we can leave the lock now
  lock.Leave();

  // finally stop and remove the finished threads but we do it outside of any
  // locks in case of any callbacks from the stop or destruction logic of
  // CLanguageInvokerThread or the ILanguageInvoker implementation
  for (vector<LanguageInvokerThread>::iterator it = tempList.begin(); it != tempList.end(); ++it)
  {
    if (!it->done)
      it->thread->Stop(true);
  }
  tempList.clear();

  lock.Enter();
  // uninitialize all invocation handlers and then remove them
  for (LanguageInvocationHandlerMap::iterator it = m_invocationHandlers.begin(); it != m_invocationHandlers.end(); ++it)
    it->second->Uninitialize();

  m_invocationHandlers.clear();
}

void CScriptInvocationManager::RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::string &extension)
{
  if (invocationHandler == NULL || extension.empty())
    return;

  string ext = extension;
  StringUtils::ToLower(ext);
  if (!StringUtils::StartsWithNoCase(ext, "."))
    ext = "." + ext;

  CSingleLock lock(m_critSection);
  if (m_invocationHandlers.find(ext) != m_invocationHandlers.end())
    return;

  m_invocationHandlers.insert(make_pair(extension, invocationHandler));

  bool known = false;
  for (std::map<std::string, ILanguageInvocationHandler*>::const_iterator it = m_invocationHandlers.begin(); it != m_invocationHandlers.end(); ++it)
  {
    if (it->second == invocationHandler)
    {
      known = true;
      break;
    }
  }

  // automatically initialize the invocation handler if it's a new one
  if (!known)
    invocationHandler->Initialize();
}

void CScriptInvocationManager::RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::set<std::string> &extensions)
{
  if (invocationHandler == NULL || extensions.empty())
    return;

  for (set<string>::const_iterator extension = extensions.begin(); extension != extensions.end(); ++extension)
    RegisterLanguageInvocationHandler(invocationHandler, *extension);
}

void CScriptInvocationManager::UnregisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler)
{
  if (invocationHandler == NULL)
    return;

  CSingleLock lock(m_critSection);
  //  get all extensions of the given language invoker
  for (map<string, ILanguageInvocationHandler*>::iterator it = m_invocationHandlers.begin(); it != m_invocationHandlers.end(); )
  {
    if (it->second == invocationHandler)
      m_invocationHandlers.erase(it++);
    else
      ++it;
  }

  // automatically uninitialize the invocation handler
  invocationHandler->Uninitialize();
}

bool CScriptInvocationManager::HasLanguageInvoker(const std::string &script) const
{
  std::string extension = URIUtils::GetExtension(script);
  StringUtils::ToLower(extension);

  CSingleLock lock(m_critSection);
  map<string, ILanguageInvocationHandler*>::const_iterator it = m_invocationHandlers.find(extension);
  return it != m_invocationHandlers.end() && it->second != NULL;
}

LanguageInvokerPtr CScriptInvocationManager::GetLanguageInvoker(const std::string &script) const
{
  std::string extension = URIUtils::GetExtension(script);
  StringUtils::ToLower(extension);

  CSingleLock lock(m_critSection);
  map<string, ILanguageInvocationHandler*>::const_iterator it = m_invocationHandlers.find(extension);
  if (it != m_invocationHandlers.end() && it->second != NULL)
    return LanguageInvokerPtr(it->second->CreateInvoker());

  return LanguageInvokerPtr();
}

int CScriptInvocationManager::ExecuteAsync(const std::string &script, const ADDON::AddonPtr &addon /* = ADDON::AddonPtr() */, const std::vector<std::string> &arguments /* = std::vector<std::string>() */)
{
  if (script.empty())
    return -1;

  if (!CFile::Exists(script, false))
  {
    CLog::Log(LOGERROR, "%s - Not executing non-existing script %s", __FUNCTION__, script.c_str());
    return -1;
  }

  LanguageInvokerPtr invoker = GetLanguageInvoker(script);
  return ExecuteAsync(script, invoker, addon, arguments);
}

int CScriptInvocationManager::ExecuteAsync(const std::string &script, LanguageInvokerPtr languageInvoker, const ADDON::AddonPtr &addon /* = ADDON::AddonPtr() */, const std::vector<std::string> &arguments /* = std::vector<std::string>() */)
{
  if (script.empty() || languageInvoker == NULL)
    return -1;

  if (!CFile::Exists(script, false))
  {
    CLog::Log(LOGERROR, "%s - Not executing non-existing script %s", __FUNCTION__, script.c_str());
    return -1;
  }

  CLanguageInvokerThreadPtr invokerThread = CLanguageInvokerThreadPtr(new CLanguageInvokerThread(languageInvoker, this));
  if (invokerThread == NULL)
    return -1;

  if (addon != NULL)
    invokerThread->SetAddon(addon);

  CSingleLock lock(m_critSection);
  invokerThread->SetId(m_nextId++);
  lock.Leave();

  LanguageInvokerThread thread = { invokerThread, script, false };
  m_scripts.insert(make_pair(invokerThread->GetId(), thread));
  m_scriptPaths.insert(make_pair(script, invokerThread->GetId()));
  invokerThread->Execute(script, arguments);

  return invokerThread->GetId();
}

int CScriptInvocationManager::ExecuteSync(const std::string &script, const ADDON::AddonPtr &addon /* = ADDON::AddonPtr() */, const std::vector<std::string> &arguments /* = std::vector<std::string>() */, uint32_t timeoutMs /* = 0 */, bool waitShutdown /* = false */)
{
  if (script.empty())
    return -1;

  if (!CFile::Exists(script, false))
  {
    CLog::Log(LOGERROR, "%s - Not executing non-existing script %s", __FUNCTION__, script.c_str());
    return -1;
  }

  LanguageInvokerPtr invoker = GetLanguageInvoker(script);
  return ExecuteSync(script, invoker, addon, arguments, timeoutMs, waitShutdown);
}

int CScriptInvocationManager::ExecuteSync(const std::string &script, LanguageInvokerPtr languageInvoker, const ADDON::AddonPtr &addon /* = ADDON::AddonPtr() */, const std::vector<std::string> &arguments /* = std::vector<std::string>() */, uint32_t timeoutMs /* = 0 */, bool waitShutdown /* = false */)
{
  int scriptId = ExecuteAsync(script, languageInvoker, addon, arguments);
  if (scriptId < 0)
    return -1;

  bool timeout = timeoutMs > 0;
  while ((!timeout || timeoutMs > 0) && IsRunning(scriptId))
  {
    unsigned int sleepMs = 100U;
    if (timeout && timeoutMs < sleepMs)
      sleepMs = timeoutMs;

    Sleep(sleepMs);

    if (timeout)
      timeoutMs -= sleepMs;
  }

  if (IsRunning(scriptId))
  {
    Stop(scriptId, waitShutdown);
    return ETIMEDOUT;
  }

  return 0;
}

bool CScriptInvocationManager::Stop(int scriptId, bool wait /* = false */)
{
  if (scriptId < 0)
    return false;

  CSingleLock lock(m_critSection);
  CLanguageInvokerThreadPtr invokerThread = getInvokerThread(scriptId).thread;
  if (invokerThread == NULL)
    return false;

  return invokerThread->Stop(wait);
}

bool CScriptInvocationManager::Stop(const std::string &scriptPath, bool wait /* = false */)
{
  if (scriptPath.empty())
    return false;

  CSingleLock lock(m_critSection);
  std::map<std::string, int>::const_iterator script = m_scriptPaths.find(scriptPath);
  if (script == m_scriptPaths.end())
    return false;

  return Stop(script->second, wait);
}

bool CScriptInvocationManager::IsRunning(int scriptId) const
{
  CSingleLock lock(m_critSection);
  LanguageInvokerThread invokerThread = getInvokerThread(scriptId);
  if (invokerThread.thread == NULL)
    return false;

  return !invokerThread.done;
}

bool CScriptInvocationManager::IsRunning(const std::string& scriptPath) const
{
  CSingleLock lock(m_critSection);
  auto it = m_scriptPaths.find(scriptPath);
  if (it == m_scriptPaths.end())
    return false;

  return IsRunning(it->second);
}

void CScriptInvocationManager::OnScriptEnded(int scriptId)
{
  if (scriptId < 0)
    return;

  CSingleLock lock(m_critSection);
  LanguageInvokerThreadMap::iterator script = m_scripts.find(scriptId);
  if (script != m_scripts.end())
    script->second.done = true;
}

CScriptInvocationManager::LanguageInvokerThread CScriptInvocationManager::getInvokerThread(int scriptId) const
{
  if (scriptId < 0)
    return LanguageInvokerThread();

  LanguageInvokerThreadMap::const_iterator script = m_scripts.find(scriptId);
  if (script == m_scripts.end())
    return LanguageInvokerThread();

  return script->second;
}
