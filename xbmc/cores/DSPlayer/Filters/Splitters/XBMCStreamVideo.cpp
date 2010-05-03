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



#include "XBMCStreamVideo.h"
#include "XBMCSplitter.h"
#include "moreuuids.h"
#include "dvdmedia.h"
#include "DShowUtil/DShowUtil.h"




//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CDSVideoStream::CDSVideoStream(LPUNKNOWN pUnk, CXBMCSplitterFilter *pParent, HRESULT *phr) :
CSourceStream(NAME("CDSVideoStream"),phr, pParent, L"DS Video Pin")
{
  ASSERT(phr);
  m_iCurrentPts = DVD_NOPTS_VALUE;
  mWidth = -1;
  mHeight = -1;
  mTime = 0;
  mLastTime = 0;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CDSVideoStream::~CDSVideoStream()
{
}


//
void CDSVideoStream::SetStream(CMediaType mt)
{
  
  m_mts.push_back(mt);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSVideoStream::CheckMediaType(const CMediaType *pMediaType)
{
  CheckPointer(pMediaType,E_POINTER);
  
  for(int i = 0; i < m_mts.size(); i++)
  {
    if(*pMediaType == m_mts[i])
      return S_OK;
  }
  return E_INVALIDARG;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSVideoStream::GetMediaType(int iPosition, CMediaType* pmt)
{
  //CAutoLock cAutoLock(m_pLock);
  if(iPosition < 0)
    return E_INVALIDARG;
  
  if(iPosition >= m_mts.size())
    return VFW_S_NO_MORE_ITEMS;

  *pmt = m_mts[iPosition];

  return S_OK;
}

//
// SetMediaType
//
// Called when a media type is agreed between filters
//
HRESULT CDSVideoStream::SetMediaType(const CMediaType *pMediaType)
{
  //CAutoLock cAutoLock(m_pFilter->pStateLock());
  // Pass the call up to my base class
  HRESULT hr = CSourceStream::SetMediaType(pMediaType);

  if(SUCCEEDED(hr))
  {
    VIDEOINFOHEADER * pvi = (VIDEOINFOHEADER *) m_mt.Format();
    if (pvi == NULL)
      return E_UNEXPECTED;

    switch(pvi->bmiHeader.biBitCount)
    {
      case 8:     // 8-bit palettized
      case 16:    // RGB565, RGB555
      case 24:    // RGB24
      case 32:    // RGB32
      // Save the current media type and bit depth
        m_MediaType = *pMediaType;

        hr = S_OK;
        break;
      default:
      // We should never agree any other media types
        ASSERT(FALSE);
        hr = E_INVALIDARG;
        break;
    }
  } 

  return hr;

} // SetMediaType


//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSVideoStream::FillBuffer(IMediaSample *pms)
{

  CheckPointer(pms,E_POINTER);
  if (m_queue.size() <= 0)
    return NOERROR;
  boost::shared_ptr<DsPacket> p = m_queue.Remove();
  BYTE *pData = 0;
  HRESULT hr;
  //Copy byte into the samples
  hr = pms->GetPointer(&pData);
  memcpy(pData, &p->at(0), p->size());
  //set the sample length  
  hr = pms->SetTime(&p->rtStart, &p->rtStop);
  hr = pms->SetSyncPoint(TRUE);

    return S_OK;
}

//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSVideoStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
  CheckPointer(pAlloc,E_POINTER);
  CheckPointer(pProperties,E_POINTER);

  CAutoLock cAutoLock(m_pFilter->pStateLock());
  HRESULT hr = NOERROR;

  VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
  pProperties->cBuffers = 1;
  pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

  ASSERT(pProperties->cbBuffer);

  // Ask the allocator to reserve us some sample memory, NOTE the function
  // can succeed (that is return NOERROR) but still not have allocated the
  // memory that we requested, so we must check we got whatever we wanted

  ALLOCATOR_PROPERTIES Actual;
  hr = pAlloc->SetProperties(pProperties,&Actual);
  if(FAILED(hr))
  {
    return hr;
  }

  // Is this allocator unsuitable

  if(Actual.cbBuffer < pProperties->cbBuffer)
  {
    return E_FAIL;
  }

  // Make sure that we have only 1 buffer (we erase the ball in the
  // old buffer to save having to zero a 200k+ buffer every time
  // we draw a frame)
  
  ASSERT(Actual.cBuffers == 1);

  return NOERROR;
}

//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSVideoStream::Notify(IBaseFilter * pSender, Quality q)
{

    return NOERROR;
}

extern __int64 g_chanka;

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSVideoStream::OnThreadCreate()
{
    /*if ( mLastTime > g_chanka )
        g_chanka = mLastTime;*/
    /*mLastTime = g_chanka;

    mTime = mLastTime;*/
    //mStream->setStartTimeAbs(mLastTime);


    /*CDisp disp = CDisp(CRefTime((REFERENCE_TIME)mTime));
    const char* str = disp;
    LOG("VideoLastTime(%s)",str);*/
  m_queue.RemoveAll();
  return NO_ERROR;
}

HRESULT CDSVideoStream::DeliverBeginFlush()
{
  m_eEndFlush.Reset();
  m_fFlushed = false;
  m_fFlushing = true;
  m_hrDeliver = S_FALSE;
  m_queue.RemoveAll();
  HRESULT hr = IsConnected() ? GetConnected()->BeginFlush() : S_OK;
  if(S_OK != hr) m_eEndFlush.Set();
  return(hr);
}

HRESULT CDSVideoStream::DeliverEndFlush()
{
  if(!ThreadExists()) return S_FALSE;
  HRESULT hr = IsConnected() ? GetConnected()->EndFlush() : S_OK;
  m_hrDeliver = S_OK;
  m_fFlushing = false;
  m_fFlushed = true;
  m_eEndFlush.Set();
  return hr;
}

HRESULT CDSVideoStream::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
  //m_brs.rtLastDeliverTime = Packet::INVALID_TIME;
  //if(m_fFlushing) return S_FALSE;
  m_rtStart = tStart;
  if(!ThreadExists()) return S_FALSE;
  HRESULT hr = __super::DeliverNewSegment(tStart, tStop, dRate);
  if(S_OK != hr) return hr;
  //MakeISCRHappy();
  //return hr;
}

int CDSVideoStream::QueueCount()
{
  return m_queue.size();
}

int CDSVideoStream::QueueSize()
{
  return m_queue.GetSize();
}

HRESULT CDSVideoStream::QueueEndOfStream()
{
  return QueuePacket(auto_ptr<DsPacket>()); // NULL means EndOfStream
}

HRESULT CDSVideoStream::QueuePacket(auto_ptr<DsPacket> p)
{
  //while(m_queue.GetSize() > MAXPACKETSIZE*100)
    //Sleep(1);

  //if(S_OK != m_hrDeliver)
  //  return m_hrDeliver;

  m_queue.Add(p);

  return S_OK;
}