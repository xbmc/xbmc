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
#include "CueInfoLoader.h"
#include "music/tags/MusicInfoTag.h"

void CueInfoLoader::Load(const std::string& aStrCuesheet, CFileItemPtr aFileItem)
{
  if (aStrCuesheet.empty())
    return;
  
  std::string p = aFileItem->GetPath();
  std::string songPath = aFileItem->GetMusicInfoTag()->GetURL();
  CCueDocument& doc = m_cache[songPath];
  if (!doc.IsLoaded())
  {
    if (doc.ParseTag(aStrCuesheet))
    {
      std::vector<std::string> MediaFileVec;
      doc.GetMediaFiles(MediaFileVec);
      for (std::vector<std::string>::iterator itMedia = MediaFileVec.begin(); itMedia != MediaFileVec.end(); itMedia++)
        doc.UpdateMediaFile(*itMedia, songPath);
    }
  }

  CSong song;
  if (doc.GetSong(aFileItem->GetMusicInfoTag()->GetTrackNumber(), song))
  {
    aFileItem->GetMusicInfoTag()->SetReplayGain(song.replayGain);
  }
}


