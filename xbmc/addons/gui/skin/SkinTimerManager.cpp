/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinTimerManager.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "interfaces/builtins/Builtins.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <chrono>
#include <mutex>

using namespace std::chrono_literals;

CSkinTimerManager::CSkinTimerManager() : CThread("SkinTimers")
{
}

void CSkinTimerManager::Start()
{
  std::unique_lock<CCriticalSection> lock(m_timerCriticalSection);
  if (!m_timers.empty())
  {
    Create();
  }
}

void CSkinTimerManager::LoadTimers(const std::string& path)
{
  CXBMCTinyXML doc;
  if (!doc.LoadFile(path))
  {
    CLog::LogF(LOGWARNING, "Could not load timers file {}: {} (row: {}, col: {})", path,
               doc.ErrorDesc(), doc.ErrorRow(), doc.ErrorCol());
    return;
  }

  TiXmlElement* root = doc.RootElement();
  if (!root || !StringUtils::EqualsNoCase(root->Value(), "timers"))
  {
    CLog::LogF(LOGERROR, "Error loading timers file {}: Root element <timers> required.", path);
    return;
  }

  const TiXmlElement* timerNode = root->FirstChildElement("timer");
  std::unique_lock<CCriticalSection> lock(m_timerCriticalSection);
  while (timerNode)
  {
    LoadTimerInternal(timerNode);
    timerNode = timerNode->NextSiblingElement("timer");
  }
}

void CSkinTimerManager::LoadTimerInternal(const TiXmlElement* node)
{
  if ((!node->FirstChild("name") || !node->FirstChild("name")->FirstChild() ||
       node->FirstChild("name")->FirstChild()->ValueStr().empty()))
  {
    CLog::LogF(LOGERROR, "Missing required field name for valid skin. Ignoring timer.");
    return;
  }

  INFO::InfoPtr startInfo{nullptr};
  INFO::InfoPtr resetInfo{nullptr};
  INFO::InfoPtr stopInfo{nullptr};
  std::string timerName = node->FirstChild("name")->FirstChild()->Value();
  std::string startAction;
  std::string stopAction;
  bool resetOnStart{false};

  if (m_timers.count(timerName) > 0)
  {
    CLog::LogF(LOGWARNING,
               "Ignoring timer with name {} - another timer with the same name already exists",
               timerName);
    return;
  }

  if (node->FirstChild("start") && node->FirstChild("start")->FirstChild() &&
      !node->FirstChild("start")->FirstChild()->ValueStr().empty())
  {
    startInfo = CServiceBroker::GetGUI()->GetInfoManager().Register(
        node->FirstChild("start")->FirstChild()->ValueStr());
    // check if timer needs to be reset after start
    if (node->Attribute("reset") && StringUtils::EqualsNoCase(node->Attribute("reset"), "true"))
    {
      resetOnStart = true;
    }
  }

  if (node->FirstChild("reset") && node->FirstChild("reset")->FirstChild() &&
      !node->FirstChild("reset")->FirstChild()->ValueStr().empty())
  {
    resetInfo = CServiceBroker::GetGUI()->GetInfoManager().Register(
        node->FirstChild("reset")->FirstChild()->ValueStr());
  }

  if (node->FirstChild("stop") && node->FirstChild("stop")->FirstChild() &&
      !node->FirstChild("stop")->FirstChild()->ValueStr().empty())
  {
    stopInfo = CServiceBroker::GetGUI()->GetInfoManager().Register(
        node->FirstChild("stop")->FirstChild()->ValueStr());
  }

  if (node->FirstChild("onstart") && node->FirstChild("onstart")->FirstChild() &&
      !node->FirstChild("onstart")->FirstChild()->ValueStr().empty())
  {
    if (!CBuiltins::GetInstance().HasCommand(node->FirstChild("onstart")->FirstChild()->ValueStr()))
    {
      CLog::LogF(LOGERROR,
                 "Unknown onstart builtin action {} for timer {}, the action will be ignored",
                 node->FirstChild("onstart")->FirstChild()->ValueStr(), timerName);
    }
    else
    {
      startAction = node->FirstChild("onstart")->FirstChild()->ValueStr();
    }
  }

  if (node->FirstChild("onstop") && node->FirstChild("onstop")->FirstChild() &&
      !node->FirstChild("onstop")->FirstChild()->ValueStr().empty())
  {
    if (!CBuiltins::GetInstance().HasCommand(node->FirstChild("onstop")->FirstChild()->ValueStr()))
    {
      CLog::LogF(LOGERROR,
                 "Unknown onstop builtin action {} for timer {}, the action will be ignored",
                 node->FirstChild("onstop")->FirstChild()->ValueStr(), timerName);
    }
    else
    {
      stopAction = node->FirstChild("onstop")->FirstChild()->ValueStr();
    }
  }

  m_timers[timerName] = std::make_unique<CSkinTimer>(
      CSkinTimer(timerName, startInfo, resetInfo, stopInfo, startAction, stopAction, resetOnStart));
}

bool CSkinTimerManager::TimerIsRunning(const std::string& timer) const
{
  std::unique_lock<CCriticalSection> lock(m_timerCriticalSection);
  if (m_timers.count(timer) == 0)
  {
    CLog::LogF(LOGERROR, "Couldn't find Skin Timer with name: {}", timer);
    return false;
  }
  return m_timers.at(timer)->IsRunning();
}

float CSkinTimerManager::GetTimerElapsedSeconds(const std::string& timer) const
{
  std::unique_lock<CCriticalSection> lock(m_timerCriticalSection);
  if (m_timers.count(timer) == 0)
  {
    CLog::LogF(LOGERROR, "Couldn't find Skin Timer with name: {}", timer);
    return 0;
  }
  return m_timers.at(timer)->GetElapsedSeconds();
}

void CSkinTimerManager::TimerStart(const std::string& timer) const
{
  std::unique_lock<CCriticalSection> lock(m_timerCriticalSection);
  if (m_timers.count(timer) == 0)
  {
    CLog::LogF(LOGERROR, "Couldn't find Skin Timer with name: {}", timer);
    return;
  }
  m_timers.at(timer)->Start();
}

void CSkinTimerManager::TimerStop(const std::string& timer) const
{
  std::unique_lock<CCriticalSection> lock(m_timerCriticalSection);
  if (m_timers.count(timer) == 0)
  {
    CLog::LogF(LOGERROR, "Couldn't find Skin Timer with name: {}", timer);
    return;
  }
  m_timers.at(timer)->Stop();
}

void CSkinTimerManager::Stop()
{
  StopThread();

  // skintimers, as infomanager clients register info conditions/expressions in the infomanager.
  // The infomanager is linked to skins, being initialized or cleared when
  // skins are loaded (or unloaded). All the registered boolean conditions from
  // skin timers will end up being removed when the skin is unloaded. However, to
  // self-contain this component unregister them all here.
  std::unique_lock<CCriticalSection> lock(m_timerCriticalSection);
  for (auto const& [key, val] : m_timers)
  {
    const std::unique_ptr<CSkinTimer>::pointer timer = val.get();
    if (timer->GetStartCondition())
    {
      CServiceBroker::GetGUI()->GetInfoManager().UnRegister(timer->GetStartCondition());
    }
    if (timer->GetStopCondition())
    {
      CServiceBroker::GetGUI()->GetInfoManager().UnRegister(timer->GetStopCondition());
    }
    if (timer->GetResetCondition())
    {
      CServiceBroker::GetGUI()->GetInfoManager().UnRegister(timer->GetResetCondition());
    }
  }
  m_timers.clear();
}

void CSkinTimerManager::StopThread(bool bWait /*= true*/)
{
  std::unique_lock<CCriticalSection> lock(m_timerCriticalSection);
  m_bStop = true;
  CThread::StopThread(bWait);
}

void CSkinTimerManager::Process()
{
  while (!m_bStop)
  {
    for (auto const& [key, val] : m_timers)
    {
      const std::unique_ptr<CSkinTimer>::pointer timer = val.get();
      if (!timer->IsRunning() && timer->VerifyStartCondition())
      {
        timer->Start();
      }
      else if (timer->IsRunning() && timer->VerifyStopCondition())
      {
        timer->Stop();
      }
      if (timer->GetElapsedSeconds() > 0 && timer->VerifyResetCondition())
      {
        timer->Reset();
      }
    }
    Sleep(500ms);
  }
}
