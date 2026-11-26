/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoScannerStreams.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/IDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "jobs/JobManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Thread.h"
#include "utils/StreamDetails.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScannerLookups.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <utility>

namespace KODI::VIDEO
{
namespace
{
//! How long the feeder waits before it looks again at what holds the reads back
constexpr std::chrono::milliseconds WAIT{500};

//! The thread handing the files to the job manager, for as long as it has files to hand
class CFeeder : public CThread
{
public:
  explicit CFeeder(std::function<void()> work)
    : CThread("StreamsAfterScan"),
      m_work(std::move(work))
  {
    Create();
  }
  ~CFeeder() override { StopThread(); }

protected:
  void Process() override { m_work(); }

private:
  const std::function<void()> m_work;
};

//! The video as the scan had it, as far as the probe and the search for its subtitles read it
CFileItem ItemToRead(const StreamsToRead& file)
{
  CFileItem item{file.path, false};
  item.SetDynPath(file.dynPath);
  CVideoInfoTag& tag{*item.GetVideoInfoTag()};
  tag.m_strFileNameAndPath = file.file;
  tag.m_iSeason = file.season;
  tag.m_iEpisode = file.episode;
  return item;
}

//! The folder a video's external subtitles are looked for in
std::string SubtitleFolder(const StreamsToRead& file)
{
  std::string folder;
  std::string name;
  CUtil::GetVideoBasePathAndFileName(ItemToRead(file).GetSubtitleAnchorPath(), folder, name);
  return folder;
}

//! Have the library shown again, as the library's queue does once its jobs are done
void Refresh()
{
  CUtil::DeleteVideoDatabaseDirectoryCache();
  CGUIComponent* gui{CServiceBroker::GetGUI()};
  if (gui)
  {
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    gui->GetWindowManager().SendThreadMessage(message);
  }
  if (const auto announcements{CServiceBroker::GetAnnouncementManager()})
    announcements->Announce(
        ANNOUNCEMENT::VideoLibraryEvent{ANNOUNCEMENT::EVENT::LIBRARY::Refresh{}});
}
} // namespace

void CVideoInfoScannerStreams::Defer(const CFileItem& video) const
{
  const CVideoInfoTag* tag{video.HasVideoInfoTag() ? video.GetVideoInfoTag() : nullptr};
  // Its file is the one a probe opens: the tag's, else the dynamic path
  const bool tagged{tag && !tag->m_strFileNameAndPath.empty()};
  StreamsToRead file{.path = video.GetPath(),
                     .dynPath = video.GetDynPath(),
                     .file = tagged ? tag->m_strFileNameAndPath : video.GetDynPath(),
                     .season = tag ? tag->m_iSeason : -1,
                     .episode = tag ? tag->m_iEpisode : -1};

  std::unique_lock lock(m_lock);
  if (const auto place{m_places.find(file.file)}; place != m_places.end())
    m_deferred[place->second] = std::move(file);
  else
  {
    m_places.emplace(file.file, m_deferred.size());
    m_deferred.push_back(std::move(file));
  }
}

std::vector<StreamsToRead> CVideoInfoScannerStreams::Deferred() const
{
  std::unique_lock lock(m_lock);
  return m_deferred;
}

void CVideoInfoScannerStreams::ReadDeferred()
{
  std::vector<StreamsToRead> files;
  {
    std::unique_lock lock(m_lock);
    files.swap(m_deferred);
    m_places.clear();
  }
  if (files.empty())
    return;

  CLog::Log(LOGINFO,
            "VideoInfoScanner: Reading the streams of {} files once the images queued are cached",
            files.size());
  CStreamsAfterScan::GetInstance().Add(std::move(files));
}

CStreamsAfterScan& CStreamsAfterScan::GetInstance()
{
  static CStreamsAfterScan instance;
  return instance;
}

CStreamsAfterScan::~CStreamsAfterScan()
{
  // Kodi stopped the reads as it exited (CVideoLibraryQueue::CancelAllJobs()), so all that is left
  // is a feeder that may still be giving up
  {
    std::unique_lock lock(m_lock);
    m_stop = true;
  }
  m_changed.notify_all();
  m_feeder.reset();
}

void CStreamsAfterScan::Add(std::vector<StreamsToRead> files)
{
  if (files.empty())
    return;

  std::unique_ptr<CThread> finished;
  {
    std::unique_lock lock(m_lock);
    // Being stopped, or Kodi exiting
    const auto jobManager{CServiceBroker::GetJobManager()};
    if (m_stop || !jobManager || !jobManager->IsRunning())
      return;

    for (StreamsToRead& file : files)
    {
      if (!m_queued.insert(file.file).second)
      {
        // Added again, as the last video of the file has it
        const auto queued{std::ranges::find(m_files, file.file, &StreamsToRead::file)};
        if (queued != m_files.end())
          *queued = std::move(file);
        continue;
      }
      m_files.push_back(std::move(file));
    }

    if (m_feeding)
      return;

    // The feeder that ran out of files before is let go of, once this one is made
    finished = std::move(m_feeder);
    m_feeding = true;
    m_feeder = std::make_unique<CFeeder>([this] { Feed(); });
  }
}

void CStreamsAfterScan::Stop()
{
  std::unique_ptr<CThread> feeder;
  {
    std::unique_lock lock(m_lock);
    if (!m_files.empty())
      CLog::Log(LOGINFO, "VideoInfoScanner: The streams of {} files are left unread",
                m_files.size());
    m_files.clear();
    m_queued.clear();
    m_stop = true;
    feeder = std::move(m_feeder);
  }
  m_changed.notify_all();

  // A read already running finishes, and writes what it read
  m_reads.CancelJobs();
  feeder.reset();

  std::unique_lock lock(m_lock);
  m_stop = false;
}

size_t CStreamsAfterScan::Pending() const
{
  std::unique_lock lock(m_lock);
  return m_files.size();
}

bool CStreamsAfterScan::IsReading() const
{
  return m_reads.IsProcessing();
}

bool CStreamsAfterScan::IsIdle() const
{
  std::unique_lock lock(m_lock);
  return !m_feeding;
}

bool CStreamsAfterScan::Stopping() const
{
  std::unique_lock lock(m_lock);
  return m_stop;
}

bool CStreamsAfterScan::WaitForTurn()
{
  while (!Stopping())
  {
    // Kodi is exiting
    const auto jobManager{CServiceBroker::GetJobManager()};
    if (!jobManager || !jobManager->IsRunning())
      return false;

    // After the scans, and after the images queued, those the scans queued among them
    const auto textureCache{CServiceBroker::GetTextureCache()};
    if (!CScanProbe::Scanning() && (!textureCache || !textureCache->IsCaching()))
      return true;

    std::unique_lock lock(m_lock);
    m_changed.wait_for(lock, WAIT, [this] { return m_stop; });
  }
  return false;
}

bool CStreamsAfterScan::WaitForRead()
{
  // A read waits while playback pauses its priority. Stop() cancels it, which ends the wait.
  while (!m_reads.WaitForCompletion(WAIT))
  {
    if (Stopping())
      return false;
  }
  return true;
}

void CStreamsAfterScan::Feed()
{
  const auto start{std::chrono::steady_clock::now()};
  struct Run
  {
    CFolderListings listings;
    std::atomic<size_t> written{0};
  };
  // Shared with the reads, as one may outlive the feeder once it is stopped
  const auto run{std::make_shared<Run>()};
  std::string folder;
  size_t handed{0};
  bool done{false};

  while (WaitForTurn())
  {
    StreamsToRead file;
    {
      std::unique_lock lock(m_lock);
      if (m_files.empty())
      {
        // Files added from now on are read by a feeder of their own
        m_feeding = false;
        done = true;
        break;
      }
      file = std::move(m_files.front());
      m_files.pop_front();
      m_queued.erase(file.file);
    }

    // What was listed of a folder for subtitles is let go of once its files are read, as theirs are
    // read in the order the scan added them
    if (std::string subtitleFolder{SubtitleFolder(file)}; subtitleFolder != folder)
    {
      if (!folder.empty())
        run->listings.Forget(folder);
      folder = std::move(subtitleFolder);
    }

    // One file at a time, each a job, so that no other starts while a video plays
    m_reads.Submit(
        [file = std::move(file), run]
        {
          if (Read(file, &run->listings))
            ++run->written;
        });
    ++handed;
    if (!WaitForRead())
      break;
  }

  if (!done)
  {
    std::unique_lock lock(m_lock);
    m_feeding = false;
  }

  const auto took{std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start)};
  CLog::Log(LOGINFO, "VideoInfoScanner: {} the streams of {} files, {} of them written, in {} ms",
            done ? "Read" : "Stopped reading", handed, run->written.load(), took.count());

  if (done && run->written > 0)
    Refresh();
}

bool CStreamsAfterScan::Read(const StreamsToRead& file, CFolderListings* listings /* = nullptr */)
{
  // As the scan would have, unless the user no longer wants a video's file probed
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS))
    return false;

  CVideoDatabase database;
  if (!database.Open())
    return false;

  // Read meanwhile, as the thumb loader reads those of a video it shows
  if (CStreamDetails stored; database.GetStreamDetails(file.file, stored))
  {
    CLog::LogF(LOGDEBUG, "Filestream details of video file {} already read",
               CURL::GetRedacted(file.file));
    return false;
  }

  CFileItem item{ItemToRead(file)};
  {
    // After the probes of a scan running meanwhile, whose videos it needs sooner
    const CVideoLookups::CPlace last{std::numeric_limits<size_t>::max() - 1};
    CScanProbe::GetFileStreamDetails(&item, XFILE::DIR_FLAG_DEFAULTS, nullptr, listings);
  }

  // A file whose own streams can't be read may still have external subtitles, which are written
  const CVideoInfoTag& tag{*item.GetVideoInfoTag()};
  if (!tag.HasStreamDetails())
  {
    CLog::LogF(LOGDEBUG, "No filestream details extracted from video file {}",
               CURL::GetRedacted(file.file));
    return false;
  }

  // Gone from the library meanwhile, as a clean removes it
  const int idFile{database.GetFileId(item)};
  if (idFile < 0)
    return false;

  database.BeginTransaction();
  if (!database.SetStreamDetailsForFileId(tag.m_streamDetails, idFile))
  {
    database.RollbackTransaction();
    return false;
  }
  if (!database.CommitTransaction())
    return false;

  CLog::LogF(LOGDEBUG, "Extracted filestream details from video file {}",
             CURL::GetRedacted(file.file));
  return true;
}
} // namespace KODI::VIDEO
