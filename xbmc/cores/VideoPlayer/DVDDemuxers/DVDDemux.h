/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Interface/StreamInfo.h"
#include "cores/FFmpeg.h"

#include <memory>
#include <string>
#include <vector>

#include <fmt/format.h>

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/defs.h>
#include <libavutil/dovi_meta.h>
#include <libavutil/mastering_display_metadata.h>
}

#ifndef __GNUC__
#pragma warning(pop)
#endif

struct DemuxPacket;
struct DemuxCryptoSession;

namespace ADDON
{
class IAddonProvider;
}

enum class StreamType
{
  NONE = 0, // if unknown
  AUDIO, // audio stream
  VIDEO, // video stream
  DATA, // data stream
  SUBTITLE, // subtitle stream
  TELETEXT, // Teletext data stream
  RADIO_RDS, // Radio RDS data stream
  AUDIO_ID3 // Audio ID3 data stream
};

template<>
struct fmt::formatter<StreamType> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(const StreamType& type, FormatContext& ctx) const
  {
    return fmt::formatter<std::string_view>::format(enumToStreamType(type), ctx);
  }

private:
  static constexpr std::string_view enumToStreamType(StreamType type)
  {
    using namespace std::literals::string_view_literals;
    switch (type)
    {
      using enum StreamType;
      case NONE:
        return "none"sv;
      case AUDIO:
        return "audio"sv;
      case VIDEO:
        return "video"sv;
      case DATA:
        return "data"sv;
      case SUBTITLE:
        return "subtitle"sv;
      case TELETEXT:
        return "teletext"sv;
      case RADIO_RDS:
        return "radio rds"sv;
      case AUDIO_ID3:
        return "audio id3"sv;
    }
    throw std::invalid_argument("no streamtype string found");
  }
};

enum StreamSource
{
  STREAM_SOURCE_NONE = 0x000,
  STREAM_SOURCE_DEMUX = 0x100,
  STREAM_SOURCE_NAV = 0x200,
  STREAM_SOURCE_DEMUX_SUB = 0x300,
  STREAM_SOURCE_TEXT = 0x400,
  STREAM_SOURCE_VIDEOMUX = 0x500
};

#define STREAM_SOURCE_MASK(a) ((a)&0xf00)

/*
 * CDemuxStream
 * Base class for all demuxer streams
 */
class CDemuxStream
{
public:
  CDemuxStream() = default;
  explicit CDemuxStream(StreamType t) : type(t) {}
  virtual ~CDemuxStream() = default;
  CDemuxStream(CDemuxStream&&) = default;

  virtual std::string GetStreamName();

  int uniqueId{0}; // unique stream id
  int dvdNavId{0};
  int64_t demuxerId{-1}; // id of the associated demuxer
  AVCodecID codec{AV_CODEC_ID_NONE};
  unsigned int codec_fourcc{0}; // if available
  int profile{
      AV_PROFILE_UNKNOWN}; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
  int level{
      AV_LEVEL_UNKNOWN}; // encoder level of the stream reported by the decoder. used to qualify hw decoders.
  StreamType type{StreamType::NONE};
  int source{STREAM_SOURCE_NONE};

  int iDuration{0}; // in mseconds
  void* pPrivate{nullptr}; // private pointer for the demuxer
  FFmpegExtraData extraData;

  StreamFlags flags{StreamFlags::FLAG_NONE};
  std::string language; // RFC 5646 language code (empty string if undefined)
  bool disabled{false}; // set when stream is disabled. (when no decoder exists)

  std::string name;
  std::string codecName;

  int changes{0}; // increment on change which player may need to know about

  std::shared_ptr<DemuxCryptoSession> cryptoSession;
  std::shared_ptr<ADDON::IAddonProvider> externalInterfaces;
};

class CDemuxStreamVideo : public CDemuxStream
{
public:
  CDemuxStreamVideo() : CDemuxStream(StreamType::VIDEO) {}

  ~CDemuxStreamVideo() override = default;
  int iFpsScale = 0; // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int iFpsRate = 0;
  bool interlaced = false; // unknown or progressive => false, otherwise true.
  int iHeight = 0; // height of the stream reported by the demuxer
  int iWidth = 0; // width of the stream reported by the demuxer
  double fAspect = 0; // display aspect of stream
  bool bVFR = false; // variable framerate
  bool bPTSInvalid = false; // pts cannot be trusted (avi's).
  bool bForcedAspect = false; // aspect is forced from container
  int iOrientation = 0; // orientation of the video in degrees counter clockwise
  int iBitsPerPixel = 0;
  int iBitRate = 0;
  int bitDepth = 0;

  AVColorSpace colorSpace = AVCOL_SPC_UNSPECIFIED;
  AVColorRange colorRange = AVCOL_RANGE_UNSPECIFIED;
  AVColorPrimaries colorPrimaries = AVCOL_PRI_UNSPECIFIED;
  AVColorTransferCharacteristic colorTransferCharacteristic = AVCOL_TRC_UNSPECIFIED;

  std::shared_ptr<AVMasteringDisplayMetadata> masteringMetaData;
  std::shared_ptr<AVContentLightMetadata> contentLightMetaData;

  std::string stereo_mode; // expected stereo mode
  StreamHdrType hdr_type = StreamHdrType::HDR_TYPE_NONE; // type of HDR for this stream (hdr10, etc)
  AVDOVIDecoderConfigurationRecord dovi{};
};

class CDemuxStreamAudio : public CDemuxStream
{
public:
  CDemuxStreamAudio() : CDemuxStream(StreamType::AUDIO) {}

  ~CDemuxStreamAudio() override = default;

  std::string GetStreamType() const;

  int iChannels{0};
  int iSampleRate{0};
  int iBlockAlign{0};
  int iBitRate{0};
  int iBitsPerSample{0};
  uint64_t iChannelLayout{0};
  std::string m_channelLayoutName;
};

class CDemuxStreamSubtitle : public CDemuxStream
{
public:
  CDemuxStreamSubtitle() : CDemuxStream(StreamType::SUBTITLE) {}
};

class CDemuxStreamTeletext : public CDemuxStream
{
public:
  CDemuxStreamTeletext() : CDemuxStream(StreamType::TELETEXT) {}
};

class CDemuxStreamAudioID3 : public CDemuxStream
{
public:
  CDemuxStreamAudioID3() : CDemuxStream(StreamType::AUDIO_ID3) {}
};

class CDemuxStreamRadioRDS : public CDemuxStream
{
public:
  CDemuxStreamRadioRDS() : CDemuxStream(StreamType::RADIO_RDS) {}
};

class CDVDDemux
{
public:
  CDVDDemux()
    : m_demuxerId(NewGuid())
  {
  }
  virtual ~CDVDDemux() = default;


  /*
   * Reset the entire demuxer (same result as closing and opening it)
   */
  virtual bool Reset() = 0;

  /*
   * Aborts any internal reading that might be stalling main thread
   * NOTICE - this can be called from another thread
   */
  virtual void Abort() {}

  /*
   * Flush the demuxer, if any data is kept in buffers, this should be freed now
   */
  virtual void Flush() = 0;

  /*
   * Read a packet, returns nullptr on error
   *
   */
  virtual DemuxPacket* Read() = 0;

  /*
   * Seek, time in msec calculated from stream start
   */
  virtual bool SeekTime(double time, bool backwards = false, double* startpts = nullptr) = 0;

  /*
   * Seek to a specified chapter.
   * startpts can be updated to the point where display should start
   */
  virtual bool SeekChapter(int chapter, double* startpts = nullptr) { return false; }

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
  virtual void GetChapterName(std::string& strChapterName, int chapterIdx = -1) {}

  /*
   * Get the position of a chapter
   * \param chapterIdx -1 for current chapter, else a chapter index
   */
  virtual int64_t GetChapterPos(int chapterIdx = -1) { return 0; }

  /*
   * Set the playspeed, if demuxer can handle different
   * speeds of playback
   */
  virtual void SetSpeed(int iSpeed) {}

  /*
   * Let demuxer know if we want to fill demux queue
   */
  virtual void FillBuffer(bool mode) {}

  /*
   * returns the total time in msec
   */
  virtual int GetStreamLength() { return 0; }

  /*
   * returns the stream or nullptr on error
   */
  virtual CDemuxStream* GetStream(int64_t demuxerId, int iStreamId) const
  {
    return GetStream(iStreamId);
  };

  virtual std::vector<CDemuxStream*> GetStreams() const = 0;

  /*
   * return nr of streams, 0 if none
   */
  virtual int GetNrOfStreams() const = 0;

  /*
   * get a list of available programs
   */
  virtual int GetPrograms(std::vector<ProgramInfo>& programs) { return 0; }

  /*
   * select programs
   */
  virtual void SetProgram(int progId) {}

  /*
   * returns opened filename
   */
  virtual std::string GetFileName() { return ""; }

  /*
   * return nr of subtitle streams, 0 if none
   */
  int GetNrOfSubtitleStreams() const;

  /*
   * return a user-presentable codec name of the given stream
   */
  virtual std::string GetStreamCodecName(int64_t demuxerId, int iStreamId)
  {
    return GetStreamCodecName(iStreamId);
  };

  /*
   * enable / disable demux stream
   */
  virtual void EnableStream(int64_t demuxerId, int id, bool enable) { EnableStream(id, enable); }

  /*
  * implicitly enable and open a demux stream for playback
  */
  virtual void OpenStream(int64_t demuxerId, int id) { OpenStream(id); }

  /*
   * sets desired width / height for video stream
   * adaptive demuxers like DASH can use this to choose best fitting video stream
   */
  virtual void SetVideoResolution(unsigned int width, unsigned int height) {}

  /*
  * return the id of the demuxer
  */
  int64_t GetDemuxerId() const { return m_demuxerId; }

protected:
  virtual void EnableStream(int id, bool enable) {}
  virtual void OpenStream(int id) {}
  virtual CDemuxStream* GetStream(int iStreamId) const = 0;
  virtual std::string GetStreamCodecName(int iStreamId) { return ""; }

  int GetNrOfStreams(StreamType streamType) const;

private:
  static int64_t NewGuid()
  {
    static int64_t guid = 0;
    return guid++;
  }

  int64_t m_demuxerId{0};
};
