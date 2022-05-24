/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScriptRunner.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/generic/RunningScriptObserver.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <vector>

using namespace std::chrono_literals;

ADDON::AddonPtr CScriptRunner::GetAddon() const
{
  return m_addon;
}

CScriptRunner::CScriptRunner() : m_scriptDone(true)
{ }

bool CScriptRunner::StartScript(const ADDON::AddonPtr& addon, const std::string& path)
{
  return RunScriptInternal(addon, path, 0, false);
}

bool CScriptRunner::RunScript(const ADDON::AddonPtr& addon,
                              const std::string& path,
                              int handle,
                              bool resume)
{
  return RunScriptInternal(addon, path, handle, resume, true);
}

void CScriptRunner::SetDone()
{
  m_scriptDone.Set();
}

int CScriptRunner::ExecuteScript(const ADDON::AddonPtr& addon, const std::string& path, bool resume)
{
  return ExecuteScript(addon, path, -1, resume);
}

int CScriptRunner::ExecuteScript(const ADDON::AddonPtr& addon,
                                 const std::string& path,
                                 int handle,
                                 bool resume)
{
  if (addon == nullptr || path.empty())
    return false;

  CURL url(path);

  // get options and remove them from the URL because we can then use the url
  // to generate the base path which is passed to the add-on script
  auto options = url.GetOptions();
  url.SetOptions("");

  // setup our parameters to send the script
  std::vector<std::string> argv = {url.Get(), // base path
                                   StringUtils::Format("{:d}", handle), options,
                                   StringUtils::Format("resume:{}", resume)};

  bool reuseLanguageInvoker = false;
  const auto reuseLanguageInvokerIt = addon->ExtraInfo().find("reuselanguageinvoker");
  if (reuseLanguageInvokerIt != addon->ExtraInfo().end())
    reuseLanguageInvoker = reuseLanguageInvokerIt->second == "true";

  // run the script
  CLog::Log(LOGDEBUG, "CScriptRunner: running add-on script {:s}('{:s}', '{:s}', '{:s}')",
            addon->Name(), argv[0], argv[1], argv[2]);
  int scriptId = CScriptInvocationManager::GetInstance().ExecuteAsync(addon->LibPath(), addon, argv,
                                                                      reuseLanguageInvoker, handle);
  if (scriptId < 0)
    CLog::Log(LOGERROR, "CScriptRunner: unable to run add-on script {:s}", addon->Name());

  return scriptId;
}

bool CScriptRunner::RunScriptInternal(const ADDON::AddonPtr& addon,
                                      const std::string& path,
                                      int handle,
                                      bool resume,
                                      bool wait /* = true */)
{
  if (addon == nullptr || path.empty())
    return false;

  // reset our wait event
  m_scriptDone.Reset();

  // store the add-on
  m_addon = addon;

  int scriptId = ExecuteScript(addon, path, handle, resume);
  if (scriptId < 0)
    return false;

  // we don't need to wait for the script to end
  if (!wait)
    return true;

  // wait for our script to finish
  return WaitOnScriptResult(scriptId, addon->LibPath(), addon->Name());
}

bool CScriptRunner::WaitOnScriptResult(int scriptId,
                                       const std::string& path,
                                       const std::string& name)
{
  bool cancelled = false;

  // Add-on scripts can be called from the main and other threads. If called
  // form the main thread, we need to bring up the BusyDialog in order to
  // keep the render loop alive
  if (CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    if (!m_scriptDone.Wait(20ms))
    {
      // observe the script until it's finished while showing the busy dialog
      CRunningScriptObserver scriptObs(scriptId, m_scriptDone);

      auto& wm = CServiceBroker::GetGUI()->GetWindowManager();
      if (wm.IsModalDialogTopmost(WINDOW_DIALOG_PROGRESS))
      {
        auto progress = wm.GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
        if (!progress->WaitOnEvent(m_scriptDone))
          cancelled = true;
      }
      else if (!CGUIDialogBusy::WaitOnEvent(m_scriptDone, 200))
        cancelled = true;

      scriptObs.Abort();
    }
  }
  else
  {
    // wait for the script to finish or be cancelled
    while (!IsCancelled() && CScriptInvocationManager::GetInstance().IsRunning(scriptId) &&
           !m_scriptDone.Wait(20ms))
      ;

    // give the script 30 seconds to exit before we attempt to stop it
    XbmcThreads::EndTime<> timer(30s);
    while (!timer.IsTimePast() && CScriptInvocationManager::GetInstance().IsRunning(scriptId) &&
           !m_scriptDone.Wait(20ms))
      ;
  }

  if (cancelled || IsCancelled())
  {
    // cancel the script
    if (scriptId != -1 && CScriptInvocationManager::GetInstance().IsRunning(scriptId))
    {
      CLog::Log(LOGDEBUG, "CScriptRunner: cancelling add-on script {:s} (id = {:d})", name,
                scriptId);
      CScriptInvocationManager::GetInstance().Stop(scriptId);
    }
  }

  return !cancelled && !IsCancelled() && IsSuccessful();
}
