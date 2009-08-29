/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * Common data structures shared between XBMC and PVR clients
 */

#ifndef __PVRCLIENT_TYPES_H__
#define __PVRCLIENT_TYPES_H__

#define MIN_XBMC_PVRDLL_API 1

//#define __cdecl
//#define __declspec(x)
#include <string.h>
#include <time.h>
#include "xbmc_addon_types.h"

#ifndef _LINUX
enum CodecID;
#else
extern "C" {
#if (defined USE_EXTERNAL_FFMPEG)
  #if (defined HAVE_LIBAVCODEC_AVCODEC_H)
    #include <libavcodec/avcodec.h>
  #elif (defined HAVE_FFMPEG_AVCODEC_H)
    #include <ffmpeg/avcodec.h>
  #endif
#else
  #include "../../cores/ffmpeg/avcodec.h"
#endif
}
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

#ifdef __cplusplus
extern "C" {
#endif

  typedef void*         PVRHANDLE;
  typedef void*         PVRDEMUXHANDLE;

  /**
  * PVR Client Error Codes
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
    PVR_ERROR_ALREADY_PRESENT      = -9
  } PVR_ERROR;

  /**
  * PVR Client Event Codes
  * Sent via PVRManager callback
  */
  typedef enum {
    PVR_EVENT_UNKNOWN              = 0,
    PVR_EVENT_CLOSE                = 1,
    PVR_EVENT_RECORDINGS_CHANGE    = 2,
    PVR_EVENT_CHANNELS_CHANGE      = 3,
    PVR_EVENT_TIMERS_CHANGE        = 4
  } PVR_EVENT;

#if PRAGMA_PACK
#pragma pack(1)
#endif

  /**
  * PVR Client Properties
  * Returned on client initialization
  */
  typedef struct PVR_SERVERPROPS {
    bool SupportChannelLogo;            /* Client support transfer of channel logos */
    bool SupportChannelSettings;        /* Client support changing channels on backend */
    bool SupportTimeShift;              /* Client handle Live TV Timeshift, otherwise it is handled by XBMC */
    bool SupportEPG;                    /* Client provide EPG information */
    bool SupportTV;                     /* Client provide TV Channels, is false for Radio only clients */
    bool SupportRadio;                  /* Client provide also Radio Channels */
    bool SupportRecordings;             /* Client support playback of recordings stored on the backend */
    bool SupportTimers;                 /* Client support creation and editing of timers */
    bool SupportTeletext;               /* Client channels can have Teletext data */
    bool SupportDirector;               /* Client provide information about multifeed channels, like Sky Select */
    bool SupportBouquets;               /* Client support Bouqets */
    bool HandleInputStream;             /* Input stream is handled by the client if set, can be false for http */
    bool HandleDemuxing;                /* Demux of stream is handled by the client, as example TVFrontend (htsp protocol) */
  } ATTRIBUTE_PACKED PVR_SERVERPROPS;

  /**
  * EPG Channel Definition
  * Is used by the TransferChannelEntry function to inform XBMC that this
  * channel is present.
  */
  typedef struct PVR_CHANNEL {
    int             uid;                /* Unique identifier for this channel */
    int             number;             /* The backend channel number */

    const char     *name;               /* Channel name provided by the Broadcast */
    const char     *callsign;           /* Channel name provided by the user (if present) */
    const char     *iconpath;           /* Path to the channel icon (if present) */

    bool            encrypted;          /* This is a encrypted channel */
    bool            radio;              /* This is a radio channel */
    bool            hide;               /* This channel is hidden by the user */
    bool            recording;          /* This channel is currently recording */
    bool            teletext;           /* This channel provide Teletext */

    int             bouquet;            /* Bouquet ID this channel have (if supported) */

    bool            multifeed;          /* This is a multifeed channel */
    int             multifeed_master;   /* The Master multifeed channel, multifeed_master==number for master itself */
    int             multifeed_number;   /* The own number inside multifeed channel list */

    /* The Stream URL to access this channel, it can be all types of protocol and types
     * are supported by XBMC or in case the client read the stream use pvr://client_>>ClientID<</channels/>>number<<
     * as URL.
     * Examples:
     * Open a Transport Stream over the Client reading functions where client_"1" is the Client ID and
     * 123 the channel number.
     *   pvr://client_1/channels/123.ts
     * Open a VOB file over http and use XBMC's own filereader.
     *   http://192.168.0.120:3000/PS/C-61441-10008-53621+1.vob
     */
    const char     *stream_url;
  } ATTRIBUTE_PACKED PVR_CHANNEL;

  /**
  * EPG Bouquet Definition
  */
  typedef struct PVR_BOUQUET {
    char*  Name;
    char*  Category;
    int    Number;
  } ATTRIBUTE_PACKED PVR_BOUQUET;

  /**
  * EPG Programme Definition
  * Used to signify an individual broadcast, whether it is also a recording, timer etc.
  */
  typedef struct PVR_PROGINFO {
//    int           bouquet;
//    int           recording;
//    int           rec_status;
//    int           event_flags;

    unsigned int  uid;
    int           channum;
    const char   *title;
    const char   *subtitle;
    const char   *description;
    time_t        starttime;
    time_t        endtime;
    const char   *genre;
    int           genre_type;
    int           genre_sub_type;
  } ATTRIBUTE_PACKED PVR_PROGINFO;

  /**
   * TV Timer Definition
   */
  typedef struct PVR_TIMERINFO {
    int           index;
    int           active;
    const char   *title;
    int           channelNum;
    time_t        starttime;
    time_t        endtime;
    time_t        firstday;
    int           recording;
    int           priority;
    int           lifetime;
    int           repeat;
    int           repeatflags;
  } ATTRIBUTE_PACKED PVR_TIMERINFO;

  /**
   * TV Recording Definition
   */
  typedef struct PVR_RECORDINGINFO {
    int           index;
    const char   *title;
    const char   *subtitle;
    const char   *description;
    const char   *channelName;
    time_t        starttime;
    int           duration;
    double        framesPerSecond;
    int           priority;
    int           lifetime;

  } ATTRIBUTE_PACKED PVR_RECORDINGINFO;

  enum stream_type
  {
    PVR_STREAM_NONE,    // if unknown
    PVR_STREAM_AUDIO,   // audio stream
    PVR_STREAM_VIDEO,   // video stream
    PVR_STREAM_DATA,    // data stream
    PVR_STREAM_SUBTITLE // subtitle stream
  };
  
  enum stream_source {
    PVR_STREAM_SOURCE_NONE          = 0x000,
    PVR_STREAM_SOURCE_DEMUX         = 0x100,
    PVR_STREAM_SOURCE_NAV           = 0x200,
    PVR_STREAM_SOURCE_DEMUX_SUB     = 0x300,
    PVR_STREAM_SOURCE_TEXT          = 0x400
  };

  /**
   * TV Recording Definition
   */
  typedef struct PVR_DEMUXSTREAMINFO {
    /* General Stream information */
    int           index;
    stream_type   type;
    stream_source source;
    CodecID       codec;            // FFMPEG Codec ID
    unsigned int  codec_fourcc;     // if available
    char          language[4];      // ISO 639 3-letter language code (empty string if undefined)
    const char   *name;
    int           duration;         // in mseconds

    /* Audio Stream information, only set for "type==STREAM_AUDIO" */
    int           channels;
    int           sampleRate;
    int           block_align;
    int           bit_rate;
    int           bits_per_sample;
    
    /* Video Stream information, only set for "type==STREAM_VIDEO" */
    int           fps_scale;        // scale of 1000 and a rate of 29970 will result in 29.97 fps
    int           fps_rate;
    int           height;           // height of the stream reported by the demuxer
    int           width;            // width of the stream reported by the demuxer
    float         aspect;           // display aspect of stream
    bool          vfr;              // variable framerate
  } ATTRIBUTE_PACKED PVR_DEMUXSTREAMINFO;

#if PRAGMA_PACK
#pragma pack()
#endif


  /* WARNING: MAKE SURE IF "DemuxPacket" INSIDE "DVDDemux.h" IS CHANGED THIS STRUCT MUST
   *          ALSO CHANGED
   * TODO:    Is there a better way to do this???
   */
  typedef struct demux_packet
  {
    BYTE* pData;   // data
    int iSize;     // data size
    int iStreamId; // integer representing the stream index
    int iGroupId;  // the group this data belongs to, used to group data from different streams together

    double pts; // pts in DVD_TIME_BASE
    double dts; // dts in DVD_TIME_BASE
    double duration; // duration in DVD_TIME_BASE if available
  } demux_packet_t;

  // Structure to transfer the above functions to XBMC
  typedef struct PVRClient
  {
    ADDON_STATUS (__cdecl* Create)(ADDON_HANDLE hdl, int ClientID);

    /** PVR General Functions **/
    PVR_ERROR (__cdecl* GetProperties)(PVR_SERVERPROPS *props);
    const char* (__cdecl* GetBackendName)();
    const char* (__cdecl* GetBackendVersion)();
    const char* (__cdecl* GetConnectionString)();
    PVR_ERROR (__cdecl* GetDriveSpace)(long long *total, long long *used);

    /** PVR EPG Functions **/
    PVR_ERROR (__cdecl* RequestEPGForChannel)(PVRHANDLE handle, unsigned int number, time_t start, time_t end);
  
    /** PVR Bouquets Functions **/
    int (__cdecl* GetNumBouquets)();

    /** PVR Channel Functions **/
    int (__cdecl* GetNumChannels)();
    PVR_ERROR (__cdecl* RequestChannelList)(PVRHANDLE handle, int radio);
//    PVR_ERROR (__cdecl* GetChannelSettings)(cPVRChannelInfoTag *result);
//    PVR_ERROR (__cdecl* UpdateChannelSettings)(const cPVRChannelInfoTag &chaninfo);
//    PVR_ERROR (__cdecl* AddChannel)(const PVR_CHANNEL &info);
//    PVR_ERROR (__cdecl* DeleteChannel)(unsigned int number);
//    PVR_ERROR (__cdecl* RenameChannel)(unsigned int number, CStdString &newname);
//    PVR_ERROR (__cdecl* MoveChannel)(unsigned int number, unsigned int newnumber);

    /** PVR Recording Functions **/
    int (__cdecl* GetNumRecordings)();
    PVR_ERROR (__cdecl* RequestRecordingsList)(PVRHANDLE handle);
    PVR_ERROR (__cdecl* DeleteRecording)(const PVR_RECORDINGINFO &recinfo);
    PVR_ERROR (__cdecl* RenameRecording)(const PVR_RECORDINGINFO &recinfo, const char *newname);

    /** PVR Timer Functions **/
    int (__cdecl* GetNumTimers)();
    PVR_ERROR (__cdecl* RequestTimerList)(PVRHANDLE handle);
    PVR_ERROR (__cdecl* AddTimer)(const PVR_TIMERINFO &timerinfo);
    PVR_ERROR (__cdecl* DeleteTimer)(const PVR_TIMERINFO &timerinfo, bool force);
    PVR_ERROR (__cdecl* RenameTimer)(const PVR_TIMERINFO &timerinfo, const char *newname);
    PVR_ERROR (__cdecl* UpdateTimer)(const PVR_TIMERINFO &timerinfo);

    /** PVR Teletext Functions **/
    bool (__cdecl* TeletextPagePresent)(unsigned int channel, unsigned int Page, unsigned int subPage);
    bool (__cdecl* ReadTeletextPage)(BYTE *buf, unsigned int channel, unsigned int Page, unsigned int subPage);

    /** PVR Live Stream Functions **/
    bool (__cdecl* OpenLiveStream)(unsigned int channel);
    void (__cdecl* CloseLiveStream)();
    int (__cdecl* ReadLiveStream)(BYTE* buf, int buf_size);
    int (__cdecl* GetCurrentClientChannel)();
    bool (__cdecl* SwitchChannel)(unsigned int channel);

    /** PVR Recording Stream Functions **/
    bool (__cdecl* OpenRecordedStream)(const PVR_RECORDINGINFO &recinfo);
    void (__cdecl* CloseRecordedStream)(void);
    int (__cdecl* ReadRecordedStream)(BYTE* buf, int buf_size);
    __int64 (__cdecl* SeekRecordedStream)(__int64 pos, int whence);
    __int64 (__cdecl* LengthRecordedStream)(void);

    /** PVR Demux Stream Functions **/
    bool (__cdecl* OpenTVDemux)(PVRDEMUXHANDLE handle, unsigned int channel);
    bool (__cdecl* OpenRecordingDemux)(PVRDEMUXHANDLE handle, const PVR_RECORDINGINFO &recinfo);
    void (__cdecl* DisposeDemux)();
    void (__cdecl* ResetDemux)();
    void (__cdecl* FlushDemux)();
    void (__cdecl* AbortDemux)();
    void (__cdecl* SetDemuxSpeed)(int iSpeed);
    demux_packet_t* (__cdecl* ReadDemux)();
    bool (__cdecl* SeekDemuxTime)(int time, bool backwords, double* startpts);
    int (__cdecl* GetDemuxStreamLength)();
  
  } PVRClient;

#ifdef __cplusplus
}
#endif

#endif //__PVRCLIENT_TYPES_H__
