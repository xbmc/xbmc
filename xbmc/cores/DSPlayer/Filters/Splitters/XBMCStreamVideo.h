#pragma once
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



#include "streams.h"


#include "DVDPlayer/DVDClock.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxFFmpeg.h"

#include "DSStreamInfo.h"
#include "DSPacketQueue.h"
class CXBMCSplitterFilter;

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

  void SetStream(CMediaType mt);
  
  // Queueing

  HANDLE GetThreadHandle() {ASSERT(m_hThread != NULL); return m_hThread;}
  void SetThreadPriority(int nPriority) {if(m_hThread) ::SetThreadPriority(m_hThread, nPriority);}
  //HRESULT Active();
  //HRESULT Inactive();
  HRESULT DeliverBeginFlush();
  HRESULT DeliverEndFlush();
  HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

  int QueueCount();
  int QueueSize();
  HRESULT QueueEndOfStream();
  HRESULT QueuePacket(std::auto_ptr<DsPacket> p);
protected:
  CMediaType m_MediaType;
  std::vector<CMediaType> m_mts;
private:
  REFERENCE_TIME m_rtStart;
  CDemuxPacketQueue m_queue;
  HRESULT m_hrDeliver;
  bool m_fFlushing, m_fFlushed;
  CAMEvent m_eEndFlush;
  enum {CMD_EXIT};
  

    
    int mWidth;
    int mHeight;

    __int64 mTime;
    __int64 mLastTime;
    double   m_iCurrentPts; // used for stream length estimation
};
