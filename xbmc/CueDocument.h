/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "music/Song.h"

#include <string>
#include <vector>

#define MAX_PATH_SIZE 1024

class CFileItem;
class CFileItemList;
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

  bool LoadTracks(CFileItemList& scannedItems, const CFileItem& item);

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
