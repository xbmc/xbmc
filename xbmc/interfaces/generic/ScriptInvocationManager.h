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
#include <vector>

class CLanguageInvokerThread;
typedef std::shared_ptr<CLanguageInvokerThread> CLanguageInvokerThreadPtr;

class CScriptInvocationManager
{
public:
  static CScriptInvocationManager& GetInstance();

  void Process();
  void Uninitialize();

  void RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::string &extension);
  void RegisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler, const std::set<std::string> &extensions);
  void UnregisterLanguageInvocationHandler(ILanguageInvocationHandler *invocationHandler);
  bool HasLanguageInvoker(const std::string &script) const;
  std::shared_ptr<ILanguageInvoker> GetLanguageInvoker(const std::string& script);

  /*!
  * \brief Returns addon_handle if last reusable invoker is ready to use.
  */
  int GetReusablePluginHandle(const std::string& script);

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
                   bool reuseable = false);

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

  int m_nextId = 0;
  mutable CCriticalSection m_critSection;
};
