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
CDSAudioStream::CDSAudioStream(HRESULT *phr, CXBMCSplitterFilter *pParent, LPCWSTR pPinName) :
    CSourceStream(NAME("CDSAudioStream"),phr, pParent, pPinName)
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
    m_dllAvFormat.Load(); m_dllAvCodec.Load();
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CDSAudioStream::~CDSAudioStream()
{
}


//We are setting the media type at the same time
void CDSAudioStream::SetAVStream(AVStream* pStream)
{
  
  m_pStream = pStream;
  m_pVideoCodecCtx = m_pStream->codec;
  AVCodec *pCodec;

  pCodec = m_dllAvCodec.avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
  int resopen = m_dllAvCodec.avcodec_open(m_pVideoCodecCtx,pCodec);
  CMediaType mt;
  mt.InitMediaType();
  WAVEFORMATEX *wavefmt;

  wavefmt =(WAVEFORMATEX*)calloc(sizeof(WAVEFORMATEX) + m_pVideoCodecCtx->extradata_size,1);

  wavefmt->wFormatTag= m_pVideoCodecCtx->codec_tag;
  wavefmt->nChannels= m_pVideoCodecCtx->channels;
  wavefmt->nSamplesPerSec= m_pVideoCodecCtx->sample_rate;
  wavefmt->nAvgBytesPerSec= m_pVideoCodecCtx->bit_rate/8;
  wavefmt->nBlockAlign= m_pVideoCodecCtx->block_align ? m_pVideoCodecCtx->block_align : 1;
  wavefmt->wBitsPerSample= m_pVideoCodecCtx->bits_per_coded_sample;
  wavefmt->cbSize= m_pVideoCodecCtx->extradata_size;
  if(m_pVideoCodecCtx->extradata_size)
    memcpy(wavefmt + 1, m_pVideoCodecCtx->extradata, m_pVideoCodecCtx->extradata_size);
  HRESULT hr = CreateAudioMediaType(wavefmt,&mt,TRUE);

  /* end bitmapinfoheader */
  
  //mt.SetSampleSize(pvi->bmiHeader.biSizeImage);
  //memset(mt.Format(), 0, mt.FormatLength());
  //mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
  
  

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
    /*WAVEFORMATEX* waveFormat = (WAVEFORMATEX*)pMediaType->Format();
    mChannels = waveFormat->nChannels;
    mSamplesPerSec = waveFormat->nSamplesPerSec;
    mBitsPerSample = waveFormat->wBitsPerSample;
    mBytesPerSample = (mBitsPerSample/8) * mChannels;*/
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
void CDSAudioStream::fillNextFrame(unsigned char* _buffer, int _buffersize, __int64& time_)
{
    if ( m_pStream )
        fillNextFrameFromStream(_buffer,_buffersize,time_);
    else
        fillNextFrameProcedural(_buffer,_buffersize,time_);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CDSAudioStream::fillNextFrameFromStream(unsigned char* _buffer, int _buffersize, __int64& time_)
{
  //FIXME
  //ASSERT(m_pStream->getType() == NWSTREAM_TYPE_MEDIA && mStream->getSubType() == NWSTREAM_SUBTYPE_MEDIA_AUDIO);

    time_ = 0;

    while ( time_ == 0 )
    {
        int bytesPerSample = mChannels * (mBitsPerSample/8);
        int samples = _buffersize/bytesPerSample;
        ASSERT(samples > 0);
        ASSERT(_buffersize == (samples*bytesPerSample));
        unsigned char* buffer = _buffer;
        __int64 time = 0;
        bool firstIteration = true;
        while ( samples )
        {
            __int64 timeAux = 0;
            int samplesProcessed = processNewSamplesFromStream(buffer, samples, timeAux);
            samples -= samplesProcessed;
            ASSERT(samples >= 0);
            buffer += samplesProcessed*bytesPerSample;

            if ( firstIteration )
            {
                time = timeAux;
                firstIteration = false;
            }
        }
        
        time = time + 1;
        //mTime = mStream->getStartTimeAbs();
        mLastTime = time;
        if ( time >= mTime )
            time_ = time - mTime;
        else
            time_ = 0;
    }
    time_ = time_ - 1;
    
    // Log time
    /*CDisp disp = CDisp(CRefTime((REFERENCE_TIME)time_));
    const char* str = disp;
    LOG("CDSAudioStream time(%s)", str);*/
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
int CDSAudioStream::processNewSamplesFromStream(unsigned char* _buffer, int _samples, __int64& time_)
{
    int samplesProcessed = 0;

    // Este trozo esta comentado para poder hacer el cambio del INWStream al INWStreamReader
    ASSERT(false);
    // Read a new block if needed
    /*NWStreamBlockAudio* audioBlock = (NWStreamBlockAudio*)mStreamBlock;
    if ( audioBlock == 0 )
    {
        mStreamBlock = mStream->readBlock();

        if ( mStreamBlock )
        {
            ASSERT(mStreamBlock->getType() == NWSTREAM_TYPE_MEDIA && mStreamBlock->getSubType() == NWSTREAM_SUBTYPE_MEDIA_AUDIO);
            audioBlock = (NWStreamBlockAudio*)mStreamBlock;

            if ( !audioBlock->IsEnd() )
            {
                ASSERT(mBitsPerSample == audioBlock->getBitsPerSample());
                ASSERT(mChannels == audioBlock->getChannels());
                ASSERT(mSamplesPerSec == audioBlock->getSamplesPerSec());
            }
            mSBAvailableSamples = audioBlock->getSamples();
            mSBBuffer = audioBlock->getBuffer();
        }
        else
        {
            mSBAvailableSamples = 0;
            mSBBuffer = 0;
        }
    }
    

    if ( audioBlock && !audioBlock->IsEnd() )
    {
        // Calc time
        int samplesOffset = audioBlock->getSamples() - mSBAvailableSamples;
        __int64 timeOffset = ((__int64)(samplesOffset) * (__int64)(10000000)) / (__int64)(mSamplesPerSec);
        time_ = audioBlock->getTime() + timeOffset;

        
        ASSERT(_samples > 0 && mSBAvailableSamples > 0 );
        samplesProcessed = (_samples < mSBAvailableSamples) ? _samples : mSBAvailableSamples;
        mSBAvailableSamples -= samplesProcessed;
        
        int bytesPerSample = mChannels * (mBitsPerSample/8);
        int bytesToCopy = samplesProcessed*bytesPerSample;    
        
        memcpy(_buffer,mSBBuffer,bytesToCopy);
        mSBBuffer += bytesToCopy;

        if ( mSBAvailableSamples == 0 )
        {
            DISPOSE(mStreamBlock);
            mStreamBlock = 0;
            mSBAvailableSamples = 0;
            mSBBuffer = 0;
        }
    }
    else
    {
        samplesProcessed = _samples;
        int bytesPerSample = mChannels * (mBitsPerSample/8);
        int bytesToCopy = samplesProcessed*bytesPerSample;    
        memset(_buffer,0,bytesToCopy);
        time_ = mLastTime;
        if ( audioBlock && audioBlock->IsEnd() )
            mEOS = true;
    }*/

    return samplesProcessed;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CDSAudioStream::fillNextFrameProcedural(unsigned char* _buffer, int _buffersize, __int64& time_)
{
    //memset(_buffer,0,_buffersize);    
    for ( int i = 0 ; i < _buffersize ; ++i )
        _buffer[i] = rand();

    // Update time
    time_ = mTime;
    int bytesPerSample = mChannels * (mBitsPerSample/8);
    int samples = _buffersize / bytesPerSample;
    mTime += ((__int64)(samples) * (__int64)(10000000)) / (__int64)(mSamplesPerSec);
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

    if ( !mEOS )
    {
        BYTE *pData = 0;
        pms->GetPointer(&pData);
        long lDataLen = pms->GetSize();

        ZeroMemory(pData, lDataLen);
        {
            __int64 time = 0;
            fillNextFrame(pData,lDataLen,time);

            // Set time
            REFERENCE_TIME timeStart = (REFERENCE_TIME)time;
            REFERENCE_TIME timeEnd = timeStart;
            pms->SetTime(&timeStart,&timeEnd);
        }

        pms->SetSyncPoint(TRUE);

        HRESULT hr = pms->SetActualDataLength(pms->GetSize());
        if (FAILED(hr))
            return hr;
    }
    else
    {
        return S_FALSE;
    }

    // Set the sample's properties.
    /*hr = pms->SetPreroll(FALSE);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pms->SetMediaType(NULL);
    if (FAILED(hr)) {
        return hr;
    }
   
    hr = pms->SetDiscontinuity(!m_fFirstSampleDelivered);
    if (FAILED(hr)) {
        return hr;
    }
    
    hr = pms->SetSyncPoint(!m_fFirstSampleDelivered);
    if (FAILED(hr)) {
        return hr;
    }*/


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
HRESULT CDSAudioStream::GetDeliveryBuffer(IMediaSample ** ppSample,REFERENCE_TIME * pStartTime, REFERENCE_TIME * pEndTime, DWORD dwFlags)
{
    return CSourceStream::GetDeliveryBuffer(ppSample,pStartTime,pEndTime,dwFlags);
}
