#include "../stdafx.h"
#include "MediaPacketQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMediaPacketQueue::CMediaPacketQueue(CStdString& aName)
{
  m_strName = aName;
  m_pBuffer = new BYTE[ MPQ_PACKET_SIZE * MPQ_MAX_PACKETS ];

  Flush();
}

void CMediaPacketQueue::Flush()
{
  m_dwPacketR = 0;
  m_dwPacketW = 0;

  for ( int i = 0; i < MPQ_MAX_PACKETS; i++ )
  {
    // Initialize all the packets to be available
    m_adwStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;
  }
}

CMediaPacketQueue::~CMediaPacketQueue(void)
{
  delete[] m_pBuffer;
}

bool CMediaPacketQueue::Write(LPBYTE pSource)
{
  if ( m_adwStatus[ m_dwPacketW ] != XMEDIAPACKET_STATUS_PENDING )
  {
    // Copy the data into the buffer
    memcpy( m_pBuffer + m_dwPacketW * MPQ_PACKET_SIZE,
            pSource, MPQ_PACKET_SIZE );

    m_adwStatus[ m_dwPacketW ] = XMEDIAPACKET_STATUS_PENDING;

    m_dwPacketW = ( m_dwPacketW + 1 ) % MPQ_MAX_PACKETS;
    return true;
  }

  return false;
}

bool CMediaPacketQueue::Read(XMEDIAPACKET& aPacket)
{
  if ( m_adwStatus[ m_dwPacketR ] == XMEDIAPACKET_STATUS_PENDING ) //&&
    //   m_dwPacketR != m_dwPacketW )
  {
    ZeroMemory(&aPacket, sizeof(XMEDIAPACKET));
    aPacket.dwMaxSize = MPQ_PACKET_SIZE;
    aPacket.pvBuffer = m_pBuffer + m_dwPacketR * MPQ_PACKET_SIZE;
    aPacket.pdwStatus = &m_adwStatus[ m_dwPacketR ];

    m_dwPacketR = ( m_dwPacketR + 1 ) % MPQ_MAX_PACKETS;
    return true;
  }

  return false;
}

bool CMediaPacketQueue::IsEmpty()
{
  for ( int i = 0; i < MPQ_MAX_PACKETS; i++ )
  {
    if (m_adwStatus[i] != XMEDIAPACKET_STATUS_SUCCESS)
      return false;
  }
  return true;
}

int CMediaPacketQueue::Size()
{
  int count = 0;
  for ( int i = 0; i < MPQ_MAX_PACKETS; i++ )
  {
    if (m_adwStatus[i] != XMEDIAPACKET_STATUS_SUCCESS)
      count++;
  }
  return count;
}


