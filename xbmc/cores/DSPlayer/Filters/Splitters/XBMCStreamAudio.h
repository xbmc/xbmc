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
  CDSAudioStream(LPUNKNOWN pUnk, CXBMCSplitterFilter *pParent, HRESULT *phr);
  ~CDSAudioStream ();

    
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

   void SetStream(CMediaType mt);
  double ConvertTimestamp(int64_t pts, int den, int num);
  void UpdateCurrentPTS();
  
protected:
  CMediaType m_MediaType;
  AVFormatContext *m_pAudioFormatCtx;
  AVCodecContext *m_pAudioCodecCtx;

  DllAvFormat m_dllAvFormat;
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;
  std::vector<CMediaType> m_mts;
private:
  void Flush();
  int mBitsPerSample;
  int mChannels;
  int mSamplesPerSec;
  int mBytesPerSample;

  __int64 mTime;
  __int64 mLastTime;

  const unsigned char* mSBBuffer;
  int mSBAvailableSamples;

  bool mEOS;
  double   m_iCurrentPts; // used for stream length estimation
};

#endif
