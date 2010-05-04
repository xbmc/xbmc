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

#include "XBMCStreamAudio.h"
#include "XBMCSplitter.h"
#include "wxdebug.h"
#include "moreuuids.h"
#include "utils/Win32Exception.h"

#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "DShowUtil/DShowUtil.h"
#include "MMReg.h"
const int WaveBufferSize = 16*1024;     // Size of each allocated buffer
                                        // Originally used to be 2K, but at
                                        // 44khz/16bit/stereo you would get
                                        // audio breaks with a transform in the
                                        // middle.

const int BITS_PER_BYTE = 8;


//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CDSAudioStream::CDSAudioStream(LPUNKNOWN pUnk, CXBMCSplitterFilter *pParent, HRESULT *phr) 
: CSourceStream(NAME("CDSAudioStream"),phr, pParent, L"DS Audio Pin")
{
    ASSERT(phr);
    

    mBitsPerSample = -1;
    mChannels = -1;
    mSamplesPerSec = -1;
    mTime = 0;
    mLastTime = 0;
    mSBAvailableSamples = 0;
    mSBBuffer = 0;

    mEOS = false;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CDSAudioStream::~CDSAudioStream()
{
}

void CDSAudioStream::SetStream(CMediaType mt)
{
  m_mts.push_back(mt);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSAudioStream::CheckMediaType(const CMediaType *pMediaType)
{
  CheckPointer(pMediaType,E_POINTER);
  const CMediaType *pMT;
  for ( std::vector<CMediaType>::iterator it = m_mts.begin(); it != m_mts.end(); it++)
  {
    if (( (*it).majortype == MEDIATYPE_Audio) )
      return S_OK;
  }

  return E_INVALIDARG;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSAudioStream::GetMediaType(int iPosition, CMediaType* pmt)
{
  CAutoLock cAutoLock(m_pLock);

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
HRESULT CDSAudioStream::SetMediaType(const CMediaType *pMediaType)
{
  CAutoLock cAutoLock(m_pFilter->pStateLock());
  // Pass the call up to my base class
  HRESULT hr = CSourceStream::SetMediaType(pMediaType);

  if(SUCCEEDED(hr))
  {
    m_MediaType = *pMediaType;
    WAVEFORMATEX* waveFormat = (WAVEFORMATEX*)pMediaType->Format();
    mChannels = waveFormat->nChannels;
    mSamplesPerSec = waveFormat->nSamplesPerSec;
    mBitsPerSample = waveFormat->wBitsPerSample;
    mBytesPerSample = (mBitsPerSample/8) * mChannels;
  }
  else
  {
    ASSERT(FALSE);
    hr = E_INVALIDARG;
  }

  return hr;

} // SetMediaType

//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSAudioStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties)
{
  CheckPointer(pAlloc,E_POINTER);
  CheckPointer(pProperties,E_POINTER);

  CAutoLock cAutoLock(m_pFilter->pStateLock());
  HRESULT hr = NOERROR;
  
  WAVEFORMATEX *pwfexCurrent = (WAVEFORMATEX*)m_mt.Format();
  pProperties->cBuffers=4;
  pProperties->cbBuffer=48000*8*4/5;
  pProperties->cbAlign=1;
  pProperties->cbPrefix=0;
  
  ALLOCATOR_PROPERTIES actual;
 if(FAILED(hr=pAlloc->SetProperties(pProperties,&actual))) 
   return hr;
 return pProperties->cBuffers>actual.cBuffers || pProperties->cbBuffer>actual.cbBuffer?E_FAIL:S_OK;

    if(WAVE_FORMAT_PCM == pwfexCurrent->wFormatTag)
    {
        pProperties->cbBuffer = WaveBufferSize;
    }
    else
    {
        return E_FAIL;
    }
    /*else
    {
        // This filter only supports two formats: PCM and ADPCM. 
        ASSERT(WAVE_FORMAT_ADPCM == pwfexCurrent->wFormatTag);

        pProperties->cbBuffer = pwfexCurrent->nBlockAlign;

        MMRESULT mmr = acmStreamSize(m_hPCMToMSADPCMConversionStream,
                                     pwfexCurrent->nBlockAlign,
                                     &m_dwTempPCMBufferSize,
                                     ACM_STREAMSIZEF_DESTINATION);

        // acmStreamSize() returns 0 if no error occurs.
        if(0 != mmr)
        {
            return E_FAIL;
        }
    }*/

    int nBitsPerSample = pwfexCurrent->wBitsPerSample;
    int nSamplesPerSec = pwfexCurrent->nSamplesPerSec;
    int nChannels = pwfexCurrent->nChannels;

    pProperties->cBuffers = (nChannels * nSamplesPerSec * nBitsPerSample) / 
                            (pProperties->cbBuffer * BITS_PER_BYTE);

    // Get 1/2 second worth of buffers
    pProperties->cBuffers /= 2;
    if(pProperties->cBuffers < 1)
        pProperties->cBuffers = 1 ;

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
    {
        ASSERT(false);
        return hr;
    }

    // Is this allocator unsuitable

    if(Actual.cbBuffer < pProperties->cbBuffer)
    {
        return E_FAIL;
    }

    return NOERROR;
}

//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSAudioStream::FillBuffer(IMediaSample *pms)
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

HRESULT CDSAudioStream::DeliverBeginFlush()
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

HRESULT CDSAudioStream::DeliverEndFlush()
{
  if(!ThreadExists()) return S_FALSE;
  HRESULT hr = IsConnected() ? GetConnected()->EndFlush() : S_OK;
  m_hrDeliver = S_OK;
  m_fFlushing = false;
  m_fFlushed = true;
  m_eEndFlush.Set();
  return hr;
}

HRESULT CDSAudioStream::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
  //m_brs.rtLastDeliverTime = Packet::INVALID_TIME;
  if(m_fFlushing) 
    return S_FALSE;
  m_rtStart = tStart;
  if(!ThreadExists()) return S_FALSE;
  HRESULT hr = __super::DeliverNewSegment(tStart, tStop, dRate);
  //if(S_OK != hr) 
  //  return hr;
  //What this is doing??
  //MakeISCRHappy();
  return hr;
}

int CDSAudioStream::QueueCount()
{
  return m_queue.size();
}

int CDSAudioStream::QueueSize()
{
  return m_queue.GetSize();
}

HRESULT CDSAudioStream::QueueEndOfStream()
{
  return QueuePacket(std::auto_ptr<DsPacket>()); // NULL means EndOfStream
}

HRESULT CDSAudioStream::QueuePacket(std::auto_ptr<DsPacket> p)
{
  while(m_queue.GetSize() > MAXPACKETSIZE*100)
    Sleep(1);

  //if(S_OK != m_hrDeliver)
  //  return m_hrDeliver;

  m_queue.Add(p);

  return S_OK;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
/*HRESULT CDSAudioStream::CompleteConnect(IPin *pReceivePin)
{
    WAVEFORMATEX *pwfexCurrent = (WAVEFORMATEX*)m_mt.Format();

    return S_OK;
}*/

//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSAudioStream::Notify(IBaseFilter * pSender, Quality q)
{

    return NOERROR;
}

__int64 g_chanka = 0;

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSAudioStream::OnThreadCreate()
{ 
  //FIXME
  //mStream->setStartTimeAbs(mLastTime);

    CDisp disp = CDisp(CRefTime((REFERENCE_TIME)mLastTime));
    const char* str = disp;
    CLog::Log(LOGDEBUG,"%s AudioLastTime(%s)", __FUNCTION__, str);

    return NO_ERROR;
}


//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
/*HRESULT CDSAudioStream::GetDeliveryBuffer(IMediaSample ** ppSample,REFERENCE_TIME * pStartTime, REFERENCE_TIME * pEndTime, DWORD dwFlags)
{
    return CSourceStream::GetDeliveryBuffer(ppSample,pStartTime,pEndTime,dwFlags);
}*/