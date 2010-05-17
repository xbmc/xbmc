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

#include "moreuuids.h"
extern "C"
{
  #include "libavformat/avformat.h"
}
#include "math.h"
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
  
  codec_id = CODEC_ID_NONE;
  type = STREAM_NONE;
  software = false;

  if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;

  fpsscale = 0;
  fpsrate  = 0;
  avgtimeperframe = 0;
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
  info.Assign(right, withextradata);
  return Equal(info, withextradata);
}


// ASSIGNMENT
void CDSStreamInfo::Assign(const CDSStreamInfo& right, bool withextradata)
{
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

  // VIDEO
  fpsscale = right.fpsscale;
  fpsrate  = right.fpsrate;
  avgtimeperframe   = right.avgtimeperframe;
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
  codec_id = right.codec;
  type = right.type;
  RECT empty_tagrect = {0,0,0,0};
  m_dllAvCodec.Load();
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
    //mtype.subtype = FOURCCMap(avstream->codec_id->codec_tag);
    //TODO convert audio codecid into wave format ex: ac3 is WAVE_FORMAT_DOLBY_AC3 0x2000
    mtype = g_GuidHelper.initAudioType(avstream->codec->codec_id);
    
    if (mtype.subtype == GUID_NULL)
      mtype.subtype = FOURCCMap(avstream->codec->codec_tag);
    WAVEFORMATEX* wvfmt = (WAVEFORMATEX*)mtype.AllocFormatBuffer(sizeof(WAVEFORMATEX) + avstream->codec->extradata_size);
    //avstream->codec_id->codec_tag could be used but not every audio codec as it defined
    m_dllAvFormat.Load();
    //based on mplayerww concept
    //av_codec_get_tag
    if(avstream->codec->codec_tag == MKTAG('m', 'p', '4', 'a'))
      avstream->codec->codec_tag = 0;
    if(!avstream->codec->codec_tag)
      avstream->codec->codec_tag = m_dllAvFormat.av_codec_get_tag(mp_wav_taglists, avstream->codec->codec_id);
    wvfmt->wFormatTag= avstream->codec->codec_tag;
    //wvfmt->wFormatTag = mtype.subtype.Data1;
    wvfmt->nChannels = avstream->codec->channels;
    wvfmt->nSamplesPerSec= avstream->codec->sample_rate;
    wvfmt->wBitsPerSample= avstream->codec->bits_per_coded_sample;
    wvfmt->cbSize = 0;

    if (avstream->codec->block_align > 0 )
      wvfmt->nBlockAlign = avstream->codec->block_align;
    else
      wvfmt->nBlockAlign = (WORD)((wvfmt->nChannels * wvfmt->wBitsPerSample) / 8);// avstream->codec_id->block_align ? avstream->codec_id->block_align : 1;
      
    
    wvfmt->nAvgBytesPerSec= avstream->codec->bit_rate/8;//wvfmt->nSamplesPerSec * wvfmt->nBlockAlign;

    wvfmt->cbSize = avstream->codec->extradata_size;
    if (avstream->codec->extradata_size > 0)
    {
      
      memcpy(wvfmt + 1, avstream->codec->extradata, avstream->codec->extradata_size);
    }
    //TODO Fix the sample size
    if (wvfmt->wBitsPerSample == 0)
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
    const CDemuxStreamVideo *stream = static_cast<const CDemuxStreamVideo*>(&right);
    AVStream* avstream = static_cast<AVStream*>(stream->pPrivate);
    

    /* FormatType status */
    /* VideoInfo working*/
    /* VideoInfo2 */
    /* MPEGVideo */
    /* MPEG2Video working*/
    
    mtype = g_GuidHelper.initVideoType(avstream->codec->codec_id);
    //get the average time perframe in 100nanosec unit
    //Still to be verified in real time but the mathematics make sense
    // 23.97 fps = 417166;
    //AvgTimePerFrame =  10000000 / fps
    avgtimeperframe = (REFERENCE_TIME)(10000000 / ((float)stream->iFpsRate / (float)stream->iFpsScale));

    //videoinfoheader2 has deinterlace flags and aspect ratio
    //see http://msdn.microsoft.com/en-us/library/dd407326%28VS.85%29.aspx for more info
    //Is it necessary to use it?
    //if ((mtype.formattype == FORMAT_VideoInfo) && (avstream->codec->sample_aspect_ratio.den > 0 && avstream->codec->sample_aspect_ratio.num > 0))
      //mtype.formattype = FORMAT_VideoInfo2;
    if (mtype.formattype == FORMAT_VideoInfo)
    {
      mtype.bTemporalCompression = 0;
      
      mtype.bFixedSizeSamples = stream->bVFR ? 0 : 1; //hummm is it the right value???

      VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)mtype.ReallocFormatBuffer(ULONG(sizeof(VIDEOINFOHEADER) + extrasize));
    
      
      
      pvi->AvgTimePerFrame = avgtimeperframe;
      pvi->dwBitErrorRate = 0;
      
      pvi->dwBitRate = avstream->codec->bit_rate;
      pvi->rcSource = empty_tagrect;//Some codecs like wmv are setting that value to the video current value
      pvi->rcTarget = empty_tagrect;
      pvi->rcTarget.right = pvi->rcSource.right = stream->iWidth;
      pvi->rcTarget.bottom = pvi->rcSource.bottom = stream->iHeight;
      pvi->bmiHeader.biSize = ULONG(sizeof(BITMAPINFOHEADER) + extrasize);
      int testsize = mtype.cbFormat - sizeof(VIDEOINFOHEADER);//This should be the size of the extrasize
      memcpy((BYTE*)mtype.pbFormat + sizeof(VIDEOINFOHEADER), extradata, extrasize);

      pvi->bmiHeader.biWidth= stream->iWidth;
      pvi->bmiHeader.biHeight= stream->iHeight;
      
      pvi->bmiHeader.biBitCount= avstream->codec->bits_per_coded_sample;
      pvi->bmiHeader.biSizeImage = stream->iWidth * stream->iHeight * avstream->codec->bits_per_coded_sample / 8;
      pvi->bmiHeader.biCompression= mtype.subtype.Data1;
      //TOFIX The bitplanes is depending on the subtype
      pvi->bmiHeader.biPlanes = 1;
      pvi->bmiHeader.biClrUsed = 0;
      pvi->bmiHeader.biClrImportant = 0;
      pvi->bmiHeader.biYPelsPerMeter = 0;
      pvi->bmiHeader.biXPelsPerMeter = 0;
    }
    else if (mtype.formattype == FORMAT_VideoInfo2)
    {
      mtype.bTemporalCompression = 0;
      
      mtype.bFixedSizeSamples = stream->bVFR ? 0 : 1; //hummm is it the right value???

      VIDEOINFOHEADER2 *pvi = (VIDEOINFOHEADER2*)mtype.ReallocFormatBuffer(ULONG(sizeof(VIDEOINFOHEADER2) + extrasize));
    
      
      
      pvi->AvgTimePerFrame = avgtimeperframe;
      pvi->dwBitErrorRate = 0;
      
      pvi->dwBitRate = avstream->codec->bit_rate;
      pvi->rcSource = empty_tagrect;//Some codecs like wmv are setting that value to the video current value
      pvi->rcTarget = empty_tagrect;
      pvi->rcTarget.right = pvi->rcSource.right = stream->iWidth;
      pvi->rcTarget.bottom = pvi->rcSource.bottom = stream->iHeight;
      pvi->bmiHeader.biSize = ULONG(sizeof(BITMAPINFOHEADER) + extrasize);
      int testsize = mtype.cbFormat - sizeof(VIDEOINFOHEADER2);//This should be the size of the extrasize
      memcpy((BYTE*)mtype.pbFormat + sizeof(VIDEOINFOHEADER2), extradata, extrasize);

      pvi->bmiHeader.biWidth= stream->iWidth;
      pvi->bmiHeader.biHeight= stream->iHeight;
      
      
      //FIXME only the codecs using huffyuv table are giving a number of bits so need to parse the number of byte from the pix_fmt
      //the more common used bits per pixel is 24
      pvi->bmiHeader.biBitCount= 24;
      pvi->bmiHeader.biSizeImage = m_dllAvCodec.avpicture_get_size(PIX_FMT_RGB24, avstream->codec->width, avstream->codec->height);//stream->iWidth * stream->iHeight * avstream->codec->bits_per_coded_sample / 8;
      pvi->bmiHeader.biCompression= mtype.subtype.Data1;
      pvi->bmiHeader.biPlanes = 1;
      pvi->bmiHeader.biClrUsed = 0;
      pvi->bmiHeader.biClrImportant = 0;
      pvi->bmiHeader.biYPelsPerMeter = 0;
      pvi->bmiHeader.biXPelsPerMeter = 0;
      AVRational display_aspect_ratio;


      g_GuidHelper.math_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                  avstream->codec->width*avstream->sample_aspect_ratio.num,
                  avstream->codec->height*avstream->sample_aspect_ratio.den,
                  1024*1024);
      pvi->dwPictAspectRatioX = display_aspect_ratio.num;
      pvi->dwPictAspectRatioY = display_aspect_ratio.den;
      pvi->dwInterlaceFlags = 0;
      pvi->dwCopyProtectFlags = 0;
      pvi->dwControlFlags = 0;
      

    }
    else if (mtype.formattype == FORMAT_MPEG2Video)
    {
      MPEG2VIDEOINFO *mpeginfo = (MPEG2VIDEOINFO*)mtype.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + avstream->codec->extradata_size);

      //(not sure about this but fixed the sample i tried)
      if (avstream->codec->has_b_frames == 1)
        mpeginfo->hdr.AvgTimePerFrame = 0;
      else
        mpeginfo->hdr.AvgTimePerFrame = avgtimeperframe;
      
      //ff_h264_decode_seq_parameter_set from libavcodec is actually contain the SPS & PTS needed to fill the dwsequenceheader
      mpeginfo->hdr.dwBitErrorRate = 0;
      
      mpeginfo->hdr.dwBitRate = avstream->codec->bit_rate;
      //mpeginfo->hdr.dwInterlaceFlags
      
      mpeginfo->hdr.rcSource = empty_tagrect;
      mpeginfo->hdr.rcTarget = empty_tagrect;
      
      mpeginfo->hdr.dwInterlaceFlags = 0;
      mpeginfo->hdr.dwCopyProtectFlags = 0;
      mpeginfo->hdr.dwControlFlags = 0;
      mpeginfo->hdr.dwPictAspectRatioX = (DWORD)stream->iWidth;
      mpeginfo->hdr.dwPictAspectRatioY = (DWORD)stream->iHeight;
      mpeginfo->hdr.bmiHeader.biSize = sizeof(mpeginfo->hdr.bmiHeader);
      mpeginfo->hdr.bmiHeader.biWidth= stream->iWidth;
      mpeginfo->hdr.bmiHeader.biHeight= stream->iHeight;
      
      mpeginfo->hdr.bmiHeader.biBitCount= 24;
      mpeginfo->hdr.bmiHeader.biSizeImage = stream->iWidth * stream->iHeight * avstream->codec->bits_per_coded_sample / 8;
      mpeginfo->hdr.bmiHeader.biCompression = mtype.subtype.Data1;
      mpeginfo->hdr.bmiHeader.biPlanes = 1;
      mpeginfo->hdr.bmiHeader.biClrUsed = 0;
      mpeginfo->hdr.bmiHeader.biClrImportant = 0;
      mpeginfo->hdr.bmiHeader.biYPelsPerMeter = 0;
      mpeginfo->hdr.bmiHeader.biXPelsPerMeter = 0;
      mpeginfo->dwStartTimeCode = 0;// is there any case where it wouldnt be 0?????
      mpeginfo->dwProfile = avstream->codec->profile;
      mpeginfo->dwLevel = avstream->codec->level;
      
      //from ffmpeg full info
      //ex: 01 64 00 1f ff e1 00 19 67 64 00 1f ac 34 e4 01 40 16 ec 04 40 00 00 fa 40 00 2e e0 23 c6 0c 64 80 01 00 05 68 ee b2 c8 b0
      //from directshow
      //ex:                    00 19 67 64 00 1f ac 34  e4 01 40 16 ec 04 40 00 00 fa 40 00 2e e0 23 c6  0c 64 80 00 05 68 ee b2 c8 b0 
      //based on those test the extradata from ffmpeg required for the mpeginfo header is starting at index 7
      //typedef struct SPS in h264.h from ffmpeg source might have all info we need for the MPEG2VIDEOINFO
      
      std::vector<BYTE> extravect;
      extravect.resize(avstream->codec->extradata_size);
      memcpy(&extravect.at(0), avstream->codec->extradata, avstream->codec->extradata_size);
      mpeginfo->dwFlags = (extravect.at(4) & 3) + 1;
      //remove the start of the sequence not neeeded
      extravect.erase(extravect.begin(),extravect.begin() + 6);

      
      //mkv usualy end with 00 05 68 EE B2 C8 B0
      //and from the test i done with 20 different samples the byte right before that is Useless
      extravect.erase(extravect.begin() + extravect.size() - 8);
      //drop before 6 and the byte 33(no clue about why i have to remove this one)
      mpeginfo->cbSequenceHeader = extravect.size();
      
      memcpy(mpeginfo->dwSequenceHeader, &extravect.at(0) , extravect.size());

    }
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