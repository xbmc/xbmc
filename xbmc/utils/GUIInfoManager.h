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

#define PLAYER_HAS_MEDIA              1
#define PLAYER_HAS_AUDIO              2
#define PLAYER_HAS_VIDEO              3
#define PLAYER_PLAYING                4
#define PLAYER_PAUSED                 5 
#define PLAYER_REWINDING              6
#define PLAYER_REWINDING_2x           7
#define PLAYER_REWINDING_4x           8
#define PLAYER_REWINDING_8x           9
#define PLAYER_REWINDING_16x         10
#define PLAYER_REWINDING_32x         11
#define PLAYER_FORWARDING            12
#define PLAYER_FORWARDING_2x         13
#define PLAYER_FORWARDING_4x         14
#define PLAYER_FORWARDING_8x         15
#define PLAYER_FORWARDING_16x        16
#define PLAYER_FORWARDING_32x        17
#define PLAYER_CAN_RECORD            18
#define PLAYER_RECORDING             19
#define PLAYER_CACHING               20 
#define PLAYER_DISPLAY_AFTER_SEEK    21
#define PLAYER_PROGRESS              22
#define PLAYER_SEEKBAR               23
#define PLAYER_SEEKTIME              24
#define PLAYER_SEEKING               25
#define PLAYER_SHOWTIME              26
#define PLAYER_TIME                  27  
#define PLAYER_TIME_REMAINING        28
#define PLAYER_DURATION              29
#define PLAYER_SHOWCODEC             30
#define PLAYER_SHOWINFO              31
#define PLAYER_VOLUME                32
#define PLAYER_MUTED                 33

#define WEATHER_CONDITIONS          100
#define WEATHER_TEMPERATURE         101
#define WEATHER_LOCATION            102

#define SYSTEM_TIME                 110
#define SYSTEM_DATE                 111
#define SYSTEM_CPU_TEMPERATURE      112
#define SYSTEM_GPU_TEMPERATURE      113
#define SYSTEM_FAN_SPEED            114
#define SYSTEM_FREE_SPACE_C         115
#define SYSTEM_FREE_SPACE_C         115
/* 
#define SYSTEM_FREE_SPACE_D         116 //116 is reserved for space on D
*/
#define SYSTEM_FREE_SPACE_E         117
#define SYSTEM_FREE_SPACE_F         118
#define SYSTEM_FREE_SPACE_G         119
#define SYSTEM_BUILD_VERSION        120
#define SYSTEM_BUILD_DATE           121
#define SYSTEM_ETHERNET_LINK_ACTIVE 122
#define SYSTEM_FPS                  123
#define SYSTEM_KAI_CONNECTED        124
#define SYSTEM_ALWAYS_TRUE          125   // useful for <visible fade="10" start="hidden">true</visible>, to fade in a control
#define SYSTEM_ALWAYS_FALSE         126   // used for <visible fade="10">false</visible>, to fade out a control (ie not particularly useful!)
#define SYSTEM_MEDIA_DVD            127
#define SYSTEM_NO_SUCH_ALARM        128
#define SYSTEM_HAS_ALARM            129
#define SYSTEM_AUTODETECTION        130
#define SYSTEM_FREE_MEMORY          131
#define SYSTEM_SCREEN_MODE          132
#define SYSTEM_SCREEN_WIDTH         133
#define SYSTEM_SCREEN_HEIGHT        134
#define SYSTEM_CURRENT_WINDOW       135
#define SYSTEM_CURRENT_CONTROL      136
#define SYSTEM_XBOX_NICKNAME        137
#define SYSTEM_DVD_LABEL            138

#define LCD_PLAY_ICON               160
#define LCD_PROGRESS_BAR            161
#define LCD_CPU_TEMPERATURE         162
#define LCD_GPU_TEMPERATURE         163
#define LCD_FAN_SPEED               164
#define LCD_DATE                    166
#define LCD_FREE_SPACE_C            167
/*
#define LCD_FREE_SPACE_D            168 // 168 is reserved for space on D
*/
#define LCD_FREE_SPACE_E            169
#define LCD_FREE_SPACE_F            170
#define LCD_FREE_SPACE_G            171

#define NETWORK_IP_ADDRESS          190

#define MUSICPLAYER_TITLE           200
#define MUSICPLAYER_ALBUM           201
#define MUSICPLAYER_ARTIST          202
#define MUSICPLAYER_GENRE           203
#define MUSICPLAYER_YEAR            204
#define MUSICPLAYER_TIME            205
#define MUSICPLAYER_TIME_REMAINING  206
#define MUSICPLAYER_TIME_SPEED      207
#define MUSICPLAYER_TRACK_NUMBER    208
#define MUSICPLAYER_DURATION        209
#define MUSICPLAYER_COVER           210
#define MUSICPLAYER_BITRATE         211
#define MUSICPLAYER_PLAYLISTLEN     212
#define MUSICPLAYER_PLAYLISTPOS     213
#define MUSICPLAYER_CHANNELS        214
#define MUSICPLAYER_BITSPERSAMPLE   215
#define MUSICPLAYER_SAMPLERATE      216
#define MUSICPLAYER_CODEC           217

#define VIDEOPLAYER_TITLE           250
#define VIDEOPLAYER_GENRE           251
#define VIDEOPLAYER_DIRECTOR        252
#define VIDEOPLAYER_YEAR            253
#define VIDEOPLAYER_TIME            254
#define VIDEOPLAYER_TIME_REMAINING  255
#define VIDEOPLAYER_TIME_SPEED      256
#define VIDEOPLAYER_DURATION        257
#define VIDEOPLAYER_COVER           258
#define VIDEOPLAYER_USING_OVERLAYS  259
#define VIDEOPLAYER_ISFULLSCREEN    260
#define VIDEOPLAYER_HASMENU         261
#define VIDEOPLAYER_PLAYLISTLEN     262
#define VIDEOPLAYER_PLAYLISTPOS     263

#define AUDIOSCROBBLER_ENABLED      300
#define AUDIOSCROBBLER_CONN_STATE   301
#define AUDIOSCROBBLER_SUBMIT_INT   302
#define AUDIOSCROBBLER_FILES_CACHED 303
#define AUDIOSCROBBLER_SUBMIT_STATE 304

#define LISTITEM_THUMB              310
#define LISTITEM_LABEL              311
#define LISTITEM_TITLE              312
#define LISTITEM_TRACKNUMBER        313
#define LISTITEM_ARTIST             314
#define LISTITEM_ALBUM              315
#define LISTITEM_YEAR               316
#define LISTITEM_GENRE              317
#define LISTITEM_ICON               318
#define LISTITEM_DIRECTOR           319

#define MUSICPM_ENABLED             350
#define MUSICPM_SONGSPLAYED         351
#define MUSICPM_MATCHINGSONGS       352
#define MUSICPM_MATCHINGSONGSPICKED 353
#define MUSICPM_MATCHINGSONGSLEFT   354
#define MUSICPM_RELAXEDSONGSPICKED  355
#define MUSICPM_RANDOMSONGSPICKED   356

#define PLAYLIST_LENGTH             390
#define PLAYLIST_POSITION           391
#define PLAYLIST_RANDOM             392
#define PLAYLIST_REPEAT             393
#define PLAYLIST_ISRANDOM           394
#define PLAYLIST_ISREPEAT           395
#define PLAYLIST_ISREPEATONE        396

#define VISUALISATION_LOCKED        400
#define VISUALISATION_PRESET        401
#define VISUALISATION_NAME          402
#define VISUALISATION_ENABLED       403

#define SKIN_HAS_THEME_START        500
#define SKIN_HAS_THEME_END          509 // allow for max 10 themes

#define SKIN_HAS_SETTING_START      510
#define SKIN_HAS_SETTING_END        600 // allow 90

#define WINDOW_IS_VISIBLE           9995
#define WINDOW_NEXT                 9996
#define WINDOW_PREVIOUS             9997
#define WINDOW_IS_MEDIA             9998
#define WINDOW_ACTIVE_START         WINDOW_HOME
#define WINDOW_ACTIVE_END           WINDOW_PYTHON_END

#define SYSTEM_IDLE_TIME_START      20000
#define SYSTEM_IDLE_TIME_FINISH     21000 // 1000 seconds

#define CONTROL_IS_VISIBLE          29998
#define CONTROL_GROUP_HAS_FOCUS     29999
#define CONTROL_HAS_FOCUS_START     30000
#define CONTROL_HAS_FOCUS_END       31000 // only up to control id 1000

#define BUTTON_SCROLLER_HAS_ICON_START 31000
#define BUTTON_SCROLLER_HAS_ICON_END   31200  // only allow 100 buttons (normally start at 101)

#define VERSION_STRING "1.1.0"

// the multiple information vector
#define MULTI_INFO_START              40000
#define MULTI_INFO_END                41000 // 1000 references is all we have for now
#define COMBINED_VALUES_START        100000



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
  string GetLabel(int info);

  CStdString GetImage(int info, int contextWindow = WINDOW_INVALID);

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
  CStdString GetPlaylistLabel(int item);
  CStdString GetMusicPartyModeLabel(int item);
  string GetFreeSpace(int drive, bool shortText = false);
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

  string GetSystemHeatInfo(const CStdString &strInfo);
  void UpdateFPS();
  inline float GetFPS() const { return m_fps; };

  void SetAutodetectedXbox(bool set) { m_hasAutoDetectedXbox = set; };
  bool HasAutodetectedXbox() const { return m_hasAutoDetectedXbox; };

  void SetNextWindow(int windowID) { m_nextWindowID = windowID; };
  void SetPreviousWindow(int windowID) { m_prevWindowID = windowID; };

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

  int m_nextWindowID;
  int m_prevWindowID;

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
