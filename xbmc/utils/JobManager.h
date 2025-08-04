/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Job.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <array>
#include <queue>
#include <string>
#include <vector>

class CJobManager;

class CJobWorker : public CThread
{
public:
  explicit CJobWorker(CJobManager& manager);
  ~CJobWorker() override;

  void Process() override;

private:
  CJobManager& m_jobManager;
};

template<typename F>
class CLambdaJob : public CJob
{
public:
  explicit CLambdaJob(F function) : m_function(std::move(function)) {}
  ~CLambdaJob() override = default;

  bool DoWork() override
  {
    m_function();
    return true;
  }

  bool Equals(const CJob* job) const override { return this == job; }

private:
  CLambdaJob() = delete;

  F m_function;
};

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

public:
  /*!
   \brief CJobQueue constructor
   \param lifo whether the queue should be processed last in first out or first in first out.  Defaults to false (first in first out)
   \param jobsAtOnce number of jobs at once to process.  Defaults to 1.
   \param priority priority of this queue.
   \sa CJob
   */
  CJobQueue(bool lifo = false,
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
  void OnJobNotify(CJob* job);
  void QueueNextJob();

  using Queue = std::deque<CJobPointer>;
  using Processing = std::vector<CJobPointer>;
  Queue m_jobQueue;
  Processing m_processing;

  unsigned int m_jobsAtOnce{5};
  CJob::PRIORITY m_priority{CJob::PRIORITY::PRIORITY_LOW};
  mutable CCriticalSection m_section;
  bool m_lifo{false};
};

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
  friend class CJobWorker; // for access to RemoveWorker()

  CJobManager(const CJobManager&) = delete;
  CJobManager const& operator=(CJobManager const&) = delete;

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
