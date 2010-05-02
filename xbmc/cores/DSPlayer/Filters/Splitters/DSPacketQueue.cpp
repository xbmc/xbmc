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


#include "DSPacketQueue.h"
#include "DShowUtil/DShowUtil.h"
//
// CDemuxPacketQueue
//

CDemuxPacketQueue::CDemuxPacketQueue() : m_size(0)
{
}

void CDemuxPacketQueue::Add(boost::shared_ptr<DsPacket> p)
{
  CAutoLock cAutoLock(this);

  if(p.get())
  {
    m_size += p->GetDataSize();
    if(p->bAppendable && !p->bDiscontinuity && !p->pmt
    && p->rtStart == DsPacket::INVALID_TIME
    && ! empty() && back()->rtStart != DsPacket::INVALID_TIME)
    {
      boost::shared_ptr<DsPacket> tail;
      tail =  back();
      int oldsize = tail->size();
      int newsize = tail->size() + p->size();
      tail->resize(newsize, dsmax(1024, newsize)); // doubles the reserved buffer size
      //Not sure about this one
      //memcpy(&tail[0] + oldsize, &(p.get())[0], p.get()->size());
      /*
      GetTail()->Append(*p); // too slow
      */
      return;
    }
  }

   push_back(p);
}

boost::shared_ptr<DsPacket> CDemuxPacketQueue::Remove()
{
  /*
  CAutoLock cAutoLock(this);
	ASSERT(__super::GetCount() > 0);
	CAutoPtr<DsPacket> p = RemoveHead();
	if(p) m_size -= p->GetDataSize();
	return p;
  */
  CAutoLock cAutoLock(this);
  ASSERT(__super::size() > 0);
  boost::shared_ptr<DsPacket> p;
  p = front();
  erase(begin());
  //p = auto_ptr<DsPacket>(front());
  if(p.get()) 
    m_size -= p.get()->GetDataSize();
  return p;
}

void CDemuxPacketQueue::RemoveAll()
{
  CAutoLock cAutoLock(this);
  m_size = 0;
  while (!__super::empty())
    __super::pop_back();
}

int CDemuxPacketQueue::size()
{
  CAutoLock cAutoLock(this); 
  return __super::size();
}

int CDemuxPacketQueue::GetSize()
{
  CAutoLock cAutoLock(this); 
  return m_size;
}