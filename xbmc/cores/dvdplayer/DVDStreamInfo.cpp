#include "stdafx.h"

#include "DVDStreamInfo.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDDemuxers/DVDDemux.h"

CDVDStreamInfo::CDVDStreamInfo()                                                     { extradata = NULL; Clear(); }
CDVDStreamInfo::CDVDStreamInfo(const CDVDStreamInfo &right, bool withextradata )     { extradata = NULL; Clear(); Assign(right, withextradata); }
CDVDStreamInfo::CDVDStreamInfo(const CDemuxStream &right, bool withextradata )       { extradata = NULL; Clear(); Assign(right, withextradata); }

CDVDStreamInfo::~CDVDStreamInfo()
{
  if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;
}


void CDVDStreamInfo::Clear()
{
  codec = CODEC_ID_NONE;
  type = STREAM_NONE;

  if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;

  fpsscale = 0;
  fpsrate = 0;
  height = 0;
  width = 0;
  aspect = 0.0;

  channels = 0;
  samplerate = 0;

  identifier = 0;
}

bool CDVDStreamInfo::Equal(const CDVDStreamInfo& right, bool withextradata)
{
  if( codec != right.codec
  ||  type != right.type ) return false;

  if( withextradata )
  {
    if( extrasize != right.extrasize ) return false;
    if( extrasize )
    {
      if( memcmp(extradata, right.extradata, extrasize) != 0 ) return false;
    }
  }

  // VIDEO
  if( fpsscale != right.fpsscale
  ||  fpsrate != right.fpsscale
  ||  height != right.height
  ||  width != right.width 
  ||  aspect != right.aspect) return false;

  // AUDIO
  if( channels != right.channels
  ||  samplerate != right.samplerate ) return false;

  // SUBTITLE
  if( identifier != right.identifier ) return false;

  return true;
}

bool CDVDStreamInfo::Equal(const CDemuxStream& right, bool withextradata)
{
  CDVDStreamInfo info;
  info.Assign(right, withextradata);
  return Equal(info, withextradata);
}


// ASSIGNMENT
void CDVDStreamInfo::Assign(const CDVDStreamInfo& right, bool withextradata)
{
  codec = right.codec;
  type = right.type;

  if( extradata && extrasize ) free(extradata);

  if( withextradata && right.extrasize )
  {
    extrasize = right.extrasize;
    extradata = malloc(extrasize);
    memcpy(extradata, right.extradata, extrasize);
  }
  else
  {
    extrasize = 0;
    extradata = 0;
  }

  // VIDEO
  fpsscale = right.fpsscale;
  fpsrate = right.fpsscale;
  height = right.height;
  width = right.width;
  aspect = right.aspect;

  // AUDIO
  channels = right.channels;
  samplerate = right.samplerate;

  // SUBTITLE
  identifier = right.identifier;
}

void CDVDStreamInfo::Assign(const CDemuxStream& right, bool withextradata)
{
  Clear();

  codec = right.codec;
  type = right.type;

  if( withextradata && right.ExtraSize )
  {
    extrasize = right.ExtraSize;
    extradata = malloc(extrasize);
    memcpy(extradata, right.ExtraData, extrasize);
  }

  if( right.type == STREAM_AUDIO )
  {
    const CDemuxStreamAudio *stream = static_cast<const CDemuxStreamAudio*>(&right);
    channels = stream->iChannels;
    samplerate = stream->iSampleRate;
  }
  else if(  right.type == STREAM_VIDEO )
  {
    const CDemuxStreamVideo *stream = static_cast<const CDemuxStreamVideo*>(&right);
    fpsscale = stream->iFpsScale;
    fpsrate = stream->iFpsRate;
    height = stream->iHeight;
    width = stream->iWidth;
    aspect = stream->fAspect;
  }
  else if(  right.type == STREAM_SUBTITLE )
  {
    const CDemuxStreamSubtitle *stream = static_cast<const CDemuxStreamSubtitle*>(&right);
    identifier = stream->identifier;
  }
}
