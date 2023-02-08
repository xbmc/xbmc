/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/LibExportSettings.h"
#include "threads/CriticalSection.h"
#include "utils/JobManager.h"

#include <map>
#include <set>

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
  \brief Enqueue a music library import job.
  \param[in] xmlFile    xml file to import
  \param[in] showDialog Show a progress dialog while (asynchronously) exporting, otherwise export in synchronous
  */
  void ImportLibrary(const std::string& xmlFile, bool showDialog = false);

  /*!
   \brief Enqueue a music library update job, scanning tags embedded in music files and optionally scraping additional data.
   \param[in] strDirectory Directory to scan or "" (empty string) for a global scan.
   \param[in] flags Flags for controlling the scanning process.  See xbmc/music/infoscanner/MusicInfoScanner.h for possible values.
   \param[in] showProgress Whether or not to show a progress dialog. Defaults to true
   */
  void ScanLibrary(const std::string& strDirectory, int flags = 0, bool showProgress = true);

  /*!
   \brief Enqueue an album scraping job fetching additional album data.
   \param[in] strDirectory Virtual path that identifies which albums to process or "" (empty string) for all albums
   \param[in] refresh Whether or not to refresh data for albums that have previously been scraped
  */
  void StartAlbumScan(const std::string& strDirectory, bool refresh = false);

  /*!
   \brief Enqueue an artist scraping job fetching additional artist data.
   \param[in] strDirectory Virtual path that identifies which artists to process or "" (empty string) for all artists
   \param[in] refresh Whether or not to refresh data for artists that have previously been scraped
   */
  void StartArtistScan(const std::string& strDirectory, bool refresh = false);

  /*!
   \brief Check if a library scan or cleaning is in progress.
   \return True if a scan or clean is in progress, false otherwise
   */
  bool IsScanningLibrary() const;

  /*!
   \brief Stop and dequeue all scanning jobs.
   */
  void StopLibraryScanning();

  /*!
   \brief Enqueue an asynchronous library cleaning job.
   \param[in] showDialog Show a model progress dialog while cleaning. Default is false.
   */
  void CleanLibrary(bool showDialog = false);

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

  bool m_modal = false;
  bool m_cleaning = false;
};
