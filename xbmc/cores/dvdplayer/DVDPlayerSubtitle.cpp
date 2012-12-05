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

#include "DVDPlayerSubtitle.h"
#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDCodecs/Overlay/DVDOverlayCodec.h"
#include "DVDClock.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "DVDSubtitles/DVDSubtitleParser.h"
#include "DVDSubtitles/DVDSubtitleStream.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#ifdef _LINUX
#include "config.h"
#endif

using namespace std;

CDVDPlayerSubtitle::CDVDPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer)
{
  m_pOverlayContainer = pOverlayContainer;

  m_pSubtitleFileParser = NULL;
  m_pSubtitleStream = NULL;
  m_pOverlayCodec = NULL;
  m_lastPts = DVD_NOPTS_VALUE;
}

CDVDPlayerSubtitle::~CDVDPlayerSubtitle()
{
  CloseStream(false);
}


void CDVDPlayerSubtitle::Flush()
{
  SendMessage(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
}

void CDVDPlayerSubtitle::SendMessage(CDVDMsg* pMsg)
{
  if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
  {
    CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)pMsg;
    DemuxPacket* pPacket = pMsgDemuxerPacket->GetPacket();

    if (m_pOverlayCodec)
    {
      int result = m_pOverlayCodec->Decode(pPacket);

      if(result == OC_OVERLAY)
      {
        CDVDOverlay* overlay;

        while((overlay = m_pOverlayCodec->GetOverlay()) != NULL)
        {
          overlay->iGroupId = pPacket->iGroupId;
          m_pOverlayContainer->Add(overlay);
          overlay->Release();
        }
      }
    }
    else if (m_streaminfo.codec == CODEC_ID_DVD_SUBTITLE)
    {
      CDVDOverlaySpu* pSPUInfo = m_dvdspus.AddData(pPacket->pData, pPacket->iSize, pPacket->pts);
      if (pSPUInfo)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayer::ProcessSubData: Got complete SPU packet");
        pSPUInfo->iGroupId = pPacket->iGroupId;
        m_pOverlayContainer->Add(pSPUInfo);
        pSPUInfo->Release();
      }
    }

  }
  else if( pMsg->IsType(CDVDMsg::SUBTITLE_CLUTCHANGE) )
  {
    CDVDMsgSubtitleClutChange* pData = (CDVDMsgSubtitleClutChange*)pMsg;
    for (int i = 0; i < 16; i++)
    {
      BYTE* color = m_dvdspus.m_clut[i];
      BYTE* t = (BYTE*)pData->m_data[i];

// pData->m_data[i] points to an uint32_t
// Byte swapping is needed between big and little endian systems
#ifdef WORDS_BIGENDIAN
      color[0] = t[1]; // Y
      color[1] = t[2]; // Cr
      color[2] = t[3]; // Cb
#else
      color[0] = t[2]; // Y
      color[1] = t[0]; // Cr
      color[2] = t[1]; // Cb
#endif
    }
    m_dvdspus.m_bHasClut = true;
  }
  else if( pMsg->IsType(CDVDMsg::GENERAL_FLUSH)
        || pMsg->IsType(CDVDMsg::GENERAL_RESET) )
  {
    m_dvdspus.Reset();
    if (m_pSubtitleFileParser)
      m_pSubtitleFileParser->Reset();

    if (m_pOverlayCodec)
      m_pOverlayCodec->Flush();

    m_lastPts = DVD_NOPTS_VALUE;
  }

  pMsg->Release();
}

bool CDVDPlayerSubtitle::OpenStream(CDVDStreamInfo &hints, string &filename)
{
  CloseStream(false);
  m_streaminfo = hints;

  // okey check if this is a filesubtitle
  if(filename.size() && filename != "dvd" )
  {
    m_pSubtitleFileParser = CDVDFactorySubtitle::CreateParser(filename);
    if (!m_pSubtitleFileParser)
    {
      CLog::Log(LOGERROR, "%s - Unable to create subtitle parser", __FUNCTION__);
      CloseStream(false);
      return false;
    }

    if (!m_pSubtitleFileParser->Open(hints))
    {
      CLog::Log(LOGERROR, "%s - Unable to init subtitle parser", __FUNCTION__);
      CloseStream(false);
      return false;
    }
    m_pSubtitleFileParser->Reset();
    return true;
  }

  // dvd's use special subtitle decoder
  if(hints.codec == CODEC_ID_DVD_SUBTITLE && filename == "dvd")
    return true;

  m_pOverlayCodec = CDVDFactoryCodec::CreateOverlayCodec(hints);
  if(m_pOverlayCodec)
    return true;

  CLog::Log(LOGERROR, "%s - Unable to init overlay codec", __FUNCTION__);
  return false;
}

void CDVDPlayerSubtitle::CloseStream(bool flush)
{
  if(m_pSubtitleStream)
    SAFE_DELETE(m_pSubtitleStream);
  if(m_pSubtitleFileParser)
    SAFE_DELETE(m_pSubtitleFileParser);
  if(m_pOverlayCodec)
    SAFE_DELETE(m_pOverlayCodec);

  m_dvdspus.FlushCurrentPacket();

  if(flush)
    m_pOverlayContainer->Clear();
}

void CDVDPlayerSubtitle::Process(double pts)
{
  if (m_pSubtitleFileParser)
  {
    if(pts == DVD_NOPTS_VALUE)
      return;

    if (pts + DVD_SEC_TO_TIME(1) < m_lastPts)
    {
      m_pOverlayContainer->Clear();
      m_pSubtitleFileParser->Reset();
    }

    if(m_pOverlayContainer->GetSize() >= 5)
      return;

    CDVDOverlay* pOverlay = m_pSubtitleFileParser->Parse(pts);
    // add all overlays which fit the pts
    while(pOverlay)
    {
      m_pOverlayContainer->Add(pOverlay);
      pOverlay = m_pSubtitleFileParser->Parse(pts);
    }

    m_lastPts = pts;
  }
}

bool CDVDPlayerSubtitle::AcceptsData()
{
  // FIXME : This may still be causing problems + magic number :(
  return m_pOverlayContainer->GetSize() < 5;
}

void CDVDPlayerSubtitle::GetCurrentSubtitle(CStdString& strSubtitle, double pts)
{
  strSubtitle = "";

  Process(pts); // TODO: move to separate thread?

  CSingleLock lock(*m_pOverlayContainer);
  VecOverlays* pOverlays = m_pOverlayContainer->GetOverlays();
  if (pOverlays)
  {
    for(vector<CDVDOverlay*>::iterator it = pOverlays->begin();it != pOverlays->end();it++)
    {
      CDVDOverlay* pOverlay = *it;

      if (pOverlay->IsOverlayType(DVDOVERLAY_TYPE_TEXT)
      && (pOverlay->iPTSStartTime <= pts)
      && (pOverlay->iPTSStopTime >= pts || pOverlay->iPTSStopTime == 0LL))
      {
        CDVDOverlayText::CElement* e = ((CDVDOverlayText*)pOverlay)->m_pHead;
        while (e)
        {
          if (e->IsElementType(CDVDOverlayText::ELEMENT_TYPE_TEXT))
          {
            CDVDOverlayText::CElementText* t = (CDVDOverlayText::CElementText*)e;
            strSubtitle += t->m_text;
            strSubtitle += "\n";
          }
          e = e->pNext;
        }
      }
    }
  }
  strSubtitle.TrimRight('\n');
}
