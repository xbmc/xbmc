/*
 *      Copyright (C) 2005-2010 Team XBMC
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
#include "Streams.h"
#include <list>
class DsPacket : public std::vector<BYTE>
{
public:
	DWORD TrackNumber;
	BOOL bDiscontinuity, bSyncPoint, bAppendable;
	static const REFERENCE_TIME INVALID_TIME = _I64_MIN;
	REFERENCE_TIME rtStart, rtStop;
	AM_MEDIA_TYPE* pmt;
	DsPacket() {pmt = NULL; bDiscontinuity = bAppendable = FALSE;}
	virtual ~DsPacket() {if(pmt) DeleteMediaType(pmt);}
	virtual int GetDataSize() {return size();}
	void SetData(const void* ptr, DWORD len) 
  {
    resize(len); 
    memcpy(&this[0], ptr, len);
  }
};

class CDemuxPacketQueue 
  : public CCritSec,
    protected std::list<boost::shared_ptr<DsPacket>>
{
  int m_size;
public:
  CDemuxPacketQueue();
  void Add(boost::shared_ptr<DsPacket> p);
  boost::shared_ptr<DsPacket> Remove();
  void RemoveAll();
  int size(), GetSize();
  
};