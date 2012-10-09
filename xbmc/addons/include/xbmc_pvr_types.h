#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __PVRCLIENT_TYPES_H__
#define __PVRCLIENT_TYPES_H__

#ifdef _WIN32
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif
#include <string.h>

#include "xbmc_addon_types.h"
#include "xbmc_epg_types.h"

/*! @note Define "USE_DEMUX" at compile time if demuxing in the PVR add-on is used.
 *        Also XBMC's "DVDDemuxPacket.h" file must be in the include path of the add-on,
 *        and the add-on should set bHandlesDemuxing to true.
 */
#ifdef USE_DEMUX
#include "DVDDemuxPacket.h"
#else
struct DemuxPacket;
#endif

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

#define PVR_ADDON_NAME_STRING_LENGTH         1024
#define PVR_ADDON_URL_STRING_LENGTH          1024
#define PVR_ADDON_DESC_STRING_LENGTH         1024
#define PVR_ADDON_INPUT_FORMAT_STRING_LENGTH 32

/* using the default avformat's MAX_STREAMS value to be safe */
#define PVR_STREAM_MAX_STREAMS 20

/* current PVR API version */
#define XBMC_PVR_API_VERSION "1.5.0"

/* min. PVR API version */
#define XBMC_PVR_MIN_API_VERSION "1.5.0"

#ifdef __cplusplus
extern "C" {
#endif

  /*!
   * @brief PVR add-on error codes
   */
  typedef enum
  {
    PVR_ERROR_NO_ERROR           = 0,  /*!< @brief no error occurred */
    PVR_ERROR_UNKNOWN            = -1, /*!< @brief an unknown error occurred */
    PVR_ERROR_NOT_IMPLEMENTED    = -2, /*!< @brief the method that XBMC called is not implemented by the add-on */
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
    PVR_TIMER_STATE_NEW          = 0, /*!< @brief a new, unsaved timer */
    PVR_TIMER_STATE_SCHEDULED    = 1, /*!< @brief the timer is scheduled for recording */
    PVR_TIMER_STATE_RECORDING    = 2, /*!< @brief the timer is currently recordings */
    PVR_TIMER_STATE_COMPLETED    = 3, /*!< @brief the recording completed successfully */
    PVR_TIMER_STATE_ABORTED      = 4, /*!< @brief recording started, but was aborted */
    PVR_TIMER_STATE_CANCELLED    = 5, /*!< @brief the timer was scheduled, but was canceled */
    PVR_TIMER_STATE_CONFLICT_OK  = 6, /*!< @brief the scheduled timer conflicts with another one, but will be recorded */
    PVR_TIMER_STATE_CONFLICT_NOK = 7, /*!< @brief the scheduled timer conflicts with another one and won't be recorded */
    PVR_TIMER_STATE_ERROR        = 8  /*!< @brief the timer is scheduled, but can't be recorded for some reason */
  } PVR_TIMER_STATE;

  /*!
   * @brief Properties passed to the Create() method of an add-on.
   */
  typedef struct PVR_PROPERTIES
  {
    const char* strUserPath;           /*!< @brief path to the user profile */
    const char* strClientPath;         /*!< @brief path to this add-on */
  } PVR_PROPERTIES;

  /*!
   * @brief PVR add-on capabilities. All capabilities are set to "false" as default.
   * If a capabilty is set to true, then the corresponding methods from xbmc_pvr_dll.h need to be implemented.
   */
  typedef struct PVR_ADDON_CAPABILITIES
  {
    bool bSupportsEPG;                  /*!< @brief true if the add-on provides EPG information */
    bool bSupportsTV;                   /*!< @brief true if this add-on provides TV channels */
    bool bSupportsRadio;                /*!< @brief true if this add-on supports radio channels */
    bool bSupportsRecordings;           /*!< @brief true if this add-on supports playback of recordings stored on the backend */
    bool bSupportsTimers;               /*!< @brief true if this add-on supports the creation and editing of timers */
    bool bSupportsChannelGroups;        /*!< @brief true if this add-on supports channel groups */
    bool bSupportsChannelScan;          /*!< @brief true if this add-on support scanning for new channels on the backend */
    bool bHandlesInputStream;           /*!< @brief true if this add-on provides an input stream. false if XBMC handles the stream. */
    bool bHandlesDemuxing;              /*!< @brief true if this add-on demultiplexes packets. */
    bool bSupportsRecordingFolders;     /*!< @brief true if the backend supports timers / recordings in folders. */
    bool bSupportsRecordingPlayCount;   /*!< @brief true if the backend supports play count for recordings. */
    bool bSupportsLastPlayedPosition;   /*!< @brief true if the backend supports store/retrieve of last played position for recordings. */
  } ATTRIBUTE_PACKED PVR_ADDON_CAPABILITIES;

  /*!
   * @brief PVR stream properties
   */
  typedef struct PVR_STREAM_PROPERTIES
  {
    unsigned int iStreamCount;
    struct PVR_STREAM
    {
      unsigned int iStreamIndex;       /*!< @brief (required) stream index */
      unsigned int iPhysicalId;        /*!< @brief (required) physical index */
      unsigned int iCodecType;         /*!< @brief (required) codec type id */
      unsigned int iCodecId;           /*!< @brief (required) codec id */
      char         strLanguage[4];     /*!< @brief (required) language id */
      int          iIdentifier;        /*!< @brief (required) stream id */
      int          iFPSScale;          /*!< @brief (required) scale of 1000 and a rate of 29970 will result in 29.97 fps */
      int          iFPSRate;           /*!< @brief (required) FPS rate */
      int          iHeight;            /*!< @brief (required) height of the stream reported by the demuxer */
      int          iWidth;             /*!< @brief (required) width of the stream reported by the demuxer */
      float        fAspect;            /*!< @brief (required) display aspect ratio of the stream */
      int          iChannels;          /*!< @brief (required) amount of channels */
      int          iSampleRate;        /*!< @brief (required) sample rate */
      int          iBlockAlign;        /*!< @brief (required) block alignment */
      int          iBitRate;           /*!< @brief (required) bit rate */
      int          iBitsPerSample;     /*!< @brief (required) bits per sample */
     } stream[PVR_STREAM_MAX_STREAMS]; /*!< @brief (required) the streams */
   } ATTRIBUTE_PACKED PVR_STREAM_PROPERTIES;

  /*!
   * @brief Signal status information
   */
  typedef struct PVR_SIGNAL_STATUS
  {
    char   strAdapterName[PVR_ADDON_NAME_STRING_LENGTH];   /*!< @brief (optional) name of the adapter that's being used */
    char   strAdapterStatus[PVR_ADDON_NAME_STRING_LENGTH]; /*!< @brief (optional) status of the adapter that's being used */
    int    iSNR;                                           /*!< @brief (optional) signal/noise ratio */
    int    iSignal;                                        /*!< @brief (optional) signal strength */
    long   iBER;                                           /*!< @brief (optional) bit error rate */
    long   iUNC;                                           /*!< @brief (optional) uncorrected blocks */
    double dVideoBitrate;                                  /*!< @brief (optional) video bitrate */
    double dAudioBitrate;                                  /*!< @brief (optional) audio bitrate */
    double dDolbyBitrate;                                  /*!< @brief (optional) dolby bitrate */
  } ATTRIBUTE_PACKED PVR_SIGNAL_STATUS;

  /*!
   * @brief Menu hooks that are available in the context menus while playing a stream via this add-on.
   */
  typedef struct PVR_MENUHOOK
  {
    unsigned int iHookId;              /*!< @brief (required) this hook's identifier */
    unsigned int iLocalizedStringId;   /*!< @brief (required) the id of the label for this hook in g_localizeStrings */
  } ATTRIBUTE_PACKED PVR_MENUHOOK;

  /*!
   * @brief Representation of a TV or radio channel.
   */
  typedef struct PVR_CHANNEL
  {
    unsigned int iUniqueId;                                            /*!< @brief (required) unique identifier for this channel */
    bool         bIsRadio;                                             /*!< @brief (required) true if this is a radio channel, false if it's a TV channel */
    unsigned int iChannelNumber;                                       /*!< @brief (optional) channel number of this channel on the backend */
    char         strChannelName[PVR_ADDON_NAME_STRING_LENGTH];         /*!< @brief (optional) channel name given to this channel */
    char         strInputFormat[PVR_ADDON_INPUT_FORMAT_STRING_LENGTH]; /*!< @brief (optional) input format type. types can be found in ffmpeg/libavformat/allformats.c
                                                                                   leave empty if unknown */
    char         strStreamURL[PVR_ADDON_URL_STRING_LENGTH];            /*!< @brief (optional) the URL to use to access this channel.
                                                                                   leave empty to use this add-on to access the stream.
                                                                                   set to a path that's supported by XBMC otherwise. */
    unsigned int iEncryptionSystem;                                    /*!< @brief (optional) the encryption ID or CaID of this channel */
    char         strIconPath[PVR_ADDON_URL_STRING_LENGTH];             /*!< @brief (optional) path to the channel icon (if present) */
    bool         bIsHidden;                                            /*!< @brief (optional) true if this channel is marked as hidden */
  } ATTRIBUTE_PACKED PVR_CHANNEL;

  typedef struct PVR_CHANNEL_GROUP
  {
    char         strGroupName[PVR_ADDON_NAME_STRING_LENGTH]; /*!< @brief (required) name of this channel group */
    bool         bIsRadio;                                   /*!< @brief (required) true if this is a radio channel group, false otherwise. */
  } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP;

  typedef struct PVR_CHANNEL_GROUP_MEMBER
  {
    char         strGroupName[PVR_ADDON_NAME_STRING_LENGTH]; /*!< @brief (required) name of the channel group to add the channel to */
    unsigned int iChannelUniqueId;                           /*!< @brief (required) unique id of the member */
    unsigned int iChannelNumber;                             /*!< @brief (optional) channel number within the group */
  } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP_MEMBER;

  /*!
   * @brief Representation of a timer event.
   */
  typedef struct PVR_TIMER {
    unsigned int    iClientIndex;                              /*!< @brief (required) the index of this timer given by the client */
    int             iClientChannelUid;                         /*!< @brief (required) unique identifier of the channel to record on */
    time_t          startTime;                                 /*!< @brief (required) start time of the recording in UTC. instant timers that are sent to the add-on by xbmc will have this value set to 0 */
    time_t          endTime;                                   /*!< @brief (required) end time of the recording in UTC */
    PVR_TIMER_STATE state;                                     /*!< @brief (required) the state of this timer */
    char            strTitle[PVR_ADDON_NAME_STRING_LENGTH];    /*!< @brief (optional) title of this timer */
    char            strDirectory[PVR_ADDON_URL_STRING_LENGTH]; /*!< @brief (optional) the directory where the recording will be stored in */
    char            strSummary[PVR_ADDON_DESC_STRING_LENGTH];  /*!< @brief (optional) the summary for this timer */
    int             iPriority;                                 /*!< @brief (optional) the priority of this timer */
    int             iLifetime;                                 /*!< @brief (optional) lifetimer of this timer in days */
    bool            bIsRepeating;                              /*!< @brief (optional) true if this is a recurring timer */
    time_t          firstDay;                                  /*!< @brief (optional) the first day this recording is active in case of a repeating event */
    int             iWeekdays;                                 /*!< @brief (optional) weekday mask */
    int             iEpgUid;                                   /*!< @brief (optional) epg event id */
    unsigned int    iMarginStart;                              /*!< @brief (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
    unsigned int    iMarginEnd;                                /*!< @brief (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */
    int             iGenreType;                                /*!< @brief (optional) genre type */
    int             iGenreSubType;                             /*!< @brief (optional) genre sub type */
  } ATTRIBUTE_PACKED PVR_TIMER;
  /*!
   * @brief Representation of a recording.
   */
  typedef struct PVR_RECORDING {
    char   strRecordingId[PVR_ADDON_NAME_STRING_LENGTH];  /*!< @brief (required) unique id of the recording on the client. */
    char   strTitle[PVR_ADDON_NAME_STRING_LENGTH];        /*!< @brief (required) the title of this recording */
    char   strStreamURL[PVR_ADDON_URL_STRING_LENGTH];     /*!< @brief (required) stream URL to access this recording */
    char   strDirectory[PVR_ADDON_URL_STRING_LENGTH];     /*!< @brief (optional) directory of this recording on the client */
    char   strPlotOutline[PVR_ADDON_DESC_STRING_LENGTH];  /*!< @brief (optional) plot outline */
    char   strPlot[PVR_ADDON_DESC_STRING_LENGTH];         /*!< @brief (optional) plot */
    char   strChannelName[PVR_ADDON_NAME_STRING_LENGTH];  /*!< @brief (optional) channel name */
    char   strIconPath[PVR_ADDON_URL_STRING_LENGTH];      /*!< @brief (optional) icon path */
    char   strThumbnailPath[PVR_ADDON_URL_STRING_LENGTH]; /*!< @brief (optional) thumbnail path */
    char   strFanartPath[PVR_ADDON_URL_STRING_LENGTH];    /*!< @brief (optional) fanart path */
    time_t recordingTime;                                 /*!< @brief (optional) start time of the recording */
    int    iDuration;                                     /*!< @brief (optional) duration of the recording in seconds */
    int    iPriority;                                     /*!< @brief (optional) priority of this recording (from 0 - 100) */
    int    iLifetime;                                     /*!< @brief (optional) life time in days of this recording */
    int    iGenreType;                                    /*!< @brief (optional) genre type */
    int    iGenreSubType;                                 /*!< @brief (optional) genre sub type */
    int    iPlayCount;                                    /*!< @brief (optional) play count of this recording on the client */
  } ATTRIBUTE_PACKED PVR_RECORDING;

  /*!
   * @brief Structure to transfer the methods from xbmc_pvr_dll.h to XBMC
   */
  typedef struct PVRClient
  {
    const char*  (__cdecl* GetPVRAPIVersion)(void);
    const char*  (__cdecl* GetMininumPVRAPIVersion)(void);
    PVR_ERROR    (__cdecl* GetAddonCapabilities)(PVR_ADDON_CAPABILITIES*);
    PVR_ERROR    (__cdecl* GetStreamProperties)(PVR_STREAM_PROPERTIES*);
    const char*  (__cdecl* GetBackendName)(void);
    const char*  (__cdecl* GetBackendVersion)(void);
    const char*  (__cdecl* GetConnectionString)(void);
    PVR_ERROR    (__cdecl* GetDriveSpace)(long long*, long long*);
    PVR_ERROR    (__cdecl* MenuHook)(const PVR_MENUHOOK&);
    PVR_ERROR    (__cdecl* GetEpg)(ADDON_HANDLE, const PVR_CHANNEL&, time_t, time_t);
    int          (__cdecl* GetChannelGroupsAmount)(void);
    PVR_ERROR    (__cdecl* GetChannelGroups)(ADDON_HANDLE, bool);
    PVR_ERROR    (__cdecl* GetChannelGroupMembers)(ADDON_HANDLE, const PVR_CHANNEL_GROUP&);
    PVR_ERROR    (__cdecl* DialogChannelScan)(void);
    int          (__cdecl* GetChannelsAmount)(void);
    PVR_ERROR    (__cdecl* GetChannels)(ADDON_HANDLE, bool);
    PVR_ERROR    (__cdecl* DeleteChannel)(const PVR_CHANNEL&);
    PVR_ERROR    (__cdecl* RenameChannel)(const PVR_CHANNEL&);
    PVR_ERROR    (__cdecl* MoveChannel)(const PVR_CHANNEL&);
    PVR_ERROR    (__cdecl* DialogChannelSettings)(const PVR_CHANNEL&);
    PVR_ERROR    (__cdecl* DialogAddChannel)(const PVR_CHANNEL&);
    int          (__cdecl* GetRecordingsAmount)(void);
    PVR_ERROR    (__cdecl* GetRecordings)(ADDON_HANDLE);
    PVR_ERROR    (__cdecl* DeleteRecording)(const PVR_RECORDING&);
    PVR_ERROR    (__cdecl* RenameRecording)(const PVR_RECORDING&);
    PVR_ERROR    (__cdecl* SetRecordingPlayCount)(const PVR_RECORDING&, int);
    PVR_ERROR    (__cdecl* SetRecordingLastPlayedPosition)(const PVR_RECORDING&, int);
    int          (__cdecl* GetRecordingLastPlayedPosition)(const PVR_RECORDING&);
    int          (__cdecl* GetTimersAmount)(void);
    PVR_ERROR    (__cdecl* GetTimers)(ADDON_HANDLE);
    PVR_ERROR    (__cdecl* AddTimer)(const PVR_TIMER&);
    PVR_ERROR    (__cdecl* DeleteTimer)(const PVR_TIMER&, bool);
    PVR_ERROR    (__cdecl* UpdateTimer)(const PVR_TIMER&);
    bool         (__cdecl* OpenLiveStream)(const PVR_CHANNEL&);
    void         (__cdecl* CloseLiveStream)(void);
    int          (__cdecl* ReadLiveStream)(unsigned char*, unsigned int);
    long long    (__cdecl* SeekLiveStream)(long long, int);
    long long    (__cdecl* PositionLiveStream)(void);
    long long    (__cdecl* LengthLiveStream)(void);
    int          (__cdecl* GetCurrentClientChannel)(void);
    bool         (__cdecl* SwitchChannel)(const PVR_CHANNEL&);
    PVR_ERROR    (__cdecl* SignalStatus)(PVR_SIGNAL_STATUS&);
    const char*  (__cdecl* GetLiveStreamURL)(const PVR_CHANNEL&);
    bool         (__cdecl* OpenRecordedStream)(const PVR_RECORDING&);
    void         (__cdecl* CloseRecordedStream)(void);
    int          (__cdecl* ReadRecordedStream)(unsigned char*, unsigned int);
    long long    (__cdecl* SeekRecordedStream)(long long, int);
    long long    (__cdecl* PositionRecordedStream)(void);
    long long    (__cdecl* LengthRecordedStream)(void);
    void         (__cdecl* DemuxReset)(void);
    void         (__cdecl* DemuxAbort)(void);
    void         (__cdecl* DemuxFlush)(void);
    DemuxPacket* (__cdecl* DemuxRead)(void);
    unsigned int (__cdecl* GetChannelSwitchDelay)(void);
    bool         (__cdecl* CanPauseStream)(void);
    void         (__cdecl* PauseStream)(bool);
    bool         (__cdecl* CanSeekStream)(void);
  } PVRClient;

#ifdef __cplusplus
}
#endif

#endif //__PVRCLIENT_TYPES_H__
