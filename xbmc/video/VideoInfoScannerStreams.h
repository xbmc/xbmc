/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/JobQueue.h"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CFileItem;
class CFolderListings;
class CThread;

namespace KODI::VIDEO
{
//! \brief The file of a video a scan added without its streams, to be probed once it is done
struct StreamsToRead
{
  //! The video's path and dynamic path, by which its external subtitles are looked for
  std::string path;
  std::string dynPath;
  //! The file probed, under which the library holds the video
  std::string file;
  //! The season and episode of an episode, as its own subtitles are named by them
  int season{-1};
  int episode{-1};
};

/*!
 \brief The streams of the videos a scan adds, which it reads by probing their files as it adds
 them, unless videolibrary/extractstreamdetailsafterscrape says to read them once it is done
 (CStreamsAfterScan). Streams an nfo gives, or a bluray's playlist, are written as the scan adds the
 video either way.

 One belongs to a scan, and lives as long as it does.
 */
class CVideoInfoScannerStreams
{
public:
  //! \brief Whether the scan leaves the probes to once it is done. Only a scan does, not a refresh.
  void ReadAfterScan(bool afterScan) { m_afterScan = afterScan; }
  bool ReadsAfterScan() const { return m_afterScan; }

  /*!
   \brief Leave the probe of a video the scan has added to once it is done. A file of several
   videos, such as the episodes of one file, is probed once, its streams then those of each, and its
   external subtitles those of the last of them added, as when probed as they are added.
   */
  void Defer(const CFileItem& video) const;

  //! \brief The files left to probe so far, in the order the scan added their videos
  std::vector<StreamsToRead> Deferred() const;

  /*!
   \brief Have the files left to probe read in the background (CStreamsAfterScan), once the images
   the scan queued are cached. Called once the scan is done, after its art is queued.
   */
  void ReadDeferred();

private:
  bool m_afterScan{false};
  //! Mutable, as videos are added by const methods too, as their art is
  mutable std::mutex m_lock;
  mutable std::vector<StreamsToRead> m_deferred;
  //! The place of each file in m_deferred
  mutable std::unordered_map<std::string, size_t> m_places;
};

/*!
 \brief Reads the streams of the videos scans added without them, in the background, and writes them
 to the library as the scans would have.

 A file is read once no scan runs and the texture cache has cached the images queued to it, the
 scans' own among them, so that the art comes first. Each file is a job of its own, one at a time,
 at the priority playback pauses, so that no other starts while a video plays. Files a later scan
 adds are read after those already queued, each once.

 What is still queued when Kodi exits or logs off is dropped, as are the files of a stopped scan:
 those videos keep no streams until the video thumb loader reads them as they are shown, as it does
 a video's whose streams are unknown where "Extract video information from files" is on.
 */
class CStreamsAfterScan
{
public:
  static CStreamsAfterScan& GetInstance();
  ~CStreamsAfterScan();
  CStreamsAfterScan(const CStreamsAfterScan&) = delete;
  CStreamsAfterScan& operator=(const CStreamsAfterScan&) = delete;

  //! \brief Read these files after those queued already, a file queued already once
  void Add(std::vector<StreamsToRead> files);
  //! \brief Read no more, dropping the files queued, and wait for the reads to be let go of
  void Stop();

  //! \brief The files queued and not yet handed to the job manager
  size_t Pending() const;
  //! \brief Whether a file's job is with the job manager, waiting for its turn or running
  bool IsReading() const;
  //! \brief Whether every file queued has been read, or dropped
  bool IsIdle() const;

  /*!
   \brief Probe a video's file, unless its streams are known already, and write its streams to the
   library, as the scan adding the video would have
   \param listings the folders listed for subtitles, kept from one file to the next
   \return whether streams were written
   */
  static bool Read(const StreamsToRead& file, CFolderListings* listings = nullptr);

private:
  CStreamsAfterScan() = default;

  //! \brief Hands the files queued to the job manager one at a time, each once it may be read
  void Feed();
  //! \brief Wait for no scan to run and the texture cache to cache what is queued \return false to
  //! read no more
  bool WaitForTurn();
  //! \brief Wait for the file handed to the job manager to be read \return false to read no more
  bool WaitForRead();
  bool Stopping() const;

  mutable std::mutex m_lock;
  std::condition_variable m_changed;
  //! In the order added
  std::deque<StreamsToRead> m_files;
  //! The files of m_files
  std::unordered_set<std::string> m_queued;
  bool m_stop{false};
  //! Whether the feeder is handing files to the job manager, until it has none left
  bool m_feeding{false};
  std::unique_ptr<CThread> m_feeder;
  //! The reads, at the priority playback pauses
  CJobQueue m_reads{false, 1, CJob::PRIORITY_LOW_PAUSABLE};
};
} // namespace KODI::VIDEO
