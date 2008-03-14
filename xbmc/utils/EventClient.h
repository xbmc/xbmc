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
    CEventClient() 
    {
      m_bGreeted = false; 
    }

    CEventClient(SOCKETS::CAddress& addr)
    {
      m_bGreeted = false;
      m_remoteAddr = addr; 
    }

    virtual ~CEventClient() 
    {
      FreeQueues();
    }

    // add packet to queue
    bool AddPacket(EVENTPACKET::CEventPacket *packet);

    // return true if client received ping with the last 1 minute
    bool Alive();

    // process the packet queue
    bool ProcessQueue();

    // execute the queued up events
    void ExecuteEvents();

    // deallocate all packets in the queues
    void FreeQueues();

    // 

  protected:
    bool ProcessPacket(EVENTPACKET::CEventPacket *packet);

    // packet handlers
    virtual bool OnPacketHELO(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketBYE(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketBUTTON(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketMOUSE(EVENTPACKET::CEventPacket *packet);
    virtual bool OnPacketNOTIFICATION(EVENTPACKET::CEventPacket *packet);

    // returns true if the client has received the HELO packet
    bool Greeted() { return m_bGreeted; }

    // reset the timeout counter
    void ResetTimeout()
    {
      m_lastPing = time(NULL);
    }

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
    
    // Parse a single 32-bit integer (converts from network order to host order)
    bool ParseUInt32(unsigned char* &payload, int &psize, unsigned int& parsedVal);

    // Parse a single 16-bit integer (converts from network order to host order)
    bool ParseUInt16(unsigned char* &payload, int &psize, unsigned short& parsedVal);

    std::map<unsigned int, EVENTPACKET::CEventPacket*>  m_seqPackets;
    std::queue<EVENTPACKET::CEventPacket*>              m_readyPackets;
    std::string       m_deviceName;
    unsigned int      m_iSeqPayloadSize;
    int               m_iCurrentSeqLen;
    time_t            m_lastPing;
    SOCKETS::CAddress m_remoteAddr;
    int               m_iRemotePort;
    EVENTPACKET::LogoType m_eLogoType;
    bool              m_bGreeted;

    // button and mouse state
    unsigned int      m_iKeyCode;
    std::string       m_buttonName;
    std::string       m_mapName;    
    float             m_fAmount;
    bool              m_bRepeat;
    float             m_fLastPress;
    float             m_fRepeatDelay;
    float             m_fMouseX;
    float             m_fMouseY;
  };

}

#endif // _LINUX
#endif // __EVENT_CLIENT_H__
