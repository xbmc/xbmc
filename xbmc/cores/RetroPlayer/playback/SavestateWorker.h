/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace KODI::RETRO
{
// One reusable snapshot limits both memory consumption and outstanding saves.
template<typename Snapshot>
class CSavestateWorker
{
public:
  CSavestateWorker(std::unique_ptr<Snapshot> snapshot, std::function<void(Snapshot&)> commit)
    : m_available(std::move(snapshot)),
      m_commit(std::move(commit)),
      m_thread([this] { Process(); })
  {
  }

  ~CSavestateWorker()
  {
    Drain();
    {
      std::lock_guard lock(m_mutex);
      m_stop = true;
    }
    m_condition.notify_all();
    m_thread.join();
  }

  std::unique_ptr<Snapshot> TryAcquire()
  {
    std::unique_lock lock(m_mutex, std::try_to_lock);
    return lock.owns_lock() ? std::move(m_available) : nullptr;
  }

  std::unique_ptr<Snapshot> Acquire()
  {
    std::unique_lock lock(m_mutex);
    m_condition.wait(lock, [this] { return m_available != nullptr; });
    return std::move(m_available);
  }

  bool TrySubmit(std::unique_ptr<Snapshot>& snapshot)
  {
    std::unique_lock lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock())
      return false;
    m_pending = std::move(snapshot);
    lock.unlock();
    m_condition.notify_all();
    return true;
  }

  void Submit(std::unique_ptr<Snapshot>& snapshot)
  {
    {
      std::lock_guard lock(m_mutex);
      m_pending = std::move(snapshot);
    }
    m_condition.notify_all();
  }

  void Release(std::unique_ptr<Snapshot>& snapshot)
  {
    {
      std::lock_guard lock(m_mutex);
      m_available = std::move(snapshot);
    }
    m_condition.notify_all();
  }

  std::exception_ptr Drain()
  {
    std::unique_lock lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_pending && !m_processing; });
    return m_error;
  }

private:
  friend class CSavestateWorkerTestAccess;

  void Process()
  {
    std::unique_lock lock(m_mutex);
    while (true)
    {
      m_condition.wait(lock, [this] { return m_stop || m_pending != nullptr; });
      if (m_stop)
        return;
      auto snapshot = std::move(m_pending);
      m_processing = true;
      lock.unlock();
      std::exception_ptr error;
      try
      {
        m_commit(*snapshot);
      }
      catch (...)
      {
        error = std::current_exception();
      }
      lock.lock();
      m_error = error;
      m_available = std::move(snapshot);
      m_processing = false;
      m_condition.notify_all();
    }
  }

  std::mutex m_mutex;
  std::condition_variable m_condition;
  std::unique_ptr<Snapshot> m_available;
  std::unique_ptr<Snapshot> m_pending;
  bool m_processing{false};
  bool m_stop{false};
  std::exception_ptr m_error;
  const std::function<void(Snapshot&)> m_commit;
  std::thread m_thread;
};
} // namespace KODI::RETRO
