/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScriptInvocationManager.h"

#include "addons/AddonSystemSettings.h"
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
  for (LanguageInvokerThreadMap::iterator it = m_scripts.begin(); it != m_scripts.end(); )
  {
    if (it->second->IsDone() && !it->second->Reuseable())
    {
      m_scripts.erase(it++);
    }
    else
      ++it;
  }

  std::vector<ILanguageInvocationHandler*> invocationHandlers;
  invocationHandlers.reserve(m_invocationHandlers.size());
  
  for (auto& handler : m_invocationHandlers)
    invocationHandlers.push_back(handler.second);

  lock.unlock();

  for (auto& handler : invocationHandlers)
  {
    handler->Process();
  }

  // threads to stop exits scope and kills all threads.
}

void CScriptInvocationManager::Uninitialize()
{
  // execute Process() once more to handle the remaining scripts
  Process();

  std::unique_lock lock(m_critSection);
  m_scripts.clear();

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
    bool reuseable /* = false */)
{
  if (script.empty())
    return -1;

  if (!CFileUtils::Exists(script, false))
  {
    CLog::Log(LOGERROR, "{} - Not executing non-existing script {}", __FUNCTION__, script);
    return -1;
  }

  // We're going to try to find a thread to reuse.
  // If we can't find a suitable one, we'll kill another ready reusable thread off so we can replace it for this script.
  if (reuseable)
  {
    const int maxReusableThreads = ADDON::CAddonSystemSettings::GetInstance().GetMaxReusableThreads();
    
    CLanguageInvokerThreadPtr firstDoneReusableThread;
    const auto reusableThreads = GetReusableThreads();
    for (const CLanguageInvokerThreadPtr& thread : reusableThreads)
    {
      if (!thread->IsDone())
      {
        continue;
      }

      if (thread->GetScript() == script)
      {
        thread->GetInvoker()->Reset();
        thread->Execute(script, arguments);
        return thread->GetId();
      }
      else if (!firstDoneReusableThread)
      {
        firstDoneReusableThread = thread;
      }
    }

    // If we've run out of resumable threads.
    if (reusableThreads.size() >= maxReusableThreads)
    {
      // And there are no threads we can close down.
      if (!firstDoneReusableThread)
      {
        // We can't create a reusable thread.
        reuseable = false;
      }
      else
      {
        // Otherwise close down an existing reusable thread and make a new one.
        firstDoneReusableThread->Stop();
      }
    }
  }

  return ExecuteAsync(script, GetLanguageInvoker(script), addon, arguments, reuseable);
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

  auto thread = AddThread(languageInvoker, reuseable);
  if (addon != NULL)
    thread->SetAddon(addon);
  thread->Execute(script, arguments);
  return thread->GetId();
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

  CLanguageInvokerThreadPtr invokerThread = GetThread(scriptId);
  if (invokerThread == NULL)
    return false;

  return invokerThread->Stop(wait);
}

void CScriptInvocationManager::StopRunningScripts(bool wait /* = false */)
{
  for (auto& it : GetAllThreads())
  {
    if (!it->IsDone())
      it->Stop(wait);
  }
}

bool CScriptInvocationManager::Stop(const std::string &scriptPath, bool wait /* = false */)
{
  if (scriptPath.empty())
    return false;

  bool stoppedAny = false;
  for (auto& it : GetAllThreads())
  {
    if (!it->IsDone() && it->GetScript() == scriptPath)
    {
      it->Stop(wait);
      stoppedAny = true;
    }
  }

  return stoppedAny;
}

bool CScriptInvocationManager::IsRunning(int scriptId) const
{
  if (auto thread = GetThread(scriptId))
  {
    return !thread->IsDone();
  }
  return false;
}

bool CScriptInvocationManager::IsRunning(const std::string& scriptPath) const
{
  for (auto& it : GetAllThreads())
  {
    if (!it->IsDone() && it->GetScript() == scriptPath)
    {
      return true;
    }
  }

  return false;
}

CLanguageInvokerThreadPtr CScriptInvocationManager::AddThread(
    const std::shared_ptr<ILanguageInvoker>& languageInvoker, bool reusable)
{
  std::unique_lock lock(m_critSection);
  CLanguageInvokerThreadPtr newInvokerThread =
      std::make_shared<CLanguageInvokerThread>(languageInvoker, reusable);
  
  if (newInvokerThread)
  {
    newInvokerThread->SetId(m_nextId++);
    m_scripts.insert(std::make_pair(newInvokerThread->GetId(), newInvokerThread));
    return newInvokerThread;
  }
  return nullptr;
}

CLanguageInvokerThreadPtr CScriptInvocationManager::GetThread(int scriptId) const
{
  if (scriptId < 0)
    return {};

  std::unique_lock lock(m_critSection);
  LanguageInvokerThreadMap::const_iterator script = m_scripts.find(scriptId);
  if (script == m_scripts.end())
    return {};

  return script->second;
}

std::vector<CLanguageInvokerThreadPtr> CScriptInvocationManager::GetAllThreads() const
{
  std::unique_lock lock(m_critSection);
  std::vector<CLanguageInvokerThreadPtr> tempList;

  for (const auto& script : m_scripts)
    if (script.second)
        tempList.push_back(script.second);

  return tempList;
}

std::vector<CLanguageInvokerThreadPtr> CScriptInvocationManager::GetReusableThreads() const
{
  std::unique_lock lock(m_critSection);
  std::vector<CLanguageInvokerThreadPtr> tempList;

  for (const auto& script : m_scripts)
    if (script.second && script.second->Reuseable())
      tempList.push_back(script.second);

  return tempList;
}