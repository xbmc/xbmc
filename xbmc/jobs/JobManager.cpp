/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JobManager.h"

#include "jobs/IJobCallback.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

bool CJob::ShouldCancel(unsigned int progress, unsigned int total) const
{
  if (m_progressCallback)
    return m_progressCallback->OnJobProgress(progress, total, this);
  return false;
}

size_t CJob::GetPendingCallbackCount() const
{
  if (m_progressCallback)
    return m_progressCallback->GetPendingCallbackCount(this);
  return 0;
}

class CJobManager::CJobWorker : private CThread
{
public:
  explicit CJobWorker(CJobManager& manager) : CThread("JobWorker"), m_jobManager(manager)
  {
    Create(true); // start work immediately, and kill ourselves when we're done
  }

  ~CJobWorker() override
  {
    m_jobManager.RemoveWorker(this);
    if (!IsAutoDelete())
      StopThread();
  }

  void Process() override
  {
    SetPriority(ThreadPriority::LOWEST);
    while (true)
    {
      // request an item from our manager (this call is blocking)
      CJob* job{m_jobManager.GetNextJob()};
      if (!job)
        break;

      bool success{false};
      try
      {
        success = job->DoWork();
      }
      catch (...)
      {
        CLog::LogF(LOGERROR, "Error processing job {}", job->GetType());
      }
      m_jobManager.OnJobComplete(success, job);
    }
  }

private:
  CJobManager& m_jobManager;
};

struct CJobManager::JobFinder
{
  explicit JobFinder(const CJob* job) : m_job(job) {}
  bool operator()(const CWorkItem& workItem) const { return workItem.GetJob() == m_job; }

  const CJob* m_job{nullptr};
};

bool CJobManager::IsRunning() const
{
  std::unique_lock lock(m_section);
  return m_running;
}

void CJobManager::Restart()
{
  std::unique_lock lock(m_section);

  if (m_running)
    throw std::logic_error("CJobManager already running");
  m_running = true;
}

void CJobManager::CancelJobs()
{
  std::unique_lock lock(m_section);
  m_running = false;

  // clear any pending jobs
  for (unsigned int priority = CJob::PRIORITY_LOW_PAUSABLE; priority <= CJob::PRIORITY_DEDICATED;
       ++priority)
  {
    std::ranges::for_each(m_jobQueue[priority],
                          [](CWorkItem& wi)
                          {
                            for (auto* callback : wi.GetCallbacks())
                              callback->OnJobAbort(wi.GetId(), wi.GetJob());
                            wi.FreeJob();
                          });
    m_jobQueue[priority].clear();
  }

  // cancel any callbacks on jobs still processing
  std::ranges::for_each(m_processing,
                        [](CWorkItem& wi)
                        {
                          for (auto* callback : wi.GetCallbacks())
                            callback->OnJobAbort(wi.GetId(), wi.GetJob());
                          wi.Cancel();
                        });

  // tell our workers to finish
  while (!m_workers.empty())
  {
    lock.unlock();
    m_jobEvent.Set();
    std::this_thread::yield(); // yield after setting the event to give the workers some time to die
    lock.lock();
  }
}

unsigned int CJobManager::AddJob(CJob* job, IJobCallback* callback, CJob::PRIORITY priority)
{
  std::unique_lock lock(m_section);

  if (!m_running)
  {
    delete job;
    return 0;
  }

  // Check if we have this job already in the queue - if so, add callback to existing job
  auto it = std::ranges::find_if(m_jobQueue[priority],
                                 [job](const CWorkItem& wi) { return wi.GetJob()->Equals(job); });
  if (it != m_jobQueue[priority].end())
  {
    it->AddCallback(callback);
    delete job;
    return it->GetId();
  }

  // Check if an equal job is already processing - if so, add callback to it.
  // Note: Jobs that have moved to completion phase (removed from m_processing)
  // won't be found here, causing a new job to be created. This is intentional -
  // the completing job's results are about to be delivered to existing callbacks.
  auto procIt = std::ranges::find_if(m_processing, [job](const CWorkItem& wi)
                                     { return wi.GetJob()->Equals(job); });
  if (procIt != m_processing.end())
  {
    procIt->AddCallback(callback);
    delete job;
    return procIt->GetId();
  }

  // increment the job counter, ensuring 0 (invalid job) is never hit
  m_jobCounter++;
  if (m_jobCounter == 0)
    m_jobCounter++;

  // create a work item for this job
  CWorkItem work(job, m_jobCounter, priority, callback);
  m_jobQueue[priority].emplace_back(work);

  StartWorkers(priority);
  return work.GetId();
}

void CJobManager::CancelJob(unsigned int jobID)
{
  std::unique_lock lock(m_section);

  // check whether we have this job in the queue
  for (unsigned int priority = CJob::PRIORITY_LOW_PAUSABLE; priority <= CJob::PRIORITY_DEDICATED;
       ++priority)
  {
    const auto i = std::ranges::find_if(m_jobQueue[priority],
                                        [jobID](const auto& wi) { return wi.GetId() == jobID; });
    if (i != m_jobQueue[priority].cend())
    {
      CWorkItem item(std::move(*i));
      m_jobQueue[priority].erase(i);
      item.FreeJob();
      return;
    }
  }
  // or if we're processing it
  const auto it =
      std::ranges::find_if(m_processing, [jobID](const auto& wi) { return wi.GetId() == jobID; });
  if (it != m_processing.cend())
    it->Cancel(); // job is in progress, so only thing to do is to remove all callbacks
}

void CJobManager::StartWorkers(CJob::PRIORITY priority)
{
  std::unique_lock lock(m_section);

  // check how many free threads we have
  if (m_processing.size() >= GetMaxWorkers(priority))
    return;

  // do we have any sleeping threads?
  if (m_processing.size() < m_workers.size())
  {
    m_jobEvent.Set();
    return;
  }

  // everyone is busy - we need more workers
  m_workers.emplace_back(new CJobWorker(*this));
}

CJob* CJobManager::PopJob()
{
  std::unique_lock lock(m_section);
  for (int priority = CJob::PRIORITY_DEDICATED; priority >= CJob::PRIORITY_LOW_PAUSABLE; --priority)
  {
    // Check whether we're pausing pausable jobs
    if (priority == CJob::PRIORITY_LOW_PAUSABLE && m_pauseJobs)
      continue;

    if (!m_jobQueue[priority].empty() &&
        m_processing.size() < GetMaxWorkers(CJob::PRIORITY(priority)))
    {
      // pop the job off the queue
      const CWorkItem job{m_jobQueue[priority].front()};
      m_jobQueue[priority].pop_front();

      // add to the processing vector
      m_processing.emplace_back(job);
      job.GetJob()->SetProgressCallback(this);
      return job.GetJob();
    }
  }
  return nullptr;
}

void CJobManager::PauseJobs()
{
  std::unique_lock lock(m_section);
  m_pauseJobs = true;
}

void CJobManager::UnPauseJobs()
{
  std::unique_lock lock(m_section);
  m_pauseJobs = false;
}

bool CJobManager::IsProcessing(const CJob::PRIORITY& priority) const
{
  std::unique_lock lock(m_section);

  if (m_pauseJobs && priority == CJob::PRIORITY::PRIORITY_LOW_PAUSABLE)
    return false;

  return std::ranges::any_of(m_processing,
                             [priority](const auto& wi) { return wi.GetPriority() == priority; });
}

int CJobManager::IsProcessing(const std::string& type) const
{
  std::unique_lock lock(m_section);

  return static_cast<int>(std::ranges::count_if(
      m_processing,
      [this, &type](const auto& wi)
      {
        return (!m_pauseJobs || wi.GetPriority() != CJob::PRIORITY::PRIORITY_LOW_PAUSABLE) &&
               (std::string(wi.GetJob()->GetType()) == type);
      }));
}

CJob* CJobManager::GetNextJob()
{
  std::unique_lock lock(m_section);
  while (m_running)
  {
    // grab a job off the queue if we have one
    CJob* job = PopJob();
    if (job)
      return job;
    // no jobs are left - sleep for 30 seconds to allow new jobs to come in
    lock.unlock();
    bool newJob = m_jobEvent.Wait(30000ms);
    lock.lock();
    if (!newJob)
      break;
  }
  // ensure no jobs have come in during the period after
  // timeout and before we held the lock
  return PopJob();
}

bool CJobManager::OnJobProgress(unsigned int progress, unsigned int total, const CJob* job) const
{
  std::unique_lock lock(m_section);
  // find the job in the processing queue, and check whether it's cancelled (no callbacks)
  const auto i = std::ranges::find_if(m_processing, JobFinder(job));
  if (i != m_processing.cend())
  {
    CWorkItem item(*i);
    lock.unlock(); // leave section prior to call
    if (!item.GetCallbacks().empty())
    {
      for (auto* callback : item.GetCallbacks())
        callback->OnJobProgress(item.GetId(), progress, total, job);
      return false;
    }
  }
  return true; // couldn't find the job, or it's been cancelled
}

void CJobManager::OnJobComplete(bool success, CJob* job)
{
  std::unique_lock lock(m_section);
  // find the job in the processing queue
  auto i = std::ranges::find_if(m_processing, JobFinder(job));
  if (i != m_processing.end())
  {
    // Move work item out of m_processing to avoid iterator invalidation
    // when another thread modifies m_processing during callback execution
    CWorkItem item(std::move(*i));
    m_processing.erase(i);

    // Handle cancelled jobs (no callbacks remaining)
    if (item.GetCallbacks().empty())
    {
      item.FreeJob();
      return;
    }

    // Track pending callbacks so CJob::IsShared() can query the count.
    // Last callback (count==1) doesn't need to copy since it's the sole owner.
    m_pendingCallbacks[job] = item.GetCallbacks().size();

    while (IJobCallback* callback = item.PopCallback())
    {
      lock.unlock();
      try
      {
        callback->OnJobComplete(item.GetId(), success, job);
      }
      catch (...)
      {
        CLog::LogF(LOGERROR, "Error processing job {}", job->GetType());
      }
      lock.lock();
      // Update pending count for next callback
      m_pendingCallbacks[job] = item.GetCallbacks().size();
    }

    m_pendingCallbacks.erase(job);
    item.FreeJob();
  }
}

size_t CJobManager::GetPendingCallbackCount(const CJob* job) const
{
  std::unique_lock lock(m_section);
  auto it = m_pendingCallbacks.find(job);
  return it != m_pendingCallbacks.end() ? it->second : 0;
}

void CJobManager::RemoveWorker(const CJobWorker* worker)
{
  std::unique_lock lock(m_section);
  // remove our worker
  const auto i = std::ranges::find(m_workers, worker);
  if (i != m_workers.cend())
    m_workers.erase(i); // workers auto-delete
}

unsigned int CJobManager::GetMaxWorkers(CJob::PRIORITY priority)
{
  static const unsigned int max_workers = 5;
  if (priority == CJob::PRIORITY_DEDICATED)
    return 10000; // A large number..
  return max_workers - (CJob::PRIORITY_HIGH - priority);
}
