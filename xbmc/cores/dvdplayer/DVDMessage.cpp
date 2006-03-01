
#include "../../stdafx.h"
#include "DVDMessage.h"
#include "DVDDemuxers/DVDDemuxUtils.h"

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
}