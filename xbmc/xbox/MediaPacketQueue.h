/*!
	\file MediaPacketQueue.h
	\brief 
	*/

#ifndef GUILIB_MediaPacketQueueEx_H
#define GUILIB_MediaPacketQueueEx_H

#pragma once

#define MPQ_MAX_PACKETS	60
#define MPQ_PACKET_SIZE	20

class CMediaPacketQueue
{
public:

	CMediaPacketQueue(CStdString& aQueueName);
	virtual ~CMediaPacketQueue(void);

	bool Write(LPBYTE pSource);
	bool Read(XMEDIAPACKET& aPacket);

	void Flush();

	bool IsEmpty();
	int	 Size();

protected:

	CStdString m_strName;

    BYTE*	m_pBuffer;
    DWORD	m_adwStatus[MPQ_MAX_PACKETS];
    DWORD	m_dwPacketR;
	DWORD	m_dwPacketW;
};

#endif
