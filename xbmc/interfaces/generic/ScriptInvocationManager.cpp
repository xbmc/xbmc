/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScriptInvocationManager.h"

#include "interfaces/generic/ILanguageInvocationHandler.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "interfaces/generic/LanguageInvokerThread.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <cerrno>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

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
  std::unique_lock lock(m_critSection);
  // go through all active threads and find and remove all which are done
  std::vector<LanguageInvokerThread> tempList;
  for (LanguageInvokerThreadMap::iterator it = m_scripts.begin(); it != m_scripts.end(); )
  {
    if (it->second.done && !it->second.thread->Reuseable())
    {
      m_scripts.erase(it++);
    }
    else
      ++it;
  }

  // we can leave the lock now
  lock.unlock();

  // let the invocation handlers do their processing
  for (auto& it : m_invocationHandlers)
    it.second->Process();
}

void CScriptInvocationManager::Uninitialize()
{
  std::unique_lock lock(m_critSection);

  // execute Process() once more to handle the remaining scripts
  Process();

  // it is safe to release early, thread must be in m_scripts too
  //m_lastInvokerThread = nullptr;

  // make sure all scripts are done
  std::vector<LanguageInvokerThread> tempList;
  for (const auto& script : m_scripts)
    tempList.push_back(script.second);

  m_scripts.clear();

  // we can leave the lock now
  lock.unlock();

  // finally stop and remove the finished threads but we do it outside of any
  // locks in case of any callbacks from the stop or destruction logic of
  // CLanguageInvokerThread or the ILanguageInvoker implementation
  for (auto& it : tempList)
  {
    if (!it.done)
      it.thread->Stop(true);
  }

  lock.lock();

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

  std::unique_lock lock(m_critSection);
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

  std::unique_lock lock(m_critSection);
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

  std::unique_lock lock(m_critSection);
  std::map<std::string, ILanguageInvocationHandler*>::const_iterator it = m_invocationHandlers.find(extension);
  return it != m_invocationHandlers.end() && it->second != NULL;
}

int CScriptInvocationManager::GetReusablePluginHandle(const std::string& script)
{
  return -1;
}

std::shared_ptr<ILanguageInvoker> CScriptInvocationManager::GetLanguageInvoker(
    const std::string& script)
{
  std::unique_lock lock(m_critSection);

  std::string extension = URIUtils::GetExtension(script);
  StringUtils::ToLower(extension);

  std::map<std::string, ILanguageInvocationHandler*>::const_iterator it = m_invocationHandlers.find(extension);
  if (it != m_invocationHandlers.end() && it->second != NULL)
    return std::shared_ptr<ILanguageInvoker>(it->second->CreateInvoker());

  return {};
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

  if (!CFileUtils::Exists(script, false))
  {
    CLog::Log(LOGERROR, "{} - Not executing non-existing script {}", __FUNCTION__, script);
    return -1;
  }

  int countAlreadyReusableForThisScript = 0;

  if (reuseable)
  {
    std::unique_lock lock(m_critSection);

    for (auto& it : m_scripts)
    {
      if (it.second.script == script && it.second.thread->Reuseable())
      {
        countAlreadyReusableForThisScript++;
        if (it.second.thread->GetState() == InvokerStateScriptDone)
        {
          CLanguageInvokerThreadPtr reusedThread = it.second.thread;
          it.second.done = false;
          if (addon != NULL)
            reusedThread->SetAddon(addon);

          reusedThread->GetInvoker()->Reset();
          lock.unlock();
          reusedThread->Execute(script, arguments);

          return reusedThread->GetId();
        }
      }
    }
  }

  return ExecuteAsync(script, GetLanguageInvoker(script), addon, arguments,
                      reuseable && countAlreadyReusableForThisScript <= 2);
}

int CScriptInvocationManager::ExecuteAsync(
    const std::string& script,
    const std::shared_ptr<ILanguageInvoker>& languageInvoker,
    const ADDON::AddonPtr& addon /* = ADDON::AddonPtr() */,
    const std::vector<std::string>& arguments /* = std::vector<std::string>() */,
    bool reuseable)
{
  if (script.empty() || languageInvoker == NULL)
    return -1;

  if (!CFileUtils::Exists(script, false))
  {
    CLog::Log(LOGERROR, "{} - Not executing non-existing script {}", __FUNCTION__, script);
    return -1;
  }

  std::unique_lock lock(m_critSection);

  CLanguageInvokerThreadPtr newInvokerThread = std::make_shared<CLanguageInvokerThread>(languageInvoker, this, reuseable);
  if (newInvokerThread == NULL)
    return -1;

  if (addon != NULL)
    newInvokerThread->SetAddon(addon);

  newInvokerThread->SetId(m_nextId++);

  LanguageInvokerThread thread = {newInvokerThread, script, false};
  m_scripts.insert(std::make_pair(newInvokerThread->GetId(), thread));
  // After we leave the lock, m_lastInvokerThread can be released -> copy!
  CLanguageInvokerThreadPtr invokerThread = newInvokerThread;
  lock.unlock();
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

  if (!CFileUtils::Exists(script, false))
  {
    CLog::Log(LOGERROR, "{} - Not executing non-existing script {}", __FUNCTION__, script);
    return -1;
  }

  auto invoker = GetLanguageInvoker(script);
  return ExecuteSync(script, invoker, addon, arguments, timeoutMs, waitShutdown);
}

int CScriptInvocationManager::ExecuteSync(
    const std::string& script,
    const std::shared_ptr<ILanguageInvoker>& languageInvoker,
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

    KODI::TIME::Sleep(std::chrono::milliseconds(sleepMs));

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

  std::unique_lock lock(m_critSection);
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
      Stop(it.first, wait);
  }
}

bool CScriptInvocationManager::Stop(const std::string &scriptPath, bool wait /* = false */)
{
  if (scriptPath.empty())
    return false;

  for (auto& it : m_scripts)
  {
    if (!it.second.done && it.second.script == scriptPath)
    {
      Stop(it.first, wait);
    }
  }
}

bool CScriptInvocationManager::IsRunning(int scriptId) const
{
  std::unique_lock lock(m_critSection);
  LanguageInvokerThread invokerThread = getInvokerThread(scriptId);
  if (invokerThread.thread == NULL)
    return false;

  return !invokerThread.done;
}

bool CScriptInvocationManager::IsRunning(const std::string& scriptPath) const
{
  for (auto& it : m_scripts)
  {
    if (!it.second.done && it.second.script == scriptPath)
    {
      return true;
    }
  }

  return false;
}

void CScriptInvocationManager::OnExecutionDone(int scriptId)
{
  if (scriptId < 0)
    return;

  std::unique_lock lock(m_critSection);
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
