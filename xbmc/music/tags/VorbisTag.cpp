/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "VorbisTag.h"

using namespace MUSIC_INFO;

CVorbisTag::CVorbisTag()
{

}

CVorbisTag::~CVorbisTag()
{

}

int CVorbisTag::ParseTagEntry(CStdString& strTagEntry)
{
  CStdString strTagValue;
  CStdString strTagType;

  // Split tag entry like ARTIST=Sublime
  SplitEntry( strTagEntry, strTagType, strTagValue);

  // Save tag entry to members

  CMusicInfoTag& tag=m_musicInfoTag;

  if ( strTagType == "ARTIST" )
  {
    tag.AppendArtist(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "ALBUMARTIST" || strTagType == "ALBUM ARTIST" || strTagType == "ENSEMBLE")
  {
    tag.AppendAlbumArtist(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "TITLE" )
  {
    tag.SetTitle(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "ALBUM" )
  {
    tag.SetAlbum(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "TRACKNUMBER" )
  {
    tag.SetTrackNumber(atoi(strTagValue));
  }

  if ( strTagType == "DISCNUMBER" )
  {
    tag.SetPartOfSet(atoi(strTagValue));
  }

  if ( strTagType == "DATE" )
  {
    SYSTEMTIME dateTime;
    dateTime.wYear = atoi(strTagValue);
    tag.SetReleaseDate(dateTime);
  }

  if ( strTagType == "GENRE" )
  {
    tag.AppendGenre(strTagValue);
  }

  if ( strTagType == "MUSICBRAINZ_TRACKID" )
  {
    tag.SetMusicBrainzTrackID(strTagValue);
  }

  if ( strTagType == "MUSICBRAINZ_ARTISTID" )
  {
    tag.SetMusicBrainzArtistID(strTagValue);
  }

  if ( strTagType == "MUSICBRAINZ_ALBUMID" )
  {
    tag.SetMusicBrainzAlbumID(strTagValue);
  }

  if ( strTagType == "MUSICBRAINZ_ALBUMARTISTID" )
  {
    tag.SetMusicBrainzAlbumArtistID(strTagValue);
  }

  if ( strTagType == "MUSICBRAINZ_TRMID" )
  {
    tag.SetMusicBrainzTRMID(strTagValue);
  }

  if ( strTagType == "COMMENT" )
    tag.SetComment(strTagValue);

  if ( strTagType == "LYRICS" )
    tag.SetLyrics(strTagValue);


  if ( strTagType == "RATING" && strTagValue.GetLength() == 1 && strTagValue[0] > '0' && strTagValue[0] < '6')
    tag.SetRating(strTagValue[0]);

  //  Get new style replay gain info
  if (strTagType=="REPLAYGAIN_TRACK_GAIN")
  {
    m_replayGain.iTrackGain = (int)(atof(strTagValue.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  else if (strTagType=="REPLAYGAIN_TRACK_PEAK")
  {
    m_replayGain.fTrackPeak = (float)atof(strTagValue.c_str());
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  else if (strTagType=="REPLAYGAIN_ALBUM_GAIN")
  {
    m_replayGain.iAlbumGain = (int)(atof(strTagValue.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  else if (strTagType=="REPLAYGAIN_ALBUM_PEAK")
  {
    m_replayGain.fAlbumPeak = (float)atof(strTagValue.c_str());
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }

  //  Get old style replay gain info
  if (strTagType=="RG_RADIO")
  {
    m_replayGain.iTrackGain = (int)(atof(strTagValue.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  else if (strTagType=="RG_PEAK")
  {
    m_replayGain.fTrackPeak = (float)atof(strTagValue.c_str());
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  else if (strTagType=="RG_AUDIOPHILE")
  {
    m_replayGain.iAlbumGain = (int)(atof(strTagValue.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  return 0;
}

void CVorbisTag::SplitEntry(const CStdString& strTagEntry, CStdString& strTagType, CStdString& strTagValue)
{
  int nPos = strTagEntry.Find( '=' );

  if ( nPos > -1 )
  {
    // we use UTF-8 internally
    strTagValue = strTagEntry.Mid( nPos + 1 );
    strTagType = strTagEntry.Left( nPos );
    strTagType.ToUpper();
  }
}
