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

#include "DSStreamInfo.h"

#include "DVDPlayer/DVDCodecs/DVDCodecs.h"
#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DSGuidHelper.h"
#include "MMReg.h"
extern "C"
{
  #include "libavformat/avformat.h"
}
CDSStreamInfo::CDSStreamInfo()                                                     { extradata = NULL; Clear(); }
CDSStreamInfo::CDSStreamInfo(const CDSStreamInfo &right, bool withextradata )     { extradata = NULL; Clear(); Assign(right, withextradata); }
CDSStreamInfo::CDSStreamInfo(const CDemuxStream &right, bool withextradata )       { extradata = NULL; Clear(); Assign(right, withextradata); }

CDSStreamInfo::~CDSStreamInfo()
{
  if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;
}


void CDSStreamInfo::Clear()
{
  
  codec = CODEC_ID_NONE;
  type = STREAM_NONE;
  software = false;

  if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;

  fpsscale = 0;
  fpsrate  = 0;
  height   = 0;
  width    = 0;
  aspect   = 0.0;
  vfr      = false;
  stills   = false;

  channels   = 0;
  samplerate = 0;
  blockalign = 0;
  bitrate    = 0;
  bitspersample = 0;

  identifier = 0;
}

bool CDSStreamInfo::Equal(const CDSStreamInfo& right, bool withextradata)
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
  ||  fpsrate  != right.fpsrate
  ||  height   != right.height
  ||  width    != right.width
  ||  stills   != right.stills
  ||  vfr      != right.vfr) return false;

  // AUDIO
  if( channels      != right.channels
  ||  samplerate    != right.samplerate
  ||  blockalign    != right.blockalign
  ||  bitrate       != right.bitrate
  ||  bitspersample != right.bitspersample ) return false;

  // SUBTITLE
  if( identifier != right.identifier ) return false;

  return true;
}

bool CDSStreamInfo::Equal(const CDemuxStream& right, bool withextradata)
{
  CDSStreamInfo info;
  info.Assign(right, withextradata);
  return Equal(info, withextradata);
}


// ASSIGNMENT
void CDSStreamInfo::Assign(const CDSStreamInfo& right, bool withextradata)
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
  fpsrate  = right.fpsrate;
  height   = right.height;
  width    = right.width;
  aspect   = right.aspect;
  stills   = right.stills;

  // AUDIO
  channels      = right.channels;
  samplerate    = right.samplerate;
  blockalign    = right.blockalign;
  bitrate       = right.bitrate;
  bitspersample = right.bitspersample;

  // SUBTITLE
  identifier = right.identifier;
}

void CDSStreamInfo::Assign(const CDemuxStream& right, bool withextradata)
{
  Clear();
  mtype.InitMediaType();
  codec = right.codec;
  type = right.type;

  if( withextradata && right.ExtraSize )
  {
    extrasize = right.ExtraSize;
    extradata = malloc(extrasize);
    memcpy(extradata, right.ExtraData, extrasize);
  }
  /****************/
  /* AUDIO STREAM */
  /****************/
  if( right.type == STREAM_AUDIO )
  {
    const CDemuxStreamAudio *stream = static_cast<const CDemuxStreamAudio*>(&right);
    AVStream* avstream = static_cast<AVStream*>(stream->pPrivate);

    mtype.majortype = MEDIATYPE_Audio;
    mtype.formattype = FORMAT_WaveFormatEx;
    mtype.subtype = FOURCCMap(avstream->codec->codec_tag);
    
    
    WAVEFORMATEX *wvfmt = (WAVEFORMATEX*)mtype.AllocFormatBuffer(sizeof(WAVEFORMATEX) + extrasize);
    //if the extrasize is of 12 its a mp3
    if (extrasize == 12)
    {
    }
    wvfmt->wFormatTag = avstream->codec->codec_tag;
    wvfmt->nChannels = avstream->codec->channels;
    wvfmt->nSamplesPerSec= avstream->codec->sample_rate;
    wvfmt->nAvgBytesPerSec= avstream->codec->bit_rate/8;
    wvfmt->nBlockAlign= avstream->codec->block_align ? avstream->codec->block_align : 1;
    wvfmt->wBitsPerSample= avstream->codec->bits_per_coded_sample;
    wvfmt->cbSize= avstream->codec->extradata_size;
    mtype.SetSampleSize(channels * (bitrate / 8));

    if(avstream->codec->extradata_size)
      memcpy(wvfmt + 1, avstream->codec->extradata, avstream->codec->extradata_size);
    mtype.pbFormat = (PBYTE)wvfmt;
    
    //wvfmt->wFormatTag
    //MPEGLAYER3WAVEFORMAT *
    
    channels      = stream->iChannels;
    samplerate    = stream->iSampleRate;
    blockalign    = stream->iBlockAlign;
    bitrate       = stream->iBitRate;
    bitspersample = stream->iBitsPerSample;
  }
  /****************/
  /* VIDEO STREAM */
  /****************/
  else if(  right.type == STREAM_VIDEO )
  {
    const CDemuxStreamVideo *stream = static_cast<const CDemuxStreamVideo*>(&right);
    AVStream* avstream = static_cast<AVStream*>(stream->pPrivate);
    mtype.majortype = MEDIATYPE_Video;
    mtype.formattype = FORMAT_VideoInfo;
  
    mtype.bTemporalCompression = 0;
    
    mtype.bFixedSizeSamples = stream->bVFR ? 0 : 1; //hummm is it the right value???

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)mtype.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
  
  
  //Still to be verified in real time but the mathematics make sense
  // 23.97 fps = 417166;
  //so fps = 10000000.0 / AvgTimePerFrame
  //or 
  //AvgTimePerFrame =  10000000 / fps 
  pvi->AvgTimePerFrame = (REFERENCE_TIME)(10000000 / (stream->iFpsRate / stream->iFpsScale));
  
  pvi->dwBitErrorRate = 0;
  
  pvi->dwBitRate = avstream->codec->bit_rate;
  pvi->bmiHeader.biSize = sizeof(pvi->bmiHeader);
  pvi->bmiHeader.biWidth= stream->iWidth;
  pvi->bmiHeader.biHeight= stream->iHeight;
  
  pvi->bmiHeader.biBitCount= avstream->codec->bits_per_coded_sample;
  pvi->bmiHeader.biSizeImage = stream->iWidth * stream->iHeight * avstream->codec->bits_per_coded_sample / 8;
  pvi->bmiHeader.biCompression= stream->codec_fourcc;
  //TOFIX The bitplanes is depending on the subtype
  pvi->bmiHeader.biPlanes = 1;
  pvi->bmiHeader.biClrUsed = 0;
  pvi->bmiHeader.biClrImportant = 0;
  pvi->bmiHeader.biYPelsPerMeter = 0;
  pvi->bmiHeader.biXPelsPerMeter = 0;
  
  //This still need to be verified only tested xvid
  mtype.subtype = MEDIATYPE_Video;
  mtype.subtype.Data1 = pvi->bmiHeader.biCompression;

  mtype.SetFormat((PBYTE)pvi,sizeof(VIDEOINFOHEADER));
  //Not sure if its really working but in case the other way dont work will try this one
  /*const FOURCC *fccs=g_GuidHelper.getCodecFOURCCs(avstream->codec->codec_id);
  std::vector<FOURCC> lstFourcc;
  for (const FOURCC *fcc=fccs;*fcc;fcc++)
  {
    lstFourcc.push_back(*fcc);
  }*/
    
    fpsscale  = stream->iFpsScale;
    fpsrate   = stream->iFpsRate;
    height    = stream->iHeight;
    width     = stream->iWidth;
    aspect    = stream->fAspect;
    vfr       = stream->bVFR;
  }
  else if(  right.type == STREAM_SUBTITLE )
  {
    const CDemuxStreamSubtitle *stream = static_cast<const CDemuxStreamSubtitle*>(&right);
    identifier = stream->identifier;
  }
}
