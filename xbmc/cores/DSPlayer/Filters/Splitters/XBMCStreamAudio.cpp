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
  
}
#if 0
//We are setting the media type at the same time
void CDSAudioStream::SetAVStream(AVStream* pStream,AVFormatContext* pFmt)
{
  
  m_pStream = pStream;
  m_pAudioFormatCtx = pFmt;
  m_pAudioCodecCtx = m_pStream->codec;
  AVCodec *pCodec;

  pCodec = m_dllAvCodec.avcodec_find_decoder(m_pAudioCodecCtx->codec_id);
  int resopen = m_dllAvCodec.avcodec_open(m_pAudioCodecCtx,pCodec);
  CMediaType mt;
  mt.InitMediaType();
  mt.majortype = MEDIATYPE_Audio;
  WAVEFORMATEX *wavefmt = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
  if( m_pAudioCodecCtx->codec_id == CODEC_ID_MP3)
  {
    wavefmt->wFormatTag = WAVE_FORMAT_MPEGLAYER3;
  }
  else
  {
    ///wavefmt->wFormatTag = m_pAudioCodecCtx->codec_tag;
  }
  
  wavefmt->nChannels= m_pAudioCodecCtx->channels;
  wavefmt->nSamplesPerSec= m_pAudioCodecCtx->sample_rate;
  wavefmt->nAvgBytesPerSec= m_pAudioCodecCtx->bit_rate/8;
  wavefmt->nBlockAlign= m_pAudioCodecCtx->block_align ? m_pAudioCodecCtx->block_align : 1;
  wavefmt->wBitsPerSample= m_pAudioCodecCtx->bits_per_coded_sample;
  wavefmt->cbSize= m_pAudioCodecCtx->extradata_size;
  //if(m_pAudioCodecCtx->extradata_size)
  //  memcpy(wavefmt + 1, m_pAudioCodecCtx->extradata, m_pAudioCodecCtx->extradata_size);
  
  
  //mt.subtype.Data1 = wavefmt->wFormatTag;
  mt.lSampleSize = m_pAudioCodecCtx->bit_rate*3;
  mt.bFixedSizeSamples = 1;
  mt.bTemporalCompression = 0;
  mt.cbFormat = 18 + wavefmt->cbSize;
  mt.pbFormat = (PBYTE)wavefmt;
  //mt.SetFormat((PBYTE)wavefmt, sizeof(WAVEFORMATEX));
  mt.formattype = FORMAT_WaveFormatEx;
  mt.subtype = MEDIASUBTYPE_MP3;
  
  //mt.pbFormat = wavefmt;
  //mt.pbFormat = (PBYTE)wavefmt;

  
  

  m_mts.push_back(mt);
  
  

  
}
#endif
void CDSAudioStream::Flush()
{

  m_iCurrentPts = DVD_NOPTS_VALUE;

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
    if (( (*it).majortype == MEDIATYPE_Audio) )//&&( (*it).subtype == MEDIASUBTYPE_XVID ) && ((*it).formattype == FORMAT_VideoInfo))
      return S_OK;
  }

  return E_INVALIDARG;
  
  
  CheckPointer(pMediaType,E_POINTER);


    // we only want fixed size video
    //
    if( *(pMediaType->Type()) != MEDIATYPE_Video )
    {
        return E_INVALIDARG;
    }
    if( !pMediaType->IsFixedSize( ) ) 
    {
        return E_INVALIDARG;
    }
    if( *pMediaType->Subtype( ) != MEDIASUBTYPE_RGB24 )
    {
        return E_INVALIDARG;
    }
    if( *pMediaType->FormatType( ) != FORMAT_VideoInfo )
    {
        return E_INVALIDARG;
    }

    // Get the format area of the media type
    //
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();

    if (pvi == NULL)
    {
        return E_INVALIDARG;
    }

    if( pvi->bmiHeader.biHeight < 0 )
    {
        return E_INVALIDARG;
    }

    return S_OK;
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

  
  
  int ret = 0;
  if(ret >= 0)
    UpdateCurrentPTS();
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

void CDSAudioStream::UpdateCurrentPTS()
{
  m_iCurrentPts = DVD_NOPTS_VALUE;
  for(unsigned int i = 0; i < m_pAudioFormatCtx->nb_streams; i++)
  {
    AVStream *stream = m_pAudioFormatCtx->streams[i];
    if(stream && stream->cur_dts != (int64_t)AV_NOPTS_VALUE)
    {
      double ts = ConvertTimestamp(stream->cur_dts, stream->time_base.den, stream->time_base.num);
      if(m_iCurrentPts == DVD_NOPTS_VALUE || m_iCurrentPts > ts )
        m_iCurrentPts = ts;
    }
  }

}

double CDSAudioStream::ConvertTimestamp(int64_t pts, int den, int num)
{
  if (pts == (int64_t)AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completly exact timestamp anyway
  double timestamp = (double)pts * num  / den;
  double starttime = 0.0f;

  if (m_pAudioFormatCtx->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = (double)m_pAudioFormatCtx->start_time / AV_TIME_BASE;

  if(timestamp > starttime)
    timestamp -= starttime;
  else if( timestamp + 0.1f > starttime )
    timestamp = 0;

  return timestamp*DVD_TIME_BASE;
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
  
  CAutoLock cAutoLock(m_pFilter->pStateLock());
  BYTE *pData = 0;
  pms->GetPointer(&pData);
  long lDataLen = pms->GetSize();
  ZeroMemory(pData, lDataLen);
#if 0 
  if(m_dllAvFormat.av_read_frame(m_pAudioFormatCtx, &m_pPacket)<0)
  {
    //last frame
  }
  else
  {
    memcpy(pData,m_pPacket.data,m_pPacket.size);
  }
#endif
  pms->SetSyncPoint(TRUE);
  return NOERROR;
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