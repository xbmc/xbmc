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


#ifndef DSSTREAMSOURCESTREAMAUDIO_H_
#define DSSTREAMSOURCESTREAMAUDIO_H_

#include "streams.h"
#include "Codecs/DllAvFormat.h"
#include "Codecs/DllAvCodec.h"

class CXBMCSplitterFilter;


//********************************************************************
//
//********************************************************************
class CDSAudioStream : public CSourceStream
{
public:
  CDSAudioStream  (HRESULT *phr, CXBMCSplitterFilter *pParent, LPCWSTR pPinName);
  ~CDSAudioStream ();

    

  void SetAVStream(AVStream* pStream);
  // CSourceStream
  HRESULT FillBuffer(IMediaSample* _samp);
  HRESULT GetMediaType(int _position, CMediaType* _pmt);
  HRESULT CheckMediaType(const CMediaType *pMediaType);
  HRESULT SetMediaType(const CMediaType *pMediaType);
  

  // CBaseOutputPin
  HRESULT DecideBufferSize(IMemAllocator* _memAlloc, ALLOCATOR_PROPERTIES* _properties);
  //virtual HRESULT CompleteConnect(IPin *pReceivePin);
  virtual HRESULT OnThreadCreate();

    // CBasePin
  virtual HRESULT __stdcall Notify(IBaseFilter * pSender, Quality q);

    // CBaseOutputPin
    HRESULT GetDeliveryBuffer(IMediaSample ** ppSample,REFERENCE_TIME * pStartTime, REFERENCE_TIME * pEndTime, DWORD dwFlags);
protected:
  CMediaType m_MediaType;
  int m_nCurrentBitDepth;

  AVCodecContext *m_pVideoCodecCtx;

  DllAvFormat m_dllAvFormat;
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;
private:
  AVStream* m_pStream;
  std::vector<CMediaType> m_mts;

  void fillNextFrame(unsigned char* _buffer, int _buffersize, __int64& time_);
  void fillNextFrameProcedural(unsigned char* _buffer, int _buffersize, __int64& time_);
  void fillNextFrameFromStream(unsigned char* _buffer, int _buffersize, __int64& time_);
  int processNewSamplesFromStream(unsigned char* _buffer, int _samples, __int64& time_);

  int mBitsPerSample;
  int mChannels;
  int mSamplesPerSec;
  int mBytesPerSample;

  __int64 mTime;
  __int64 mLastTime;

  const unsigned char* mSBBuffer;
  int mSBAvailableSamples;

  bool mEOS;
};

#endif
