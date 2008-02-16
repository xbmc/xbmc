#pragma once

#include "MusicDatabase.h"

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
  CCueDocument(void);
  ~CCueDocument(void);
  // USED
  bool Parse(const CStdString &strFile);
  void GetSongs(VECSONGS &songs);
  CStdString GetMediaPath();
  CStdString GetMediaTitle();
  void GetMediaFiles(std::vector<CStdString>& mediaFiles);

private:

  // USED for file access
  XFILE::CFile m_file;
  char m_szBuffer[1024];

  // Member variables
  CStdString m_strArtist;  // album artist
  CStdString m_strAlbum;  // album title
  int m_iTrack;   // current track
  int m_iTotalTracks;  // total tracks
  float m_replayGainAlbumGain;
  float m_replayGainAlbumPeak;

  // cuetrack array
  std::vector<CCueTrack> m_Track;

  bool ReadNextLine(CStdString &strLine);
  bool ExtractQuoteInfo(const CStdString &line, CStdString &quote);
  int ExtractTimeFromIndex(const CStdString &index);
  int ExtractNumericInfo(const CStdString &info);
  bool ResolvePath(CStdString &strPath, const CStdString &strBase);
};
