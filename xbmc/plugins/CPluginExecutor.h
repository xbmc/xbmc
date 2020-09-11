/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "messaging/IMessageTarget.h"
#include "plugins/actions/AsyncGetPluginResult.h"
#include "threads/CriticalSection.h"

namespace PLUGIN
{
/*!
 \brief CPluginExecutor is a service broker component designed to handle plugin resolution requests
 offloaded from the main thread via messages and pipe the respective result to the specified receiver.
 Plugin resolution tasks are launched as jobs, as soon as they are received.
 \anchor CPluginExecutor
 \sa AsyncGetPluginResult
*/
class CPluginExecutor : public KODI::MESSAGING::IMessageTarget
{
public:
  /*!
   \brief Default constructor
  */
  CPluginExecutor();
  
  /*!
   \brief Stops any \ref AsyncGetPluginResult "AsyncGetPluginResult" actions that may be executing on
   the background, clearing up the "queue".
  */
  ~CPluginExecutor();

  /*!
   \brief Registers the service as an application messenger target
  */
  void Initialize();
  
  /*!
   \brief Checks if the executor has any pending jobs on the background
   \return true if any AsyncGetPluginResult actions are executing on the background
  */
  bool IsBusy() const;

  /*!
   \brief ApplicationMessenger mask identifier
  */
  int GetMessageMask() override;
  
  /*!
   \brief Handler for the received ApplicationMessenger messages
  */
  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

private:
  /*!
   \brief Adds the AsyncGetPluginResult action to the queue of running actions/jobs
   \anchor CPluginExecutor_Add
   \param action the action to execute
  */
  void Add(AsyncGetPluginResult* action);
  
  /*!
   \brief Success callback
  */
  template<typename F>
  /*!
   \brief Adds the action to be executed to the queue (see \ref CPluginExecutor_Add "Add") and starts
   the execution right away on a different thread as a job
   \anchor CPluginExecutor_Add
   \param path the original plugin:// path of the item
   \param action the AsyncGetPluginResult action to execute
   \param sucessCallback the lambda function to execute when the plugin path resolution succeeds
  */
  void Submit(const std::string& path, AsyncGetPluginResult* action, F& sucessCallback);
  
  /*!
   \brief Cancels all AsyncGetPluginResult tasks that may be executing on the background
  */
  void StopAll() const;
  
  /*!
   \brief Always called after an AsyncGetPluginResult completes (either success or failure)
   to perform cleanup tasks (namely, the removal from the builtin container/queue of actions)
  */
  void OnExecutionEnded(AsyncGetPluginResult* action);
  
  // container of AsyncGetPluginResult actions being executed in the background
  std::vector<AsyncGetPluginResult*> m_actions;
  
  // lock for the the AsyncGetPluginResult container (m_actions)
  mutable CCriticalSection m_criticalsection;
  
};
} // namespace PLUGIN
