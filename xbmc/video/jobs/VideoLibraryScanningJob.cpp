/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryScanningJob.h"
#include "video/VideoDatabase.h"

CVideoLibraryScanningJob::CVideoLibraryScanningJob(const std::string& directory, bool scanAll /* = false */, bool showProgress /* = true */)
  : m_scanner(),
    m_directory(directory),
    m_showProgress(showProgress),
    m_scanAll(scanAll)
{ }

CVideoLibraryScanningJob::~CVideoLibraryScanningJob() = default;

bool CVideoLibraryScanningJob::Cancel()
{
  if (!m_scanner.IsScanning())
    return true;

  m_scanner.Stop();
  return true;
}

bool CVideoLibraryScanningJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CVideoLibraryScanningJob* scanningJob = dynamic_cast<const CVideoLibraryScanningJob*>(job);
  if (scanningJob == NULL)
    return false;

  return m_directory == scanningJob->m_directory &&
         m_scanAll == scanningJob->m_scanAll;
}

bool CVideoLibraryScanningJob::Work(CVideoDatabase &db)
{
  m_scanner.ShowDialog(m_showProgress);
  m_scanner.Start(m_directory, m_scanAll);

  return true;
}
