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
#include "Thread.h"
#include "DVDMessageQueue.h"
#include "DSStreamInfo.h"
#include <deque>
class CXBMCSplitterFilter;

//********************************************************************
//
//********************************************************************
class CDSOutputPin : public CSourceStream
{
public:
  CDSOutputPin(CSource *pFilter, HRESULT* phr, LPCWSTR pName, CDSStreamInfo& hints);
  virtual ~CDSOutputPin();

  void SetNewStartTime(REFERENCE_TIME aTime);
	bool IsProcessing(void) { return m_bIsProcessing; }
	void SetProcessingFlag()  { m_bIsProcessing = true; }

  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  void WaitForBuffers();
  bool AcceptsData()                                    { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0)     { m_messageQueue.Put(pMsg, priority); }

  STDMETHODIMP Notify(IBaseFilter* pSender, Quality q){return E_NOTIMPL;}

  // classes
  CDVDMessageQueue m_messageQueue;
  //CDVDMessageQueue& m_messageParent;

  //void PushBlock(KaxBlockGroup & aBlock);

  
	HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();

  void Flush();
	void EndStream();
	void Reset();
	void DisableWriteBlock();
	void EnableWriteBlock();
	void SetPauseMode(bool bPauseMode);
protected:
  virtual HRESULT FillBuffer(IMediaSample *pSamp);
  virtual HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
  virtual HRESULT CheckMediaType(const CMediaType* pMediaType);

  // called from CBaseOutputPin during connection to ask for
  // the count and size of buffers we need.
  virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);

	virtual HRESULT OnThreadStartPlay(void);
	virtual HRESULT OnThreadDestroy(void);

	virtual HRESULT DoBufferProcessingLoop(void);

	//void SendOneHeaderPerSample(binary* CodecPrivateData, int DataLen);
	
	void UpdateFromSeek();

private:
  CDSStreamInfo m_hints;
  std::list<DVDMessageListItem> m_packets;// Packet queue
  REFERENCE_TIME          mPrevStartTime;
	REFERENCE_TIME          m_rtStart;
  bool					m_bDiscontinuity;
  std::deque<CMediaType>	m_mts;
  bool					m_bIsProcessing;
};