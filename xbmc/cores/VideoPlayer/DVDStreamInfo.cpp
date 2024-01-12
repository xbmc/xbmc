/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDStreamInfo.h"

#include "DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/Interface/DemuxCrypto.h"

#include <cstring>

CDVDStreamInfo::CDVDStreamInfo() : extradata{}
{
  Clear();
}
CDVDStreamInfo::CDVDStreamInfo(const CDVDStreamInfo& right, bool withextradata) : extradata{}
{
  Clear();
  Assign(right, withextradata);
}
CDVDStreamInfo::CDVDStreamInfo(const CDemuxStream& right, bool withextradata) : extradata{}
{
  Clear();
  Assign(right, withextradata);
}

CDVDStreamInfo::~CDVDStreamInfo() = default;

void CDVDStreamInfo::Clear()
{
  codec = AV_CODEC_ID_NONE;
  type = STREAM_NONE;
  uniqueId = -1;
  source = STREAM_SOURCE_NONE;
  codecOptions = 0;
  codec_tag  = 0;
  flags = 0;
  filename.clear();
  dvd = false;

  extradata = {};

  cryptoSession = nullptr;
  externalInterfaces = nullptr;

  fpsscale = 0;
  fpsrate  = 0;
  interlaced = false;
  height   = 0;
  width    = 0;
  aspect   = 0.0;
  vfr      = false;
  stills   = false;
  level    = 0;
  profile  = 0;
  ptsinvalid = false;
  forced_aspect = false;
  bitsperpixel = 0;
  hdrType = StreamHdrType::HDR_TYPE_NONE;
  colorSpace = AVCOL_SPC_UNSPECIFIED;
  colorRange = AVCOL_RANGE_UNSPECIFIED;
  colorPrimaries = AVCOL_PRI_UNSPECIFIED;
  colorTransferCharacteristic = AVCOL_TRC_UNSPECIFIED;
  masteringMetadata = nullptr;
  contentLightMetadata = nullptr;
  stereo_mode.clear();
  dovi = {};

  channels   = 0;
  samplerate = 0;
  blockalign = 0;
  bitrate    = 0;
  bitspersample = 0;
  channellayout = 0;

  orientation = 0;
  bitdepth = 0;
}

bool CDVDStreamInfo::Equal(const CDVDStreamInfo& right, int compare)
{
  if (codec != right.codec || type != right.type ||
      ((compare & COMPARE_ID) && uniqueId != right.uniqueId) ||
      ((compare & COMPARE_ID) && demuxerId != right.demuxerId) || codec_tag != right.codec_tag ||
      flags != right.flags)
    return false;

  if (compare & COMPARE_EXTRADATA && extradata != right.extradata)
  {
    return false;
  }

  // VIDEO
  // clang-format off
  if (fpsscale != right.fpsscale
  || fpsrate != right.fpsrate
  || interlaced != right.interlaced
  || height != right.height
  || width != right.width
  || stills != right.stills
  || level != right.level
  || profile != right.profile
  || ptsinvalid != right.ptsinvalid
  || forced_aspect != right.forced_aspect
  || bitsperpixel != right.bitsperpixel
  || bitdepth != right.bitdepth
  || vfr != right.vfr
  || hdrType != right.hdrType
  || colorSpace != right.colorSpace
  || colorRange != right.colorRange
  || colorPrimaries != right.colorPrimaries
  || colorTransferCharacteristic != right.colorTransferCharacteristic
  || stereo_mode != right.stereo_mode)
    return false;
  // clang-format on

  if (masteringMetadata && right.masteringMetadata)
  {
    if (masteringMetadata->has_luminance != right.masteringMetadata->has_luminance
      || masteringMetadata->has_primaries != right.masteringMetadata->has_primaries)
      return false;

    if (masteringMetadata->has_primaries)
    {
      for (unsigned int i(0); i < 3; ++i)
        for (unsigned int j(0); j < 2; ++j)
          if (av_cmp_q(masteringMetadata->display_primaries[i][j], right.masteringMetadata->display_primaries[i][j]))
            return false;
      for (unsigned int i(0); i < 2; ++i)
        if (av_cmp_q(masteringMetadata->white_point[i], right.masteringMetadata->white_point[i]))
          return false;
    }

    if (masteringMetadata->has_luminance)
    {
      if (av_cmp_q(masteringMetadata->min_luminance, right.masteringMetadata->min_luminance)
      || av_cmp_q(masteringMetadata->max_luminance, right.masteringMetadata->max_luminance))
        return false;
    }
  }
  else if (masteringMetadata || right.masteringMetadata)
    return false;

  if (contentLightMetadata && right.contentLightMetadata)
  {
    if (contentLightMetadata->MaxCLL != right.contentLightMetadata->MaxCLL
    || contentLightMetadata->MaxFALL != right.contentLightMetadata->MaxFALL)
      return false;
  }
  else if (contentLightMetadata || right.contentLightMetadata)
    return false;

  if (0 != std::memcmp(&dovi, &right.dovi, sizeof(AVDOVIDecoderConfigurationRecord)))
    return false;

  // AUDIO
  if( channels      != right.channels
  ||  samplerate    != right.samplerate
  ||  blockalign    != right.blockalign
  ||  bitrate       != right.bitrate
  ||  bitspersample != right.bitspersample
  ||  channellayout != right.channellayout)
    return false;

  // SUBTITLE

  // Crypto
  if ((cryptoSession == nullptr) != (right.cryptoSession == nullptr))
    return false;

  if (cryptoSession && !(*cryptoSession == *right.cryptoSession))
    return false;

  return true;
}

bool CDVDStreamInfo::Equal(const CDemuxStream& right, bool withextradata)
{
  CDVDStreamInfo info;
  info.Assign(right, withextradata);
  return Equal(info, withextradata ? COMPARE_ALL : COMPARE_ALL & ~COMPARE_EXTRADATA);
}


// ASSIGNMENT
void CDVDStreamInfo::Assign(const CDVDStreamInfo& right, bool withextradata)
{
  codec = right.codec;
  type = right.type;
  uniqueId = right.uniqueId;
  demuxerId = right.demuxerId;
  source = right.source;
  codec_tag = right.codec_tag;
  flags = right.flags;
  filename = right.filename;
  dvd = right.dvd;

  if (withextradata && right.extradata)
  {
    extradata = right.extradata;
  }
  else
  {
    extradata = {};
  }

  cryptoSession = right.cryptoSession;
  externalInterfaces = right.externalInterfaces;

  // VIDEO
  fpsscale = right.fpsscale;
  fpsrate  = right.fpsrate;
  interlaced = right.interlaced;
  height   = right.height;
  width    = right.width;
  aspect   = right.aspect;
  stills   = right.stills;
  level    = right.level;
  profile  = right.profile;
  ptsinvalid = right.ptsinvalid;
  forced_aspect = right.forced_aspect;
  orientation = right.orientation;
  bitsperpixel = right.bitsperpixel;
  bitdepth = right.bitdepth;
  vfr = right.vfr;
  codecOptions = right.codecOptions;
  hdrType = right.hdrType;
  colorSpace = right.colorSpace;
  colorRange = right.colorRange;
  colorPrimaries = right.colorPrimaries;
  colorTransferCharacteristic = right.colorTransferCharacteristic;
  masteringMetadata = right.masteringMetadata;
  contentLightMetadata = right.contentLightMetadata;
  stereo_mode = right.stereo_mode;
  dovi = right.dovi;

  // AUDIO
  channels      = right.channels;
  samplerate    = right.samplerate;
  blockalign    = right.blockalign;
  bitrate       = right.bitrate;
  bitspersample = right.bitspersample;
  channellayout = right.channellayout;

  // SUBTITLE
}

void CDVDStreamInfo::Assign(const CDemuxStream& right, bool withextradata)
{
  Clear();

  codec = right.codec;
  type = right.type;
  uniqueId = right.uniqueId;
  demuxerId = right.demuxerId;
  source = right.source;
  codec_tag = right.codec_fourcc;
  profile = right.profile;
  level = right.level;
  flags = right.flags;

  if (withextradata && right.extraData)
  {
    extradata = right.extraData;
  }

  cryptoSession = right.cryptoSession;
  externalInterfaces = right.externalInterfaces;

  if (right.type == STREAM_AUDIO)
  {
    const CDemuxStreamAudio *stream = static_cast<const CDemuxStreamAudio*>(&right);
    channels      = stream->iChannels;
    samplerate    = stream->iSampleRate;
    blockalign    = stream->iBlockAlign;
    bitrate       = stream->iBitRate;
    bitspersample = stream->iBitsPerSample;
    channellayout = stream->iChannelLayout;
  }
  else if (right.type == STREAM_VIDEO)
  {
    const CDemuxStreamVideo *stream = static_cast<const CDemuxStreamVideo*>(&right);
    fpsscale  = stream->iFpsScale;
    fpsrate   = stream->iFpsRate;
    interlaced = stream->interlaced;
    height    = stream->iHeight;
    width     = stream->iWidth;
    aspect    = stream->fAspect;
    vfr       = stream->bVFR;
    ptsinvalid = stream->bPTSInvalid;
    forced_aspect = stream->bForcedAspect;
    orientation = stream->iOrientation;
    bitsperpixel = stream->iBitsPerPixel;
    bitdepth = stream->bitDepth;
    hdrType = stream->hdr_type;
    colorSpace = stream->colorSpace;
    colorRange = stream->colorRange;
    colorPrimaries = stream->colorPrimaries;
    colorTransferCharacteristic = stream->colorTransferCharacteristic;
    masteringMetadata = stream->masteringMetaData;
    contentLightMetadata = stream->contentLightMetaData;
    stereo_mode = stream->stereo_mode;
    dovi = stream->dovi;
  }
  else if (right.type == STREAM_SUBTITLE)
  {
  }
}
