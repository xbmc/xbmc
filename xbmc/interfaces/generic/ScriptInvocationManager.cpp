/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScriptInvocationManager.h"

#include "filesystem/File.h"
#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/LanguageInvokerThread.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <cerrno>
#include <utility>
#include <vector>

using namespace XFILE;

CScriptInvocationManager::~CScriptInvocationManager()
{
  Uninitialize();
}

CScriptInvocationManager& CScriptInvocationManager::GetInstance()
{
  static CScriptInvocationManager s_instance;
  return s_instance;
}

void CScriptInvocationManager::Process()
{
  CSingleLock lock(m_critSection);
  // go through all active threads and find and remove all which are done
  std::vector<LanguageInvokerThread> tempList;
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
  for (const auto& it : tempList)
    m_scriptPaths.erase(it.script);

  // we can leave the lock now
  lock.Leave();

  // finally remove the finished threads but we do it outside of any locks in
  // case of any callbacks from the destruction of the CLanguageInvokerThread
  tempList.clear();

  // let the invocation handlers do their processing
  for (auto& it : m_invocationHandlers)
    it.second->Process();
}

void CScriptInvocationManager::Uninitialize()
{
  CSingleLock lock(m_critSection);

  // execute Process() once more to handle the remaining scripts
  Process();

  // it is safe to relese early, thread must be in m_scripts too
  m_lastInvokerThread = nullptr;

  // make sure all scripts are done
  std::vector<LanguageInvokerThread> tempList;
  for (const auto& script : m_scripts)
    tempList.push_back(script.second);

  m_scripts.clear();
  m_scriptPaths.clear();

  // we can leave the lock now
  lock.Leave();

  // finally stop and remove the finished threads but we do it outside of any
  // locks in case of any callbacks from the stop or destruction logic of
  // CLanguageInvokerThread or the ILanguageInvoker implementation
  for (auto& it : tempList)
  {
    if (!it.done)
      it.thread->Stop(true);
  }

  lock.Enter();

  tempList.clear();

  // uninitialize all invocation handlers and then remove them
  for (auto& it : m_invocationHandlers)
    it.second->Uninitialize();

  m_invocationHandlers.clear();
}

void CScriptInvocationManager::RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::string &extension)
{
  if (invocationHandler == NULL || extension.empty())
    return;

  std::string ext = extension;
  StringUtils::ToLower(ext);
  if (!StringUtils::StartsWithNoCase(ext, "."))
    ext = "." + ext;

  CSingleLock lock(m_critSection);
  if (m_invocationHandlers.find(ext) != m_invocationHandlers.end())
    return;

  m_invocationHandlers.insert(std::make_pair(extension, invocationHandler));

  bool known = false;
  for (const auto& it : m_invocationHandlers)
  {
    if (it.second == invocationHandler)
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

  for (const auto& extension : extensions)
    RegisterLanguageInvocationHandler(invocationHandler, extension);
}

void CScriptInvocationManager::UnregisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler)
{
  if (invocationHandler == NULL)
    return;

  CSingleLock lock(m_critSection);
  //  get all extensions of the given language invoker
  for (std::map<std::string, ILanguageInvocationHandler*>::iterator it = m_invocationHandlers.begin(); it != m_invocationHandlers.end(); )
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
  std::map<std::string, ILanguageInvocationHandler*>::const_iterator it = m_invocationHandlers.find(extension);
  return it != m_invocationHandlers.end() && it->second != NULL;
}

int CScriptInvocationManager::GetReusablePluginHandle(const std::string& script)
{
  CSingleLock lock(m_critSection);

  if (m_lastInvokerThread)
  {
    if (m_lastInvokerThread->Reuseable(script))
      return m_lastPluginHandle;
    m_lastInvokerThread->Release();
    m_lastInvokerThread = nullptr;
  }
  return -1;
}

LanguageInvokerPtr CScriptInvocationManager::GetLanguageInvoker(const std::string& script)
{
  CSingleLock lock(m_critSection);

  if (m_lastInvokerThread)
  {
    if (m_lastInvokerThread->Reuseable(script))
    {
      CLog::Log(LOGDEBUG, "%s - Reusing LanguageInvokerThread %d for script %s", __FUNCTION__,
                m_lastInvokerThread->GetId(), script.c_str());
      m_lastInvokerThread->GetInvoker()->Reset();
      return m_lastInvokerThread->GetInvoker();
    }
    m_lastInvokerThread->Release();
    m_lastInvokerThread = nullptr;
  }

  std::string extension = URIUtils::GetExtension(script);
  StringUtils::ToLower(extension);

  std::map<std::string, ILanguageInvocationHandler*>::const_iterator it = m_invocationHandlers.find(extension);
  if (it != m_invocationHandlers.end() && it->second != NULL)
    return LanguageInvokerPtr(it->second->CreateInvoker());

  return LanguageInvokerPtr();
}

int CScriptInvocationManager::ExecuteAsync(
    const std::string& script,
    const ADDON::AddonPtr& addon /* = ADDON::AddonPtr() */,
    const std::vector<std::string>& arguments /* = std::vector<std::string>() */,
    bool reuseable /* = false */,
    int pluginHandle /* = -1 */)
{
  if (script.empty())
    return -1;

  if (!CFile::Exists(script, false))
  {
    CLog::Log(LOGERROR, "%s - Not executing non-existing script %s", __FUNCTION__, script.c_str());
    return -1;
  }

  LanguageInvokerPtr invoker = GetLanguageInvoker(script);
  return ExecuteAsync(script, invoker, addon, arguments, reuseable, pluginHandle);
}

int CScriptInvocationManager::ExecuteAsync(
    const std::string& script,
    LanguageInvokerPtr languageInvoker,
    const ADDON::AddonPtr& addon /* = ADDON::AddonPtr() */,
    const std::vector<std::string>& arguments /* = std::vector<std::string>() */,
    bool reuseable /* = false */,
    int pluginHandle /* = -1 */)
{
  if (script.empty() || languageInvoker == NULL)
    return -1;

  if (!CFile::Exists(script, false))
  {
    CLog::Log(LOGERROR, "%s - Not executing non-existing script %s", __FUNCTION__, script.c_str());
    return -1;
  }

  CSingleLock lock(m_critSection);

  if (m_lastInvokerThread && m_lastInvokerThread->GetInvoker() == languageInvoker)
  {
    if (addon != NULL)
      m_lastInvokerThread->SetAddon(addon);

    // After we leave the lock, m_lastInvokerThread can be released -> copy!
    CLanguageInvokerThreadPtr invokerThread = m_lastInvokerThread;
    lock.Leave();
    invokerThread->Execute(script, arguments);

    return invokerThread->GetId();
  }

  m_lastInvokerThread =
      CLanguageInvokerThreadPtr(new CLanguageInvokerThread(languageInvoker, this, reuseable));
  if (m_lastInvokerThread == NULL)
    return -1;

  if (addon != NULL)
    m_lastInvokerThread->SetAddon(addon);

  m_lastInvokerThread->SetId(m_nextId++);
  m_lastPluginHandle = pluginHandle;

  LanguageInvokerThread thread = {m_lastInvokerThread, script, false};
  m_scripts.insert(std::make_pair(m_lastInvokerThread->GetId(), thread));
  m_scriptPaths.insert(std::make_pair(script, m_lastInvokerThread->GetId()));
  // After we leave the lock, m_lastInvokerThread can be released -> copy!
  CLanguageInvokerThreadPtr invokerThread = m_lastInvokerThread;
  lock.Leave();
  invokerThread->Execute(script, arguments);

  return invokerThread->GetId();
}

int CScriptInvocationManager::ExecuteSync(
    const std::string& script,
    const ADDON::AddonPtr& addon /* = ADDON::AddonPtr() */,
    const std::vector<std::string>& arguments /* = std::vector<std::string>() */,
    uint32_t timeoutMs /* = 0 */,
    bool waitShutdown /* = false */)
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

int CScriptInvocationManager::ExecuteSync(
    const std::string& script,
    LanguageInvokerPtr languageInvoker,
    const ADDON::AddonPtr& addon /* = ADDON::AddonPtr() */,
    const std::vector<std::string>& arguments /* = std::vector<std::string>() */,
    uint32_t timeoutMs /* = 0 */,
    bool waitShutdown /* = false */)
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

    KODI::TIME::Sleep(sleepMs);

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

void CScriptInvocationManager::StopRunningScripts(bool wait /* = false */)
{
  for (auto& it : m_scripts)
  {
    if (!it.second.done)
      Stop(it.second.script, wait);
  }
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

void CScriptInvocationManager::OnExecutionDone(int scriptId)
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
