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

#include <algorithm>
#include <cerrno>
#include <memory>
#include <mutex>
#include <tuple>
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
  std::erase_if(m_scripts,
                [&paths = m_scriptPaths](const auto& it)
                {
                  const auto& [key, script] = it;
                  if (script.done)
                    paths.erase(script.script);
                  return script.done;
                });

  // we can leave the lock now
  lock.unlock();

  // let the invocation handlers do their processing
  std::ranges::for_each(m_invocationHandlers, [](auto& handler) { handler.second->Process(); });
}

void CScriptInvocationManager::Uninitialize()
{
  std::unique_lock lock(m_critSection);

  // execute Process() once more to handle the remaining scripts
  Process();

  // it is safe to release early, thread must be in m_scripts too
  m_lastInvokerThread = nullptr;

  // make sure all scripts are done
  std::vector<LanguageInvokerThread> tempList;
  std::ranges::transform(m_scripts, std::back_inserter(tempList),
                         [](const auto& script) { return script.second; });

  m_scripts.clear();
  m_scriptPaths.clear();

  // we can leave the lock now
  lock.unlock();

  // finally stop and remove the finished threads but we do it outside of any
  // locks in case of any callbacks from the stop or destruction logic of
  // CLanguageInvokerThread or the ILanguageInvoker implementation
  std::ranges::for_each(tempList,
                        [](auto& val)
                        {
                          if (!val.done)
                            val.thread->Stop(true);
                        });

  lock.lock();

  tempList.clear();

  // uninitialize all invocation handlers and then remove them
  std::ranges::for_each(m_invocationHandlers,
                        [](auto& handler) { handler.second->Uninitialize(); });

  m_invocationHandlers.clear();
}

void CScriptInvocationManager::RegisterLanguageInvocationHandler(
    ILanguageInvocationHandler* invocationHandler, const std::string& extension)
{
  if (invocationHandler == nullptr || extension.empty())
    return;

  std::string ext = extension;
  StringUtils::ToLower(ext);
  if (!StringUtils::StartsWithNoCase(ext, "."))
    ext = "." + ext;

  std::unique_lock lock(m_critSection);

  const bool known =
      std::ranges::any_of(m_invocationHandlers, [invocationHandler](const auto& handler)
                          { return handler.second == invocationHandler; });

  bool inserted;
  std::tie(std::ignore, inserted) = m_invocationHandlers.emplace(ext, invocationHandler);

  // automatically initialize the invocation handler if it's a new one
  if (inserted && !known)
    invocationHandler->Initialize();
}

void CScriptInvocationManager::RegisterLanguageInvocationHandler(
    ILanguageInvocationHandler* invocationHandler, const std::set<std::string>& extensions)
{
  if (invocationHandler == nullptr || extensions.empty())
    return;

  std::ranges::for_each(extensions, [&invocationHandler, this](const auto& extension)
                        { RegisterLanguageInvocationHandler(invocationHandler, extension); });
}

void CScriptInvocationManager::UnregisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler)
{
  if (invocationHandler == nullptr)
    return;

  std::unique_lock lock(m_critSection);
  //  get all extensions of the given language invoker
  std::erase_if(m_invocationHandlers, [&invocationHandler](const auto& handler)
                { return handler.second == invocationHandler; });

  // automatically uninitialize the invocation handler
  invocationHandler->Uninitialize();
}

bool CScriptInvocationManager::HasLanguageInvoker(const std::string &script) const
{
  std::string extension = URIUtils::GetExtension(script);
  StringUtils::ToLower(extension);

  std::unique_lock lock(m_critSection);
  const auto it = m_invocationHandlers.find(extension);
  return it != m_invocationHandlers.end() && it->second != nullptr;
}

int CScriptInvocationManager::GetReusablePluginHandle(const std::string& script)
{
  std::unique_lock lock(m_critSection);

  if (m_lastInvokerThread)
  {
    if (m_lastInvokerThread->Reuseable(script))
      return m_lastPluginHandle;
    m_lastInvokerThread->Release();
    m_lastInvokerThread = nullptr;
  }
  return -1;
}

std::shared_ptr<ILanguageInvoker> CScriptInvocationManager::GetLanguageInvoker(
    const std::string& script)
{
  std::unique_lock lock(m_critSection);

  if (m_lastInvokerThread)
  {
    if (m_lastInvokerThread->Reuseable(script))
    {
      CLog::Log(LOGDEBUG, "{} - Reusing LanguageInvokerThread {} for script {}", __FUNCTION__,
                m_lastInvokerThread->GetId(), script);
      m_lastInvokerThread->GetInvoker()->Reset();
      return m_lastInvokerThread->GetInvoker();
    }
    m_lastInvokerThread->Release();
    m_lastInvokerThread = nullptr;
  }

  std::string extension = URIUtils::GetExtension(script);
  StringUtils::ToLower(extension);

  const auto it = m_invocationHandlers.find(extension);
  if (it != m_invocationHandlers.end() && it->second != nullptr)
    return it->second->CreateInvoker();

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

  auto invoker = GetLanguageInvoker(script);
  return ExecuteAsync(script, invoker, addon, arguments, reuseable, pluginHandle);
}

int CScriptInvocationManager::ExecuteAsync(
    const std::string& script,
    const std::shared_ptr<ILanguageInvoker>& languageInvoker,
    const ADDON::AddonPtr& addon /* = ADDON::AddonPtr() */,
    const std::vector<std::string>& arguments /* = std::vector<std::string>() */,
    bool reuseable /* = false */,
    int pluginHandle /* = -1 */)
{
  if (script.empty() || languageInvoker == nullptr)
    return -1;

  if (!CFileUtils::Exists(script, false))
  {
    CLog::Log(LOGERROR, "{} - Not executing non-existing script {}", __FUNCTION__, script);
    return -1;
  }

  std::unique_lock lock(m_critSection);

  if (m_lastInvokerThread && m_lastInvokerThread->GetInvoker() == languageInvoker)
  {
    if (addon != nullptr)
      m_lastInvokerThread->SetAddon(addon);

    // After we leave the lock, m_lastInvokerThread can be released -> copy!
    auto invokerThread = m_lastInvokerThread;
    lock.unlock();
    invokerThread->Execute(script, arguments);

    return invokerThread->GetId();
  }

  m_lastInvokerThread = std::make_shared<CLanguageInvokerThread>(languageInvoker, this, reuseable);
  if (m_lastInvokerThread == nullptr)
    return -1;

  if (addon != nullptr)
    m_lastInvokerThread->SetAddon(addon);

  m_lastInvokerThread->SetId(m_nextId++);
  m_lastPluginHandle = pluginHandle;

  LanguageInvokerThread thread = {m_lastInvokerThread, script, false};
  m_scripts.insert(std::make_pair(m_lastInvokerThread->GetId(), thread));
  m_scriptPaths.insert(std::make_pair(script, m_lastInvokerThread->GetId()));
  // After we leave the lock, m_lastInvokerThread can be released -> copy!
  auto invokerThread = m_lastInvokerThread;
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
  auto invokerThread = getInvokerThread(scriptId).thread;
  if (invokerThread == nullptr)
    return false;

  return invokerThread->Stop(wait);
}

void CScriptInvocationManager::StopRunningScripts(bool wait /* = false */)
{
  std::ranges::for_each(m_scripts,
                        [wait, this](auto& script)
                        {
                          if (!script.second.done)
                            Stop(script.second.script, wait);
                        });
}

bool CScriptInvocationManager::Stop(const std::string &scriptPath, bool wait /* = false */)
{
  if (scriptPath.empty())
    return false;

  std::unique_lock lock(m_critSection);
  const auto script = m_scriptPaths.find(scriptPath);
  if (script == m_scriptPaths.end())
    return false;

  return Stop(script->second, wait);
}

bool CScriptInvocationManager::IsRunning(int scriptId) const
{
  std::unique_lock lock(m_critSection);
  LanguageInvokerThread invokerThread = getInvokerThread(scriptId);
  if (invokerThread.thread == nullptr)
    return false;

  return !invokerThread.done;
}

bool CScriptInvocationManager::IsRunning(const std::string& scriptPath) const
{
  std::unique_lock lock(m_critSection);
  const auto it = m_scriptPaths.find(scriptPath);
  if (it == m_scriptPaths.end())
    return false;

  return IsRunning(it->second);
}

void CScriptInvocationManager::OnExecutionDone(int scriptId)
{
  if (scriptId < 0)
    return;

  std::unique_lock lock(m_critSection);
  const auto script = m_scripts.find(scriptId);
  if (script != m_scripts.end())
    script->second.done = true;
}

CScriptInvocationManager::LanguageInvokerThread CScriptInvocationManager::getInvokerThread(int scriptId) const
{
  if (scriptId < 0)
    return LanguageInvokerThread();

  const auto script = m_scripts.find(scriptId);
  if (script == m_scripts.end())
    return LanguageInvokerThread();

  return script->second;
}
