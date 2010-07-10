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
#include "application.h"
#include "DVDPlayer/DVDCodecs/DVDCodecs.h"
#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"
#include "DSGuidHelper.h"
#include "MMReg.h"
#include "moreuuids.h"
#include "DShowUtil/DShowUtil.h"

extern "C"
{
  #include "libavformat/avformat.h"
}

CDSStreamInfo::CDSStreamInfo(){ extradata = NULL; Clear(); }
CDSStreamInfo::CDSStreamInfo(const CDSStreamInfo &right, bool withextradata, const char *containerFormat)     
{
  extradata = NULL; 
  Clear(); 
  Assign(right, withextradata, containerFormat); 
}

CDSStreamInfo::CDSStreamInfo(const CDemuxStream &right, bool withextradata, const char *containerFormat)  
{ 
  extradata = NULL; 
  Clear(); 
  Assign(right, withextradata, containerFormat); 
}

CDSStreamInfo::~CDSStreamInfo()
{
  if( extradata && extrasize ) 
    free(extradata);
  extradata = NULL;
  extrasize = 0;
}


void CDSStreamInfo::Clear()
{

  codec_id = CODEC_ID_NONE;
  type = STREAM_NONE;
  software = false;

  if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;

  fpsscale = 0;
  fpsrate  = 0;
  m_avgtimeperframe = 0;
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
  if( codec_id != right.codec_id
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
  info.Assign(right, withextradata, "");
  return Equal(info, withextradata);
}


// ASSIGNMENT
void CDSStreamInfo::Assign(const CDSStreamInfo& right, bool withextradata, const char* containerFormat)
{
  ContainerFormat = CStdString(containerFormat);
  codec_id = right.codec_id;
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
  PinNameW = right.PinNameW;
  // VIDEO
  fpsscale = right.fpsscale;
  fpsrate  = right.fpsrate;
  m_avgtimeperframe   = right.m_avgtimeperframe;
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

void CDSStreamInfo::Assign(const CDemuxStream& right, bool withextradata, const char* containerFormat)
{
  Clear();
  ContainerFormat = CStdString(containerFormat);
  mtype.InitMediaType();
  codec_id = right.codec;
  type = right.type;
  RECT empty_tagrect = {0,0,0,0};
  m_dllAvCodec.Load(); m_dllAvFormat.Load(); m_dllAvCodec.Load(); m_dllAvUtil.Load();
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
    PinNameW = L"Audio";
    const CDemuxStreamAudio *stream = static_cast<const CDemuxStreamAudio*>(&right);
    AVStream *avstream = static_cast<AVStream*>(stream->pPrivate);

    avstream->codec->codec_tag = m_dllAvFormat.av_codec_get_tag(mp_wav_taglists, avstream->codec->codec_id);

    mtype = g_GuidHelper.initAudioType(avstream->codec->codec_id, avstream->codec->codec_tag);

    //Link to WAVEFORMATEX structure
    //http://msdn.microsoft.com/en-us/library/dd390970%28VS.85%29.aspx

    WAVEFORMATEX* wvfmt = (WAVEFORMATEX*)mtype.AllocFormatBuffer(sizeof(WAVEFORMATEX) + avstream->codec->extradata_size);
    
    if (avstream->codec->codec_id == CODEC_ID_AAC)
    {
      WAVEFORMATEX* wfe = (WAVEFORMATEX*)new BYTE[sizeof(WAVEFORMATEX)+5];
      memset(wfe, 0, sizeof(WAVEFORMATEX)+5);
      wfe->wFormatTag = WAVE_FORMAT_AAC;
      wfe->nChannels = avstream->codec->channels <= 6 ? avstream->codec->channels : 2;
      wfe->nSamplesPerSec = avstream->codec->sample_rate;
      wfe->nBlockAlign = avstream->pts_wrap_bits;//not sure but seems to work for all the sample i have

      wfe->nAvgBytesPerSec = 1546;//To fix
      wfe->cbSize = DShowUtil::MakeAACInitData((BYTE*)(wfe+1), 1, wfe->nSamplesPerSec, wfe->nChannels);//to fix get the profile correctly
      //DShowUtil::MakeAACInitData((BYTE*)(wfe+1), h.profile, wfe->nSamplesPerSec, wfe->nChannels);
      mtype.SetFormatType(&FORMAT_WaveFormatEx);
      mtype.SetFormat((BYTE*)wfe, sizeof(WAVEFORMATEX)+wfe->cbSize);
      return;
    }
    
    wvfmt->wFormatTag= mtype.subtype.Data1;
    wvfmt->nChannels = avstream->codec->channels;
    wvfmt->nSamplesPerSec = avstream->codec->sample_rate;
    wvfmt->wBitsPerSample = avstream->codec->bits_per_coded_sample;
    if (wvfmt->wBitsPerSample == 0)
      wvfmt->wBitsPerSample = m_dllAvCodec.av_get_bits_per_sample_format(avstream->codec->sample_fmt);
    wvfmt->cbSize = 0;

    if ( avstream->codec->block_align > 0 )
      wvfmt->nBlockAlign = avstream->codec->block_align;
    else
    {

      if ( wvfmt->wBitsPerSample == 0 )
      {
        CLog::Log(LOGERROR,"bits per sample is 0 this is NO GOOD");  
      }
      wvfmt->nBlockAlign = (WORD)((wvfmt->nChannels * wvfmt->wBitsPerSample) / 8);

    }


    wvfmt->nAvgBytesPerSec= wvfmt->nSamplesPerSec * wvfmt->nBlockAlign;

    wvfmt->cbSize = avstream->codec->extradata_size + sizeof(WAVEFORMATEX);
    if (avstream->codec->extradata_size > 0)
    {
      memcpy(wvfmt + 1, avstream->codec->extradata, avstream->codec->extradata_size);
    }
    //TODO Fix the sample size 
    if (avstream->codec->bits_per_coded_sample == 0)
      mtype.SetSampleSize(256000);


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
    PinNameW = L"Video";
    const CDemuxStreamVideo *stream = static_cast<const CDemuxStreamVideo*>(&right);
    AVStream* avstream = static_cast<AVStream*>(stream->pPrivate);

    avstream->codec->codec_tag = m_dllAvFormat.av_codec_get_tag(mp_bmp_taglists, avstream->codec->codec_id);

    mtype = g_GuidHelper.initVideoType(avstream->codec->codec_id, avstream->codec->codec_tag);

    mtype.bTemporalCompression = 0;
    mtype.bFixedSizeSamples = stream->bVFR ? 0 : 1; //hummm is it the right value???
    //mtype.subtype = FOURCCMap(avstream->codec->codec_tag);


    //videoinfoheader2 has deinterlace flags and aspect ratio
    //see http://msdn.microsoft.com/en-us/library/dd407326%28VS.85%29.aspx for more info
    //Is it necessary to use it?
    //if ((mtype.formattype == FORMAT_VideoInfo) && (avstream->codec->sample_aspect_ratio.den > 0 && avstream->codec->sample_aspect_ratio.num > 0))
    //mtype.formattype = FORMAT_VideoInfo2;
    if (mtype.formattype == FORMAT_VideoInfo)
    {
      mtype.pbFormat = (BYTE *)g_GuidHelper.CreateVIH(stream, avstream, &mtype.cbFormat);
    }
    else if (mtype.formattype == FORMAT_VideoInfo2)
    {
      mtype.pbFormat = (BYTE *)g_GuidHelper.CreateVIH2(stream, avstream, &mtype.cbFormat);
    }
    else if (mtype.formattype == FORMAT_MPEGVideo)
    {
      mtype.pbFormat = (BYTE *)g_GuidHelper.CreateMPEG1VI(stream, avstream, &mtype.cbFormat);
    }
    else if (mtype.formattype == FORMAT_MPEG2Video)
    {
      mtype.pbFormat = (BYTE *)g_GuidHelper.CreateMPEG2VI(stream, avstream, &mtype.cbFormat, (ContainerFormat == "mpegts"));
    }

    fpsscale  = stream->iFpsScale;
    fpsrate   = stream->iFpsRate;
    height    = stream->iHeight;
    width     = stream->iWidth;
    aspect    = stream->fAspect;
    vfr       = stream->bVFR;
  }
  /**/
  else if(  right.type == STREAM_SUBTITLE )
  {
    PinNameW = L"Subtitle";
    const CDemuxStreamSubtitle *stream = static_cast<const CDemuxStreamSubtitle*>(&right);
    AVStream* avstream = static_cast<AVStream*>(stream->pPrivate);
    identifier = stream->identifier;
    mtype.majortype = MEDIATYPE_Subtitle;
    mtype.formattype = FORMAT_SubtitleInfo;

    SUBTITLEINFO *psi = (SUBTITLEINFO *)mtype.AllocFormatBuffer(sizeof(SUBTITLEINFO) + extrasize);
    memset(psi, 0, mtype.FormatLength());

    if (m_dllAvFormat.av_metadata_get(avstream->metadata, "language", NULL, 0))
    {
      char *lang = m_dllAvFormat.av_metadata_get(avstream->metadata, "language", NULL, 0)->value;
      strncpy_s(psi->IsoLang, 4, lang, _TRUNCATE);
    }

    if (m_dllAvFormat.av_metadata_get(avstream->metadata, "title", NULL, 0))
    {
      char *title = m_dllAvFormat.av_metadata_get(avstream->metadata, "title", NULL, 0)->value;
      
      // convert to wchar
      size_t convertedChars = 0;
      mbstowcs_s(&convertedChars, psi->TrackName, 256, title, _TRUNCATE);
    }
    memcpy(mtype.pbFormat + (psi->dwOffset = sizeof(SUBTITLEINFO)), extradata, extrasize);

    mtype.subtype = avstream->codec->codec_id == CODEC_ID_TEXT ? MEDIASUBTYPE_UTF8 :
      avstream->codec->codec_id == CODEC_ID_SSA ? MEDIASUBTYPE_SSA :
      MEDIASUBTYPE_NULL;

    mtype.lSampleSize = 1;
  }
}