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

//bool: true if we are caching
//int: current progress when caching (can be used in progressbars or sliders)
#define PLAYER_CACHING               20

#define PLAYER_DISPLAY_AFTER_SEEK    21
#define PLAYER_PROGRESS              22

//This has multiple values
//bool: true if seekbar is visible
//int: progress value 0-100 (can be used in progressbars or sliders)
#define PLAYER_SEEKBAR               23
#define PLAYER_SEEKTIME              24
#define PLAYER_SEEKING               25
#define PLAYER_SHOWTIME              26

/*!
 \ingroup strings
 \brief 
 */
class CGUIInfoManager
{
public:
  CGUIInfoManager(void);
  virtual ~CGUIInfoManager(void);

  void Clear();

  int TranslateString(const CStdString &strCondition);
  bool GetBool(int condition) const;
  int GetInt(int info) const;
  wstring GetLabel(int info);

  CStdString GetImage(int info);

  CStdString GetTime(bool bSeconds = false);
  CStdString GetDate(bool bNumbersOnly = false);

  void SetCurrentItem(CFileItem &item);
  void ResetCurrentItem();
  // Current song stuff
  /// \brief Retrieves tag info (if necessary) and fills in our current song path.
  void SetCurrentSong(CFileItem &item);
  void SetCurrentSongTag(const CMusicInfoTag &tag) { m_currentSong.m_musicInfoTag = tag; m_currentSong.m_lStartOffset = 0;};
  const CMusicInfoTag &GetCurrentSongTag() const { return m_currentSong.m_musicInfoTag; };

  // Current movie stuff
  void SetCurrentMovie(CFileItem &item);
  const CIMDBMovie &GetCurrentMovie() const { return m_currentMovie; };

  CStdString GetMusicLabel(int item);
  CStdString GetVideoLabel(int item);
  wstring GetFreeSpace(int drive);
  __int64 GetPlayTime();  // in ms
  CStdString GetCurrentPlayTime();
  int GetPlayTimeRemaining();
  int GetTotalPlayTime();
  CStdString GetCurrentPlayTimeRemaining();
  CStdString GetVersion();
  CStdString GetBuild();

  bool GetDisplayAfterSeek() const;
  void SetDisplayAfterSeek(DWORD TimeOut = 2500);
  void SetSeeking(bool seeking) { m_playerSeeking = seeking; };
  void SetShowTime(bool showtime) { m_playerShowTime = showtime; };

  bool m_performingSeek;
protected:
  int TranslateSingleString(const CStdString &strCondition);

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
  bool m_playerSeeking;
  bool m_playerShowTime;

  class CCombinedValue
  {
  public:
    CStdString m_info;    // the text expression
    int m_id;             // the id used to identify this expression
    list<int> m_postfix;  // the postfix binary expression
    void operator=(const CCombinedValue& mSrc);
  };

  int GetOperator(const char ch);
  int TranslateBooleanExpression(const CStdString &expression);
  bool EvaluateBooleanExpression(const CCombinedValue &expression, bool &result) const;

  std::vector<CCombinedValue> m_CombinedValues;
};

/*!
 \ingroup strings
 \brief 
 */
extern CGUIInfoManager g_infoManager;
#endif
