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
#include "DVDClock.h"

#define CODEC_FORCE_SOFTWARE 0x01
#define CODEC_ALLOW_FALLBACK 0x02
#define CODEC_INTERLACED     0x40
#define CODEC_UNKNOWN_I_P    0x80

class CDemuxStream;
struct DemuxCryptoSession;

enum DOVIELType : int
{
  TYPE_NONE = 0,
  TYPE_FEL,
  TYPE_MEL
};

struct DOVIFrameMetadata
{
  double pts;
  uint16_t level1_min_pq = 0;
  uint16_t level1_max_pq = 0;
  uint16_t level1_avg_pq = 0;

  bool has_level5_metadata = false;
  uint16_t level5_active_area_left_offset = 0;
  uint16_t level5_active_area_right_offset = 0;
  uint16_t level5_active_area_top_offset = 0;
  uint16_t level5_active_area_bottom_offset = 0;
};

struct DOVIStreamMetadata
{
  uint16_t source_min_pq = 0;
  uint16_t source_max_pq = 0;

  bool has_level6_metadata = false;
  uint16_t level6_max_lum = 0;
  uint16_t level6_min_lum = 0;
  uint16_t level6_max_cll = 0;
  uint16_t level6_max_fall = 0;

  std::string meta_version = "";
};

struct DOVIStreamInfo
{
  DOVIELType dovi_el_type = DOVIELType::TYPE_NONE;
  bool has_config = false;
  bool has_header = false;
  AVDOVIDecoderConfigurationRecord dovi = {};
};

struct HDRStaticMetadataInfo
{
  bool has_mdcv_metadata = false;
  uint32_t max_lum = 0;
  uint32_t min_lum = 0;
  std::string colour_primaries = "";

  bool has_cll_metadata = false;
  uint16_t max_cll = 0;
  uint16_t max_fall = 0;
};

class CDVDStreamInfo
{
public:
  CDVDStreamInfo();
  CDVDStreamInfo(const CDVDStreamInfo &right, bool withextradata = true);
  CDVDStreamInfo(const CDemuxStream &right, bool withextradata = true);

  ~CDVDStreamInfo();

  void Clear(); // clears current information
  bool Equal(const CDVDStreamInfo& right, int compare) const;
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
  DOVIELType dovi_el_type = DOVIELType::TYPE_NONE;
  CDVDClock *pClock;

  static constexpr AVDOVIDecoderConfigurationRecord empty_dovi{}; // For comparison

  // AUDIO
  int channels;
  int samplerate;
  int bitrate;
  int blockalign;
  int bitspersample;
  uint64_t channellayout;

  // SUBTITLE
  int m_3dSubtitlePlane;

  // CODEC EXTRADATA
  FFmpegExtraData extradata; // extra data for codec to use
  unsigned int codec_tag; // extra identifier hints for decoding

  // Crypto initialization Data
  std::shared_ptr<DemuxCryptoSession> cryptoSession;
  std::shared_ptr<ADDON::IAddonProvider> externalInterfaces;

  bool operator==(const CDVDStreamInfo& right) const { return Equal(right, COMPARE_ALL); }
  bool operator!=(const CDVDStreamInfo& right) const { return !Equal(right, COMPARE_ALL); }

  CDVDStreamInfo& operator=(const CDVDStreamInfo& right)
  {
    if (this != &right)
      Assign(right, true);

    return *this;
  }

  bool operator==(const CDemuxStream& right) const {
    return Equal(CDVDStreamInfo(right, true), COMPARE_ALL);
  }
  bool operator!=(const CDemuxStream& right) const {
    return !Equal(CDVDStreamInfo(right, true), COMPARE_ALL);
  }

  CDVDStreamInfo& operator=(const CDemuxStream& right)
  {
    Assign(right, true);
    return *this;
  }
};
