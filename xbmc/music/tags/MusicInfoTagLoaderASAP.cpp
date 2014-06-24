/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "MusicInfoTagLoaderASAP.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "MusicInfoTag.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderASAP::CMusicInfoTagLoaderASAP()
{
}

CMusicInfoTagLoaderASAP::~CMusicInfoTagLoaderASAP()
{
}

bool CMusicInfoTagLoaderASAP::Load(const std::string &strFile, CMusicInfoTag &tag, EmbeddedArt *art)
{
  tag.SetLoaded(false);

  if (!m_dll.Load())
    return false;

  std::string strFileToLoad = strFile;
  int song = -1;
  std::string strExtension = URIUtils::GetExtension(strFile);
  if (StringUtils::EqualsNoCase(strExtension, ".asapstream"))
  {
    std::string strFileName = URIUtils::GetFileName(strFile);
    size_t iStart = strFileName.rfind('-') + 1;
    song = atoi(strFileName.substr(iStart, strFileName.size() - iStart - 11).c_str()) - 1;
    strFileToLoad = URIUtils::GetDirectory(strFile);
    URIUtils::RemoveSlashAtEnd(strFileToLoad);
  }

  ASAP_SongInfo songInfo;
  if (!m_dll.asapGetInfo(strFileToLoad.c_str(), song, &songInfo))
    return false;

  tag.SetURL(strFileToLoad);
  if (songInfo.name[0] != '\0')
    tag.SetTitle(songInfo.name);
  if (songInfo.author[0] != '\0')
    tag.SetArtist(songInfo.author);
  if (song >= 0)
    tag.SetTrackNumber(song + 1);
  if (songInfo.duration >= 0)
    tag.SetDuration(songInfo.duration / 1000);
  if (songInfo.year > 0)
  {
    SYSTEMTIME dateTime = { (WORD)songInfo.year, (WORD)songInfo.month, 0, 
                            (WORD)songInfo.day, 0, 0, 0, 0 };
    tag.SetReleaseDate(dateTime);
  }
  tag.SetLoaded(true);
  return true;
}
