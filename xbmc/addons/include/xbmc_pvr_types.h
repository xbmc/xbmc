#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

/*! @note Define "USE_DEMUX" on compile time if demuxing inside pvr
 *        addon is used. Also XBMC's "DVDDemuxPacket.h" file must be inside
 *        the include path of the pvr addon.
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

/*! @name PVR entry content event types */
//@{
#define EPG_EVENT_CONTENTMASK_MOVIEDRAMA               0x10
#define EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS       0x20
#define EPG_EVENT_CONTENTMASK_SHOW                     0x30
#define EPG_EVENT_CONTENTMASK_SPORTS                   0x40
#define EPG_EVENT_CONTENTMASK_CHILDRENYOUTH            0x50
#define EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE         0x60
#define EPG_EVENT_CONTENTMASK_ARTSCULTURE              0x70
#define EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
#define EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE       0x90
#define EPG_EVENT_CONTENTMASK_LEISUREHOBBIES           0xA0
#define EPG_EVENT_CONTENTMASK_SPECIAL                  0xB0
#define EPG_EVENT_CONTENTMASK_USERDEFINED              0xF0
//@}


/* using the default avformat's MAX_STREAMS value to be safe */
#define PVR_STREAM_MAX_STREAMS 20

#ifdef __cplusplus
extern "C" {
#endif

  /*!
   * @brief Handle used to return data from the PVR add-on to CPVRClient
   */
  struct PVR_HANDLE_STRUCT
  {
    void *callerAddress;  /*!< address of the caller */
    void *dataAddress;    /*!< address to store data in */
    int   dataIdentifier; /*!< parameter to pass back when calling the callback */
  };
  typedef PVR_HANDLE_STRUCT *PVR_HANDLE;

  /*! \brief PVR Client Error Codes
   */
  typedef enum
  {
    PVR_ERROR_NO_ERROR          = 0,
    PVR_ERROR_UNKOWN            = -1,
    PVR_ERROR_NOT_IMPLEMENTED   = -2,
    PVR_ERROR_SERVER_ERROR      = -3,
    PVR_ERROR_SERVER_TIMEOUT    = -4,
    PVR_ERROR_NOT_SYNC          = -5,
    PVR_ERROR_NOT_DELETED       = -6,
    PVR_ERROR_NOT_SAVED         = -7,
    PVR_ERROR_RECORDING_RUNNING = -8,
    PVR_ERROR_ALREADY_PRESENT   = -9,
    PVR_ERROR_NOT_POSSIBLE      = -10
  } PVR_ERROR;

  /*!
   * @brief Properties passed to the Create() method of an add-on.
   */
  typedef struct PVR_PROPERTIES
  {
    int         iClienId;              /*!< @brief (required) database ID of the client */
    const char *strUserPath;           /*!< @brief (required) path to the user profile */
    const char *strClientPath;         /*!< @brief (required) path to this add-on */
  } PVR_PROPERTIES;

  /*!
   * @brief PVR add-on event codes, sent via PVRManager callback.
   */
  typedef enum
  {
    PVR_EVENT_UNKNOWN               = 0,
    PVR_EVENT_CLOSE                 = 1,
    PVR_EVENT_RECORDINGS_CHANGE     = 2,
    PVR_EVENT_CHANNELS_CHANGE       = 3,
    PVR_EVENT_TIMERS_CHANGE         = 4,
    PVR_EVENT_CHANNEL_GROUPS_CHANGE = 5
  } PVR_EVENT;

  /*!
   * @brief PVR add-on capabilities. All capabilities are set to "false" as default.
   */
  typedef struct PVR_ADDON_CAPABILITIES
  {
    bool bSupportsChannelLogo;          /*!< @brief (optional) true if this add-on supports channel logos */
    bool bSupportsChannelSettings;      /*!< @brief (optional) true if this add-on supports changing channel settings on the backend */
    bool bSupportsTimeshift;            /*!< @brief (optional) true if the backend will handle timeshift. false if XBMC should handle it. */
    bool bSupportsEPG;                  /*!< @brief (optional) true if the add-on provides EPG information */
    bool bSupportsTV;                   /*!< @brief (optional) true if this add-on provides TV channels */
    bool bSupportsRadio;                /*!< @brief (optional) true if this add-on supports radio channels */
    bool bSupportsRecordings;           /*!< @brief (optional) true if this add-on supports playback of recordings stored on the backend */
    bool bSupportsTimers;               /*!< @brief (optional) true if this add-on supports the creation and editing of timers */
    bool bSupportsChannelGroups;        /*!< @brief (optional) true if this add-on supports channel groups */
    bool bSupportsChannelScan;          /*!< @brief (optional) true if this add-on support scanning for new channels on the backend */
    bool bHandlesInputStream;           /*!< @brief (optional) true if this add-on provides an input stream. false if XBMC handles the stream. */
    bool bHandlesDemuxing;              /*!< @brief (optional) true if this add-on demultiplexes packets. */
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
      unsigned int iIdentifier;        /*!< @brief (required) stream id */
      unsigned int iFPSScale;          /*!< @brief (required) scale of 1000 and a rate of 29970 will result in 29.97 fps */
      unsigned int iFPSRate;           /*!< @brief (required) FPS rate */
      unsigned int iHeight;            /*!< @brief (required) height of the stream reported by the demuxer */
      unsigned int iWidth;             /*!< @brief (required) width of the stream reported by the demuxer */
      float        fAspect;            /*!< @brief (required) display aspect ratio of the stream */
      unsigned int iChannels;          /*!< @brief (required) amount of channels */
      unsigned int iSampleRate;        /*!< @brief (required) sample rate */
      unsigned int iBlockAlign;        /*!< @brief (required) block alignment */
      unsigned int iBitRate;           /*!< @brief (required) bit rate */
      unsigned int iBitsPerSample;     /*!< @brief (required) bits per sample */
     } stream[PVR_STREAM_MAX_STREAMS]; /*!< @brief (required) the streams */
   } ATTRIBUTE_PACKED PVR_STREAM_PROPERTIES;

  /*!
   * @brief Signal status information
   */
  typedef struct PVR_SIGNAL_STATUS
  {
    char   strAdapterName[1024];       /*!< @brief (optional) name of the adapter that's being used */
    char   strAdapterStatus[1024];     /*!< @brief (optional) status of the adapter that's being used */
    int    iSNR;                       /*!< @brief (optional) signal/noise ratio */
    int    iSignal;                    /*!< @brief (optional) signal strength */
    long   iBER;                       /*!< @brief (optional) bit error rate */
    long   iUNC;                       /*!< @brief (optional) uncorrected blocks */
    double dVideoBitrate;              /*!< @brief (optional) video bitrate */
    double dAudioBitrate;              /*!< @brief (optional) audio bitrate */
    double dDolbyBitrate;              /*!< @brief (optional) dolby bitrate */
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
    unsigned int iUniqueId;            /*!< @brief (required) unique identifier for this channel */
    bool         bIsRadio;             /*!< @brief (required) true if this is a radio channel, false if it's a TV channel */
    unsigned int iChannelNumber;       /*!< @brief (optional) channel number of this channel on the backend */
    const char * strChannelName;       /*!< @brief (optional) channel name given to this channel */
    const char * strInputFormat;       /*!< @brief (optional) input format type. types can be found in ffmpeg/libavformat/allformats.c
                                                              leave empty if unknown */
    const char * strStreamURL;         /*!< @brief (optional) the URL to use to access this channel.
                                                              leave empty to use this add-on to access the stream.
                                                              set to a path that's supported by XBMC otherwise. */
    unsigned int iEncryptionSystem;    /*!< @brief (optional) the encryption ID or CaID of this channel */
    const char * strIconPath;          /*!< @brief (optional) path to the channel icon (if present) */
    bool         bIsHidden;            /*!< @brief (optional) true if this channel is marked as hidden */
  } ATTRIBUTE_PACKED PVR_CHANNEL;

  typedef struct PVR_CHANNEL_GROUP
  {
    const char * strGroupName;         /*!< @brief (required) name of this channel group */
    bool         bIsRadio;             /*!< @brief (required) true if this is a radio channel group, false otherwise. */
  } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP;

  typedef struct PVR_CHANNEL_GROUP_MEMBER
  {
    const char * strGroupName;         /*!< @brief (required) name of the channel group to add the channel to */
    unsigned int iChannelUniqueId;     /*!< @brief (required) unique id of the member */
    unsigned int iChannelNumber;       /*!< @brief (optional) channel number within the group */
  } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP_MEMBER;

  /*!
   * @brief Representation of an EPG event.
   */
  typedef struct EPG_TAG {
    unsigned int  iUniqueBroadcastId;  /*!< @brief (required) identifier for this event */
    const char *  strTitle;            /*!< @brief (required) this event's title */
    unsigned int  iChannelNumber;      /*!< @brief (required) the number of the channel this event occurs on */
    time_t        startTime;           /*!< @brief (required) start time in UTC */
    time_t        endTime;             /*!< @brief (required) end time in UTC */
    const char *  strPlotOutline;      /*!< @brief (optional) plot outline */
    const char *  strPlot;             /*!< @brief (optional) plot */
    const char *  strIconPath;         /*!< @brief (optional) icon path */
    int           iGenreType;          /*!< @brief (optional) genre type */
    int           iGenreSubType;       /*!< @brief (optional) genre sub type */
    time_t        firstAired;          /*!< @brief (optional) first aired in UTC */
    int           iParentalRating;     /*!< @brief (optional) parental rating */
    int           iStarRating;         /*!< @brief (optional) star rating */
    bool          bNotify;             /*!< @brief (optional) notify the user when this event starts */
    int           iSeriesNumber;       /*!< @brief (optional) series number */
    int           iEpisodeNumber;      /*!< @brief (optional) episode number */
    int           iEpisodePartNumber;  /*!< @brief (optional) episode part number */
    const char *  strEpisodeName;      /*!< @brief (optional) episode name */
  } ATTRIBUTE_PACKED EPG_TAG;

  /*!
   * @brief Representation of a timer event.
   */
  typedef struct PVR_TIMER {
    unsigned int  iClientIndex;        /*!< @brief (required) the index of this timer given by the client */
    int           iClientChannelUid;   /*!< @brief (required) unique identifier of the channel to record on */
    time_t        startTime;           /*!< @brief (required) start time of the recording in UTC */
    time_t        endTime;             /*!< @brief (required) end time of the recording in UTC */
    bool          bIsActive;           /*!< @brief (required) true if this timer is active, false if it's inactive */
    const char *  strTitle;            /*!< @brief (optional) title of this timer */
    const char *  strDirectory;        /*!< @brief (optional) the directory where the recording will be stored in */
    const char *  strSummary;          /*!< @brief (optional) the summary for this timer */
    bool          bIsRecording;        /*!< @brief (optional) true if this timer is currently recording, false otherwise */
    int           iPriority;           /*!< @brief (optional) the priority of this timer */
    int           iLifetime;           /*!< @brief (optional) lifetimer of this timer in days */
    bool          bIsRepeating;        /*!< @brief (optional) true if this is a recurring timer */
    time_t        firstDay;            /*!< @brief (optional) the first day this recording is active in case of a repeating event */
    int           iWeekdays;           /*!< @brief (optional) weekday mask */
    int           iEpgUid;             /*!< @brief (optional) epg event id */
    unsigned int  iMarginStart;        /*!< @brief (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
    unsigned int  iMarginEnd;          /*!< @brief (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */
    int           iGenreType;          /*!< @brief (optional) genre type */
    int           iGenreSubType;       /*!< @brief (optional) genre sub type */
  } ATTRIBUTE_PACKED PVR_TIMER;

  /*!
   * @brief Representation of a recording.
   */
  typedef struct PVR_RECORDING {
    int           iClientIndex;         /*!< @brief (required) index number of the recording on the client */
    const char *  strTitle;             /*!< @brief (required) the title of this recording */
    const char *  strStreamURL;         /*!< @brief (required) stream URL to access this recording */
    const char *  strDirectory;         /*!< @brief (optional) directory of this recording on the client */
    const char *  strPlotOutline;       /*!< @brief (optional) plot outline */
    const char *  strPlot;              /*!< @brief (optional) plot */
    const char *  strChannelName;       /*!< @brief (optional) channel name */
    time_t        recordingTime;        /*!< @brief (optional) start time of the recording */
    int           iDuration;            /*!< @brief (optional) duration of the recording */
    int           iPriority;            /*!< @brief (optional) priority of this recording (from 0 - 100) */
    int           iLifetime;            /*!< @brief (optional) life time in days of this recording */
    int           iGenreType;           /*!< @brief (optional) genre type */
    int           iGenreSubType;        /*!< @brief (optional) genre sub type */
  } ATTRIBUTE_PACKED PVR_RECORDING;

  /*!
   * @brief Structure to transfer the PVR functions to XBMC
   */
  typedef struct PVRClient
  {
    /** @name PVR server methods */
    //@{
    PVR_ERROR    (__cdecl* GetAddonCapabilities)(PVR_ADDON_CAPABILITIES *pCapabilities);
    PVR_ERROR    (__cdecl* GetStreamProperties)(PVR_STREAM_PROPERTIES *pProperties);
    const char * (__cdecl* GetBackendName)(void);
    const char * (__cdecl* GetBackendVersion)(void);
    const char * (__cdecl* GetConnectionString)(void);
    PVR_ERROR    (__cdecl* GetDriveSpace)(long long *iTotal, long long *iUsed);
    PVR_ERROR    (__cdecl* DialogChannelScan)(void);
    PVR_ERROR    (__cdecl* MenuHook)(const PVR_MENUHOOK &menuhook);
    //@}

    /** @name PVR EPG methods */
    //@{
    PVR_ERROR    (__cdecl* GetEpg)(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
    //@}

    /** @name PVR channel group methods */
    //@{
    int          (__cdecl* GetChannelGroupsAmount)(void);
    PVR_ERROR    (__cdecl* GetChannelGroups)(PVR_HANDLE handle, bool bRadio);
    PVR_ERROR    (__cdecl* GetChannelGroupMembers)(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group);
    //@}

    /** @name PVR channel methods */
    //@{
    int          (__cdecl* GetChannelsAmount)(void);
    PVR_ERROR    (__cdecl* GetChannels)(PVR_HANDLE handle, bool bRadio);
    PVR_ERROR    (__cdecl* DeleteChannel)(const PVR_CHANNEL &channel);
    PVR_ERROR    (__cdecl* RenameChannel)(const PVR_CHANNEL &channel);
    PVR_ERROR    (__cdecl* MoveChannel)(const PVR_CHANNEL &channel);
    PVR_ERROR    (__cdecl* DialogChannelSettings)(const PVR_CHANNEL &channel);
    PVR_ERROR    (__cdecl* DialogAddChannel)(const PVR_CHANNEL &channel);
    //@}

    /** @name PVR recording methods */
    //@{
    int          (__cdecl* GetRecordingsAmount)(void);
    PVR_ERROR    (__cdecl* GetRecordings)(PVR_HANDLE handle);
    PVR_ERROR    (__cdecl* DeleteRecording)(const PVR_RECORDING &recording);
    PVR_ERROR    (__cdecl* RenameRecording)(const PVR_RECORDING &recording);
    //@}

    /** @name PVR timer methods */
    //@{
    int          (__cdecl* GetTimersAmount)(void);
    PVR_ERROR    (__cdecl* GetTimers)(PVR_HANDLE handle);
    PVR_ERROR    (__cdecl* AddTimer)(const PVR_TIMER &timer);
    PVR_ERROR    (__cdecl* DeleteTimer)(const PVR_TIMER &timer, bool bForceDelete);
    PVR_ERROR    (__cdecl* UpdateTimer)(const PVR_TIMER &timer);
    //@}

    /** @name PVR live stream methods */
    //@{
    bool         (__cdecl* OpenLiveStream)(const PVR_CHANNEL &channel);
    void         (__cdecl* CloseLiveStream)(void);
    int          (__cdecl* ReadLiveStream)(unsigned char *pBuffer, unsigned int iBufferSize);
    long long    (__cdecl* SeekLiveStream)(long long iPosition, int iWhence /* = SEEK_SET */);
    long long    (__cdecl* PositionLiveStream)(void);
    long long    (__cdecl* LengthLiveStream)(void);
    int          (__cdecl* GetCurrentClientChannel)(void);
    bool         (__cdecl* SwitchChannel)(const PVR_CHANNEL &channel);
    PVR_ERROR    (__cdecl* SignalStatus)(PVR_SIGNAL_STATUS &signalStatus);
    const char*  (__cdecl* GetLiveStreamURL)(const PVR_CHANNEL &channel);
    //@}

    /** @name PVR recording stream methods */
    //@{
    bool         (__cdecl* OpenRecordedStream)(const PVR_RECORDING &recording);
    void         (__cdecl* CloseRecordedStream)(void);
    int          (__cdecl* ReadRecordedStream)(unsigned char *pBuffer, unsigned int iBufferSize);
    long long    (__cdecl* SeekRecordedStream)(long long iPosition, int iWhence /* = SEEK_SET */);
    long long    (__cdecl* PositionRecordedStream)(void);
    long long    (__cdecl* LengthRecordedStream)(void);
    //@}

    /** @name PVR demultiplexer methods */
    //@{
    void         (__cdecl* DemuxReset)(void);
    void         (__cdecl* DemuxAbort)(void);
    void         (__cdecl* DemuxFlush)(void);
    DemuxPacket* (__cdecl* DemuxRead)(void);
    //@}

  } PVRClient;

#ifdef __cplusplus
}
#endif

#endif //__PVRCLIENT_TYPES_H__
