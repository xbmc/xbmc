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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "CueParser.h"
#include "CueParserCallback.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

CueParser::CueParser()
{
}

void CueParser::reset()
{
  m_file.Empty();
  m_currentTrack.reset();
  m_currentTrack.m_trackNumber = 0;
  m_globalData.reset();
  m_is_va = false;
  m_tracks.clear();
}

bool CueParser::parse(CueParserCallback& callback)
{
  reset();

  CStdString strLine;
  // Run through the .CUE file and extract the tracks...
  while (true)
  {
    if (!callback.onDataNeeded(strLine))
      break;
    if (strLine.find('\n') != -1)
      CLog::Log(LOGINFO, "Error!!!");

    CStdString cmd = cutFirstWord(strLine);
    if (cmd == "INDEX")
    {
      if (!onIndex(strLine))
      {
        OutputDebugString("CUE INDEX parsing error");
        return false;
      }
    }
    else if (cmd == "FLAGS")
    {
      if (!onFlags(strLine))
      {
        OutputDebugString("CUE FLAGS parsing error");
        return false;
      }
    }
    else if (cmd == "PREGAP")
    {
      if (!onPregap(strLine))
      {
        OutputDebugString("CUE PREGAP parsing error");
        return false;
      }
    }
    else if (cmd == "TRACK")
    {
      if (!onTrack(strLine))
      {
        OutputDebugString("CUE TRACK parsing error");
        return false;
      }
    }
    else if (cmd == "FILE")
    {
      if (!onFile(strLine))
      {
        OutputDebugString("CUE FILE parsing error");
        return false;
      }
      if (!callback.onFile(m_file))
        return false;
    }
    else if (cmd == "REM")
    {
      bool success = false;
      CStdString name = cutFirstWord(strLine);
      CStdString value;
      success = extractString(strLine, value, false);
      if (success)
      {
        if (name == "GENRE" || name == "DATE" || name == "DISCID" || name == "COMMENT" ||
          name == "REPLAYGAIN_TRACK_GAIN" || name == "REPLAYGAIN_TRACK_PEAK" || 
          name == "REPLAYGAIN_ALBUM_GAIN" || name == "REPLAYGAIN_ALBUM_PEAK")
        {
          success = onMeta(name, value);
        }
      }
      if (!success)
      {
        OutputDebugString("CUE parsing error");
        return false;
      }
    }
    else
    {
      bool success = true;
      CStdString value;
      if (cmd == "SONGWRITER" || cmd == "TITLE" || cmd == "PERFORMER")
      {
        success = extractString(strLine ,value, true);
        if (cmd == "PERFORMER")
          cmd = "ARTIST";
        if (success)
          success = onMeta(cmd, value);
      }
      else if (cmd == "CATALOG" || cmd == "ISRC")
        success = extractString(strLine ,value, false) && onMeta(cmd, value);
      if (!success)
      {
        OutputDebugString("CUE parsing error");
        return false;
      }
    }
  }
  if (m_currentTrack.m_trackNumber)
  {
    if (finalizeTrack())
    {
      m_currentTrack.m_trackNumber = 0;
      for (Tracks::iterator iter = m_tracks.begin(); iter != m_tracks.end(); ++iter)
      {
        CSong song;
        // set base data
        song.strFileName = iter->m_file;
        song.iTrack = iter->m_trackNumber;
        song.iStartOffset = iter->m_indexes.start();
        song.iEndOffset = iter->m_endOffset;
        if (song.iEndOffset)
          song.iDuration = (song.iEndOffset - song.iStartOffset + 37) / 75; // what is 37???
        else
          song.iDuration = 0;
        // set track title
        song.strTitle = iter->m_meta.value("TITLE");
        if (song.strTitle.length() == 0)
          song.strTitle.Format("Track %2d", iter->m_trackNumber);
        // set album
        song.strAlbum = iter->m_meta.value("ALBUM");
        if (song.strAlbum.IsEmpty())
          song.strAlbum = m_globalData.m_meta.value("ALBUM");
        // set artist
        if (m_is_va)
        {
          CStdString globalArtist = m_globalData.m_meta.value("ARTIST");
          CStdString localArtist = iter->m_meta.value("ARTIST");
          if (globalArtist.GetLength())
          {
            song.albumArtist = StringUtils::Split(globalArtist, g_advancedSettings.m_musicItemSeparator);
            if (localArtist.GetLength())
              song.artist = StringUtils::Split(localArtist, g_advancedSettings.m_musicItemSeparator);
            else
              song.artist = song.albumArtist;
          }
          else
          {
            if (localArtist.GetLength())
              song.artist = StringUtils::Split(localArtist, g_advancedSettings.m_musicItemSeparator);
          }
        }
        song.strComment = iter->m_meta.value("COMMENT");
        // set genre
        CStdString tmp = m_globalData.m_meta.value("GENRE");
        if (tmp.GetLength())
          song.genre = StringUtils::Split(tmp, g_advancedSettings.m_musicItemSeparator);
        // set year
        tmp = iter->m_meta.value("DATE");
        song.iYear = atoi(tmp.c_str());
        if (song.iYear <= 0)
          song.iYear = 0;

        // \todo Other meta fields, replay gain???
        if (!callback.onTrackReady(song))
          return false;
      }
    }
    return true;
  }
  return false;
}

bool CueParser::onMeta(const CStdString& name, const CStdString& value)
{
  if (!value.length() || !name.length())
    return false;
  if (!m_currentTrack.m_trackNumber)
  {
    if (name == "TITLE")
      m_globalData.m_meta.insert("ALBUM", value);
    else
    {
      if (name == "ARTIST")
        m_albumArtist = value;
      m_globalData.m_meta.insert(name, value);
    }
  }
  else
  {
    if (!m_is_va)
    {
      if (name == "ARTIST")
      {
        if (m_albumArtist.length())
          m_is_va = !m_albumArtist.Equals(value);
      }
    }
    m_currentTrack.m_meta.insert(name, value);
  }
  return true;
}

bool CueParser::onIndex(const CStdString& strLine)
{
  CStdString indexStr;
  unsigned i = 0;
  for (; i < strLine.length() && isdigit(strLine[i]); ++i)
    indexStr += strLine[i];
  if (!indexStr.length())
    return false;
  unsigned index = atoi(indexStr.c_str());
  if (index > 99)
    return false;

  unsigned time;
  CStdString timeStr = strLine.Mid(i);
  if (!extractTime(timeStr.Trim(), time))
    return false;
  m_currentTrack.m_indexes.setIndex(index, time);
  return true;
}

bool CueParser::onTrack(const CStdString& strLine)
{
  int idx = strLine.Find("AUDIO");
  if (idx < 0)
    return false;
  CStdString number = strLine.Left(idx-1).Trim();
  unsigned i = 0;
  for (; i < number.length() && isdigit(number[i]); ++i)
    ;
  if (i < number.length())
    return false;
  int track = atoi(number.c_str());
  if (track < 1 || track > 99)
    return false;
  bool success = true;
  if (m_currentTrack.m_trackNumber)
    success = finalizeTrack();
  if (success)
  {
    m_currentTrack.m_trackNumber = track;
    m_currentTrack.m_file = m_file;
  }
  return (!m_currentTrack.m_file.IsEmpty());
}

bool CueParser::finalizeTrack()
{
  if (!m_currentTrack.m_indexes.isValid())
    return false;
  if (!m_currentTrack.m_indexes.updatePreGap(m_currentTrack.m_pregap))
    return false;
  if (m_currentTrack.m_file.IsEmpty())
    return false;

  Tracks::reverse_iterator last = m_tracks.rbegin();
  if (last != m_tracks.rend())
    last->m_endOffset = m_currentTrack.m_indexes.start() - m_currentTrack.m_indexes.pregap();

  m_tracks.push_back(m_currentTrack);
  m_currentTrack.reset();
  return true;
}

bool CueParser::onFile(const CStdString& strLine)
{
  CStdString file;
  if (extractString(strLine, file, true))
    m_file = file;
  return m_file.size() > 0;
}

bool CueParser::onFlags(const CStdString& strLine)
{
  m_globalData.m_flags = strLine;
  return true;
}

bool CueParser::onPregap(const CStdString& strLine)
{
  return extractTime(strLine, m_currentTrack.m_pregap);
}

bool CueParser::extractString(const CStdString &line, CStdString &outStr, bool spaceIsError)
{
  outStr.Empty();
  int left = line.Find('\"');
  if (left >= 0)
  {
    int right = line.Find('\"', left + 1);
    if (right < 0)
      return false;
    outStr = line.Mid(left + 1, right - left - 1);
  }
  else
  {
    if (spaceIsError && line.find(' ') >= 0)
      return false;
    outStr = line.Mid(0);
  }
  g_charsetConverter.unknownToUTF8(outStr);
  return true;
}

bool CueParser::extractTime(const CStdString& timeStr, unsigned& time)
{
  CStdStringArray timePatrs;
  StringUtils::SplitString(timeStr, ":", timePatrs);
  unsigned i = 0;
  for (; i < timePatrs.size(); ++i)
  {
    unsigned j = 0;
    for (; j < timePatrs[i].length() && isdigit(timePatrs[i][j]); ++j)
      ;
    if (j < timePatrs[i].length())
      return false;
  }

  if (i == 0)
    return false;

  CStdString frames;
  CStdString seconds;
  CStdString minutes;

  switch (timePatrs.size())
  {
  case 1:
    frames = timePatrs[0];
    break;
  case 2:
    seconds = timePatrs[0];
    frames = timePatrs[1];
    break;
  case 3:
    minutes = timePatrs[0];
    seconds = timePatrs[1];
    frames = timePatrs[2];
    break;
  default:
    return false;
  }
  time = 0;
  if (frames.length())
    time = atoi(frames.c_str());
  if (seconds.length())
    time += 75 * atoi(seconds.c_str());
  if (minutes.length())
    time += 75 * 60 * atoi(minutes.c_str());
  return true;
}

CStdString CueParser::cutFirstWord(CStdString& inputStr)
{
  CStdString out;
  int idx = inputStr.Find(' ');
  if (idx > 0)
  {
    out = inputStr.Left(idx);
    inputStr.Delete(0, idx);
    inputStr.TrimLeft();
  }
  else
  {
    out = inputStr;
    inputStr.Empty();
  }
  return out;
}

void CueParser::TrackInfo::reset()
{
  m_file.Empty();
  m_meta.clear();
  m_indexes.reset();
  m_endOffset = 0;
  m_pregap = 0;
}

void CueParser::GlobalInfo::reset()
{
  m_meta.clear();
  m_flags.Empty();
}

