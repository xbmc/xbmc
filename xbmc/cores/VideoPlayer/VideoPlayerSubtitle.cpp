/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerSubtitle.h"

#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/Overlay/DVDOverlay.h"
#include "DVDCodecs/Overlay/DVDOverlayCodec.h"
#include "DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "DVDSubtitles/DVDSubtitleParser.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/log.h"

#include <mutex>

CVideoPlayerSubtitle::CVideoPlayerSubtitle(CDVDOverlayContainer* pOverlayContainer, CProcessInfo &processInfo)
: IDVDStreamPlayer(processInfo)
{
  m_pOverlayContainer = pOverlayContainer;
  m_lastPts = DVD_NOPTS_VALUE;
}

CVideoPlayerSubtitle::~CVideoPlayerSubtitle()
{
  CloseStream(true);
}


void CVideoPlayerSubtitle::Flush()
{
  SendMessage(std::make_shared<CDVDMsg>(CDVDMsg::GENERAL_FLUSH), 0);
}

void CVideoPlayerSubtitle::SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
  {
    auto pMsgDemuxerPacket = std::static_pointer_cast<CDVDMsgDemuxerPacket>(pMsg);
    DemuxPacket* pPacket = pMsgDemuxerPacket->GetPacket();

    if (m_pOverlayCodec)
    {
      OverlayMessage result = m_pOverlayCodec->Decode(pPacket);

      if (result == OverlayMessage::OC_OVERLAY)
      {
        std::shared_ptr<CDVDOverlay> overlay;

        while ((overlay = m_pOverlayCodec->GetOverlay()))
        {
          m_pOverlayContainer->ProcessAndAddOverlayIfValid(overlay);
        }
      }
    }
    else if (m_streaminfo.codec == AV_CODEC_ID_DVD_SUBTITLE)
    {
      std::shared_ptr<CDVDOverlaySpu> pSPUInfo =
          m_dvdspus.AddData(pPacket->pData, pPacket->iSize, pPacket->pts);
      if (pSPUInfo)
      {
        CLog::Log(LOGDEBUG, "CVideoPlayer::ProcessSubData: Got complete SPU packet");
        m_pOverlayContainer->ProcessAndAddOverlayIfValid(pSPUInfo);
      }
    }

  }
  else if( pMsg->IsType(CDVDMsg::SUBTITLE_CLUTCHANGE) )
  {
    auto pData = std::static_pointer_cast<CDVDMsgSubtitleClutChange>(pMsg);
    for (int i = 0; i < 16; i++)
    {
      uint8_t* color = m_dvdspus.m_clut[i];
      uint8_t* t = (uint8_t*)pData->m_data[i];

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

    /* We must flush active overlays on flush or if we have a file
     * parser since it will re-populate active items.  */
    if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH) || m_pSubtitleFileParser)
      m_pOverlayContainer->Flush();

    m_lastPts = DVD_NOPTS_VALUE;
  }
}

bool CVideoPlayerSubtitle::OpenStream(CDVDStreamInfo &hints, std::string &filename)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  CloseStream(false);
  m_streaminfo = hints;

  // okey check if this is a filesubtitle
  if(filename.size() && filename != "dvd" )
  {
    m_pSubtitleFileParser.reset(CDVDFactorySubtitle::CreateParser(filename));
    if (!m_pSubtitleFileParser)
    {
      CLog::Log(LOGERROR, "{} - Unable to create subtitle parser", __FUNCTION__);
      CloseStream(true);
      return false;
    }

    CLog::Log(LOGDEBUG, "Created subtitles parser: {}", m_pSubtitleFileParser->GetName());

    if (!m_pSubtitleFileParser->Open(hints))
    {
      CLog::Log(LOGERROR, "{} - Unable to init subtitle parser", __FUNCTION__);
      CloseStream(true);
      return false;
    }
    m_pSubtitleFileParser->Reset();
    return true;
  }

  // dvd's use special subtitle decoder
  if(hints.codec == AV_CODEC_ID_DVD_SUBTITLE && filename == "dvd")
    return true;

  m_pOverlayCodec = CDVDFactoryCodec::CreateOverlayCodec(hints);
  if (m_pOverlayCodec)
  {
    CLog::Log(LOGDEBUG, "Created subtitles overlay codec: {}", m_pOverlayCodec->GetName());
    return true;
  }

  CLog::Log(LOGERROR, "{} - Unable to init overlay codec", __FUNCTION__);
  return false;
}

void CVideoPlayerSubtitle::CloseStream(bool bWaitForBuffers)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  m_pSubtitleFileParser.reset();
  m_pOverlayCodec.reset();

  m_dvdspus.FlushCurrentPacket();

  if (!bWaitForBuffers)
    m_pOverlayContainer->Clear();
}

void CVideoPlayerSubtitle::Process(double pts, double offset)
{
  std::unique_lock<CCriticalSection> lock(m_section);

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

    std::shared_ptr<CDVDOverlay> pOverlay = m_pSubtitleFileParser->Parse(pts);
    // add all overlays which fit the pts
    while(pOverlay)
    {
      pOverlay->iPTSStartTime -= offset;
      if(pOverlay->iPTSStopTime != 0.0)
        pOverlay->iPTSStopTime -= offset;

      m_pOverlayContainer->ProcessAndAddOverlayIfValid(pOverlay);
      pOverlay = m_pSubtitleFileParser->Parse(pts);
    }

    m_lastPts = pts;
  }
}

bool CVideoPlayerSubtitle::AcceptsData() const
{
  // FIXME : This may still be causing problems + magic number :(
  return m_pOverlayContainer->GetSize() < 5;
}

