/*
 *      Copyright (C) 2010 Team XBMC
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

#pragma once

#include <vector>
#include <string>
#include "DSStreamInfo.h"
#include "PacketQueue.h"

class CDSOutputPin
  : public CBaseOutputPin
  , protected CAMThread
{
public:
  CDSOutputPin(CDSStreamInfo& hints, LPCWSTR pName, CBaseFilter *pFilter, CCritSec *pLock, HRESULT *phr, const char* container = "", int nBuffers = 0);
  ~CDSOutputPin();

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // IQualityControl
  STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) { return E_NOTIMPL; }

  // CBaseOutputPin
  HRESULT CheckConnect(IPin* pPin);
  HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);
  HRESULT CheckMediaType(const CMediaType* pmt);
  HRESULT GetMediaType(int iPosition, CMediaType* pmt);
  HRESULT Active();
  HRESULT Inactive();

  HRESULT DeliverBeginFlush();
  HRESULT DeliverEndFlush();

  HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

  int QueueCount();
  HRESULT QueuePacket(Packet *pPacket);
  HRESULT QueueEndOfStream();
  bool IsDiscontinuous();

  DWORD GetStreamId() { return m_streamId; };
  void SetStreamId(DWORD newStreamId) { m_streamId = newStreamId; };

  void SetNewMediaType(CMediaType pmt) { CAutoLock lock(&m_csMT); m_newMT = new CMediaType(pmt); }

private:
  enum {CMD_EXIT};
  DWORD ThreadProc();

  HRESULT DeliverPacket(Packet *pPacket);

private:
  CCritSec m_csMT;
  CDSStreamInfo m_hints;
  std::vector<CMediaType> m_mts;
  CPacketQueue m_queue;

  std::string m_containerFormat;

  REFERENCE_TIME m_rtStart;

  // Flush control
  bool m_fFlushing, m_fFlushed;
  CAMEvent m_eEndFlush;

  HRESULT m_hrDeliver;

  int m_nBuffers;

  DWORD m_streamId;
  CMediaType *m_newMT;
};
