/*
 *  Copyright (C) 2010-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InitRetry.h"

#include "utils/log.h"
#include "utils/StringUtils.h"

#include <thread>

namespace
{
constexpr const char* ToString(InitComponentState state)
{
  switch (state)
  {
  case InitComponentState::Succeeded:
      return "success";
    case InitComponentState::Fallback:
      return "fallback";
    case InitComponentState::Failed:
    default:
      return "failed";
  }
}
} // unnamed namespace

std::mutex CInitStatusTracker::m_mutex;
std::map<std::string, InitComponentStatus> CInitStatusTracker::m_statuses;

void CInitStatusTracker::Record(const std::string& component,
                                InitComponentState state,
                                const std::string& message,
                                int attempts,
                                int maxAttempts)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  std::string attemptInfo;
  if (attempts > 0 && maxAttempts > 0)
    attemptInfo = StringUtils::Format(" (attempt {}/{})", attempts, maxAttempts);

  CLog::Log(LOGDEBUG, "InitStatusTracker - component '{}' marked as {}{}{}", component,
            ToString(state), attemptInfo,
            message.empty() ? "" : (": " + message));
  m_statuses[component] = {state, message, attempts, maxAttempts};
}

std::map<std::string, InitComponentStatus> CInitStatusTracker::GetStatuses()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_statuses;
}

RetryExecutionResult CInitRetry::Execute(const std::string& component,
                                         AttemptFunction attempt,
                                         FallbackFunction fallback,
                                         int maxRetries,
                                         std::chrono::milliseconds retryDelay)
{
  RetryExecutionResult result;
  result.maxRetries = maxRetries;

  if (!attempt)
  {
    CLog::Log(LOGERROR, "InitRetry::Execute - invalid attempt callback for {}", component);
    return result;
  }

  if (!fallback)
  {
    CLog::Log(LOGWARNING, "InitRetry::Execute - no fallback provided for {}", component);
  }

  int attemptsMade = 0;
  for (int attemptIndex = 0; attemptIndex < maxRetries; ++attemptIndex)
  {
    ++attemptsMade;
    if (attemptIndex > 0)
    {
      CLog::Log(LOGWARNING,
                "InitRetry::Execute - {} initialization retry {}/{} after {} ms",
                component, attemptIndex + 1, maxRetries, retryDelay.count());
      std::this_thread::sleep_for(retryDelay);
    }

    if (attempt())
    {
      result.succeeded = true;
      result.attemptsMade = attemptsMade;
  CInitStatusTracker::Record(component, InitComponentState::Succeeded, "", attemptsMade,
                             maxRetries);
      return result;
    }
  }

  result.attemptsMade = attemptsMade;
  result.fallbackInvoked = (fallback != nullptr);
  if (fallback)
  {
    result.fallbackSucceeded = fallback();
    CInitStatusTracker::Record(
        component,
        result.fallbackSucceeded ? InitComponentState::Fallback : InitComponentState::Failed,
        result.fallbackSucceeded ? "fallback active" : "fallback failed", attemptsMade,
        maxRetries);
  }
  else
  {
    CInitStatusTracker::Record(component, InitComponentState::Failed, "", attemptsMade,
                               maxRetries);
  }

  return result;
}

CDeferredRetryManager& CDeferredRetryManager::GetInstance()
{
  static CDeferredRetryManager instance;
  return instance;
}

void CDeferredRetryManager::Schedule(const std::string& component,
                                     AttemptFunction attempt,
                                     SuccessFunction onSuccess,
                                     FailureFunction onFailure,
                                     int maxRetries,
                                     std::chrono::milliseconds retryDelay,
                                     std::chrono::milliseconds initialDelay)
{
  if (!attempt)
  {
    CLog::Log(LOGERROR, "DeferredRetryManager::Schedule - invalid attempt callback for {}",
              component);
    return;
  }

  std::lock_guard<std::mutex> lock(m_schedulerMutex);
  std::thread(&CDeferredRetryManager::Worker, this, component, std::move(attempt),
              std::move(onSuccess), std::move(onFailure), maxRetries, retryDelay, initialDelay)
      .detach();
}

void CDeferredRetryManager::Worker(std::string component,
                                   AttemptFunction attempt,
                                   SuccessFunction onSuccess,
                                   FailureFunction onFailure,
                                   int maxRetries,
                                   std::chrono::milliseconds retryDelay,
                                   std::chrono::milliseconds initialDelay)
{
  if (initialDelay.count() > 0)
    std::this_thread::sleep_for(initialDelay);

  for (int attemptIndex = 0; attemptIndex < maxRetries; ++attemptIndex)
  {
    if (attemptIndex > 0)
      std::this_thread::sleep_for(retryDelay);

    CLog::Log(LOGINFO, "DeferredRetryManager - attempting to recover component '{}' ({}/{})",
              component, attemptIndex + 1, maxRetries);

    if (attempt())
    {
      CLog::Log(LOGINFO, "DeferredRetryManager - component '{}' recovered successfully",
                component);
  CInitStatusTracker::Record(component, InitComponentState::Succeeded, "recovered after retry",
                             attemptIndex + 1, maxRetries);
      if (onSuccess)
        onSuccess();
      return;
    }
  }

  CLog::Log(LOGERROR, "DeferredRetryManager - component '{}' failed to recover after {} attempts",
            component, maxRetries);
  CInitStatusTracker::Record(component, InitComponentState::Failed,
                             "deferred retry exhausted", maxRetries, maxRetries);
  if (onFailure)
    onFailure();
}
