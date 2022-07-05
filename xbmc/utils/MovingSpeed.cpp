/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MovingSpeed.h"

#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"

void UTILS::MOVING_SPEED::CMovingSpeed::AddEventConfig(uint32_t eventId,
                                                       float acceleration,
                                                       float maxVelocity,
                                                       uint32_t resetTimeout)
{
  EventCfg eventCfg{acceleration, maxVelocity, resetTimeout};
  m_eventsData.emplace(eventId, EventData{eventCfg});
}

void UTILS::MOVING_SPEED::CMovingSpeed::AddEventConfig(uint32_t eventId, EventCfg event)
{
  m_eventsData.emplace(eventId, EventData{event});
}

void UTILS::MOVING_SPEED::CMovingSpeed::AddEventMapConfig(MapEventConfig& configs)
{
  for (auto& cfg : configs)
  {
    AddEventConfig(static_cast<uint32_t>(cfg.first), cfg.second);
  }
}

void UTILS::MOVING_SPEED::CMovingSpeed::Reset()
{
  m_currentEventId = 0;
  for (auto& eventPair : m_eventsData)
  {
    Reset(eventPair.first);
  }
}

void UTILS::MOVING_SPEED::CMovingSpeed::Reset(uint32_t eventId)
{
  auto mapIt = m_eventsData.find(eventId);
  if (mapIt == m_eventsData.end())
  {
    CLog::LogF(LOGWARNING, "Cannot reset Event ID {} configuration", eventId);
  }
  else
  {
    EventData& event = mapIt->second;
    event.m_currentVelocity = 1.0f;
    event.m_lastFrameTime = 0;
  }
}

float UTILS::MOVING_SPEED::CMovingSpeed::GetUpdatedDistance(uint32_t eventId)
{
  auto mapEventIt = m_eventsData.find(eventId);

  if (mapEventIt == m_eventsData.end())
  {
    CLog::LogF(LOGDEBUG, "No event set for event ID {}", eventId);
    return 0;
  }

  EventData& eventData = mapEventIt->second;
  EventCfg& eventCfg = eventData.m_config;

  uint32_t currentFrameTime{CTimeUtils::GetFrameTime()};
  uint32_t deltaFrameTime{currentFrameTime - eventData.m_lastFrameTime};
  float distance = (eventCfg.m_delta == 0.0f) ? 1.0f : eventCfg.m_delta;

  if (eventData.m_lastFrameTime != 0 && deltaFrameTime > eventCfg.m_resetTimeout)
  {
    // If the delta time exceed the timeout then reset values
    Reset(eventId);
  }
  else if (m_currentEventId != eventId)
  {
    // If the event id is changed then reset values
    Reset(eventId);
  }
  else if (eventData.m_lastFrameTime != 0)
  {
    // Calculate the new speed based on time so as not to depend on the frame rate
    eventData.m_currentVelocity +=
        eventCfg.m_acceleration * (static_cast<float>(deltaFrameTime) / 1000);

    if (eventCfg.m_maxVelocity > 0 && eventData.m_currentVelocity > eventCfg.m_maxVelocity)
      eventData.m_currentVelocity = eventCfg.m_maxVelocity;

    distance = eventData.m_currentVelocity * (static_cast<float>(deltaFrameTime) / 1000);
    if (eventCfg.m_delta > 0.0f)
      distance = MathUtils::RoundF(distance, eventCfg.m_delta);
  }

  m_currentEventId = eventId;
  eventData.m_lastFrameTime = currentFrameTime;
  return distance;
}

UTILS::MOVING_SPEED::EventType UTILS::MOVING_SPEED::ParseEventType(std::string_view eventType)
{
  if (eventType == "up")
    return EventType::UP;
  else if (eventType == "down")
    return EventType::DOWN;
  else if (eventType == "left")
    return EventType::LEFT;
  else if (eventType == "right")
    return EventType::RIGHT;
  else
  {
    CLog::LogF(LOGERROR, "Unsupported event type \"{}\"", eventType);
    return EventType::NONE;
  }
}
