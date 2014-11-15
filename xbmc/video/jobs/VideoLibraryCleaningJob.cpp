/*
 *      Copyright (C) 2014 Team XBMC
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

#include "VideoLibraryCleaningJob.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "video/VideoDatabase.h"

using namespace std;

CVideoLibraryCleaningJob::CVideoLibraryCleaningJob(const std::set<int>& paths /* = std::set<int>() */, bool showDialog /* = false */)
  : CVideoLibraryProgressJob(NULL),
    m_paths(paths),
    m_showDialog(showDialog)
{ }

CVideoLibraryCleaningJob::CVideoLibraryCleaningJob(const std::set<int>& paths, CGUIDialogProgressBarHandle* progressBar)
  : CVideoLibraryProgressJob(progressBar),
    m_paths(paths),
    m_showDialog(false)
{ }

CVideoLibraryCleaningJob::~CVideoLibraryCleaningJob()
{ }

bool CVideoLibraryCleaningJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CVideoLibraryCleaningJob* cleaningJob = dynamic_cast<const CVideoLibraryCleaningJob*>(job);
  if (cleaningJob == NULL)
    return false;

  return m_paths == cleaningJob->m_paths &&
         m_showDialog == cleaningJob->m_showDialog;
}

bool CVideoLibraryCleaningJob::Work(CVideoDatabase &db)
{
  db.CleanDatabase(GetProgressBar(), m_paths, m_showDialog);
  return true;
}
