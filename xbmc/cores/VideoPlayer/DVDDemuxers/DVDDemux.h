#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <string>
#include <vector>
#include "system.h"

struct DemuxPacket;
class CDVDInputStream;

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable:4244)
#endif

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

extern "C" {
#include "libavcodec/avcodec.h"
}

#ifndef __GNUC__
#pragma warning(pop)
#endif

enum StreamType
{
  STREAM_NONE = 0,// if unknown
  STREAM_AUDIO,   // audio stream
  STREAM_VIDEO,   // video stream
  STREAM_DATA,    // data stream
  STREAM_SUBTITLE,// subtitle stream
  STREAM_TELETEXT, // Teletext data stream
  STREAM_RADIO_RDS // Radio RDS data stream
};

enum StreamSource {
  STREAM_SOURCE_NONE          = 0x000,
  STREAM_SOURCE_DEMUX         = 0x100,
  STREAM_SOURCE_NAV           = 0x200,
  STREAM_SOURCE_DEMUX_SUB     = 0x300,
  STREAM_SOURCE_TEXT          = 0x400,
  STREAM_SOURCE_VIDEOMUX      = 0x500
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
    uniqueId = 0;
    dvdNavId = 0;
    demuxerId = -1;
    codec = (AVCodecID)0; // AV_CODEC_ID_NONE
    codec_fourcc = 0;
    profile = FF_PROFILE_UNKNOWN;
    level = FF_LEVEL_UNKNOWN;
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
    realtime = false;
    bandwidth = 0;
  }

  virtual ~CDemuxStream()
  {
    delete [] ExtraData;
  }

  virtual std::string GetStreamName();

  int uniqueId;          // unique stream id
  int dvdNavId;
  int64_t demuxerId; // id of the associated demuxer
  AVCodecID codec;
  unsigned int codec_fourcc; // if available
  int profile; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
  int level;   // encoder level of the stream reported by the decoder. used to qualify hw decoders.
  StreamType type;
  int source;
  bool realtime;
  unsigned int bandwidth;

  int iDuration; // in mseconds
  void* pPrivate; // private pointer for the demuxer
  uint8_t*     ExtraData; // extra data for codec to use
  unsigned int ExtraSize; // size of extra data

  char language[4]; // ISO 639 3-letter language code (empty string if undefined)
  bool disabled; // set when stream is disabled. (when no decoder exists)

  std::string codecName;

  int  changes; // increment on change which player may need to know about

  enum EFlags
  { FLAG_NONE             = 0x0000 
  , FLAG_DEFAULT          = 0x0001
  , FLAG_DUB              = 0x0002
  , FLAG_ORIGINAL         = 0x0004
  , FLAG_COMMENT          = 0x0008
  , FLAG_LYRICS           = 0x0010
  , FLAG_KARAOKE          = 0x0020
  , FLAG_FORCED           = 0x0040
  , FLAG_HEARING_IMPAIRED = 0x0080
  , FLAG_VISUAL_IMPAIRED  = 0x0100
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
  std::string stereo_mode; // expected stereo mode
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
    iChannelLayout = 0;
    type = STREAM_AUDIO;
  }

  virtual ~CDemuxStreamAudio() {}

  std::string GetStreamType();

  int iChannels;
  int iSampleRate;
  int iBlockAlign;
  int iBitRate;
  int iBitsPerSample;
  uint64_t iChannelLayout;
};

class CDemuxStreamSubtitle : public CDemuxStream
{
public:
  CDemuxStreamSubtitle() : CDemuxStream()
  {
    type = STREAM_SUBTITLE;
  }
};

class CDemuxStreamTeletext : public CDemuxStream
{
public:
  CDemuxStreamTeletext() : CDemuxStream()
  {
    type = STREAM_TELETEXT;
  }
};

class CDemuxStreamRadioRDS : public CDemuxStream
{
public:
  CDemuxStreamRadioRDS() : CDemuxStream()
  {
    type = STREAM_RADIO_RDS;
  }
};

class CDVDDemux
{
public:

  CDVDDemux() : m_demuxerId(NewGuid()) {}
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
   * Get the name of a chapter
   * \param strChapterName[out] Name of chapter
   * \param chapterIdx -1 for current chapter, else a chapter index
   */
  virtual void GetChapterName(std::string& strChapterName, int chapterIdx=-1) {}

  /*
   * Get the position of a chapter
   * \param chapterIdx -1 for current chapter, else a chapter index
   */
  virtual int64_t GetChapterPos(int chapterIdx=-1) { return 0; }

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
   * returns the stream or NULL on error
   */
  virtual CDemuxStream* GetStream(int64_t demuxerId, int iStreamId) const { return GetStream(iStreamId); };

  virtual std::vector<CDemuxStream*> GetStreams() const = 0;

  /*
   * return nr of streams, 0 if none
   */
  virtual int GetNrOfStreams() const = 0;

  /*
   * returns opened filename
   */
  virtual std::string GetFileName() = 0;

  /*
   * return nr of subtitle streams, 0 if none
   */
  int GetNrOfSubtitleStreams();

  /*
   * return a user-presentable codec name of the given stream
   */
  virtual std::string GetStreamCodecName(int64_t demuxerId, int iStreamId) { return GetStreamCodecName(iStreamId); };

  /*
   * enable / disable demux stream
   */
  virtual void EnableStream(int64_t demuxerId, int id, bool enable) { EnableStream(id, enable); };

  /*
   * sets desired width / height for video stream
   * adaptive demuxers like DASH can use this to choose best fitting video stream
   */
  virtual void SetVideoResolution(int width, int height) {};
  
  /*
  * return the id of the demuxer
  */
  int64_t GetDemuxerId() { return m_demuxerId; };

protected:
  virtual void EnableStream(int id, bool enable) {};
  virtual CDemuxStream* GetStream(int iStreamId) const = 0;
  virtual std::string GetStreamCodecName(int iStreamId) { return ""; };

  int GetNrOfStreams(StreamType streamType);

  int64_t m_demuxerId;

private:

  int64_t NewGuid()
  {
    static int64_t guid = 0;
    return guid++;
  }
};
