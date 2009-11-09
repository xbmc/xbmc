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


#include "ISubPic.h"
#include "dshowutil\DShowUtil.h"
#include "log.h"
//
// ISubPicImpl
//

ISubPicImpl::ISubPicImpl() 
	: CUnknown(NAME("ISubPicImpl"), NULL)
	, m_rtStart(0), m_rtStop(0)
	, m_rtSegmentStart(0), m_rtSegmentStop(0)
	, m_rcDirty(0, 0, 0, 0), m_maxsize(0, 0), m_size(0, 0), m_vidrect(0, 0, 0, 0)
	, m_VirtualTextureSize(0, 0), m_VirtualTextureTopLeft (0, 0)
{
}

STDMETHODIMP ISubPicImpl::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(ISubPic)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPic

STDMETHODIMP_(REFERENCE_TIME) ISubPicImpl::GetStart()
{
	return(m_rtStart);
}

STDMETHODIMP_(REFERENCE_TIME) ISubPicImpl::GetStop()
{
	return(m_rtStop);
}

STDMETHODIMP_(REFERENCE_TIME) ISubPicImpl::GetSegmentStart()
{
	if (m_rtSegmentStart)
		return(m_rtSegmentStart);
	return(m_rtStart);
}

STDMETHODIMP_(REFERENCE_TIME) ISubPicImpl::GetSegmentStop()
{
	if (m_rtSegmentStop)
		return(m_rtSegmentStop);
	return(m_rtStop);
}

STDMETHODIMP_(void) ISubPicImpl::SetSegmentStart(REFERENCE_TIME rtStart)
{
	m_rtSegmentStart = rtStart;
}

STDMETHODIMP_(void) ISubPicImpl::SetSegmentStop(REFERENCE_TIME rtStop)
{
	m_rtSegmentStop = rtStop;
}



STDMETHODIMP_(void) ISubPicImpl::SetStart(REFERENCE_TIME rtStart)
{
	m_rtStart = rtStart;
}

STDMETHODIMP_(void) ISubPicImpl::SetStop(REFERENCE_TIME rtStop)
{
	m_rtStop = rtStop;
}

STDMETHODIMP ISubPicImpl::CopyTo(ISubPic* pSubPic)
{
	if(!pSubPic)
		return E_POINTER;

	pSubPic->SetStart(m_rtStart);
	pSubPic->SetStop(m_rtStop);
	pSubPic->SetSegmentStart(m_rtSegmentStart);
	pSubPic->SetSegmentStop(m_rtSegmentStop);
	pSubPic->SetDirtyRect(m_rcDirty);
	pSubPic->SetSize(m_size, m_vidrect);
	pSubPic->SetVirtualTextureSize(m_VirtualTextureSize, m_VirtualTextureTopLeft);

	return S_OK;
}

STDMETHODIMP ISubPicImpl::GetDirtyRect(RECT* pDirtyRect)
{
	return pDirtyRect ? *pDirtyRect = m_rcDirty, S_OK : E_POINTER;
}

STDMETHODIMP ISubPicImpl::GetSourceAndDest(SIZE* pSize, RECT* pRcSource, RECT* pRcDest)
{
	CheckPointer (pRcSource, E_POINTER);
	CheckPointer (pRcDest,	 E_POINTER);

	if(m_size.cx > 0 && m_size.cy > 0)
	{
		CRect		rcTemp = m_rcDirty;

		// FIXME
		rcTemp.DeflateRect(1, 1);

		*pRcSource = rcTemp;

		rcTemp.OffsetRect (m_VirtualTextureTopLeft);
		*pRcDest = CRect (rcTemp.left   * pSize->cx / m_VirtualTextureSize.cx,
						  rcTemp.top    * pSize->cy / m_VirtualTextureSize.cy,
						  rcTemp.right  * pSize->cx / m_VirtualTextureSize.cx,
						  rcTemp.bottom * pSize->cy / m_VirtualTextureSize.cy);

		return S_OK;
	}
	else
		return E_INVALIDARG;
}

STDMETHODIMP ISubPicImpl::SetDirtyRect(RECT* pDirtyRect)
{
	return pDirtyRect ? m_rcDirty = *pDirtyRect, S_OK : E_POINTER;
}

STDMETHODIMP ISubPicImpl::GetMaxSize(SIZE* pMaxSize)
{
	return pMaxSize ? *pMaxSize = m_maxsize, S_OK : E_POINTER;
}

STDMETHODIMP ISubPicImpl::SetSize(SIZE size, RECT vidrect)
{
	m_size = size;
	m_vidrect = vidrect;

	if(m_size.cx > m_maxsize.cx)
	{
		m_size.cy = MulDiv(m_size.cy, m_maxsize.cx, m_size.cx);
		m_size.cx = m_maxsize.cx;
	}

	if(m_size.cy > m_maxsize.cy)
	{
		m_size.cx = MulDiv(m_size.cx, m_maxsize.cy, m_size.cy);
		m_size.cy = m_maxsize.cy;
	}

	if(m_size.cx != size.cx || m_size.cy != size.cy)
	{
		m_vidrect.top = MulDiv(m_vidrect.top, m_size.cx, size.cx);
		m_vidrect.bottom = MulDiv(m_vidrect.bottom, m_size.cx, size.cx);
		m_vidrect.left = MulDiv(m_vidrect.left, m_size.cy, size.cy);
		m_vidrect.right = MulDiv(m_vidrect.right, m_size.cy, size.cy);
	}
	m_VirtualTextureSize = m_size;

	return S_OK;
}

STDMETHODIMP ISubPicImpl::SetVirtualTextureSize (const SIZE pSize, const POINT pTopLeft)
{
	m_VirtualTextureSize.SetSize (pSize.cx, pSize.cy);
	m_VirtualTextureTopLeft.SetPoint (pTopLeft.x, pTopLeft.y);
	
	return S_OK;
}

//
// ISubPicAllocatorImpl
//

ISubPicAllocatorImpl::ISubPicAllocatorImpl(SIZE cursize, bool fDynamicWriteOnly, bool fPow2Textures)
	: CUnknown(NAME("ISubPicAllocatorImpl"), NULL)
	, m_cursize(cursize)
	, m_fDynamicWriteOnly(fDynamicWriteOnly)
	, m_fPow2Textures(fPow2Textures)
{
	m_curvidrect = CRect(CPoint(0,0), m_cursize);
}

STDMETHODIMP ISubPicAllocatorImpl::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(ISubPicAllocator)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicAllocator

STDMETHODIMP ISubPicAllocatorImpl::SetCurSize(SIZE cursize)
{
	m_cursize = cursize; 
	return S_OK;
}

STDMETHODIMP ISubPicAllocatorImpl::SetCurVidRect(RECT curvidrect)
{
	m_curvidrect = curvidrect; 
	return S_OK;
}

STDMETHODIMP ISubPicAllocatorImpl::GetStatic(ISubPic** ppSubPic)
{
	if(!ppSubPic)
		return E_POINTER;

	if(!m_pStatic)
	{
		if(!Alloc(true, &m_pStatic) || !m_pStatic) 
			return E_OUTOFMEMORY;
	}

	m_pStatic->SetSize(m_cursize, m_curvidrect);

	(*ppSubPic = m_pStatic)->AddRef();

	return S_OK;
}

STDMETHODIMP ISubPicAllocatorImpl::AllocDynamic(ISubPic** ppSubPic)
{
	if(!ppSubPic)
		return E_POINTER;

	if(!Alloc(false, ppSubPic) || !*ppSubPic)
		return E_OUTOFMEMORY;

	(*ppSubPic)->SetSize(m_cursize, m_curvidrect);

	return S_OK;
}

STDMETHODIMP_(bool) ISubPicAllocatorImpl::IsDynamicWriteOnly()
{
	return(m_fDynamicWriteOnly);
}

STDMETHODIMP ISubPicAllocatorImpl::ChangeDevice(IUnknown* pDev)
{
	m_pStatic = NULL;
	return S_OK;
}


//
// ISubPicProviderImpl
//

ISubPicProviderImpl::ISubPicProviderImpl(CCritSec* pLock)
	: CUnknown(NAME("ISubPicProviderImpl"), NULL)
	, m_pLock(pLock)
{
}

ISubPicProviderImpl::~ISubPicProviderImpl()
{
}

STDMETHODIMP ISubPicProviderImpl::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ISubPicProvider)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP ISubPicProviderImpl::Lock()
{
	return m_pLock ? m_pLock->Lock(), S_OK : E_FAIL;
}

STDMETHODIMP ISubPicProviderImpl::Unlock()
{
	return m_pLock ? m_pLock->Unlock(), S_OK : E_FAIL;
}

//
// ISubPicQueueImpl
//

ISubPicQueueImpl::ISubPicQueueImpl(ISubPicAllocator* pAllocator, HRESULT* phr) 
	: CUnknown(NAME("ISubPicQueueImpl"), NULL)
	, m_pAllocator(pAllocator)
	, m_rtNow(0)
	, m_rtNowLast(0)
	, m_fps(25.0)
{
	if(phr) *phr = S_OK;

	if(!m_pAllocator)
	{
		if(phr) *phr = E_FAIL;
		return;
	}
}

ISubPicQueueImpl::~ISubPicQueueImpl()
{
}

STDMETHODIMP ISubPicQueueImpl::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(ISubPicQueue)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicQueue

STDMETHODIMP ISubPicQueueImpl::SetSubPicProvider(ISubPicProvider* pSubPicProvider)
{
	CAutoLock cAutoLock(&m_csSubPicProvider);

//	if(m_pSubPicProvider != pSubPicProvider)
	{
		m_pSubPicProvider = pSubPicProvider;

		Invalidate();
	}

	return S_OK;
}

STDMETHODIMP ISubPicQueueImpl::GetSubPicProvider(ISubPicProvider** pSubPicProvider)
{
	if(!pSubPicProvider)
		return E_POINTER;

	CAutoLock cAutoLock(&m_csSubPicProvider);

	if(m_pSubPicProvider)
		(*pSubPicProvider = m_pSubPicProvider)->AddRef();

	return !!*pSubPicProvider ? S_OK : E_FAIL;
}

STDMETHODIMP ISubPicQueueImpl::SetFPS(double fps)
{
	m_fps = fps;

	return S_OK;
}

STDMETHODIMP ISubPicQueueImpl::SetTime(REFERENCE_TIME rtNow)
{
	m_rtNow = rtNow;

	return S_OK;
}

// private

HRESULT ISubPicQueueImpl::RenderTo(ISubPic* pSubPic, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double fps, BOOL bIsAnimated)
{
	HRESULT hr = E_FAIL;

	if(!pSubPic)
		return hr;

	CComPtr<ISubPicProvider> pSubPicProvider;
	if(FAILED(GetSubPicProvider(&pSubPicProvider)) || !pSubPicProvider)
		return hr;

	if(FAILED(pSubPicProvider->Lock()))
		return hr;

	SubPicDesc spd;
	if(SUCCEEDED(pSubPic->ClearDirtyRect(0xFF000000))
	&& SUCCEEDED(pSubPic->Lock(spd)))
	{
		CRect r(0,0,0,0);
		hr = pSubPicProvider->Render(spd, bIsAnimated ? rtStart : ((rtStart+rtStop)/2), fps, r);

		pSubPic->SetStart(rtStart);
		pSubPic->SetStop(rtStop);

		pSubPic->Unlock(r);
	}

	pSubPicProvider->Unlock();

	return hr;
}

//
// CSubPicQueue
//

CSubPicQueue::CSubPicQueue(int nMaxSubPic, BOOL bDisableAnim, ISubPicAllocator* pAllocator, HRESULT* phr) 
	: ISubPicQueueImpl(pAllocator, phr)
	, m_nMaxSubPic(nMaxSubPic)
	, m_bDisableAnim(bDisableAnim)
	,m_rtQueueMin(0)
	,m_rtQueueMax(0)
{
	if(phr && FAILED(*phr))
		return;

	if(m_nMaxSubPic < 1)
		{if(phr) *phr = E_INVALIDARG; return;}

	m_fBreakBuffering = false;
	for(int i = 0; i < EVENT_COUNT; i++) 
		m_ThreadEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	CAMThread::Create();
}

CSubPicQueue::~CSubPicQueue()
{
	m_fBreakBuffering = true;
	SetEvent(m_ThreadEvents[EVENT_EXIT]);
	CAMThread::Close();
	for(int i = 0; i < EVENT_COUNT; i++) 
		CloseHandle(m_ThreadEvents[i]);
}

// ISubPicQueue

STDMETHODIMP CSubPicQueue::SetFPS(double fps)
{
	HRESULT hr = __super::SetFPS(fps);
	if(FAILED(hr)) return hr;

	SetEvent(m_ThreadEvents[EVENT_TIME]);

	return S_OK;
}

STDMETHODIMP CSubPicQueue::SetTime(REFERENCE_TIME rtNow)
{
	HRESULT hr = __super::SetTime(rtNow);
	if(FAILED(hr)) return hr;

	SetEvent(m_ThreadEvents[EVENT_TIME]);

	return S_OK;
}

STDMETHODIMP CSubPicQueue::Invalidate(REFERENCE_TIME rtInvalidate)
{
	{
//		CAutoLock cQueueLock(&m_csQueueLock);
//		RemoveAll();

		m_rtInvalidate = rtInvalidate;
		m_fBreakBuffering = true;
#if DSubPicTraceLevel > 0
		TRACE(_T("Invalidate: %f\n"), double(rtInvalidate) / 10000000.0);
#endif

		SetEvent(m_ThreadEvents[EVENT_TIME]);
	}

	return S_OK;
}

STDMETHODIMP_(bool) CSubPicQueue::LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic> &ppSubPic)
{

	CAutoLock cQueueLock(&m_csQueueLock);

	REFERENCE_TIME rtBestStop = 0x7fffffffffffffffi64;
	POSITION pos = m_Queue.GetHeadPosition();
#if DSubPicTraceLevel > 2
	TRACE("Find: ");
#endif
	while(pos)
	{
		CComPtr<ISubPic> pSubPic = m_Queue.GetNext(pos);
		REFERENCE_TIME rtStart = pSubPic->GetStart();
		REFERENCE_TIME rtStop = pSubPic->GetStop();
		REFERENCE_TIME rtSegmentStop = pSubPic->GetSegmentStop();
		if(rtNow >= rtStart && rtNow < rtSegmentStop)
		{
			REFERENCE_TIME Diff = rtNow - rtStop;
			if (Diff < rtBestStop)
			{
				rtBestStop = Diff;
//				TRACE("   %f->%f", double(Diff) / 10000000.0, double(rtStop) / 10000000.0);
				ppSubPic = pSubPic;
			}
#if DSubPicTraceLevel > 2
			else
				TRACE("   !%f->%f", double(Diff) / 10000000.0, double(rtStop) / 10000000.0);
#endif
		}
#if DSubPicTraceLevel > 2
		else
			TRACE("   !!%f->%f", double(rtStart) / 10000000.0, double(rtSegmentStop) / 10000000.0);
#endif

	}
#if DSubPicTraceLevel > 2
	TRACE("\n");
#endif
	if (!ppSubPic)
	{
#if DSubPicTraceLevel > 1
		TRACE("NO Display: %f\n", double(rtNow) / 10000000.0);
#endif
	}
	else
	{
#if DSubPicTraceLevel > 0
		REFERENCE_TIME rtStart = (ppSubPic)->GetStart();
		REFERENCE_TIME rtSegmentStop = (ppSubPic)->GetSegmentStop();
		CRect r;
		(ppSubPic)->GetDirtyRect(&r);
		TRACE("Display: %f->%f   %f    %dx%d\n", double(rtStart) / 10000000.0, double(rtSegmentStop) / 10000000.0, double(rtNow) / 10000000.0, r.Width(), r.Height());
#endif
	}

	return(!!ppSubPic);
}

STDMETHODIMP CSubPicQueue::GetStats(int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	CAutoLock cQueueLock(&m_csQueueLock);

	nSubPics = m_Queue.GetCount();
	rtNow = m_rtNow;
	rtStart = m_rtQueueMin;
	if (rtStart == 0x7fffffffffffffffi64)
		rtStart = 0;
	rtStop = m_rtQueueMax;
	if (rtStop == 0xffffffffffffffffi64)
		rtStop = 0;

	return S_OK;
}

STDMETHODIMP CSubPicQueue::GetStats(int nSubPic, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	CAutoLock cQueueLock(&m_csQueueLock);

	rtStart = rtStop = -1;

	if(nSubPic >= 0 && nSubPic < (int)m_Queue.GetCount())
	{
		if(POSITION pos = m_Queue.FindIndex(nSubPic))
		{
			rtStart = m_Queue.GetAt(pos)->GetStart();
			rtStop = m_Queue.GetAt(pos)->GetStop();
		}
	}
	else
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

// private

REFERENCE_TIME CSubPicQueue::UpdateQueue()
{
	CAutoLock cQueueLock(&m_csQueueLock);

	REFERENCE_TIME rtNow = m_rtNow;
	REFERENCE_TIME rtNowCompare = rtNow;

	if (rtNow < m_rtNowLast)
	{
		m_Queue.RemoveAll();
		m_rtNowLast = rtNow;
	}
	else
	{
		m_rtNowLast = rtNow;

		m_rtQueueMin = 0x7fffffffffffffffi64;
		m_rtQueueMax = 0xffffffffffffffffi64;

		REFERENCE_TIME rtBestStop = 0x7fffffffffffffffi64;
		POSITION SavePos = 0;
		{
			POSITION Iter = m_Queue.GetHeadPosition();
			while(Iter)
			{
				POSITION ThisPos = Iter;
				ISubPic *pSubPic = m_Queue.GetNext(Iter);
				REFERENCE_TIME rtStart = pSubPic->GetStart();
				REFERENCE_TIME rtStop = pSubPic->GetStop();
				REFERENCE_TIME rtSegmentStop = pSubPic->GetSegmentStop();
				if(rtNow >= rtStart && rtNow < rtSegmentStop)
				{
					REFERENCE_TIME Diff = rtNow - rtStop;
					if (Diff < rtBestStop)
					{
						rtBestStop = Diff;
						SavePos = ThisPos;
					}
				}
			}
		}

	#if DSubPicTraceLevel > 3
		if (SavePos)
		{
			ISubPic *pSubPic = GetAt(SavePos);
			REFERENCE_TIME rtStart = pSubPic->GetStart();
			REFERENCE_TIME rtStop = pSubPic->GetStop();
			TRACE("Save: %f->%f\n", double(rtStart) / 10000000.0, double(rtStop) / 10000000.0);
		}
	#endif
		{
			POSITION Iter = m_Queue.GetHeadPosition();
			while(Iter)
			{
				POSITION ThisPos = Iter;
				ISubPic *pSubPic = m_Queue.GetNext(Iter);

				REFERENCE_TIME rtStart = pSubPic->GetStart();
				REFERENCE_TIME rtStop = pSubPic->GetStop();

				if (rtStop <= rtNowCompare && ThisPos != SavePos)
				{
	#if DSubPicTraceLevel > 0
					TRACE("Remove: %f->%f\n", double(rtStart) / 10000000.0, double(rtStop) / 10000000.0);
	#endif
					m_Queue.RemoveAt(ThisPos);
					continue;
				}
				if (rtStop > rtNow)
					rtNow = rtStop;
				m_rtQueueMin = min(m_rtQueueMin, rtStart);
				m_rtQueueMax = max(m_rtQueueMax, rtStop);
			}
		}
	}

	return(rtNow);
}

int CSubPicQueue::GetQueueCount()
{
	CAutoLock cQueueLock(&m_csQueueLock);

	return m_Queue.GetCount();
}

void CSubPicQueue::AppendQueue(ISubPic* pSubPic)
{
	CAutoLock cQueueLock(&m_csQueueLock);

	m_Queue.AddTail(pSubPic);
}

// overrides

DWORD CSubPicQueue::ThreadProc()
{	
	BOOL bDisableAnim = m_bDisableAnim;
	SetThreadPriority(m_hThread, bDisableAnim ? THREAD_PRIORITY_LOWEST : THREAD_PRIORITY_ABOVE_NORMAL/*THREAD_PRIORITY_BELOW_NORMAL*/);

	bool bAgain = true;
	while(1)
	{
		DWORD Ret = WaitForMultipleObjects(EVENT_COUNT, m_ThreadEvents, FALSE, bAgain ? 0 : INFINITE);
		bAgain = false;

		if (Ret == WAIT_TIMEOUT)
			;
		else if ((Ret - WAIT_OBJECT_0) != EVENT_TIME)
			break;
		double fps = m_fps;
		REFERENCE_TIME rtTimePerFrame = 10000000.0/fps;
		REFERENCE_TIME rtNow = UpdateQueue();

		int nMaxSubPic = m_nMaxSubPic;

		CComPtr<ISubPicProvider> pSubPicProvider;
		if(SUCCEEDED(GetSubPicProvider(&pSubPicProvider)) && pSubPicProvider
		&& SUCCEEDED(pSubPicProvider->Lock()))
		{
			for(POSITION pos = pSubPicProvider->GetStartPosition(rtNow, fps); 
				pos && !m_fBreakBuffering && GetQueueCount() < (size_t)nMaxSubPic; 
				pos = pSubPicProvider->GetNext(pos))
			{
				REFERENCE_TIME rtStart = pSubPicProvider->GetStart(pos, fps);
				REFERENCE_TIME rtStop = pSubPicProvider->GetStop(pos, fps);

				if(m_rtNow >= rtStart)
				{
//						m_fBufferUnderrun = true;
					if(m_rtNow >= rtStop) continue;
				}

				if(rtStart >= m_rtNow + 60*10000000i64) // we are already one minute ahead, this should be enough
					break;

				if(rtNow < rtStop)
				{
					REFERENCE_TIME rtCurrent = max(rtNow, rtStart);
					bool bIsAnimated = pSubPicProvider->IsAnimated(pos) && !bDisableAnim;
					while (rtCurrent < rtStop)
					{
						
						SIZE	MaxTextureSize, VirtualSize;
						POINT	VirtualTopLeft;
						HRESULT	hr2;
						if (SUCCEEDED (hr2 = pSubPicProvider->GetTextureSize(pos, MaxTextureSize, VirtualSize, VirtualTopLeft)))
							m_pAllocator->SetMaxTextureSize(MaxTextureSize);

						CComPtr<ISubPic> pStatic;
						if(FAILED(m_pAllocator->GetStatic(&pStatic)))
							break;

						HRESULT hr;
						if (bIsAnimated)
						{
							if (rtCurrent < m_rtNow + rtTimePerFrame)
								rtCurrent = min(m_rtNow + rtTimePerFrame, rtStop-1);

							REFERENCE_TIME rtEndThis = min(rtCurrent + rtTimePerFrame, rtStop);
							hr = RenderTo(pStatic, rtCurrent, rtEndThis, fps, bIsAnimated);
							pStatic->SetSegmentStart(rtStart);
							pStatic->SetSegmentStop(rtStop);
#if DSubPicTraceLevel > 0
							CRect r;
							pStatic->GetDirtyRect(&r);
							TRACE("Render: %f->%f    %f->%f      %dx%d\n", double(rtCurrent) / 10000000.0, double(rtEndThis) / 10000000.0, double(rtStart) / 10000000.0, double(rtStop) / 10000000.0, r.Width(), r.Height());
#endif
							rtCurrent = rtEndThis;


						}
						else
						{
							hr = RenderTo(pStatic, rtStart, rtStop, fps, bIsAnimated);
							rtCurrent = rtStop;
						}			
#if DSubPicTraceLevel > 0
						if (m_rtNow > rtCurrent)
						{
							TRACE("BEHIND\n");
						}
#endif

						if(FAILED(hr))
							break;

						if(S_OK != hr) // subpic was probably empty
							continue;

						CComPtr<ISubPic> pDynamic;
						if(FAILED(m_pAllocator->AllocDynamic(&pDynamic))
						|| FAILED(pStatic->CopyTo(pDynamic)))
							break;

						if (SUCCEEDED (hr2))
							pDynamic->SetVirtualTextureSize (VirtualSize, VirtualTopLeft);

						AppendQueue(pDynamic);
						bAgain = true;

						if (GetQueueCount() >= (size_t)nMaxSubPic)
							break;
					}
				}
			}

			pSubPicProvider->Unlock();
		}

		if(m_fBreakBuffering)
		{
			bAgain = true;
			CAutoLock cQueueLock(&m_csQueueLock);

			REFERENCE_TIME rtInvalidate = m_rtInvalidate;

			POSITION Iter = m_Queue.GetHeadPosition();
			while(Iter)
			{
				POSITION ThisPos = Iter;
				ISubPic *pSubPic = m_Queue.GetNext(Iter);

				REFERENCE_TIME rtStart = pSubPic->GetStart();
				REFERENCE_TIME rtStop = pSubPic->GetStop();

				if (rtStop > rtInvalidate)
				{
#if DSubPicTraceLevel >= 0
					CLog::Log(LOGDEBUG,"Removed subtitle because of invalidation: %f->%f\n", double(rtStart) / 10000000.0, double(rtStop) / 10000000.0);
#endif
					m_Queue.RemoveAt(ThisPos);
					continue;
				}
			}

/*
			while(GetCount() && GetTail()->GetStop() > rtInvalidate)
			{
				if(GetTail()->GetStart() < rtInvalidate) GetTail()->SetStop(rtInvalidate);
				else 
				{
					RemoveTail();
				}
			}
			*/

			m_fBreakBuffering = false;
		}
	}

	return(0);
}

//
// CSubPicQueueNoThread
//

CSubPicQueueNoThread::CSubPicQueueNoThread(ISubPicAllocator* pAllocator, HRESULT* phr) 
	: ISubPicQueueImpl(pAllocator, phr)
{
}

CSubPicQueueNoThread::~CSubPicQueueNoThread()
{
}

// ISubPicQueue

STDMETHODIMP CSubPicQueueNoThread::Invalidate(REFERENCE_TIME rtInvalidate)
{
	CAutoLock cQueueLock(&m_csLock);

	m_pSubPic = NULL;

	return S_OK;
}

STDMETHODIMP_(bool) CSubPicQueueNoThread::LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic> &ppSubPic)
{

	CComPtr<ISubPic> pSubPic;

	{
		CAutoLock cAutoLock(&m_csLock);

		if(!m_pSubPic)
		{
			if(FAILED(m_pAllocator->AllocDynamic(&m_pSubPic)))
				return(false);
		}

		pSubPic = m_pSubPic;
	}

	if(pSubPic->GetStart() <= rtNow && rtNow < pSubPic->GetStop())
	{
		ppSubPic = pSubPic;
	}
	else
	{
		CComPtr<ISubPicProvider> pSubPicProvider;
		if(SUCCEEDED(GetSubPicProvider(&pSubPicProvider)) && pSubPicProvider
		&& SUCCEEDED(pSubPicProvider->Lock()))
		{
			double fps = m_fps;

			if(POSITION pos = pSubPicProvider->GetStartPosition(rtNow, fps))
			{
				REFERENCE_TIME rtStart = pSubPicProvider->GetStart(pos, fps);
				REFERENCE_TIME rtStop = pSubPicProvider->GetStop(pos, fps);

				if(pSubPicProvider->IsAnimated(pos))
				{
					rtStart = rtNow;
					rtStop = rtNow+1;
				}

				if(rtStart <= rtNow && rtNow < rtStop)
				{
					SIZE	MaxTextureSize, VirtualSize;
					POINT	VirtualTopLeft;
					HRESULT	hr2;
					if (SUCCEEDED (hr2 = pSubPicProvider->GetTextureSize(pos, MaxTextureSize, VirtualSize, VirtualTopLeft)))
						m_pAllocator->SetMaxTextureSize(MaxTextureSize);
					
					if(m_pAllocator->IsDynamicWriteOnly())
					{
						CComPtr<ISubPic> pStatic;
						if(SUCCEEDED(m_pAllocator->GetStatic(&pStatic))
						&& SUCCEEDED(RenderTo(pStatic, rtStart, rtStop, fps, false))
						&& SUCCEEDED(pStatic->CopyTo(pSubPic)))
							ppSubPic = pSubPic;
					}
					else
					{
						if(SUCCEEDED(RenderTo(m_pSubPic, rtStart, rtStop, fps, false)))
							ppSubPic = pSubPic;
					}
					if (SUCCEEDED(hr2))
						pSubPic->SetVirtualTextureSize (VirtualSize, VirtualTopLeft);
				}
			}

			pSubPicProvider->Unlock();

			if(ppSubPic)
			{
				CAutoLock cAutoLock(&m_csLock);

				m_pSubPic = ppSubPic;
			}
		}
	}

	return(!!ppSubPic);
}

STDMETHODIMP CSubPicQueueNoThread::GetStats(int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	CAutoLock cAutoLock(&m_csLock);

	nSubPics = 0;
	rtNow = m_rtNow;
	rtStart = rtStop = 0;

	if(m_pSubPic)
	{
		nSubPics = 1;
		rtStart = m_pSubPic->GetStart();
		rtStop = m_pSubPic->GetStop();
	}

	return S_OK;
}

STDMETHODIMP CSubPicQueueNoThread::GetStats(int nSubPic, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop)
{
	CAutoLock cAutoLock(&m_csLock);

	if(!m_pSubPic || nSubPic != 0)
		return E_INVALIDARG;

	rtStart = m_pSubPic->GetStart();
	rtStop = m_pSubPic->GetStop();

	return S_OK;
}

//
// ISubPicAllocatorPresenterImpl
//

ISubPicAllocatorPresenterImpl::ISubPicAllocatorPresenterImpl(HWND hWnd, HRESULT& hr, CStdString *_pError)
	: CUnknown(NAME("ISubPicAllocatorPresenterImpl"), NULL)
	, m_hWnd(hWnd)
	, m_NativeVideoSize(0, 0), m_AspectRatio(0, 0)
	, m_VideoRect(0, 0, 0, 0), m_WindowRect(0, 0, 0, 0)
	, m_fps(25.0)
	, m_rtSubtitleDelay(0)
{
    if(!IsWindow(m_hWnd)) 
	{
		hr = E_INVALIDARG; 
		if (_pError)
			*_pError += "Invalid window handle in ISubPicAllocatorPresenterImpl\n";
		return;
	}
	GetWindowRect(m_hWnd, &m_WindowRect);
	SetVideoAngle(Vector(), false);
	hr = S_OK;
}

ISubPicAllocatorPresenterImpl::~ISubPicAllocatorPresenterImpl()
{
}

STDMETHODIMP ISubPicAllocatorPresenterImpl::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{

	return 
		QI(ISubPicAllocatorPresenter)
		QI(ISubPicAllocatorPresenter2)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void ISubPicAllocatorPresenterImpl::AlphaBltSubPic(CSize size, SubPicDesc* pTarget)
{
	CComPtr<ISubPic> pSubPic;
	if(m_pSubPicQueue->LookupSubPic(m_rtNow, pSubPic))
	{
		CRect rcSource, rcDest;
		if (SUCCEEDED (pSubPic->GetSourceAndDest(&size, rcSource, rcDest)))
			pSubPic->AlphaBlt(rcSource, rcDest, pTarget);
/*		SubPicDesc spd;
		pSubPic->GetDesc(spd);

		if(spd.w > 0 && spd.h > 0)
		{
			CRect r;
			pSubPic->GetDirtyRect(r);

			// FIXME
			r.DeflateRect(1, 1);

			CRect rDstText(
				r.left * size.cx / spd.w,
				r.top * size.cy / spd.h,
				r.right * size.cx / spd.w,
				r.bottom * size.cy / spd.h);

			pSubPic->AlphaBlt(r, rDstText, pTarget);
		}*/
	}
}

// ISubPicAllocatorPresenter

STDMETHODIMP_(SIZE) ISubPicAllocatorPresenterImpl::GetVideoSize(bool fCorrectAR)
{
	CSize VideoSize(m_NativeVideoSize);

	if(fCorrectAR && m_AspectRatio.cx > 0 && m_AspectRatio.cy > 0)
		VideoSize.cx = (LONGLONG(VideoSize.cy)*LONGLONG(m_AspectRatio.cx))/LONGLONG(m_AspectRatio.cy);

	return(VideoSize);
}

STDMETHODIMP_(void) ISubPicAllocatorPresenterImpl::SetPosition(RECT w, RECT v)
{
	bool fWindowPosChanged = !!(m_WindowRect != w);
	bool fWindowSizeChanged = !!(m_WindowRect.Size() != CRect(w).Size());

	m_WindowRect = w;

	bool fVideoRectChanged = !!(m_VideoRect != v);

	m_VideoRect = v;

	if(fWindowSizeChanged || fVideoRectChanged)
	{
		if(m_pAllocator)
		{
			m_pAllocator->SetCurSize(m_WindowRect.Size());
			m_pAllocator->SetCurVidRect(m_VideoRect);
		}

		if(m_pSubPicQueue)
		{
			m_pSubPicQueue->Invalidate();
		}
	}

	if(fWindowPosChanged || fVideoRectChanged)
		Paint(fWindowSizeChanged || fVideoRectChanged);
}

STDMETHODIMP_(void) ISubPicAllocatorPresenterImpl::SetTime(REFERENCE_TIME rtNow)
{
/*
	if(m_rtNow <= rtNow && rtNow <= m_rtNow + 1000000)
		return;
*/
	m_rtNow = rtNow - m_rtSubtitleDelay;

	if(m_pSubPicQueue)
	{
		m_pSubPicQueue->SetTime(m_rtNow);
	}
}

STDMETHODIMP_(void) ISubPicAllocatorPresenterImpl::SetSubtitleDelay(int delay_ms)
{
	m_rtSubtitleDelay = delay_ms*10000i64;
}

STDMETHODIMP_(int) ISubPicAllocatorPresenterImpl::GetSubtitleDelay()
{
	return (m_rtSubtitleDelay/10000);
}

STDMETHODIMP_(double) ISubPicAllocatorPresenterImpl::GetFPS()
{
	return(m_fps);
}

STDMETHODIMP_(void) ISubPicAllocatorPresenterImpl::SetSubPicProvider(ISubPicProvider* pSubPicProvider)
{
	m_SubPicProvider = pSubPicProvider;

	if(m_pSubPicQueue)
		m_pSubPicQueue->SetSubPicProvider(pSubPicProvider);
}

STDMETHODIMP_(void) ISubPicAllocatorPresenterImpl::Invalidate(REFERENCE_TIME rtInvalidate)
{
	if(m_pSubPicQueue)
		m_pSubPicQueue->Invalidate(rtInvalidate);
}

#include <math.h>

void ISubPicAllocatorPresenterImpl::Transform(CRect r, Vector v[4])
{
	v[0] = Vector(r.left, r.top, 0);
	v[1] = Vector(r.right, r.top, 0);
	v[2] = Vector(r.left, r.bottom, 0);
	v[3] = Vector(r.right, r.bottom, 0);

	Vector center(r.CenterPoint().x, r.CenterPoint().y, 0);
	int l = (int)(Vector(r.Size().cx, r.Size().cy, 0).Length()*1.5f)+1;

	for(int i = 0; i < 4; i++)
	{
		v[i] = m_xform << (v[i] - center);
		v[i].z = v[i].z / l + 0.5f;
		v[i].x /= v[i].z*2;
		v[i].y /= v[i].z*2;
		v[i] += center;
	}
}

STDMETHODIMP ISubPicAllocatorPresenterImpl::SetVideoAngle(Vector v, bool fRepaint)
{
	m_xform = XForm(Ray(Vector(0, 0, 0), v), Vector(1, 1, 1), false);
	if(fRepaint) Paint(true);
	return S_OK;
}
