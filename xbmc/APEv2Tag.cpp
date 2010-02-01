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

#include "APEv2Tag.h"


using namespace MUSIC_INFO;

CAPEv2Tag::CAPEv2Tag()
{
  m_nTrackNum = 0;
  m_nDiscNum = 0;
  m_rating = '0';
}

CAPEv2Tag::~CAPEv2Tag()
{

}

bool CAPEv2Tag::ReadTag(const char* filename)
{
  if (!filename || !m_dll.Load())
    return false;

  // Read in our tag using our dll
  apetag *tag = m_dll.apetag_init();
  m_dll.apetag_read(tag, (char*)filename, 0);
  if (!tag)
    return false;

  if (apefrm_getstr(tag, (char*)"Title"))
    m_strTitle = apefrm_getstr(tag, (char*)"Title");
  if (apefrm_getstr(tag, (char*)"Album"))
    m_strAlbum = apefrm_getstr(tag, (char*)"Album");
  if (apefrm_getstr(tag, (char*)"AlbumArtist"))
    m_strAlbumArtist = apefrm_getstr(tag, (char*)"AlbumArtist");
  if (apefrm_getstr(tag, (char*)"Album Artist"))
    m_strAlbumArtist = apefrm_getstr(tag, (char*)"Album Artist");
  if (apefrm_getstr(tag, (char*)"Artist"))
    m_strArtist = apefrm_getstr(tag, (char*)"Artist");
  if (apefrm_getstr(tag, (char*)"Genre"))
    m_strGenre = apefrm_getstr(tag, (char*)"Genre");
  if (apefrm_getstr(tag, (char*)"Year"))
    m_strYear = apefrm_getstr(tag, (char*)"Year");
  if (apefrm_getstr(tag, (char*)"Track"))
    m_nTrackNum = atoi(apefrm_getstr(tag, (char*)"Track"));
  if (apefrm_getstr(tag, (char*)"Media"))
  {
    // cd number is usually "CD 1/3"
    char *num = apefrm_getstr(tag, (char*)"Media");
    while (!isdigit(*num) && num != '\0') num++;
    if (isdigit(*num))
      m_nDiscNum = atoi(num);
  }
  if (apefrm_getstr(tag, (char*)"Comment"))
    m_strComment = apefrm_getstr(tag, (char*)"Comment");
  if (apefrm_getstr(tag, (char*)"Lyrics"))
    m_strLyrics = apefrm_getstr(tag, (char*)"Lyrics");
  if (apefrm_getstr(tag, (char*)"Rating"))
  { // rating number is usually a single digit, 1-5.  0 is unknown.
      char temp = apefrm_getstr(tag, (char*)"Rating")[0];
      if (temp >= '0' && temp < '6')
        m_rating = temp;
  }

  // Replay gain info
  GetReplayGainFromTag(tag);

  m_dll.apetag_free(tag);
  return true;
}

void CAPEv2Tag::GetReplayGainFromTag(apetag *tag)
{
  if (!tag) return;

  //  foobar2000 saves gain info as lowercase key items
  if (apefrm_getstr(tag, (char*)"replaygain_track_gain"))
  {
    m_replayGain.iTrackGain = (int)(atof(apefrm_getstr(tag, (char*)"replaygain_track_gain"))*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  if (apefrm_getstr(tag, (char*)"replaygain_track_peak"))
  {
    m_replayGain.fTrackPeak = (float)atof(apefrm_getstr(tag, (char*)"replaygain_track_peak"));
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  if (apefrm_getstr(tag, (char*)"replaygain_album_gain"))
  {
    m_replayGain.iAlbumGain = (int)(atof(apefrm_getstr(tag, (char*)"replaygain_album_gain"))*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  if (apefrm_getstr(tag, (char*)"replaygain_album_peak"))
  {
    m_replayGain.fAlbumPeak = (float)atof(apefrm_getstr(tag, (char*)"replaygain_album_peak"));
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }

  // MP3GAIN saves gain info as uppercase key items
  if (apefrm_getstr(tag, (char*)"REPLAYGAIN_TRACK_GAIN"))
  {
    m_replayGain.iTrackGain = (int)(atof(apefrm_getstr(tag, (char*)"REPLAYGAIN_TRACK_GAIN"))*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  if (apefrm_getstr(tag, (char*)"REPLAYGAIN_TRACK_PEAK"))
  {
    m_replayGain.fTrackPeak = (float)atof(apefrm_getstr(tag, (char*)"REPLAYGAIN_TRACK_PEAK"));
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  if (apefrm_getstr(tag, (char*)"REPLAYGAIN_ALBUM_GAIN"))
  {
    m_replayGain.iAlbumGain = (int)(atof(apefrm_getstr(tag, (char*)"REPLAYGAIN_ALBUM_GAIN"))*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  if (apefrm_getstr(tag, (char*)"REPLAYGAIN_ALBUM_PEAK"))
  {
    m_replayGain.fAlbumPeak = (float)atof(apefrm_getstr(tag, (char*)"REPLAYGAIN_ALBUM_PEAK"));
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }
}
