/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "system.h"
#include "DVDOverlayCodecText.h"
#include "DVDOverlayText.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecs.h"
#include "utils/log.h"
#include "cores/dvdplayer/DVDSubtitles/DVDSubtitleTagSami.h"

CDVDOverlayCodecText::CDVDOverlayCodecText() : CDVDOverlayCodec("Text Subtitle Decoder")
{
  m_pOverlay = NULL;
  m_bIsSSA = false;
}

CDVDOverlayCodecText::~CDVDOverlayCodecText()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

bool CDVDOverlayCodecText::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  m_bIsSSA = (hints.codec == AV_CODEC_ID_SSA);
  if(hints.codec == AV_CODEC_ID_TEXT || hints.codec == AV_CODEC_ID_SSA)
    return true;
  if(hints.codec == AV_CODEC_ID_SUBRIP)
    return true;
  return false;
}

void CDVDOverlayCodecText::Dispose()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

int CDVDOverlayCodecText::Decode(DemuxPacket *pPacket)
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);

  if(!pPacket)
    return OC_ERROR;
  
  uint8_t *data = pPacket->pData;
  int      size = pPacket->iSize;
  
  m_pOverlay = new CDVDOverlayText();
  CDVDOverlayCodec::GetAbsoluteTimes(m_pOverlay->iPTSStartTime, m_pOverlay->iPTSStopTime, pPacket, m_pOverlay->replace);

  char *start, *end, *p;
  start = (char*)data;
  end   = (char*)data + size;
  p     = (char*)data;

  if (m_bIsSSA)
  {
    // currently just skip the prefixed ssa fields (8 fields)
    int nFieldCount = 8;
    while (nFieldCount > 0 && start < end)
    {
      if (*start == ',')
        nFieldCount--;

      start++;
      p++;
    }
  }

  CDVDSubtitleTagSami TagConv;
  bool Taginit = TagConv.Init();

  while(p<end)
  {
    if(*p == '{')
    {
      if(p>start)
      {
        if(Taginit)
          TagConv.ConvertLine(m_pOverlay, start, p-start);
        else
          m_pOverlay->AddElement(new CDVDOverlayText::CElementText(start, p-start));
      }
      start = p+1;

      while(*p != '}' && p<end)
        p++;

      char* override = (char*)malloc(p-start + 1);
      memcpy(override, start, p-start);
      override[p-start] = '\0';
      CLog::Log(LOGINFO, "%s - Skipped formatting tag %s", __FUNCTION__, override);
      free(override);

      start = p+1;
    }
    p++;
  }
  if(p>start)
  {
    if(Taginit)
    {
      TagConv.ConvertLine(m_pOverlay, start, p-start);
      TagConv.CloseTag(m_pOverlay);
    }
    else
      m_pOverlay->AddElement(new CDVDOverlayText::CElementText(start, p-start));
  }
  return OC_OVERLAY;
}

void CDVDOverlayCodecText::Reset()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

void CDVDOverlayCodecText::Flush()
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

CDVDOverlay* CDVDOverlayCodecText::GetOverlay()
{
  if(m_pOverlay)
  {
    CDVDOverlay* overlay = m_pOverlay;
    m_pOverlay = NULL;
    return overlay;
  }
  return NULL;
}
