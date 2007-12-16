/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "APEv2tag.h"


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

bool CAPEv2Tag::ReadTag(const char* filename, bool checkID3Tag)
{
  if (!filename || !m_dll.Load())
    return false;

  // Read in our tag using our dll
  IAPETag *tag = m_dll.GetAPETag(filename, checkID3Tag);
  if (!tag)
    return false;

  int chars = 256;
  char buffer[256];
  if (tag->GetFieldString(L"Title", buffer, &chars, TRUE) != -1)
    m_strTitle = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Album", buffer, &chars, TRUE) != -1)
    m_strAlbum = buffer;
  chars = 256;
  if (tag->GetFieldString(L"AlbumArtist", buffer, &chars, TRUE) != -1)
    m_strAlbumArtist = buffer;
  chars = 256;  // could also have a space in it
  if (tag->GetFieldString(L"Album Artist", buffer, &chars, TRUE) != -1)
    m_strAlbumArtist = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Artist", buffer, &chars, TRUE) != -1)
    m_strArtist = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Genre", buffer, &chars, TRUE) != -1)
    m_strGenre = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Year", buffer, &chars, TRUE) != -1)
    m_strYear = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Track", buffer, &chars, TRUE) != -1)
    m_nTrackNum = atoi(buffer);
  chars = 256;
  if (tag->GetFieldString(L"Media", buffer, &chars, TRUE) != -1)
  {
    // cd number is usually "CD 1/3"
    char *num = buffer;
    while (!isdigit(*num) && num < buffer + chars) num++;
    if (isdigit(*num))
      m_nDiscNum = atoi(num);
  }
  chars=256;
  if (tag->GetFieldString(L"Comment", buffer, &chars, TRUE) != -1)
    m_strComment = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Lyrics", buffer, &chars, TRUE) != -1)
    m_strLyrics = buffer;
  chars = 256;
  if (tag->GetFieldString(L"Rating", buffer, &chars, TRUE) != -1)
  { // rating number is usually a single digit, 1-5.  0 is unknown.
    if (buffer[0] >= '0' && buffer[0] < '6')
      m_rating = buffer[0];
  }

  // Replay gain info
  GetReplayGainFromTag(tag);

  delete tag;
  return true;
}

void CAPEv2Tag::GetReplayGainFromTag(IAPETag *tag)
{
  if (!tag) return;
  char buffer[16];
  int chars = 16;

  //  foobar2000 saves gain info as lowercase key items
  if (tag->GetFieldString(L"replaygain_track_gain", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.iTrackGain = (int)(atof(buffer)*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  chars = 16;
  if (tag->GetFieldString(L"replaygain_track_peak", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.fTrackPeak = (float)atof(buffer);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  chars = 16;
  if (tag->GetFieldString(L"replaygain_album_gain", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.iAlbumGain = (int)(atof(buffer)*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  chars = 16;
  if (tag->GetFieldString(L"replaygain_album_peak", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.fAlbumPeak = (float)atof(buffer);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }

  // MP3GAIN saves gain info as uppercase key items
  chars = 16;
  if (tag->GetFieldString(L"REPLAYGAIN_TRACK_GAIN", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.iTrackGain = (int)(atof(buffer)*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  chars = 16;
  if (tag->GetFieldString(L"REPLAYGAIN_TRACK_PEAK", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.fTrackPeak = (float)atof(buffer);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  chars = 16;
  if (tag->GetFieldString(L"REPLAYGAIN_ALBUM_GAIN", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.iAlbumGain = (int)(atof(buffer)*100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  chars = 16;
  if (tag->GetFieldString(L"REPLAYGAIN_ALBUM_PEAK", buffer, &chars, TRUE) != -1)
  {
    m_replayGain.fAlbumPeak = (float)atof(buffer);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }
}
