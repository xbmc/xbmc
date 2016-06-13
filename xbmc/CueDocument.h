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

#include <string>
#include <vector>

#include "music/Song.h"

#define MAX_PATH_SIZE 1024

class CueReader;

class CCueDocument
{
  class CCueTrack
  {
  public:
    CCueTrack()
      : iTrackNumber(0)
      , iStartTime(0)
      , iEndTime(0)
    {
    }
    std::string strArtist;
    std::string strTitle;
    std::string strFile;
    int iTrackNumber;
    int iStartTime;
    int iEndTime;
    ReplayGain::Info replayGain;
  };
public:
  CCueDocument(void);
  ~CCueDocument(void);
  // USED
  bool ParseFile(const std::string &strFilePath);
  bool ParseTag(const std::string &strContent);
  void GetSongs(VECSONGS &songs);
  bool GetSong(int aTrackNumber, CSong& aSong);
  std::string GetMediaPath();
  std::string GetMediaTitle();
  void GetMediaFiles(std::vector<std::string>& mediaFiles);
  void UpdateMediaFile(const std::string& oldMediaFile, const std::string& mediaFile);
  bool IsOneFilePerTrack() const;
  bool IsLoaded() const;
private:
  void Clear();
  bool Parse(CueReader& reader, const std::string& strFile = std::string());

  // Member variables
  std::string m_strArtist;  // album artist
  std::string m_strAlbum;  // album title
  std::string m_strGenre;  // album genre
  int m_iYear;            //album year
  int m_iTrack;   // current track
  int m_iDiscNumber;  // Disc number
  ReplayGain::Info m_albumReplayGain;

  bool m_bOneFilePerTrack;

  // cuetrack array
  typedef std::vector<CCueTrack> Tracks;
  Tracks m_tracks;

  std::string ExtractInfo(const std::string &line);
  int ExtractTimeFromIndex(const std::string &index);
  int ExtractNumericInfo(const std::string &info);
  bool ResolvePath(std::string &strPath, const std::string &strBase);
};
