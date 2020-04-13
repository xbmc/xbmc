/*
*  Copyright (C) 2020 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "AsyncGetPluginResult.h"
#include "FileItem.h"
#include "filesystem/PluginDirectory.h"
#include "utils/URIUtils.h"


PLUGIN::AsyncGetPluginResult::AsyncGetPluginResult(CFileItem& item,
                                                       const bool resume)
  : m_item(&item), m_resume(resume)
{
  m_event.Reset();
  m_cancelled = false;
  m_pluginDirHandler = new XFILE::CPluginDirectory;
}

bool PLUGIN::AsyncGetPluginResult::ExecutionSuceeded() const
{
  return m_success && !m_cancelled;
}

bool PLUGIN::AsyncGetPluginResult::ResolutionSuceeded() const
{
  return m_success && !URIUtils::IsPlugin(m_item->GetDynPath());
}

void PLUGIN::AsyncGetPluginResult::Cancel()
{
  m_cancelled = true;
}

bool PLUGIN::AsyncGetPluginResult::IsCancelled() const
{
  return m_cancelled;
}

bool PLUGIN::AsyncGetPluginResult::ExecuteAndWait()
{
  Run();
  return ExecutionSuceeded();
}

void PLUGIN::AsyncGetPluginResult::Run()
{
  const auto& scriptExecutionInfo = m_pluginDirHandler->TriggerScriptExecution(m_item->GetPath(), m_resume);

  // if the script executes quick enough there's no need to launch the observer
  if (!m_pluginDirHandler->m_fetchComplete.WaitMSec(20))
  {
    // launch an observer for the running script
    XFILE::CPluginDirectory::CScriptObserver scriptObs(scriptExecutionInfo.Id, m_event);

    // let the script run until cancel is flagged or the execution ends
    while (!m_cancelled && !m_event.Signaled() && !m_pluginDirHandler->m_fetchComplete.WaitMSec(20))
      ;

    // Force stop the running script in case it was manually cancelled
    if (m_cancelled)
      m_pluginDirHandler->ForceStopRunningScript(scriptExecutionInfo);

    // Finish and abort the observer
    Finish();
    scriptObs.Abort();
  }
  else
  {
    Finish();
  }
}

void PLUGIN::AsyncGetPluginResult::Finish()
{
  // set the action sucess
  m_success = m_pluginDirHandler->m_success && !m_cancelled;

  // update result item
  if (m_success)
    m_pluginDirHandler->UpdateResultItem(*m_item, m_pluginDirHandler->m_fileResult);

  // flag the action event
  m_event.Set();
  
}
