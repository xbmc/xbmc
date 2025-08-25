/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/IJobCallback.h"
#include "jobs/Job.h"
#include "jobs/LambdaJob.h"
#include "threads/CriticalSection.h"

#include <queue>
#include <vector>

/*!
 \ingroup jobs
 \brief Job Queue class to handle a queue of unique jobs to be processed sequentially

 Holds a queue of jobs to be processed sequentially, either first in,first out
 or last in, first out.  Jobs are unique, so queueing multiple copies of the same job
 (based on result of virtual CJob::Equals()) will not add additional jobs.

 Classes should subclass this class and override OnJobCallback should they require
 information from the job.

 \sa CJob and IJobCallback
 */
class CJobQueue : public IJobCallback
{
public:
  /*!
   \brief CJobQueue constructor
   \param lifo whether the queue should be processed last in first out or first in first out.  Defaults to false (first in first out)
   \param jobsAtOnce number of jobs at once to process.  Defaults to 1.
   \param priority priority of this queue.
   \sa CJob
   */
  explicit CJobQueue(bool lifo = false,
                     unsigned int jobsAtOnce = 1,
                     CJob::PRIORITY priority = CJob::PRIORITY_LOW);

  /*!
   \brief CJobQueue destructor
   Cancels any in-process jobs, and destroys the job queue.
   \sa CJob
   */
  ~CJobQueue() override;

  /*!
   \brief Add a job to the queue
   On completion of the job, destruction of the job queue or in case the job could not be added successfully, the CJob object will be destroyed.
   \param job a pointer to the job to add. The job should be subclassed from CJob.
   \return True if the job was added successfully, false otherwise.
   In case of failure, the passed CJob object will be deleted before returning from this method.
   \sa CJob
   */
  bool AddJob(CJob* job);

  /*!
   \brief Add a function f to this job queue
   */
  template<typename F>
  void Submit(F&& f)
  {
    AddJob(new CLambdaJob<F>(std::forward<F>(f)));
  }

  /*!
   \brief Cancel a job in the queue
   Cancels a job in the queue. Any job currently being processed may complete after this
   call has completed, but OnJobComplete will not be performed. If the job is only queued
   then it will be removed from the queue and deleted.
   \param job a pointer to the job to cancel. The job should be subclassed from CJob.
   \sa CJob
   */
  void CancelJob(const CJob* job);

  /*!
   \brief Cancel all jobs in the queue
   Removes all jobs from the queue. Any job currently being processed may complete after this
   call has completed, but OnJobComplete will not be performed.
   \sa CJob
   */
  void CancelJobs();

  /*!
   \brief Check whether the queue is processing a job
   */
  bool IsProcessing() const;

  /*!
   \brief The callback used when a job completes.

   CJobQueue implementation will cleanup the internal processing queue and then queue the next
   job at the job manager, if any.

   \param jobID the unique id of the job (as retrieved from CJobManager::AddJob)
   \param success the result from the DoWork call
   \param job the job that has been processed.
   \sa CJobManager, IJobCallback and CJob
   */
  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;

  /*!
   \brief The callback used when a job will be aborted.

   CJobQueue implementation will cleanup the internal processing queue and then queue the next
   job at the job manager, if any.

   \param jobID the unique id of the job (as retrieved from CJobManager::AddJob)
   \param job the job that has been aborted.
   \sa CJobManager, IJobCallback and CJob
   */
  void OnJobAbort(unsigned int jobID, CJob* job) override;

protected:
  /*!
   \brief Returns if we still have jobs waiting to be processed
   NOTE: This function does not take into account the jobs that are currently processing
   */
  bool QueueEmpty() const;

private:
  struct JobFinder;

  class CJobPointer
  {
  public:
    explicit CJobPointer(CJob* job) : m_job(job) {}

    void CancelJob();

    void FreeJob()
    {
      delete m_job;
      m_job = nullptr;
    }

    CJob* GetJob() const { return m_job; }
    unsigned int GetId() const { return m_id; }
    void SetId(unsigned int jobId) { m_id = jobId; }

  private:
    CJob* m_job{nullptr};
    unsigned int m_id{0};
  };

  void OnJobNotify(const CJob* job);
  void QueueNextJob();

  using Queue = std::deque<CJobPointer>;
  using Processing = std::vector<CJobPointer>;
  Queue m_jobQueue;
  Processing m_processing;

  unsigned int m_jobsAtOnce{1};
  CJob::PRIORITY m_priority{CJob::PRIORITY::PRIORITY_LOW};
  mutable CCriticalSection m_section;
  bool m_lifo{false};
};
