
#include "stdafx.h"
#include "vorbistag.h"


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
    if (tag.GetArtist().GetLength())
    {
      CStdString strArtist=tag.GetArtist() + " / " + strTagValue;
      tag.SetArtist(strArtist);
    }
    else
      tag.SetArtist(strTagValue);
  }

  if ( strTagType == "TITLE" )
  {
    tag.SetTitle(strTagValue);
    tag.SetLoaded();
  }

  if ( strTagType == "ALBUM" )
  {
    tag.SetAlbum(strTagValue);
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
    if (tag.GetGenre().GetLength())
    {
      CStdString strGenre=tag.GetGenre() + " / " + strTagValue;
      tag.SetGenre(strGenre);
    }
    else
      tag.SetGenre(strTagValue);
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
    CStdString strValue=strTagEntry.Mid( nPos + 1 );
    g_charsetConverter.utf8ToStringCharset(strValue, strTagValue);
    strTagType = strTagEntry.Left( nPos );
    strTagType.ToUpper();
  }
}
