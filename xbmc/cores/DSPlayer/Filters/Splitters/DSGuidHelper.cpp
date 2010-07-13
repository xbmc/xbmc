#include "DSGuidHelper.h"
#include "windows.h"
#include "moreuuids.h"
#include "ffmpeg_mediaguids.h"

#ifndef max
#define max(A,B)	( (A) > (B) ? (A):(B)) 
#endif

CDSGuidHelper g_GuidHelper;

CMediaType CDSGuidHelper::initAudioType(CodecID codecId, unsigned int codecTag)
{
  CMediaType thetype;
  thetype.InitMediaType();
  thetype.majortype = MEDIATYPE_Audio;
  thetype.subtype = FOURCCMap(codecTag);
  thetype.formattype = FORMAT_WaveFormatEx; //default value

  // special cases
  switch(codecId)
  {
  case CODEC_ID_AC3:
    thetype.subtype = MEDIASUBTYPE_DOLBY_AC3;
    break;
  case CODEC_ID_AAC:
    thetype.subtype = MEDIASUBTYPE_AAC;
    break;
  case CODEC_ID_DTS:
    thetype.subtype = MEDIASUBTYPE_DTS;
    break;
  case CODEC_ID_TRUEHD:
    thetype.subtype = MEDIASUBTYPE_DOLBY_AC3;
    break;
  }
  return thetype;
}

CMediaType CDSGuidHelper::initVideoType(CodecID codecId, unsigned int codecTag)
{
  CMediaType thetype;
  thetype.InitMediaType();
  thetype.majortype = MEDIATYPE_Video;
  thetype.subtype = FOURCCMap(codecTag);
  thetype.formattype = FORMAT_VideoInfo; //default value

  switch(codecId)
  {
  case CODEC_ID_ASV1:
  case CODEC_ID_ASV2:
    thetype.formattype = FORMAT_VideoInfo2;
    break;
  case CODEC_ID_FLV1:
    thetype.formattype = FORMAT_VideoInfo2;
    break;
  case CODEC_ID_H263:
  case CODEC_ID_H263I:
    thetype.subtype = FOURCCMap(FOURCC_H263);
    break;
  case CODEC_ID_H264:
    thetype.formattype = FORMAT_MPEG2Video;
    break;
  case CODEC_ID_HUFFYUV:
    thetype.formattype = FORMAT_VideoInfo2;
    break;
  case CODEC_ID_MPEG1VIDEO:
    thetype.formattype = FORMAT_MPEGVideo;
    thetype.subtype = MEDIASUBTYPE_MPEG1Payload;
    break;
  case CODEC_ID_MPEG2VIDEO:
    thetype.formattype = FORMAT_MPEG2Video;
    break;
  case CODEC_ID_RV10:
  case CODEC_ID_RV20:
  case CODEC_ID_RV30:
  case CODEC_ID_RV40:
    thetype.formattype = FORMAT_VideoInfo2;
    break;
  case CODEC_ID_VC1:
    thetype.formattype = FORMAT_VideoInfo2;
    break;
  }

  return thetype;

}

int64_t CDSGuidHelper::lavc_gcd(int64_t a, int64_t b)
{
  if(b) return lavc_gcd(b, a%b);
  else  return a;
}

uint64_t ff_abs(int64_t x)
{
  return uint64_t((x<0) ? -x : x);
}

int CDSGuidHelper::math_reduce(int *dst_nom, int *dst_den, int64_t nom, int64_t den, int64_t max)
{
  AVRational a0={0,1}, a1={1,0};
  int sign= (nom<0) ^ (den<0);
  int64_t gcd= lavc_gcd(ff_abs(nom), ff_abs(den));

  if(gcd){
    nom = ff_abs(nom)/gcd;
    den = ff_abs(den)/gcd;
  }
  if(nom<=max && den<=max){
    a1.num=(int)nom;a1.den=(int)den;//= (AVRational){nom, den};
    den=0;
  }

  while(den){
    int64_t x       = nom / den;
    int64_t next_den= nom - den*x;
    int64_t a2n= x*a1.num + a0.num;
    int64_t a2d= x*a1.den + a0.den;

    if(a2n > max || a2d > max) break;

    a0= a1;
    a1.num=(int)a2n;a1.den=(int)a2d;//= (AVRational){a2n, a2d};
    nom= den;
    den= next_den;
  }
  ASSERT(lavc_gcd(a1.num, a1.den) == 1U);

  *dst_nom = sign ? -a1.num : a1.num;
  *dst_den = a1.den;

  return den==0;
}

DWORD avc_quant(BYTE *src, BYTE *dst, int extralen)
{
  DWORD cb = 0;
  BYTE* src_end = (BYTE *) src + extralen;
  BYTE* dst_end = (BYTE *) dst + extralen;
  src += 5;
  for (int i = 0; i < 2; i++)
  {
    for (int n = *(src++) & 0x1f; n > 0; n--)
    {
      unsigned len = (((unsigned)src[0] << 8) | src[1]) + 2;
      if(src + len > src_end || dst + len > dst_end) { ASSERT(0); break; }
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

// ISO/IEC 14496-10:2004 Annex B Byte stream format
DWORD avc_annexb_parser(BYTE *src, BYTE *dst, int extralen)
{
  BYTE *endpos = src + extralen;
  BYTE *spspos = 0, *ppspos = 0;
  UINT16 spslen = 0, ppslen = 0;

  BYTE *p = src;

  // skip any trailing bytes until we find a header
  while(p < (endpos-4) && next_bits(p, 24) != 0x000001 && 
    next_bits(p, 32) != 0x00000001)
  {
    // skip one
    p++;
  }

  // Repeat while:
  //    We're not at the end of the stream
  //    We're at a section start
  //    We still need SPS or PPS
  while(p < (endpos-4) && (next_bits(p, 24) == 0x000001 || next_bits(p, 32) == 0x00000001) && (!spspos || !ppspos))
  {
    // Skip the bytestream nal header
    if (next_bits(p, 32) == 0x000001) {
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
    else if (unit_type == 8 && ref_idc != 0) { // Picture parameter set
      ppspos = p;
    }

    // go to end of block
    while(1) {
      // either we find another NAL unit block, or the end of the stream
      if((p < (endpos-4) && (next_bits(p, 24) == 0x000001 || next_bits(p, 32) == 0x00000001))
        || (p == endpos)) {
          break;
      } else {
        p++;
      }
    }
    // if a position is set, but doesnt have a length yet, its just been discovered
    // (or something went wrong)
    if(spspos && !spslen) {
      spslen = p - spspos;
    } else if (ppspos && !ppslen) {
      ppslen = p - ppspos;
    }
  }

  // if we can't parse the header, we just don't do anything with it
  // Alternative: copy it as-is, without parsing?
  if (!spspos || !spslen || !ppspos || !ppslen)
    return 0;

  // Keep marker for length calcs
  BYTE *dstmarker = dst;

  // The final extradata format is quite simpel
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

  return (dst - dstmarker);
}

VIDEOINFOHEADER *CDSGuidHelper::CreateVIH(const CDemuxStreamVideo *stream, AVStream* avstream, ULONG *size)
{
  VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)CoTaskMemAlloc(ULONG(sizeof(VIDEOINFOHEADER) + avstream->codec->extradata_size));
  memset(pvi, 0, sizeof(VIDEOINFOHEADER));
  pvi->AvgTimePerFrame = (REFERENCE_TIME)(10000000 / ((float)stream->iFpsRate / (float)stream->iFpsScale));;
  pvi->dwBitErrorRate = 0;
  pvi->dwBitRate = avstream->codec->bit_rate;
  RECT empty_tagrect = {0,0,0,0};
  pvi->rcSource = empty_tagrect;//Some codecs like wmv are setting that value to the video current value
  pvi->rcTarget = empty_tagrect;
  pvi->rcTarget.right = pvi->rcSource.right = stream->iWidth;
  pvi->rcTarget.bottom = pvi->rcSource.bottom = stream->iHeight;
  pvi->bmiHeader.biSize = ULONG(sizeof(BITMAPINFOHEADER) + avstream->codec->extradata_size);

  memcpy((BYTE*)&pvi->bmiHeader + sizeof(BITMAPINFOHEADER), avstream->codec->extradata, avstream->codec->extradata_size);
  pvi->bmiHeader.biWidth = stream->iWidth;
  pvi->bmiHeader.biHeight = stream->iHeight;

  pvi->bmiHeader.biBitCount = avstream->codec->bits_per_coded_sample;
  pvi->bmiHeader.biSizeImage = stream->iWidth * stream->iHeight * pvi->bmiHeader.biBitCount / 8;
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

VIDEOINFOHEADER2 *CDSGuidHelper::CreateVIH2(const CDemuxStreamVideo *stream, AVStream* avstream, ULONG *size)
{
  int extra = 0;
  BYTE *extradata;

  // Create a VIH that we'll convert
  VIDEOINFOHEADER *vih = CreateVIH(stream, avstream, size);

  if(vih->bmiHeader.biSize > sizeof(BITMAPINFOHEADER)) 
  {
    extra = vih->bmiHeader.biSize - sizeof(BITMAPINFOHEADER);
    // increase extra size by one, because VIH2 requires one 0 byte between header and extra data
    extra++;

    extradata = (BYTE*)&vih->bmiHeader + sizeof(BITMAPINFOHEADER);
  }

  VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2 *)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER2) + extra); 
  memset(vih2, 0, sizeof(VIDEOINFOHEADER2));

  vih2->rcSource = vih->rcSource;
  vih2->rcTarget = vih->rcTarget;
  vih2->dwBitRate = vih->dwBitRate;
  vih2->dwBitErrorRate = vih->dwBitErrorRate;
  vih2->AvgTimePerFrame = vih->AvgTimePerFrame;
  vih2->dwPictAspectRatioX = vih->bmiHeader.biWidth;
  vih2->dwPictAspectRatioY = vih->bmiHeader.biHeight;
  memcpy(&vih2->bmiHeader, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
  vih2->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  vih2->dwInterlaceFlags = 0;
  vih2->dwCopyProtectFlags = 0;
  vih2->dwControlFlags = 0;
  vih2->dwReserved2 = 0;

  if(extra) {
    // The first byte after the infoheader has to be 0
    *((BYTE*)vih2 + sizeof(VIDEOINFOHEADER2)) = 0;
    // after that, the extradata .. size reduced by one again
    memcpy((BYTE*)vih2 + sizeof(VIDEOINFOHEADER2) + 1, extradata, extra - 1);
  }

  // Free the VIH that we converted
  CoTaskMemFree((PVOID)vih);

  *size = sizeof(VIDEOINFOHEADER2) + extra;
  return vih2;
}

MPEG1VIDEOINFO *CDSGuidHelper::CreateMPEG1VI(const CDemuxStreamVideo *stream, AVStream* avstream, ULONG *size)
{
  int extra = 0;
  BYTE *extradata;

  // Create a VIH that we'll convert
  VIDEOINFOHEADER *vih = CreateVIH(stream, avstream, size);

  if(vih->bmiHeader.biSize > sizeof(BITMAPINFOHEADER)) 
  {
    extra = vih->bmiHeader.biSize - sizeof(BITMAPINFOHEADER);
    extradata = (BYTE*)&vih->bmiHeader + sizeof(BITMAPINFOHEADER);
  }

  MPEG1VIDEOINFO *mp1vi = (MPEG1VIDEOINFO *)CoTaskMemAlloc(sizeof(MPEG1VIDEOINFO) + max(extra - 1, 0)); 
  memset(mp1vi, 0, sizeof(MPEG1VIDEOINFO));

  // The MPEG1VI is a thin wrapper around a VIH, so its easy!
  memcpy(&mp1vi->hdr, vih, sizeof(VIDEOINFOHEADER));
  mp1vi->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  mp1vi->dwStartTimeCode = 0; // is this not 0 anywhere..?

  // copy extradata over
  if(extra) {
    mp1vi->cbSequenceHeader = extra;
    memcpy(mp1vi->bSequenceHeader, extradata, extra);
  }

  // Free the VIH that we converted
  CoTaskMemFree((PVOID)vih);

  // The '1' is from the allocated space of bSequenceHeader
  *size = sizeof(MPEG1VIDEOINFO) + max(mp1vi->cbSequenceHeader - 1, 0);
  return mp1vi;
}

MPEG2VIDEOINFO *CDSGuidHelper::CreateMPEG2VI(const CDemuxStreamVideo *stream, AVStream *avstream, ULONG *size, bool is_mpegts_format)
{
  int extra = 0;
  BYTE *extradata;

  // Create a VIH that we'll convert
  VIDEOINFOHEADER2 *vih2 = CreateVIH2(stream, avstream, size);

  if(*size > sizeof(VIDEOINFOHEADER2))
  {
    extra = *size - sizeof(VIDEOINFOHEADER2);
    extra--;
  }
  MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)CoTaskMemAlloc(sizeof(MPEG2VIDEOINFO) + max(extra - 4, 0)); 
  memset(mp2vi, 0, sizeof(MPEG2VIDEOINFO));
  memcpy(&mp2vi->hdr, vih2, sizeof(VIDEOINFOHEADER2));
  mp2vi->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

  // Set profile/level if we know them
  mp2vi->dwProfile = (avstream->codec->profile != FF_PROFILE_UNKNOWN) ? avstream->codec->profile : 0;
  mp2vi->dwLevel = (avstream->codec->level != FF_LEVEL_UNKNOWN) ? avstream->codec->level : 0;
  //mp2vi->dwFlags = 4; // where do we get flags otherwise..?
#if TEST_INTERNAL_VIDEO_DECODER
  mp2vi->cbSequenceHeader = avstream->codec->extradata_size;
  memcpy(&mp2vi->dwSequenceHeader[0], avstream->codec->extradata, avstream->codec->extradata_size);
#else
  if(extra)
  {
    extradata = (BYTE*)vih2 + sizeof(VIDEOINFOHEADER2) + 1;

    if(avstream->codec->codec_id == CODEC_ID_H264)
    {
      if(!is_mpegts_format)
      {
        mp2vi->dwProfile = extradata[1];
        mp2vi->dwLevel = extradata[3];
        mp2vi->dwFlags = (extradata[4] & 3 + 1);
        mp2vi->cbSequenceHeader = avc_quant(extradata,
          (BYTE *)(&mp2vi->dwSequenceHeader[0]), extra);
      } else {
        // EXPERIMENTAL FUNCTION!
        // Do all MPEG-TS use the Annex B format?
        mp2vi->dwFlags = 4;
        mp2vi->cbSequenceHeader = avc_annexb_parser(extradata,
          (BYTE *)(&mp2vi->dwSequenceHeader[0]), extra);
      }
    }
    else
    {
      mp2vi->cbSequenceHeader = extra;
      memcpy(&mp2vi->dwSequenceHeader[0], extradata, extra);
    }
  }
#endif

  // Free the VIH2 that we converted
  CoTaskMemFree((PVOID)vih2);

  *size = SIZE_MPEG2VIDEOINFO(mp2vi);
  return mp2vi;
}
