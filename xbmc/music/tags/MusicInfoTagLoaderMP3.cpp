/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "MusicInfoTagLoaderMP3.h"
#include "APEv2Tag.h"
#include "Id3Tag.h"
#include "settings/AdvancedSettings.h"
#include "filesystem/File.h"
#include "utils/log.h"

using namespace MUSIC_INFO;



using namespace MUSIC_INFO;
using namespace XFILE;

CMusicInfoTagLoaderMP3::CMusicInfoTagLoaderMP3(void)
{

}

CMusicInfoTagLoaderMP3::~CMusicInfoTagLoaderMP3()
{
}

bool CMusicInfoTagLoaderMP3::Load(const CStdString& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  try
  {
    // retrieve the ID3 Tag info from strFileName
    // and put it in tag
    CID3Tag id3tag;
    id3tag.SetArt(art);
    if (id3tag.Read(strFileName))
    {
      id3tag.GetMusicInfoTag(tag);
      m_replayGainInfo=id3tag.GetReplayGain();
    }

    // Check for an APEv2 tag
    CAPEv2Tag apeTag;
    if (PrioritiseAPETags() && apeTag.ReadTag(strFileName.c_str()))
    { // found - let's copy over the additional info (if any)
      if (apeTag.GetArtist().size())
      {
        tag.SetArtist(apeTag.GetArtist());
        tag.SetLoaded();
      }
      if (apeTag.GetAlbum().size())
      {
        tag.SetAlbum(apeTag.GetAlbum());
        tag.SetLoaded();
      }
      if (apeTag.GetAlbumArtist().size())
      {
        tag.SetAlbumArtist(apeTag.GetAlbumArtist());
        tag.SetLoaded();
      }
      if (apeTag.GetTitle().size())
      {
        tag.SetTitle(apeTag.GetTitle());
        tag.SetLoaded();
      }
      if (apeTag.GetGenre().size())
        tag.SetGenre(apeTag.GetGenre());

      if (apeTag.GetLyrics().size())
        tag.SetLyrics(apeTag.GetLyrics());

      if (apeTag.GetYear().size())
      {
        SYSTEMTIME time;
        ZeroMemory(&time, sizeof(SYSTEMTIME));
        time.wYear = atoi(apeTag.GetYear().c_str());
        tag.SetReleaseDate(time);
      }
      if (apeTag.GetTrackNum())
        tag.SetTrackNumber(apeTag.GetTrackNum());
      if (apeTag.GetDiscNum())
        tag.SetPartOfSet(apeTag.GetDiscNum());
      if (apeTag.GetComment().size())
        tag.SetComment(apeTag.GetComment());
      if (apeTag.GetReplayGain().iHasGainInfo)
        m_replayGainInfo = apeTag.GetReplayGain();
      if (apeTag.GetRating() > '0')
        tag.SetRating(apeTag.GetRating());
      tag.SetCompilation(apeTag.GetCompilation());
    }

//    tag.SetDuration(ReadDuration(strFileName));

    return tag.Loaded();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader mp3: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}

bool CMusicInfoTagLoaderMP3::PrioritiseAPETags() const
{
  return g_advancedSettings.m_prioritiseAPEv2tags;
}


