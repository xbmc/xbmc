/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPluginExecutor.h"
#include "FileItem.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"
#include "utils/JobManager.h"
#include "utils/URIUtils.h"

namespace PLUGIN
{
static const unsigned int maxResolutionAttempts = 4;
}

using namespace KODI::MESSAGING;
using namespace PLUGIN;

CPluginExecutor::CPluginExecutor() = default;

CPluginExecutor::~CPluginExecutor()
{
  StopAll();
}

void CPluginExecutor::Initialize()
{
  CApplicationMessenger::GetInstance().RegisterReceiver(this);
}

void CPluginExecutor::StopAll() const
{
  CSingleLock lock(m_criticalsection);
  for (auto action: m_actions)
    action->Cancel();
}

bool CPluginExecutor::IsBusy() const
{
  CSingleLock lock(m_criticalsection);
  return m_actions.size() > 0;
}

template<typename F>
void CPluginExecutor::Submit(const std::string& path, AsyncGetPluginResult* action, F& sucessCallback)
{
  Add(action);

  CJobManager::GetInstance().Submit([this, path, action, sucessCallback]() {
    int i = 0;
    CLog::Log(LOGDEBUG, "Starting resolving plugin with path %s", path);
    do {
      if (action->ExecuteAndWait())
      {
        if (action->ResolutionSuceeded())
        {
          sucessCallback();
          break;
        }
      }
      else if (action->IsCancelled())
      {
        CLog::Log(LOGDEBUG, "Plugin resolution with path %s has been canceled", path);
        break;
      }
      i++;
    }
    while (!action->IsCancelled() && i <= maxResolutionAttempts);
    
    if (!action->IsCancelled() && !action->ResolutionSuceeded())
      CLog::Log(LOGERROR, "Plugin item with path (%s) failed to resolve after %i resolution attempts",
      path, maxResolutionAttempts);

    this->OnExecutionEnded(action);
  });
}

void CPluginExecutor::Add(AsyncGetPluginResult* action)
{
  CSingleLock lock(m_criticalsection);
  m_actions.emplace_back(action);
}

void CPluginExecutor::OnExecutionEnded(AsyncGetPluginResult* action)
{
  CSingleLock lock(m_criticalsection);
  m_actions.erase(std::remove(m_actions.begin(), m_actions.end(), action), m_actions.end());
}

int CPluginExecutor::GetMessageMask()
{
  return TMSG_MASK_PLUGINEXECUTOR;
}

void CPluginExecutor::OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_PLUGIN_EXECUTOR_GET_RESULT:
  {
    if (pMsg->lpVoid)
    {
      CFileItem* item = static_cast<CFileItem*>(pMsg->lpVoid);
      if (!URIUtils::IsPlugin(item->GetDynPath()))
        return;

      const auto player = pMsg->strParam;
      const bool resume = static_cast<bool>(pMsg->param2);
      const auto destination = pMsg->param1;

      const std::string path = item->GetPath();
      auto action = new AsyncGetPluginResult(*item, resume);
      auto callback = [destination, item, player]() {
        if (!URIUtils::IsPlugin(item->GetDynPath()))
        {
          CApplicationMessenger::GetInstance().PostMsg(destination, 0, 0,
          static_cast<void*>(item), player);
        }
      };
      Submit(path, action, callback);
    }
    break;
  }
  case TMSG_PLUGIN_EXECUTOR_STOP_ALL:
  {
    StopAll();
    break;
  }

  default:
    break;
  }
}
