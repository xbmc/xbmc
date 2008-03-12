#ifndef __EVENT_CLIENT_H__
#define __EVENT_CLIENT_H__

#include "include.h"
#include "Thread.h"
#include "CriticalSection.h"
#include "Socket.h"
#include "EventPacket.h"
#include <map>
#include <queue>

#ifdef _LINUX

namespace EVENTCLIENT
{
  /**********************************************************************/
  /* UDP EventClient Class                                              */
  /**********************************************************************/
  // - clients timeout if they don't receive at least 1 ping in 1 minute
  // - sequence packets timeout after 5 seconds
  class CEventClient
  {
  public:
    CEventClient() {}
    CEventClient(SOCKETS::CAddress& addr) 
    {
      // todo: initialize stuff 
      m_remoteAddr = addr; 
    }
    virtual ~CEventClient() {}

    // add packet to queue
    bool AddPacket(EVENTPACKET::CEventPacket *packet);

    // return true if client received ping with the last 1 minute
    bool Alive();

    // process the packet queue
    bool ProcessQueue();

    // execute the queued up events
    void ExecuteEvents();

  protected:
    bool ProcessPacket(EVENTPACKET::CEventPacket *packet);

    virtual bool OnPacketHELO(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketBYE(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketBUTTON(EVENTPACKET::CEventPacket *packet);

    // helper functions

    // Parses a null terminated string from payload.
    // After parsing successfully:
    //   1. payload is incremented to end of string
    //   2. psize is decremented by length of string
    //   3. parsedVal contains the parsed string
    //   4. true is returned
    bool ParseString(unsigned char* &payload, int &psize, std::string& parsedVal);

    // Parses a single byte (same behavior as ParseString)
    bool ParseByte(unsigned char* &payload, int &psize, unsigned char& parsedVal);

    std::map<unsigned int, EVENTPACKET::CEventPacket*>  m_seqPackets;
    std::queue<EVENTPACKET::CEventPacket*>              m_readyPackets;
    std::string       m_deviceName;
    int               m_iCurrentSeqLen;
    time_t            m_lastPing;
    SOCKETS::CAddress m_remoteAddr;
    EVENTPACKET::LogoType m_eLogoType;
  };

}

#endif // _LINUX
#endif // __EVENT_CLIENT_H__
