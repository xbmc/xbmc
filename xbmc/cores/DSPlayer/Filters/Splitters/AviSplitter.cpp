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


#include <mmreg.h>
#include "AviFile.h"
#include "AviSplitter.h"

//
// CAviSplitterFilter
//

CAviSplitterFilter::CAviSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
  : CBaseSplitterFilter(NAME("CAviSplitterFilter"), pUnk, phr, __uuidof(this))
  , m_timeformat(TIME_FORMAT_MEDIA_TIME)
{
}

STDMETHODIMP CAviSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);

  *ppv = NULL;

  return 
    __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAviSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
  CheckPointer(pAsyncReader, E_POINTER);

  HRESULT hr = E_FAIL;

  m_pFile.reset(NULL);//.Free();
  m_tFrame.Free();

  m_pFile.reset(DNew CAviFile(pAsyncReader, hr));
  if(!m_pFile.get()) return E_OUTOFMEMORY;

  bool fShiftDown = !!(::GetKeyState(VK_SHIFT)&0x8000);
  bool fShowWarningText = !m_pFile->IsInterleaved(fShiftDown);

  if(SUCCEEDED(hr) && (fShowWarningText || fShiftDown))
  {
    bool fHideWarning = true;//!!AfxGetApp()->GetProfileInt(_T("Settings"), _T("HideAviSplitterWarning"), 0);

    if(!fHideWarning && !dynamic_cast<CAviSourceFilter*>(this) || fShiftDown)
    {
      //THIS IS THE ERROR WINDOW
      //CAviReportWnd wnd;
      //fHideWarning = wnd.DoModal(m_pFile, fHideWarning, fShowWarningText);
      //AfxGetApp()->WriteProfileInt(_T("Settings"), _T("HideAviSplitterWarning"), fHideWarning);
    }

    if(fShowWarningText) hr = E_FAIL;
  }

  if(FAILED(hr)) 
  {
    m_pFile.release();
    return hr;
  }

  m_rtNewStart = m_rtCurrent = 0;
  m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->GetTotalTime();

  bool fHasIndex = false;

  for(DWORD i = 0; !fHasIndex && i < m_pFile->m_strms.size(); i++)
    if(m_pFile->m_strms[i]->cs.size() > 0) 
      fHasIndex = true;

  for(DWORD i = 0; i < m_pFile->m_strms.size(); i++)
  {
    CAviFile::strm_t* s = m_pFile->m_strms[i].get();

    if(fHasIndex && s->cs.size() == 0) continue;

    CMediaType mt;
    vector<CMediaType> mts;
    
    CStdStringW name, label;

    if(s->strh.fccType == FCC('vids'))
    {
      label = L"Video";

      ASSERT(s->strf.size() >= sizeof(BITMAPINFOHEADER));
      BITMAPINFOHEADER* pbmi = &((BITMAPINFO*)&s->strf[0])->bmiHeader;

      mt.majortype = MEDIATYPE_Video;
      mt.subtype = FOURCCMap(pbmi->biCompression);
      mt.formattype = FORMAT_VideoInfo;
      VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + s->strf.size() - sizeof(BITMAPINFOHEADER));
      memset(mt.Format(), 0, mt.FormatLength());
      memcpy(&pvih->bmiHeader,&s->strf[0],s->strf.size());
      //memcpy(&pvih->bmiHeader, s->strf.GetData(), s->strf.size());
      if(s->strh.dwRate > 0) pvih->AvgTimePerFrame = 10000000i64 * s->strh.dwScale / s->strh.dwRate;
      switch(pbmi->biCompression)
      {
      case BI_RGB: case BI_BITFIELDS: mt.subtype = 
            pbmi->biBitCount == 1 ? MEDIASUBTYPE_RGB1 :
            pbmi->biBitCount == 4 ? MEDIASUBTYPE_RGB4 :
            pbmi->biBitCount == 8 ? MEDIASUBTYPE_RGB8 :
            pbmi->biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
            pbmi->biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
            pbmi->biBitCount == 32 ? MEDIASUBTYPE_ARGB32 :
            MEDIASUBTYPE_NULL;
            break;
//      case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
//      case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
      }

      if(s->cs.size() && pvih->AvgTimePerFrame > 0)
      {
        __int64 size = 0;
        for(int i = 0; i < s->cs.size(); i++)
          size += s->cs[i].orgsize;
        pvih->dwBitRate = size*8 / s->cs.size() * 10000000i64 / pvih->AvgTimePerFrame;
      }

      mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
        ? s->strh.dwSuggestedBufferSize*3/2
        : (pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*4));
      mts.push_back(mt);
    }
    else if(s->strh.fccType == FCC('auds') || s->strh.fccType == FCC('amva'))
    {
      label = L"Audio";

      ASSERT(s->strf.size() >= sizeof(WAVEFORMATEX)
        || s->strf.size() == sizeof(PCMWAVEFORMAT));

      WAVEFORMATEX* pwfe = (WAVEFORMATEX*)&s->strf[0];
      if(pwfe->nBlockAlign == 0) continue;

      mt.majortype = MEDIATYPE_Audio;
      if (m_pFile->m_isamv)
        mt.subtype = FOURCCMap(MAKEFOURCC('A','M','V','A'));
      else
        mt.subtype = FOURCCMap(pwfe->wFormatTag);
      mt.formattype = FORMAT_WaveFormatEx;
      mt.SetFormat(&s->strf[0], dsmax(s->strf.size(), sizeof(WAVEFORMATEX)));
      pwfe = (WAVEFORMATEX*)mt.Format();
      if(s->strf.size() == sizeof(PCMWAVEFORMAT)) pwfe->cbSize = 0;
      if(pwfe->wFormatTag == WAVE_FORMAT_PCM) pwfe->nBlockAlign = pwfe->nChannels*pwfe->wBitsPerSample>>3;
      if(pwfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) mt.subtype = FOURCCMap(WAVE_FORMAT_PCM); // audio renderer doesn't accept fffe in the subtype
      mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
        ? s->strh.dwSuggestedBufferSize*3/2
        : (pwfe->nChannels*pwfe->nSamplesPerSec*32>>3));
      mts.push_back(mt);
    }
    else if(s->strh.fccType == FCC('mids'))
    {
      label = L"Midi";

      mt.majortype = MEDIATYPE_Midi;
      mt.subtype = MEDIASUBTYPE_NULL;
      mt.formattype = FORMAT_None;
      mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
        ? s->strh.dwSuggestedBufferSize*3/2
        : (1024*1024));
      mts.push_back(mt);
    }
    else if(s->strh.fccType == FCC('txts'))
    {
      label = L"Text";

      mt.majortype = MEDIATYPE_Text;
      mt.subtype = MEDIASUBTYPE_NULL;
      mt.formattype = FORMAT_None;
      mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
        ? s->strh.dwSuggestedBufferSize*3/2
        : (1024*1024));
      mts.push_back(mt);
    }
    else if(s->strh.fccType == FCC('iavs'))
    {
      label = L"Interleaved";

      ASSERT(s->strh.fccHandler == FCC('dvsd'));

      mt.majortype = MEDIATYPE_Interleaved;
      mt.subtype = FOURCCMap(s->strh.fccHandler);
      mt.formattype = FORMAT_DvInfo;
      //mt.SetFormat(s->strf.GetData(), dsmax(s->strf.size(), sizeof(DVINFO)));
      BYTE *newFmt;
      memcpy(newFmt, &s->strf[0], s->strf.size());
      mt.SetFormat(newFmt, dsmax(s->strf.size(), sizeof(DVINFO)));
      mt.SetSampleSize(s->strh.dwSuggestedBufferSize > 0 
        ? s->strh.dwSuggestedBufferSize*3/2
        : (1024*1024));
      mts.push_back(mt);
    }

    if(mts.empty())
    {
      CLog::Log(LOGERROR,"CAviSourceFilter: Unsupported stream (%d)",i);
      continue;
    }

    //Put filename at front sometime(eg. ~temp.avi) will cause filter graph
    //stop check this pin. Not sure the reason exactly. but it happens.
    //If you know why, please emailto: tomasen@gmail.com 
    if(s->strn.IsEmpty())
      name.Format(L"%s %d", label  , i);
    else
      name.Format(L"%s %d %s", label  , i , CStdStringW(s->strn) );

    HRESULT hr;

    auto_ptr<CBaseSplitterOutputPin> pPinOut(DNew CAviSplitterOutputPin(mts, name, this, this, &hr));
    AddOutputPin(i, pPinOut);
  }
  //std::map<DWORD, CStdStringA> m_pFile->m_info.begin;
  std::map<DWORD,CStdStringA>::iterator it;
  for ( it=m_pFile->m_info.begin() ; it != m_pFile->m_info.end(); it++ )
  {
    DWORD fcc;
    CStdStringW value = DShowUtil::AToW((*it).second);
    
    switch((*it).first)
    {
    case FCC('INAM'): 
      SetProperty(L"TITL", CStdStringW(value)); 
      break;
    case FCC('IART'): 
      SetProperty(L"AUTH", CStdStringW(value)); 
      break;
    case FCC('ICOP'): 
      SetProperty(L"CPYR", CStdStringW(value)); 
      break;
    case FCC('ISBJ'): 
      SetProperty(L"DESC", CStdStringW(value)); 
      break;
    }
  }

  m_tFrame.Attach(DNew DWORD[m_pFile->m_avih.dwStreams]);

  return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CAviSplitterFilter::DemuxInit()
{
  if(!m_pFile.get()) return(false);

  // reindex if needed

  bool fReIndex = false;

  for(int i = 0; i < (int)m_pFile->m_avih.dwStreams && !fReIndex; i++)
  {
    if(m_pFile->m_strms[i]->cs.size() == 0 && GetOutputPin(i)) 
      fReIndex = true;
  }

  if(fReIndex)
  {
    m_pFile->EmptyIndex();

    m_fAbort = false;
    m_nOpenProgress = 0;

    m_rtDuration = 0;

    Com::SmartAutoVectorPtr<UINT64> pSize;
    pSize.Allocate(m_pFile->m_avih.dwStreams);
    memset((UINT64*)pSize, 0, sizeof(UINT64)*m_pFile->m_avih.dwStreams);
    m_pFile->Seek(0);
        ReIndex(m_pFile->GetLength(), pSize);

    if(m_fAbort) m_pFile->EmptyIndex();

    m_fAbort = false;
    m_nOpenProgress = 100;
  }

  return(true);
}

HRESULT CAviSplitterFilter::ReIndex(__int64 end, UINT64* pSize)
{
  HRESULT hr = S_OK;

  while(S_OK == hr && m_pFile->GetPos() < end && SUCCEEDED(hr) && !m_fAbort)
  {
    __int64 pos = m_pFile->GetPos();

    DWORD id = 0, size;
    if(S_OK != m_pFile->Read(id) || id == 0)
      return E_FAIL;

    if(id == FCC('RIFF') || id == FCC('LIST'))
    {
      if(S_OK != m_pFile->Read(size) || S_OK != m_pFile->Read(id))
        return E_FAIL;

      size += (size&1) + 8;

      if(id == FCC('AVI ') || id == FCC('AVIX') || id == FCC('movi') || id == FCC('rec '))
        hr = ReIndex(pos + size, pSize);
    }
    else
    {
      if(S_OK != m_pFile->Read(size))
        return E_FAIL;

      DWORD TrackNumber = TRACKNUM(id);

      if(TrackNumber < m_pFile->m_strms.size())
      {
        CAviFile::strm_t* s = m_pFile->m_strms[TrackNumber].get();

        WORD type = TRACKTYPE(id);

        if(type == 'db' || type == 'dc' || /*type == 'pc' ||*/ type == 'wb'
        || type == 'iv' || type == '__' || type == 'xx')
        {
          CAviFile::strm_t::chunk c;
          c.filepos = pos;
          c.size = pSize[TrackNumber];
          c.orgsize = size;
          c.fKeyFrame = size > 0; // TODO: find a better way...
          c.fChunkHdr = true;
          s->cs.push_back(c);

          pSize[TrackNumber] += s->GetChunkSize(size);

          REFERENCE_TIME rt = s->GetRefTime(s->cs.size()-1, pSize[TrackNumber]);
          m_rtDuration = dsmax(rt, m_rtDuration);
        }
      }

      size += (size&1) + 8;
    }

    m_pFile->Seek(pos + size);

    m_nOpenProgress = m_pFile->GetPos()*100/m_pFile->GetLength();

    DWORD cmd;
    if(CheckRequest(&cmd))
    {
      if(cmd == CMD_EXIT) m_fAbort = true;
      else Reply(S_OK);
    }
  }

  return hr;
}

void CAviSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
  memset((DWORD*)m_tFrame, 0, sizeof(DWORD)*m_pFile->m_avih.dwStreams);
  m_pFile->Seek(0);

  DbgLog((LOG_TRACE, 0, _T("Seek: %I64d"), rt/10000));

  if(rt > 0)
  {
    UINT64 minfp = _I64_MAX;

    for(int j = 0; j < (int)m_pFile->m_strms.size(); j++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[j].get();

      int f = s->GetKeyFrame(rt);
      UINT64 fp = f >= 0 ? s->cs[f].filepos : m_pFile->GetLength();

      if(!s->IsRawSubtitleStream())
        minfp = dsmin(minfp, fp);
    }

    for(int j = 0; j < (int)m_pFile->m_strms.size(); j++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[j].get();

      for(int i = 0; i < s->cs.size(); i++)
      {
        CAviFile::strm_t::chunk& c = s->cs[i];
        if(c.filepos >= minfp)
        {
          m_tFrame[j] = i;
          break;
        }
      }
    }

    DbgLog((LOG_TRACE, 0, _T("minfp: %I64d"), minfp));
  }
}

bool CAviSplitterFilter::DemuxLoop()
{
  HRESULT hr = S_OK;

  int nTracks = (int)m_pFile->m_strms.size();

  vector<BOOL> fDiscontinuity;
  fDiscontinuity.resize(nTracks);
  memset(&fDiscontinuity[0], 0, nTracks*sizeof(bool));

  while(SUCCEEDED(hr) && !CheckRequest(NULL))
  {
    int minTrack = nTracks;
    UINT64 minFilePos = _I64_MAX;

    for(int i = 0; i < nTracks; i++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[i].get();

      DWORD f = m_tFrame[i];
      if(f >= (DWORD)s->cs.size()) 
        continue;

      bool fUrgent = s->IsRawSubtitleStream();

      if(fUrgent || s->cs[f].filepos < minFilePos)
      {
        minTrack = i;
        minFilePos = s->cs[f].filepos;
      }

      if(fUrgent) 
        break;
    }

    if(minTrack == nTracks)
      break;

    DWORD& f = m_tFrame[minTrack];

    do
    {
      CAviFile::strm_t* s = m_pFile->m_strms[minTrack].get();

      m_pFile->Seek(s->cs[f].filepos);

      DWORD size = 0;

      if(s->cs[f].fChunkHdr)
      {
        DWORD id = 0;
        if(S_OK != m_pFile->Read(id) || id == 0 || minTrack != TRACKNUM(id)
        || S_OK != m_pFile->Read(size))
        {
          fDiscontinuity[minTrack] = true;
          break;
        }

        UINT64 expectedsize = -1;
        expectedsize = f < (DWORD)s->cs.size()-1
          ? s->cs[f+1].size - s->cs[f].size
          : s->totalsize - s->cs[f].size;

        if(expectedsize != s->GetChunkSize(size))
        {
          fDiscontinuity[minTrack] = true;
          // ASSERT(0);
          break;
        }
      }
      else
      {
        size = s->cs[f].orgsize;
      }

      auto_ptr<Packet> p(DNew Packet());

      p->TrackNumber = minTrack;
      p->bSyncPoint = (BOOL)s->cs[f].fKeyFrame;
      p->bDiscontinuity = fDiscontinuity[minTrack];
      p->rtStart = s->GetRefTime(f, s->cs[f].size);
      p->rtStop = s->GetRefTime(f+1, f+1 < (DWORD)s->cs.size() ? s->cs[f+1].size : s->totalsize);
      
      p->resize(size);
      if(S_OK != (hr = m_pFile->ByteRead(&p->at(0), p->size()))) 
        return(true); // break;
/*
      DbgLog((LOG_TRACE, 0, _T("%d (%d): %I64d - %I64d, %I64d - %I64d (size = %d)"), 
        minTrack, (int)p->bSyncPoint,
        (p->rtStart)/10000, (p->rtStop)/10000, 
        (p->rtStart-m_rtStart)/10000, (p->rtStop-m_rtStart)/10000,
        size));
*/
      hr = DeliverPacket(p);

      fDiscontinuity[minTrack] = false;
    }
    while(0);

    f++;
  }

  return(true);
}

// IMediaSeeking

STDMETHODIMP CAviSplitterFilter::GetDuration(LONGLONG* pDuration)
{
  CheckPointer(pDuration, E_POINTER);
  CheckPointer(m_pFile.get(), VFW_E_NOT_CONNECTED);

  if(m_timeformat == TIME_FORMAT_FRAME)
  {
    for(int i = 0; i < (int)m_pFile->m_strms.size(); i++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[i].get();
      if(s->strh.fccType == FCC('vids'))
      {
        *pDuration = s->cs.size();
        return S_OK;
      }
    }

    return E_UNEXPECTED;
  }

  return __super::GetDuration(pDuration);
}

//

STDMETHODIMP CAviSplitterFilter::IsFormatSupported(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  HRESULT hr = __super::IsFormatSupported(pFormat);
  if(S_OK == hr) return hr;
  return *pFormat == TIME_FORMAT_FRAME ? S_OK : S_FALSE;
}

STDMETHODIMP CAviSplitterFilter::GetTimeFormat(GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  *pFormat = m_timeformat;
  return S_OK;
}

STDMETHODIMP CAviSplitterFilter::IsUsingTimeFormat(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  return *pFormat == m_timeformat ? S_OK : S_FALSE;
}

STDMETHODIMP CAviSplitterFilter::SetTimeFormat(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  if(S_OK != IsFormatSupported(pFormat)) return E_FAIL;
  m_timeformat = *pFormat;
  return S_OK;
}

STDMETHODIMP CAviSplitterFilter::GetStopPosition(LONGLONG* pStop)
{
  CheckPointer(pStop, E_POINTER);
  if(FAILED(__super::GetStopPosition(pStop))) return E_FAIL;
  if(m_timeformat == TIME_FORMAT_MEDIA_TIME) return S_OK;
  LONGLONG rt = *pStop;
  if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, rt, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
  return S_OK;
}

STDMETHODIMP CAviSplitterFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
  CheckPointer(pTarget, E_POINTER);

  const GUID& SourceFormat = pSourceFormat ? *pSourceFormat : m_timeformat;
  const GUID& TargetFormat = pTargetFormat ? *pTargetFormat : m_timeformat;
  
  if(TargetFormat == SourceFormat)
  {
    *pTarget = Source; 
    return S_OK;
  }
  else if(TargetFormat == TIME_FORMAT_FRAME && SourceFormat == TIME_FORMAT_MEDIA_TIME)
  {
    for(int i = 0; i < (int)m_pFile->m_strms.size(); i++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[i].get();
      if(s->strh.fccType == FCC('vids'))
      {
        *pTarget = s->GetFrame(Source);
        return S_OK;
      }
    }
  }
  else if(TargetFormat == TIME_FORMAT_MEDIA_TIME && SourceFormat == TIME_FORMAT_FRAME)
  {
    for(int i = 0; i < (int)m_pFile->m_strms.size(); i++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[i].get();
      if(s->strh.fccType == FCC('vids'))
      {
        if(Source < 0 || Source >= s->cs.size()) return E_FAIL;
        CAviFile::strm_t::chunk& c = s->cs[(int)Source];
        *pTarget = s->GetRefTime((DWORD)Source, c.size);
        return S_OK;
      }
    }
  }
  
  return E_FAIL;
}

STDMETHODIMP CAviSplitterFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
  HRESULT hr;
  if(FAILED(hr = __super::GetPositions(pCurrent, pStop)) || m_timeformat != TIME_FORMAT_FRAME)
    return hr;

  if(pCurrent)
    if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_FRAME, *pCurrent, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
  if(pStop)
    if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, *pStop, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;

  return S_OK;
}

HRESULT CAviSplitterFilter::SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
  if(m_timeformat != TIME_FORMAT_FRAME)
    return __super::SetPositionsInternal(id, pCurrent, dwCurrentFlags, pStop, dwStopFlags);

  if(!pCurrent && !pStop
  || (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
    && (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
    return S_OK;

  REFERENCE_TIME 
    rtCurrent = m_rtCurrent,
    rtStop = m_rtStop;

  if((dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
  && FAILED(ConvertTimeFormat(&rtCurrent, &TIME_FORMAT_FRAME, rtCurrent, &TIME_FORMAT_MEDIA_TIME))) 
    return E_FAIL;
  if((dwStopFlags&AM_SEEKING_PositioningBitsMask)
  && FAILED(ConvertTimeFormat(&rtStop, &TIME_FORMAT_FRAME, rtStop, &TIME_FORMAT_MEDIA_TIME)))
    return E_FAIL;

  if(pCurrent)
  switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
  {
  case AM_SEEKING_NoPositioning: break;
  case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
  case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
  case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
  }

  if(pStop)
  switch(dwStopFlags&AM_SEEKING_PositioningBitsMask)
  {
  case AM_SEEKING_NoPositioning: break;
  case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
  case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
  case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
  }

  if((dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
  && pCurrent)
    if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_MEDIA_TIME, rtCurrent, &TIME_FORMAT_FRAME))) return E_FAIL;
  if((dwStopFlags&AM_SEEKING_PositioningBitsMask)
  && pStop)
    if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_MEDIA_TIME, rtStop, &TIME_FORMAT_FRAME))) return E_FAIL;

  return __super::SetPositionsInternal(id, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

// IKeyFrameInfo

STDMETHODIMP CAviSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
  if(!m_pFile.get()) return E_UNEXPECTED;

  HRESULT hr = S_OK;

  nKFs = 0;

  for(int i = 0; i < (int)m_pFile->m_strms.size(); i++)
  {
    CAviFile::strm_t* s = m_pFile->m_strms[i].get();
    if(s->strh.fccType != FCC('vids')) continue;

    for(int j = 0; j < s->cs.size(); j++)
    {
      CAviFile::strm_t::chunk& c = s->cs[j];
      if(c.fKeyFrame) nKFs++;
    }

    if(nKFs == s->cs.size())
      hr = S_FALSE;

    break;
  }

  return hr;
}

STDMETHODIMP CAviSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
  CheckPointer(pFormat, E_POINTER);
  CheckPointer(pKFs, E_POINTER);

  if(!m_pFile.get()) return E_UNEXPECTED;
  if(*pFormat != TIME_FORMAT_MEDIA_TIME && *pFormat != TIME_FORMAT_FRAME) return E_INVALIDARG;

  UINT nKFsTmp = 0;

  for(int i = 0; i < (int)m_pFile->m_strms.size(); i++)
  {
    CAviFile::strm_t* s = m_pFile->m_strms[i].get();
    if(s->strh.fccType != FCC('vids')) continue;

    bool fConvertToRefTime = !!(*pFormat == TIME_FORMAT_MEDIA_TIME);

    for(int j = 0; j < s->cs.size() && nKFsTmp < nKFs; j++)
    {
      if(s->cs[j].fKeyFrame)
        pKFs[nKFsTmp++] = fConvertToRefTime ? s->GetRefTime(j, s->cs[j].size) : j;
    }

    break;
  }

  nKFs = nKFsTmp;

  return S_OK;
}

//
// CAviSourceFilter
//

CAviSourceFilter::CAviSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
  : CAviSplitterFilter(pUnk, phr)
{
  m_clsid = __uuidof(this);
  m_pInput.release();
  
}

//
// CAviSplitterOutputPin
//

CAviSplitterOutputPin::CAviSplitterOutputPin(vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
  : CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
{
}

HRESULT CAviSplitterOutputPin::CheckConnect(IPin* pPin)
{
  int iPosition = 0;
  CMediaType mt;
  while(S_OK == GetMediaType(iPosition++, &mt))
  {
    if(mt.majortype == MEDIATYPE_Video 
    && (mt.subtype == FOURCCMap(FCC('IV32'))
    || mt.subtype == FOURCCMap(FCC('IV31'))
    || mt.subtype == FOURCCMap(FCC('IF09'))))
    {
      CLSID clsid = DShowUtil::GetCLSID(DShowUtil::GetFilterFromPin(pPin));
      if(clsid == CLSID_VideoMixingRenderer || clsid == CLSID_OverlayMixer)
        return E_FAIL;
    }

    mt.InitMediaType();
  }

  return __super::CheckConnect(pPin);
}

