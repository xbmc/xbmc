#pragma once

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

#include "music/Song.h"
#include "filesystem/File.h"

#define MAX_PATH_SIZE 1024

class CCueDocument
{
  class CCueTrack
  {
  public:
    CCueTrack()
    {
      iTrackNumber = 0;
      iStartTime = 0;
      iEndTime = 0;
      replayGainTrackGain = 0.0f;
      replayGainTrackPeak = 0.0f;
    }
    std::string strArtist;
    std::string strTitle;
    std::string strFile;
    int iTrackNumber;
    int iStartTime;
    int iEndTime;
    float replayGainTrackGain;
    float replayGainTrackPeak;
  };

public:
  CCueDocument(void);
  ~CCueDocument(void);
  // USED
  bool Parse(const std::string &strFile);
  void GetSongs(VECSONGS &songs);
  std::string GetMediaPath();
  std::string GetMediaTitle();
  void GetMediaFiles(std::vector<std::string>& mediaFiles);

private:

  // USED for file access
  XFILE::CFile m_file;
  char m_szBuffer[1024];

  // Member variables
  std::string m_strArtist;  // album artist
  std::string m_strAlbum;  // album title
  std::string m_strGenre;  // album genre
  int m_iYear;            //album year
  int m_iTrack;   // current track
  int m_iTotalTracks;  // total tracks
  int m_iDiscNumber;  // Disc number
  float m_replayGainAlbumGain;
  float m_replayGainAlbumPeak;

  // cuetrack array
  std::vector<CCueTrack> m_Track;

  bool ReadNextLine(std::string &strLine);
  std::string ExtractInfo(const std::string &line);
  int ExtractTimeFromIndex(const std::string &index);
  int ExtractNumericInfo(const std::string &info);
  bool ResolvePath(std::string &strPath, const std::string &strBase);
};
