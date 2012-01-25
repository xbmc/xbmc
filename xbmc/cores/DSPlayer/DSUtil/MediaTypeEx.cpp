#include "stdafx.h"
#include "MediaTypeEx.h"

#include <mmreg.h>
#include <initguid.h>
#include "DSUtil.h"

#pragma pack(push, 1)
typedef struct
{
  WAVEFORMATEX Format;
  BYTE bBigEndian;
  BYTE bsid;
  BYTE lfeon;
  BYTE copyrightb;
  BYTE nAuxBitsCode;  //  Aux bits per frame
} DOLBYAC3WAVEFORMAT;
#pragma pack(pop)

CMediaTypeEx::CMediaTypeEx()
{
}

CStdStringW CMediaTypeEx::ToString(IPin* pPin)
{
  CStdStringW packing, type, codec, dim, rate, dur;

  // TODO

  if(majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
  {
    packing = L"Encrypted MPEG2 Pack";
  }
  else if(majortype == MEDIATYPE_MPEG2_PACK)
  {
    packing = L"MPEG2 Pack";
  }
  else if(majortype == MEDIATYPE_MPEG2_PES)
  {
    packing = L"MPEG2 PES";
  }
  
  if(majortype == MEDIATYPE_Video)
  {
    type = L"Video";

    BITMAPINFOHEADER bih;
    bool fBIH = ExtractBIH(this, &bih);

    int w, h, arx, ary;
    bool fDim = ExtractDim(this, w, h, arx, ary);

    if(fBIH && bih.biCompression)
    {
      codec = GetVideoCodecName(subtype, bih.biCompression);
    }

    if(codec.IsEmpty())
    {
      if(formattype == FORMAT_MPEGVideo) codec = L"MPEG1 Video";
      else if(formattype == FORMAT_MPEG2_VIDEO) codec = L"MPEG2 Video";
      else if(formattype == FORMAT_DiracVideoInfo) codec = L"Dirac Video";
    }

    if(fDim)
    {
      dim.Format(L"%dx%d", w, h);
      if(w*ary != h*arx) dim.Format(L"%s (%d:%d)", dim, arx, ary);
    }

    if(formattype == FORMAT_VideoInfo || formattype == FORMAT_MPEGVideo)
    {
      VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pbFormat;
      if(vih->AvgTimePerFrame) rate.Format(L"%0.2ffps ", 10000000.0f / vih->AvgTimePerFrame);
      if(vih->dwBitRate) rate.Format(L"%s%dKbps", rate, vih->dwBitRate/1000);
    }
    else if(formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO || formattype == FORMAT_DiracVideoInfo)
    {
      VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pbFormat;
      if(vih->AvgTimePerFrame) rate.Format(L"%0.2ffps ", 10000000.0f / vih->AvgTimePerFrame);
      if(vih->dwBitRate) rate.Format(L"%s%dKbps", rate, vih->dwBitRate/1000);
    }

    rate.Trim();

    if(subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
    {
      type = L"Subtitle";
      codec = L"DVD Subpicture";
    }
  }
  else if(majortype == MEDIATYPE_Audio)
  {
    type = L"Audio";

    if(formattype == FORMAT_WaveFormatEx)
    {
      WAVEFORMATEX* wfe = (WAVEFORMATEX*)Format();

      if(wfe->wFormatTag/* > WAVE_FORMAT_PCM && wfe->wFormatTag < WAVE_FORMAT_EXTENSIBLE
      && wfe->wFormatTag != WAVE_FORMAT_IEEE_FLOAT*/
      || subtype != GUID_NULL)
      {
        codec = GetAudioCodecName(subtype, wfe->wFormatTag);
        dim.Format(L"%dHz"), wfe->nSamplesPerSec;
        if(wfe->nChannels == 1) dim.Format(L"%s mono"), CStdString(dim);
        else if(wfe->nChannels == 2) dim.Format(L"%s stereo"), CStdString(dim);
        else dim.Format(L"%s %dch"), CStdString(dim), wfe->nChannels;
        if(wfe->nAvgBytesPerSec) rate.Format(L"%dKbps"), wfe->nAvgBytesPerSec*8/1000;
      }
    }
    else if(formattype == FORMAT_VorbisFormat)
    {
      VORBISFORMAT* vf = (VORBISFORMAT*)Format();

      codec = GetAudioCodecName(subtype, 0);
      dim.Format(L"%dHz"), vf->nSamplesPerSec;
      if(vf->nChannels == 1) dim.Format(L"%s mono"), CStdString(dim);
      else if(vf->nChannels == 2) dim.Format(L"%s stereo"), CStdString(dim);
      else dim.Format(L"%s %dch"), CStdString(dim), vf->nChannels;
      if(vf->nAvgBitsPerSec) rate.Format(L"%dKbps"), vf->nAvgBitsPerSec/1000;
    }
    else if(formattype == FORMAT_VorbisFormat2)
    {
      VORBISFORMAT2* vf = (VORBISFORMAT2*)Format();

      codec = GetAudioCodecName(subtype, 0);
      dim.Format(L"%dHz"), vf->SamplesPerSec;
      if(vf->Channels == 1) dim.Format(L"%s mono"), CStdString(dim);
      else if(vf->Channels == 2) dim.Format(L"%s stereo"), CStdString(dim);
      else dim.Format(L"%s %dch"), CStdString(dim), vf->Channels;
    }        
  }
  else if(majortype == MEDIATYPE_Text)
  {
    type = L"Text";
  }
  else if(majortype == MEDIATYPE_Subtitle)
  {
    type = L"Subtitle";
    codec = GetSubtitleCodecName(subtype);
  }
  else
  {
    type = L"Unknown";
  }

  IMediaSeeking* pMS;
  if (pPin && SUCCEEDED(pPin->QueryInterface(__uuidof(IMediaSeeking),(void**)&pMS)))
  {
    REFERENCE_TIME rtDur = 0;
    if(SUCCEEDED(pMS->GetDuration(&rtDur)) && rtDur)
    {
      rtDur /= 10000000;
      int s = rtDur%60;
      rtDur /= 60;
      int m = rtDur%60;
      rtDur /= 60;
      int h = (int)rtDur;
      if(h) dur.Format(L"%d:%02d:%02d"), h, m, s;
      else if(m) dur.Format(L"%02d:%02d"), m, s;
      else if(s) dur.Format(L"%ds"), s;
    }
  }

  CStdStringW str;
  if(!codec.IsEmpty()) str += codec + L" ";
  if(!dim.IsEmpty()) str += dim + L" ";
  if(!rate.IsEmpty()) str += rate + L" ";
  if(!dur.IsEmpty()) str += dur + L" ";
  //str.Trim(L" ,");
  str.TrimLeft(L" ,");
  
  if(!str.IsEmpty()) str = type + L": " + str;
  else str = type;

  return str;
}

CStdStringA CMediaTypeEx::GetVideoCodecName(const GUID& subtype, DWORD biCompression, DWORD* fourcc)
{
  CStdStringA str;
  
  static std::map<DWORD, CStdStringA> names;
  //static CAtlMap<DWORD, CStdString> names;

  if(names.empty())
  {
    names['WMV1'] = "Windows Media Video 7|wmv";
    names['WMV2'] = "Windows Media Video 8|wmv";
    names['WMV3'] = "Windows Media Video 9|wmv";
    names['DIV3'] = "DivX 3|divx";
    names['MP43'] = "MSMPEG4v3|mp4";
    names['MP42'] = "MSMPEG4v2|mp4";
    names['MP41'] = "MSMPEG4v1|mp4";
    names['DX50'] = "DivX 5|divx";
    names['DIVX'] = "DivX 6|divx";
    names['XVID'] = "Xvid|xvid";
    names['MP4V'] = "MPEG4 Video|mp4";
    names['AVC1'] = "MPEG4 Video (H264)|h264";
    names['H264'] = "MPEG4 Video (H264)|h264";
    names['RV10'] = "RealVideo 1|rv";
    names['RV20'] = "RealVideo 2|rv";
    names['RV30'] = "RealVideo 3|rv";
    names['RV40'] = "RealVideo 4|rv";
    names['FLV1'] = "Flash Video 1|flv";
    names['FLV4'] = "Flash Video 4|flv";
    names['VP50'] = "On2 VP5";
    names['VP60'] = "On2 VP6";
    names['SVQ3'] = "SVQ3";
    names['SVQ1'] = "SVQ1";
    names['H263'] = "H263";
    names['WVC1'] = "WMV 9 Advanced Profile (VC1)|vc1";
    names['WMV3'] = "WMV 9 (VC1)|vc1";
    names['CCV1'] = "MPEG4 Video (H264)|h264";
    // names[''] = "";
  }

  if(biCompression)
  {
    BYTE* b = (BYTE*)&biCompression;

    for(int i = 0; i < 4; i++)
    {
      if(b[i] >= 'a' && b[i] <= 'z') 
        b[i] = toupper(b[i]);
    }

    std::map<DWORD, CStdStringA>::iterator it = names.find(MAKEFOURCC(b[3], b[2], b[1], b[0]));
    if (it == names.end())
    {
      if(subtype == MEDIASUBTYPE_DiracVideo) str = "Dirac Video";
      // else if(subtype == ) str = "";
      else if(biCompression < 256) str.Format("%d"), biCompression;
      else str.Format("%4.4hs"), &biCompression;
    } else {
      str = it->second;
    }

    if (fourcc)
      *fourcc = MAKEFOURCC(b[3], b[2], b[1], b[0]);
  }

  return str;
}

CStdStringA CMediaTypeEx::GetAudioCodecName(const GUID& subtype, WORD wFormatTag)
{
  CStdStringA str;

  static std::map<WORD, CStdStringA> names;

  if(names.empty())
  {
    names[WAVE_FORMAT_PCM] = "PCM";
    names[WAVE_FORMAT_EXTENSIBLE] = "WAVE_FORMAT_EXTENSIBLE";
    names[WAVE_FORMAT_IEEE_FLOAT] = "IEEE Float";
    names[WAVE_FORMAT_ADPCM] = "MS ADPCM";
    names[WAVE_FORMAT_ALAW] = "aLaw";
    names[WAVE_FORMAT_MULAW] = "muLaw";
    names[WAVE_FORMAT_DRM] = "DRM";
    names[WAVE_FORMAT_OKI_ADPCM] = "OKI ADPCM";
    names[WAVE_FORMAT_DVI_ADPCM] = "DVI ADPCM";
    names[WAVE_FORMAT_IMA_ADPCM] = "IMA ADPCM";
    names[WAVE_FORMAT_MEDIASPACE_ADPCM] = "Mediaspace ADPCM";
    names[WAVE_FORMAT_SIERRA_ADPCM] = "Sierra ADPCM";
    names[WAVE_FORMAT_G723_ADPCM] = "G723 ADPCM";
    names[WAVE_FORMAT_DIALOGIC_OKI_ADPCM] = "Dialogic OKI ADPCM";
    names[WAVE_FORMAT_MEDIAVISION_ADPCM] = "Media Vision ADPCM";
    names[WAVE_FORMAT_YAMAHA_ADPCM] = "Yamaha ADPCM";
    names[WAVE_FORMAT_DSPGROUP_TRUESPEECH] = "DSP Group Truespeech";
    names[WAVE_FORMAT_DOLBY_AC2] = "Dolby AC2|ac2";
    names[WAVE_FORMAT_GSM610] = "GSM610";
    names[WAVE_FORMAT_MSNAUDIO] = "MSN Audio";
    names[WAVE_FORMAT_ANTEX_ADPCME] = "Antex ADPCME";
    names[WAVE_FORMAT_CS_IMAADPCM] = "Crystal Semiconductor IMA ADPCM";
    names[WAVE_FORMAT_ROCKWELL_ADPCM] = "Rockwell ADPCM";
    names[WAVE_FORMAT_ROCKWELL_DIGITALK] = "Rockwell Digitalk";
    names[WAVE_FORMAT_G721_ADPCM] = "G721";
    names[WAVE_FORMAT_G728_CELP] = "G728";
    names[WAVE_FORMAT_MSG723] = "MSG723";
    names[WAVE_FORMAT_MPEG] = "MPEG Audio|mpeg";
    names[WAVE_FORMAT_MPEGLAYER3] = "MPEG Audio Layer 3|mp3";
    names[WAVE_FORMAT_LUCENT_G723] = "Lucent G723";
    names[WAVE_FORMAT_VOXWARE] = "Voxware";
    names[WAVE_FORMAT_G726_ADPCM] = "G726";
    names[WAVE_FORMAT_G722_ADPCM] = "G722";
    names[WAVE_FORMAT_G729A] = "G729A";
    names[WAVE_FORMAT_MEDIASONIC_G723] = "MediaSonic G723";
    names[WAVE_FORMAT_ZYXEL_ADPCM] = "ZyXEL ADPCM";
    names[WAVE_FORMAT_RHETOREX_ADPCM] = "Rhetorex ADPCM";
    names[WAVE_FORMAT_VIVO_G723] = "Vivo G723";
    names[WAVE_FORMAT_VIVO_SIREN] = "Vivo Siren";
    names[WAVE_FORMAT_DIGITAL_G723] = "Digital G723";
    names[WAVE_FORMAT_SANYO_LD_ADPCM] = "Sanyo LD ADPCM";
    names[WAVE_FORMAT_CREATIVE_ADPCM] = "Creative ADPCM";
    names[WAVE_FORMAT_CREATIVE_FASTSPEECH8] = "Creative Fastspeech 8";
    names[WAVE_FORMAT_CREATIVE_FASTSPEECH10] = "Creative Fastspeech 10";
    names[WAVE_FORMAT_UHER_ADPCM] = "UHER ADPCM";
    names[WAVE_FORMAT_DOLBY_AC3] = "Dolby AC3|ac3";
    names[WAVE_FORMAT_DVD_DTS] = "DTS|dca";
    names[WAVE_FORMAT_AAC] = "AAC";
    names[WAVE_FORMAT_FLAC] = "FLAC";
    names[WAVE_FORMAT_TTA1] = "TTA";
    names[WAVE_FORMAT_14_4] = "RealAudio 14.4";
    names[WAVE_FORMAT_28_8] = "RealAudio 28.8";
    names[WAVE_FORMAT_ATRC] = "RealAudio ATRC";
    names[WAVE_FORMAT_COOK] = "RealAudio COOK";
    names[WAVE_FORMAT_DNET] = "RealAudio DNET";
    names[WAVE_FORMAT_RAAC] = "RealAudio RAAC";
    names[WAVE_FORMAT_RACP] = "RealAudio RACP";
    names[WAVE_FORMAT_SIPR] = "RealAudio SIPR";
    names[WAVE_FORMAT_PS2_PCM] = "PS2 PCM";
    names[WAVE_FORMAT_PS2_ADPCM] = "PS2 ADPCM";
    names[0x0160] = "Windows Media Audio|wma";
    names[0x0161] = "Windows Media Audio|wma";
    names[0x0162] = "Windows Media Audio|wma";
    names[0x0163] = "Windows Media Audio|wma";
  }

  std::map<WORD, CStdStringA>::iterator it = names.find(wFormatTag);
  if(it == names.end())
  {
    if(subtype == MEDIASUBTYPE_Vorbis) str = "Vorbis (deprecated)";
    else if(subtype == MEDIASUBTYPE_Vorbis2) str = "Vorbis";
    else if(subtype == MEDIASUBTYPE_MP4A) str = "MPEG4 Audio|mp4";
    else if(subtype == MEDIASUBTYPE_FLAC_FRAMED) str = "FLAC (framed)|flac";
    else if(subtype == MEDIASUBTYPE_DOLBY_AC3) str += "Dolby AC3|ac3";
    else if(subtype == MEDIASUBTYPE_DTS) str += "DTS|dca";
    else str.Format("0x%04x"), wFormatTag;
  } else {
    str = it->second;
  }

  if(wFormatTag == WAVE_FORMAT_PCM)
  {
    if(subtype == MEDIASUBTYPE_DOLBY_AC3) str += " (AC3)|ac3";
    else if(subtype == MEDIASUBTYPE_DTS) str += " (DTS)|dca";
  }

  return str;
}

CStdStringA CMediaTypeEx::GetSubtitleCodecName(const GUID& subtype)
{
  CStdStringA str;

/*  static CAtlMap<GUID, CStdString> names;

  if(names.IsEmpty())
  {
    names[MEDIASUBTYPE_UTF8] = "UTF-8";
    names[MEDIASUBTYPE_SSA] = "SubStation Alpha";
    names[MEDIASUBTYPE_ASS] = "Advanced SubStation Alpha";
    names[MEDIASUBTYPE_ASS2] = "Advanced SubStation Alpha";
    names[MEDIASUBTYPE_SSF] = "Stuctured Subtitle Format";
    names[MEDIASUBTYPE_USF] = "Universal Subtitle Format";
    names[MEDIASUBTYPE_VOBSUB] = "VobSub";
    // names[''] = "";
  }

  if(names.Lookup(subtype, str))
  {

  }
*/
  return str;
}

/*void CMediaTypeEx::Dump(CAtlList<CStdString>& sl)
{
  CStdString str;

  sl.RemoveAll();

  int fmtsize = 0;

  CStdString major = CStringFromGUID(majortype);
  CStdString sub = CStringFromGUID(subtype);
  CStdString format = CStringFromGUID(formattype);

  sl.AddTail(ToString() + "\n");  

  sl.AddTail("AM_MEDIA_TYPE: ");
  str.Format("majortype: %s %s"), CStdString(GuidNames[majortype]), major;
  sl.AddTail(str);
  str.Format("subtype: %s %s"), CStdString(GuidNames[subtype]), sub;
  sl.AddTail(str);
  str.Format("formattype: %s %s"), CStdString(GuidNames[formattype]), format;
  sl.AddTail(str);
  str.Format("bFixedSizeSamples: %d"), bFixedSizeSamples;
  sl.AddTail(str);
  str.Format("bTemporalCompression: %d"), bTemporalCompression;
  sl.AddTail(str);
  str.Format("lSampleSize: %d"), lSampleSize;
  sl.AddTail(str);
  str.Format("cbFormat: %d"), cbFormat;
  sl.AddTail(str);

  sl.AddTail("");

  if(formattype == FORMAT_VideoInfo || formattype == FORMAT_VideoInfo2
  || formattype == FORMAT_MPEGVideo || formattype == FORMAT_MPEG2_VIDEO)
  {
    fmtsize = 
      formattype == FORMAT_VideoInfo ? sizeof(VIDEOINFOHEADER) :
      formattype == FORMAT_VideoInfo2 ? sizeof(VIDEOINFOHEADER2) :
      formattype == FORMAT_MPEGVideo ? sizeof(MPEG1VIDEOINFO)-1 :
      formattype == FORMAT_MPEG2_VIDEO ? sizeof(MPEG2VIDEOINFO)-4 :
      0;

    VIDEOINFOHEADER& vih = *(VIDEOINFOHEADER*)pbFormat;
    BITMAPINFOHEADER* bih = &vih.bmiHeader;

    sl.AddTail("VIDEOINFOHEADER:");
    str.Format("rcSource: (%d,%d)-(%d,%d)"), vih.rcSource.left, vih.rcSource.top, vih.rcSource.right, vih.rcSource.bottom;
    sl.AddTail(str);
    str.Format("rcTarget: (%d,%d)-(%d,%d)"), vih.rcTarget.left, vih.rcTarget.top, vih.rcTarget.right, vih.rcTarget.bottom;
    sl.AddTail(str);
    str.Format("dwBitRate: %d"), vih.dwBitRate;
    sl.AddTail(str);
    str.Format("dwBitErrorRate: %d"), vih.dwBitErrorRate;
    sl.AddTail(str);
    str.Format("AvgTimePerFrame: %I64d"), vih.AvgTimePerFrame;
    sl.AddTail(str);

    sl.AddTail("");

    if(formattype == FORMAT_VideoInfo2 || formattype == FORMAT_MPEG2_VIDEO)
    {
      VIDEOINFOHEADER2& vih = *(VIDEOINFOHEADER2*)pbFormat;
      bih = &vih.bmiHeader;

      sl.AddTail("VIDEOINFOHEADER2:");
      str.Format("dwInterlaceFlags: 0x%08x"), vih.dwInterlaceFlags;
      sl.AddTail(str);
      str.Format("dwCopyProtectFlags: 0x%08x"), vih.dwCopyProtectFlags;
      sl.AddTail(str);
      str.Format("dwPictAspectRatioX: %d"), vih.dwPictAspectRatioX;
      sl.AddTail(str);
      str.Format("dwPictAspectRatioY: %d"), vih.dwPictAspectRatioY;
      sl.AddTail(str);
      str.Format("dwControlFlags: 0x%08x"), vih.dwControlFlags;
      sl.AddTail(str);
      str.Format("dwReserved2: 0x%08x"), vih.dwReserved2;
      sl.AddTail(str);

      sl.AddTail("");
    }

    if(formattype == FORMAT_MPEGVideo)
    {
      MPEG1VIDEOINFO& mvih = *(MPEG1VIDEOINFO*)pbFormat;

      sl.AddTail("MPEG1VIDEOINFO:");
      str.Format("dwStartTimeCode: %d"), mvih.dwStartTimeCode;
      sl.AddTail(str);
      str.Format("cbSequenceHeader: %d"), mvih.cbSequenceHeader;
      sl.AddTail(str);

      sl.AddTail("");
    }
    else if(formattype == FORMAT_MPEG2_VIDEO)
    {
      MPEG2VIDEOINFO& mvih = *(MPEG2VIDEOINFO*)pbFormat;

      sl.AddTail("MPEG2VIDEOINFO:");
      str.Format("dwStartTimeCode: %d"), mvih.dwStartTimeCode;
      sl.AddTail(str);
      str.Format("cbSequenceHeader: %d"), mvih.cbSequenceHeader;
      sl.AddTail(str);
      str.Format("dwProfile: 0x%08x"), mvih.dwProfile;
      sl.AddTail(str);
      str.Format("dwLevel: 0x%08x"), mvih.dwLevel;
      sl.AddTail(str);
      str.Format("dwFlags: 0x%08x"), mvih.dwFlags;
      sl.AddTail(str);

      sl.AddTail("");
    }

    sl.AddTail("BITMAPINFOHEADER:");
    str.Format("biSize: %d"), bih->biSize;
    sl.AddTail(str);
    str.Format("biWidth: %d"), bih->biWidth;
    sl.AddTail(str);
    str.Format("biHeight: %d"), bih->biHeight;
    sl.AddTail(str);
    str.Format("biPlanes: %d"), bih->biPlanes;
    sl.AddTail(str);
    str.Format("biBitCount: %d"), bih->biBitCount;
    sl.AddTail(str);
    if(bih->biCompression < 256) str.Format("biCompression: %d"), bih->biCompression;
    else str.Format("biCompression: %4.4hs"), &bih->biCompression;
    sl.AddTail(str);
    str.Format("biSizeImage: %d"), bih->biSizeImage;
    sl.AddTail(str);
    str.Format("biXPelsPerMeter: %d"), bih->biXPelsPerMeter;
    sl.AddTail(str);
    str.Format("biYPelsPerMeter: %d"), bih->biYPelsPerMeter;
    sl.AddTail(str);
    str.Format("biClrUsed: %d"), bih->biClrUsed;
    sl.AddTail(str);
    str.Format("biClrImportant: %d"), bih->biClrImportant;
    sl.AddTail(str);

    sl.AddTail("");
    }
  else if(formattype == FORMAT_WaveFormatEx)
  {
    fmtsize = sizeof(WAVEFORMATEX);

    WAVEFORMATEX& wfe = *(WAVEFORMATEX*)pbFormat;

    sl.AddTail("WAVEFORMATEX:");
    str.Format("wFormatTag: 0x%04x"), wfe.wFormatTag;
    sl.AddTail(str);
    str.Format("nChannels: %d"), wfe.nChannels;
    sl.AddTail(str);
    str.Format("nSamplesPerSec: %d"), wfe.nSamplesPerSec;
    sl.AddTail(str);
    str.Format("nAvgBytesPerSec: %d"), wfe.nAvgBytesPerSec;
    sl.AddTail(str);
    str.Format("nBlockAlign: %d"), wfe.nBlockAlign;
    sl.AddTail(str);
    str.Format("wBitsPerSample: %d"), wfe.wBitsPerSample;
    sl.AddTail(str);
    str.Format("cbSize: %d (extra bytes)"), wfe.cbSize;
    sl.AddTail(str);

    sl.AddTail("");

    if(wfe.wFormatTag != WAVE_FORMAT_PCM && wfe.cbSize > 0)
    {
      if(wfe.wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe.cbSize == sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX))
      {
        fmtsize = sizeof(WAVEFORMATEXTENSIBLE);

        WAVEFORMATEXTENSIBLE& wfe = *(WAVEFORMATEXTENSIBLE*)pbFormat;

        sl.AddTail("WAVEFORMATEXTENSIBLE:");
        if(wfe.Format.wBitsPerSample != 0) str.Format("wValidBitsPerSample: %d"), wfe.Samples.wValidBitsPerSample;
        else str.Format("wSamplesPerBlock: %d"), wfe.Samples.wSamplesPerBlock;
        sl.AddTail(str);
        str.Format("dwChannelMask: 0x%08x"), wfe.dwChannelMask;
        sl.AddTail(str);
        str.Format("SubFormat: %s"), CStringFromGUID(wfe.SubFormat);
        sl.AddTail(str);

        sl.AddTail("");
      }
      else if(wfe.wFormatTag == WAVE_FORMAT_DOLBY_AC3 && wfe.cbSize == sizeof(DOLBYAC3WAVEFORMAT)-sizeof(WAVEFORMATEX))
      {
        fmtsize = sizeof(DOLBYAC3WAVEFORMAT);

        DOLBYAC3WAVEFORMAT& wfe = *(DOLBYAC3WAVEFORMAT*)pbFormat;

        sl.AddTail("DOLBYAC3WAVEFORMAT:");
        str.Format("bBigEndian: %d"), wfe.bBigEndian;
        sl.AddTail(str);
        str.Format("bsid: %d"), wfe.bsid;
        sl.AddTail(str);
        str.Format("lfeon: %d"), wfe.lfeon;
        sl.AddTail(str);
        str.Format("copyrightb: %d"), wfe.copyrightb;
        sl.AddTail(str);
        str.Format("nAuxBitsCode: %d"), wfe.nAuxBitsCode;
        sl.AddTail(str);

        sl.AddTail("");
      }
    }
  }
  else if(formattype == FORMAT_VorbisFormat)
  {
    fmtsize = sizeof(VORBISFORMAT);

    VORBISFORMAT& vf = *(VORBISFORMAT*)pbFormat;

    sl.AddTail("VORBISFORMAT:");
    str.Format("nChannels: %d"), vf.nChannels;
    sl.AddTail(str);
    str.Format("nSamplesPerSec: %d"), vf.nSamplesPerSec;
    sl.AddTail(str);
    str.Format("nMinBitsPerSec: %d"), vf.nMinBitsPerSec;
    sl.AddTail(str);
    str.Format("nAvgBitsPerSec: %d"), vf.nAvgBitsPerSec;
    sl.AddTail(str);
    str.Format("nMaxBitsPerSec: %d"), vf.nMaxBitsPerSec;
    sl.AddTail(str);
    str.Format("fQuality: %.3f"), vf.fQuality;
    sl.AddTail(str);

    sl.AddTail("");
  }
  else if(formattype == FORMAT_VorbisFormat2)
  {
    fmtsize = sizeof(VORBISFORMAT2);

    VORBISFORMAT2& vf = *(VORBISFORMAT2*)pbFormat;

    sl.AddTail("VORBISFORMAT:");
    str.Format("Channels: %d"), vf.Channels;
    sl.AddTail(str);
    str.Format("SamplesPerSec: %d"), vf.SamplesPerSec;
    sl.AddTail(str);
    str.Format("BitsPerSample: %d"), vf.BitsPerSample;
    sl.AddTail(str);
    str.Format("HeaderSize: {%d, %d, %d}"), vf.HeaderSize[0], vf.HeaderSize[1], vf.HeaderSize[2];
    sl.AddTail(str);

    sl.AddTail("");
  }
  else if(formattype == FORMAT_SubtitleInfo)
  {
    fmtsize = sizeof(SUBTITLEINFO);

    SUBTITLEINFO& si = *(SUBTITLEINFO*)pbFormat;

    sl.AddTail("SUBTITLEINFO:");
    str.Format("dwOffset: %d"), si.dwOffset;
    sl.AddTail(str);
    str.Format("IsoLang: %s"), CStdString(CStdStringA(si.IsoLang, sizeof(si.IsoLang)-1));
    sl.AddTail(str);
    str.Format("TrackName: %s"), CStdString(CStdStringW(si.TrackName, sizeof(si.TrackName)-1));
    sl.AddTail(str);

    sl.AddTail("");
  }

  if(cbFormat > 0)
  {
    sl.AddTail("pbFormat:");

    for(int i = 0, j = (cbFormat + 15) & ~15; i < j; i += 16)
    {
      str.Format("%04x:"), i;

      for(int k = i, l = min(i + 16, (int)cbFormat); k < l; k++)
      {
        CStdString byte;
        byte.Format("%c%02x"), fmtsize > 0 && fmtsize == k ? '|' : ' ', pbFormat[k];
        str += byte;
      }

      for(int k = min(i + 16, (int)cbFormat), l = i + 16; k < l; k++)
      {
        str += "   ";
      }

      str += ' ';

      for(int k = i, l = min(i + 16, (int)cbFormat); k < l; k++)
      {
        unsigned char c = (unsigned char)pbFormat[k];
        CStdStringA ch;
        ch.Format("%c", c >= 0x20 ? c : '.');
        str += ch;
      }

      sl.AddTail(str);
    }

    sl.AddTail("");
  }
}
*/