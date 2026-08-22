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

CReusableInvokerClaim& CReusableInvokerClaim::operator=(CReusableInvokerClaim&& other) noexcept
{
  if (this != &other)
  {
    Release();
    m_thread = std::move(other.m_thread);
    m_pluginHandle = other.m_pluginHandle;
    other.m_thread = nullptr;
    other.m_pluginHandle = -1;
  }
  return *this;
}

void CReusableInvokerClaim::Release()
{
  if (!m_thread)
    return;

  m_thread->ReleaseClaim();
  m_thread = nullptr;
  m_pluginHandle = -1;
}

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
  m_lastPluginHandle = -1;

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

CReusableInvokerClaim CScriptInvocationManager::ClaimReusableInvoker(const std::string& script)
{
  std::unique_lock lock(m_critSection);

  if (m_lastInvokerThread)
  {
    // The interpreter is only worth claiming together with the plugin handle it
    // was created for. Without one the caller has to allocate a new handle and
    // would run on a fresh invoker anyway, so claiming here would just mark a
    // slot that the returned claim could never be matched back to.
    if (m_lastPluginHandle >= 0 && m_lastInvokerThread->Reuseable(script) &&
        m_lastInvokerThread->Claim())
    {
      m_lastInvokerThread->GetInvoker()->Reset();
      CLog::Log(LOGDEBUG, "{} - claimed reusable plugin handle {} on LanguageInvokerThread {}",
                __FUNCTION__, m_lastPluginHandle, m_lastInvokerThread->GetId());
      return {m_lastInvokerThread, m_lastPluginHandle};
    }
    ReleaseLastInvokerIfIdle();
  }
  return {};
}

std::shared_ptr<ILanguageInvoker> CScriptInvocationManager::GetLanguageInvoker(
    const std::string& script, int pluginHandle /* = -1 */)
{
  std::unique_lock lock(m_critSection);

  if (m_lastInvokerThread)
  {
    // The caller holds a claim on this thread, which was granted for this script
    // and stops anyone else taking it, so the reuse checks were already made at
    // claim time. Stop() can still have run since, which is what is left to test.
    const bool claimedByCaller =
        pluginHandle >= 0 && pluginHandle == m_lastPluginHandle && m_lastInvokerThread->IsClaimed();
    if (claimedByCaller)
    {
      if (!m_lastInvokerThread->IsStopRequested() && !m_lastInvokerThread->IsProcessDone())
        return m_lastInvokerThread->GetInvoker();
    }
    else
    {
      // Scripts run without a plugin handle have no claim holder of their own,
      // so take the claim here; ExecuteAsync drops it once the thread has been
      // handed the script. A claim held by somebody else fails this and falls
      // through to a new invoker.
      if (pluginHandle < 0 && m_lastInvokerThread->Reuseable(script) &&
          m_lastInvokerThread->Claim())
      {
        CLog::Log(LOGDEBUG, "{} - Reusing LanguageInvokerThread {} for script {}", __FUNCTION__,
                  m_lastInvokerThread->GetId(), script);
        m_lastInvokerThread->GetInvoker()->Reset();
        return m_lastInvokerThread->GetInvoker();
      }
      // A concurrent script must not Release() a claimed or running invoker.
      ReleaseLastInvokerIfIdle();
    }
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

  auto invoker = GetLanguageInvoker(script, pluginHandle);
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
    // A claim taken by GetLanguageInvoker for a script without a plugin handle
    // has no other owner, so it is dropped once the thread has the script.
    // Handing over is safe because the invoker was Reset() out of ScriptDone to
    // run this script, and IsBusy() counts anything but a parked ScriptDone
    // thread as occupied, so nothing can Release() it in the gap.
    const bool ownsClaim = pluginHandle < 0;
    lock.unlock();
    const bool started = invokerThread->Execute(script, arguments);
    if (ownsClaim)
      invokerThread->ReleaseClaim();

    if (started)
      return invokerThread->GetId();

    // Only reachable if the thread was stopped between the claim and here. Say
    // so rather than handing back an id for a script that will never run and
    // leaving the caller to wait on it.
    CLog::Log(LOGDEBUG, "{} - LanguageInvokerThread {} refused {}, it is stopping", __FUNCTION__,
              invokerThread->GetId(), script);
    return -1;
  }

  const bool lastBusy = LastInvokerOccupied();
  auto invokerThread =
      std::make_shared<CLanguageInvokerThread>(languageInvoker, this, reuseable && !lastBusy);

  if (addon != nullptr)
    invokerThread->SetAddon(addon);

  invokerThread->SetId(m_nextId++);

  LanguageInvokerThread thread = {invokerThread, script, false};
  m_scripts.insert(std::make_pair(invokerThread->GetId(), thread));
  m_scriptPaths.insert(std::make_pair(script, invokerThread->GetId()));

  // Do not steal the reusable slot from a claimed or running last thread, and
  // only ever park a thread there that is allowed to be reused. A service
  // add-on runs until shutdown, so letting one occupy the slot pinned it for
  // the session and turned reuselanguageinvoker off for everything else.
  if (!lastBusy && reuseable)
  {
    ReleaseLastInvokerIfIdle();
    m_lastInvokerThread = invokerThread;
    m_lastPluginHandle = pluginHandle;
  }

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

bool CScriptInvocationManager::LastInvokerOccupied() const
{
  return m_lastInvokerThread && m_lastInvokerThread->IsBusy();
}

void CScriptInvocationManager::ReleaseLastInvokerIfIdle()
{
  if (!LastInvokerOccupied() && m_lastInvokerThread)
  {
    m_lastInvokerThread->Release();
    m_lastInvokerThread = nullptr;
    m_lastPluginHandle = -1;
  }
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
