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

#include "utils/StdString.h"
#include "system.h"
#include "DVDDemuxPacket.h"

class CDVDInputStream;

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable:4244)
#endif

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#ifndef _LINUX
enum CodecID;
#include <libavcodec/avcodec.h>
#else
extern "C" {
#if (defined USE_EXTERNAL_FFMPEG)
  #if (defined HAVE_LIBAVCODEC_AVCODEC_H)
    #include <libavcodec/avcodec.h>
  #elif (defined HAVE_FFMPEG_AVCODEC_H)
    #include <ffmpeg/avcodec.h>
  #endif
#else
  #include "libavcodec/avcodec.h"
#endif
}
#endif

#ifndef __GNUC__
#pragma warning(pop)
#endif

enum AVDiscard;

enum StreamType
{
  STREAM_NONE,    // if unknown
  STREAM_AUDIO,   // audio stream
  STREAM_VIDEO,   // video stream
  STREAM_DATA,    // data stream
  STREAM_SUBTITLE,// subtitle stream
  STREAM_TELETEXT // Teletext data stream
};

enum StreamSource {
  STREAM_SOURCE_NONE          = 0x000,
  STREAM_SOURCE_DEMUX         = 0x100,
  STREAM_SOURCE_NAV           = 0x200,
  STREAM_SOURCE_DEMUX_SUB     = 0x300,
  STREAM_SOURCE_TEXT          = 0x400
};

#define STREAM_SOURCE_MASK(a) ((a) & 0xf00)

/*
 * CDemuxStream
 * Base class for all demuxer streams
 */
class CDemuxStream
{
public:
  CDemuxStream()
  {
    iId = 0;
    iPhysicalId = 0;
    codec = (CodecID)0; // CODEC_ID_NONE
    codec_fourcc = 0;
    profile = FF_PROFILE_UNKNOWN;
    level = 0;
    type = STREAM_NONE;
    source = STREAM_SOURCE_NONE;
    iDuration = 0;
    pPrivate = NULL;
    ExtraData = NULL;
    ExtraSize = 0;
    memset(language, 0, sizeof(language));
    disabled = false;
    changes = 0;
    flags = FLAG_NONE;
  }

  virtual ~CDemuxStream() {}

  virtual void GetStreamInfo(std::string& strInfo)
  {
    strInfo = "";
  }

  virtual void GetStreamName(std::string& strInfo);

  virtual void      SetDiscard(AVDiscard discard);
  virtual AVDiscard GetDiscard();

  int iId;         // most of the time starting from 0
  int iPhysicalId; // id
  CodecID codec;
  unsigned int codec_fourcc; // if available
  int profile; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
  int level;   // encoder level of the stream reported by the decoder. used to qualify hw decoders.
  StreamType type;
  int source;

  int iDuration; // in mseconds
  void* pPrivate; // private pointer for the demuxer
  void* ExtraData; // extra data for codec to use
  unsigned int ExtraSize; // size of extra data

  char language[4]; // ISO 639 3-letter language code (empty string if undefined)
  bool disabled; // set when stream is disabled. (when no decoder exists)

  int  changes; // increment on change which player may need to know about

  enum EFlags
  { FLAG_NONE     = 0x0000 
  , FLAG_DEFAULT  = 0x0001
  , FLAG_DUB      = 0x0002
  , FLAG_ORIGINAL = 0x0004
  , FLAG_COMMENT  = 0x0008
  , FLAG_LYRICS   = 0x0010
  , FLAG_KARAOKE  = 0x0020
  , FLAG_FORCED   = 0x0040
  } flags;
};

class CDemuxStreamVideo : public CDemuxStream
{
public:
  CDemuxStreamVideo() : CDemuxStream()
  {
    iFpsScale = 0;
    iFpsRate = 0;
    iHeight = 0;
    iWidth = 0;
    fAspect = 0.0;
    bVFR = false;
    bPTSInvalid = false;
    bForcedAspect = false;
    type = STREAM_VIDEO;
    iOrientation = 0;
    iBitsPerPixel = 0;
  }

  virtual ~CDemuxStreamVideo() {}
  int iFpsScale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int iFpsRate;
  int iHeight; // height of the stream reported by the demuxer
  int iWidth; // width of the stream reported by the demuxer
  float fAspect; // display aspect of stream
  bool bVFR;  // variable framerate
  bool bPTSInvalid; // pts cannot be trusted (avi's).
  bool bForcedAspect; // aspect is forced from container
  int iOrientation; // orientation of the video in degress counter clockwise
  int iBitsPerPixel;
};

class CDemuxStreamAudio : public CDemuxStream
{
public:
  CDemuxStreamAudio() : CDemuxStream()
  {
    iChannels = 0;
    iSampleRate = 0;
    iBlockAlign = 0;
    iBitRate = 0;
    iBitsPerSample = 0;
    type = STREAM_AUDIO;
  }

  virtual ~CDemuxStreamAudio() {}

  void GetStreamType(std::string& strInfo);

  int iChannels;
  int iSampleRate;
  int iBlockAlign;
  int iBitRate;
  int iBitsPerSample;
};

class CDemuxStreamSubtitle : public CDemuxStream
{
public:
  CDemuxStreamSubtitle() : CDemuxStream()
  {
    identifier = 0;
    type = STREAM_SUBTITLE;
  }

  int identifier;
};

class CDemuxStreamTeletext : public CDemuxStream
{
public:
  CDemuxStreamTeletext() : CDemuxStream()
  {
    type = STREAM_TELETEXT;
  }
  virtual void GetStreamInfo(std::string& strInfo);
};

class CDVDDemux
{
public:

  CDVDDemux() {}
  virtual ~CDVDDemux() {}


  /*
   * Reset the entire demuxer (same result as closing and opening it)
   */
  virtual void Reset() = 0;

  /*
   * Aborts any internal reading that might be stalling main thread
   * NOTICE - this can be called from another thread
   */
  virtual void Abort() = 0;

  /*
   * Flush the demuxer, if any data is kept in buffers, this should be freed now
   */
  virtual void Flush() = 0;

  /*
   * Read a packet, returns NULL on error
   *
   */
  virtual DemuxPacket* Read() = 0;

  /*
   * Seek, time in msec calculated from stream start
   */
  virtual bool SeekTime(int time, bool backwords = false, double* startpts = NULL) = 0;

  /*
   * Seek to a specified chapter.
   * startpts can be updated to the point where display should start
   */
  virtual bool SeekChapter(int chapter, double* startpts = NULL) { return false; }

  /*
   * Get the number of chapters available
   */
  virtual int GetChapterCount() { return 0; }

  /*
   * Get current chapter
   */
  virtual int GetChapter() { return 0; }

  /*
   * Get the name of the current chapter
   */
  virtual void GetChapterName(std::string& strChapterName) {}

  /*
   * Set the playspeed, if demuxer can handle different
   * speeds of playback
   */
  virtual void SetSpeed(int iSpeed) = 0;

  /*
   * returns the total time in msec
   */
  virtual int GetStreamLength() = 0;

  /*
   * returns the stream or NULL on error, starting from 0
   */
  virtual CDemuxStream* GetStream(int iStreamId) = 0;

  /*
   * return nr of streams, 0 if none
   */
  virtual int GetNrOfStreams() = 0;

  /*
   * returns opened filename
   */
  virtual std::string GetFileName() = 0;
  /*
   * return nr of audio streams, 0 if none
   */
  int GetNrOfAudioStreams();

  /*
   * return nr of video streams, 0 if none
   */
  int GetNrOfVideoStreams();

  /*
   * return nr of subtitle streams, 0 if none
   */
  int GetNrOfSubtitleStreams();

  /*
   * return nr of teletext streams, 0 if none
   */
  int GetNrOfTeletextStreams();

  /*
   * return the audio stream, or NULL if it does not exist
   */
  CDemuxStreamAudio* GetStreamFromAudioId(int iAudioIndex);

  /*
   * return the video stream, or NULL if it does not exist
   */
  CDemuxStreamVideo* GetStreamFromVideoId(int iVideoIndex);

  /*
   * return the subtitle stream, or NULL if it does not exist
   */
  CDemuxStreamSubtitle* GetStreamFromSubtitleId(int iSubtitleIndex);

  /*
   * return the teletext stream, or NULL if it does not exist
   */
  CDemuxStreamTeletext* GetStreamFromTeletextId(int iTeletextIndex);

  /*
   * return a user-presentable codec name of the given stream
   */
  virtual void GetStreamCodecName(int iStreamId, CStdString &strName) {};
};
