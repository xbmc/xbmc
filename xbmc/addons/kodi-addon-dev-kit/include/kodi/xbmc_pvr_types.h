/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonBase.h"

#ifdef BUILD_KODI_ADDON
#include "InputStreamConstants.h"
#else
#include "cores/VideoPlayer/Interface/Addon/InputStreamConstants.h"
#endif

#ifdef USE_DEMUX
#include "DemuxPacket.h"
#else
struct DemuxPacket;
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define PVR_ADDON_NAME_STRING_LENGTH 1024
#define PVR_ADDON_URL_STRING_LENGTH 1024
#define PVR_ADDON_DESC_STRING_LENGTH 1024
#define PVR_ADDON_INPUT_FORMAT_STRING_LENGTH 32
#define PVR_ADDON_EDL_LENGTH 32
#define PVR_ADDON_TIMERTYPE_ARRAY_SIZE 32
#define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE 512
#define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE_SMALL 128
#define PVR_ADDON_TIMERTYPE_STRING_LENGTH 128
#define PVR_ADDON_ATTRIBUTE_DESC_LENGTH 128
#define PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE 512
#define PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH 64
#define PVR_ADDON_DATE_STRING_LENGTH 32

#define XBMC_INVALID_CODEC_ID 0
#define XBMC_INVALID_CODEC \
  { \
    XBMC_CODEC_TYPE_UNKNOWN, XBMC_INVALID_CODEC_ID \
  }

#define PVR_STREAM_PROPERTY_STREAMURL "streamurl"
#define PVR_STREAM_PROPERTY_INPUTSTREAM STREAM_PROPERTY_INPUTSTREAM
#define PVR_STREAM_PROPERTY_MIMETYPE "mimetype"
#define PVR_STREAM_PROPERTY_ISREALTIMESTREAM STREAM_PROPERTY_ISREALTIMESTREAM
#define PVR_STREAM_PROPERTY_EPGPLAYBACKASLIVE "epgplaybackaslive"
#define PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG

#define PVR_STREAM_MAX_STREAMS 20

#ifdef __cplusplus
extern "C" {
#endif

  typedef unsigned int xbmc_codec_id_t;

  typedef enum xbmc_codec_type_t
  {
    XBMC_CODEC_TYPE_UNKNOWN = -1,
    XBMC_CODEC_TYPE_VIDEO,
    XBMC_CODEC_TYPE_AUDIO,
    XBMC_CODEC_TYPE_DATA,
    XBMC_CODEC_TYPE_SUBTITLE,
    XBMC_CODEC_TYPE_RDS,
    XBMC_CODEC_TYPE_NB
  } xbmc_codec_type_t;

  typedef struct xbmc_codec_t
  {
    enum xbmc_codec_type_t codec_type;
    xbmc_codec_id_t codec_id;
  } xbmc_codec_t;

  typedef enum EPG_EVENT_CONTENTMASK
  {
    EPG_EVENT_CONTENTMASK_UNDEFINED = 0x00,
    EPG_EVENT_CONTENTMASK_MOVIEDRAMA = 0x10,
    EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS = 0x20,
    EPG_EVENT_CONTENTMASK_SHOW = 0x30,
    EPG_EVENT_CONTENTMASK_SPORTS = 0x40,
    EPG_EVENT_CONTENTMASK_CHILDRENYOUTH = 0x50,
    EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE = 0x60,
    EPG_EVENT_CONTENTMASK_ARTSCULTURE = 0x70,
    EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS = 0x80,
    EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE = 0x90,
    EPG_EVENT_CONTENTMASK_LEISUREHOBBIES = 0xA0,
    EPG_EVENT_CONTENTMASK_SPECIAL = 0xB0,
    EPG_EVENT_CONTENTMASK_USERDEFINED = 0xF0
  } EPG_EVENT_CONTENTMASK;

  #define EPG_GENRE_USE_STRING 0x100
  #define EPG_STRING_TOKEN_SEPARATOR ","

  #define EPG_TAG_FLAG_UNDEFINED 0x00000000
  #define EPG_TAG_FLAG_IS_SERIES 0x00000001
  #define EPG_TAG_FLAG_IS_NEW 0x00000002
  #define EPG_TAG_FLAG_IS_PREMIERE 0x00000004
  #define EPG_TAG_FLAG_IS_FINALE 0x00000008
  #define EPG_TAG_FLAG_IS_LIVE 0x00000010

  #define EPG_TAG_INVALID_UID 0

  #define EPG_TAG_INVALID_SERIES_EPISODE -1

  typedef enum EPG_EVENT_STATE
  {
    EPG_EVENT_CREATED = 0,
    EPG_EVENT_UPDATED = 1,
    EPG_EVENT_DELETED = 2,
  } EPG_EVENT_STATE;

  typedef struct EPG_TAG
  {
    unsigned int iUniqueBroadcastId;
    unsigned int iUniqueChannelId;
    const char* strTitle;
    time_t startTime;
    time_t endTime;
    const char* strPlotOutline;
    const char* strPlot;
    const char* strOriginalTitle;
    const char* strCast;
    const char* strDirector;
    const char* strWriter;
    int iYear;
    const char* strIMDBNumber;
    const char* strIconPath;
    int iGenreType;
    int iGenreSubType;
    const char* strGenreDescription;
    const char* strFirstAired;
    int iParentalRating;
    int iStarRating;
    int iSeriesNumber;
    int iEpisodeNumber;
    int iEpisodePartNumber;
    const char* strEpisodeName;
    unsigned int iFlags;
    const char* strSeriesLink;
  } ATTRIBUTE_PACKED EPG_TAG;

  #define PVR_TIMER_TYPE_NONE 0
  #define PVR_TIMER_NO_CLIENT_INDEX 0
  #define PVR_TIMER_NO_PARENT PVR_TIMER_NO_CLIENT_INDEX
  #define PVR_TIMER_NO_EPG_UID EPG_TAG_INVALID_UID
  #define PVR_TIMER_ANY_CHANNEL -1

  #define PVR_TIMER_TYPE_ATTRIBUTE_NONE 0x00000000

  #define PVR_TIMER_TYPE_IS_MANUAL 0x00000001
  #define PVR_TIMER_TYPE_IS_REPEATING 0x00000002
  #define PVR_TIMER_TYPE_IS_READONLY 0x00000004
  #define PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES 0x00000008
  #define PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE 0x00000010
  #define PVR_TIMER_TYPE_SUPPORTS_CHANNELS 0x00000020
  #define PVR_TIMER_TYPE_SUPPORTS_START_TIME 0x00000040
  #define PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH 0x00000080
  #define PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH 0x00000100
  #define PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY 0x00000200
  #define PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS 0x00000400
  #define PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES 0x00000800
  #define PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN 0x00001000
  #define PVR_TIMER_TYPE_SUPPORTS_PRIORITY 0x00002000
  #define PVR_TIMER_TYPE_SUPPORTS_LIFETIME 0x00004000
  #define PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS 0x00008000
  #define PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP 0x00010000
  #define PVR_TIMER_TYPE_SUPPORTS_END_TIME 0x00020000
  #define PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME 0x00040000
  #define PVR_TIMER_TYPE_SUPPORTS_END_ANYTIME 0x00080000
  #define PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS 0x00100000
  #define PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE 0x00200000
  #define PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE 0x00400000
  #define PVR_TIMER_TYPE_REQUIRES_EPG_SERIES_ON_CREATE 0x00800000
  #define PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL 0x01000000
  #define PVR_TIMER_TYPE_REQUIRES_EPG_SERIESLINK_ON_CREATE 0x02000000
  #define PVR_TIMER_TYPE_SUPPORTS_READONLY_DELETE 0x04000000
  #define PVR_TIMER_TYPE_IS_REMINDER 0x08000000
  #define PVR_TIMER_TYPE_SUPPORTS_START_MARGIN 0x10000000
  #define PVR_TIMER_TYPE_SUPPORTS_END_MARGIN 0x20000000

  #define PVR_WEEKDAY_NONE 0x00
  #define PVR_WEEKDAY_MONDAY 0x01
  #define PVR_WEEKDAY_TUESDAY 0x02
  #define PVR_WEEKDAY_WEDNESDAY 0x04
  #define PVR_WEEKDAY_THURSDAY 0x08
  #define PVR_WEEKDAY_FRIDAY 0x10
  #define PVR_WEEKDAY_SATURDAY 0x20
  #define PVR_WEEKDAY_SUNDAY 0x40
  #define PVR_WEEKDAY_ALLDAYS \
    ( \
      PVR_WEEKDAY_MONDAY | PVR_WEEKDAY_TUESDAY | PVR_WEEKDAY_WEDNESDAY | PVR_WEEKDAY_THURSDAY | \
      PVR_WEEKDAY_FRIDAY | PVR_WEEKDAY_SATURDAY | PVR_WEEKDAY_SUNDAY \
    )


  #define EPG_TIMEFRAME_UNLIMITED -1
  #define PVR_CHANNEL_INVALID_UID -1
  #define PVR_DESCRAMBLE_INFO_NOT_AVAILABLE -1

  typedef enum PVR_ERROR
  {
    PVR_ERROR_NO_ERROR = 0,
    PVR_ERROR_UNKNOWN = -1,
    PVR_ERROR_NOT_IMPLEMENTED = -2,
    PVR_ERROR_SERVER_ERROR = -3,
    PVR_ERROR_SERVER_TIMEOUT = -4,
    PVR_ERROR_REJECTED = -5,
    PVR_ERROR_ALREADY_PRESENT = -6,
    PVR_ERROR_INVALID_PARAMETERS = -7,
    PVR_ERROR_RECORDING_RUNNING = -8,
    PVR_ERROR_FAILED = -9,
  } PVR_ERROR;

  typedef enum PVR_TIMER_STATE
  {
    PVR_TIMER_STATE_NEW = 0,
    PVR_TIMER_STATE_SCHEDULED = 1,
    PVR_TIMER_STATE_RECORDING = 2,
    PVR_TIMER_STATE_COMPLETED = 3,
    PVR_TIMER_STATE_ABORTED = 4,
    PVR_TIMER_STATE_CANCELLED = 5,
    PVR_TIMER_STATE_CONFLICT_OK = 6,
    PVR_TIMER_STATE_CONFLICT_NOK = 7,
    PVR_TIMER_STATE_ERROR = 8,
    PVR_TIMER_STATE_DISABLED = 9,
  } PVR_TIMER_STATE;

  typedef enum PVR_MENUHOOK_CAT
  {
    PVR_MENUHOOK_UNKNOWN = -1,
    PVR_MENUHOOK_ALL = 0,
    PVR_MENUHOOK_CHANNEL = 1,
    PVR_MENUHOOK_TIMER = 2,
    PVR_MENUHOOK_EPG = 3,
    PVR_MENUHOOK_RECORDING = 4,
    PVR_MENUHOOK_DELETED_RECORDING = 5,
    PVR_MENUHOOK_SETTING = 6,
  } PVR_MENUHOOK_CAT;

  typedef enum PVR_CONNECTION_STATE
  {
    PVR_CONNECTION_STATE_UNKNOWN = 0,
    PVR_CONNECTION_STATE_SERVER_UNREACHABLE = 1,
    PVR_CONNECTION_STATE_SERVER_MISMATCH = 2,
    PVR_CONNECTION_STATE_VERSION_MISMATCH = 3,
    PVR_CONNECTION_STATE_ACCESS_DENIED = 4,
    PVR_CONNECTION_STATE_CONNECTED = 5,
    PVR_CONNECTION_STATE_DISCONNECTED = 6,
    PVR_CONNECTION_STATE_CONNECTING = 7,
  } PVR_CONNECTION_STATE;

  typedef enum PVR_RECORDING_CHANNEL_TYPE
  {
    PVR_RECORDING_CHANNEL_TYPE_UNKNOWN = 0,
    PVR_RECORDING_CHANNEL_TYPE_TV = 1,
    PVR_RECORDING_CHANNEL_TYPE_RADIO = 2,
  } PVR_RECORDING_CHANNEL_TYPE;

  typedef struct PVR_NAMED_VALUE
  {
    char strName[PVR_ADDON_NAME_STRING_LENGTH];
    char strValue[PVR_ADDON_NAME_STRING_LENGTH];
  } ATTRIBUTE_PACKED PVR_NAMED_VALUE;

  typedef struct AddonProperties_PVR
  {
    const char* strUserPath;
    const char* strClientPath;
    int iEpgMaxDays;
  } AddonProperties_PVR;

  typedef struct PVR_ATTRIBUTE_INT_VALUE
  {
    int iValue;
    char strDescription[PVR_ADDON_ATTRIBUTE_DESC_LENGTH];
  } ATTRIBUTE_PACKED PVR_ATTRIBUTE_INT_VALUE;

  typedef struct PVR_ADDON_CAPABILITIES
  {
    bool bSupportsEPG;
    bool bSupportsEPGEdl;
    bool bSupportsTV;
    bool bSupportsRadio;
    bool bSupportsRecordings;
    bool bSupportsRecordingsUndelete;
    bool bSupportsTimers;
    bool bSupportsChannelGroups;
    bool bSupportsChannelScan;
    bool bSupportsChannelSettings;
    bool bHandlesInputStream;
    bool bHandlesDemuxing;
    bool bSupportsRecordingPlayCount;
    bool bSupportsLastPlayedPosition;
    bool bSupportsRecordingEdl;
    bool bSupportsRecordingsRename;
    bool bSupportsRecordingsLifetimeChange;
    bool bSupportsDescrambleInfo;
    bool bSupportsAsyncEPGTransfer;
    bool bSupportsRecordingSize;

    unsigned int iRecordingsLifetimesSize;
    PVR_ATTRIBUTE_INT_VALUE recordingsLifetimeValues[PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE];
  } ATTRIBUTE_PACKED PVR_ADDON_CAPABILITIES;

  typedef struct PVR_STREAM_PROPERTIES
  {
    unsigned int iStreamCount;
    struct PVR_STREAM
    {
      unsigned int iPID;
      xbmc_codec_type_t iCodecType;
      xbmc_codec_id_t iCodecId;
      char strLanguage[4];
      int iSubtitleInfo;
      int iFPSScale;
      int iFPSRate;
      int iHeight;
      int iWidth;
      float fAspect;
      int iChannels;
      int iSampleRate;
      int iBlockAlign;
      int iBitRate;
      int iBitsPerSample;
    } stream[PVR_STREAM_MAX_STREAMS];
  } ATTRIBUTE_PACKED PVR_STREAM_PROPERTIES;

  typedef struct PVR_SIGNAL_STATUS
  {
    char strAdapterName[PVR_ADDON_NAME_STRING_LENGTH];
    char strAdapterStatus[PVR_ADDON_NAME_STRING_LENGTH];
    char strServiceName[PVR_ADDON_NAME_STRING_LENGTH];
    char strProviderName[PVR_ADDON_NAME_STRING_LENGTH];
    char strMuxName[PVR_ADDON_NAME_STRING_LENGTH];
    int iSNR;
    int iSignal;
    long iBER;
    long iUNC;
  } ATTRIBUTE_PACKED PVR_SIGNAL_STATUS;

  typedef struct PVR_DESCRAMBLE_INFO
  {
    int iPid;
    int iCaid;
    int iProvid;
    int iEcmTime;
    int iHops;
    char strCardSystem[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strReader[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strFrom[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strProtocol[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
  } ATTRIBUTE_PACKED PVR_DESCRAMBLE_INFO;

  typedef struct PVR_MENUHOOK
  {
    unsigned int iHookId;
    unsigned int iLocalizedStringId;
    enum PVR_MENUHOOK_CAT category;
  } ATTRIBUTE_PACKED PVR_MENUHOOK;

  #define PVR_CHANNEL_UNKNOWN_ORDER 0

  typedef struct PVR_CHANNEL
  {
    unsigned int iUniqueId;
    bool bIsRadio;
    unsigned int iChannelNumber;
    unsigned int iSubChannelNumber;
    char strChannelName[PVR_ADDON_NAME_STRING_LENGTH];
    char strInputFormat[PVR_ADDON_INPUT_FORMAT_STRING_LENGTH];
    unsigned int iEncryptionSystem;
    char strIconPath[PVR_ADDON_URL_STRING_LENGTH];
    bool bIsHidden;
    bool bHasArchive;
    int iOrder;
  } ATTRIBUTE_PACKED PVR_CHANNEL;

  typedef struct PVR_CHANNEL_GROUP
  {
    char strGroupName[PVR_ADDON_NAME_STRING_LENGTH];
    bool bIsRadio;
    unsigned int iPosition;
  } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP;

  typedef struct PVR_CHANNEL_GROUP_MEMBER
  {
    char strGroupName[PVR_ADDON_NAME_STRING_LENGTH];
    unsigned int iChannelUniqueId;
    unsigned int iChannelNumber;
    unsigned int iSubChannelNumber;
    int iOrder;
  } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP_MEMBER;

  typedef PVR_ATTRIBUTE_INT_VALUE PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE;

  typedef struct PVR_TIMER_TYPE
  {
    unsigned int iId;
    unsigned int iAttributes;
    char strDescription[PVR_ADDON_TIMERTYPE_STRING_LENGTH];

    unsigned int iPrioritiesSize;
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE priorities[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
    int iPrioritiesDefault;

    unsigned int iLifetimesSize;
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE lifetimes[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
    int iLifetimesDefault;

    unsigned int iPreventDuplicateEpisodesSize;
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE
    preventDuplicateEpisodes[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
    unsigned int iPreventDuplicateEpisodesDefault;

    unsigned int iRecordingGroupSize;
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE recordingGroup[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
    unsigned int iRecordingGroupDefault;

    unsigned int iMaxRecordingsSize;
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE maxRecordings[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE_SMALL];
    int iMaxRecordingsDefault;
  } ATTRIBUTE_PACKED PVR_TIMER_TYPE;

  typedef struct PVR_TIMER
  {
    unsigned int iClientIndex;
    unsigned int iParentClientIndex;
    int iClientChannelUid;
    time_t startTime;
    time_t endTime;
    bool bStartAnyTime;
    bool bEndAnyTime;
    enum PVR_TIMER_STATE state;
    unsigned int iTimerType;
    char strTitle[PVR_ADDON_NAME_STRING_LENGTH];
    char strEpgSearchString[PVR_ADDON_NAME_STRING_LENGTH];
    bool bFullTextEpgSearch;
    char strDirectory[PVR_ADDON_URL_STRING_LENGTH];
    char strSummary[PVR_ADDON_DESC_STRING_LENGTH];
    int iPriority;
    int iLifetime;
    int iMaxRecordings;
    unsigned int iRecordingGroup;
    time_t firstDay;
    unsigned int iWeekdays;
    unsigned int iPreventDuplicateEpisodes;
    unsigned int iEpgUid;
    unsigned int iMarginStart;
    unsigned int iMarginEnd;
    int iGenreType;
    int iGenreSubType;
    char strSeriesLink[PVR_ADDON_URL_STRING_LENGTH];
  } ATTRIBUTE_PACKED PVR_TIMER;

  /* PVR_RECORDING.iFlags values */
  #define PVR_RECORDING_FLAG_UNDEFINED 0x00000000
  #define PVR_RECORDING_FLAG_IS_SERIES 0x00000001
  #define PVR_RECORDING_FLAG_IS_NEW 0x00000002
  #define PVR_RECORDING_FLAG_IS_PREMIERE 0x00000004
  #define PVR_RECORDING_FLAG_IS_FINALE 0x00000008
  #define PVR_RECORDING_FLAG_IS_LIVE 0x00000010

  #define PVR_RECORDING_INVALID_SERIES_EPISODE EPG_TAG_INVALID_SERIES_EPISODE

  typedef struct PVR_RECORDING
  {
    char strRecordingId[PVR_ADDON_NAME_STRING_LENGTH];
    char strTitle[PVR_ADDON_NAME_STRING_LENGTH];
    char strEpisodeName[PVR_ADDON_NAME_STRING_LENGTH];
    int iSeriesNumber;
    int iEpisodeNumber;
    int iYear;
    char strDirectory[PVR_ADDON_URL_STRING_LENGTH];
    char strPlotOutline[PVR_ADDON_DESC_STRING_LENGTH];
    char strPlot[PVR_ADDON_DESC_STRING_LENGTH];
    char strGenreDescription[PVR_ADDON_DESC_STRING_LENGTH];
    char strChannelName[PVR_ADDON_NAME_STRING_LENGTH];
    char strIconPath[PVR_ADDON_URL_STRING_LENGTH];
    char strThumbnailPath[PVR_ADDON_URL_STRING_LENGTH];
    char strFanartPath[PVR_ADDON_URL_STRING_LENGTH];
    time_t recordingTime;
    int iDuration;
    int iPriority;
    int iLifetime;
    int iGenreType;
    int iGenreSubType;
    int iPlayCount;
    int iLastPlayedPosition;
    bool bIsDeleted;
    unsigned int iEpgEventId;
    int iChannelUid;
    PVR_RECORDING_CHANNEL_TYPE channelType;
    char strFirstAired[PVR_ADDON_DATE_STRING_LENGTH];
    unsigned int iFlags;
    int64_t sizeInBytes;
  } ATTRIBUTE_PACKED PVR_RECORDING;

  typedef enum PVR_EDL_TYPE
  {
    PVR_EDL_TYPE_CUT = 0,
    PVR_EDL_TYPE_MUTE = 1,
    PVR_EDL_TYPE_SCENE = 2,
    PVR_EDL_TYPE_COMBREAK = 3
  } PVR_EDL_TYPE;

  typedef struct PVR_EDL_ENTRY
  {
    int64_t start;
    int64_t end;
    enum PVR_EDL_TYPE type;
  } ATTRIBUTE_PACKED PVR_EDL_ENTRY;

  typedef struct PVR_MENUHOOK_DATA
  {
    enum PVR_MENUHOOK_CAT cat;
    union data {
      int iEpgUid;
      struct PVR_CHANNEL channel;
      struct PVR_TIMER timer;
      struct PVR_RECORDING recording;
    } data;
  } ATTRIBUTE_PACKED PVR_MENUHOOK_DATA;

  typedef struct PVR_STREAM_TIMES
  {
    time_t startTime;
    int64_t ptsStart;
    int64_t ptsBegin;
    int64_t ptsEnd;
  } ATTRIBUTE_PACKED PVR_STREAM_TIMES;

  struct AddonInstance_PVR;

  typedef struct AddonToKodiFuncTable_PVR
  {
    // Pointer inside Kodi where used from him to find his class
    KODI_HANDLE kodiInstance;

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // General callback functions
    void (*AddMenuHook)(void* kodiInstance, PVR_MENUHOOK* hook);
    void (*Recording)(void* kodiInstance, const char* Name, const char* FileName, bool On);
    void (*ConnectionStateChange)(void* kodiInstance,
                                  const char* strConnectionString,
                                  enum PVR_CONNECTION_STATE newState,
                                  const char* strMessage);
    void (*EpgEventStateChange)(void* kodiInstance,
                                struct EPG_TAG* tag,
                                enum EPG_EVENT_STATE newState);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Transfer functions where give data back to Kodi, e.g. GetChannels calls TransferChannelEntry
    void (*TransferChannelEntry)(void* kodiInstance,
                                 const ADDON_HANDLE handle,
                                 const struct PVR_CHANNEL* chan);
    void (*TransferChannelGroup)(void* kodiInstance,
                                 const ADDON_HANDLE handle,
                                 const struct PVR_CHANNEL_GROUP* group);
    void (*TransferChannelGroupMember)(void* kodiInstance,
                                       const ADDON_HANDLE handle,
                                       const struct PVR_CHANNEL_GROUP_MEMBER* member);
    void (*TransferEpgEntry)(void* kodiInstance,
                             const ADDON_HANDLE handle,
                             const struct EPG_TAG* epgentry);
    void (*TransferRecordingEntry)(void* kodiInstance,
                                   const ADDON_HANDLE handle,
                                   const struct PVR_RECORDING* recording);
    void (*TransferTimerEntry)(void* kodiInstance,
                               const ADDON_HANDLE handle,
                               const struct PVR_TIMER* timer);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Kodi inform interface functions
    void (*TriggerChannelUpdate)(void* kodiInstance);
    void (*TriggerChannelGroupsUpdate)(void* kodiInstance);
    void (*TriggerEpgUpdate)(void* kodiInstance, unsigned int iChannelUid);
    void (*TriggerRecordingUpdate)(void* kodiInstance);
    void (*TriggerTimerUpdate)(void* kodiInstance);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Stream demux interface functions
    void (*FreeDemuxPacket)(void* kodiInstance, struct DemuxPacket* pPacket);
    struct DemuxPacket* (*AllocateDemuxPacket)(void* kodiInstance, int iDataSize);
    struct xbmc_codec_t (*GetCodecByName)(const void* kodiInstance, const char* strCodecName);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // New functions becomes added below and can be on another API change (where
    // breaks min API version) moved up.
  } AddonToKodiFuncTable_PVR;

  /*!
   * @brief Structure to transfer the methods from addon to Kodi
   */
  typedef struct KodiToAddonFuncTable_PVR
  {
    // Pointer inside addon where used on them to find his instance class (currently unused!)
    KODI_HANDLE addonInstance;

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // General interface functions
    enum PVR_ERROR(__cdecl* GetCapabilities)(const struct AddonInstance_PVR*,
                                             struct PVR_ADDON_CAPABILITIES*);
    const char*(__cdecl* GetBackendName)(const struct AddonInstance_PVR*);
    const char*(__cdecl* GetBackendVersion)(const struct AddonInstance_PVR*);
    const char*(__cdecl* GetBackendHostname)(const struct AddonInstance_PVR*);
    const char*(__cdecl* GetConnectionString)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* GetDriveSpace)(const struct AddonInstance_PVR*, long long*, long long*);
    enum PVR_ERROR(__cdecl* MenuHook)(const struct AddonInstance_PVR*,
                                      const struct PVR_MENUHOOK*,
                                      const struct PVR_MENUHOOK_DATA*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel interface functions

    int(__cdecl* GetChannelsAmount)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* GetChannels)(const struct AddonInstance_PVR*, ADDON_HANDLE, bool);
    enum PVR_ERROR(__cdecl* GetChannelStreamProperties)(const struct AddonInstance_PVR*,
                                                        const struct PVR_CHANNEL*,
                                                        struct PVR_NAMED_VALUE*,
                                                        unsigned int*);
    enum PVR_ERROR(__cdecl* GetSignalStatus)(const struct AddonInstance_PVR*,
                                             int,
                                             struct PVR_SIGNAL_STATUS*);
    enum PVR_ERROR(__cdecl* GetDescrambleInfo)(const struct AddonInstance_PVR*,
                                               int,
                                               struct PVR_DESCRAMBLE_INFO*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel group interface functions
    int(__cdecl* GetChannelGroupsAmount)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* GetChannelGroups)(const struct AddonInstance_PVR*, ADDON_HANDLE, bool);
    enum PVR_ERROR(__cdecl* GetChannelGroupMembers)(const struct AddonInstance_PVR*,
                                                    ADDON_HANDLE,
                                                    const struct PVR_CHANNEL_GROUP*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel edit interface functions
    enum PVR_ERROR(__cdecl* DeleteChannel)(const struct AddonInstance_PVR*,
                                           const struct PVR_CHANNEL*);
    enum PVR_ERROR(__cdecl* RenameChannel)(const struct AddonInstance_PVR*,
                                           const struct PVR_CHANNEL*);
    enum PVR_ERROR(__cdecl* OpenDialogChannelSettings)(const struct AddonInstance_PVR*,
                                                       const struct PVR_CHANNEL*);
    enum PVR_ERROR(__cdecl* OpenDialogChannelAdd)(const struct AddonInstance_PVR*,
                                                  const struct PVR_CHANNEL*);
    enum PVR_ERROR(__cdecl* OpenDialogChannelScan)(const struct AddonInstance_PVR*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // EPG interface functions
    enum PVR_ERROR(__cdecl* GetEPGForChannel)(const struct AddonInstance_PVR*,
                                              ADDON_HANDLE,
                                              int,
                                              time_t,
                                              time_t);
    enum PVR_ERROR(__cdecl* IsEPGTagRecordable)(const struct AddonInstance_PVR*,
                                                const struct EPG_TAG*,
                                                bool*);
    enum PVR_ERROR(__cdecl* IsEPGTagPlayable)(const struct AddonInstance_PVR*,
                                              const struct EPG_TAG*,
                                              bool*);
    enum PVR_ERROR(__cdecl* GetEPGTagEdl)(const struct AddonInstance_PVR*,
                                          const struct EPG_TAG*,
                                          struct PVR_EDL_ENTRY[],
                                          int*);
    enum PVR_ERROR(__cdecl* GetEPGTagStreamProperties)(const struct AddonInstance_PVR*,
                                                       const struct EPG_TAG*,
                                                       struct PVR_NAMED_VALUE*,
                                                       unsigned int*);
    enum PVR_ERROR(__cdecl* SetEPGTimeFrame)(const struct AddonInstance_PVR*, int);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Recording interface functions
    int(__cdecl* GetRecordingsAmount)(const struct AddonInstance_PVR*, bool);
    enum PVR_ERROR(__cdecl* GetRecordings)(const struct AddonInstance_PVR*, ADDON_HANDLE, bool);
    enum PVR_ERROR(__cdecl* DeleteRecording)(const struct AddonInstance_PVR*,
                                             const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* UndeleteRecording)(const struct AddonInstance_PVR*,
                                               const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* DeleteAllRecordingsFromTrash)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* RenameRecording)(const struct AddonInstance_PVR*,
                                             const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* SetRecordingLifetime)(const struct AddonInstance_PVR*,
                                                  const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* SetRecordingPlayCount)(const struct AddonInstance_PVR*,
                                                   const struct PVR_RECORDING*,
                                                   int);
    enum PVR_ERROR(__cdecl* SetRecordingLastPlayedPosition)(const struct AddonInstance_PVR*,
                                                            const struct PVR_RECORDING*,
                                                            int);
    int(__cdecl* GetRecordingLastPlayedPosition)(const struct AddonInstance_PVR*,
                                                 const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* GetRecordingEdl)(const struct AddonInstance_PVR*,
                                             const struct PVR_RECORDING*,
                                             struct PVR_EDL_ENTRY[],
                                             int*);
    enum PVR_ERROR(__cdecl* GetRecordingSize)(const struct AddonInstance_PVR*,
                                              const PVR_RECORDING*,
                                              int64_t*);
    enum PVR_ERROR(__cdecl* GetRecordingStreamProperties)(const struct AddonInstance_PVR*,
                                                          const struct PVR_RECORDING*,
                                                          struct PVR_NAMED_VALUE*,
                                                          unsigned int*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Timer interface functions
    enum PVR_ERROR(__cdecl* GetTimerTypes)(const struct AddonInstance_PVR*,
                                           struct PVR_TIMER_TYPE[],
                                           int*);
    int(__cdecl* GetTimersAmount)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* GetTimers)(const struct AddonInstance_PVR*, ADDON_HANDLE);
    enum PVR_ERROR(__cdecl* AddTimer)(const struct AddonInstance_PVR*, const struct PVR_TIMER*);
    enum PVR_ERROR(__cdecl* DeleteTimer)(const struct AddonInstance_PVR*,
                                         const struct PVR_TIMER*,
                                         bool);
    enum PVR_ERROR(__cdecl* UpdateTimer)(const struct AddonInstance_PVR*, const struct PVR_TIMER*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Powersaving interface functions
    void(__cdecl* OnSystemSleep)(const struct AddonInstance_PVR*);
    void(__cdecl* OnSystemWake)(const struct AddonInstance_PVR*);
    void(__cdecl* OnPowerSavingActivated)(const struct AddonInstance_PVR*);
    void(__cdecl* OnPowerSavingDeactivated)(const struct AddonInstance_PVR*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Live stream read interface functions
    bool(__cdecl* OpenLiveStream)(const struct AddonInstance_PVR*, const struct PVR_CHANNEL*);
    void(__cdecl* CloseLiveStream)(const struct AddonInstance_PVR*);
    int(__cdecl* ReadLiveStream)(const struct AddonInstance_PVR*, unsigned char*, unsigned int);
    long long(__cdecl* SeekLiveStream)(const struct AddonInstance_PVR*, long long, int);
    long long(__cdecl* LengthLiveStream)(const struct AddonInstance_PVR*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Recording stream read interface functions
    bool(__cdecl* OpenRecordedStream)(const struct AddonInstance_PVR*, const struct PVR_RECORDING*);
    void(__cdecl* CloseRecordedStream)(const struct AddonInstance_PVR*);
    int(__cdecl* ReadRecordedStream)(const struct AddonInstance_PVR*, unsigned char*, unsigned int);
    long long(__cdecl* SeekRecordedStream)(const struct AddonInstance_PVR*, long long, int);
    long long(__cdecl* LengthRecordedStream)(const struct AddonInstance_PVR*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Stream demux interface functions
    enum PVR_ERROR(__cdecl* GetStreamProperties)(const struct AddonInstance_PVR*,
                                                 struct PVR_STREAM_PROPERTIES*);
    struct DemuxPacket*(__cdecl* DemuxRead)(const struct AddonInstance_PVR*);
    void(__cdecl* DemuxReset)(const struct AddonInstance_PVR*);
    void(__cdecl* DemuxAbort)(const struct AddonInstance_PVR*);
    void(__cdecl* DemuxFlush)(const struct AddonInstance_PVR*);
    void(__cdecl* SetSpeed)(const struct AddonInstance_PVR*, int);
    void(__cdecl* FillBuffer)(const struct AddonInstance_PVR*, bool);
    bool(__cdecl* SeekTime)(const struct AddonInstance_PVR*, double, bool, double*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // General stream interface functions
    bool(__cdecl* CanPauseStream)(const struct AddonInstance_PVR*);
    void(__cdecl* PauseStream)(const struct AddonInstance_PVR*, bool);
    bool(__cdecl* CanSeekStream)(const struct AddonInstance_PVR*);
    bool(__cdecl* IsRealTimeStream)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* GetStreamTimes)(const struct AddonInstance_PVR*,
                                            struct PVR_STREAM_TIMES*);
    enum PVR_ERROR(__cdecl* GetStreamReadChunkSize)(const struct AddonInstance_PVR*, int*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // New functions becomes added below and can be on another API change (where
    // breaks min API version) moved up.
  } KodiToAddonFuncTable_PVR;

  typedef struct AddonInstance_PVR
  {
    struct AddonProperties_PVR* props;
    struct AddonToKodiFuncTable_PVR* toKodi;
    struct KodiToAddonFuncTable_PVR* toAddon;
  } AddonInstance_PVR;

#ifdef __cplusplus
}
#endif
