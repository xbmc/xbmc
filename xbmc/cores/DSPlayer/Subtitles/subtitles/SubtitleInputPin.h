/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "..\subpic\ISubPic.h"

//
// CSubtitleInputPin
//

class CSubtitleInputPin : public CBaseInputPin
{
  CCritSec m_csReceive;

  CCritSec* m_pSubLock;
  Com::SmartPtr<ISubStream> m_pSubStream;

protected:
  virtual void AddSubStream(ISubStream* pSubStream) = 0;
  virtual void RemoveSubStream(ISubStream* pSubStream) = 0;
  virtual void InvalidateSubtitle(REFERENCE_TIME rtStart, ISubStream* pSubStream) = 0;
  bool         IsHdmvSub(const CMediaType* pmt);

public:
  CSubtitleInputPin(CBaseFilter* pFilter, CCritSec* pLock, CCritSec* pSubLock, HRESULT* phr);

  HRESULT CheckMediaType(const CMediaType* pmt);
  HRESULT CompleteConnect(IPin* pReceivePin);
  HRESULT BreakConnect();
  STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
  STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
  STDMETHODIMP Receive(IMediaSample* pSample);

  ISubStream* GetSubStream() {return m_pSubStream;}
};
