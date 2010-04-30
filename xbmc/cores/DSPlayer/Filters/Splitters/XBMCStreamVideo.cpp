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
#include "utils/Win32Exception.h"

#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"

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
  m_bMatroska = false;
  m_bAVI = true;
    mWidth = -1;
    mHeight = -1;
    mTime = 0;
    mLastTime = 0;
    m_dllAvFormat.Load(); m_dllAvCodec.Load();
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
CDSVideoStream::~CDSVideoStream()
{
}

//We are setting the media type at the same time
void CDSVideoStream::SetAVStream(AVStream* pStream,AVFormatContext* pFmt)
{
  
  m_pStream = pStream;
  m_pVideoFormatCtx = pFmt;
  m_pVideoCodecCtx = m_pStream->codec;
  AVCodec *pCodec;

  pCodec = m_dllAvCodec.avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
  int resopen = m_dllAvCodec.avcodec_open(m_pVideoCodecCtx,pCodec);
  //m_dllAvFormat.av
  //VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)pMediaType->AllocFormatBuffer(SIZE_VIDEOHEADER + m_cbBitmapInfo);
  
  AVFrame *pFrame;

  pFrame=m_dllAvCodec.avcodec_alloc_frame();
  int numbytes;
  numbytes = m_dllAvCodec.avpicture_get_size(m_pVideoCodecCtx->pix_fmt,m_pVideoCodecCtx->width,m_pVideoCodecCtx->height);
      


    //m_pStream->codec->
  CMediaType mt;
  mt.InitMediaType();
  VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(SIZE_PREHEADER + numbytes);
  if (pvi == 0) 
    return;//E_OUTOFMEMORY
  ZeroMemory(pvi, mt.cbFormat);
  pvi->AvgTimePerFrame = 417083;
  //setting the BITMAPINFOHEADER
  mt.SetType(&MEDIATYPE_Video);
  mt.SetSubtype(&MEDIASUBTYPE_XVID);
  mt.SetFormatType(&FORMAT_VideoInfo);
  mt.SetTemporalCompression(FALSE);
  BITMAPINFOHEADER *bmphdr;
  bmphdr=(BITMAPINFOHEADER*)calloc(sizeof(BITMAPINFOHEADER) + m_pVideoCodecCtx->extradata_size,1);
  bmphdr->biSize= sizeof(BITMAPINFOHEADER) + m_pVideoCodecCtx->extradata_size;
  bmphdr->biWidth= m_pVideoCodecCtx->width;
  bmphdr->biHeight= m_pVideoCodecCtx->height;
  bmphdr->biBitCount= m_pVideoCodecCtx->bits_per_coded_sample;
  bmphdr->biSizeImage = bmphdr->biWidth * bmphdr->biHeight * bmphdr->biBitCount/8;
  bmphdr->biCompression= m_pVideoCodecCtx->codec_tag;
  
  memcpy(&(pvi->bmiHeader), bmphdr, sizeof(bmphdr));
  CopyMemory(HEADER(pvi),bmphdr,sizeof(BITMAPINFOHEADER));
  /* end bitmapinfoheader */
  
  //mt.SetSampleSize(pvi->bmiHeader.biSizeImage);
  //memset(mt.Format(), 0, mt.FormatLength());
  //mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
  
  

  m_mts.push_back(mt);
  
  

  
}
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
HRESULT CDSVideoStream::CheckMediaType(const CMediaType *pMediaType)
{
  CheckPointer(pMediaType,E_POINTER);
  const CMediaType *pMT;
  for ( std::vector<CMediaType>::iterator it = m_mts.begin(); it != m_mts.end(); it++)
  {
    if (( (*it).majortype == MEDIATYPE_Video) &&( (*it).subtype == MEDIASUBTYPE_XVID ) && ((*it).formattype == FORMAT_VideoInfo))
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
HRESULT CDSVideoStream::GetMediaType(int iPosition, CMediaType* pmt)
{
  CAutoLock cAutoLock(m_pLock);

  if(iPosition < 0)
    return E_INVALIDARG;
  
  if(iPosition >= m_mts.size())
    return VFW_S_NO_MORE_ITEMS;

  *pmt = m_mts[iPosition];

  return S_OK;
    // Extract the size from the first queued frame (which is an
    // special frame to send the size)
    if ( mWidth == -1 && mHeight == -1 )
    {
        getSize();
    }

    VIDEOINFOHEADER vih;
    memset( &vih, 0, sizeof( vih ) );
    
    vih.bmiHeader.biCompression = BI_RGB;
    vih.bmiHeader.biBitCount    = 24;
    vih.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    vih.bmiHeader.biWidth        = mWidth;
    vih.bmiHeader.biHeight       = mHeight;
    vih.bmiHeader.biPlanes       = 1;
    vih.bmiHeader.biSizeImage    = GetBitmapSize(&vih.bmiHeader);
    vih.bmiHeader.biClrImportant = 0;
    vih.AvgTimePerFrame = 0;//UNITS * 1 / 15; // TODO

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetFormat( (BYTE*) &vih, sizeof( vih ) );
    pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
    pmt->SetSampleSize(vih.bmiHeader.biSizeImage);

    return NOERROR;
}

//
// SetMediaType
//
// Called when a media type is agreed between filters
//
HRESULT CDSVideoStream::SetMediaType(const CMediaType *pMediaType)
{
  CAutoLock cAutoLock(m_pFilter->pStateLock());
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
        m_nCurrentBitDepth = pvi->bmiHeader.biBitCount;
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

    BYTE *pData = 0;
    pms->GetPointer(&pData);
    long lDataLen = pms->GetSize();

    ZeroMemory(pData, lDataLen);
    {
        __int64 time = 0;
        if ( fillNextFrame(pData,lDataLen,time) )
        {
            // Set time
            REFERENCE_TIME timeStart = (REFERENCE_TIME)time;
            REFERENCE_TIME timeEnd = timeStart;
            pms->SetTime(&timeStart,&timeEnd);
        }
        else
            return S_FALSE;
    }

    pms->SetSyncPoint(TRUE);
    return NOERROR;
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



//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CDSVideoStream::getSize()
{
  
  if ( m_pStream )
  {
    mWidth = m_pStream->codec->width;
    mHeight = m_pStream->codec->height;      
  }
  else
    getSizeProcedural();
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CDSVideoStream::getSizeFromStream()
{
    ASSERT(false);
    /*INWStreamBlock* streamBlock = mStream->readBlock(false);
    ASSERT(streamBlock->getType() == NWSTREAM_TYPE_MEDIA && streamBlock->getSubType() == NWSTREAM_SUBTYPE_MEDIA_VIDEO);
    NWStreamBlockVideo* videoBlock = (NWStreamBlockVideo*)streamBlock;

    ASSERT(videoBlock->getFrameBuffer() == 0);

    mWidth = videoBlock->getWidth();
    mHeight = videoBlock->getHeight();

    DISPOSE(streamBlock);*/
}


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void CDSVideoStream::getSizeProcedural()
{
    mWidth = 320;
    mHeight = 240;
}



void CDSVideoStream::Flush()
{
  if (m_pVideoFormatCtx)
    m_dllAvFormat.av_read_frame_flush(m_pVideoFormatCtx);

  m_iCurrentPts = DVD_NOPTS_VALUE;

}
//********************************************************************
//
//********************************************************************
//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
bool CDSVideoStream::fillNextFrame(unsigned char* _buffer, int _buffersize, __int64& time_)
{
    if ( m_pStream )
        return fillNextFrameFromStream(_buffer,_buffersize,time_);
    else
        return fillNextFrameProcedural(_buffer,_buffersize,time_);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
bool CDSVideoStream::fillNextFrameFromStream(unsigned char* _buffer, int _buffersize, __int64& time_)
{
    bool EOS = false;
    time_ = 0;

  // Este trozo esta comentado para poder hacer el cambio del INWStream al INWStreamReader
  //ASSERT(false);
  
  AVPacket pkt;
  DemuxPacket* pPacket = NULL;
  bool bReturnEmpty = false;
  if (m_pVideoFormatCtx)
  {  
    pkt.size = 0;
    pkt.data = NULL;
    pkt.stream_index = MAX_STREAMS;
    int result = 0;
    try
    {
      result = m_dllAvFormat.av_read_frame(m_pVideoFormatCtx, &pkt);
    }
    catch(const win32_exception &e)
    {
      e.writelog(__FUNCTION__);
      result = AVERROR(EFAULT);
    }
    
  
  if (result == AVERROR(EINTR) || result == AVERROR(EAGAIN))
    {
      // timeout, probably no real error, return empty packet
      bReturnEmpty = true;
    }
    else if (result < 0)
    {
      Flush();
    }
    else if (pkt.size < 0 || pkt.stream_index >= MAX_STREAMS)
    {
      // XXX, in some cases ffmpeg returns a negative packet size
      if(m_pVideoFormatCtx->pb && !m_pVideoFormatCtx->pb->eof_reached)
      {
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() no valid packet");
        bReturnEmpty = true;
        Flush();
      }
      else
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() returned invalid packet and eof reached");

      m_dllAvCodec.av_free_packet(&pkt);
    }
    else
    {
      AVStream *stream = m_pVideoFormatCtx->streams[pkt.stream_index];

      if (m_pVideoFormatCtx->nb_programs)
      {
        /* check so packet belongs to selected program */
        /*for (unsigned int i = 0; i < m_pVideoFormatCtx->programs[m_program]->nb_stream_indexes; i++)
        {
          if(pkt.stream_index == (int)m_pVideoFormatCtx->programs[m_program]->stream_index[i])
          {
            pPacket = CDVDDemuxUtils::AllocateDemuxPacket(pkt.size);
            break;
          }
        }*/

        if (!pPacket)
          bReturnEmpty = true;
      }
      else
        pPacket = CDVDDemuxUtils::AllocateDemuxPacket(pkt.size);

      if (pPacket)
      {
        // lavf sometimes bugs out and gives 0 dts/pts instead of no dts/pts
        // since this could only happens on initial frame under normal
        // circomstances, let's assume it is wrong all the time
        if(pkt.dts == 0)
          pkt.dts = AV_NOPTS_VALUE;
        if(pkt.pts == 0)
          pkt.pts = AV_NOPTS_VALUE;

        if(m_bMatroska && stream->codec && stream->codec->codec_type == CODEC_TYPE_VIDEO)
        { // matroska can store different timestamps
          // for different formats, for native stored
          // stuff it is pts, but for ms compatibility
          // tracks, it is really dts. sadly ffmpeg
          // sets these two timestamps equal all the
          // time, so we select it here instead
          if(stream->codec->codec_tag == 0)
            pkt.dts = AV_NOPTS_VALUE;
          else
            pkt.pts = AV_NOPTS_VALUE;
        }

        // we need to get duration slightly different for matroska embedded text subtitels
        if(m_bMatroska && stream->codec->codec_id == CODEC_ID_TEXT && pkt.convergence_duration != 0)
            pkt.duration = pkt.convergence_duration;

        if(m_bAVI && stream->codec && stream->codec->codec_type == CODEC_TYPE_VIDEO)
        {
          // AVI's always have borked pts, specially if m_pVideoFormatCtx->flags includes
          // AVFMT_FLAG_GENPTS so always use dts
          pkt.pts = AV_NOPTS_VALUE;
        }

        // copy contents into our own packet
        pPacket->iSize = pkt.size;

        // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
        if (pkt.data)
          memcpy(pPacket->pData, pkt.data, pPacket->iSize);

        pPacket->pts = ConvertTimestamp(pkt.pts, stream->time_base.den, stream->time_base.num);
        pPacket->dts = ConvertTimestamp(pkt.dts, stream->time_base.den, stream->time_base.num);
        pPacket->duration =  DVD_SEC_TO_TIME((double)pkt.duration * stream->time_base.num / stream->time_base.den);

        // used to guess streamlength
        if (pPacket->dts != DVD_NOPTS_VALUE && (pPacket->dts > m_iCurrentPts || m_iCurrentPts == DVD_NOPTS_VALUE))
          m_iCurrentPts = pPacket->dts;


        // check if stream has passed full duration, needed for live streams
        if(pkt.dts != (int64_t)AV_NOPTS_VALUE)
        {
            int64_t duration;
            duration = pkt.dts;
            if(stream->start_time != (int64_t)AV_NOPTS_VALUE)
              duration -= stream->start_time;

            if(duration > stream->duration)
            {
              stream->duration = duration;
              duration = m_dllAvUtil.av_rescale_rnd(stream->duration, stream->time_base.num * AV_TIME_BASE, stream->time_base.den, AV_ROUND_NEAR_INF);
              if ((m_pVideoFormatCtx->duration == (int64_t)AV_NOPTS_VALUE && m_pVideoFormatCtx->file_size > 0)
              ||  (m_pVideoFormatCtx->duration != (int64_t)AV_NOPTS_VALUE && duration > m_pVideoFormatCtx->duration))
                m_pVideoFormatCtx->duration = duration;
            }
        }

        // check if stream seem to have grown since start
        if(m_pVideoFormatCtx->file_size > 0 && m_pVideoFormatCtx->pb)
        {
          if(m_pVideoFormatCtx->pb->pos > m_pVideoFormatCtx->file_size)
            m_pVideoFormatCtx->file_size = m_pVideoFormatCtx->pb->pos;
        }

        pPacket->iStreamId = pkt.stream_index; // XXX just for now
      }
      m_dllAvCodec.av_free_packet(&pkt);
    }
  }
    return !EOS;
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
bool CDSVideoStream::fillNextFrameProcedural(unsigned char* _buffer, int _buffersize, __int64& time_)
{
    for ( int i = 0 ; i < _buffersize ; ++i )
    {
        _buffer[i] = rand();
    }

    time_ = mTime;
    //LOG("New frame (%d)\n",time_);
    mTime += 666666; // 15 fps

    return true;
}

double CDSVideoStream::ConvertTimestamp(int64_t pts, int den, int num)
{
  if (pts == (int64_t)AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completly exact timestamp anyway
  double timestamp = (double)pts * num  / den;
  double starttime = 0.0f;

  if (m_pVideoFormatCtx->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = (double)m_pVideoFormatCtx->start_time / AV_TIME_BASE;

  if(timestamp > starttime)
    timestamp -= starttime;
  else if( timestamp + 0.1f > starttime )
    timestamp = 0;

  return timestamp*DVD_TIME_BASE;
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

    return NO_ERROR;
}

