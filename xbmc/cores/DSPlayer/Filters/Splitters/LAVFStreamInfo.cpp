/*
*      Copyright (C) 2010 Team XBMC
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

#include "LAVFStreamInfo.h"
#include "LAVFGuidHelper.h"
#include "moreuuids.h"
#include "mmreg.h"
#include <vector>

CLAVFStreamInfo::CLAVFStreamInfo(AVStream *avstream, const char* containerFormat, HRESULT &hr)
  : CStreamInfo()
  , m_containerFormat(containerFormat)
{
  m_containerFormat = std::string(containerFormat);
  // FFMpeg renamed the format at some point in time
  if (m_containerFormat == "matroska,webm")
  {
    m_containerFormat = "matroska";
  }

  switch (avstream->codec->codec_type)
  {
  case AVMEDIA_TYPE_AUDIO:
    hr = CreateAudioMediaType(avstream);
    break;
  case AVMEDIA_TYPE_VIDEO:
    hr = CreateVideoMediaType(avstream);
    break;
  case AVMEDIA_TYPE_SUBTITLE:
    hr = CreateSubtitleMediaType(avstream);
    break;
  default:
    hr = E_FAIL;
    break;
  }
}

CLAVFStreamInfo::~CLAVFStreamInfo()
{
}

STDMETHODIMP CLAVFStreamInfo::CreateAudioMediaType(AVStream *avstream)
{
  DllAvFormat dllAvFormat;
  dllAvFormat.Load();
  DllAvCodec dllAvCodec;
  dllAvCodec.Load();
  if (avstream->codec->codec_tag == 0)
  {
    avstream->codec->codec_tag = dllAvFormat.av_codec_get_tag(mp_wav_taglists, avstream->codec->codec_id);
  }
  CMediaType mtype = g_GuidHelper.initAudioType(avstream->codec->codec_id, avstream->codec->codec_tag);

  if (mtype.formattype == FORMAT_WaveFormatEx)
  {
    bool doExtra = true;
    WAVEFORMATEX *wvfmt = NULL;
    // Special Logic for the MPEG1 Audio Formats (MP1, MP2)
    if (mtype.subtype == MEDIASUBTYPE_MPEG1AudioPayload)
    {
      doExtra = false;
      MPEG1WAVEFORMAT* mfmt = (MPEG1WAVEFORMAT*)mtype.AllocFormatBuffer(sizeof(MPEG1WAVEFORMAT));
      memset(wvfmt, 0, sizeof(MPEG1WAVEFORMAT));

      avstream->codec->codec_tag = WAVE_FORMAT_MPEG;
      avstream->codec->block_align = (avstream->codec->codec_id == CODEC_ID_MP1)
                                     ? (12 * avstream->codec->bit_rate / avstream->codec->sample_rate) * 4
                                     : 144 * avstream->codec->bit_rate / avstream->codec->sample_rate;

      mfmt->dwHeadBitrate = avstream->codec->bit_rate;
      mfmt->fwHeadMode = avstream->codec->channels == 1 ? ACM_MPEG_SINGLECHANNEL : ACM_MPEG_DUALCHANNEL;
      mfmt->fwHeadLayer = (avstream->codec->codec_id == CODEC_ID_MP1) ? ACM_MPEG_LAYER1 : ACM_MPEG_LAYER2;

      mfmt->wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX);
      wvfmt = (WAVEFORMATEX *)mfmt;
    }
    else if (mtype.subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO)
    {
      avstream->codec->codec_tag = WAVE_FORMAT_PCM;
      wvfmt = (WAVEFORMATEX_HDMV_LPCM*)mtype.AllocFormatBuffer(sizeof(WAVEFORMATEX_HDMV_LPCM));
      memset(wvfmt, 0, sizeof(WAVEFORMATEX_HDMV_LPCM));
      wvfmt->cbSize = sizeof(WAVEFORMATEX_HDMV_LPCM) - sizeof(WAVEFORMATEX);
    }
    else
    {
      wvfmt = (WAVEFORMATEX*)mtype.AllocFormatBuffer(sizeof(WAVEFORMATEX) + avstream->codec->extradata_size);
      memset(wvfmt, 0, sizeof(WAVEFORMATEX));
    }

    // Check if its LATM by any chance
    // This doesn't seem to work with any decoders i tested, but at least we won't connect to any wrong decoder
    if (m_containerFormat == "mpegts" && avstream->codec->codec_id == CODEC_ID_AAC)
    {
      // PESContext in mpegts.c
      int *pes = (int *)avstream->priv_data;
      if (pes[2] == 0x11)
      {
        avstream->codec->codec_tag = WAVE_FORMAT_LATM_AAC;
        mtype.subtype = MEDIASUBTYPE_LATM_AAC;
      }
    }

    // TODO: values for this are non-trivial, see <mmreg.h>
    wvfmt->wFormatTag = avstream->codec->codec_tag;

    wvfmt->nChannels = avstream->codec->channels;
    wvfmt->nSamplesPerSec = avstream->codec->sample_rate;
    wvfmt->nAvgBytesPerSec = avstream->codec->bit_rate / 8;

    if (avstream->codec->codec_id == CODEC_ID_AAC)
    {
      wvfmt->wBitsPerSample = 0;
      wvfmt->nBlockAlign = 1;
    }
    else
    {
      wvfmt->wBitsPerSample = avstream->codec->bits_per_coded_sample;
      if (wvfmt->wBitsPerSample == 0)
      {
        wvfmt->wBitsPerSample = dllAvCodec.av_get_bits_per_sample_format(avstream->codec->sample_fmt);
      }

      if ( avstream->codec->block_align > 0 )
      {
        wvfmt->nBlockAlign = avstream->codec->block_align;
      }
      else
      {
        if ( wvfmt->wBitsPerSample == 0 )
        {
          CLog::Log(LOGERROR,"BitsPerSample is 0, no good!");
        }
        wvfmt->nBlockAlign = (WORD)((wvfmt->nChannels * wvfmt->wBitsPerSample) / 8);
      }
    }

    if (doExtra && avstream->codec->extradata_size > 0)
    {
      wvfmt->cbSize = avstream->codec->extradata_size;
      memcpy(wvfmt + 1, avstream->codec->extradata, avstream->codec->extradata_size);
    }
  }
  else if (mtype.formattype == FORMAT_VorbisFormat)
  {
    // With Matroska and Ogg we know how to split up the extradata
    // and put it into a VorbisFormat2
    if (m_containerFormat == "matroska" || m_containerFormat == "ogg")
    {
      BYTE *p = avstream->codec->extradata;
      std::vector<int> sizes;
      // read the number of blocks, and then the sizes of the individual blocks
      for (BYTE n = *p++; n > 0; n--)
      {
        int size = 0;
        // Xiph Lacing
        do
        {
          size = *p;
        }
        while (*p++ == 0xFF);
        sizes.push_back(size);
      }
      int totalsize = 0;
      for (unsigned int i = 0; i < sizes.size(); i++)
        totalsize += sizes[i];

      // Get the size of the last block
      sizes.push_back(avstream->codec->extradata_size - (p - avstream->codec->extradata) - totalsize);
      totalsize += sizes[sizes.size()-1];

      // 3 blocks is the currently valid Vorbis format
      if (sizes.size() == 3)
      {
        mtype.subtype = MEDIASUBTYPE_Vorbis2;
        mtype.formattype = FORMAT_VorbisFormat2;
        VORBISFORMAT2* pvf2 = (VORBISFORMAT2*)mtype.AllocFormatBuffer(sizeof(VORBISFORMAT2) + totalsize);
        memset(pvf2, 0, sizeof(VORBISFORMAT2));
        pvf2->Channels = avstream->codec->channels;
        pvf2->SamplesPerSec = avstream->codec->sample_rate;
        pvf2->BitsPerSample = avstream->codec->bits_per_coded_sample;
        BYTE* p2 = mtype.pbFormat + sizeof(VORBISFORMAT2);
        for (unsigned int i = 0; i < sizes.size(); p += sizes[i], p2 += sizes[i], i++)
        {
          memcpy(p2, p, pvf2->HeaderSize[i] = sizes[i]);
        }
        mtypes.push_back(mtype);
      }
    }
    // Old vorbis header without extradata
    mtype.subtype = MEDIASUBTYPE_Vorbis;
    mtype.formattype = FORMAT_VorbisFormat;

    VORBISFORMAT *vfmt = (VORBISFORMAT *)mtype.AllocFormatBuffer(sizeof(VORBISFORMAT));
    memset(vfmt, 0, sizeof(VORBISFORMAT));
    vfmt->nChannels = avstream->codec->channels;
    vfmt->nSamplesPerSec = avstream->codec->sample_rate;
    vfmt->nAvgBitsPerSec = avstream->codec->bit_rate;
    vfmt->nMinBitsPerSec = vfmt->nMaxBitsPerSec = (DWORD) - 1;
  }

  //TODO Fix the sample size
  //if (avstream->codec->bits_per_coded_sample == 0)

  mtypes.push_back(mtype);
  return S_OK;
}

STDMETHODIMP CLAVFStreamInfo::CreateVideoMediaType(AVStream *avstream)
{
  DllAvFormat dllAvFormat;
  dllAvFormat.Load();
  if (avstream->codec->codec_tag == 0)
  {
    avstream->codec->codec_tag = dllAvFormat.av_codec_get_tag(mp_bmp_taglists, avstream->codec->codec_id);
  }
  CMediaType mtype = g_GuidHelper.initVideoType(avstream->codec->codec_id, avstream->codec->codec_tag);

  mtype.bTemporalCompression = 1;
  mtype.bFixedSizeSamples = 0; // TODO

  // If we need aspect info, we switch to VIH2
  AVRational r = avstream->sample_aspect_ratio;
  if (mtype.formattype == FORMAT_VideoInfo && (r.den > 0 && r.num > 0 && (r.den > 1 || r.num > 1)))
  {
    mtype.formattype = FORMAT_VideoInfo2;
  }

  if (mtype.formattype == FORMAT_VideoInfo)
  {
    mtype.pbFormat = (BYTE *)g_GuidHelper.CreateVIH(avstream, &mtype.cbFormat);
  }
  else if (mtype.formattype == FORMAT_VideoInfo2)
  {
    mtype.pbFormat = (BYTE *)g_GuidHelper.CreateVIH2(avstream, &mtype.cbFormat, (m_containerFormat == "mpegts"));
  }
  else if (mtype.formattype == FORMAT_MPEGVideo)
  {
    mtype.pbFormat = (BYTE *)g_GuidHelper.CreateMPEG1VI(avstream, &mtype.cbFormat);
  }
  else if (mtype.formattype == FORMAT_MPEG2Video)
  {
    mtype.pbFormat = (BYTE *)g_GuidHelper.CreateMPEG2VI(avstream, &mtype.cbFormat, (m_containerFormat == "mpegts"));
    // mpeg-ts ships its stuff in annexb form, and MS defines annexb to go with H264 instead of AVC1
    // sadly, ffdshow doesnt connect to H264 (and doesnt work on annexb in general)
    if (m_containerFormat == "mpegts" && avstream->codec->codec_id == CODEC_ID_H264)
    {
      mtype.subtype = MEDIASUBTYPE_H264;
      ((MPEG2VIDEOINFO *)mtype.pbFormat)->hdr.bmiHeader.biCompression = FOURCCMap(&MEDIASUBTYPE_H264).Data1;
    }
  }

  mtypes.push_back(mtype);
  return S_OK;
}

STDMETHODIMP CLAVFStreamInfo::CreateSubtitleMediaType(AVStream *avstream)
{
  // Skip teletext
  if (avstream->codec->codec_id == CODEC_ID_DVB_TELETEXT)
  {
    return E_FAIL;
  }
  CMediaType mtype;
  mtype.majortype = MEDIATYPE_Subtitle;
  mtype.formattype = FORMAT_SubtitleInfo;
  // create format info
  SUBTITLEINFO *subInfo = (SUBTITLEINFO *)mtype.AllocFormatBuffer(sizeof(SUBTITLEINFO) + avstream->codec->extradata_size);
  memset(subInfo, 0, mtype.FormatLength());
  DllAvFormat dllAvFormat;
  dllAvFormat.Load();
  if (dllAvFormat.av_metadata_get(avstream->metadata, "language", NULL, 0))
  {
    char *lang = dllAvFormat.av_metadata_get(avstream->metadata, "language", NULL, 0)->value;
    strncpy_s(subInfo->IsoLang, 4, lang, _TRUNCATE);
  }

  if (dllAvFormat.av_metadata_get(avstream->metadata, "title", NULL, 0))
  {
    // read metadata
    char *title = dllAvFormat.av_metadata_get(avstream->metadata, "title", NULL, 0)->value;
    // convert to wchar
    MultiByteToWideChar(CP_UTF8, 0, title, -1, subInfo->TrackName, 256);
  }

  // Extradata
  memcpy(mtype.pbFormat + (subInfo->dwOffset = sizeof(SUBTITLEINFO)), avstream->codec->extradata, avstream->codec->extradata_size);

  // TODO CODEC_ID_MOV_TEXT
  mtype.subtype = avstream->codec->codec_id == CODEC_ID_TEXT ? MEDIASUBTYPE_UTF8 :
                  avstream->codec->codec_id == CODEC_ID_SSA ? MEDIASUBTYPE_ASS :
                  avstream->codec->codec_id == CODEC_ID_HDMV_PGS_SUBTITLE ? MEDIASUBTYPE_HDMVSUB :
                  avstream->codec->codec_id == CODEC_ID_DVD_SUBTITLE ? MEDIASUBTYPE_VOBSUB :
                  avstream->codec->codec_id == CODEC_ID_DVB_SUBTITLE ? MEDIASUBTYPE_DVB_SUBTITLES :
                  MEDIASUBTYPE_NULL;

  mtypes.push_back(mtype);
  return S_OK;
}
