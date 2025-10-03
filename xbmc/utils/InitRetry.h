/*
 *  Copyright (C) 2010-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>

enum class InitComponentState
{
  Succeeded,
  Fallback,
  Failed
};

struct InitComponentStatus
{
  InitComponentState state{InitComponentState::Failed};
  std::string message;
  int attempts{0};
  int maxAttempts{0};
};

struct RetryExecutionResult
{
  bool succeeded{false};
  bool fallbackInvoked{false};
  bool fallbackSucceeded{false};
  int attemptsMade{0};
  int maxRetries{0};
};

class CInitStatusTracker
{
public:
  static void Record(const std::string& component,
                     InitComponentState state,
                     const std::string& message = "",
                     int attempts = 0,
                     int maxAttempts = 0);
  static std::map<std::string, InitComponentStatus> GetStatuses();

private:
  static std::mutex m_mutex;
  static std::map<std::string, InitComponentStatus> m_statuses;
};

class CInitRetry
{
public:
  using AttemptFunction = std::function<bool()>;
  using FallbackFunction = std::function<bool()>;

  static RetryExecutionResult Execute(const std::string& component,
                                      AttemptFunction attempt,
                                      FallbackFunction fallback,
                                      int maxRetries,
                                      std::chrono::milliseconds retryDelay);
};

class CDeferredRetryManager
{
public:
  using AttemptFunction = std::function<bool()>;
  using SuccessFunction = std::function<void()>;
  using FailureFunction = std::function<void()>;

  static CDeferredRetryManager& GetInstance();

  void Schedule(const std::string& component,
                AttemptFunction attempt,
                SuccessFunction onSuccess,
                FailureFunction onFailure,
                int maxRetries,
                std::chrono::milliseconds retryDelay,
                std::chrono::milliseconds initialDelay = std::chrono::milliseconds(0));

private:
  void Worker(std::string component,
              AttemptFunction attempt,
              SuccessFunction onSuccess,
              FailureFunction onFailure,
              int maxRetries,
              std::chrono::milliseconds retryDelay,
              std::chrono::milliseconds initialDelay);

  std::mutex m_schedulerMutex;
};
