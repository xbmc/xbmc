/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <math.h>
#include "StreamDetails.h"
#include "StreamUtils.h"
#include "Variant.h"

void CStreamDetail::Archive(CArchive &ar)
{
  // there's nothing to do here, the type is stored externally and parent isn't stored
}
void CStreamDetail::Serialize(CVariant &value) const
{
  // there's nothing to do here, the type is stored externally and parent isn't stored
}

CStreamDetailVideo::CStreamDetailVideo() :
  CStreamDetail(CStreamDetail::VIDEO), m_iWidth(0), m_iHeight(0), m_fAspect(0.0), m_iDuration(0)
{
}

void CStreamDetailVideo::Archive(CArchive& ar)
{
  CStreamDetail::Archive(ar);
  if (ar.IsStoring())
  {
    ar << m_strCodec;
    ar << m_fAspect;
    ar << m_iHeight;
    ar << m_iWidth;
    ar << m_iDuration;
  }
  else
  {
    ar >> m_strCodec;
    ar >> m_fAspect;
    ar >> m_iHeight;
    ar >> m_iWidth;
    ar >> m_iDuration;
  }
}
void CStreamDetailVideo::Serialize(CVariant& value) const
{
  value["codec"] = m_strCodec;
  value["aspect"] = m_fAspect;
  value["height"] = m_iHeight;
  value["width"] = m_iWidth;
  value["duration"] = m_iDuration;
}

bool CStreamDetailVideo::IsWorseThan(CStreamDetail *that)
{
  if (that->m_eType != CStreamDetail::VIDEO)
    return true;

  // Best video stream is that with the most pixels
  CStreamDetailVideo *sdv = (CStreamDetailVideo *)that;
  return (sdv->m_iWidth * sdv->m_iHeight) > (m_iWidth * m_iHeight);
}

CStreamDetailAudio::CStreamDetailAudio() :
  CStreamDetail(CStreamDetail::AUDIO), m_iChannels(-1)
{
}

void CStreamDetailAudio::Archive(CArchive& ar)
{
  CStreamDetail::Archive(ar);
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

bool CStreamDetailAudio::IsWorseThan(CStreamDetail *that)
{
  if (that->m_eType != CStreamDetail::AUDIO)
    return true;

  CStreamDetailAudio *sda = (CStreamDetailAudio *)that;
  // First choice is the thing with the most channels
  if (sda->m_iChannels > m_iChannels)
    return true;
  if (m_iChannels > sda->m_iChannels)
    return false;

  // In case of a tie, revert to codec priority
  return StreamUtils::GetCodecPriority(sda->m_strCodec) > StreamUtils::GetCodecPriority(m_strCodec);
}

CStreamDetailSubtitle::CStreamDetailSubtitle() :
  CStreamDetail(CStreamDetail::SUBTITLE)
{
}

void CStreamDetailSubtitle::Archive(CArchive& ar)
{
  CStreamDetail::Archive(ar);
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

bool CStreamDetailSubtitle::IsWorseThan(CStreamDetail *that)
{
  if (that->m_eType != CStreamDetail::SUBTITLE)
    return true;

  // the preferred subtitle should be the one in the user's language
  if (m_pParent)
  {
    if (m_pParent->m_strLanguage == m_strLanguage)
      return false;  // already the best
    else
      return (m_pParent->m_strLanguage == ((CStreamDetailSubtitle *)that)->m_strLanguage);
  }
  return false;
}

CStreamDetails& CStreamDetails::operator=(const CStreamDetails &that)
{
  if (this != &that)
  {
    Reset();
    std::vector<CStreamDetail *>::const_iterator iter;
    for (iter = that.m_vecItems.begin(); iter != that.m_vecItems.end(); iter++)
    {
      switch ((*iter)->m_eType)
      {
      case CStreamDetail::VIDEO:
        AddStream(new CStreamDetailVideo((const CStreamDetailVideo &)(**iter)));
        break;
      case CStreamDetail::AUDIO:
        AddStream(new CStreamDetailAudio((const CStreamDetailAudio &)(**iter)));
        break;
      case CStreamDetail::SUBTITLE:
        AddStream(new CStreamDetailSubtitle((const CStreamDetailSubtitle &)(**iter)));
        break;
      }
    }

    DetermineBestStreams();
  }  /* if this != that */

  return *this;
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

int CStreamDetails::GetStreamCount(CStreamDetail::StreamType type) const
{
  int retVal = 0;
  std::vector<CStreamDetail *>::const_iterator iter;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
    if ((*iter)->m_eType == type)
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
  *this = that;
}

void CStreamDetails::AddStream(CStreamDetail *item)
{
  item->m_pParent = this;
  m_vecItems.push_back(item);
}

void CStreamDetails::Reset(void)
{
  m_pBestVideo = NULL;
  m_pBestAudio = NULL;
  m_pBestSubtitle = NULL;

  std::vector<CStreamDetail *>::iterator iter;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
    delete *iter;
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

  std::vector<CStreamDetail *>::const_iterator iter;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
    if ((*iter)->m_eType == type)
    {
      idx--;
      if (idx < 1)
        return *iter;
    }

  return NULL;
}

CStdString CStreamDetails::GetVideoCodec(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_strCodec;
  else
    return "";
}

float CStreamDetails::GetVideoAspect(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_fAspect;
  else
    return 0.0;
}

int CStreamDetails::GetVideoWidth(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_iWidth;
  else
    return 0;
}

int CStreamDetails::GetVideoHeight(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_iHeight;
  else
    return 0;
}

int CStreamDetails::GetVideoDuration(int idx) const
{
  CStreamDetailVideo *item = (CStreamDetailVideo *)GetNthStream(CStreamDetail::VIDEO, idx);
  if (item)
    return item->m_iDuration;
  else
    return 0;
}

CStdString CStreamDetails::GetAudioCodec(int idx) const
{
  CStreamDetailAudio *item = (CStreamDetailAudio *)GetNthStream(CStreamDetail::AUDIO, idx);
  if (item)
    return item->m_strCodec;
  else
    return "";
}

CStdString CStreamDetails::GetAudioLanguage(int idx) const
{
  CStreamDetailAudio *item = (CStreamDetailAudio *)GetNthStream(CStreamDetail::AUDIO, idx);
  if (item)
    return item->m_strLanguage;
  else
    return "";
}

int CStreamDetails::GetAudioChannels(int idx) const
{
  CStreamDetailAudio *item = (CStreamDetailAudio *)GetNthStream(CStreamDetail::AUDIO, idx);
  if (item)
    return item->m_iChannels;
  else
    return -1;
}

CStdString CStreamDetails::GetSubtitleLanguage(int idx) const
{
  CStreamDetailSubtitle *item = (CStreamDetailSubtitle *)GetNthStream(CStreamDetail::SUBTITLE, idx);
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

    std::vector<CStreamDetail *>::const_iterator iter;
    for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
    {
      // the type goes before the actual item.  When loading we need
      // to know the type before we can construct an instance to serialize
      ar << (int)(*iter)->m_eType;
      ar << (**iter);
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

  std::vector<CStreamDetail *>::const_iterator iter;
  CVariant v;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
  {
    v.clear();
    (*iter)->Serialize(v);
    switch ((*iter)->m_eType)
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

  std::vector<CStreamDetail *>::const_iterator iter;
  for (iter = m_vecItems.begin(); iter != m_vecItems.end(); iter++)
  {
    CStreamDetail **champion;
    switch ((*iter)->m_eType)
    {
    case CStreamDetail::VIDEO:
      champion = (CStreamDetail **)&m_pBestVideo;
      break;
    case CStreamDetail::AUDIO:
      champion = (CStreamDetail **)&m_pBestAudio;
      break;
    case CStreamDetail::SUBTITLE:
      champion = (CStreamDetail **)&m_pBestSubtitle;
      break;
    default:
      champion = NULL;
    }  /* switch type */

    if (!champion)
      continue;

    if ((*champion == NULL) || (*champion)->IsWorseThan(*iter))
      *champion = *iter;
  }  /* for each */
}

const float VIDEOASPECT_EPSILON = 0.025f;

CStdString CStreamDetails::VideoDimsToResolutionDescription(int iWidth, int iHeight)
{
  if (iWidth == 0 || iHeight == 0)
    return "";

  else if (iWidth <= 720 && iHeight <= 480)
    return "480";
  // 720x576 (PAL) (768 when rescaled for square pixels)
  else if (iWidth <= 768 && iHeight <= 576)
    return "576";
  // 960x540 (sometimes 544 which is multiple of 16)
  else if (iWidth <= 960 && iHeight <= 544)
    return "540";
  // 1280x720
  else if (iWidth <= 1280 && iHeight <= 720)
    return "720";
  // 1920x1080
  else
    return "1080";
}

CStdString CStreamDetails::VideoAspectToAspectDescription(float fAspect)
{
  if (fAspect == 0.0f)
    return "";

  // Given that we're never going to be able to handle every single possibility in
  // aspect ratios, particularly when cropping prior to video encoding is taken into account
  // the best we can do is take the "common" aspect ratios, and return the closest one available.
  // The cutoffs are the geometric mean of the two aspect ratios either side.
  if (fAspect < 1.4859f) // sqrt(1.33*1.66)
    return "1.33";
  else if (fAspect < 1.7190f) // sqrt(1.66*1.78)
    return "1.66";
  else if (fAspect < 1.8147f) // sqrt(1.78*1.85)
    return "1.78";
  else if (fAspect < 2.0174f) // sqrt(1.85*2.20)
    return "1.85";
  else if (fAspect < 2.2738f) // sqrt(2.20*2.35)
    return "2.20";
  return "2.35";
}
