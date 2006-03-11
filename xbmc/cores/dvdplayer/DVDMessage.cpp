
#include "../../stdafx.h"
#include "DVDMessage.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"

/**
 * CDVDMsgGeneralStreamChange --- GENERAL_STREAMCHANGE
 */
CDVDMsgGeneralStreamChange::CDVDMsgGeneralStreamChange(CDVDStreamInfo* pInfo) : CDVDMsg(GENERAL_STREAMCHANGE)
{
  m_pInfo = pInfo;
}

CDVDMsgGeneralStreamChange::~CDVDMsgGeneralStreamChange()
{
  if (m_pInfo)
  {
    delete m_pInfo;
  }
}

/**
 * CDVDMsgGeneralStreamChange --- GENERAL_STREAMCHANGE
 */
CDVDMsgGeneralSynchronize::CDVDMsgGeneralSynchronize(DWORD timeout) : CDVDMsg(GENERAL_SYNCHRONIZE)
{
  m_objects = 0;
  m_timeout = GetTickCount() + timeout;
}

void CDVDMsgGeneralSynchronize::Wait(volatile bool *abort)
{
  InterlockedIncrement(&m_objects);

  if( abort )
    while( !(*abort) && m_timeout > GetTickCount() && m_objects < GetNrOfReferences()) Sleep(1);
  else
    while( m_timeout > GetTickCount() && m_objects < GetNrOfReferences() ) Sleep(1);
}

/**
 * CDVDMsgDemuxerPacket --- DEMUXER_PACKET
 */
CDVDMsgDemuxerPacket::CDVDMsgDemuxerPacket(CDVDDemux::DemuxPacket* pPacket, unsigned int packetSize) : CDVDMsg(DEMUXER_PACKET)
{
  m_pPacket = pPacket;
  m_packetSize = packetSize;
}

CDVDMsgDemuxerPacket::~CDVDMsgDemuxerPacket()
{
  if (m_pPacket)
  {
    CDVDDemuxUtils::FreeDemuxPacket(m_pPacket);
  }
}
