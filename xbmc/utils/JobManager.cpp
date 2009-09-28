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

#include "JobManager.h"
#include <algorithm>
#include "SingleLock.h"

using namespace std;

bool CJob::ShouldCancel(unsigned int progress, unsigned int total) const
{
  if (m_callback)
    return m_callback->OnJobProgress(progress, total, this);
  return false;
}

CJobWorker::CJobWorker(CJobManager *manager)
{
  m_jobManager = manager;
  SetName("Jobworker");
  Create(true); // start work immediately, and kill ourselves when we're done
}

CJobWorker::~CJobWorker()
{
  StopThread();
}

void CJobWorker::Process()
{
  while (true)
  {
    // request an item from our manager (this call is blocking)
    CJob *job = m_jobManager->GetNextJob(this);
    if (!job)
      break;
    
    // we have a job to do
    job->DoWork();
    m_jobManager->OnJobComplete(job);
  }
}

CJobManager &CJobManager::GetInstance()
{
  static CJobManager sJobManager;
  return sJobManager;
}

CJobManager::CJobManager()
{
  m_jobCounter = 0;
  m_running = true;
}

CJobManager::~CJobManager()
{
  CSingleLock lock(m_section);
  m_running = false;
  
  // clear any pending jobs
  for (unsigned int priority = CJob::PRIORITY_LOW; priority <= CJob::PRIORITY_HIGH; ++priority)
  {
    for_each(m_jobQueue[priority].begin(), m_jobQueue[priority].end(), mem_fun_ref(&CWorkItem::FreeJob));
    m_jobQueue[priority].clear();
  }
  
  // cancel any callbacks on jobs still processing
  for_each(m_processing.begin(), m_processing.end(), mem_fun_ref(&CWorkItem::Cancel));

  // and tell our workers to finish
  while (m_workers.size())
  {
    lock.Leave();
    m_jobEvent.Set();
    Sleep(0); // yield after setting the event to give the workers some time to die
    lock.Enter();
  }
}

unsigned int CJobManager::AddJob(CJob *job, IJobCallback *callback, CJob::PRIORITY priority)
{
  CSingleLock lock(m_section);

  // create a work item for this job
  CWorkItem work(job, m_jobCounter++, callback);
  m_jobQueue[priority].push_back(work);

  StartWorkers(priority);
  return work.m_id;
}

void CJobManager::CancelJob(unsigned int jobID)
{
  CSingleLock lock(m_section);

  // check whether we have this job in the queue
  for (unsigned int priority = CJob::PRIORITY_LOW; priority <= CJob::PRIORITY_HIGH; ++priority)
  {
    JobQueue::iterator i = find(m_jobQueue[priority].begin(), m_jobQueue[priority].end(), jobID);
    if (i != m_jobQueue[priority].end())
    {
      delete i->m_job;
      m_jobQueue[priority].erase(i);
      return;
    }
  }
  // or if we're processing it
  Processing::iterator it = find(m_processing.begin(), m_processing.end(), jobID);
  if (it != m_processing.end())
    it->m_callback = NULL; // job is in progress, so only thing to do is to remove callback
}

void CJobManager::StartWorkers(CJob::PRIORITY priority)
{
  CSingleLock lock(m_section);

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
  m_workers.push_back(new CJobWorker(this));
}

CJob *CJobManager::GetNextJob(const CJobWorker *worker)
{
  CSingleLock lock(m_section);
  while (m_running)
  {
    // grab a job off the queue if we have one
    for (int priority = CJob::PRIORITY_HIGH; priority >= CJob::PRIORITY_LOW; --priority)
    {
      if (m_jobQueue[priority].size())
      {
        CWorkItem job = m_jobQueue[priority].front();
        m_jobQueue[priority].pop_front();
        // add to the processing vector
        m_processing.push_back(job);
        return job.m_job;
      }
    }
    // no jobs are left - sleep for 30 seconds to allow new jobs to come in
    lock.Leave();
    if (!m_jobEvent.WaitMSec(30000))
    { // timeout - say goodbye to the thread
      break;
    }
    lock.Enter();
    m_jobEvent.Reset();
  }
  RemoveWorker(worker);
  return NULL;
}

bool CJobManager::OnJobProgress(unsigned int progress, unsigned int total, const CJob *job) const
{
  CSingleLock lock(m_section);
  // find the job in the processing queue, and check whether it's cancelled (no callback)
  Processing::const_iterator i = find(m_processing.begin(), m_processing.end(), job);
  if (i != m_processing.end())
  {
    CWorkItem item(*i);
    lock.Leave(); // leave section prior to call
    if (item.m_callback)
      item.m_callback->OnJobProgress(item.m_id, progress, total, job);
    return false;
  }
  return true; // couldn't find the job, or it's been cancelled
}

void CJobManager::OnJobComplete(CJob *job)
{
  CSingleLock lock(m_section);
  // remove the job from the processing queue
  Processing::iterator i = find(m_processing.begin(), m_processing.end(), job);
  if (i != m_processing.end())
  {
    // tell any listeners we're done with the job, then delete it
    CWorkItem item(*i);
    m_processing.erase(i);
    lock.Leave();
    if (item.m_callback)
      item.m_callback->OnJobComplete(item.m_id, item.m_job);
    item.FreeJob();
  }
}

void CJobManager::RemoveWorker(const CJobWorker *worker)
{
  CSingleLock lock(m_section);
  // remove our worker
  Workers::iterator i = find(m_workers.begin(), m_workers.end(), worker);
  if (i != m_workers.end())
    m_workers.erase(i); // workers auto-delete
}

unsigned int CJobManager::GetMaxWorkers(CJob::PRIORITY priority) const
{
  static const unsigned int max_workers = 5;
  return max_workers - (CJob::PRIORITY_HIGH - priority);
}
