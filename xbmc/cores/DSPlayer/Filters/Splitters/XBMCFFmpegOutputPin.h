#pragma once
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



#include "streams.h"
#include "../BaseFilters/BaseSplitter.h"

class CXBMCSplitterFilter;

//********************************************************************
//
//********************************************************************
class CXBMCFFmpegOutputPin : public CBaseSplitterOutputPin, protected CCritSec
{
  boost::shared_ptr<Packet> m_p;
  std::list<boost::shared_ptr<Packet>> m_pl;
	REFERENCE_TIME m_rtPrev, m_rtOffset, m_rtMaxShift;
	bool m_fHasAccessUnitDelimiters;

protected:
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT DeliverPacket(Packet* p);
	HRESULT DeliverEndFlush();
public:
  CXBMCFFmpegOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

  HRESULT CheckConnect(IPin* pPin);
};

