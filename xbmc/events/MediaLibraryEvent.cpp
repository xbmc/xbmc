/*
 *      Copyright (C) 2015 Team Kodi
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

#include "MediaLibraryEvent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "utils/URIUtils.h"

CMediaLibraryEvent::CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, EventLevel level /* = EventLevel::Information */)
  : CUniqueEvent(label, description, level),
    m_mediaType(mediaType),
    m_mediaPath(mediaPath)
{ }

CMediaLibraryEvent::CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, EventLevel level /* = EventLevel::Information */)
  : CUniqueEvent(label, description, icon, level),
    m_mediaType(mediaType),
    m_mediaPath(mediaPath)
{ }

CMediaLibraryEvent::CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level /* = EventLevel::Information */)
  : CUniqueEvent(label, description, icon, details, level),
    m_mediaType(mediaType),
    m_mediaPath(mediaPath)
{ }

CMediaLibraryEvent::CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, const CVariant& executionLabel, EventLevel level /* = EventLevel::Information */)
  : CUniqueEvent(label, description, icon, details, executionLabel, level),
    m_mediaType(mediaType),
    m_mediaPath(mediaPath)
{ }

std::string CMediaLibraryEvent::GetExecutionLabel() const
{
  std::string executionLabel = CUniqueEvent::GetExecutionLabel();
  if (!executionLabel.empty())
    return executionLabel;

  return g_localizeStrings.Get(24140);
}

bool CMediaLibraryEvent::Execute() const
{
  if (!CanExecute())
    return false;

  int windowId = -1;
  std::string path = m_mediaPath;
  if (m_mediaType == MediaTypeVideo || m_mediaType == MediaTypeMovie || m_mediaType == MediaTypeVideoCollection ||
      m_mediaType == MediaTypeTvShow || m_mediaType == MediaTypeSeason || m_mediaType == MediaTypeEpisode ||
      m_mediaType == MediaTypeMusicVideo)
  {
    if (path.empty())
    {
      if (m_mediaType == MediaTypeVideo)
        path = "sources://video/";
      else if (m_mediaType == MediaTypeMovie)
        path = "videodb://movies/titles/";
      else if (m_mediaType == MediaTypeVideoCollection)
        path = "videodb://movies/sets/";
      else if (m_mediaType == MediaTypeMusicVideo)
        path = "videodb://musicvideos/titles/";
      else if (m_mediaType == MediaTypeTvShow || m_mediaType == MediaTypeSeason || m_mediaType == MediaTypeEpisode)
        path = "videodb://tvshows/titles/";
    }
    else
    {
      //! @todo remove the filename for now as CGUIMediaWindow::GetDirectory() can't handle it
      if (m_mediaType == MediaTypeMovie || m_mediaType == MediaTypeMusicVideo || m_mediaType == MediaTypeEpisode)
        path = URIUtils::GetDirectory(path);
    }

    windowId = WINDOW_VIDEO_NAV;
  }
  else if (m_mediaType == MediaTypeMusic || m_mediaType == MediaTypeArtist ||
           m_mediaType == MediaTypeAlbum || m_mediaType == MediaTypeSong)
  {
    if (path.empty())
    {
      if (m_mediaType == MediaTypeMusic)
        path = "sources://music/";
      else if (m_mediaType == MediaTypeArtist)
        path = "musicdb://artists/";
      else if (m_mediaType == MediaTypeAlbum)
        path = "musicdb://albums/";
      else if (m_mediaType == MediaTypeSong)
        path = "musicdb://songs/";
    }
    else
    {
      //! @todo remove the filename for now as CGUIMediaWindow::GetDirectory() can't handle it
      if (m_mediaType == MediaTypeSong)
        path = URIUtils::GetDirectory(path);
    }

    windowId = WINDOW_MUSIC_NAV;
  }

  if (windowId < 0)
    return false;

  std::vector<std::string> params;
  params.push_back(path);
  params.push_back("return");
  g_windowManager.ActivateWindow(windowId, params);
  return true;
}
