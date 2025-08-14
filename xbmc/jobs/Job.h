/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stddef.h>

class CJobManager;

/*!
 \ingroup jobs
 \brief Base class for jobs that are executed asynchronously.

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
  enum PRIORITY
  {
    PRIORITY_LOW_PAUSABLE = 0,
    PRIORITY_LOW,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_DEDICATED, // will create a new worker if no worker is available at queue time
  };

  CJob() = default;

  /*!
   \brief Destructor for job objects.

   Jobs are destroyed by the CJobManager after the OnJobComplete() or OnJobAbort() callback is
   complete.  CJob subclasses should therefore supply a virtual destructor to cleanup any memory
   allocated by complete or cancelled jobs.

   \sa CJobManager
   */
  virtual ~CJob() = default;

  /*!
   \brief Main workhorse function of CJob instances

   All CJob subclasses must implement this function, performing all processing.  Once this function
   is complete, the OnJobComplete() callback is called, and the job is then destroyed.

   \sa CJobManager, IJobCallback::OnJobComplete()
   */
  virtual bool DoWork() = 0; // function to do the work

  /*!
   \brief Function that returns the type of job.

   CJob subclasses may optionally implement this function to specify the type of job.
   This is useful for the CJobManager::AddJob() routine, which preempts similar jobs
   with the new job.

   \return a unique character string describing the job.
   \sa CJobManager
   */
  virtual const char* GetType() const { return ""; }

  /*!
   \brief Function that compares this job instance with the given job instance.

   CJob subclasses may optionally implement this to provide customized comparison functionality.
   This is useful for the CJobManager::AddJob() routine, which preempts similar jobs
   with the new job.

   \param job the job to compared with this CJob instance.
   \return if true, the two jobs are equal.

   \sa CJobManager::AddJob()
   */
  virtual bool Equals(const CJob* job) const { return false; }

  /*!
   \brief Function to set a callback for jobs to report progress.

   \param callback the callback to use to report progress.

   \sa IJobCallback::OnJobProgress()
   */
  void SetProgressCallback(CJobManager* callback) { m_progressCallback = callback; }

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
  CJobManager* m_progressCallback{nullptr};
};
