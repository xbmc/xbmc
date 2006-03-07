
#include "../../stdafx.h"
#include "DVDMessage.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"

void CDVDMessage::FreeMessageData(DVDMsg msg, DVDMsgData msg_data)
{
  // a message can consist of more than one message type
  // but in every case, there can only be one msg data type at most
  // so only if else here
  
  if (DVDMSG_IS(msg, DVDMSG_DEMUX_DATA_PACKET))
  {
    CDVDDemux::DemuxPacket* pPacket = (CDVDDemux::DemuxPacket*)msg_data;
    CDVDDemuxUtils::FreeDemuxPacket(pPacket);
  }

  if(DVDMSG_IS(msg, DVDMSG_GENERAL_STREAMCHANGE) && msg_data)
  {
    delete (CDVDStreamInfo*)msg_data;
  }

  if(DVDMSG_IS(msg, DVDMSG_GENERAL_SETCLOCK) && msg_data)
  {
    delete (SDVDMsgSetClock*)msg_data;
  }

  if(DVDMSG_IS(msg, DVDMSG_GENERAL_SYNCRONIZE) && msg_data)
  {
    ((CDVDMsgSyncronize*)msg_data)->Release();
  }
}

CDVDMsgSyncronize::CDVDMsgSyncronize(long objects, DWORD timeout)
{
  m_objects = 0;
  m_references = objects;
  m_timeout = GetTickCount() + timeout;
}

CDVDMsgSyncronize::~CDVDMsgSyncronize()
{

}

long CDVDMsgSyncronize::Release()
{
  long count = InterlockedDecrement(&m_references);
  if( count == 0 ) delete this;
  return count;
}

void CDVDMsgSyncronize::Wait(bool *abort)
{
  m_objects++;

  if( abort )
    while( !(*abort) && m_timeout > GetTickCount() && m_objects < m_references ) Sleep(1);
  else
    while( m_timeout > GetTickCount() && m_objects < m_references ) Sleep(1);
}