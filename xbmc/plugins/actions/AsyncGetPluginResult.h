/*
*  Copyright (C) 2020 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/
#pragma once

#include "threads/Event.h"
#include "threads/IRunnable.h"

#include <atomic>

class CFileItem;

namespace XFILE
{
class CPluginDirectory;
}

namespace PLUGIN
{
/*!
 \brief AsyncGetPluginResult is an async action to resolve plugin paths (e.g. derive the
 dynpath playable path from the raw path of a plugin:// based fileitem). It is intended to be used
 by other components to drive the plugin execution on behalf of the main thread.
 It can also be executed syncronously on the callee thread
 (see \ref AsyncGetPluginResult_ExecuteAndWait "ExecuteAndWait") in cases where plugin resolution tasks
 are fundamental and this execution does not happens on the main thread (e.g. as part of a job, scrapper, etc).
 It is usefull since plugins are written in python and kodi has no control over the addon code or the time it
 takes to execute. Hence, plugin resolution tasks should not block the main thread nor the render loop.
 This action supports cancellation of the inherent script execution.
 \anchor AsyncGetPluginResult
 \sa CPluginExecutor "CPluginExecutor"
*/
class AsyncGetPluginResult : public IRunnable
{
public:
  /*!
  \brief No default constructor
  */
  AsyncGetPluginResult() = delete;

  /*!
  \brief Creates the plugin resolver action
  \param resultItem[in,out] The CFIleitem to resolve (path is a plugin://; dynpath will get filled)
  \param resume[in] if resume should be passed to the script that will be executed as an
  argument
  */
  AsyncGetPluginResult(CFileItem& item, const bool resume);

  /*!
  \brief Checks if the async execution had success i.e., if the plugin (and ihnerently the script)
  executed sucessfully and the action was not canceled in the meantime
  \anchor AsyncGetPluginResult_ExecutionSuceeded
  \return true if the execution was successfull and not cancelled
  */
  bool ExecutionSuceeded() const;
  
  /*!
  \brief Checks whether the action has been already canceled
  \return true if the action/plugin execution was canceled
  */
  bool IsCancelled() const;
  
  /*!
  \brief Checks whether the \ref AsyncGetPluginResult_ExecutionSuceeded "ExecutionSuceeded"
  and the CFileItem path is no longer a plugin:// (i.e. dynpath is properly filled)
  \return true if the action/plugin execution was successful and the item is no longer a plugin:// path
  */
  bool ResolutionSuceeded() const;

  /*!
  \brief Cancels the async action
  */
  void Cancel() override;

  /*!
  \brief Executes this async action syncronously, waiting for the result
  \anchor AsyncGetPluginResult_ExecuteAndWait
  \return true if the execution was successfull
  */
  bool ExecuteAndWait();

private:
  /*!
  \brief IRunnable implementation
  \anchor AsyncGetPluginResult_Run
  */
  void Run() override;

  /*!
  \brief Flags the async event, sets the success variables and updates the result item (called
  internally as part of Run)
  \sa AsyncGetPluginResult_Run "Run"
  */
  void Finish();

  // propagated for script execution
  CFileItem* m_item;
  bool m_resume = false;

  // part of future/async
  CEvent m_event;
  std::atomic<bool> m_success;
  std::atomic<bool> m_cancelled;

  // handler for CPluginDirectory static function calls
  XFILE::CPluginDirectory* m_pluginDirHandler;
};
} // namespace PLUGIN
