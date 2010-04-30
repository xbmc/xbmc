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

#ifndef DSSTREAMSOURCESTREAMVIDEO_H_
#define DSSTREAMSOURCESTREAMVIDEO_H_

#include "streams.h"
#include "Codecs/DllAvFormat.h"
#include "Codecs/DllAvCodec.h"
#include "DVDPlayer/DVDClock.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"
class CXBMCSplitterFilter;
struct AVPacket;
//********************************************************************
//
//********************************************************************
class CDSVideoStream : public CSourceStream
{
public:
  CDSVideoStream  (LPUNKNOWN pUnk, CXBMCSplitterFilter *pParent, HRESULT *phr);
  ~CDSVideoStream ();

  // CBaseOutputPin
  HRESULT DecideBufferSize(IMemAllocator* _memAlloc, ALLOCATOR_PROPERTIES* _properties);

  
  // CSourceStream
  HRESULT FillBuffer(IMediaSample* _samp);
  HRESULT GetMediaType(int _position, CMediaType* _pmt);
  HRESULT CheckMediaType(const CMediaType *pMediaType);
  HRESULT SetMediaType(const CMediaType *pMediaType);
  virtual HRESULT OnThreadCreate();

    // CBasePin
  HRESULT __stdcall Notify(IBaseFilter * pSender, Quality q);

  void SetAVStream(AVStream* pStream,AVFormatContext* pFmt);
  double ConvertTimestamp(int64_t pts, int den, int num);
  void UpdateCurrentPTS();
protected:
  CMediaType m_MediaType;
  int m_nCurrentBitDepth;
  AVFormatContext *m_pVideoFormatCtx;
  AVCodecContext *m_pVideoCodecCtx;
  
  DllAvFormat m_dllAvFormat;
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;
  bool m_bMatroska;
  bool m_bAVI;
private:
  AVPacket m_pPacket;
  AVStream*        m_pStream;
  std::vector<CMediaType> m_mts;
    void getSize();
    void getSizeFromStream();
    void getSizeProcedural();

    bool fillNextFrame(unsigned char* _buffer, int _buffersize, __int64& time_);
    bool fillNextFrameFromStream(unsigned char* _buffer, int _buffersize, __int64& time_);
    bool fillNextFrameProcedural(unsigned char* _buffer, int _buffersize, __int64& time_);
  void Flush();

    
    int mWidth;
    int mHeight;

    __int64 mTime;
    __int64 mLastTime;
    double   m_iCurrentPts; // used for stream length estimation
};

#endif
