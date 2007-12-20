/*!
\file GUIInfoManager.h
\brief 
*/

#ifndef GUILIB_GUIInfoManager_H
#define GUILIB_GUIInfoManager_H
#pragma once

#include "../musicInfoTag.h"
#include "../FileItem.h"
#include "../VideoDatabase.h"
#include "../StringUtils.h"
#include "../Temperature.h"
#include "../utils/CriticalSection.h"
#include "IMsgTargetCallback.h"

// conditions for window retrieval
#define WINDOW_CONDITION_HAS_LIST_ITEMS  1
#define WINDOW_CONDITION_IS_MEDIA_WINDOW 2

#define OPERATOR_NOT  3
#define OPERATOR_AND  2
#define OPERATOR_OR   1

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
#define PLAYER_HASDURATION           34
#define PLAYER_CHAPTER               35
#define PLAYER_CHAPTERCOUNT          36
#define PLAYER_TIME_SPEED            37
#define PLAYER_FINISH_TIME           38
#define PLAYER_CACHELEVEL            39

#define WEATHER_CONDITIONS          100
#define WEATHER_TEMPERATURE         101
#define WEATHER_LOCATION            102
#define WEATHER_IS_FETCHED          103

#define SYSTEM_PROGRESS_BAR         107
#define SYSTEM_LANGUAGE             108
#define SYSTEM_LAUNCHING_XBE        109
#define SYSTEM_TIME                 110
#define SYSTEM_DATE                 111
#define SYSTEM_CPU_TEMPERATURE      112
#define SYSTEM_GPU_TEMPERATURE      113
#define SYSTEM_FAN_SPEED            114
#define SYSTEM_FREE_SPACE_C         115
// #define SYSTEM_FREE_SPACE_D         116 //116 is reserved for space on D
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
#define SYSTEM_DVDREADY             128
#define SYSTEM_HAS_ALARM            129
#define SYSTEM_SCREEN_MODE          132
#define SYSTEM_SCREEN_WIDTH         133
#define SYSTEM_SCREEN_HEIGHT        134
#define SYSTEM_CURRENT_WINDOW       135
#define SYSTEM_CURRENT_CONTROL      136
#define SYSTEM_XBOX_NICKNAME        137
#define SYSTEM_DVD_LABEL            138
#define SYSTEM_HAS_DRIVE_F          139
#define SYSTEM_HASLOCKS             140
#define SYSTEM_ISMASTER             141
#define SYSTEM_TRAYOPEN	            142
#define SYSTEM_KAI_ENABLED          143
#define SYSTEM_ALARM_POS            144
#define SYSTEM_LOGGEDON             145
#define SYSTEM_PROFILENAME          146
#define SYSTEM_PROFILETHUMB         147
#define SYSTEM_HAS_LOGINSCREEN      148
#define SYSTEM_HAS_DRIVE_G          149
#define SYSTEM_HDD_SMART            150
#define SYSTEM_HDD_TEMPERATURE      151
#define SYSTEM_HDD_MODEL            152
#define SYSTEM_HDD_SERIAL           153
#define SYSTEM_HDD_FIRMWARE         154
#define SYSTEM_HDD_PASSWORD         156
#define SYSTEM_HDD_LOCKSTATE        157
#define SYSTEM_HDD_LOCKKEY          158
#define SYSTEM_INTERNET_STATE       159
#define LCD_PLAY_ICON               160
#define LCD_PROGRESS_BAR            161
#define LCD_CPU_TEMPERATURE         162
#define LCD_GPU_TEMPERATURE         163
#define LCD_HDD_TEMPERATURE         164
#define LCD_FAN_SPEED               165
#define LCD_DATE                    166
#define LCD_FREE_SPACE_C            167
// #define LCD_FREE_SPACE_D            168 // 168 is reserved for space on D
#define LCD_FREE_SPACE_E            169
#define LCD_FREE_SPACE_F            170
#define LCD_FREE_SPACE_G            171

#define NETWORK_IP_ADDRESS          190
#define NETWORK_MAC_ADDRESS         191
#define NETWORK_IS_DHCP             192
#define NETWORK_LINK_STATE          193
#define NETWORK_SUBNET_ADDRESS      194
#define NETWORK_GATEWAY_ADDRESS     195
#define NETWORK_DNS1_ADDRESS        196
#define NETWORK_DNS2_ADDRESS        197
#define NETWORK_DHCP_ADDRESS        198

#define MUSICPLAYER_TITLE           200
#define MUSICPLAYER_ALBUM           201
#define MUSICPLAYER_ARTIST          202
#define MUSICPLAYER_GENRE           203
#define MUSICPLAYER_YEAR            204
#define MUSICPLAYER_TRACK_NUMBER    208
#define MUSICPLAYER_COVER           210
#define MUSICPLAYER_BITRATE         211
#define MUSICPLAYER_PLAYLISTLEN     212
#define MUSICPLAYER_PLAYLISTPOS     213
#define MUSICPLAYER_CHANNELS        214
#define MUSICPLAYER_BITSPERSAMPLE   215
#define MUSICPLAYER_SAMPLERATE      216
#define MUSICPLAYER_CODEC           217
#define MUSICPLAYER_DISC_NUMBER     218
#define MUSICPLAYER_RATING          219
#define MUSICPLAYER_COMMENT         220
#define MUSICPLAYER_LYRICS          221

#define VIDEOPLAYER_TITLE           250
#define VIDEOPLAYER_GENRE           251
#define VIDEOPLAYER_DIRECTOR        252
#define VIDEOPLAYER_YEAR            253
#define VIDEOPLAYER_COVER           258
#define VIDEOPLAYER_USING_OVERLAYS  259
#define VIDEOPLAYER_ISFULLSCREEN    260
#define VIDEOPLAYER_HASMENU         261
#define VIDEOPLAYER_PLAYLISTLEN     262
#define VIDEOPLAYER_PLAYLISTPOS     263
#define VIDEOPLAYER_EVENT           264
#define VIDEOPLAYER_ORIGINALTITLE   265
#define VIDEOPLAYER_PLOT            266
#define VIDEOPLAYER_PLOT_OUTLINE    267
#define VIDEOPLAYER_EPISODE         268
#define VIDEOPLAYER_SEASON          269
#define VIDEOPLAYER_RATING          270
#define VIDEOPLAYER_TVSHOW          271
#define VIDEOPLAYER_PREMIERED       272
#define VIDEOPLAYER_CONTENT         273
#define VIDEOPLAYER_STUDIO          274
#define VIDEOPLAYER_MPAA            275
#define VIDEOPLAYER_CAST            276
#define VIDEOPLAYER_CAST_AND_ROLE   277
#define VIDEOPLAYER_ARTIST          278
#define VIDEOPLAYER_ALBUM           279
#define VIDEOPLAYER_WRITER          280
#define VIDEOPLAYER_TAGLINE         281
#define VIDEOPLAYER_HAS_INFO        282
#define VIDEOPLAYER_TOP250          283

#define AUDIOSCROBBLER_ENABLED      300
#define AUDIOSCROBBLER_CONN_STATE   301
#define AUDIOSCROBBLER_SUBMIT_INT   302
#define AUDIOSCROBBLER_FILES_CACHED 303
#define AUDIOSCROBBLER_SUBMIT_STATE 304
#define LASTFM_RADIOPLAYING         305
#define LASTFM_CANLOVE              306
#define LASTFM_CANBAN               307


#define CONTAINER_FOLDERTHUMB       360
#define CONTAINER_FOLDERPATH        361
#define CONTAINER_CONTENT           362
#define CONTAINER_HAS_THUMB         363
#define CONTAINER_SORT_METHOD       364
#define CONTAINER_ON_NEXT           365
#define CONTAINER_ON_PREVIOUS       366
#define CONTAINER_HAS_FOCUS         367
#define CONTAINER_ROW               368
#define CONTAINER_COLUMN            369
#define CONTAINER_POSITION          370
#define CONTAINER_VIEWMODE          371
#define CONTAINER_HAS_NEXT          372
#define CONTAINER_HAS_PREVIOUS      373
#define CONTAINER_SUBITEM           374

#define MUSICPM_ENABLED             380
#define MUSICPM_SONGSPLAYED         381
#define MUSICPM_MATCHINGSONGS       382
#define MUSICPM_MATCHINGSONGSPICKED 383
#define MUSICPM_MATCHINGSONGSLEFT   384
#define MUSICPM_RELAXEDSONGSPICKED  385
#define MUSICPM_RANDOMSONGSPICKED   386

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

#define STRING_IS_EMPTY             410

#define SKIN_HAS_THEME_START        500
#define SKIN_HAS_THEME_END          599 // allow for max 100 themes

#define SKIN_BOOL                   600
#define SKIN_STRING                 601
#define SKIN_HAS_MUSIC_OVERLAY      602
#define SKIN_HAS_VIDEO_OVERLAY      603

#define SYSTEM_TOTAL_MEMORY         644
#define SYSTEM_CPU_USAGE            645
#define SYSTEM_USED_MEMORY_PERCENT  646
#define SYSTEM_USED_MEMORY          647
#define SYSTEM_FREE_MEMORY          648
#define SYSTEM_FREE_MEMORY_PERCENT  649
#define SYSTEM_DVD_MODEL            650
#define SYSTEM_DVD_FIRMWARE         651
#define SYSTEM_HDD_BOOTDATE         652
#define SYSTEM_HDD_CYCLECOUNT       653
#define SYSTEM_UPTIME               654
#define SYSTEM_TOTALUPTIME          655
#define SYSTEM_CPUFREQUENCY         656
#define SYSTEM_XBOX_VERSION         657
#define SYSTEM_AV_PACK_INFO         658
#define SYSTEM_SCREEN_RESOLUTION    659
#define SYSTEM_VIDEO_ENCODER_INFO   660
#define SYSTEM_XBOX_SERIAL          661
#define SYSTEM_CONTROLLER_PORT_1    662
#define SYSTEM_CONTROLLER_PORT_2    663
#define SYSTEM_CONTROLLER_PORT_3    664
#define SYSTEM_CONTROLLER_PORT_4    665
#define SYSTEM_MPLAYER_VERSION      666
#define SYSTEM_KERNEL_VERSION       667
#define SYSTEM_XBE_REGION           668
#define SYSTEM_DVD_ZONE             669
#define SYSTEM_XBOX_PRODUCE_INFO    670
#define SYSTEM_XBOX_BIOS            671
#define SYSTEM_XBOX_MODCHIP         672
#define SYSTEM_USED_SPACE_X         673
#define SYSTEM_FREE_SPACE_X         674
#define SYSTEM_USED_SPACE_Y         675
#define SYSTEM_FREE_SPACE_Y         676
#define SYSTEM_USED_SPACE_Z         677
#define SYSTEM_FREE_SPACE_Z         678
#define SYSTEM_FREE_SPACE           679
#define SYSTEM_USED_SPACE           680
#define SYSTEM_TOTAL_SPACE          681
#define SYSTEM_USED_SPACE_PERCENT   682
#define SYSTEM_FREE_SPACE_PERCENT   683
#define SYSTEM_USED_SPACE_C         684
#define SYSTEM_TOTAL_SPACE_C        685
#define SYSTEM_USED_SPACE_PERCENT_C 686
#define SYSTEM_FREE_SPACE_PERCENT_C 687
#define SYSTEM_USED_SPACE_E         688
#define SYSTEM_TOTAL_SPACE_E        689
#define SYSTEM_USED_SPACE_PERCENT_E 690
#define SYSTEM_FREE_SPACE_PERCENT_E 691
#define SYSTEM_USED_SPACE_F         692
#define SYSTEM_TOTAL_SPACE_F        693
#define SYSTEM_USED_SPACE_PERCENT_F 694
#define SYSTEM_FREE_SPACE_PERCENT_F 695
#define SYSTEM_USED_SPACE_G         696
#define SYSTEM_TOTAL_SPACE_G        697
#define SYSTEM_USED_SPACE_PERCENT_G 698
#define SYSTEM_FREE_SPACE_PERCENT_G 699
#define SYSTEM_DVD_TRAY_STATE       700
#define SYSTEM_TOTAL_SPACE_X        701
#define SYSTEM_TOTAL_SPACE_Y        702
#define SYSTEM_TOTAL_SPACE_Z        703

#define SYSTEM_PLATFORM_XBOX        740
#define SYSTEM_PLATFORM_LINUX       741
#define SYSTEM_PLATFORM_WINDOWS     742

#define XLINK_KAI_USERNAME          750
#define SKIN_THEME                  800
#define SKIN_COLOUR_THEME           801

#define SLIDE_INFO_START            900
#define SLIDE_INFO_END              980

#define WINDOW_IS_TOPMOST           9994
#define WINDOW_IS_VISIBLE           9995
#define WINDOW_NEXT                 9996
#define WINDOW_PREVIOUS             9997
#define WINDOW_IS_MEDIA             9998
#define WINDOW_IS_ACTIVE            9999

#define SYSTEM_IDLE_TIME_START      20000
#define SYSTEM_IDLE_TIME_FINISH     21000 // 1000 seconds

#define CONTROL_IS_ENABLED          29997
#define CONTROL_IS_VISIBLE          29998
#define CONTROL_GROUP_HAS_FOCUS     29999
#define CONTROL_HAS_FOCUS           30000
#define BUTTON_SCROLLER_HAS_ICON    30001

#define VERSION_STRING "pre-2.1"

#define LISTITEM_START              35000
#define LISTITEM_THUMB              (LISTITEM_START)
#define LISTITEM_LABEL              (LISTITEM_START + 1)
#define LISTITEM_TITLE              (LISTITEM_START + 2)
#define LISTITEM_TRACKNUMBER        (LISTITEM_START + 3)
#define LISTITEM_ARTIST             (LISTITEM_START + 4)
#define LISTITEM_ALBUM              (LISTITEM_START + 5)
#define LISTITEM_YEAR               (LISTITEM_START + 6)
#define LISTITEM_GENRE              (LISTITEM_START + 7)
#define LISTITEM_ICON               (LISTITEM_START + 8)
#define LISTITEM_DIRECTOR           (LISTITEM_START + 9)
#define LISTITEM_OVERLAY            (LISTITEM_START + 10)
#define LISTITEM_LABEL2             (LISTITEM_START + 11)
#define LISTITEM_FILENAME           (LISTITEM_START + 12)
#define LISTITEM_DATE               (LISTITEM_START + 13)
#define LISTITEM_SIZE               (LISTITEM_START + 14)
#define LISTITEM_RATING             (LISTITEM_START + 15)
#define LISTITEM_PROGRAM_COUNT      (LISTITEM_START + 16)
#define LISTITEM_DURATION           (LISTITEM_START + 17)
#define LISTITEM_ISPLAYING          (LISTITEM_START + 18)
#define LISTITEM_ISSELECTED         (LISTITEM_START + 19)
#define LISTITEM_PLOT               (LISTITEM_START + 20)
#define LISTITEM_PLOT_OUTLINE       (LISTITEM_START + 21)
#define LISTITEM_EPISODE            (LISTITEM_START + 22)
#define LISTITEM_SEASON             (LISTITEM_START + 23)
#define LISTITEM_TVSHOW             (LISTITEM_START + 24)
#define LISTITEM_PREMIERED          (LISTITEM_START + 25)
#define LISTITEM_COMMENT            (LISTITEM_START + 26)
#define LISTITEM_ACTUAL_ICON        (LISTITEM_START + 27)
#define LISTITEM_PATH               (LISTITEM_START + 28)
#define LISTITEM_PICTURE_PATH       (LISTITEM_START + 29)
#define LISTITEM_PICTURE_DATETIME   (LISTITEM_START + 30)
#define LISTITEM_PICTURE_RESOLUTION (LISTITEM_START + 31)
#define LISTITEM_STUDIO             (LISTITEM_START + 32)
#define LISTITEM_MPAA               (LISTITEM_START + 33)
#define LISTITEM_CAST               (LISTITEM_START + 34)
#define LISTITEM_CAST_AND_ROLE      (LISTITEM_START + 35)
#define LISTITEM_WRITER             (LISTITEM_START + 36)
#define LISTITEM_TAGLINE            (LISTITEM_START + 37)
#define LISTITEM_TOP250             (LISTITEM_START + 38)

#define LISTITEM_PROPERTY_START     (LISTITEM_START + 200)
#define LISTITEM_PROPERTY_END       (LISTITEM_PROPERTY_START + 1000)
#define LISTITEM_END                (LISTITEM_PROPERTY_END)

// the multiple information vector
#define MULTI_INFO_START              40000
#define MULTI_INFO_END                41000 // 1000 references is all we have for now
#define COMBINED_VALUES_START        100000

// forward
class CInfoPortion;
class CGUIWindow;

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
class CGUIInfoManager : public IMsgTargetCallback
{
public:
  CGUIInfoManager(void);
  virtual ~CGUIInfoManager(void);

  void Clear();
  virtual bool OnMessage(CGUIMessage &message);

  int TranslateString(const CStdString &strCondition);
  bool GetBool(int condition, DWORD dwContextWindow = 0, const CGUIListItem *item=NULL);
  int GetInt(int info, DWORD contextWindow = 0) const;
  CStdString GetLabel(int info, DWORD contextWindow = 0);

  CStdString GetImage(int info, DWORD contextWindow);

  CStdString GetTime(TIME_FORMAT format = TIME_FORMAT_GUESS) const;
  CStdString GetDate(bool bNumbersOnly = false);
  CStdString GetDuration(TIME_FORMAT format = TIME_FORMAT_GUESS) const;

  void SetCurrentItem(CFileItem &item);
  void ResetCurrentItem();
  // Current song stuff
  /// \brief Retrieves tag info (if necessary) and fills in our current song path.
  void SetCurrentSong(CFileItem &item);
  void SetCurrentAlbumThumb(const CStdString thumbFileName);
  void SetCurrentMovie(CFileItem &item);
  void SetCurrentSlide(CFileItem &item);
  const CFileItem &GetCurrentSlide() const { return m_currentSlide; };
  void ResetCurrentSlide();
  void SetCurrentSongTag(const CMusicInfoTag &tag) 
  {
    CLog::Log(LOGDEBUG, "Asked to SetCurrentTag");
    *m_currentFile.GetMusicInfoTag() = tag; 
    m_currentFile.m_lStartOffset = 0;
  };
  void SetCurrentVideoTag(const CVideoInfoTag &tag) 
  {     
    *m_currentFile.GetVideoInfoTag() = tag; 
    m_currentFile.m_lStartOffset = 0;
  };

  const CMusicInfoTag *GetCurrentSongTag() const
  { 
    if (m_currentFile.HasMusicInfoTag())
      return m_currentFile.GetMusicInfoTag(); 

    return NULL;
  };
  const CVideoInfoTag* GetCurrentMovieTag() const 
  { 
    if (m_currentFile.HasVideoInfoTag())
      return m_currentFile.GetVideoInfoTag(); 
    
    return NULL;
  };

  CStdString GetMusicLabel(int item);
  CStdString GetVideoLabel(int item);
  CStdString GetPlaylistLabel(int item);
  CStdString GetMusicPartyModeLabel(int item);
  CStdString GetPictureLabel(int item) const;

  __int64 GetPlayTime() const;  // in ms
  CStdString GetCurrentPlayTime(TIME_FORMAT format = TIME_FORMAT_GUESS) const;
  int GetPlayTimeRemaining() const;
  int GetTotalPlayTime() const;
  CStdString GetCurrentPlayTimeRemaining(TIME_FORMAT format) const;
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

  string GetSystemHeatInfo(int info);

  void UpdateFPS();
  inline float GetFPS() const { return m_fps; };

  void SetNextWindow(int windowID) { m_nextWindowID = windowID; };
  void SetPreviousWindow(int windowID) { m_prevWindowID = windowID; };

  void ParseLabel(const CStdString &strLabel, vector<CInfoPortion> &multiInfo);
  CStdString GetMultiInfo(const vector<CInfoPortion> &multiInfo, DWORD contextWindow, bool preferImage = false);

  void ResetCache();

  CStdString GetItemLabel(const CFileItem *item, int info) const;
  CStdString GetItemMultiLabel(const CFileItem *item, const vector<CInfoPortion> &multiInfo);
  CStdString GetItemImage(const CFileItem *item, int info) const;

  // Called from tuxbox service thread to update current status
  void UpdateFromTuxBox();
  CStdString m_content;

  void SetLaunchingXBEName(const CStdString &name) { m_launchingXBE = name; };
  void SetContainerMoving(int id, int direction) { m_containerMoves[id] = direction; };

protected:
  // routines for window retrieval
  bool CheckWindowCondition(CGUIWindow *window, int condition) const;
  CGUIWindow *GetWindowWithCondition(DWORD contextWindow, int condition) const;

  bool GetMultiInfoBool(const GUIInfo &info, DWORD dwContextWindow = 0);
  CStdString GetMultiInfoLabel(const GUIInfo &info, DWORD dwContextWindow = 0) const;
  int TranslateSingleString(const CStdString &strCondition);
  int TranslateListItem(const CStdString &info);
  TIME_FORMAT TranslateTimeFormat(const CStdString &format);
  CStdString LocalizeTime(const CDateTime &time, TIME_FORMAT format) const;
  bool GetItemBool(const CGUIListItem *item, int condition) const;

  // Conditional string parameters for testing are stored in a vector for later retrieval.
  // The offset into the string parameters array is returned.
  int ConditionalStringParameter(const CStdString &strParameter);
  int AddMultiInfo(const GUIInfo &info);
  int AddListItemProp(const CStdString &str);

  CStdString GetAudioScrobblerLabel(int item);

  // Conditional string parameters are stored here
  CStdStringArray m_stringParameters;

  // Array of multiple information mapped to a single integer lookup
  std::vector<GUIInfo> m_multiInfo;
  std::vector<std::string> m_listitemProperties;

  CStdString m_currentMovieDuration;
  
  // Current playing stuff
  CFileItem m_currentFile;
  CStdString m_currentMovieThumb;
  unsigned int m_lastMusicBitrateTime;
  unsigned int m_MusicBitrate;
  CFileItem m_currentSlide;
  int i_SmartRequest;
 
  // fan stuff
  DWORD m_lastSysHeatInfoTime;
  int m_fanSpeed;
  CTemperature m_gpuTemp;
  CTemperature m_cpuTemp;
  
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

  CStdString m_launchingXBE;

  map<int, int> m_containerMoves;  // direction of list moving
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
  bool EvaluateBooleanExpression(const CCombinedValue &expression, bool &result, DWORD dwContextWindow, const CGUIListItem *item=NULL);

  std::vector<CCombinedValue> m_CombinedValues;

  // routines for caching the bool results
  bool IsCached(int condition, DWORD contextWindow, bool &result) const;
  void CacheBool(int condition, DWORD contextWindow, bool result);
  std::map<int, bool> m_boolCache;

  CCriticalSection m_critInfo;
};

/*!
 \ingroup strings
 \brief 
 */
extern CGUIInfoManager g_infoManager;
#endif



