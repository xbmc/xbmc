/*!
\file GUIInfoManager.h
\brief 
*/

#ifndef GUILIB_GUIInfoManager_H
#define GUILIB_GUIInfoManager_H
#pragma once

#include "../MusicInfoTag.h"
#include "../FileItem.h"
#include "../videodatabase.h"

/*!
 \ingroup strings
 \brief 
 */
class CGUIInfoManager
{
public:
  CGUIInfoManager(void);
  virtual ~CGUIInfoManager(void);

  int TranslateString(const CStdString &strCondition);
  bool GetBool(int condition) const;
  wstring GetLabel(int info);

  CStdString GetImage(int info);

  wstring GetTime(bool bSeconds = false);
  wstring GetDate(bool bNumbersOnly = false);

  void SetCurrentItem(CFileItem &item);
  void ResetCurrentItem();
  // Current song stuff
  /// \brief Retrieves tag info (if necessary) and fills in our current song path.
  void SetCurrentSong(CFileItem &item);
  void SetCurrentSongTag(const CMusicInfoTag &tag) { m_currentSong.m_musicInfoTag = tag; m_currentSong.m_lStartOffset = 0;};
  const CMusicInfoTag &GetCurrentSongTag() const { return m_currentSong.m_musicInfoTag; };
  long GetCurrentSongStart() { return m_currentSong.m_lStartOffset; };
  long GetCurrentSongEnd() { return m_currentSong.m_lEndOffset; };

  // Current movie stuff
  void SetCurrentMovie(CFileItem &item);
  const CIMDBMovie &GetCurrentMovie() const { return m_currentMovie; };

  CStdString GetMusicLabel(int item);
  CStdString GetVideoLabel(int item);
  wstring GetFreeSpace(int drive);
  int GetPlayTime();
  CStdString GetCurrentPlayTime();
  int GetPlayTimeRemaining();
  CStdString GetCurrentPlayTimeRemaining();
  CStdString GetVersion();
  CStdString GetBuild();

  bool GetDisplayAfterSeek() const;
  void SetDisplayAfterSeek(DWORD TimeOut = 2500);
protected:

  wstring GetSystemHeatInfo(const CStdString &strInfo);
  CStdString GetAudioScrobblerLabel(int item);

  // Current playing stuff
  CFileItem m_currentSong;
  CIMDBMovie m_currentMovie;
  CStdString m_currentMovieThumb;

  // fan stuff
  DWORD m_lastSysHeatInfoTime;
  int m_fanSpeed;
  float m_gpuTemp;
  float m_cpuTemp;

  //Fullscreen OSD Stuff
  DWORD m_AfterSeekTimeout;
};

/*!
 \ingroup strings
 \brief 
 */
extern CGUIInfoManager g_infoManager;
#endif
