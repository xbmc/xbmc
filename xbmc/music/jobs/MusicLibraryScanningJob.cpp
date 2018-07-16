/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicLibraryScanningJob.h"
#include "music/MusicDatabase.h"

CMusicLibraryScanningJob::CMusicLibraryScanningJob(const std::string& directory, int flags, bool showProgress /* = true */)
  : m_scanner(),
    m_directory(directory),
    m_showProgress(showProgress),
    m_flags(flags)
{ }

CMusicLibraryScanningJob::~CMusicLibraryScanningJob() = default;

bool CMusicLibraryScanningJob::Cancel()
{
  if (!m_scanner.IsScanning())
    return true;

  m_scanner.Stop();
  return true;
}

bool CMusicLibraryScanningJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CMusicLibraryScanningJob* scanningJob = dynamic_cast<const CMusicLibraryScanningJob*>(job);
  if (scanningJob == nullptr)
    return false;

  return m_directory == scanningJob->m_directory &&
         m_flags == scanningJob->m_flags;
}

bool CMusicLibraryScanningJob::Work(CMusicDatabase &db)
{
  m_scanner.ShowDialog(m_showProgress);
  if (m_flags & MUSIC_INFO::CMusicInfoScanner::SCAN_ALBUMS)
    // Scrape additional album information
    m_scanner.FetchAlbumInfo(m_directory, m_flags & MUSIC_INFO::CMusicInfoScanner::SCAN_RESCAN);
  else if (m_flags & MUSIC_INFO::CMusicInfoScanner::SCAN_ARTISTS)
    // Scrape additional artist information
    m_scanner.FetchArtistInfo(m_directory, m_flags & MUSIC_INFO::CMusicInfoScanner::SCAN_RESCAN);
  else
    // Scan tags from music files, and optionally scrape artist and album info
    m_scanner.Start(m_directory, m_flags);

  return true;
}
