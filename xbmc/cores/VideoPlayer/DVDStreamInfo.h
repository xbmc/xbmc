/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDDemuxers/DVDDemux.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/dovi_meta.h>
}

#define CODEC_FORCE_SOFTWARE 0x01
#define CODEC_ALLOW_FALLBACK 0x02

class CDemuxStream;
struct DemuxCryptoSession;

class CDVDStreamInfo
{
public:
  CDVDStreamInfo();
  CDVDStreamInfo(const CDVDStreamInfo &right, bool withextradata = true);
  CDVDStreamInfo(const CDemuxStream &right, bool withextradata = true);

  ~CDVDStreamInfo();

  void Clear(); // clears current information
  bool Equal(const CDVDStreamInfo& right, int compare);
  bool Equal(const CDemuxStream &right, bool withextradata);

  void Assign(const CDVDStreamInfo &right, bool withextradata);
  void Assign(const CDemuxStream &right, bool withextradata);

  enum
  {
    COMPARE_EXTRADATA = 1,
    COMPARE_ID = 2,
    COMPARE_ALL = 3,
  };

  AVCodecID codec;
  StreamType type;
  int uniqueId;
  int demuxerId = -1;
  int source{STREAM_SOURCE_NONE};
  int flags;
  std::string filename;
  bool dvd;
  int codecOptions;

  // VIDEO
  int fpsscale; // scale of 1001 and a rate of 60000 will result in 59.94 fps
  int fpsrate;
  bool interlaced;
  int height; // height of the stream reported by the demuxer
  int width; // width of the stream reported by the demuxer
  double aspect; // display aspect as reported by demuxer
  bool vfr; // variable framerate
  bool stills; // there may be odd still frames in video
  int level; // encoder level of the stream reported by the decoder. used to qualify hw decoders.
  int profile; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
  bool ptsinvalid;  // pts cannot be trusted (avi's).
  bool forced_aspect; // aspect is forced from container
  int orientation; // orientation of the video in degrees counter clockwise
  int bitsperpixel;
  int bitdepth;
  StreamHdrType hdrType;
  AVColorSpace colorSpace;
  AVColorRange colorRange;
  AVColorPrimaries colorPrimaries;
  AVColorTransferCharacteristic colorTransferCharacteristic;
  std::shared_ptr<AVMasteringDisplayMetadata> masteringMetadata;
  std::shared_ptr<AVContentLightMetadata> contentLightMetadata;
  std::string stereo_mode; // stereoscopic 3d mode
  AVDOVIDecoderConfigurationRecord dovi{};

  // AUDIO
  int channels;
  int samplerate;
  int bitrate;
  int blockalign;
  int bitspersample;
  uint64_t channellayout;

  // SUBTITLE

  // CODEC EXTRADATA
  FFmpegExtraData extradata; // extra data for codec to use
  unsigned int codec_tag; // extra identifier hints for decoding

  // Crypto initialization Data
  std::shared_ptr<DemuxCryptoSession> cryptoSession;
  std::shared_ptr<ADDON::IAddonProvider> externalInterfaces;

  bool operator==(const CDVDStreamInfo& right) { return Equal(right, COMPARE_ALL); }
  bool operator!=(const CDVDStreamInfo& right) { return !Equal(right, COMPARE_ALL); }

  CDVDStreamInfo& operator=(const CDVDStreamInfo& right)
  {
    if (this != &right)
      Assign(right, true);

    return *this;
  }

  bool operator==(const CDemuxStream& right)
  {
    return Equal(CDVDStreamInfo(right, true), COMPARE_ALL);
  }
  bool operator!=(const CDemuxStream& right)
  {
    return !Equal(CDVDStreamInfo(right, true), COMPARE_ALL);
  }

  CDVDStreamInfo& operator=(const CDemuxStream& right)
  {
    Assign(right, true);
    return *this;
  }
};
