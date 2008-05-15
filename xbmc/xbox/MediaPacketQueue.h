/*!
\file MediaPacketQueue.h
\brief 
*/

#ifndef MEDIAPACKETQUEUE_H
#define MEDIAPACKETQUEUE_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define MPQ_MAX_PACKETS 60
#define MPQ_PACKET_SIZE 20

class CMediaPacketQueue
{
public:

  CMediaPacketQueue(CStdString& aQueueName);
  virtual ~CMediaPacketQueue(void);

  bool Write(LPBYTE pSource);
  bool Read(XMEDIAPACKET& aPacket);

  void Flush();

  bool IsEmpty();
  int Size();

protected:

  CStdString m_strName;

  BYTE* m_pBuffer;
  DWORD m_adwStatus[MPQ_MAX_PACKETS];
  DWORD m_dwPacketR;
  DWORD m_dwPacketW;
};

#endif
