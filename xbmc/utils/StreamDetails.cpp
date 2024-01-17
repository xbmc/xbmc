/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StreamDetails.h"

#include "LangInfo.h"
#include "StreamUtils.h"
#include "utils/Archive.h"
#include "utils/LangCodeExpander.h"
#include "utils/Variant.h"

#include <math.h>

const float VIDEOASPECT_EPSILON = 0.025f;

CStreamDetailVideo::CStreamDetailVideo() :
  CStreamDetail(CStreamDetail::VIDEO)
{
}

CStreamDetailVideo::CStreamDetailVideo(const VideoStreamInfo &info, int duration) :
  CStreamDetail(CStreamDetail::VIDEO),
  m_iWidth(info.width),
  m_iHeight(info.height),
  m_fAspect(info.videoAspectRatio),
  m_iDuration(duration),
  m_strCodec(info.codecName),
  m_strStereoMode(info.stereoMode),
  m_strLanguage(info.language),
  m_strHdrType(CStreamDetails::HdrTypeToString(info.hdrType))
{
}

void CStreamDetailVideo::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strCodec;
    ar << m_fAspect;
    ar << m_iHeight;
    ar << m_iWidth;
    ar << m_iDuration;
    ar << m_strStereoMode;
    ar << m_strLanguage;
    ar << m_strHdrType;
  }
  else
  {
    ar >> m_strCodec;
    ar >> m_fAspect;
    ar >> m_iHeight;
    ar >> m_iWidth;
    ar >> m_iDuration;
    ar >> m_strStereoMode;
    ar >> m_strLanguage;
    ar >> m_strHdrType;
  }
}
void CStreamDetailVideo::Serialize(CVariant& value) const
{
  value["codec"] = m_strCodec;
  value["aspect"] = m_fAspect;
  value["height"] = m_iHeight;
  value["width"] = m_iWidth;
  value["duration"] = m_iDuration;
  value["stereomode"] = m_strStereoMode;
  value["language"] = m_strLanguage;
  value["hdrtype"] = m_strHdrType;
}

bool CStreamDetailVideo::IsWorseThan(const CStreamDetail &that) const
{
  if (that.m_eType != CStreamDetail::VIDEO)
    return true;

  // Best video stream is that with the most pixels
  const auto& sdv = static_cast<const CStreamDetailVideo&>(that);
  return (sdv.m_iWidth * sdv.m_iHeight) > (m_iWidth * m_iHeight);
}

CStreamDetailAudio::CStreamDetailAudio() :
  CStreamDetail(CStreamDetail::AUDIO)
{
}

CStreamDetailAudio::CStreamDetailAudio(const AudioStreamInfo &info) :
  CStreamDetail(CStreamDetail::AUDIO),
  m_iChannels(info.channels),
  m_strCodec(info.codecName),
  m_strLanguage(info.language)
{
}

void CStreamDetailAudio::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strCodec;
    ar << m_strLanguage;
    ar << m_iChannels;
  }
  else
  {
    ar >> m_strCodec;
    ar >> m_strLanguage;
    ar >> m_iChannels;
  }
}
void CStreamDetailAudio::Serialize(CVariant& value) const
{
  value["codec"] = m_strCodec;
  value["language"] = m_strLanguage;
  value["channels"] = m_iChannels;
}

bool CStreamDetailAudio::IsWorseThan(const CStreamDetail &that) const
{
  if (that.m_eType != CStreamDetail::AUDIO)
    return true;

  const auto& sda = static_cast<const CStreamDetailAudio&>(that);
  // First choice is the thing with the most channels
  if (sda.m_iChannels > m_iChannels)
    return true;
  if (m_iChannels > sda.m_iChannels)
    return false;

  // In case of a tie, revert to codec priority
  return StreamUtils::GetCodecPriority(sda.m_strCodec) > StreamUtils::GetCodecPriority(m_strCodec);
}

CStreamDetailSubtitle::CStreamDetailSubtitle() :
  CStreamDetail(CStreamDetail::SUBTITLE)
{
}

CStreamDetailSubtitle::CStreamDetailSubtitle(const SubtitleStreamInfo &info) :
  CStreamDetail(CStreamDetail::SUBTITLE),
  m_strLanguage(info.language)
{
}

void CStreamDetailSubtitle::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strLanguage;
  }
  else
  {
    ar >> m_strLanguage;
  }
}
void CStreamDetailSubtitle::Serialize(CVariant& value) const
{
  value["language"] = m_strLanguage;
}

bool CStreamDetailSubtitle::IsWorseThan(const CStreamDetail &that) const
{
  if (that.m_eType != CStreamDetail::SUBTITLE)
    return true;

  if (g_LangCodeExpander.CompareISO639Codes(m_strLanguage, static_cast<const CStreamDetailSubtitle &>(that).m_strLanguage))
    return false;

  // the best subtitle should be the one in the user's preferred language
  // If preferred language is set to "original" this is "eng"
  return m_strLanguage.empty() ||
    g_LangCodeExpander.CompareISO639Codes(static_cast<const CStreamDetailSubtitle &>(that).m_strLanguage, g_langInfo.GetSubtitleLanguage());
}

CStreamDetailSubtitle& CStreamDetailSubtitle::operator=(const CStreamDetailSubtitle &that)
{
  if (this != &that)
  {
    this->m_pParent = that.m_pParent;
    this->m_strLanguage = that.m_strLanguage;
  }
  return *this;
}

CStreamDetails& CStreamDetails::operator=(const CStreamDetails &that)
{
  if (this != &that)
  {
    Reset();
    for (const auto &iter : that.m_vecItems)
    {
      switch (iter->m_eType)
      {
      case CStreamDetail::VIDEO:
        AddStream(new CStreamDetailVideo(static_cast<const CStreamDetailVideo&>(*iter)));
        break;
      case CStreamDetail::AUDIO:
        AddStream(new CStreamDetailAudio(static_cast<const CStreamDetailAudio&>(*iter)));
        break;
      case CStreamDetail::SUBTITLE:
        AddStream(new CStreamDetailSubtitle(static_cast<const CStreamDetailSubtitle&>(*iter)));
        break;
      }
    }

    DetermineBestStreams();
  }  /* if this != that */

  return *this;
}

bool CStreamDetails::operator ==(const CStreamDetails &right) const
{
  if (this == &right) return true;

  if (GetVideoStreamCount()    != right.GetVideoStreamCount() ||
      GetAudioStreamCount()    != right.GetAudioStreamCount() ||
      GetSubtitleStreamCount() != right.GetSubtitleStreamCount())
    return false;

  for (int iStream=1; iStream<=GetVideoStreamCount(); iStream++)
  {
    if (GetVideoCodec(iStream)    != right.GetVideoCodec(iStream)    ||
        GetVideoWidth(iStream)    != right.GetVideoWidth(iStream)    ||
        GetVideoHeight(iStream)   != right.GetVideoHeight(iStream)   ||
        GetVideoDuration(iStream) != right.GetVideoDuration(iStream) ||
        fabs(GetVideoAspect(iStream) - right.GetVideoAspect(iStream)) > VIDEOASPECT_EPSILON)
      return false;
  }

  for (int iStream=1; iStream<=GetAudioStreamCount(); iStream++)
  {
    if (GetAudioCodec(iStream)    != right.GetAudioCodec(iStream)    ||
        GetAudioLanguage(iStream) != right.GetAudioLanguage(iStream) ||
        GetAudioChannels(iStream) != right.GetAudioChannels(iStream) )
      return false;
  }

  for (int iStream=1; iStream<=GetSubtitleStreamCount(); iStream++)
  {
    if (GetSubtitleLanguage(iStream) != right.GetSubtitleLanguage(iStream) )
      return false;
  }

  return true;
}

bool CStreamDetails::operator !=(const CStreamDetails &right) const
{
  if (this == &right) return false;

  return !(*this == right);
}

CStreamDetail *CStreamDetails::NewStream(CStreamDetail::StreamType type)
{
  CStreamDetail *retVal = NULL;
  switch (type)
  {
  case CStreamDetail::VIDEO:
    retVal = new CStreamDetailVideo();
    break;
  case CStreamDetail::AUDIO:
    retVal = new CStreamDetailAudio();
    break;
  case CStreamDetail::SUBTITLE:
    retVal = new CStreamDetailSubtitle();
    break;
  }

  if (retVal)
    AddStream(retVal);

  return retVal;
}

std::string CStreamDetails::GetVideoLanguage(int idx) const
{
  const CStreamDetailVideo* item =
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx));
  if (item)
    return item->m_strLanguage;
  else
    return "";
}

int CStreamDetails::GetStreamCount(CStreamDetail::StreamType type) const
{
  int retVal = 0;
  for (const auto &iter : m_vecItems)
    if (iter->m_eType == type)
      retVal++;
  return retVal;
}

int CStreamDetails::GetVideoStreamCount(void) const
{
  return GetStreamCount(CStreamDetail::VIDEO);
}

int CStreamDetails::GetAudioStreamCount(void) const
{
  return GetStreamCount(CStreamDetail::AUDIO);
}

int CStreamDetails::GetSubtitleStreamCount(void) const
{
  return GetStreamCount(CStreamDetail::SUBTITLE);
}

CStreamDetails::CStreamDetails(const CStreamDetails &that)
{
  m_pBestVideo = nullptr;
  m_pBestAudio = nullptr;
  m_pBestSubtitle = nullptr;
  *this = that;
}

void CStreamDetails::AddStream(CStreamDetail *item)
{
  item->m_pParent = this;
  m_vecItems.emplace_back(item);
}

void CStreamDetails::Reset(void)
{
  m_pBestVideo = nullptr;
  m_pBestAudio = nullptr;
  m_pBestSubtitle = nullptr;

  m_vecItems.clear();
}

const CStreamDetail* CStreamDetails::GetNthStream(CStreamDetail::StreamType type, int idx) const
{
  if (idx == 0)
  {
    switch (type)
    {
    case CStreamDetail::VIDEO:
      return m_pBestVideo;
      break;
    case CStreamDetail::AUDIO:
      return m_pBestAudio;
      break;
    case CStreamDetail::SUBTITLE:
      return m_pBestSubtitle;
      break;
    default:
      return NULL;
      break;
    }
  }

  for (const auto &iter : m_vecItems)
    if (iter->m_eType == type)
    {
      idx--;
      if (idx < 1)
        return iter.get();
    }

  return NULL;
}

std::string CStreamDetails::GetVideoCodec(int idx) const
{
  const CStreamDetailVideo* item =
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx));
  if (item)
    return item->m_strCodec;
  else
    return "";
}

float CStreamDetails::GetVideoAspect(int idx) const
{
  const CStreamDetailVideo* item =
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx));
  if (item)
    return item->m_fAspect;
  else
    return 0.0;
}

int CStreamDetails::GetVideoWidth(int idx) const
{
  const CStreamDetailVideo* item =
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx));
  if (item)
    return item->m_iWidth;
  else
    return 0;
}

int CStreamDetails::GetVideoHeight(int idx) const
{
  const CStreamDetailVideo* item =
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx));
  if (item)
    return item->m_iHeight;
  else
    return 0;
}

std::string CStreamDetails::GetVideoHdrType( int idx) const
{
  const CStreamDetailVideo* item =
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx));
  if (item)
    return item->m_strHdrType;
  else
    return "";
}

int CStreamDetails::GetVideoDuration(int idx) const
{
  const CStreamDetailVideo* item =
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx));
  if (item)
    return item->m_iDuration;
  else
    return 0;
}

void CStreamDetails::SetVideoDuration(int idx, const int duration)
{
  CStreamDetailVideo* item = const_cast<CStreamDetailVideo*>(
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx)));
  if (item)
    item->m_iDuration = duration;
}

std::string CStreamDetails::GetStereoMode(int idx) const
{
  const CStreamDetailVideo* item =
      dynamic_cast<const CStreamDetailVideo*>(GetNthStream(CStreamDetail::VIDEO, idx));
  if (item)
    return item->m_strStereoMode;
  else
    return "";
}

std::string CStreamDetails::GetAudioCodec(int idx) const
{
  const CStreamDetailAudio* item =
      dynamic_cast<const CStreamDetailAudio*>(GetNthStream(CStreamDetail::AUDIO, idx));
  if (item)
    return item->m_strCodec;
  else
    return "";
}

std::string CStreamDetails::GetAudioLanguage(int idx) const
{
  const CStreamDetailAudio* item =
      dynamic_cast<const CStreamDetailAudio*>(GetNthStream(CStreamDetail::AUDIO, idx));
  if (item)
    return item->m_strLanguage;
  else
    return "";
}

int CStreamDetails::GetAudioChannels(int idx) const
{
  const CStreamDetailAudio* item =
      dynamic_cast<const CStreamDetailAudio*>(GetNthStream(CStreamDetail::AUDIO, idx));
  if (item)
    return item->m_iChannels;
  else
    return -1;
}

std::string CStreamDetails::GetSubtitleLanguage(int idx) const
{
  const CStreamDetailSubtitle* item =
      dynamic_cast<const CStreamDetailSubtitle*>(GetNthStream(CStreamDetail::SUBTITLE, idx));
  if (item)
    return item->m_strLanguage;
  else
    return "";
}

void CStreamDetails::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << (int)m_vecItems.size();

    for (auto &iter : m_vecItems)
    {
      // the type goes before the actual item.  When loading we need
      // to know the type before we can construct an instance to serialize
      ar << (int)iter->m_eType;
      ar << (*iter);
    }
  }
  else
  {
    int count;
    ar >> count;

    Reset();
    for (int i=0; i<count; i++)
    {
      int type;
      CStreamDetail *p = NULL;

      ar >> type;
      p = NewStream(CStreamDetail::StreamType(type));
      if (p)
        ar >> (*p);
    }

    DetermineBestStreams();
  }
}
void CStreamDetails::Serialize(CVariant& value) const
{
  // make sure these properties are always present
  value["audio"] = CVariant(CVariant::VariantTypeArray);
  value["video"] = CVariant(CVariant::VariantTypeArray);
  value["subtitle"] = CVariant(CVariant::VariantTypeArray);

  CVariant v;
  for (const auto &iter : m_vecItems)
  {
    v.clear();
    iter->Serialize(v);
    switch (iter->m_eType)
    {
    case CStreamDetail::AUDIO:
      value["audio"].push_back(v);
      break;
    case CStreamDetail::VIDEO:
      value["video"].push_back(v);
      break;
    case CStreamDetail::SUBTITLE:
      value["subtitle"].push_back(v);
      break;
    }
  }
}

void CStreamDetails::DetermineBestStreams(void)
{
  m_pBestVideo = NULL;
  m_pBestAudio = NULL;
  m_pBestSubtitle = NULL;

  for (const auto &iter : m_vecItems)
  {
    const CStreamDetail **champion;
    switch (iter->m_eType)
    {
    case CStreamDetail::VIDEO:
      champion = (const CStreamDetail **)&m_pBestVideo;
      break;
    case CStreamDetail::AUDIO:
      champion = (const CStreamDetail **)&m_pBestAudio;
      break;
    case CStreamDetail::SUBTITLE:
      champion = (const CStreamDetail **)&m_pBestSubtitle;
      break;
    default:
      champion = NULL;
    }  /* switch type */

    if (!champion)
      continue;

    if ((*champion == NULL) || (*champion)->IsWorseThan(*iter))
      *champion = iter.get();
  }  /* for each */
}

std::string CStreamDetails::VideoDimsToResolutionDescription(int iWidth, int iHeight)
{
  if (iWidth == 0 || iHeight == 0)
    return "";

  // Anamorphic NTSC DVD
  else if (iWidth <= 854 && iHeight <= 480)
    return "480";
  // 960x540 (sometimes 544 which is multiple of 16)
  else if (iWidth <= 960 && iHeight <= 544)
    return "540";
  // includes 768x576, 720x576 and 1024x576
  else if (iWidth <= 1024 && iHeight <= 576)
    return "576";
  // 1280x720
  else if (iWidth <= 1280 && iHeight <= 962)
    return "720";
  // 1920x1080
  else if (iWidth <= 1920 && iHeight <= 1440)
    return "1080";
  // 4K
  else if (iWidth <= 4096 && iHeight <= 3072)
    return "4K";
  // 8K
  else if (iWidth <= 8192 && iHeight <= 6144)
    return "8K";
  else
    return "";
}

std::string CStreamDetails::VideoAspectToAspectDescription(float fAspect)
{
  if (fAspect == 0.0f)
    return "";

  // Given that we're never going to be able to handle every single possibility in
  // aspect ratios, particularly when cropping prior to video encoding is taken into account
  // the best we can do is take the "common" aspect ratios, and return the closest one available.
  // The cutoffs are the geometric mean of the two aspect ratios either side.
  if (fAspect < 1.0909f) // sqrt(1.00*1.19)
    return "1.00";
  else if (fAspect < 1.2581f) // sqrt(1.19*1.33)
    return "1.19";
  else if (fAspect < 1.3499f) // sqrt(1.33*1.37)
    return "1.33";
  else if (fAspect < 1.5080f) // sqrt(1.37*1.66)
    return "1.37";
  else if (fAspect < 1.7190f) // sqrt(1.66*1.78)
    return "1.66";
  else if (fAspect < 1.8147f) // sqrt(1.78*1.85)
    return "1.78";
  else if (fAspect < 1.9235f) // sqrt(1.85*2.00)
    return "1.85";
  else if (fAspect < 2.0976f) // sqrt(2.00*2.20)
    return "2.00";
  else if (fAspect < 2.2738f) // sqrt(2.20*2.35)
    return "2.20";
  else if (fAspect < 2.3749f) // sqrt(2.35*2.40)
    return "2.35";
  else if (fAspect < 2.4739f) // sqrt(2.40*2.55)
    return "2.40";
  else if (fAspect < 2.6529f) // sqrt(2.55*2.76)
    return "2.55";
  return "2.76";
}

bool CStreamDetails::SetStreams(const VideoStreamInfo& videoInfo, int videoDuration, const AudioStreamInfo& audioInfo, const SubtitleStreamInfo& subtitleInfo)
{
  if (!videoInfo.valid && !audioInfo.valid && !subtitleInfo.valid)
    return false;
  Reset();
  if (videoInfo.valid)
    AddStream(new CStreamDetailVideo(videoInfo, videoDuration));
  if (audioInfo.valid)
    AddStream(new CStreamDetailAudio(audioInfo));
  if (subtitleInfo.valid)
    AddStream(new CStreamDetailSubtitle(subtitleInfo));
  DetermineBestStreams();
  return true;
}

std::string CStreamDetails::HdrTypeToString(StreamHdrType hdrType)
{
  switch (hdrType)
  {
    case StreamHdrType::HDR_TYPE_DOLBYVISION:
      return "dolbyvision";
    case StreamHdrType::HDR_TYPE_HDR10:
      return "hdr10";
    case StreamHdrType::HDR_TYPE_HLG:
      return "hlg";
    case StreamHdrType::HDR_TYPE_NONE:
    default:
      return "";
  }
}
