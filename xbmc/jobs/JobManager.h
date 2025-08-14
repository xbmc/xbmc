/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/Job.h"
#include "jobs/LambdaJob.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

#include <array>
#include <queue>
#include <string>
#include <vector>

class IJobCallback;

/*!
 \ingroup jobs
 \brief Job Manager class for scheduling asynchronous jobs.

 Controls asynchronous job execution, by allowing clients to add and cancel jobs.
 Should be accessed via CServiceBroker::GetJobManager().  Jobs are allocated based
 on priority levels.  Lower priority jobs are executed only if there are sufficient
 spare worker threads free to allow for higher priority jobs that may arise.

 \sa CJob and IJobCallback
 */
class CJobManager final
{
public:
  CJobManager() = default;

  /*!
   \brief Returns whether the job manager is currently running.
   \return True if the job manager is running and able to process jobs, false if it has been stopped or not started.
   The method will return false after the job manager has been shut down or before it has been started.
   \sa CJob
   */
  bool IsRunning() const;

  /*!
   \brief Add a job to the threaded job manager.
   On completion or abort of the job or in case the job could not be added successfully, the CJob object will be destroyed.
   \param job a pointer to the job to add. The job should be subclassed from CJob
   \param callback a pointer to an IJobCallback instance to receive job progress and completion notices.
   \param priority the priority that this job should run at.
   \return On success, a unique identifier for this job, to be used with other interaction, 0 otherwise.
   In case of failure, the passed CJob object will be deleted before returning from this method.
   \sa CJob, IJobCallback, CancelJob()
   */
  unsigned int AddJob(CJob* job,
                      IJobCallback* callback,
                      CJob::PRIORITY priority = CJob::PRIORITY_LOW);

  /*!
   \brief Add a function f to this job manager for asynchronous execution.
   \param f the function to add.
   \param priority the priority that this job should run at.
   \sa CJob, AddJob()
   */
  template<typename F>
  void Submit(F f, CJob::PRIORITY priority = CJob::PRIORITY_LOW)
  {
    AddJob(new CLambdaJob<F>(std::move(f)), nullptr, priority);
  }

  /*!
   \brief Add a function f to this job manager for asynchronous execution.
   \param f the function to add.
   \param callback a pointer to an IJobCallback instance to receive job progress and completion notices.
   \param priority the priority that this job should run at.
   \sa CJob, IJobCallback, AddJob()
   */
  template<typename F>
  void Submit(F f, IJobCallback* callback, CJob::PRIORITY priority = CJob::PRIORITY_LOW)
  {
    AddJob(new CLambdaJob<F>(std::move(f)), callback, priority);
  }

  /*!
   \brief Cancel a job with the given id.
   \param jobID the id of the job to cancel, retrieved previously from AddJob()
   \sa AddJob()
   */
  void CancelJob(unsigned int jobID);

  /*!
   \brief Cancel all remaining jobs, preparing for shutdown
   Should be called prior to destroying any objects that may be being used as callbacks
   \sa CancelJob(), AddJob()
   */
  void CancelJobs();

  /*!
   \brief Re-start accepting jobs again
   Called after calling CancelJobs() to allow this manager to accept more jobs
   \throws std::logic_error if the manager was not previously cancelled
   \sa CancelJobs()
   */
  void Restart();

  /*!
   \brief Checks to see if any jobs of a specific type are currently processing.
   \param type Job type to search for
   \return Number of matching jobs
   */
  int IsProcessing(const std::string& type) const;

  /*!
   \brief Suspends queueing of jobs with priority PRIORITY_LOW_PAUSABLE until unpaused
   Useful to (for ex) stop queuing thumb jobs during video start/playback.
   Does not affect currently processing jobs, use IsProcessing to see if any need to be waited on
   \sa UnPauseJobs()
   */
  void PauseJobs();

  /*!
   \brief Resumes queueing of (previously paused) jobs with priority PRIORITY_LOW_PAUSABLE
   \sa PauseJobs()
   */
  void UnPauseJobs();

  /*!
   \brief Checks to see if any jobs with specific priority are currently processing.
   \param priority to search for
   \return true if processing jobs, else returns false
   */
  bool IsProcessing(const CJob::PRIORITY& priority) const;

  /*!
   \brief Callback from CJobWorker after a job has completed.
   Calls IJobCallback::OnJobComplete(), and then destroys job.
   \param job a pointer to the calling subclassed CJob instance.
   \param success the result from the DoWork call
   \sa IJobCallback, CJob
   */
  void OnJobComplete(bool success, CJob* job);

  /*!
   \brief Callback from CJob to report progress and check for cancellation.
   Checks for cancellation, and calls IJobCallback::OnJobProgress().
   \param progress amount of processing performed to date, out of total.
   \param total total amount of processing.
   \param job pointer to the calling subclassed CJob instance.
   \return true if the job has been cancelled, else returns false.
   \sa IJobCallback, CJob
   */
  bool OnJobProgress(unsigned int progress, unsigned int total, const CJob* job) const;

  /*!
   \brief Get a new job to process. Blocks until a new job is available, or a timeout has occurred.
   \sa CJob
   */
  CJob* GetNextJob();

private:
  CJobManager(const CJobManager&) = delete;
  CJobManager const& operator=(CJobManager const&) = delete;

  class CJobWorker;
  struct JobFinder;

  class CWorkItem
  {
  public:
    CWorkItem(CJob* job, unsigned int id, CJob::PRIORITY priority, IJobCallback* callback)
      : m_job(job),
        m_id(id),
        m_callback(callback),
        m_priority(priority)
    {
    }

    void FreeJob()
    {
      delete m_job;
      m_job = nullptr;
    }

    void Cancel() { m_callback = nullptr; }
    CJob* GetJob() const { return m_job; }
    unsigned int GetId() const { return m_id; }
    IJobCallback* GetCallback() const { return m_callback; }
    void SetCallback(IJobCallback* callback) { m_callback = callback; }
    CJob::PRIORITY GetPriority() const { return m_priority; }

  private:
    CJob* m_job{nullptr};
    unsigned int m_id{0};
    IJobCallback* m_callback{nullptr};
    CJob::PRIORITY m_priority{CJob::PRIORITY::PRIORITY_LOW};
  };

  /*! \brief Pop a job off the job queue and add to the processing queue ready to process
   \return the job to process, nullptr if no jobs are available
   */
  CJob* PopJob();

  void StartWorkers(CJob::PRIORITY priority);
  void RemoveWorker(const CJobWorker* worker);
  static unsigned int GetMaxWorkers(CJob::PRIORITY priority);

  unsigned int m_jobCounter{0};

  using JobQueue = std::deque<CWorkItem>;
  using Processing = std::vector<CWorkItem>;
  using Workers = std::vector<CJobWorker*>;

  std::array<JobQueue, CJob::PRIORITY_DEDICATED + 1> m_jobQueue;
  bool m_pauseJobs{false};
  Processing m_processing;
  Workers m_workers;

  mutable CCriticalSection m_section;
  CEvent m_jobEvent;
  bool m_running{true};
};
