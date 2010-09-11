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

#include "LAVFGuidHelper.h"
#include "moreuuids.h"
#include "BaseDemuxer.h"

#include "ExtradataParser.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"
CLAVFGuidHelper g_GuidHelper;

CMediaType CLAVFGuidHelper::initAudioType(CodecID codecId, unsigned int codecTag)
{
  CMediaType mediaType;
  mediaType.InitMediaType();
  mediaType.majortype = MEDIATYPE_Audio;
  mediaType.subtype = FOURCCMap(codecTag);
  mediaType.formattype = FORMAT_WaveFormatEx; //default value
  mediaType.SetSampleSize(256000);

  // special cases
  switch (codecId)
  {
  case CODEC_ID_AC3:
    mediaType.subtype = MEDIASUBTYPE_DOLBY_AC3;
    break;
  case CODEC_ID_AAC:
    mediaType.subtype = MEDIASUBTYPE_AAC;
    break;
  case CODEC_ID_DTS:
    mediaType.subtype = MEDIASUBTYPE_DTS;
    break;
  case CODEC_ID_EAC3:
    mediaType.subtype = MEDIASUBTYPE_DOLBY_DDPLUS;
    break;
  case CODEC_ID_TRUEHD:
    // Some filters don't work 100% when its set to TrueHD (ffdshow, doh!)
    //mediaType.subtype = MEDIASUBTYPE_DOLBY_TRUEHD;
    mediaType.subtype = MEDIASUBTYPE_DOLBY_AC3;
    break;
  case CODEC_ID_VORBIS:
    //TODO
    mediaType.formattype = FORMAT_VorbisFormat;
    mediaType.subtype = MEDIASUBTYPE_Vorbis;
    break;
  case CODEC_ID_MP1:
  case CODEC_ID_MP2:
    mediaType.subtype = MEDIASUBTYPE_MPEG1AudioPayload;
    break;
  case CODEC_ID_MP3:
    mediaType.subtype = MEDIASUBTYPE_MP3;
    break;
  case CODEC_ID_PCM_BLURAY:
    mediaType.subtype = MEDIASUBTYPE_HDMV_LPCM_AUDIO;
    break;
  }
  return mediaType;
}

CMediaType CLAVFGuidHelper::initVideoType(CodecID codecId, unsigned int codecTag)
{
  CMediaType mediaType;
  mediaType.InitMediaType();
  mediaType.majortype = MEDIATYPE_Video;
  mediaType.subtype = FOURCCMap(codecTag);
  mediaType.formattype = FORMAT_VideoInfo; //default value

  switch (codecId)
  {
  case CODEC_ID_ASV1:
  case CODEC_ID_ASV2:
    mediaType.formattype = FORMAT_VideoInfo2;
    break;
  case CODEC_ID_FLV1:
    mediaType.formattype = FORMAT_VideoInfo2;
    break;
  case CODEC_ID_H263:
  case CODEC_ID_H263I:
    mediaType.subtype = MEDIASUBTYPE_H263;
    break;
  case CODEC_ID_H264:
    mediaType.formattype = FORMAT_MPEG2Video;
    break;
  case CODEC_ID_HUFFYUV:
    mediaType.formattype = FORMAT_VideoInfo2;
    break;
  case CODEC_ID_MPEG1VIDEO:
    mediaType.formattype = FORMAT_MPEGVideo;
    mediaType.subtype = MEDIASUBTYPE_MPEG1Payload;
    break;
  case CODEC_ID_MPEG2VIDEO:
    mediaType.formattype = FORMAT_MPEG2Video;
    mediaType.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
    break;
  case CODEC_ID_RV10:
  case CODEC_ID_RV20:
  case CODEC_ID_RV30:
  case CODEC_ID_RV40:
    mediaType.formattype = FORMAT_VideoInfo2;
    break;
  case CODEC_ID_WMV3:
  case CODEC_ID_VC1:
    mediaType.formattype = FORMAT_VideoInfo2;
    break;
  }

  return mediaType;
}

DWORD avc_quant(BYTE *src, BYTE *dst, int extralen)
{
  DWORD cb = 0;
  BYTE* src_end = (BYTE *) src + extralen;
  BYTE* dst_end = (BYTE *) dst + extralen;
  src += 5;
  // Two runs, for sps and pps
  for (int i = 0; i < 2; i++)
  {
    for (int n = *(src++) & 0x1f; n > 0; n--)
    {
      unsigned len = (((unsigned)src[0] << 8) | src[1]) + 2;
      if (src + len > src_end || dst + len > dst_end)
      {
        ASSERT(0);
        break;
      }
      memcpy(dst, src, len);
      src += len;
      dst += len;
      cb += len;
    }
  }
  return cb;
}

// Helper function to get the next number of bits from the buffer
// Supports reading 0 to 64 bits.
UINT64 next_bits(BYTE *buf, int nBits)
{
  ASSERT(nBits >= 0 && nBits <= 64);

  UINT64 bitbuf = 0;

  int bitlen = 0;
  for (; bitlen < nBits; bitlen += 8)
  {
    bitbuf <<= 8;
    bitbuf |= *buf++;
  }
  UINT64 ret = (bitbuf >> (bitlen - nBits)) & ((1ui64 << nBits) - 1);

  return ret;
}

DWORD avc_parse_annexb(BYTE *src, BYTE *dst, int extralen)
{
  BYTE *endpos = src + extralen;
  BYTE *spspos = 0, *ppspos = 0;
  UINT16 spslen = 0, ppslen = 0;

  BYTE *p = src;

  // ISO/IEC 14496-10:2004 Annex B Byte stream format
  // skip any trailing bytes until we find a header
  while (p < (endpos - 4) && next_bits(p, 24) != 0x000001 &&
         next_bits(p, 32) != 0x00000001)
  {
    // skip one
    p++;
  }

  // Repeat while:
  //    We're not at the end of the stream
  //    We're at a section start
  //    We still need SPS or PPS
  while (p < (endpos - 4) && (next_bits(p, 24) == 0x000001 || next_bits(p, 32) == 0x00000001) && (!spspos || !ppspos))
  {
    // Skip the bytestream nal header
    if (next_bits(p, 32) == 0x000001)
    {
      p++;
    }
    p += 3;

    // first byte in the nal unit and their bit-width:
    //    zero bit  (1)
    //    ref_idc   (2)
    //    unit_type (5)
    BYTE ref_idc = *p & 0x60;
    BYTE unit_type = *p & 0x1f;
    // unit types lookup table, figure 7-1, chapter 7.4.1
    if (unit_type == 7 && ref_idc != 0) // Sequence parameter set
    {
      spspos = p;
    }
    else if (unit_type == 8 && ref_idc != 0)   // Picture parameter set
    {
      ppspos = p;
    }

    // go to end of block
    while (1)
    {
      // either we find another NAL unit block, or the end of the stream
      if ((p < (endpos - 4) && (next_bits(p, 24) == 0x000001 || next_bits(p, 32) == 0x00000001))
          || (p == endpos))
      {
        break;
      }
      else
      {
        p++;
      }
    }
    // if a position is set, but doesnt have a length yet, its just been discovered
    // (or something went wrong)
    if (spspos && !spslen)
    {
      spslen = (UINT16)(p - spspos);
    }
    else if (ppspos && !ppslen)
    {
      ppslen = (UINT16)(p - ppspos);
    }
  }

  // if we can't parse the header, we just don't do anything with it
  // Alternative: copy it as-is, without parsing?
  if (!spspos || !spslen || !ppspos || !ppslen)
    return 0;

  // Keep marker for length calcs
  BYTE *dstmarker = dst;

  // The final extradata format is quite simple
  //  A 16-bit size value of the sections, followed by the actual section data

  // copy SPS over
  *dst++ = spslen >> 8;
  *dst++ = spslen & 0xff;
  memcpy(dst, spspos, spslen);
  dst += spslen;

  // and PPS
  *dst++ = ppslen >> 8;
  *dst++ = ppslen & 0xff;
  memcpy(dst, ppspos, ppslen);
  dst += ppslen;

  return (DWORD)(dst - dstmarker);
}

VIDEOINFOHEADER *CLAVFGuidHelper::CreateVIH(const AVStream* avstream, ULONG *size)
{
  DllAvUtil dllAvUtil;
  dllAvUtil.Load();
  VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)CoTaskMemAlloc(ULONG(sizeof(VIDEOINFOHEADER) + avstream->codec->extradata_size));
  memset(pvi, 0, sizeof(VIDEOINFOHEADER));
  // Get the frame rate
  if (avstream->r_frame_rate.den > 0 &&  avstream->r_frame_rate.num > 0)
  {
    pvi->AvgTimePerFrame = dllAvUtil.av_rescale_rnd(DSHOW_TIME_BASE, avstream->r_frame_rate.den, avstream->r_frame_rate.num, AV_ROUND_NEAR_INF);
  }
  else if (avstream->avg_frame_rate.den > 0 &&  avstream->avg_frame_rate.num > 0)
  {
    pvi->AvgTimePerFrame = dllAvUtil.av_rescale_rnd(DSHOW_TIME_BASE, avstream->avg_frame_rate.den, avstream->avg_frame_rate.num, AV_ROUND_NEAR_INF);
  }
  pvi->dwBitErrorRate = 0;
  pvi->dwBitRate = avstream->codec->bit_rate;
  RECT empty_tagrect = {0, 0, 0, 0};
  pvi->rcSource = empty_tagrect;//Some codecs like wmv are setting that value to the video current value
  pvi->rcTarget = empty_tagrect;
  pvi->rcTarget.right = pvi->rcSource.right = avstream->codec->width;
  pvi->rcTarget.bottom = pvi->rcSource.bottom = avstream->codec->height;

  memcpy((BYTE*)&pvi->bmiHeader + sizeof(BITMAPINFOHEADER), avstream->codec->extradata, avstream->codec->extradata_size);
  pvi->bmiHeader.biSize = ULONG(sizeof(BITMAPINFOHEADER) + avstream->codec->extradata_size);

  pvi->bmiHeader.biWidth = avstream->codec->width;
  pvi->bmiHeader.biHeight = avstream->codec->height;
  pvi->bmiHeader.biBitCount = avstream->codec->bits_per_coded_sample;
  pvi->bmiHeader.biSizeImage = avstream->codec->width * avstream->codec->height * pvi->bmiHeader.biBitCount / 8;
  pvi->bmiHeader.biCompression = FOURCCMap(avstream->codec->codec_tag).Data1;
  //TOFIX The bitplanes is depending on the subtype
  pvi->bmiHeader.biPlanes = 1;
  pvi->bmiHeader.biClrUsed = 0;
  pvi->bmiHeader.biClrImportant = 0;
  pvi->bmiHeader.biYPelsPerMeter = 0;
  pvi->bmiHeader.biXPelsPerMeter = 0;

  *size = sizeof(VIDEOINFOHEADER) + avstream->codec->extradata_size;
  return pvi;
}

int64_t lavc_gcd(int64_t a, int64_t b)
{
  if(b) return lavc_gcd(b, a%b);
  else  return a;
}

uint64_t ff_abs(int64_t x)
{
  return uint64_t((x<0) ? -x : x);
}

int math_reduce(int *dst_nom, int *dst_den, int64_t nom, int64_t den, int64_t max)
{
  AVRational a0={0,1}, a1={1,0};
  int sign= (nom<0) ^ (den<0);
  int64_t gcd = lavc_gcd(ff_abs(nom), ff_abs(den));

  if(gcd){
    nom = ff_abs(nom)/gcd;
    den = ff_abs(den)/gcd;
  }

  if(nom<=max && den<=max){
    a1.num=(int)nom;a1.den=(int)den;//= (AVRational){nom, den};
    den=0;
  }

  while(den){
    int64_t x        = nom / den;
    int64_t next_den = nom - den*x;
    int64_t a2n = x*a1.num + a0.num;
    int64_t a2d = x*a1.den + a0.den;

    if(a2n > max || a2d > max) break;

    a0 = a1;
    a1.num = (int)a2n;
    a1.den = (int)a2d;//= (AVRational){a2n, a2d};
    nom = den;
    den = next_den;
  }
  ASSERT(lavc_gcd(a1.num, a1.den) == 1U);

  *dst_nom = sign ? -a1.num : a1.num;
  *dst_den = a1.den;

  return (den == 0);
}

VIDEOINFOHEADER2 *CLAVFGuidHelper::CreateVIH2(const AVStream* avstream, ULONG *size, bool is_mpegts_format)
{
  int extra = 0;
  BYTE *extradata = NULL;

  // Create a VIH that we'll convert
  VIDEOINFOHEADER *vih = CreateVIH(avstream, size);

  if (avstream->codec->extradata_size > 0)
  {
    extra = avstream->codec->extradata_size;
    //increase extra size by one, because VIH2 requires one 0 byte between header and extra data
    if (is_mpegts_format)
    {
      extra++;
    }

    extradata = avstream->codec->extradata;
  }

  VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2 *)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER2) + extra);
  memset(vih2, 0, sizeof(VIDEOINFOHEADER2));

  vih2->rcSource = vih->rcSource;
  vih2->rcTarget = vih->rcTarget;
  vih2->dwBitRate = vih->dwBitRate;
  vih2->dwBitErrorRate = vih->dwBitErrorRate;
  vih2->AvgTimePerFrame = vih->AvgTimePerFrame;

  // Calculate aspect ratio
  AVRational r = avstream->sample_aspect_ratio;
  AVRational rc = avstream->codec->sample_aspect_ratio;
  int num = vih->bmiHeader.biWidth, den = vih->bmiHeader.biHeight;
  if (r.den > 0 && r.num > 0 && (r.den > 1 || r.num > 1))
  {
    math_reduce(&num, &den, (int64_t)r.num * num, (int64_t)r.den * den, 255);
  }
  else if (rc.den > 0 && rc.num > 0 && (rc.den > 1 || rc.num > 1))
  {
    math_reduce(&num, &den, (int64_t)rc.num * num, (int64_t)rc.den * den, 255);
  }
  vih2->dwPictAspectRatioX = num;
  vih2->dwPictAspectRatioY = den;

  memcpy(&vih2->bmiHeader, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
  vih2->bmiHeader.biSize = sizeof(BITMAPINFOHEADER) + extra;

  vih2->dwInterlaceFlags = 0;
  vih2->dwCopyProtectFlags = 0;
  vih2->dwControlFlags = 0;
  vih2->dwReserved2 = 0;

  if (extra)
  {
    // The first byte after the infoheader has to be 0 in mpeg-ts
    if (is_mpegts_format)
    {
      *((BYTE*)vih2 + sizeof(VIDEOINFOHEADER2)) = 0;
      // after that, the extradata .. size reduced by one again
      memcpy((BYTE*)vih2 + sizeof(VIDEOINFOHEADER2) + 1, extradata, extra - 1);
    }
    else
    {
      memcpy((BYTE*)vih2 + sizeof(VIDEOINFOHEADER2), extradata, extra);
    }
  }

  // Free the VIH that we converted
  CoTaskMemFree(vih);

  *size = sizeof(VIDEOINFOHEADER2) + extra;
  return vih2;
}

MPEG1VIDEOINFO *CLAVFGuidHelper::CreateMPEG1VI(const AVStream* avstream, ULONG *size)
{
  int extra = 0;
  BYTE *extradata = NULL;

  // Create a VIH that we'll convert
  VIDEOINFOHEADER *vih = CreateVIH(avstream, size);

  if (avstream->codec->extradata_size > 0)
  {
    extra = avstream->codec->extradata_size;
    extradata = avstream->codec->extradata;
  }

  MPEG1VIDEOINFO *mp1vi = (MPEG1VIDEOINFO *)CoTaskMemAlloc(sizeof(MPEG1VIDEOINFO) + extra);
  memset(mp1vi, 0, sizeof(MPEG1VIDEOINFO));

  // The MPEG1VI is a thin wrapper around a VIH, so its easy!
  memcpy(&mp1vi->hdr, vih, sizeof(VIDEOINFOHEADER));
  mp1vi->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  mp1vi->dwStartTimeCode = 0; // is this not 0 anywhere..?
  mp1vi->hdr.bmiHeader.biPlanes = 0;
  mp1vi->hdr.bmiHeader.biCompression = 0;

  // copy extradata over
  if (extra)
  {
    CExtradataParser parser = CExtradataParser(extradata, extra);
    mp1vi->cbSequenceHeader = parser.ParseMPEGSequenceHeader(mp1vi->bSequenceHeader);
  }

  // Free the VIH that we converted
  CoTaskMemFree(vih);

  *size = SIZE_MPEG1VIDEOINFO(mp1vi);
  return mp1vi;
}

MPEG2VIDEOINFO *CLAVFGuidHelper::CreateMPEG2VI(const AVStream *avstream, ULONG *size, bool is_mpegts_format)
{
  int extra = 0;
  BYTE *extradata = NULL;

  // Create a VIH that we'll convert
  VIDEOINFOHEADER2 *vih2 = CreateVIH2(avstream, size);

  if (avstream->codec->extradata_size > 0)
  {
    extra = avstream->codec->extradata_size;
    extradata = avstream->codec->extradata;
  }

  MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)CoTaskMemAlloc(sizeof(MPEG2VIDEOINFO) + std::max(extra - 4, 0));
  memset(mp2vi, 0, sizeof(MPEG2VIDEOINFO));
  memcpy(&mp2vi->hdr, vih2, sizeof(VIDEOINFOHEADER2));
  mp2vi->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  // Set profile/level if we know them
  mp2vi->dwProfile = (avstream->codec->profile != FF_PROFILE_UNKNOWN) ? avstream->codec->profile : 0;
  mp2vi->dwLevel = (avstream->codec->level != FF_LEVEL_UNKNOWN) ? avstream->codec->level : 0;
  //mp2vi->dwFlags = 4; // where do we get flags otherwise..?

  if (extra > 0)
  {
    // Don't even go there for mpeg-ts for now, we supply annex-b
    if (avstream->codec->codec_id == CODEC_ID_H264 && !is_mpegts_format)
    {
      if (!is_mpegts_format)
      {
        mp2vi->dwProfile = extradata[1];
        mp2vi->dwLevel = extradata[3];
        mp2vi->dwFlags = (extradata[4] & 3) + 1;
        mp2vi->cbSequenceHeader = avc_quant(extradata,
                                            (BYTE *)(&mp2vi->dwSequenceHeader[0]), extra);
      }
      else
      {
        // EXPERIMENTAL FUNCTION!
        mp2vi->dwFlags = 4;
        mp2vi->cbSequenceHeader = avc_parse_annexb(extradata,
                                  (BYTE *)(&mp2vi->dwSequenceHeader[0]), extra);
      }
    }
    else if (avstream->codec->codec_id == CODEC_ID_MPEG2VIDEO)
    {
      CExtradataParser parser = CExtradataParser(extradata, extra);
      mp2vi->cbSequenceHeader = parser.ParseMPEGSequenceHeader((BYTE *) & mp2vi->dwSequenceHeader[0]);
      mp2vi->hdr.bmiHeader.biPlanes = 0;
      mp2vi->hdr.bmiHeader.biCompression = 0;
    }
    else
    {
      mp2vi->cbSequenceHeader = extra;
      memcpy(&mp2vi->dwSequenceHeader[0], extradata, extra);
    }
  }

  // Free the VIH2 that we converted
  CoTaskMemFree(vih2);

  *size = SIZE_MPEG2VIDEOINFO(mp2vi);
  return mp2vi;
}
