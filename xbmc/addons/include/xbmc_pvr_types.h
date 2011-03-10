#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

/**
 * \file xbmc_pvr_types.h
 * \brief Implementation of a PVR Client API Interface.
 * It at common data structures which a shared between XBMC and PVR clients
 *
 * \author Team XBMC
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

/*! \note Define "USE_DEMUX" on compile time if demuxing inside pvr
 * addon is used. Also XBMC's "DVDDemuxPacket.h" file must be inside
 * the include path of the pvr addon.
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

  /*! \brief PVR Event contents, used to identify the genre of a epg entry
   */
#define EVCONTENTMASK_MOVIEDRAMA               0x10
#define EVCONTENTMASK_NEWSCURRENTAFFAIRS       0x20
#define EVCONTENTMASK_SHOW                     0x30
#define EVCONTENTMASK_SPORTS                   0x40
#define EVCONTENTMASK_CHILDRENYOUTH            0x50
#define EVCONTENTMASK_MUSICBALLETDANCE         0x60
#define EVCONTENTMASK_ARTSCULTURE              0x70
#define EVCONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
#define EVCONTENTMASK_EDUCATIONALSCIENCE       0x90
#define EVCONTENTMASK_LEISUREHOBBIES           0xA0
#define EVCONTENTMASK_SPECIAL                  0xB0
#define EVCONTENTMASK_USERDEFINED              0xF0

#ifdef __cplusplus
extern "C" {
#endif

  /*! \brief PVR Client Return Data handle
   */
  struct PVRHANDLE_STRUCT
  {
    void* CALLER_ADDRESS;
    void* DATA_ADDRESS;
    int   DATA_IDENTIFIER;
  };
  typedef PVRHANDLE_STRUCT* PVRHANDLE;

  /*! \brief PVR Client startup properties
   * Passed to the Create function
   */
  struct PVR_PROPS
  {
    int clientID;
    const char *userpath;
    const char *clientpath;
  };

  /*! \brief PVR Client Error Codes
   */
  typedef enum {
    PVR_ERROR_NO_ERROR             = 0,
    PVR_ERROR_UNKOWN               = -1,
    PVR_ERROR_NOT_IMPLEMENTED      = -2,
    PVR_ERROR_SERVER_ERROR         = -3,
    PVR_ERROR_SERVER_TIMEOUT       = -4,
    PVR_ERROR_NOT_SYNC             = -5,
    PVR_ERROR_NOT_DELETED          = -6,
    PVR_ERROR_NOT_SAVED            = -7,
    PVR_ERROR_RECORDING_RUNNING    = -8,
    PVR_ERROR_ALREADY_PRESENT      = -9,
    PVR_ERROR_NOT_POSSIBLE         = -10,
  } PVR_ERROR;

  /*! \brief PVR Client Event Codes
   * Sent via PVRManager callback
   */
  typedef enum {
    PVR_EVENT_UNKNOWN              = 0,
    PVR_EVENT_CLOSE                = 1,
    PVR_EVENT_RECORDINGS_CHANGE    = 2,
    PVR_EVENT_CHANNELS_CHANGE      = 3,
    PVR_EVENT_TIMERS_CHANGE        = 4,
  } PVR_EVENT;

#if PRAGMA_PACK
#pragma pack(1)
#endif

  /*! \brief PVR Client Properties
   * Returned on client initialization
   */
  typedef struct PVR_SERVERPROPS {
    bool SupportChannelLogo;            /**< \brief Client support transfer of channel logos */
    bool SupportChannelSettings;        /**< \brief Client support changing channels on backend */
    bool SupportTimeShift;              /**< \brief Client handle Live TV Timeshift, otherwise it is handled by XBMC */
    bool SupportEPG;                    /**< \brief Client provide EPG information */
    bool SupportTV;                     /**< \brief Client provide TV Channels, is false for Radio only clients */
    bool SupportRadio;                  /**< \brief Client provide also Radio Channels */
    bool SupportRecordings;             /**< \brief Client support playback of recordings stored on the backend */
    bool SupportTimers;                 /**< \brief Client support creation and editing of timers */
    bool SupportDirector;               /**< \brief Client provide information about multifeed channels, like Sky Select */
    bool SupportBouquets;               /**< \brief Client support Bouqets */
    bool SupportChannelScan;            /**< \brief Client support Channelscan */
    bool HandleInputStream;             /**< \brief Input stream is handled by the client if set, can be false for http */
    bool HandleDemuxing;                /**< \brief Demux of stream is handled by the client, as example TVFrontend (htsp protocol) */
  } ATTRIBUTE_PACKED PVR_SERVERPROPS;

  /*! \brief  PVR Stream Properties
   * Returned on request
   */
  typedef struct PVR_STREAMPROPS {
#define PVR_STREAM_MAX_STREAMS 16
    int nstreams;
    struct PVR_STREAM {
      int id;
      int physid;
      unsigned int codec_type;
      unsigned int codec_id;
      char language[4];
      int identifier;

      int fpsscale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
      int fpsrate;
      int height; // height of the stream reported by the demuxer
      int width; // width of the stream reported by the demuxer
      float aspect; // display aspect of stream

      int channels;
      int samplerate;
      int blockalign;
      int bitrate;
      int bits_per_sample;
    } stream[PVR_STREAM_MAX_STREAMS];
  } ATTRIBUTE_PACKED PVR_STREAMPROPS;


  /*! \brief PVR channel defination
   *
   * Is used by the TransferChannelEntry function to inform XBMC that this
   * channel is present, also if a channel is opened this structure is passed in.
   */
  typedef struct PVR_CHANNEL {
    int             uid;                /**< \brief Unique identifier for this channel */
    int             number;             /**< \brief The backend channel number */

    const char     *name;               /**< \brief Channel name provided by the Broadcast */
    const char     *callsign;           /**< \brief Channel name provided by the user (if present) */
    const char     *iconpath;           /**< \brief Path to the channel icon (if present) */

    int             encryption;         /**< \brief This is a encrypted channel and have a CA Id */
    bool            radio;              /**< \brief This is a radio channel */
    bool            hide;               /**< \brief This channel is hidden by the user */
    bool            recording;          /**< \brief This channel is currently recording */

    int             bouquet;            /**< \brief Bouquet ID this channel have (if supported) */

    bool            multifeed;          /**< \brief This is a multifeed channel */
    int             multifeed_master;   /**< \brief The Master multifeed channel, multifeed_master==number for master itself */
    int             multifeed_number;   /**< \brief The own number inside multifeed channel list */

    const char     *input_format;       /**< \brief Input format type based upon ffmpeg/libavformat/allformats.c
                                             if it is unknown leave it empty */

    const char     *stream_url;         /**< \brief The Stream URL to access this channel, it can be all types of protocol
                                             and types are supported by XBMC or in case the client read the stream leave
                                             it empty as URL. */
  } ATTRIBUTE_PACKED PVR_CHANNEL;

  /*! \brief EPG Bouquet Definition
   */
  typedef struct PVR_BOUQUET {
    char*  Name;
    char*  Category;
    int    Number;
  } ATTRIBUTE_PACKED PVR_BOUQUET;

  /*! \brief EPG Programme Definition
   *
   * Used to signify an individual broadcast, whether it is also a recording, timer etc.
   */
  typedef struct PVR_PROGINFO {
    unsigned int  uid;
    int           channum;
    const char   *title;
    const char   *subtitle;
    const char   *description;
    time_t        starttime;
    time_t        endtime;
    int           genre_type;
    int           genre_sub_type;
    int           parental_rating;
  } ATTRIBUTE_PACKED PVR_PROGINFO;

  /*! \brief TV Timer Definition
   */
  typedef struct PVR_TIMERINFO {
    int           index;
    int           active;
    const char   *title;
    const char   *directory;
    int           channelNum;
    time_t        starttime;
    time_t        endtime;
    time_t        firstday;
    int           recording;
    int           priority;
    int           lifetime;
    int           repeat;
    int           repeatflags;
    int           epgid;
  } ATTRIBUTE_PACKED PVR_TIMERINFO;

  /*! \brief PVR recording defination
   *
   * Is used by the TransferRecordinglEntry function to inform XBMC about this
   * recording, also if a recording is opened this structure is passed in.
   */
  typedef struct PVR_RECORDINGINFO {
    int             index;              /**< \brief The index number of this recording, it must always set and
                                             is used to identify this recording later */
    const char     *directory;          /**< \brief The directory of this recording (used to create a organized structure).
                                             It is not required, if it is not supported on the backend
                                             leave it open */
    const char     *title;              /**< \brief The name of this recording */
    const char     *subtitle;           /**< \brief Optional subtitle */
    const char     *description;        /**< \brief Optional description of the recording content */
    const char     *channel_name;       /**< \brief Optional channel name */
    time_t          recording_time;     /**< \brief Optional time where this recording was taken */
    int             duration;           /**< \brief The duration in seconds of this recording */
    int             priority;           /**< \brief Optional priority of this recording (from 0 - 100) */
    int             lifetime;           /**< \brief Optional life time in days of this recording */
    const char     *stream_url;         /**< \brief The Stream URL to access this recording, it can be all types of protocol
                                             and types are supported by XBMC or in case the client read the stream leave
                                             it empty as URL. You can also define to play all files inside a folder if you
                                             use a asterix, as example: "/media/disk/recordings/Alien/ *.ts", in this example
                                             all files in the directory "/media/disk/recordings/Alien" with the "ts" extensions
                                             are played by XBMC. */
  } ATTRIBUTE_PACKED PVR_RECORDINGINFO;

  /*! \brief TV Stream Signal Quality Information
   */
  typedef struct PVR_SIGNALQUALITY {
    char          frontend_name[1024];
    char          frontend_status[1024];
    int           snr;
    int           signal;
    long          ber;
    long          unc;
    double        video_bitrate;
    double        audio_bitrate;
    double        dolby_bitrate;
  } ATTRIBUTE_PACKED PVR_SIGNALQUALITY;

  /*! \brief PVR Addon menu hook element
   *
   * Are available in the context menus of the TV window, to perfrom a addon related action.
   */
  typedef struct PVR_MENUHOOK {
    int           hook_id;              /**< \brief An identifier to know what hook is called back to the addon */
    int           string_id;            /**< \brief The id to a name for this item inside the language files */
  } ATTRIBUTE_PACKED PVR_MENUHOOK;

  /*! \brief PVR Recordings cut mark action types.
   */
  typedef enum {
    CUT         = 0,
    MUTE        = 1,
    SCENE       = 2,
    COMM_BREAK  = 3
  } CUT_MARK_ACTION;

  /*! \brief PVR Recordings cut mark element.
   */
  typedef struct PVR_CUT_MARK {
    long long       start;              /**< \brief Start position in milliseconds */
    long long       stop;               /**< \brief Stop position in milliseconds */
    CUT_MARK_ACTION action;             /**< \brief the action to be performed */
  } ATTRIBUTE_PACKED PVR_CUT_MARK;

#if PRAGMA_PACK
#pragma pack()
#endif

  /*! \brief Structure to transfer the PVR functions to XBMC
   */
  typedef struct PVRClient
  {
    /** \name PVR General Functions */
    PVR_ERROR (__cdecl* GetProperties)(PVR_SERVERPROPS *props);
    PVR_ERROR (__cdecl* GetStreamProperties)(PVR_STREAMPROPS *props);
    const char* (__cdecl* GetBackendName)();
    const char* (__cdecl* GetBackendVersion)();
    const char* (__cdecl* GetConnectionString)();
    PVR_ERROR (__cdecl* GetDriveSpace)(long long *total, long long *used);
    PVR_ERROR (__cdecl* GetBackendTime)(time_t *localTime, int *gmtOffset);
    PVR_ERROR (__cdecl* DialogChannelScan)();
    PVR_ERROR (__cdecl* MenuHook)(const PVR_MENUHOOK &menuhook);

    /** \name PVR EPG Functions */
    PVR_ERROR (__cdecl* RequestEPGForChannel)(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);

    /** \name PVR Bouquets Functions */
    int (__cdecl* GetNumBouquets)();
    PVR_ERROR (__cdecl* RequestBouquetsList)(PVRHANDLE handle, int radio);

    /** \name PVR Channel Functions */
    int (__cdecl* GetNumChannels)();
    PVR_ERROR (__cdecl* RequestChannelList)(PVRHANDLE handle, int radio);
    PVR_ERROR (__cdecl* DeleteChannel)(unsigned int number);
    PVR_ERROR (__cdecl* RenameChannel)(unsigned int number, const char *newname);
    PVR_ERROR (__cdecl* MoveChannel)(unsigned int number, unsigned int newnumber);
    PVR_ERROR (__cdecl* DialogChannelSettings)(const PVR_CHANNEL &channelinfo);
    PVR_ERROR (__cdecl* DialogAddChannel)(const PVR_CHANNEL &channelinfo);

    /** \name PVR Recording Functions */
    int (__cdecl* GetNumRecordings)();
    PVR_ERROR (__cdecl* RequestRecordingsList)(PVRHANDLE handle);
    PVR_ERROR (__cdecl* DeleteRecording)(const PVR_RECORDINGINFO &recinfo);
    PVR_ERROR (__cdecl* RenameRecording)(const PVR_RECORDINGINFO &recinfo, const char *newname);

    /** \name PVR Recording cut marks Functions */
    bool (__cdecl* HaveCutmarks)();
    PVR_ERROR (__cdecl* RequestCutMarksList)(PVRHANDLE handle);
    PVR_ERROR (__cdecl* AddCutMark)(const PVR_CUT_MARK &cutmark);
    PVR_ERROR (__cdecl* DeleteCutMark)(const PVR_CUT_MARK &cutmark);
    PVR_ERROR (__cdecl* StartCut)();

    /** \name PVR Timer Functions */
    int (__cdecl* GetNumTimers)();
    PVR_ERROR (__cdecl* RequestTimerList)(PVRHANDLE handle);
    PVR_ERROR (__cdecl* AddTimer)(const PVR_TIMERINFO &timerinfo);
    PVR_ERROR (__cdecl* DeleteTimer)(const PVR_TIMERINFO &timerinfo, bool force);
    PVR_ERROR (__cdecl* RenameTimer)(const PVR_TIMERINFO &timerinfo, const char *newname);
    PVR_ERROR (__cdecl* UpdateTimer)(const PVR_TIMERINFO &timerinfo);

    /** \name PVR Live Stream Functions */
    bool (__cdecl* OpenLiveStream)(const PVR_CHANNEL &channelinfo);
    void (__cdecl* CloseLiveStream)();
    int (__cdecl* ReadLiveStream)(unsigned char* buf, int buf_size);
    long long (__cdecl* SeekLiveStream)(long long pos, int whence);
    long long (__cdecl* PositionLiveStream)(void);
    long long (__cdecl* LengthLiveStream)(void);
    int (__cdecl* GetCurrentClientChannel)();
    bool (__cdecl* SwitchChannel)(const PVR_CHANNEL &channelinfo);
    PVR_ERROR (__cdecl* SignalQuality)(PVR_SIGNALQUALITY &qualityinfo);
    const char* (__cdecl* GetLiveStreamURL)(const PVR_CHANNEL &channelinfo);

    /** \name PVR Secondary Stream Functions */
    bool (__cdecl* SwapLiveTVSecondaryStream)();
    bool (__cdecl* OpenSecondaryStream)(const PVR_CHANNEL &channelinfo);
    void (__cdecl* CloseSecondaryStream)();
    int (__cdecl* ReadSecondaryStream)(unsigned char* buf, int buf_size);

    /** \name PVR Recording Stream Functions */
    bool (__cdecl* OpenRecordedStream)(const PVR_RECORDINGINFO &recinfo);
    void (__cdecl* CloseRecordedStream)(void);
    int (__cdecl* ReadRecordedStream)(unsigned char* buf, int buf_size);
    long long (__cdecl* SeekRecordedStream)(long long pos, int whence);
    long long (__cdecl* PositionRecordedStream)(void);
    long long (__cdecl* LengthRecordedStream)(void);

    /** \name Demuxer Interface */
    void (__cdecl* DemuxReset)();
    void (__cdecl* DemuxAbort)();
    void (__cdecl* DemuxFlush)();
    DemuxPacket* (__cdecl* DemuxRead)();

  } PVRClient;

#ifdef __cplusplus
}
#endif

#endif //__PVRCLIENT_TYPES_H__
