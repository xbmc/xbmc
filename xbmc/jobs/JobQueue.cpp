/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JobQueue.h"

#include "ServiceBroker.h"
#include "jobs/JobManager.h"

#include <algorithm>
#include <mutex>

void CJobQueue::CJobPointer::CancelJob()
{
  CServiceBroker::GetJobManager()->CancelJob(m_id);
  m_id = 0;
}

struct CJobQueue::JobFinder
{
  explicit JobFinder(const CJob* job) : m_job(job) {}
  bool operator()(const CJobPointer& jobPtr) const { return jobPtr.GetJob() == m_job; }

  const CJob* m_job{nullptr};
};

CJobQueue::CJobQueue(bool lifo, unsigned int jobsAtOnce, CJob::PRIORITY priority)
  : m_jobsAtOnce(jobsAtOnce),
    m_priority(priority),
    m_lifo(lifo)
{
}

CJobQueue::~CJobQueue()
{
  CancelJobs();
}

void CJobQueue::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  OnJobNotify(job);
}

void CJobQueue::OnJobAbort(unsigned int jobID, CJob* job)
{
  OnJobNotify(job);
}

void CJobQueue::CancelJob(const CJob* job)
{
  std::unique_lock lock(m_section);
  const auto i = std::ranges::find_if(m_processing, JobFinder(job));
  if (i != m_processing.cend())
  {
    i->CancelJob();
    m_processing.erase(i);
    return;
  }
  const auto j = std::ranges::find_if(m_jobQueue, JobFinder(job));
  if (j != m_jobQueue.cend())
  {
    j->FreeJob();
    m_jobQueue.erase(j);
  }
}

bool CJobQueue::AddJob(CJob* job)
{
  const auto jobMatcher = [job](const CJobPointer& jobPtr) { return jobPtr.GetJob()->Equals(job); };

  std::unique_lock lock(m_section);
  // check if we have this job already.  If so, we're done.
  if (std::ranges::find_if(m_jobQueue, jobMatcher) != m_jobQueue.cend() ||
      std::ranges::find_if(m_processing, jobMatcher) != m_processing.cend())
  {
    delete job;
    return false;
  }

  if (m_lifo)
    m_jobQueue.emplace_back(job);
  else
    m_jobQueue.emplace_front(job);

  QueueNextJob();

  return true;
}

void CJobQueue::OnJobNotify(const CJob* job)
{
  std::unique_lock lock(m_section);

  // check if this job is in our processing list
  const auto it = std::ranges::find_if(m_processing, JobFinder(job));
  if (it != m_processing.cend())
    m_processing.erase(it);

  // request a new job be queued
  QueueNextJob();
}

void CJobQueue::QueueNextJob()
{
  std::unique_lock lock(m_section);
  while (!m_jobQueue.empty() && m_processing.size() < m_jobsAtOnce)
  {
    CJobPointer& job = m_jobQueue.back();
    job.SetId(CServiceBroker::GetJobManager()->AddJob(job.GetJob(), this, m_priority));
    if (job.GetId() > 0)
    {
      m_processing.emplace_back(job);
      m_jobQueue.pop_back();
      return;
    }
    m_jobQueue.pop_back();
  }
}

void CJobQueue::CancelJobs()
{
  std::unique_lock lock(m_section);
  std::ranges::for_each(m_processing, [](CJobPointer& jp) { jp.CancelJob(); });
  std::ranges::for_each(m_jobQueue, [](CJobPointer& jp) { jp.FreeJob(); });
  m_jobQueue.clear();
  m_processing.clear();
}

bool CJobQueue::IsProcessing() const
{
  return CServiceBroker::GetJobManager()->IsRunning() &&
         (!m_processing.empty() || !m_jobQueue.empty());
}

bool CJobQueue::QueueEmpty() const
{
  std::unique_lock lock(m_section);
  return m_jobQueue.empty();
}
