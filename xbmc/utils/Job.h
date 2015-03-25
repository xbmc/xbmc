#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

class CJob;

#include <stddef.h>

#define kJobTypeMediaFlags  "mediaflags"
#define kJobTypeCacheImage  "cacheimage"
#define kJobTypeDDSCompress "ddscompress"

/*!
 \ingroup jobs
 \brief Callback interface for asynchronous jobs.

 Used by clients of the CJobManager to receive progress and completion notification of jobs.
 Clients of small jobs wishing to perform actions on job completion should implement the
 IJobCallback::OnJobComplete() function.  Clients of larger jobs may choose to implement the
 IJobCallback::OnJobProgress() function in order to be kept informed of progress.

 \sa CJobManager and CJob
 */
class IJobCallback
{
public:
  /*!
   \brief Destructor for job call back objects.
   
   \sa CJobManager and CJob
   */
  virtual ~IJobCallback() {};

  /*!
   \brief The callback used when a job completes.
   
   OnJobComplete is called at the completion of the job's DoWork() function, and is used
   to return information to the caller on the result of the job.  On returning form this function
   the CJobManager will destroy this job.
   
   \param jobID the unique id of the job (as retrieved from CJobManager::AddJob)
   \param success the result from the DoWork call
   \param job the job that has been processed.  The job will be destroyed after this function returns
   \sa CJobManager and CJob
   */
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job)=0;

  /*!
   \brief An optional callback function that a job may call while processing.
   
   OnJobProgress may be called periodically by a job during it's DoWork() function.  It is used
   by the job to report on progress.
   
   \param jobID the unique id of the job (as retrieved from CJobManager::AddJob)
   \param progress the current progress of the job, out of total.
   \param total the total amount of work to be processed.
   \param job the job that has been processed.
   \sa CJobManager and CJob
   */
  virtual void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job) {};
};

class CJobManager;

/*!
 \ingroup jobs
 \brief Base class for jobs that are executed asyncronously.
 
 Clients of the CJobManager should subclass CJob and provide the DoWork() function. Data should be
 passed to the job on creation, and any data sharing between the job and the client should be kept to within
 the callback functions if possible, and guarded with critical sections as appropriate.
 
 Jobs typically fall into two groups: small jobs that perform a single function, and larger jobs that perform a
 sequence of functions.  Clients with small jobs should implement the IJobCallback::OnJobComplete() callback to receive results.
 Clients with larger jobs may wish to implement both the IJobCallback::OnJobComplete() and IJobCallback::OnJobProgress()
 callbacks to receive updates.  Jobs may be cancelled at any point by the client via CJobManager::CancelJob(), however
 effort should be taken to ensure that any callbacks and cancellation is suitably guarded against simultaneous thread access.
 
 Handling cancellation of jobs within the OnJobProgress callback is a threadsafe operation, as all execution is
 then in the Job thread.
 
 \sa CJobManager and IJobCallback
 */
class CJob
{
public:
  /*!
   \brief Priority levels for jobs, specified by clients when adding jobs to the CJobManager.
   \sa CJobManager
   */
  enum PRIORITY {
    PRIORITY_LOW_PAUSABLE = 0,
    PRIORITY_LOW,
    PRIORITY_NORMAL,
    PRIORITY_HIGH
  };
  CJob() { m_callback = NULL; };

  /*!
   \brief Destructor for job objects.
   
   Jobs are destroyed by the CJobManager after the OnJobComplete() callback is complete.
   CJob subclasses  should therefore supply a virtual destructor to cleanup any memory allocated by
   complete or cancelled jobs.
   
   \sa CJobManager
   */
  virtual ~CJob() {};

  /*!
   \brief Main workhorse function of CJob instances
   
   All CJob subclasses must implement this function, performing all processing.  Once this function
   is complete, the OnJobComplete() callback is called, and the job is then destroyed.
   
   \sa CJobManager, IJobCallback::OnJobComplete()
   */
  virtual bool DoWork() = 0;  // function to do the work

  /*!
   \brief Function that returns the type of job.
   
   CJob subclasses may optionally implement this function to specify the type of job.
   This is useful for the CJobManager::AddLIFOJob() routine, which preempts similar jobs
   with the new job.
   
   \return a unique character string describing the job.
   \sa CJobManager
   */
  virtual const char *GetType() const { return ""; };

  virtual bool operator==(const CJob* job) const
  {
    return false;
  }

  /*!
   \brief Function for longer jobs to report progress and check whether they have been cancelled.
   
   Jobs that contain loops that may take time should check this routine each iteration of the loop,
   both to (optionally) report progress, and to check for cancellation.
   
   \param progress the amount of the job performed, out of total. 
   \param total the total amount of processing to be performed
   \return if true, the job has been asked to cancel.
   
   \sa IJobCallback::OnJobProgress()
   */
  virtual bool ShouldCancel(unsigned int progress, unsigned int total) const;
private:
  friend class CJobManager;
  CJobManager *m_callback;
};
