/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "utils/JobManager.h"

#include <map>
#include <memory>
#include <set>

class CFileItem;
class CGUIDialogProgressBarHandle;
class CVideoLibraryJob;

/*!
 \brief Queue for video library jobs.

 The queue can only process a single job at any time and every job will be
 executed at the lowest priority.
 */
class CVideoLibraryQueue : protected CJobQueue
{
public:
  ~CVideoLibraryQueue() override;

  /*!
   \brief Gets the singleton instance of the video library queue.
  */
  static CVideoLibraryQueue& GetInstance();

  /*!
   \brief Enqueue a library scan job.

   \param[in] directory Directory to scan
   \param[in] scanAll Ignore exclude setting for items. Defaults to false
   \param[in] showProgress Whether or not to show a progress dialog. Defaults to true
   */
  void ScanLibrary(const std::string& directory, bool scanAll = false, bool showProgress = true);

  /*!
   \brief Check if a library scan is in progress.

   \return True if a scan is in progress, false otherwise
   */
  bool IsScanningLibrary() const;

  /*!
   \brief Stop and dequeue all scanning jobs.
   */
  void StopLibraryScanning();

  /*!
   \brief Enqueue a library cleaning job.

   \param[in] paths Set with database IDs of paths to be cleaned
   \param[in] asynchronous Run the clean job asynchronously. Defaults to true
   \param[in] progressBar Progress bar to update in GUI. Defaults to NULL (no progress bar to update)
   \return True if the video library cleaning job has started, false otherwise
   */
  bool CleanLibrary(const std::set<int>& paths = std::set<int>(),
                    bool asynchronous = true,
                    CGUIDialogProgressBarHandle* progressBar = NULL);

  /*!
  \brief Executes a library cleaning with a modal dialog.

  \param[in] paths Set with database IDs of paths to be cleaned
  \return True if the video library cleaning job has started, false otherwise
  */
  bool CleanLibraryModal(const std::set<int>& paths = std::set<int>());

  /*!
   \brief Enqueues a job to refresh the details of the given item.

   \param[inout] item Video item to be refreshed
   \param[in] ignoreNfo Whether or not to ignore local NFO files
   \param[in] forceRefresh Whether to force a complete refresh (including NFO or internet lookup)
   \param[in] refreshAll Whether to refresh all sub-items (in case of a tvshow)
   \param[in] searchTitle Title to use for the search (instead of determining it from the item's filename/path)
   */
  void RefreshItem(std::shared_ptr<CFileItem> item,
                   bool ignoreNfo = false,
                   bool forceRefresh = true,
                   bool refreshAll = false,
                   const std::string& searchTitle = "");

  /*!
   \brief Refreshes the details of the given item with a modal dialog.

   \param[inout] item Video item to be refreshed
   \param[in] forceRefresh Whether to force a complete refresh (including NFO or internet lookup)
   \param[in] refreshAll Whether to refresh all sub-items (in case of a tvshow)
   \return True if the item has been successfully refreshed, false otherwise.
  */
  bool RefreshItemModal(std::shared_ptr<CFileItem> item,
                        bool forceRefresh = true,
                        bool refreshAll = false);

  /*!
   \brief Queue a watched status update job.

   \param[in] item Item to update watched status for
   \param[in] watched New watched status
   */
  void MarkAsWatched(const std::shared_ptr<CFileItem>& item, bool watched);

  /*!
   \brief Queue a reset resume point job.

   \param[in] item Item to reset the resume point for
   */
  void ResetResumePoint(const std::shared_ptr<CFileItem>& item);

  /*!
   \brief Adds the given job to the queue.

   \param[in] job Video library job to be queued.
   */
  void AddJob(CVideoLibraryJob *job);

  /*!
   \brief Cancels the given job and removes it from the queue.

   \param[in] job Video library job to be canceled and removed from the queue.
   */
  void CancelJob(CVideoLibraryJob *job);

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
  CVideoLibraryQueue();
  CVideoLibraryQueue(const CVideoLibraryQueue&) = delete;
  CVideoLibraryQueue const& operator=(CVideoLibraryQueue const&) = delete;

  typedef std::set<CVideoLibraryJob*> VideoLibraryJobs;
  typedef std::map<std::string, VideoLibraryJobs> VideoLibraryJobMap;
  VideoLibraryJobMap m_jobs;
  CCriticalSection m_critical;

  bool m_modal = false;
  bool m_cleaning = false;
};
