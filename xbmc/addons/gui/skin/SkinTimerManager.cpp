/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinTimerManager.h"

#include "GUIInfoManager.h"
#include "guilib/GUIAction.h"
#include "guilib/GUIComponent.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <chrono>

using namespace std::chrono_literals;

CSkinTimerManager::CSkinTimerManager(CGUIInfoManager& infoMgr) : m_infoMgr{infoMgr}
{
}

void CSkinTimerManager::LoadTimers(const std::string& path)
{
  CXBMCTinyXML2 doc;
  if (!doc.LoadFile(path))
  {
    CLog::LogF(LOGWARNING, "Could not load timers file {}: {} (Line: {})", path, doc.ErrorStr(),
               doc.ErrorLineNum());
    return;
  }

  auto* root = doc.RootElement();
  if (!root || !StringUtils::EqualsNoCase(root->Value(), "timers"))
  {
    CLog::LogF(LOGERROR, "Error loading timers file {}: Root element <timers> required.", path);
    return;
  }

  const auto* timerNode = root->FirstChildElement("timer");
  while (timerNode)
  {
    LoadTimerInternal(timerNode);
    timerNode = timerNode->NextSiblingElement("timer");
  }
}

void CSkinTimerManager::LoadTimerInternal(const tinyxml2::XMLNode* node)
{
  if ((!node->FirstChildElement("name") || !node->FirstChildElement("name")->FirstChild()))
  {
    CLog::LogF(LOGERROR, "Missing required field 'name' for valid skin timer. Ignoring timer.");
    return;
  }

  std::string timerName = node->FirstChildElement("name")->FirstChild()->Value();
  if (TimerExists(timerName))
  {
    CLog::LogF(LOGWARNING,
               "Ignoring timer with name {} - another timer with the same name already exists",
               timerName);
    return;
  }

  // timer start
  INFO::InfoPtr startInfo{nullptr};
  bool resetOnStart{false};
  if (node->FirstChildElement("start") && node->FirstChildElement("start")->FirstChild())
  {
    startInfo = m_infoMgr.Register(node->FirstChildElement("start")->FirstChild()->Value());
    // check if timer needs to be reset after start
    if (node->FirstChildElement("start")->Attribute("reset") &&
        StringUtils::EqualsNoCase(node->FirstChildElement("start")->Attribute("reset"), "true"))
    {
      resetOnStart = true;
    }
  }

  // timer reset
  INFO::InfoPtr resetInfo{nullptr};
  if (node->FirstChildElement("reset") && node->FirstChildElement("reset")->FirstChild())
  {
    resetInfo = m_infoMgr.Register(node->FirstChildElement("reset")->FirstChild()->Value());
  }
  // timer stop
  INFO::InfoPtr stopInfo{nullptr};
  if (node->FirstChildElement("stop") && node->FirstChildElement("stop")->FirstChild())
  {
    stopInfo = m_infoMgr.Register(node->FirstChildElement("stop")->FirstChild()->Value());
  }

  // process onstart actions
  CGUIAction startActions;
  startActions.EnableSendThreadMessageMode();
  const auto* onStartElement = node->FirstChildElement("onstart");
  while (onStartElement)
  {
    if (onStartElement->FirstChild())
    {
      const std::string conditionalActionAttribute =
          onStartElement->Attribute("condition") != nullptr ? onStartElement->Attribute("condition")
                                                            : "";
      startActions.Append(CGUIAction::CExecutableAction{conditionalActionAttribute,
                                                        onStartElement->FirstChild()->Value()});
    }
    onStartElement = onStartElement->NextSiblingElement("onstart");
  }

  // process onstop actions
  CGUIAction stopActions;
  stopActions.EnableSendThreadMessageMode();
  const auto* onStopElement = node->FirstChildElement("onstop");
  while (onStopElement)
  {
    if (onStopElement->FirstChild())
    {
      const std::string conditionalActionAttribute =
          onStopElement->Attribute("condition") != nullptr ? onStopElement->Attribute("condition")
                                                           : "";
      stopActions.Append(CGUIAction::CExecutableAction{conditionalActionAttribute,
                                                       onStopElement->FirstChild()->Value()});
    }
    onStopElement = onStopElement->NextSiblingElement("onstop");
  }

  m_timers[timerName] = std::make_unique<CSkinTimer>(CSkinTimer(
      timerName, startInfo, resetInfo, stopInfo, startActions, stopActions, resetOnStart));
}

bool CSkinTimerManager::TimerIsRunning(const std::string& timer) const
{
  if (auto iter = m_timers.find(timer); iter != m_timers.end())
  {
    return iter->second->IsRunning();
  }
  CLog::LogF(LOGERROR, "Couldn't find Skin Timer with name: {}", timer);
  return false;
}

float CSkinTimerManager::GetTimerElapsedSeconds(const std::string& timer) const
{
  if (auto iter = m_timers.find(timer); iter != m_timers.end())
  {
    return iter->second->GetElapsedSeconds();
  }
  CLog::LogF(LOGERROR, "Couldn't find Skin Timer with name: {}", timer);
  return 0;
}

void CSkinTimerManager::TimerStart(const std::string& timer) const
{
  if (auto iter = m_timers.find(timer); iter != m_timers.end())
  {
    return iter->second->Start();
  }
  CLog::LogF(LOGERROR, "Couldn't find Skin Timer with name: {}", timer);
}

void CSkinTimerManager::TimerStop(const std::string& timer) const
{
  if (auto iter = m_timers.find(timer); iter != m_timers.end())
  {
    return iter->second->Stop();
  }
  CLog::LogF(LOGERROR, "Couldn't find Skin Timer with name: {}", timer);
}

size_t CSkinTimerManager::GetTimerCount() const
{
  return m_timers.size();
}

bool CSkinTimerManager::TimerExists(const std::string& timer) const
{
  return m_timers.count(timer) != 0;
}

std::unique_ptr<CSkinTimer> CSkinTimerManager::GrabTimer(const std::string& timer)
{
  if (auto iter = m_timers.find(timer); iter != m_timers.end())
  {
    auto timerInstance = std::move(iter->second);
    m_timers.erase(iter);
    return timerInstance;
  }
  return {};
}

void CSkinTimerManager::Stop()
{
  // skintimers, as infomanager clients register info conditions/expressions in the infomanager.
  // The infomanager is linked to skins, being initialized or cleared when
  // skins are loaded (or unloaded). All the registered boolean conditions from
  // skin timers will end up being removed when the skin is unloaded. However, to
  // self-contain this component unregister them all here.
  for (auto const& [key, val] : m_timers)
  {
    const std::unique_ptr<CSkinTimer>::pointer timer = val.get();
    if (timer->GetStartCondition())
    {
      m_infoMgr.UnRegister(timer->GetStartCondition());
    }
    if (timer->GetStopCondition())
    {
      m_infoMgr.UnRegister(timer->GetStopCondition());
    }
    if (timer->GetResetCondition())
    {
      m_infoMgr.UnRegister(timer->GetResetCondition());
    }
  }
  m_timers.clear();
}

void CSkinTimerManager::Process()
{
  for (const auto& [key, val] : m_timers)
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
}
