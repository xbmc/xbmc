/*
 *      Copyright (C) 2018 Team XBMC
 *      http://kodi.tv
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

#include "VideoLibrarySetFileItem.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoThumbLoader.h"

using namespace KODI::MESSAGING;

CVideoLibrarySetFileItemJob::CVideoLibrarySetFileItemJob(CFileItem item)
{
  m_item = item;
}

bool CVideoLibrarySetFileItemJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CVideoLibrarySetFileItemJob* resetJob = dynamic_cast<const CVideoLibrarySetFileItemJob*>(job);
  if (!resetJob)
    return false;

  return m_item.IsSamePath(&resetJob->m_item);
}

bool CVideoLibrarySetFileItemJob::Work(CVideoDatabase &db)
{
  std::string path = m_item.GetPath();
  std::string videoInfoTagPath(m_item.GetVideoInfoTag()->m_strFileNameAndPath);
  if (videoInfoTagPath.find("removable://") == 0)
    path = videoInfoTagPath;
  db.LoadVideoInfo(path, *m_item.GetVideoInfoTag());

  // Find a thumb for this file.
  if (!m_item.HasArt("thumb"))
  {
    CVideoThumbLoader loader;
    loader.LoadItem(&m_item);
  }

  // find a thumb for this stream
  if (m_item.IsInternetStream())
  {
    // else its a video
    if (!g_application.m_strPlayListFile.empty())
    {
      CLog::Log(LOGDEBUG, "Streaming media detected... using %s to find a thumb",
                           g_application.m_strPlayListFile.c_str());
      CFileItem thumbItem(g_application.m_strPlayListFile,false);

      CVideoThumbLoader loader;
      if (loader.FillThumb(thumbItem))
        m_item.SetArt("thumb", thumbItem.GetArt("thumb"));
    }
  }

  m_item.FillInDefaultIcon();
  db.Close();

  CApplicationMessenger::GetInstance().PostMsg(TMSG_UPDATE_PLAYING_ITEM, 1,-1,
                                               static_cast<void*>(new CFileItem(m_item)));
  return true;
}
