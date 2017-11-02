#pragma once
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

#include <map>
#include <set>

#include "FileItem.h"
#include "settings/LibExportSettings.h"
#include "threads/CriticalSection.h"
#include "utils/JobManager.h"

class CGUIDialogProgressBarHandle;
class CMusicLibraryJob;
class CGUIDialogProgress;

/*!
 \brief Queue for music library jobs.

 The queue can only process a single job at any time and every job will be
 executed at the lowest priority.
 */
class CMusicLibraryQueue : protected CJobQueue
{
public:
  ~CMusicLibraryQueue() override;

  /*!
   \brief Gets the singleton instance of the music library queue.
  */
  static CMusicLibraryQueue& GetInstance();

  /*!
  \brief Enqueue a music library export job.
  \param[in] settings   Library export settings
  \param[in] showDialog Show a progress dialog while (asynchronously) exporting, otherwise export in synchronous
  */
  void ExportLibrary(const CLibExportSettings& settings, bool showDialog = false);
  
  /*!
   \brief Adds the given job to the queue.
   \param[in] job Music library job to be queued.
   */
  void AddJob(CMusicLibraryJob *job);

  /*!
   \brief Cancels the given job and removes it from the queue.
   \param[in] job Music library job to be canceled and removed from the queue.
   */
  void CancelJob(CMusicLibraryJob *job);

  /*!
   \brief Cancels all running and queued jobs.
   */
  void CancelAllJobs();

  /*!
   \brief Whether any jobs are running or not.
   */
  bool IsRunning() const;

protected:
  // implementation of IJobCallback
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  /*!
   \brief Notifies all to refresh the current listings.
   */
  void Refresh();

private:
  CMusicLibraryQueue();
  CMusicLibraryQueue(const CMusicLibraryQueue&);
  CMusicLibraryQueue const& operator=(CMusicLibraryQueue const&);

  typedef std::set<CMusicLibraryJob*> MusicLibraryJobs;
  typedef std::map<std::string, MusicLibraryJobs> MusicLibraryJobMap;
  MusicLibraryJobMap m_jobs;
  CCriticalSection m_critical;

  bool m_modal;
  bool m_exporting;
};
