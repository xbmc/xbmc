#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <queue>
#include <vector>
#include "CriticalSection.h"
#include "Thread.h"
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
   \sa CJob, IJobInterface, CancelJob()
   */
  unsigned int AddJob(CJob *job, IJobCallback *callback, CJob::PRIORITY priority = CJob::PRIORITY_LOW);

  /*!
   \brief Cancel a job with the given id.
   \param jobID the id of the job to cancel, retrieved previously from AddJob()
   \sa AddJob()
   */
  void CancelJob(unsigned int jobID);

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
   \sa IJobCallback, CJob
   */
  void  OnJobComplete(CJob *job);

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
};
