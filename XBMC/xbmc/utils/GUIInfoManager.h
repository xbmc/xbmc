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
#define PLAYER_SHOWCODEC             30
#define PLAYER_SHOWINFO              31

// structure to hold multiple integer data
// for storage referenced from a single integer
class GUIInfo
{
public:
  GUIInfo(int info, int data1 = 0, int data2 = 0)
  {
    m_info = info;
    m_data1 = data1;
    m_data2 = data2;
  }
  bool operator ==(const GUIInfo &right) const
  {
    return (m_info == right.m_info && m_data1 == right.m_data1 && m_data2 == right.m_data2);
  };
  int m_info;
  int m_data1;
  int m_data2;
};

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
  bool GetBool(int condition, DWORD dwContextWindow = 0) const;
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
  void SetCurrentAlbumThumb(const CStdString thumbFileName);
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
  void SetShowCodec(bool showcodec) { m_playerShowCodec = showcodec; };
  void SetShowInfo(bool showinfo) { m_playerShowInfo = showinfo; };
  void ToggleShowCodec() { m_playerShowCodec = !m_playerShowCodec; };
  void ToggleShowInfo() { m_playerShowInfo = !m_playerShowInfo; };

  bool m_performingSeek;

  wstring GetSystemHeatInfo(const CStdString &strInfo);
  void UpdateFPS();
  inline float GetFPS() const { return m_fps; };

  void SetAutodetectedXbox(bool set) { m_hasAutoDetectedXbox = set; };
  bool HasAutodetectedXbox() const { return m_hasAutoDetectedXbox; };

  CStdString ParseLabel(const CStdString &label);

protected:
  bool GetMultiInfoBool(const GUIInfo &info, DWORD dwContextWindow = 0) const;
  int TranslateSingleString(const CStdString &strCondition);
  CStdString GetItemLabel(const CFileItem *item, int info);

  // Conditional string parameters for testing are stored in a vector for later retrieval.
  // The offset into the string parameters array is returned.
  int ConditionalStringParameter(const CStdString &strParameter);
  int AddMultiInfo(const GUIInfo &info);

  CStdString GetAudioScrobblerLabel(int item);

  // Conditional string parameters are stored here
  CStdStringArray m_stringParameters;

  // Array of multiple information mapped to a single integer lookup
  vector<GUIInfo> m_multiInfo;

  // Current playing stuff
  CFileItem m_currentSong;
  CIMDBMovie m_currentMovie;
  CStdString m_currentMovieThumb;
  unsigned int m_lastMusicBitrateTime;
  unsigned int m_MusicBitrate;

  // fan stuff
  DWORD m_lastSysHeatInfoTime;
  int m_fanSpeed;
  float m_gpuTemp;
  float m_cpuTemp;

  //Fullscreen OSD Stuff
  DWORD m_AfterSeekTimeout;
  bool m_playerSeeking;
  bool m_playerShowTime;
  bool m_playerShowCodec;
  bool m_playerShowInfo;

  // FPS counters
  float m_fps;
  unsigned int m_frameCounter;
  unsigned int m_lastFPSTime;

  // Xbox Autodetect stuff
  bool m_hasAutoDetectedXbox;

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
  bool EvaluateBooleanExpression(const CCombinedValue &expression, bool &result, DWORD dwContextWindow) const;

  std::vector<CCombinedValue> m_CombinedValues;
};

/*!
 \ingroup strings
 \brief 
 */
extern CGUIInfoManager g_infoManager;
#endif
