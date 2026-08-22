/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "interfaces/generic/ILanguageInvoker.h"
#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

class CLanguageInvokerThread;

/*!
 * \brief RAII claim on the reusable language invoker and its plugin handle.
 *
 * While a claim is held CScriptInvocationManager will neither hand that invoker
 * to another script nor release it as idle. Dropping the claim - on scope exit,
 * an early return, or an exception - is what puts the slot back in circulation,
 * so a caller cannot leak the claimed state by forgetting to clean up.
 */
class CReusableInvokerClaim
{
public:
  CReusableInvokerClaim() = default;
  ~CReusableInvokerClaim() { Release(); }

  CReusableInvokerClaim(CReusableInvokerClaim&& other) noexcept { *this = std::move(other); }
  CReusableInvokerClaim& operator=(CReusableInvokerClaim&& other) noexcept;

  CReusableInvokerClaim(const CReusableInvokerClaim&) = delete;
  CReusableInvokerClaim& operator=(const CReusableInvokerClaim&) = delete;

  //! \brief True if a reusable invoker and its plugin handle were claimed.
  bool IsValid() const { return m_thread != nullptr; }

  //! \brief The claimed plugin handle, or -1 if nothing was claimed.
  int GetPluginHandle() const { return m_pluginHandle; }

  //! \brief Drop the claim. Safe to call on an invalid claim and more than once.
  void Release();

private:
  friend class CScriptInvocationManager;
  CReusableInvokerClaim(std::shared_ptr<CLanguageInvokerThread> thread, int pluginHandle)
    : m_thread(std::move(thread)),
      m_pluginHandle(pluginHandle)
  {
  }

  std::shared_ptr<CLanguageInvokerThread> m_thread;
  int m_pluginHandle{-1};
};

class CScriptInvocationManager
{
public:
  static CScriptInvocationManager& GetInstance();

  void Process();
  void Uninitialize();

  void RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::string &extension);
  void RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::set<std::string> &extensions);
  void UnregisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler);
  bool HasLanguageInvoker(const std::string& script) const;

  /*!
   * \brief Returns the invoker to run the given script with.
   *
   * \param script Path to the script to be executed
   * \param pluginHandle The plugin handle the script will run with, or -1 if it
   *        is not a plugin invocation. A handle matching an outstanding claim
   *        (see ClaimReusableInvoker) resolves to that claim's invoker.
   * \return The invoker to use, or a new one if nothing reusable is available
   */
  std::shared_ptr<ILanguageInvoker> GetLanguageInvoker(const std::string& script,
                                                       int pluginHandle = -1);

  /*!
   * \brief Claim the reusable invoker for the given script, if there is one.
   *
   * A valid claim gives the caller exclusive use of a parked interpreter and the
   * plugin handle it was created for. Hold it for as long as the script runs;
   * destroying it hands the invoker back. An invalid claim means the caller has
   * to allocate its own handle and will get a fresh invoker.
   *
   * \param script Path to the script about to be executed
   * \return The claim, which is invalid if nothing reusable was available
   */
  CReusableInvokerClaim ClaimReusableInvoker(const std::string& script);

  /*!
   * \brief Executes the given script asynchronously in a separate thread.
   *
   * \param script Path to the script to be executed
   * \param addon (Optional) Addon to which the script belongs
   * \param arguments (Optional) List of arguments passed to the script
   * \return -1 if an error occurred, otherwise the ID of the script
   */
  int ExecuteAsync(const std::string& script,
                   const ADDON::AddonPtr& addon = ADDON::AddonPtr(),
                   const std::vector<std::string>& arguments = std::vector<std::string>(),
                   bool reuseable = false,
                   int pluginHandle = -1);
  /*!
  * \brief Executes the given script asynchronously in a separate thread.
  *
  * \param script Path to the script to be executed
  * \param languageInvoker Language invoker to be used to execute the script
  * \param addon (Optional) Addon to which the script belongs
  * \param arguments (Optional) List of arguments passed to the script
  * \return -1 if an error occurred, otherwise the ID of the script
  */
  int ExecuteAsync(const std::string& script,
                   const std::shared_ptr<ILanguageInvoker>& languageInvoker,
                   const ADDON::AddonPtr& addon = ADDON::AddonPtr(),
                   const std::vector<std::string>& arguments = std::vector<std::string>(),
                   bool reuseable = false,
                   int pluginHandle = -1);

  /*!
  * \brief Executes the given script synchronously.
  *
  * \details The script is actually executed asynchronously but the calling
  * thread is blocked until either the script has finished or the given timeout
  * has expired. If the given timeout has expired the script's execution is
  * stopped and depending on the specified wait behaviour we wait for the
  * script's execution to finish or not.
  *
  * \param script Path to the script to be executed
  * \param addon (Optional) Addon to which the script belongs
  * \param arguments (Optional) List of arguments passed to the script
  * \param timeout (Optional) Timeout (in milliseconds) for the script's execution
  * \param waitShutdown (Optional) Whether to wait when having to forcefully stop the script's execution or not.
  * \return -1 if an error occurred, 0 if the script terminated or ETIMEDOUT if the given timeout expired
  */
  int ExecuteSync(const std::string& script,
                  const ADDON::AddonPtr& addon = ADDON::AddonPtr(),
                  const std::vector<std::string>& arguments = std::vector<std::string>(),
                  uint32_t timeoutMs = 0,
                  bool waitShutdown = false);
  /*!
  * \brief Executes the given script synchronously.
  *
  * \details The script is actually executed asynchronously but the calling
  * thread is blocked until either the script has finished or the given timeout
  * has expired. If the given timeout has expired the script's execution is
  * stopped and depending on the specified wait behaviour we wait for the
  * script's execution to finish or not.
  *
  * \param script Path to the script to be executed
  * \param languageInvoker Language invoker to be used to execute the script
  * \param addon (Optional) Addon to which the script belongs
  * \param arguments (Optional) List of arguments passed to the script
  * \param timeout (Optional) Timeout (in milliseconds) for the script's execution
  * \param waitShutdown (Optional) Whether to wait when having to forcefully stop the script's execution or not.
  * \return -1 if an error occurred, 0 if the script terminated or ETIMEDOUT if the given timeout expired
  */
  int ExecuteSync(const std::string& script,
                  const std::shared_ptr<ILanguageInvoker>& languageInvoker,
                  const ADDON::AddonPtr& addon = ADDON::AddonPtr(),
                  const std::vector<std::string>& arguments = std::vector<std::string>(),
                  uint32_t timeoutMs = 0,
                  bool waitShutdown = false);
  bool Stop(int scriptId, bool wait = false);
  bool Stop(const std::string &scriptPath, bool wait = false);

  /*!
   *\brief Stop all running scripts
   *\param wait if kodi should wait for each script to finish (default false)
  */
  void StopRunningScripts(bool wait = false);

  bool IsRunning(int scriptId) const;
  bool IsRunning(const std::string& scriptPath) const;

protected:
  friend class CLanguageInvokerThread;

  void OnExecutionDone(int scriptId);

private:
  CScriptInvocationManager() = default;
  CScriptInvocationManager(const CScriptInvocationManager&) = delete;
  CScriptInvocationManager const& operator=(CScriptInvocationManager const&) = delete;
  virtual ~CScriptInvocationManager();

  struct LanguageInvokerThread
  {
    std::shared_ptr<CLanguageInvokerThread> thread;
    std::string script;
    bool done;
  };
  using LanguageInvokerThreadMap = std::map<int, LanguageInvokerThread>;
  using LanguageInvocationHandlerMap = std::map<std::string, ILanguageInvocationHandler*>;

  LanguageInvokerThread getInvokerThread(int scriptId) const;
  void ReleaseLastInvokerIfIdle();
  //! True if the last reusable thread is claimed, has a script queued, or is
  //! running one. Caller must hold m_critSection.
  bool LastInvokerOccupied() const;

  LanguageInvocationHandlerMap m_invocationHandlers;
  LanguageInvokerThreadMap m_scripts;
  std::shared_ptr<CLanguageInvokerThread> m_lastInvokerThread;
  int m_lastPluginHandle = -1;

  std::map<std::string, int> m_scriptPaths;
  int m_nextId = 0;
  mutable CCriticalSection m_critSection;
};
