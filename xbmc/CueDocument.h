#pragma once

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

#include "music/Song.h"

#define MAX_PATH_SIZE 1024

namespace XFILE {
  class CFile;
}

class CFileItem;
class CueReader
{
  bool m_external;
public:
  CueReader() {}
  inline bool skipChar(char ch) const
  {
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
  }
  virtual bool ReadNextLine(CStdString &line) = 0;
  virtual ~CueReader() {};
};

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
    CStdString strArtist;
    CStdString strTitle;
    CStdString strFile;
    int iTrackNumber;
    int iStartTime;
    int iEndTime;
    float replayGainTrackGain;
    float replayGainTrackPeak;
  };

public:
  CCueDocument(const CFileItem &fileItem);
  ~CCueDocument();
  // USED

  bool isValid() const;
  bool isExternal() const;
  void GetSongs(VECSONGS &songs);
  CStdString GetMediaPath();
  CStdString GetMediaTitle();
  void GetMediaFiles(std::vector<CStdString>& mediaFiles);

private:
  // Member variables
  CStdString m_strArtist;  // album artist
  CStdString m_strAlbum;  // album title
  CStdString m_strGenre;  // album genre
  int m_iYear;            //album year
  int m_iTrack;   // current track
  int m_iTotalTracks;  // total tracks
  float m_replayGainAlbumGain;
  float m_replayGainAlbumPeak;
  bool m_valid;
  bool m_external;
  CStdString m_sourcePath;

  CueReader* m_reader;

  // cuetrack array
  std::vector<CCueTrack> m_Track;

  bool Parse();

  //bool ReadNextLine(XFILE::CFile& file, CStdString &strLine);
  bool ExtractQuoteInfo(const CStdString &line, CStdString &quote);
  int ExtractTimeFromIndex(const CStdString &index);
  int ExtractNumericInfo(const CStdString &info);
  bool ResolvePath(CStdString &strPath);

  bool Load(const CFileItem &fileItem);
};
