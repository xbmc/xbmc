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

/*! @note Define "USE_DEMUX" at compile time if demuxing in the PVR add-on is used.
 *        Also, "DVDDemuxPacket.h" file must be in the include path of the add-on,
 *        and the add-on should set bHandlesDemuxing to true.
 */
#ifdef USE_DEMUX
#include "DemuxPacket.h"
#else
struct DemuxPacket;
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PVR_ADDON_NAME_STRING_LENGTH          1024
#define PVR_ADDON_URL_STRING_LENGTH           1024
#define PVR_ADDON_DESC_STRING_LENGTH          1024
#define PVR_ADDON_INPUT_FORMAT_STRING_LENGTH  32
#define PVR_ADDON_EDL_LENGTH                  32
#define PVR_ADDON_TIMERTYPE_ARRAY_SIZE        32
#define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE 512
#define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE_SMALL 128
#define PVR_ADDON_TIMERTYPE_STRING_LENGTH     128
#define PVR_ADDON_ATTRIBUTE_DESC_LENGTH       128
#define PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE 512
#define PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH 64
#define PVR_ADDON_DATE_STRING_LENGTH          32

#define XBMC_INVALID_CODEC_ID   0
#define XBMC_INVALID_CODEC      { XBMC_CODEC_TYPE_UNKNOWN, XBMC_INVALID_CODEC_ID }

//============================================================================
/// @brief **PVR related stream property values**
///
/// This is used to pass additional data to Kodi on a given PVR stream.
///
/// Then transferred to livestream, recordings or EPG Tag stream using the
/// properties.
///
//@{

/// @brief the URL of the stream that should be played.
///
#define PVR_STREAM_PROPERTY_STREAMURL "streamurl"

/// @brief To define in stream properties the name of the inputstream add-on
/// that should be used.
///
/// Leave blank to use Kodi's built-in playing capabilities or to allow ffmpeg
/// to handle directly set to @ref PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG.
///
#define PVR_STREAM_PROPERTY_INPUTSTREAM STREAM_PROPERTY_INPUTSTREAM

/// @brief the MIME type of the stream that should be played.
///
#define PVR_STREAM_PROPERTY_MIMETYPE "mimetype"

/// @brief "true" to denote that the stream that should be played is a realtime
/// stream.
///
/// Any other value indicates that this is no realtime stream.
///
#define PVR_STREAM_PROPERTY_ISREALTIMESTREAM STREAM_PROPERTY_ISREALTIMESTREAM

/// @brief "true" to denote that if the stream is from an EPG tag.
///
/// It should be played is a live stream. Otherwise if it's a EPG tag it will
/// play as normal video.
///
#define PVR_STREAM_PROPERTY_EPGPLAYBACKASLIVE "epgplaybackaslive"

/// @brief Special value for @ref PVR_STREAM_PROPERTY_INPUTSTREAM to use
/// ffmpeg to directly play a stream URL.
#define PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG

//@}
//-------------------------------------------------------------------------------

/* using the default avformat's MAX_STREAMS value to be safe */
#define PVR_STREAM_MAX_STREAMS 20

#ifdef __cplusplus
extern "C" {
#endif

  typedef unsigned int xbmc_codec_id_t;

  typedef enum
  {
      XBMC_CODEC_TYPE_UNKNOWN = -1,
      XBMC_CODEC_TYPE_VIDEO,
      XBMC_CODEC_TYPE_AUDIO,
      XBMC_CODEC_TYPE_DATA,
      XBMC_CODEC_TYPE_SUBTITLE,
      XBMC_CODEC_TYPE_RDS,
      XBMC_CODEC_TYPE_NB
  } xbmc_codec_type_t;

  typedef struct
  {
    xbmc_codec_type_t codec_type;
    xbmc_codec_id_t   codec_id;
  } xbmc_codec_t;

  //--==----==----==----==----==----==----==----==----==----==----==----==----==
  //                             PVR EPG "C" definitions

  /*! @name EPG entry content event types */
  //@{
  /* These IDs come from the DVB-SI EIT table "content descriptor"
   * Also known under the name "E-book genre assignments"
   */
  typedef enum
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
  //@}

  /* Set EPGTAG.iGenreType or EPGTAG.iGenreSubType to EPG_GENRE_USE_STRING to transfer genre strings to Kodi */
  #define EPG_GENRE_USE_STRING 0x100

  /* Separator to use in strings containing different tokens, for example writers, directors, actors of an event. */
  #define EPG_STRING_TOKEN_SEPARATOR ","

  /* EPG_TAG.iFlags values */
  const unsigned int EPG_TAG_FLAG_UNDEFINED   = 0x00000000; /*!< nothing special to say about this entry */
  const unsigned int EPG_TAG_FLAG_IS_SERIES   = 0x00000001; /*!< this EPG entry is part of a series */
  const unsigned int EPG_TAG_FLAG_IS_NEW      = 0x00000002; /*!< this EPG entry will be flagged as new */
  const unsigned int EPG_TAG_FLAG_IS_PREMIERE = 0x00000004; /*!< this EPG entry will be flagged as a premiere */
  const unsigned int EPG_TAG_FLAG_IS_FINALE   = 0x00000008; /*!< this EPG entry will be flagged as a finale */
  const unsigned int EPG_TAG_FLAG_IS_LIVE     = 0x00000010; /*!< this EPG entry will be flagged as live */

  /* Special EPG_TAG.iUniqueBroadcastId value */

  /*!
   * @brief special EPG_TAG.iUniqueBroadcastId value to indicate that a tag has not a valid EPG event uid.
   */
  const unsigned int EPG_TAG_INVALID_UID = 0;

  /*!
   * @brief special EPG_TAG.iSeriesNumber, EPG_TAG.iEpisodeNumber and EPG_TAG.iEpisodePartNumber value to indicate it is not to be used
   */
  const int EPG_TAG_INVALID_SERIES_EPISODE = -1;

  /*!
   * @brief EPG event states. Used with EpgEventStateChange callback.
   */
  typedef enum
  {
    EPG_EVENT_CREATED = 0,  /*!< event created */
    EPG_EVENT_UPDATED = 1,  /*!< event updated */
    EPG_EVENT_DELETED = 2,  /*!< event deleted */
  } EPG_EVENT_STATE;

  /*!
   * @brief Representation of an EPG event.
   */
  typedef struct EPG_TAG
  {
    unsigned int  iUniqueBroadcastId;  /*!< (required) identifier for this event. Event uids must be unique for a channel. Valid uids must be greater than EPG_TAG_INVALID_UID. */
    unsigned int  iUniqueChannelId;    /*!< (required) unique identifier of the channel this event belongs to. */
    const char *  strTitle;            /*!< (required) this event's title */
    time_t        startTime;           /*!< (required) start time in UTC */
    time_t        endTime;             /*!< (required) end time in UTC */
    const char *  strPlotOutline;      /*!< (optional) plot outline */
    const char *  strPlot;             /*!< (optional) plot */
    const char *  strOriginalTitle;    /*!< (optional) originaltitle */
    const char *  strCast;             /*!< (optional) cast. Use EPG_STRING_TOKEN_SEPARATOR to separate different persons. */
    const char *  strDirector;         /*!< (optional) director(s). Use EPG_STRING_TOKEN_SEPARATOR to separate different persons. */
    const char *  strWriter;           /*!< (optional) writer(s). Use EPG_STRING_TOKEN_SEPARATOR to separate different persons. */
    int           iYear;               /*!< (optional) year */
    const char *  strIMDBNumber;       /*!< (optional) IMDBNumber */
    const char *  strIconPath;         /*!< (optional) icon path */
    int           iGenreType;          /*!< (optional) genre type */
    int           iGenreSubType;       /*!< (optional) genre sub type */
    const char *  strGenreDescription; /*!< (optional) genre. Will be used only when iGenreType == EPG_GENRE_USE_STRING or iGenreSubType == EPG_GENRE_USE_STRING. Use EPG_STRING_TOKEN_SEPARATOR to separate different genres. */
    const char *  strFirstAired;       /*!< (optional) first aired date of the event. Used only for display purposes. Specify in W3C date format "YYYY-MM-DD". */
    int           iParentalRating;     /*!< (optional) parental rating */
    int           iStarRating;         /*!< (optional) star rating */
    int           iSeriesNumber;       /*!< (optional) series number. Set to "0" for specials/pilot. For 'invalid' set to EPG_TAG_INVALID_SERIES_EPISODE */
    int           iEpisodeNumber;      /*!< (optional) episode number. For 'invalid' set to EPG_TAG_INVALID_SERIES_EPISODE */
    int           iEpisodePartNumber;  /*!< (optional) episode part number. For 'invalid' set to EPG_TAG_INVALID_SERIES_EPISODE */
    const char *  strEpisodeName;      /*!< (optional) episode name */
    unsigned int  iFlags;              /*!< (optional) bit field of independent flags associated with the EPG entry */
    const char *  strSeriesLink;       /*!< (optional) series link for this event */
  } ATTRIBUTE_PACKED EPG_TAG;

  //--==----==----==----==----==----==----==----==----==----==----==----==----==
  //                            PVR timers "C" definitions

  /*!
   * @brief numeric PVR timer type definitions (PVR_TIMER.iTimerType values)
   */
  const unsigned int PVR_TIMER_TYPE_NONE = 0; /*!< @brief "Null" value for a numeric timer type. */

  /*!
   * @brief special PVR_TIMER.iClientIndex value to indicate that a timer has not (yet) a valid client index.
   */
  const unsigned int PVR_TIMER_NO_CLIENT_INDEX = 0; /*!< @brief timer has not (yet) a valid client index. */

  /*!
   * @brief special PVR_TIMER.iParentClientIndex value to indicate that a timer has no parent.
   */
  const unsigned int PVR_TIMER_NO_PARENT = PVR_TIMER_NO_CLIENT_INDEX; /*!< @brief timer has no parent; it was not scheduled by a repeating timer. */

  /*!
   * @brief special PVR_TIMER.iEpgUid value to indicate that a timer has no EPG event uid.
   */
  const unsigned int PVR_TIMER_NO_EPG_UID = EPG_TAG_INVALID_UID; /*!< @brief timer has no EPG event uid. */

  /*!
   * @brief special PVR_TIMER.iClientChannelUid value to indicate "any channel". Useful for some repeating timer types.
   */
  const int PVR_TIMER_ANY_CHANNEL = -1; /*!< @brief denotes "any channel", not a specific one. */

  /*!
   * @brief PVR timer type attributes (PVR_TIMER_TYPE.iAttributes values)
   */
  const unsigned int PVR_TIMER_TYPE_ATTRIBUTE_NONE                    = 0x00000000;

  const unsigned int PVR_TIMER_TYPE_IS_MANUAL                         = 0x00000001; /*!< @brief defines whether this is a type for manual (time-based) or epg-based timers */
  const unsigned int PVR_TIMER_TYPE_IS_REPEATING                      = 0x00000002; /*!< @brief defines whether this is a type for repeating or one-shot timers */
  const unsigned int PVR_TIMER_TYPE_IS_READONLY                       = 0x00000004; /*!< @brief timers of this type must not be edited by Kodi */
  const unsigned int PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES             = 0x00000008; /*!< @brief timers of this type must not be created by Kodi. All other operations are allowed, though */

  const unsigned int PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE           = 0x00000010; /*!< @brief this type supports enabling/disabling of the timer (PVR_TIMER.state SCHEDULED|DISABLED) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_CHANNELS                 = 0x00000020; /*!< @brief this type supports channels (PVR_TIMER.iClientChannelUid) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_START_TIME               = 0x00000040; /*!< @brief this type supports a recording start time (PVR_TIMER.startTime) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH          = 0x00000080; /*!< @brief this type supports matching epg episode title using PVR_TIMER.strEpgSearchString */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH       = 0x00000100; /*!< @brief this type supports matching "more" epg data (not just episode title) using PVR_TIMER.strEpgSearchString. Setting FULLTEXT_EPG_MATCH implies TITLE_EPG_MATCH */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY                = 0x00000200; /*!< @brief this type supports a first day the timer gets active (PVR_TIMER.firstday) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS                 = 0x00000400; /*!< @brief this type supports weekdays for defining the recording schedule (PVR_TIMER.iWeekdays) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES = 0x00000800; /*!< @brief this type supports the "record only new episodes" feature (PVR_TIMER.iPreventDuplicateEpisodes) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN         = 0x00001000; /*!< @brief this type supports pre and post record time (PVR_TIMER.iMarginStart, PVR_TIMER.iMarginEnd) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_PRIORITY                 = 0x00002000; /*!< @brief this type supports recording priority (PVR_TIMER.iPriority) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_LIFETIME                 = 0x00004000; /*!< @brief this type supports recording lifetime (PVR_TIMER.iLifetime) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS        = 0x00008000; /*!< @brief this type supports placing recordings in user defined folders (PVR_TIMER.strDirectory) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP          = 0x00010000; /*!< @brief this type supports a list of recording groups (PVR_TIMER.iRecordingGroup) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_END_TIME                 = 0x00020000; /*!< @brief this type supports a recording end time (PVR_TIMER.endTime) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME            = 0x00040000; /*!< @brief enables an 'Any Time' over-ride option for startTime (using PVR_TIMER.bStartAnyTime) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_END_ANYTIME              = 0x00080000; /*!< @brief enables a separate 'Any Time' over-ride for endTime (using PVR_TIMER.bEndAnyTime) */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS           = 0x00100000; /*!< @brief this type supports specifying a maximum recordings setting' (PVR_TIMER.iMaxRecordings) */
  const unsigned int PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE        = 0x00200000; /*!< @brief this type should not appear on any create menus which don't provide an associated EPG tag */
  const unsigned int PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE         = 0x00400000; /*!< @brief this type should not appear on any create menus which provide an associated EPG tag */
  const unsigned int PVR_TIMER_TYPE_REQUIRES_EPG_SERIES_ON_CREATE     = 0x00800000; /*!< @brief this type should not appear on any create menus unless associated with an EPG tag with 'series' attributes (EPG_TAG.iFlags & EPG_TAG_FLAG_IS_SERIES || EPG_TAG.iSeriesNumber >= 0 || EPG_TAG.iEpisodeNumber >= 0 || EPG_TAG.iEpisodePartNumber >= 0). Implies PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL              = 0x01000000; /*!< @brief this type supports 'any channel', for example when defining a timer rule that should match any channel instaed of a particular channel */
  const unsigned int PVR_TIMER_TYPE_REQUIRES_EPG_SERIESLINK_ON_CREATE = 0x02000000; /*!< @brief this type should not appear on any create menus which don't provide an associated EPG tag with a series link */
  const unsigned int PVR_TIMER_TYPE_SUPPORTS_READONLY_DELETE          = 0x04000000; /*!< @brief this type allows deletion of an otherwise read-only timer */
  const unsigned int PVR_TIMER_TYPE_IS_REMINDER                       = 0x08000000; /*!< @brief timers of this type do trigger a reminder if time is up by calling the Kodi callback 'ReminderNotification'. */

  /*!
   * @brief PVR timer weekdays (PVR_TIMER.iWeekdays values)
   */
  const unsigned int PVR_WEEKDAY_NONE      = 0x00;
  const unsigned int PVR_WEEKDAY_MONDAY    = 0x01;
  const unsigned int PVR_WEEKDAY_TUESDAY   = 0x02;
  const unsigned int PVR_WEEKDAY_WEDNESDAY = 0x04;
  const unsigned int PVR_WEEKDAY_THURSDAY  = 0x08;
  const unsigned int PVR_WEEKDAY_FRIDAY    = 0x10;
  const unsigned int PVR_WEEKDAY_SATURDAY  = 0x20;
  const unsigned int PVR_WEEKDAY_SUNDAY    = 0x40;
  const unsigned int PVR_WEEKDAY_ALLDAYS   = PVR_WEEKDAY_MONDAY   | PVR_WEEKDAY_TUESDAY | PVR_WEEKDAY_WEDNESDAY |
                                             PVR_WEEKDAY_THURSDAY | PVR_WEEKDAY_FRIDAY  | PVR_WEEKDAY_SATURDAY  |
                                             PVR_WEEKDAY_SUNDAY;

  /*!
   * @brief timeframe value for use with SetEPGTimeFrame function to indicate "no timeframe".
   */
  const int EPG_TIMEFRAME_UNLIMITED = -1;

  /*!
   * @brief special PVR_TIMER.iClientChannelUid and PVR_RECORDING.iChannelUid value to indicate that no channel uid is available.
   */
  const int PVR_CHANNEL_INVALID_UID = -1; /*!< @brief denotes that no channel uid is available. */

  /*!
   * @brief special PVR_DESCRAMBLE_INFO value to indicate that a struct member's value is not available.
   */
  const int PVR_DESCRAMBLE_INFO_NOT_AVAILABLE = -1;

  /*!
   * @brief PVR add-on error codes
   */
  typedef enum
  {
    PVR_ERROR_NO_ERROR           = 0,  /*!< @brief no error occurred */
    PVR_ERROR_UNKNOWN            = -1, /*!< @brief an unknown error occurred */
    PVR_ERROR_NOT_IMPLEMENTED    = -2, /*!< @brief the method that Kodi called is not implemented by the add-on */
    PVR_ERROR_SERVER_ERROR       = -3, /*!< @brief the backend reported an error, or the add-on isn't connected */
    PVR_ERROR_SERVER_TIMEOUT     = -4, /*!< @brief the command was sent to the backend, but the response timed out */
    PVR_ERROR_REJECTED           = -5, /*!< @brief the command was rejected by the backend */
    PVR_ERROR_ALREADY_PRESENT    = -6, /*!< @brief the requested item can not be added, because it's already present */
    PVR_ERROR_INVALID_PARAMETERS = -7, /*!< @brief the parameters of the method that was called are invalid for this operation */
    PVR_ERROR_RECORDING_RUNNING  = -8, /*!< @brief a recording is running, so the timer can't be deleted without doing a forced delete */
    PVR_ERROR_FAILED             = -9, /*!< @brief the command failed */
  } PVR_ERROR;

  /*!
   * @brief PVR timer states
   */
  typedef enum
  {
    PVR_TIMER_STATE_NEW          = 0, /*!< @brief the timer was just created on the backend and is not yet active. This state must not be used for timers just created on the client side. */
    PVR_TIMER_STATE_SCHEDULED    = 1, /*!< @brief the timer is scheduled for recording */
    PVR_TIMER_STATE_RECORDING    = 2, /*!< @brief the timer is currently recordings */
    PVR_TIMER_STATE_COMPLETED    = 3, /*!< @brief the recording completed successfully */
    PVR_TIMER_STATE_ABORTED      = 4, /*!< @brief recording started, but was aborted */
    PVR_TIMER_STATE_CANCELLED    = 5, /*!< @brief the timer was scheduled, but was canceled */
    PVR_TIMER_STATE_CONFLICT_OK  = 6, /*!< @brief the scheduled timer conflicts with another one, but will be recorded */
    PVR_TIMER_STATE_CONFLICT_NOK = 7, /*!< @brief the scheduled timer conflicts with another one and won't be recorded */
    PVR_TIMER_STATE_ERROR        = 8, /*!< @brief the timer is scheduled, but can't be recorded for some reason */
    PVR_TIMER_STATE_DISABLED     = 9, /*!< @brief the timer was disabled by the user, can be enabled via setting the state to PVR_TIMER_STATE_SCHEDULED */
  } PVR_TIMER_STATE;

  /*!
   * @brief PVR menu hook categories
   */
  typedef enum
  {
    PVR_MENUHOOK_UNKNOWN           =-1, /*!< @brief unknown menu hook */
    PVR_MENUHOOK_ALL               = 0, /*!< @brief all categories */
    PVR_MENUHOOK_CHANNEL           = 1, /*!< @brief for channels */
    PVR_MENUHOOK_TIMER             = 2, /*!< @brief for timers */
    PVR_MENUHOOK_EPG               = 3, /*!< @brief for EPG */
    PVR_MENUHOOK_RECORDING         = 4, /*!< @brief for recordings */
    PVR_MENUHOOK_DELETED_RECORDING = 5, /*!< @brief for deleted recordings */
    PVR_MENUHOOK_SETTING           = 6, /*!< @brief for settings */
  } PVR_MENUHOOK_CAT;

  /*!
   * @brief PVR backend connection states. Used with ConnectionStateChange callback.
   */
  typedef enum
  {
    PVR_CONNECTION_STATE_UNKNOWN            = 0,  /*!< @brief unknown state (e.g. not yet tried to connect) */
    PVR_CONNECTION_STATE_SERVER_UNREACHABLE = 1,  /*!< @brief backend server is not reachable (e.g. server not existing or network down)*/
    PVR_CONNECTION_STATE_SERVER_MISMATCH    = 2,  /*!< @brief backend server is reachable, but there is not the expected type of server running (e.g. HTSP required, but FTP running at given server:port) */
    PVR_CONNECTION_STATE_VERSION_MISMATCH   = 3,  /*!< @brief backend server is reachable, but server version does not match client requirements */
    PVR_CONNECTION_STATE_ACCESS_DENIED      = 4,  /*!< @brief backend server is reachable, but denies client access (e.g. due to wrong credentials) */
    PVR_CONNECTION_STATE_CONNECTED          = 5,  /*!< @brief connection to backend server is established */
    PVR_CONNECTION_STATE_DISCONNECTED       = 6,  /*!< @brief no connection to backend server (e.g. due to network errors or client initiated disconnect)*/
    PVR_CONNECTION_STATE_CONNECTING         = 7,  /*!< @brief connecting to backend */
  } PVR_CONNECTION_STATE;

  /*!
   * @brief PVR recording channel types
   */
  typedef enum
  {
    PVR_RECORDING_CHANNEL_TYPE_UNKNOWN = 0, /*!< @brief unknown */
    PVR_RECORDING_CHANNEL_TYPE_TV      = 1, /*!< @brief TV channel */
    PVR_RECORDING_CHANNEL_TYPE_RADIO   = 2, /*!< @brief radio channel */
  } PVR_RECORDING_CHANNEL_TYPE;

  /*!
   * @brief Representation of a named value
   */
  typedef struct PVR_NAMED_VALUE {
    char  strName[PVR_ADDON_NAME_STRING_LENGTH];  /*!< @brief (required) name */
    char  strValue[PVR_ADDON_NAME_STRING_LENGTH]; /*!< @brief (required) value */
  } ATTRIBUTE_PACKED PVR_NAMED_VALUE;

  /*!
   * @brief Properties passed to the Create() method of an add-on.
   */
  typedef struct AddonProperties_PVR
  {
    const char* strUserPath;           /*!< @brief path to the user profile */
    const char* strClientPath;         /*!< @brief path to this add-on */
    int iEpgMaxDays;                   /*!< @brief if > EPG_TIMEFRAME_UNLIMITED, in async epg mode, deliver only events in the range from 'end time > now' to 'start time < now + iEpgMaxDays. EPG_TIMEFRAME_UNLIMITED, notify all events. */
  } AddonProperties_PVR;

  /*!
   * @brief Representation of a general attribute integer value.
   */
  typedef struct PVR_ATTRIBUTE_INT_VALUE
  {
    int iValue;                                           /*!< @brief (required) an integer value for a certain attribute */
    char strDescription[PVR_ADDON_ATTRIBUTE_DESC_LENGTH]; /*!< @brief (optional) a localized string describing the value. If left blank, Kodi will generate a suitable representation (like the integer value as string) */
  } ATTRIBUTE_PACKED PVR_ATTRIBUTE_INT_VALUE;

  /*!
   * @brief PVR add-on capabilities. All capabilities are set to "false" or 0 as default
   * If a capability is set to true, then the corresponding methods from xbmc_pvr_dll.h need to be implemented.
   */
  typedef struct PVR_ADDON_CAPABILITIES
  {
    bool bSupportsEPG;                  /*!< @brief true if the add-on provides EPG information */
    bool bSupportsEPGEdl;               /*!< @brief true if the backend supports retrieving an edit decision list for an EPG tag. */
    bool bSupportsTV;                   /*!< @brief true if this add-on provides TV channels */
    bool bSupportsRadio;                /*!< @brief true if this add-on supports radio channels */
    bool bSupportsRecordings;           /*!< @brief true if this add-on supports playback of recordings stored on the backend */
    bool bSupportsRecordingsUndelete;   /*!< @brief true if this add-on supports undelete of recordings stored on the backend */
    bool bSupportsTimers;               /*!< @brief true if this add-on supports the creation and editing of timers */
    bool bSupportsChannelGroups;        /*!< @brief true if this add-on supports channel groups */
    bool bSupportsChannelScan;          /*!< @brief true if this add-on support scanning for new channels on the backend */
    bool bSupportsChannelSettings;      /*!< @brief true if this add-on supports the following functions: DeleteChannel, RenameChannel, DialogChannelSettings and DialogAddChannel */
    bool bHandlesInputStream;           /*!< @brief true if this add-on provides an input stream. false if Kodi handles the stream. */
    bool bHandlesDemuxing;              /*!< @brief true if this add-on demultiplexes packets. */
    bool bSupportsRecordingPlayCount;   /*!< @brief true if the backend supports play count for recordings. */
    bool bSupportsLastPlayedPosition;   /*!< @brief true if the backend supports store/retrieve of last played position for recordings. */
    bool bSupportsRecordingEdl;         /*!< @brief true if the backend supports retrieving an edit decision list for recordings. */
    bool bSupportsRecordingsRename;     /*!< @brief true if the backend supports renaming recordings. */
    bool bSupportsRecordingsLifetimeChange; /*!< @brief true if the backend supports changing lifetime for recordings. */
    bool bSupportsDescrambleInfo;       /*!< @brief true if the backend supports descramble information for playing channels. */
    bool bSupportsAsyncEPGTransfer;     /*!< @brief true if this addon-on supports asynchronous transfer of epg events to Kodi using the callback function EpgEventStateChange. */
    bool bSupportsRecordingSize;        /*!< @brief true if this addon-on supports retrieving size of recordings. */

    unsigned int iRecordingsLifetimesSize; /*!< @brief (required) Count of possible values for PVR_RECORDING.iLifetime. 0 means lifetime is not supported for recordings or no own value definition wanted, but to use Kodi defaults of 1..365. */
    PVR_ATTRIBUTE_INT_VALUE recordingsLifetimeValues[PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE]; /*!< @brief (optional) Array containing the possible values for PVR_RECORDING.iLifetime. Must be filled if iLifetimesSize > 0 */
  } ATTRIBUTE_PACKED PVR_ADDON_CAPABILITIES;

  /*!
   * @brief PVR stream properties
   */
  typedef struct PVR_STREAM_PROPERTIES
  {
    unsigned int iStreamCount;
    struct PVR_STREAM
    {
      unsigned int      iPID;               /*!< @brief (required) PID */
      xbmc_codec_type_t iCodecType;         /*!< @brief (required) codec type this stream */
      xbmc_codec_id_t   iCodecId;           /*!< @brief (required) codec id of this stream */
      char              strLanguage[4];     /*!< @brief (required) language id */
      int               iSubtitleInfo;      /*!< @brief (required) Subtitle Info */
      int               iFPSScale;          /*!< @brief (required) scale of 1000 and a rate of 29970 will result in 29.97 fps */
      int               iFPSRate;           /*!< @brief (required) FPS rate */
      int               iHeight;            /*!< @brief (required) height of the stream reported by the demuxer */
      int               iWidth;             /*!< @brief (required) width of the stream reported by the demuxer */
      float             fAspect;            /*!< @brief (required) display aspect ratio of the stream */
      int               iChannels;          /*!< @brief (required) amount of channels */
      int               iSampleRate;        /*!< @brief (required) sample rate */
      int               iBlockAlign;        /*!< @brief (required) block alignment */
      int               iBitRate;           /*!< @brief (required) bit rate */
      int               iBitsPerSample;     /*!< @brief (required) bits per sample */
    } stream[PVR_STREAM_MAX_STREAMS];       /*!< @brief (required) the streams */
  } ATTRIBUTE_PACKED PVR_STREAM_PROPERTIES;

  /*!
   * @brief Signal status information
   */
  typedef struct PVR_SIGNAL_STATUS
  {
    char   strAdapterName[PVR_ADDON_NAME_STRING_LENGTH];   /*!< @brief (optional) name of the adapter that's being used */
    char   strAdapterStatus[PVR_ADDON_NAME_STRING_LENGTH]; /*!< @brief (optional) status of the adapter that's being used */
    char   strServiceName[PVR_ADDON_NAME_STRING_LENGTH];   /*!< @brief (optional) name of the current service */
    char   strProviderName[PVR_ADDON_NAME_STRING_LENGTH];  /*!< @brief (optional) name of the current service's provider */
    char   strMuxName[PVR_ADDON_NAME_STRING_LENGTH];       /*!< @brief (optional) name of the current mux */
    int    iSNR;                                           /*!< @brief (optional) signal/noise ratio */
    int    iSignal;                                        /*!< @brief (optional) signal strength */
    long   iBER;                                           /*!< @brief (optional) bit error rate */
    long   iUNC;                                           /*!< @brief (optional) uncorrected blocks */
  } ATTRIBUTE_PACKED PVR_SIGNAL_STATUS;

  /*!
   * @brief descramble information
   */
  typedef struct PVR_DESCRAMBLE_INFO
  {
    int iPid;     /*!< @brief (optional) pid; PVR_DESCRAMBLE_INFO_NOT_AVAILABLE if not available */
    int iCaid;    /*!< @brief (optional) caid; PVR_DESCRAMBLE_INFO_NOT_AVAILABLE if not available */
    int iProvid;  /*!< @brief (optional) provid; PVR_DESCRAMBLE_INFO_NOT_AVAILABLE if not available */
    int iEcmTime; /*!< @brief (optional) ecm time; PVR_DESCRAMBLE_INFO_NOT_AVAILABLE if not available */
    int iHops;    /*!< @brief (optional) hops; PVR_DESCRAMBLE_INFO_NOT_AVAILABLE if not available */
    char strCardSystem[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];  /*!< @brief (optional); empty string if not available */
    char strReader[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];      /*!< @brief (optional); empty string if not available */
    char strFrom[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];        /*!< @brief (optional); empty string if not available */
    char strProtocol[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];    /*!< @brief (optional); empty string if not available */
  } ATTRIBUTE_PACKED PVR_DESCRAMBLE_INFO;

  /*!
   * @brief Menu hooks that are available in the context menus while playing a stream via this add-on.
   * And in the Live TV settings dialog
   */
  typedef struct PVR_MENUHOOK
  {
    unsigned int     iHookId;              /*!< @brief (required) this hook's identifier */
    unsigned int     iLocalizedStringId;   /*!< @brief (required) the id of the label for this hook in g_localizeStrings */
    PVR_MENUHOOK_CAT category;             /*!< @brief (required) category of menu hook */
  } ATTRIBUTE_PACKED PVR_MENUHOOK;

  /*!
   * @brief special PVR_CHANNEL.iOrder and PVR_CHANNEL_GROUP_MEMBER.iOrder value to indicate this channel has an unknown order
   */
  const int PVR_CHANNEL_UNKNOWN_ORDER = 0; /*!< @brief channel has an unknown order. */

  /*!
   * @brief Representation of a TV or radio channel.
   */
  typedef struct PVR_CHANNEL
  {
    unsigned int iUniqueId;                                            /*!< @brief (required) unique identifier for this channel */
    bool         bIsRadio;                                             /*!< @brief (required) true if this is a radio channel, false if it's a TV channel */
    unsigned int iChannelNumber;                                       /*!< @brief (optional) channel number of this channel on the backend */
    unsigned int iSubChannelNumber;                                    /*!< @brief (optional) sub channel number of this channel on the backend (ATSC) */
    char         strChannelName[PVR_ADDON_NAME_STRING_LENGTH];         /*!< @brief (optional) channel name given to this channel */
    char         strInputFormat[PVR_ADDON_INPUT_FORMAT_STRING_LENGTH]; /*!< @brief (optional) input format type. types can be found in ffmpeg/libavformat/allformats.c
                                                                                   leave empty if unknown */
    unsigned int iEncryptionSystem;                                    /*!< @brief (optional) the encryption ID or CaID of this channel */
    char         strIconPath[PVR_ADDON_URL_STRING_LENGTH];             /*!< @brief (optional) path to the channel icon (if present) */
    bool         bIsHidden;                                            /*!< @brief (optional) true if this channel is marked as hidden */
    bool         bHasArchive;                                          /*!< @brief (optional) true if this channel has a server-side back buffer */
    int          iOrder;                                               /*!< @brief (optional) The value denoting the order of this channel in the 'All channels' group */
  } ATTRIBUTE_PACKED PVR_CHANNEL;

  typedef struct PVR_CHANNEL_GROUP
  {
    char         strGroupName[PVR_ADDON_NAME_STRING_LENGTH]; /*!< @brief (required) name of this channel group */
    bool         bIsRadio;                                   /*!< @brief (required) true if this is a radio channel group, false otherwise. */
    unsigned int iPosition;                                  /*!< @brief (optional) sort position of the group (0 indicates that the backend doesn't support sorting of groups) */
  } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP;

  typedef struct PVR_CHANNEL_GROUP_MEMBER
  {
    char         strGroupName[PVR_ADDON_NAME_STRING_LENGTH]; /*!< @brief (required) name of the channel group to add the channel to */
    unsigned int iChannelUniqueId;                           /*!< @brief (required) unique id of the member */
    unsigned int iChannelNumber;                             /*!< @brief (optional) channel number within the group */
    unsigned int iSubChannelNumber;                          /*!< @brief (optional) sub channel number within the group (ATSC) */
    int          iOrder;                                     /*!< @brief (optional) The value denoting the order of this channel in this group */
  } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP_MEMBER;

  /*!
   * @brief Representation of a timer type's attribute integer value.
   */
  typedef PVR_ATTRIBUTE_INT_VALUE PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE;

  /*!
   * @brief Representation of a timer type.
   */
  typedef struct PVR_TIMER_TYPE
  {
    unsigned int iId;                                       /*!< @brief (required) this type's identifier. Ids must be > PVR_TIMER_TYPE_NONE. */
    unsigned int iAttributes;                               /*!< @brief (required) defines the attributes for this type (PVR_TIMER_TYPE_* constants). */
    char strDescription[PVR_ADDON_TIMERTYPE_STRING_LENGTH]; /*!< @brief (optional) a short localized string describing the purpose of the type. (e.g.
                                                              "Any time at this channel if title matches"). If left blank, Kodi will generate a
                                                              description based on the attributes REPEATING and MANUAL. (e.g. "Repeating EPG-based." */
    /* priority value definitions */
    unsigned int iPrioritiesSize;                           /*!< @brief (required) Count of possible values for PVR_TMER.iPriority. 0 means priority
                                                              is not supported by this timer type or no own value definition wanted, but to use Kodi defaults
                                                              of 1..100. */
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE
      priorities[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];    /*!< @brief (optional) Array containing the possible values for PVR_TMER.iPriority. Must be
                                                              filled if iPrioritiesSize > 0 */
    int          iPrioritiesDefault;                        /*!< @brief (optional) The default value for PVR_TMER.iPriority. Must be filled if iPrioritiesSize > 0 */

    /* lifetime value definitions */
    unsigned int iLifetimesSize;                            /*!< @brief (required) Count of possible values for PVR_TMER.iLifetime. 0 means lifetime
                                                              is not supported by this timer type or no own value definition wanted, but to use Kodi defaults
                                                              of 1..365. */
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE
      lifetimes[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];     /*!< @brief (optional) Array containing the possible values for PVR_TMER.iLifetime. Must be
                                                              filled if iLifetimesSize > 0 */
    int          iLifetimesDefault;                         /*!< @brief (optional) The default value for PVR_TMER.iLifetime. Must be filled if iLifetimesSize > 0 */

    /* prevent duplicate episodes value definitions */
    unsigned int iPreventDuplicateEpisodesSize;             /*!< @brief (required) Count of possible values for PVR_TMER.iPreventDuplicateEpisodes. 0 means duplicate
                                                              episodes prevention is not supported by this timer type or no own value definition wanted, but to use
                                                              Kodi defaults. */
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE
      preventDuplicateEpisodes[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
                                                            /*!< @brief (optional) Array containing the possible values for PVR_TMER.iPreventDuplicateEpisodes.. Must
                                                              be filled if iPreventDuplicateEpisodesSize > 0 */
    unsigned int iPreventDuplicateEpisodesDefault;          /*!< @brief (optional) The default value for PVR_TMER.iPreventDuplicateEpisodesSize. Must be filled if iPreventDuplicateEpisodesSize > 0 */

    /* recording folder list value definitions */
    unsigned int iRecordingGroupSize;                       /*!< @brief (required) Count of possible values of PVR_TIMER.iRecordingGroup. 0 means folder lists are not supported by this timer type */
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE
      recordingGroup[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
                                                            /*!< @brief (optional) Array containing the possible values of PVR_TMER.iRecordingGroup. Must be filled if iRecordingGroupSize > 0 */
    unsigned int iRecordingGroupDefault;                    /*!< @brief (optional) The default value for PVR_TIMER.iRecordingGroup. Must be filled in if PVR_TIMER.iRecordingGroupSize > 0 */

    /* max recordings value definitions */
    unsigned int iMaxRecordingsSize;                        /*!< @brief (required) Count of possible values of PVR_TIMER.iMaxRecordings. 0 means max recordings are not supported by this timer type */
    PVR_TIMER_TYPE_ATTRIBUTE_INT_VALUE
      maxRecordings[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE_SMALL];
                                                            /*!< @brief (optional) Array containing the possible values of PVR_TMER.iMaxRecordings. */
    int iMaxRecordingsDefault;                              /*!< @brief (optional) The default value for PVR_TIMER.iMaxRecordings. Must be filled in if PVR_TIMER.iMaxRecordingsSize > 0 */

  } ATTRIBUTE_PACKED PVR_TIMER_TYPE;

  /*!
   * @brief Representation of a timer event.
   */
  typedef struct PVR_TIMER {
    unsigned int    iClientIndex;                              /*!< @brief (required) the index of this timer given by the client. PVR_TIMER_NO_CLIENT_INDEX indicates that the index was not yet set by the client, for example for new timers created by
                                                                    Kodi and passed the first time to the client. A valid index must be greater than PVR_TIMER_NO_CLIENT_INDEX. */
    unsigned int    iParentClientIndex;                        /*!< @brief (optional) for timers scheduled by a repeating timer, the index of the repeating timer that scheduled this timer (it's PVR_TIMER.iClientIndex value). Use PVR_TIMER_NO_PARENT
                                                                    to indicate that this timer was no scheduled by a repeating timer. */
    int             iClientChannelUid;                         /*!< @brief (optional) unique identifier of the channel to record on. PVR_TIMER_ANY_CHANNEL will denote "any channel", not a specific one. PVR_CHANNEL_INVALID_UID denotes that channel uid is not available.*/
    time_t          startTime;                                 /*!< @brief (optional) start time of the recording in UTC. Instant timers that are sent to the add-on by Kodi will have this value set to 0.*/
    time_t          endTime;                                   /*!< @brief (optional) end time of the recording in UTC. */
    bool            bStartAnyTime;                             /*!< @brief (optional) for EPG based (not Manual) timers indicates startTime does not apply. Default = false */
    bool            bEndAnyTime;                               /*!< @brief (optional) for EPG based (not Manual) timers indicates endTime does not apply. Default = false */
    PVR_TIMER_STATE state;                                     /*!< @brief (required) the state of this timer */
    unsigned int    iTimerType;                                /*!< @brief (required) the type of this timer. It is private to the addon and can be freely defined by the addon. The value must be greater than PVR_TIMER_TYPE_NONE.
                                                                    Kodi does not interpret this value (except for checking for PVR_TIMER_TYPE_NONE), but will pass the right id to the addon with every PVR_TIMER instance, thus the addon easily can determine
                                                                    the timer type. */
    char            strTitle[PVR_ADDON_NAME_STRING_LENGTH];    /*!< @brief (required) a title for this timer */
    char            strEpgSearchString[PVR_ADDON_NAME_STRING_LENGTH]; /*!< @brief (optional) a string used to search epg data for repeating epg-based timers. Format is backend-dependent, for example regexp */
    bool            bFullTextEpgSearch;                        /*!< @brief (optional) indicates, whether strEpgSearchString is to match against the epg episode title only or also against "other" epg data (backend-dependent) */
    char            strDirectory[PVR_ADDON_URL_STRING_LENGTH]; /*!< @brief (optional) the (relative) directory where the recording will be stored in */
    char            strSummary[PVR_ADDON_DESC_STRING_LENGTH];  /*!< @brief (optional) the summary for this timer */
    int             iPriority;                                 /*!< @brief (optional) the priority of this timer */
    int             iLifetime;                                 /*!< @brief (optional) lifetime of recordings created by this timer. > 0 days after which recordings will be deleted by the backend, < 0 addon defined integer list reference, == 0 disabled */
    int             iMaxRecordings;                            /*!< @brief (optional) maximum number of recordings this timer shall create. > 0 number of recordings, < 0 addon defined integer list reference, == 0 disabled */
    unsigned int    iRecordingGroup;                           /*!< @brief (optional) integer ref to addon/backend defined list of recording groups*/
    time_t          firstDay;                                  /*!< @brief (optional) the first day this timer is active, for repeating timers */
    unsigned int    iWeekdays;                                 /*!< @brief (optional) week days, for repeating timers (see PVR_WEEKDAY_* constant values) */
    unsigned int    iPreventDuplicateEpisodes;                 /*!< @brief (optional) 1 if backend should only record new episodes in case of a repeating epg-based timer, 0 if all episodes shall be recorded (no duplicate detection). Actual algorithm for
                                                                    duplicate detection is defined by the backend. Addons may define own values for different duplicate detection algorithms, thus this is not just a bool.*/
    unsigned int    iEpgUid;                                   /*!< @brief (optional) EPG event id associated with this timer. Event ids must be unique for a channel. Valid ids must be greater than EPG_TAG_INVALID_UID. */
    unsigned int    iMarginStart;                              /*!< @brief (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
    unsigned int    iMarginEnd;                                /*!< @brief (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */
    int             iGenreType;                                /*!< @brief (optional) genre type */
    int             iGenreSubType;                             /*!< @brief (optional) genre sub type */
    char            strSeriesLink[PVR_ADDON_URL_STRING_LENGTH]; /*!< @brief (optional) series link for this timer. If set for an epg-based timer rule, matching events will be found by checking strSeriesLink instead of strTitle (and bFullTextEpgSearch) */

  } ATTRIBUTE_PACKED PVR_TIMER;

  /* PVR_RECORDING.iFlags values */
  const unsigned int PVR_RECORDING_FLAG_UNDEFINED   = 0x00000000; /*!< @brief nothing special to say about this recording */
  const unsigned int PVR_RECORDING_FLAG_IS_SERIES   = 0x00000001; /*!< @brief this recording is part of a series */
  const unsigned int PVR_RECORDING_FLAG_IS_NEW      = 0x00000002; /*!< @brief this recording will be flagged as new */
  const unsigned int PVR_RECORDING_FLAG_IS_PREMIERE = 0x00000004; /*!< @brief this recording will be flagged as a premiere */
  const unsigned int PVR_RECORDING_FLAG_IS_FINALE   = 0x00000008; /*!< @brief this recording will be flagged as a finale */
  const unsigned int PVR_RECORDING_FLAG_IS_LIVE     = 0x00000010; /*!< @brief this recording will be flagged as live */

  /*!
   * @brief special PVR_RECORDING.iSeriesNumber and PVR_RECORDING.iEpisodeNumber value to indicate it is not to be used
   * 
   * Used if recording has no valid season and/or episode info.
   */
  const unsigned int PVR_RECORDING_INVALID_SERIES_EPISODE = EPG_TAG_INVALID_SERIES_EPISODE;

  /*!
   * @brief Representation of a recording.
   */
  typedef struct PVR_RECORDING {
    char   strRecordingId[PVR_ADDON_NAME_STRING_LENGTH];  /*!< @brief (required) unique id of the recording on the client. */
    char   strTitle[PVR_ADDON_NAME_STRING_LENGTH];        /*!< @brief (required) the title of this recording */
    char   strEpisodeName[PVR_ADDON_NAME_STRING_LENGTH];  /*!< @brief (optional) episode name (also known as subtitle) */
    int    iSeriesNumber;                                 /*!< @brief (optional) series number (usually called season). Set to "0" for specials/pilot. For 'invalid' set to PVR_RECORDING_INVALID_SERIES_EPISODE */
    int    iEpisodeNumber;                                /*!< @brief (optional) episode number within the "iSeriesNumber" season. For 'invalid' set to PVR_RECORDING_INVALID_SERIES_EPISODE */
    int    iYear;                                         /*!< @brief (optional) year of first release (use to identify a specific movie re-make) / first airing for TV shows. Set to '0' for invalid. */
    char   strDirectory[PVR_ADDON_URL_STRING_LENGTH];     /*!< @brief (optional) directory of this recording on the client */
    char   strPlotOutline[PVR_ADDON_DESC_STRING_LENGTH];  /*!< @brief (optional) plot outline */
    char   strPlot[PVR_ADDON_DESC_STRING_LENGTH];         /*!< @brief (optional) plot */
    char   strGenreDescription[PVR_ADDON_DESC_STRING_LENGTH]; /*!< @brief (optional) genre. Will be used only when iGenreType = EPG_GENRE_USE_STRING or iGenreSubType = EPG_GENRE_USE_STRING */
    char   strChannelName[PVR_ADDON_NAME_STRING_LENGTH];  /*!< @brief (optional) channel name */
    char   strIconPath[PVR_ADDON_URL_STRING_LENGTH];      /*!< @brief (optional) channel logo (icon) path */
    char   strThumbnailPath[PVR_ADDON_URL_STRING_LENGTH]; /*!< @brief (optional) thumbnail path */
    char   strFanartPath[PVR_ADDON_URL_STRING_LENGTH];    /*!< @brief (optional) fanart path */
    time_t recordingTime;                                 /*!< @brief (optional) start time of the recording */
    int    iDuration;                                     /*!< @brief (optional) duration of the recording in seconds */
    int    iPriority;                                     /*!< @brief (optional) priority of this recording (from 0 - 100) */
    int    iLifetime;                                     /*!< @brief (optional) life time in days of this recording */
    int    iGenreType;                                    /*!< @brief (optional) genre type */
    int    iGenreSubType;                                 /*!< @brief (optional) genre sub type */
    int    iPlayCount;                                    /*!< @brief (optional) play count of this recording on the client */
    int    iLastPlayedPosition;                           /*!< @brief (optional) last played position of this recording on the client */
    bool   bIsDeleted;                                    /*!< @brief (optional) shows this recording is deleted and can be undelete */
    unsigned int iEpgEventId;                             /*!< @brief (optional) EPG event id associated with this recording. Valid ids must be greater than EPG_TAG_INVALID_UID. */
    int    iChannelUid;                                   /*!< @brief (optional) unique identifier of the channel for this recording. PVR_CHANNEL_INVALID_UID denotes that channel uid is not available. */
    PVR_RECORDING_CHANNEL_TYPE channelType;               /*!< @brief (optional) channel type. Set to PVR_RECORDING_CHANNEL_TYPE_UNKNOWN if the type cannot be determined. */
    char   strFirstAired[PVR_ADDON_DATE_STRING_LENGTH];   /*!< @brief (optional) first aired date of this recording. Used only for display purposes. Specify in W3C date format "YYYY-MM-DD". */
    unsigned int iFlags;                                  /*!< @brief (optional) bit field of independent flags associated with the recording */
    int64_t sizeInBytes;                                  /*!< @brief (optional) size of the recording in bytes */
  } ATTRIBUTE_PACKED PVR_RECORDING;

  /*!
   * @brief Edit definition list (EDL)
   */
  typedef enum
  {
    PVR_EDL_TYPE_CUT      = 0, /*!< @brief cut (completely remove content) */
    PVR_EDL_TYPE_MUTE     = 1, /*!< @brief mute audio */
    PVR_EDL_TYPE_SCENE    = 2, /*!< @brief scene markers (chapter seeking) */
    PVR_EDL_TYPE_COMBREAK = 3  /*!< @brief commercial breaks */
  } PVR_EDL_TYPE;

  typedef struct PVR_EDL_ENTRY
  {
    int64_t start;     // ms
    int64_t end;       // ms
    PVR_EDL_TYPE type;
  } ATTRIBUTE_PACKED PVR_EDL_ENTRY;

  /*!
   * @brief PVR menu hook data
   */
  typedef struct PVR_MENUHOOK_DATA
  {
    PVR_MENUHOOK_CAT cat;
    union data {
      int iEpgUid;
      PVR_CHANNEL channel;
      PVR_TIMER timer;
      PVR_RECORDING recording;
    } data;
  } ATTRIBUTE_PACKED PVR_MENUHOOK_DATA;

  /*!
   * @brief times of playing stream (Live TV and recordings)
   */
  typedef struct PVR_STREAM_TIMES
  {
    time_t startTime; /*!< @brief For recordings, this must be zero. For Live TV, this is a reference time in units of time_t (UTC) from which time elapsed starts. Ideally start of tv show, but can be any other value. */
    int64_t ptsStart; /*!< @brief the pts of startTime */
    int64_t ptsBegin; /*!< @brief earliest pts player can seek back. Value is in micro seconds, relative to ptsStart. For recordings, this must be zero. For Live TV, this must be zero if not timeshifting and must point to begin of the timeshift buffer, otherwise. */
    int64_t ptsEnd;   /*!< @brief latest pts player can seek forward. Value is in micro seconds, relative to ptsStart. For recordings, this must be the total length. For Live TV, this must be zero if not timeshifting and must point to end of the timeshift buffer, otherwise. */
  } ATTRIBUTE_PACKED PVR_STREAM_TIMES;

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
                                  PVR_CONNECTION_STATE newState,
                                  const char* strMessage);
    void (*EpgEventStateChange)(void* kodiInstance, EPG_TAG* tag, EPG_EVENT_STATE newState);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Transfer functions where give data back to Kodi, e.g. GetChannels calls TransferChannelEntry
    void (*TransferChannelEntry)(void* kodiInstance,
                                 const ADDON_HANDLE handle,
                                 const PVR_CHANNEL* chan);
    void (*TransferChannelGroup)(void* kodiInstance,
                                 const ADDON_HANDLE handle,
                                 const PVR_CHANNEL_GROUP* group);
    void (*TransferChannelGroupMember)(void* kodiInstance,
                                       const ADDON_HANDLE handle,
                                       const PVR_CHANNEL_GROUP_MEMBER* member);
    void (*TransferEpgEntry)(void* kodiInstance,
                             const ADDON_HANDLE handle,
                             const EPG_TAG* epgentry);
    void (*TransferRecordingEntry)(void* kodiInstance,
                                   const ADDON_HANDLE handle,
                                   const PVR_RECORDING* recording);
    void (*TransferTimerEntry)(void* kodiInstance,
                               const ADDON_HANDLE handle,
                               const PVR_TIMER* timer);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Kodi inform interface functions
    void (*TriggerChannelUpdate)(void* kodiInstance);
    void (*TriggerChannelGroupsUpdate)(void* kodiInstance);
    void (*TriggerEpgUpdate)(void* kodiInstance, unsigned int iChannelUid);
    void (*TriggerRecordingUpdate)(void* kodiInstance);
    void (*TriggerTimerUpdate)(void* kodiInstance);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Stream demux interface functions
    void (*FreeDemuxPacket)(void* kodiInstance, DemuxPacket* pPacket);
    DemuxPacket* (*AllocateDemuxPacket)(void* kodiInstance, int iDataSize);
    xbmc_codec_t (*GetCodecByName)(const void* kodiInstance, const char* strCodecName);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // New functions becomes added below and can be on another API change (where
    // breaks min API version) moved up.
  } AddonToKodiFuncTable_PVR;

  /*!
   * @brief Structure to transfer the methods from xbmc_pvr_dll.h to Kodi
   */
  typedef struct KodiToAddonFuncTable_PVR
  {
    // Pointer inside addon where used on them to find his instance class (currently unused!)
    KODI_HANDLE addonInstance;

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // General interface functions
    PVR_ERROR(__cdecl* GetCapabilities)(PVR_ADDON_CAPABILITIES*);
    const char*(__cdecl* GetBackendName)(void);
    const char*(__cdecl* GetBackendVersion)(void);
    const char*(__cdecl* GetBackendHostname)(void);
    const char*(__cdecl* GetConnectionString)(void);
    PVR_ERROR(__cdecl* GetDriveSpace)(long long*, long long*);
    PVR_ERROR(__cdecl* MenuHook)(const PVR_MENUHOOK&, const PVR_MENUHOOK_DATA&);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel interface functions

    int(__cdecl* GetChannelsAmount)(void);
    PVR_ERROR(__cdecl* GetChannels)(ADDON_HANDLE, bool);
    PVR_ERROR(__cdecl* GetChannelStreamProperties)(const PVR_CHANNEL*,
                                                   PVR_NAMED_VALUE*,
                                                   unsigned int*);
    PVR_ERROR(__cdecl* GetSignalStatus)(int, PVR_SIGNAL_STATUS*);
    PVR_ERROR(__cdecl* GetDescrambleInfo)(int, PVR_DESCRAMBLE_INFO*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel group interface functions
    int(__cdecl* GetChannelGroupsAmount)(void);
    PVR_ERROR(__cdecl* GetChannelGroups)(ADDON_HANDLE, bool);
    PVR_ERROR(__cdecl* GetChannelGroupMembers)(ADDON_HANDLE, const PVR_CHANNEL_GROUP&);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel edit interface functions
    PVR_ERROR(__cdecl* DeleteChannel)(const PVR_CHANNEL&);
    PVR_ERROR(__cdecl* RenameChannel)(const PVR_CHANNEL&);
    PVR_ERROR(__cdecl* OpenDialogChannelSettings)(const PVR_CHANNEL&);
    PVR_ERROR(__cdecl* OpenDialogChannelAdd)(const PVR_CHANNEL&);
    PVR_ERROR(__cdecl* OpenDialogChannelScan)(void);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // EPG interface functions
    PVR_ERROR(__cdecl* GetEPGForChannel)(ADDON_HANDLE, int, time_t, time_t);
    PVR_ERROR(__cdecl* IsEPGTagRecordable)(const EPG_TAG*, bool*);
    PVR_ERROR(__cdecl* IsEPGTagPlayable)(const EPG_TAG*, bool*);
    PVR_ERROR(__cdecl* GetEPGTagEdl)(const EPG_TAG*, PVR_EDL_ENTRY[], int*);
    PVR_ERROR(__cdecl* GetEPGTagStreamProperties)(const EPG_TAG*, PVR_NAMED_VALUE*, unsigned int*);
    PVR_ERROR(__cdecl* SetEPGTimeFrame)(int);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Recording interface functions
    int(__cdecl* GetRecordingsAmount)(bool);
    PVR_ERROR(__cdecl* GetRecordings)(ADDON_HANDLE, bool);
    PVR_ERROR(__cdecl* DeleteRecording)(const PVR_RECORDING&);
    PVR_ERROR(__cdecl* UndeleteRecording)(const PVR_RECORDING&);
    PVR_ERROR(__cdecl* DeleteAllRecordingsFromTrash)(void);
    PVR_ERROR(__cdecl* RenameRecording)(const PVR_RECORDING&);
    PVR_ERROR(__cdecl* SetRecordingLifetime)(const PVR_RECORDING*);
    PVR_ERROR(__cdecl* SetRecordingPlayCount)(const PVR_RECORDING&, int);
    PVR_ERROR(__cdecl* SetRecordingLastPlayedPosition)(const PVR_RECORDING&, int);
    int(__cdecl* GetRecordingLastPlayedPosition)(const PVR_RECORDING&);
    PVR_ERROR(__cdecl* GetRecordingEdl)(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*);
    PVR_ERROR(__cdecl* GetRecordingSize)(const PVR_RECORDING*, int64_t*);
    PVR_ERROR(__cdecl* GetRecordingStreamProperties)
    (const PVR_RECORDING*, PVR_NAMED_VALUE*, unsigned int*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Timer interface functions
    PVR_ERROR(__cdecl* GetTimerTypes)(PVR_TIMER_TYPE[], int*);
    int(__cdecl* GetTimersAmount)(void);
    PVR_ERROR(__cdecl* GetTimers)(ADDON_HANDLE);
    PVR_ERROR(__cdecl* AddTimer)(const PVR_TIMER&);
    PVR_ERROR(__cdecl* DeleteTimer)(const PVR_TIMER&, bool);
    PVR_ERROR(__cdecl* UpdateTimer)(const PVR_TIMER&);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Powersaving interface functions
    void(__cdecl* OnSystemSleep)(void);
    void(__cdecl* OnSystemWake)(void);
    void(__cdecl* OnPowerSavingActivated)(void);
    void(__cdecl* OnPowerSavingDeactivated)(void);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Live stream read interface functions
    bool(__cdecl* OpenLiveStream)(const PVR_CHANNEL&);
    void(__cdecl* CloseLiveStream)(void);
    int(__cdecl* ReadLiveStream)(unsigned char*, unsigned int);
    long long(__cdecl* SeekLiveStream)(long long, int);
    long long(__cdecl* LengthLiveStream)(void);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Recording stream read interface functions
    bool(__cdecl* OpenRecordedStream)(const PVR_RECORDING&);
    void(__cdecl* CloseRecordedStream)(void);
    int(__cdecl* ReadRecordedStream)(unsigned char*, unsigned int);
    long long(__cdecl* SeekRecordedStream)(long long, int);
    long long(__cdecl* LengthRecordedStream)(void);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Stream demux interface functions
    PVR_ERROR(__cdecl* GetStreamProperties)(PVR_STREAM_PROPERTIES*);
    DemuxPacket*(__cdecl* DemuxRead)(void);
    void(__cdecl* DemuxReset)(void);
    void(__cdecl* DemuxAbort)(void);
    void(__cdecl* DemuxFlush)(void);
    void(__cdecl* SetSpeed)(int);
    void(__cdecl* FillBuffer)(bool);
    bool(__cdecl* SeekTime)(double, bool, double*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // General stream interface functions
    bool(__cdecl* CanPauseStream)(void);
    void(__cdecl* PauseStream)(bool);
    bool(__cdecl* CanSeekStream)(void);
    bool(__cdecl* IsRealTimeStream)(void);
    PVR_ERROR(__cdecl* GetStreamTimes)(PVR_STREAM_TIMES*);
    PVR_ERROR(__cdecl* GetStreamReadChunkSize)(int*);

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
