/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

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
    std::string strArtist;
    std::string strTitle;
    std::string strFile;
    int iTrackNumber = 0;
    int iStartTime = 0;
    int iEndTime = 0;
    ReplayGain::Info replayGain;
  };
public:
  ~CCueDocument(void);
  // USED
  bool ParseFile(const std::string &strFilePath);
  bool ParseTag(const std::string &strContent);
  void GetSongs(VECSONGS &songs);
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
  int m_iYear = 0;            //album year
  int m_iTrack = 0;   // current track
  int m_iDiscNumber = 0;  // Disc number
  ReplayGain::Info m_albumReplayGain;

  bool m_bOneFilePerTrack = false;

  // cuetrack array
  typedef std::vector<CCueTrack> Tracks;
  Tracks m_tracks;

  std::string ExtractInfo(const std::string &line);
  int ExtractTimeFromIndex(const std::string &index);
  int ExtractNumericInfo(const std::string &info);
  bool ResolvePath(std::string &strPath, const std::string &strBase);
};
