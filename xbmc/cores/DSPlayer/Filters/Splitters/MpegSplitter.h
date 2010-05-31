/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "../BaseFilters/BaseSplitter.h"
#include "MpegSplitterFile.h"


[uuid("DC257063-045F-4BE2-BD5B-E12279C464F0")]
class CMpegSplitterFilter : public CBaseSplitterFilter, public IAMStreamSelect
{
	REFERENCE_TIME	m_rtStartOffset;
	bool			m_pPipoBimbo;
	CHdmvClipInfo	m_ClipInfo;

protected:
	std::auto_ptr<CMpegSplitterFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);
	void	ReadClipInfo(LPCOLESTR pszFileName);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	HRESULT DemuxNextPacket(REFERENCE_TIME rtStartOffset);

public:
	CMpegSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid = __uuidof(CMpegSplitterFilter));
	void SetPipo(bool bPipo) { m_pPipoBimbo = bPipo; };

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP GetClassID(CLSID* pClsID);
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);

	// IAMStreamSelect

	STDMETHODIMP Count(DWORD* pcStreams); 
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags); 
	STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);  
};

[uuid("1365BE7A-C86A-473C-9A41-C0A6E82C9FA3")]
class CMpegSourceFilter : public CMpegSplitterFilter
{
public:
	CMpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid = __uuidof(CMpegSourceFilter));
};

class CMpegSplitterOutputPin : public CBaseSplitterOutputPin, protected CCritSec
{
	std::auto_ptr<Packet> m_p;
  std::list<boost::shared_ptr<Packet>> m_pl;
	REFERENCE_TIME m_rtPrev, m_rtOffset, m_rtMaxShift;
	bool m_fHasAccessUnitDelimiters;

protected:
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT DeliverPacket(std::auto_ptr<Packet> p);
	HRESULT DeliverEndFlush();

public:
	CMpegSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMpegSplitterOutputPin();
	STDMETHODIMP	Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt);
	void			SetMaxShift(REFERENCE_TIME rtMaxShift) { m_rtMaxShift = rtMaxShift; };
};
