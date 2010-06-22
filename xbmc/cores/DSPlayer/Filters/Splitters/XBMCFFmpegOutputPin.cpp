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



#include "XBMCFFmpegOutputPin.h"

#include "moreuuids.h"
#include "streams.h"
#include "dvdmedia.h"
#include "DShowUtil/DShowUtil.h"

#pragma warning(disable: 4355)


//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
//
// CXBMCFFmpegOutputPin
//

CXBMCFFmpegOutputPin::CXBMCFFmpegOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
  : CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

HRESULT CXBMCFFmpegOutputPin::CheckConnect(IPin* pPin)
{
  int iPosition = 0;
  CMediaType mt;
  while(S_OK == GetMediaType(iPosition++, &mt))
  {
    mt.InitMediaType();
  }

  return __super::CheckConnect(pPin);
}
HRESULT CXBMCFFmpegOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	{
		CAutoLock cAutoLock(this);
		m_rtPrev = Packet::INVALID_TIME;
		m_rtOffset = 0;
	}

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CXBMCFFmpegOutputPin::DeliverEndFlush()
{
	{
		CAutoLock cAutoLock(this);
		m_p.reset();
		m_pl.clear();
	}

	return __super::DeliverEndFlush();
}

HRESULT CXBMCFFmpegOutputPin::DeliverPacket(Packet* p)
{
	CAutoLock cAutoLock(this);

#if 0
  if(p->rtStart != Packet::INVALID_TIME)
	{
		REFERENCE_TIME rt = p->rtStart + m_rtOffset;

		// Filter invalid PTS (if too different from previous packet)
		if(m_rtPrev != Packet::INVALID_TIME)
		if(_abs64(rt - m_rtPrev) > m_rtMaxShift)
			m_rtOffset += m_rtPrev - rt;

		p->rtStart += m_rtOffset;
		p->rtStop += m_rtOffset;

		m_rtPrev = p->rtStart;
	}

  if (p->pmt)
	{
		if (*((CMediaType *)p->pmt) != m_mt)
			SetMediaType ((CMediaType*)p->pmt);
	}

	if(m_mt.subtype == MEDIASUBTYPE_AAC)
  {
    if(m_p.get() && m_p->size() == 1 && m_p->at(0) == 0xff	&& !(!p->empty() && (p->at(0) & 0xf6) == 0xf0))
			m_p.reset();

		if(!m_p.get())
    {
			BYTE* base = &p->at(0);
			BYTE* s = base;
			BYTE* e = s + p->size();

			for(; s < e; s++)
			{
				if(*s != 0xff) continue;

				if(s == e-1 || (s[1]&0xf6) == 0xf0)
				{
					memmove(base, s, e - s);
					p->resize(e - s);
					m_p = p;//.reset(p.get());
					break;
				}
        HRESULT hr = __super::DeliverPacket(p);
        return hr;
			}
		}
		else
		{
      m_p->Append(*p);
      
		}
    return S_OK;
  }
#endif
  return __super::DeliverPacket(p);
    //verify we have a frame of at least 768 byte
    /*if (m_packetdata->getDataSize() < AAC_MIN_SAMPLESIZE)
    {
      bool res;
      if (m_packetdata->getDataSize() == 0)
      {
        m_packetdata->SetStart(p->rtStart);
      }
      res = m_packetdata->writeAppend(&p->at(0),p->size());

      if (m_packetdata->getDataSize() >= AAC_MIN_SAMPLESIZE)
      {
        p->resize(m_packetdata->getDataSize());
        memcpy(&p->at(0),m_packetdata->getData(),m_packetdata->getDataSize());
        p->rtStart = m_packetdata->GetStart();
        
        m_packetdata->Clear();
        return __super::DeliverPacket(p);
      }
      
    }*/
	
	
	
}