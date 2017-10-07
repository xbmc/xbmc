/*
 *      Copyright (C) 2017 Team XBMC
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

#include "MusicLibraryQueue.h"

#include <utility>

#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "music/jobs/MusicLibraryExportJob.h"
#include "music/jobs/MusicLibraryJob.h"
#include "threads/SingleLock.h"
#include "Util.h"
#include "utils/Variant.h"

CMusicLibraryQueue::CMusicLibraryQueue()
  : CJobQueue(false, 1, CJob::PRIORITY_LOW),
    m_jobs(),
    m_modal(false),
    m_exporting(false)
{ }

CMusicLibraryQueue::~CMusicLibraryQueue()
{
  CSingleLock lock(m_critical);
  m_jobs.clear();
}

CMusicLibraryQueue& CMusicLibraryQueue::GetInstance()
{
  static CMusicLibraryQueue s_instance;
  return s_instance;
}

void CMusicLibraryQueue::ExportLibrary(const CLibExportSettings& settings, bool showDialog /* = false */)
{
  CGUIDialogProgress* progress = NULL;
  if (showDialog)
  {
    progress = g_windowManager.GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->SetHeading(CVariant{ 20196 }); //"Export music library"
      progress->SetText(CVariant{ 650 });   //"Exporting"
      progress->SetPercentage(0);
      progress->Open();
      progress->ShowProgressBar(true);
    }
  }

  CMusicLibraryExportJob* exportJob = new CMusicLibraryExportJob(settings, progress);
  if (showDialog)
  {    
    AddJob(exportJob);

    if (progress)
    {
      // Render and wait for export to complete or be cancelled
      while (progress->IsActive() && !progress->IsCanceled())
        progress->Progress();
      // Finally close progress dialog
      if (progress->IsActive())
        progress->Close();
    }
  }
  else
  {
    m_modal = true;
    exportJob->DoWork();

    delete exportJob;
    m_modal = false;
    Refresh();
  }
}

void CMusicLibraryQueue::AddJob(CMusicLibraryJob *job)
{
  if (job == NULL)
    return;

  CSingleLock lock(m_critical);
  if (!CJobQueue::AddJob(job))
    return;

  // add the job to our list of queued/running jobs
  std::string jobType = job->GetType();
  MusicLibraryJobMap::iterator jobsIt = m_jobs.find(jobType);
  if (jobsIt == m_jobs.end())
  {
    MusicLibraryJobs jobs;
    jobs.insert(job);
    m_jobs.insert(std::make_pair(jobType, jobs));
  }
  else
    jobsIt->second.insert(job);
}

void CMusicLibraryQueue::CancelJob(CMusicLibraryJob *job)
{
  if (job == NULL)
    return;

  CSingleLock lock(m_critical);
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
  MusicLibraryJobMap::iterator jobsIt = m_jobs.find(jobType);
  if (jobsIt != m_jobs.end())
    jobsIt->second.erase(job);
}

void CMusicLibraryQueue::CancelAllJobs()
{
  CSingleLock lock(m_critical);
  CJobQueue::CancelJobs();

  // remove all scanning jobs
  m_jobs.clear();
}

bool CMusicLibraryQueue::IsRunning() const
{
  return CJobQueue::IsProcessing() || m_modal;
}

void CMusicLibraryQueue::Refresh()
{
  CUtil::DeleteMusicDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  g_windowManager.SendThreadMessage(msg);
}

void CMusicLibraryQueue::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (success)
  {
    if (QueueEmpty())
      Refresh();
  }

  {
    CSingleLock lock(m_critical);
    // remove the job from our list of queued/running jobs
    MusicLibraryJobMap::iterator jobsIt = m_jobs.find(job->GetType());
    if (jobsIt != m_jobs.end())
      jobsIt->second.erase(static_cast<CMusicLibraryJob*>(job));
  }

  return CJobQueue::OnJobComplete(jobID, success, job);
}
