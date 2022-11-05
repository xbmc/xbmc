/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryQueue.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "video/jobs/VideoLibraryCleaningJob.h"
#include "video/jobs/VideoLibraryJob.h"
#include "video/jobs/VideoLibraryMarkWatchedJob.h"
#include "video/jobs/VideoLibraryRefreshingJob.h"
#include "video/jobs/VideoLibraryResetResumePointJob.h"
#include "video/jobs/VideoLibraryScanningJob.h"

#include <mutex>
#include <utility>

CVideoLibraryQueue::CVideoLibraryQueue()
  : CJobQueue(false, 1, CJob::PRIORITY_LOW),
    m_jobs()
{ }

CVideoLibraryQueue::~CVideoLibraryQueue()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  m_jobs.clear();
}

CVideoLibraryQueue& CVideoLibraryQueue::GetInstance()
{
  static CVideoLibraryQueue s_instance;
  return s_instance;
}

void CVideoLibraryQueue::ScanLibrary(const std::string& directory, bool scanAll /* = false */ , bool showProgress /* = true */)
{
  AddJob(new CVideoLibraryScanningJob(directory, scanAll, showProgress));
}

bool CVideoLibraryQueue::IsScanningLibrary() const
{
  // check if the library is being cleaned synchronously
  if (m_cleaning)
    return true;

  // check if the library is being scanned asynchronously
  VideoLibraryJobMap::const_iterator scanningJobs = m_jobs.find("VideoLibraryScanningJob");
  if (scanningJobs != m_jobs.end() && !scanningJobs->second.empty())
    return true;

  // check if the library is being cleaned asynchronously
  VideoLibraryJobMap::const_iterator cleaningJobs = m_jobs.find("VideoLibraryCleaningJob");
  if (cleaningJobs != m_jobs.end() && !cleaningJobs->second.empty())
    return true;

  return false;
}

void CVideoLibraryQueue::StopLibraryScanning()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  VideoLibraryJobMap::const_iterator scanningJobs = m_jobs.find("VideoLibraryScanningJob");
  if (scanningJobs == m_jobs.end())
    return;

  // get a copy of the scanning jobs because CancelJob() will modify m_scanningJobs
  VideoLibraryJobs tmpScanningJobs(scanningJobs->second.begin(), scanningJobs->second.end());

  // cancel all scanning jobs
  for (VideoLibraryJobs::const_iterator job = tmpScanningJobs.begin(); job != tmpScanningJobs.end(); ++job)
    CancelJob(*job);
  Refresh();
}

bool CVideoLibraryQueue::CleanLibrary(const std::set<int>& paths /* = std::set<int>() */,
                                      bool asynchronous /* = true */,
                                      CGUIDialogProgressBarHandle* progressBar /* = NULL */)
{
  CVideoLibraryCleaningJob* cleaningJob = new CVideoLibraryCleaningJob(paths, progressBar);

  if (asynchronous)
    AddJob(cleaningJob);
  else
  {
    // we can't perform a modal library cleaning if other jobs are running
    if (IsRunning())
      return false;

    m_modal = true;
    m_cleaning = true;
    cleaningJob->DoWork();

    delete cleaningJob;
    m_cleaning = false;
    m_modal = false;
    Refresh();
  }

  return true;
}

bool CVideoLibraryQueue::CleanLibraryModal(const std::set<int>& paths /* = std::set<int>() */)
{
  // we can't perform a modal library cleaning if other jobs are running
  if (IsRunning())
    return false;

  m_modal = true;
  m_cleaning = true;
  CVideoLibraryCleaningJob cleaningJob(paths, true);
  cleaningJob.DoWork();
  m_cleaning = false;
  m_modal = false;
  Refresh();

  return true;
}

void CVideoLibraryQueue::RefreshItem(std::shared_ptr<CFileItem> item,
                                     bool ignoreNfo /* = false */,
                                     bool forceRefresh /* = true */,
                                     bool refreshAll /* = false */,
                                     const std::string& searchTitle /* = "" */)
{
  AddJob(new CVideoLibraryRefreshingJob(std::move(item), forceRefresh, refreshAll, ignoreNfo,
                                        searchTitle));
}

bool CVideoLibraryQueue::RefreshItemModal(std::shared_ptr<CFileItem> item,
                                          bool forceRefresh /* = true */,
                                          bool refreshAll /* = false */)
{
  // we can't perform a modal item refresh if other jobs are running
  if (IsRunning())
    return false;

  m_modal = true;
  CVideoLibraryRefreshingJob refreshingJob(std::move(item), forceRefresh, refreshAll);

  bool result = refreshingJob.DoModal();
  m_modal = false;

  return result;
}

void CVideoLibraryQueue::MarkAsWatched(const std::shared_ptr<CFileItem>& item, bool watched)
{
  if (item == NULL)
    return;

  AddJob(new CVideoLibraryMarkWatchedJob(item, watched));
}

void CVideoLibraryQueue::ResetResumePoint(const std::shared_ptr<CFileItem>& item)
{
  if (item == nullptr)
    return;

  AddJob(new CVideoLibraryResetResumePointJob(item));
}

void CVideoLibraryQueue::AddJob(CVideoLibraryJob *job)
{
  if (job == NULL)
    return;

  std::unique_lock<CCriticalSection> lock(m_critical);
  if (!CJobQueue::AddJob(job))
    return;

  // add the job to our list of queued/running jobs
  std::string jobType = job->GetType();
  VideoLibraryJobMap::iterator jobsIt = m_jobs.find(jobType);
  if (jobsIt == m_jobs.end())
  {
    VideoLibraryJobs jobs;
    jobs.insert(job);
    m_jobs.insert(std::make_pair(jobType, jobs));
  }
  else
    jobsIt->second.insert(job);
}

void CVideoLibraryQueue::CancelJob(CVideoLibraryJob *job)
{
  if (job == NULL)
    return;

  std::unique_lock<CCriticalSection> lock(m_critical);
  // remember the job type needed later because the job might be deleted
  // in the call to CJobQueue::CancelJob()
  std::string jobType;
  if (job->GetType() != NULL)
    jobType = job->GetType();

  // check if the job supports cancellation and cancel it
  if (job->CanBeCancelled())
    job->Cancel();

  // remove the job from the job queue
  CJobQueue::CancelJob(job);

  // remove the job from our list of queued/running jobs
  VideoLibraryJobMap::iterator jobsIt = m_jobs.find(jobType);
  if (jobsIt != m_jobs.end())
    jobsIt->second.erase(job);
}

void CVideoLibraryQueue::CancelAllJobs()
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  CJobQueue::CancelJobs();

  // remove all scanning jobs
  m_jobs.clear();
}

bool CVideoLibraryQueue::IsRunning() const
{
  return CJobQueue::IsProcessing() || m_modal;
}

void CVideoLibraryQueue::Refresh()
{
  CUtil::DeleteVideoDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CVideoLibraryQueue::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (success)
  {
    if (QueueEmpty())
      Refresh();
  }

  {
    std::unique_lock<CCriticalSection> lock(m_critical);
    // remove the job from our list of queued/running jobs
    VideoLibraryJobMap::iterator jobsIt = m_jobs.find(job->GetType());
    if (jobsIt != m_jobs.end())
      jobsIt->second.erase(static_cast<CVideoLibraryJob*>(job));
  }

  return CJobQueue::OnJobComplete(jobID, success, job);
}
