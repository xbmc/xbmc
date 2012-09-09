#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <queue>
#include <vector>
#include <string>
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "Job.h"

class CJobManager;

class CJobWorker : public CThread
{
public:
  CJobWorker(CJobManager *manager);
  virtual ~CJobWorker();

  void Process();
private:
  CJobManager  *m_jobManager;
};

/*!
 \ingroup jobs
 \brief Job Queue class to handle a queue of unique jobs to be processed sequentially

 Holds a queue of jobs to be processed sequentially, either first in,first out
 or last in, first out.  Jobs are unique, so queueing multiple copies of the same job
 (based on the CJob::operator==) will not add additional jobs.

 Classes should subclass this class and override OnJobCallback should they require
 information from the job.

 \sa CJob and IJobCallback
 */
class CJobQueue: public IJobCallback
{
  class CJobPointer
  {
  public:
    CJobPointer(CJob *job)
    {
      m_job = job;
      m_id = 0;
    };
    void CancelJob();
    void FreeJob()
    {
      delete m_job;
      m_job = NULL;
    };
    bool operator==(const CJob *job) const
    {
      if (m_job)
        return *m_job == job;
      return false;
    };
    CJob *m_job;
    unsigned int m_id;
  };
public:
  /*!
   \brief CJobQueue constructor
   \param lifo whether the queue should be processed last in first out or first in first out.  Defaults to false (first in first out)
   \param jobsAtOnce number of jobs at once to process.  Defaults to 1.
   \param priority priority of this queue.
   \sa CJob
   */
  CJobQueue(bool lifo = false, unsigned int jobsAtOnce = 1, CJob::PRIORITY priority = CJob::PRIORITY_LOW);

  /*!
   \brief CJobQueue destructor
   Cancels any in-process jobs, and destroys the job queue.
   \sa CJob
   */
  virtual ~CJobQueue();

  /*!
   \brief Add a job to the queue
   On completion of the job (or destruction of the job queue) the CJob object will be destroyed.
   \param job a pointer to the job to add. The job should be subclassed from CJob.
   \sa CJob
   */
  void AddJob(CJob *job);

  /*!
   \brief Cancel a job in the queue
   Cancels a job in the queue. Any job currently being processed may complete after this
   call has completed, but OnJobComplete will not be performed. If the job is only queued
   then it will be removed from the queue and deleted.
   \param job a pointer to the job to cancel. The job should be subclassed from CJob.
   \sa CJob
   */
  void CancelJob(const CJob *job);

  /*!
   \brief Cancel all jobs in the queue
   Removes all jobs from the queue. Any job currently being processed may complete after this
   call has completed, but OnJobComplete will not be performed.
   \sa CJob
   */
  void CancelJobs();

  /*!
   \brief The callback used when a job completes.

   OnJobComplete is called at the completion of the CJob::DoWork function, and is used
   to return information to the caller on the result of the job.  On returning from this function
   the CJobManager will destroy this job.

   Subclasses should override this function if they wish to transfer information from the job prior
   to it's deletion.  They must then call this base class function, which will move on to the next
   job.

   \sa CJobManager, IJobCallback and  CJob
   */
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  void QueueNextJob();

  typedef std::deque<CJobPointer> Queue;
  typedef std::vector<CJobPointer> Processing;
  Queue m_jobQueue;
  Processing m_processing;

  unsigned int m_jobsAtOnce;
  CJob::PRIORITY m_priority;
  CCriticalSection m_section;
  bool m_lifo;
};

/*!
 \ingroup jobs
 \brief Job Manager class for scheduling asynchronous jobs.

 Controls asynchronous job execution, by allowing clients to add and cancel jobs.
 Should be accessed via CJobManager::GetInstance().  Jobs are allocated based on
 priority levels.  Lower priority jobs are executed only if there are sufficient
 spare worker threads free to allow for higher priority jobs that may arise.

 \sa CJob and IJobCallback
 */
class CJobManager
{
  class CWorkItem
  {
  public:
    CWorkItem(CJob *job, unsigned int id, IJobCallback *callback)
    {
      m_job = job;
      m_id = id;
      m_callback = callback;
    }
    bool operator==(unsigned int jobID) const
    {
      return m_id == jobID;
    };
    bool operator==(const CJob *job) const
    {
      return m_job == job;
    };
    void FreeJob()
    {
      delete m_job;
      m_job = NULL;
    };
    void Cancel()
    {
      m_callback = NULL;
    };
    CJob         *m_job;
    unsigned int  m_id;
    IJobCallback *m_callback;
  };

public:
  /*!
   \brief The only way through which the global instance of the CJobManager should be accessed.
   \return the global instance.
   */
  static CJobManager &GetInstance();

  /*!
   \brief Add a job to the threaded job manager.
   \param job a pointer to the job to add. The job should be subclassed from CJob
   \param callback a pointer to an IJobCallback instance to receive job progress and completion notices.
   \param priority the priority that this job should run at.
   \return a unique identifier for this job, to be used with other interaction
   \sa CJob, IJobCallback, CancelJob()
   */
  unsigned int AddJob(CJob *job, IJobCallback *callback, CJob::PRIORITY priority = CJob::PRIORITY_LOW);

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
   \brief Suspends queueing of the specified type until unpaused
   Useful to (for ex) stop queuing thumb jobs during video playback. Only affects PRIORITY_LOW or lower.
   Does not affect currently processing jobs, use IsProcessing to see if any need to be waited on
   Types accumulate, so more than one can be set at a time.
   Refcounted, so UnPause() must be called once for each Pause().
   \param pausedType only jobs of this type will be affected
   \sa UnPause(), IsPaused(), IsProcessing()
   */
  void Pause(const std::string &pausedType);

  /*!
   \brief Resumes queueing of the specified type
   \param pausedType only jobs of this type will be affected
   \sa Pause(), IsPaused(), IsProcessing()
   */
  void UnPause(const std::string &pausedType);

  /*!
   \brief Checks if jobs of specified type are paused.
   \param pausedType only jobs of this type will be affected
   \sa Pause(), UnPause(), IsProcessing()
   */
  bool IsPaused(const std::string &pausedType);

  /*!
   \brief Checks to see if any jobs of a specific type are currently processing.
   \param pausedType Job type to search for
   \return Number of matching jobs
   \sa Pause(), UnPause(), IsPaused()
   */
  int IsProcessing(const std::string &pausedType);

protected:
  friend class CJobWorker;
  friend class CJob;

  /*!
   \brief Get a new job to process. Blocks until a new job is available, or a timeout has occurred.
   \param worker a pointer to the current CJobWorker instance requesting a job.
   \sa CJob
   */
  CJob *GetNextJob(const CJobWorker *worker);

  /*!
   \brief Callback from CJobWorker after a job has completed.
   Calls IJobCallback::OnJobComplete(), and then destroys job.
   \param job a pointer to the calling subclassed CJob instance.
   \param success the result from the DoWork call
   \sa IJobCallback, CJob
   */
  void  OnJobComplete(bool success, CJob *job);

  /*!
   \brief Callback from CJob to report progress and check for cancellation.
   Checks for cancellation, and calls IJobCallback::OnJobProgress().
   \param progress amount of processing performed to date, out of total.
   \param total total amount of processing.
   \param job pointer to the calling subclassed CJob instance.
   \return true if the job has been cancelled, else returns false.
   \sa IJobCallback, CJob
   */
  bool  OnJobProgress(unsigned int progress, unsigned int total, const CJob *job) const;

private:
  // private construction, and no assignements; use the provided singleton methods
  CJobManager();
  CJobManager(const CJobManager&);
  CJobManager const& operator=(CJobManager const&);
  virtual ~CJobManager();

  /*! \brief Pop a job off the job queue and add to the processing queue ready to process
   \return the job to process, NULL if no jobs are available
   */
  CJob *PopJob();

  void StartWorkers(CJob::PRIORITY priority);
  void RemoveWorker(const CJobWorker *worker);
  unsigned int GetMaxWorkers(CJob::PRIORITY priority) const;

  unsigned int m_jobCounter;

  typedef std::deque<CWorkItem>    JobQueue;
  typedef std::vector<CWorkItem>   Processing;
  typedef std::vector<CJobWorker*> Workers;

  JobQueue   m_jobQueue[CJob::PRIORITY_HIGH+1];
  Processing m_processing;
  Workers    m_workers;

  CCriticalSection m_section;
  CEvent           m_jobEvent;
  bool             m_running;
  std::vector<std::string>  m_pausedTypes;
};
