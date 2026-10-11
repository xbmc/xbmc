/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaLibraryEvent.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "music/MusicDbPaths.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoDbPaths.h"

CMediaLibraryEvent::CMediaLibraryEvent(KODI::MEDIA::TYPE mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, EventLevel level /* = EventLevel::Information */)
  : CUniqueEvent(label, description, level),
    m_mediaType(mediaType),
    m_mediaPath(mediaPath)
{ }

CMediaLibraryEvent::CMediaLibraryEvent(KODI::MEDIA::TYPE mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level /* = EventLevel::Information */)
  : CUniqueEvent(label, description, icon, details, level),
    m_mediaType(mediaType),
    m_mediaPath(mediaPath)
{ }

std::string CMediaLibraryEvent::GetExecutionLabel() const
{
  std::string executionLabel = CUniqueEvent::GetExecutionLabel();
  if (!executionLabel.empty())
    return executionLabel;

  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(24140);
}

bool CMediaLibraryEvent::Execute() const
{
  if (!CanExecute())
    return false;

  int windowId = -1;
  std::string path = m_mediaPath;
  if (m_mediaType == KODI::MEDIA::TYPE::VIDEO || m_mediaType == KODI::MEDIA::TYPE::MOVIE || m_mediaType == KODI::MEDIA::TYPE::VIDEO_COLLECTION ||
      m_mediaType == KODI::MEDIA::TYPE::TV_SHOW || m_mediaType == KODI::MEDIA::TYPE::SEASON || m_mediaType == KODI::MEDIA::TYPE::EPISODE ||
      m_mediaType == KODI::MEDIA::TYPE::MUSIC_VIDEO)
  {
    if (path.empty())
    {
      if (m_mediaType == KODI::MEDIA::TYPE::VIDEO)
        path = "sources://video/";
      else if (m_mediaType == KODI::MEDIA::TYPE::MOVIE)
        path = KODI::VIDEO::DB_PATH::MOVIE_TITLES;
      else if (m_mediaType == KODI::MEDIA::TYPE::VIDEO_COLLECTION)
        path = KODI::VIDEO::DB_PATH::MOVIE_SETS;
      else if (m_mediaType == KODI::MEDIA::TYPE::MUSIC_VIDEO)
        path = KODI::VIDEO::DB_PATH::MUSICVIDEO_TITLES;
      else if (m_mediaType == KODI::MEDIA::TYPE::TV_SHOW || m_mediaType == KODI::MEDIA::TYPE::SEASON || m_mediaType == KODI::MEDIA::TYPE::EPISODE)
        path = KODI::VIDEO::DB_PATH::TVSHOW_TITLES;
    }
    else
    {
      //! @todo remove the filename for now as CGUIMediaWindow::GetDirectory() can't handle it
      if (m_mediaType == KODI::MEDIA::TYPE::MOVIE || m_mediaType == KODI::MEDIA::TYPE::MUSIC_VIDEO || m_mediaType == KODI::MEDIA::TYPE::EPISODE)
        path = URIUtils::GetDirectory(path);
    }

    windowId = WINDOW_VIDEO_NAV;
  }
  else if (m_mediaType == KODI::MEDIA::TYPE::MUSIC || m_mediaType == KODI::MEDIA::TYPE::ARTIST ||
           m_mediaType == KODI::MEDIA::TYPE::ALBUM || m_mediaType == KODI::MEDIA::TYPE::SONG)
  {
    if (path.empty())
    {
      if (m_mediaType == KODI::MEDIA::TYPE::MUSIC)
        path = "sources://music/";
      else if (m_mediaType == KODI::MEDIA::TYPE::ARTIST)
        path = KODI::MUSIC::DB_PATH::ARTISTS;
      else if (m_mediaType == KODI::MEDIA::TYPE::ALBUM)
        path = KODI::MUSIC::DB_PATH::ALBUMS;
      else if (m_mediaType == KODI::MEDIA::TYPE::SONG)
        path = KODI::MUSIC::DB_PATH::SONGS;
    }
    else
    {
      //! @todo remove the filename for now as CGUIMediaWindow::GetDirectory() can't handle it
      if (m_mediaType == KODI::MEDIA::TYPE::SONG)
        path = URIUtils::GetDirectory(path);
    }

    windowId = WINDOW_MUSIC_NAV;
  }

  if (windowId < 0)
    return false;

  std::vector<std::string> params;
  params.push_back(path);
  params.emplace_back("return");
  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(windowId, params);
  return true;
}
