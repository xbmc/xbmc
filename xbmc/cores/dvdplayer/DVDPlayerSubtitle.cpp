
#include "stdafx.h"
#include "DVDPlayerSubtitle.h"
#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDCodecs/Overlay/DVDOverlayCodecFFmpeg.h"
#include "DVDClock.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "DVDSubtitles/DVDSubtitleParser.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDDemuxers/DVDDemuxUtils.h"

CDVDPlayerSubtitle::CDVDPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer)
{
  m_pOverlayContainer = pOverlayContainer;
  
  m_pSubtitleFileParser = NULL;
  m_pSubtitleStream = NULL;
  m_pOverlayCodec = NULL;
}

CDVDPlayerSubtitle::~CDVDPlayerSubtitle()
{
  CloseStream(false);
}

  
void CDVDPlayerSubtitle::Flush()
{
  SendMessage(new CDVDMsgGeneralFlush());
}

void CDVDPlayerSubtitle::SendMessage(CDVDMsg* pMsg)
{
  if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
  {
    CDVDMsgDemuxerPacket* pMsgDemuxerPacket = (CDVDMsgDemuxerPacket*)pMsg;
    CDVDDemux::DemuxPacket* pPacket = pMsgDemuxerPacket->GetPacket();

    if (m_streaminfo.codec == CODEC_ID_DVD_SUBTITLE)
    {
      CSPUInfo* pSPUInfo = m_dvdspus.AddData(pPacket->pData, pPacket->iSize, pPacket->pts);
      if (pSPUInfo)
      {
        CLog::Log(LOGDEBUG, "CDVDPlayer::ProcessSubData: Got complete SPU packet");
        pSPUInfo->iGroupId = pPacket->iGroupId;
        m_pOverlayContainer->Add(pSPUInfo);
        pSPUInfo->Release();
      }
    } 
    else if (m_streaminfo.codec == CODEC_ID_DVB_SUBTITLE)
    {
      if(!m_pOverlayCodec)
      {
        pMsg->Release();
        return;
      }

      int result = m_pOverlayCodec->Decode(pPacket->pData, pPacket->iSize, pPacket->dts);

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
  }
  else if( pMsg->IsType(CDVDMsg::SUBTITLE_CLUTCHANGE) )
  {
    CDVDMsgSubtitleClutChange* pData = (CDVDMsgSubtitleClutChange*)pMsg;
    for (int i = 0; i < 16; i++)
    {
      BYTE* color = m_dvdspus.m_clut[i];
      BYTE* t = (BYTE*)pData->m_data[i];

      color[0] = t[2]; // Y
      color[1] = t[1]; // Cr
      color[2] = t[0]; // Cb
    }
    m_dvdspus.m_bHasClut = true;
  }
  else if( pMsg->IsType(CDVDMsg::GENERAL_FLUSH) )
  {
    m_dvdspus.Reset();
    if (m_pSubtitleFileParser) 
      m_pSubtitleFileParser->Reset();

    if (m_pOverlayCodec)
      m_pOverlayCodec->Flush();
  }

  pMsg->Release();
}

bool CDVDPlayerSubtitle::OpenStream(CDVDStreamInfo &hints, string &filename)
{
  CloseStream(false);
  m_streaminfo = hints;

  // okey check if this is a filesubtitle
  if(filename.size())
  {
    m_pSubtitleStream = new CDVDSubtitleStream();
    if (!m_pSubtitleStream)
    {      
      CLog::Log(LOGERROR, "%s - Unable to create subtitle stream", __FUNCTION__);
      CloseStream(false);
      return false;
    }
    m_pSubtitleFileParser = CDVDFactorySubtitle::CreateParser(m_pSubtitleStream, filename);
    if (!m_pSubtitleFileParser)
    {
      CLog::Log(LOGERROR, "% - Unable to create subtitle parser", __FUNCTION__);
      CloseStream(false);
      return false;
    }

    if (!m_pSubtitleFileParser->Init())
    {
      CLog::Log(LOGERROR, "% - Unable to init subtitle parser", __FUNCTION__);
      CloseStream(false);
      return false;
    }
  }

  if(hints.codec == CODEC_ID_DVB_SUBTITLE)
  {
    m_pOverlayCodec = new CDVDOverlayCodecFFmpeg();
    if(!m_pOverlayCodec->Open(hints, CDVDCodecOptions()))
    {
      CLog::Log(LOGERROR, "%s - Unable to init overlay codec", __FUNCTION__);
      CloseStream(false);
      return false;
    }
  }

  return true;
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

}

void CDVDPlayerSubtitle::Process(double pts)
{
  if (m_pSubtitleFileParser && m_pOverlayContainer->GetSize() < 5 && pts != DVD_NOPTS_VALUE)
  {
    CDVDOverlay* pOverlay = m_pSubtitleFileParser->Parse(pts);
    if (pOverlay)
      m_pOverlayContainer->Add(pOverlay);
  }
}

bool CDVDPlayerSubtitle::GetCurrentSubtitle(CStdString& strSubtitle, double pts)
{
  strSubtitle = "";
  
  Process(pts); // TODO: move to separate thread?

  m_pOverlayContainer->Lock();
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
  m_pOverlayContainer->Unlock();
  strSubtitle.TrimRight('\n');
  return !strSubtitle.IsEmpty();
}
