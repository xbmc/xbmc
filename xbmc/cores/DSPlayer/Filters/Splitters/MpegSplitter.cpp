/* 
 * (C) 2003-2006 Gabest
 * (C) 2006-2010 see AUTHORS
 *	http://www.gabest.org
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
#include <initguid.h>
#include <dmodshow.h>
#include "MpegSplitter.h"
#include <moreuuids.h>
#include "DShowUtil/DShowUtil.h"
#include "boost/throw_exception.hpp"
//
// CMpegSplitterFilter
//

CMpegSplitterFilter::CMpegSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseSplitterFilter(NAME("CMpegSplitterFilter"), pUnk, phr, clsid)
	, m_pPipoBimbo(false)
{
}

STDMETHODIMP CMpegSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

    return 
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CMpegSplitterFilter::GetClassID(CLSID* pClsID)
{
	CheckPointer (pClsID, E_POINTER);

	if (m_pPipoBimbo)
	{
		memcpy (pClsID, &CLSID_WMAsfReader, sizeof (GUID));
		return S_OK;
	}
	else
		return __super::GetClassID(pClsID);
}

void CMpegSplitterFilter::ReadClipInfo(LPCOLESTR pszFileName)
{
	if (wcslen (pszFileName) > 0)
	{
		WCHAR		Drive[_MAX_DRIVE];
		WCHAR		Dir[_MAX_PATH];
		WCHAR		Filename[_MAX_PATH];
		WCHAR		Ext[_MAX_EXT];
		
		if (_wsplitpath_s (pszFileName, Drive, countof(Drive), Dir, countof(Dir), Filename, countof(Filename), Ext, countof(Ext)) == 0)
		{
			CStdString	strClipInfo;

			if (Drive[0])
				strClipInfo.Format (_T("%s\\%s\\..\\CLIPINF\\%s.clpi"), Drive, Dir, Filename);
			else
				strClipInfo.Format (_T("%s\\..\\CLIPINF\\%s.clpi"), Dir, Filename);
      LPCSTR clip;
      
      clip = LPCTSTR(strClipInfo.c_str());
			//m_ClipInfo.ReadInfo (clip);
		}
	}
}

STDMETHODIMP CMpegSplitterFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{	
	return __super::Load (pszFileName, pmt);
}


HRESULT CMpegSplitterFilter::DemuxNextPacket(REFERENCE_TIME rtStartOffset)
{
	HRESULT hr;
	BYTE b;

	if(m_pFile->m_type == CMpegSplitterFile::ps || m_pFile->m_type == CMpegSplitterFile::es)
	{
		if(!m_pFile->NextMpegStartCode(b))
			return S_FALSE;

		if(b == 0xba) // program stream header
		{
			CMpegSplitterFile::pshdr h;
			if(!m_pFile->Read(h))
				return S_FALSE;
		}
		else if(b == 0xbb) // program stream system header
		{
			CMpegSplitterFile::pssyshdr h;
			if(!m_pFile->Read(h))
				return S_FALSE;
		}
#if (EVO_SUPPORT == 0)
		else if(b >= 0xbd && b < 0xf0) // pes packet
#else
		else if((b >= 0xbd && b < 0xf0) || (b == 0xfd)) // pes packet
#endif
		{
			CMpegSplitterFile::peshdr h;

			if(!m_pFile->Read(h, b) || !h.len) return S_FALSE;

			if(h.type == CMpegSplitterFile::mpeg2 && h.scrambling)
			{
				ASSERT(0);
				return E_FAIL;
			}

			__int64 pos = m_pFile->GetPos();

			DWORD TrackNumber = m_pFile->AddStream(0, b, h.len);

			if(GetOutputPin(TrackNumber))
			{
				std::auto_ptr<Packet> p(DNew Packet());

				p->TrackNumber = TrackNumber;
				p->bSyncPoint = !!h.fpts;
				p->bAppendable = !h.fpts;
				p->rtStart = h.fpts ? (h.pts - rtStartOffset) : Packet::INVALID_TIME;
				p->rtStop = p->rtStart+1;
				p->resize(h.len - (size_t)(m_pFile->GetPos() - pos));

				m_pFile->ByteRead(&p->at (0), h.len - (m_pFile->GetPos() - pos));

				hr = DeliverPacket(p);
			}
			m_pFile->Seek(pos + h.len);
		}
	}
	else if(m_pFile->m_type == CMpegSplitterFile::ts)
	{
		CMpegSplitterFile::trhdr h;

		if(!m_pFile->Read(h)) 
			return S_FALSE;


		__int64 pos = m_pFile->GetPos();

		if(h.payload && h.payloadstart)
			m_pFile->UpdatePrograms(h);

		if(h.payload && h.pid >= 16 && h.pid < 0x1fff && !h.scrambling)
		{
			DWORD TrackNumber = h.pid;

			CMpegSplitterFile::peshdr h2;

			if(h.payloadstart && m_pFile->NextMpegStartCode(b, 4) && m_pFile->Read(h2, b)) // pes packet
			{
				if(h2.type == CMpegSplitterFile::mpeg2 && h2.scrambling)
				{
					ASSERT(0);
					return E_FAIL;
				}
				TrackNumber = m_pFile->AddStream(h.pid, b, h.bytes - (DWORD)(m_pFile->GetPos() - pos));
			}

			if(GetOutputPin(TrackNumber))
			{
				std::auto_ptr<Packet> p(DNew Packet());

				p->TrackNumber = TrackNumber;
				p->bSyncPoint = !!h2.fpts;
				p->bAppendable = !h2.fpts;

				if (h.fPCR)
				{
					CRefTime rtNow;
					StreamTime(rtNow);
          CLog::Log(LOGDEBUG,"Now=%S   PCR=%S\n", DShowUtil::ReftimeToString(rtNow.m_time), DShowUtil::ReftimeToString(h.PCR));
				}
				if (h2.fpts && h.pid == 241)
				{
					CLog::Log(LOGDEBUG,"Sub=%S\n", DShowUtil::ReftimeToString(h2.pts - rtStartOffset));
				}

				p->rtStart = h2.fpts ? (h2.pts - rtStartOffset) : Packet::INVALID_TIME;
				p->rtStop = p->rtStart+1;
				p->resize(h.bytes - (size_t)(m_pFile->GetPos() - pos));
        
				int nBytes = int(h.bytes - (m_pFile->GetPos() - pos));
				m_pFile->ByteRead(&p->at(0), nBytes);

				hr = DeliverPacket(p);
			}
		}

		m_pFile->Seek(h.next);
	}
	else if(m_pFile->m_type == CMpegSplitterFile::pva)
	{
		CMpegSplitterFile::pvahdr h;
		if(!m_pFile->Read(h))
			return S_FALSE;

		DWORD TrackNumber = h.streamid;

		__int64 pos = m_pFile->GetPos();

		if(GetOutputPin(TrackNumber))
		{
			std::auto_ptr<Packet> p(DNew Packet());

			p->TrackNumber = TrackNumber;
			p->bSyncPoint = !!h.fpts;
			p->bAppendable = !h.fpts;
			p->rtStart = h.fpts ? (h.pts - rtStartOffset) : Packet::INVALID_TIME;
			p->rtStop = p->rtStart+1;
			p->resize(h.length);

			m_pFile->ByteRead(&p->at(0), h.length);
			hr = DeliverPacket(p);
		}

		m_pFile->Seek(pos + h.length);
	}

	return S_OK;
}

//

HRESULT CMpegSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.release();
  //LPCTSTR pf = GetPartFilename(pAsyncReader);
  LPCOLESTR pfstr = LPCOLESTR(m_fn.c_str());
	ReadClipInfo (pfstr);
	m_pFile.reset(new CMpegSplitterFile(pAsyncReader, hr, m_ClipInfo.IsHdmv(), m_ClipInfo));

	if(!m_pFile.get()) 
    return E_OUTOFMEMORY;

	if(FAILED(hr))
	{
		m_pFile.release();
		return hr;
	}

	// Create
	if (m_ClipInfo.IsHdmv())
	{
		for (int i=0; i<m_ClipInfo.GetStreamNumber(); i++)
		{
			CHdmvClipInfo::Stream* stream = m_ClipInfo.GetStreamByIndex (i);
			if (stream->m_Type == PRESENTATION_GRAPHICS_STREAM)
			{
				m_pFile->AddHdmvPGStream (stream->m_PID, stream->m_LanguageCode);
			}
		}
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	for(int i = 0; i < countof(m_pFile->m_streams); i++)
	{
    std::vector<CMpegSplitterFile::stream>::iterator it = m_pFile->m_streams[i].begin();
    
		while(it != m_pFile->m_streams[i].end())
		{
			CMpegSplitterFile::stream& s = *it;

			std::vector<CMediaType> mts;
			mts.push_back(s.mt);

			CStdStringW name = CMpegSplitterFile::CStreamList::ToString(i);
      typedef std::pair<const unsigned char*,size_t> TdataSize;
        TdataSize thepair;
      if (s.mt.formattype == FORMAT_MPEG2Video)
      {
        
        MPEG2VIDEOINFO *mpeg2info=(MPEG2VIDEOINFO*)s.mt.pbFormat;
        const uint8_t* cbseq = (const uint8_t*)mpeg2info->dwSequenceHeader;
        
        thepair = std::make_pair(mpeg2info->cbSequenceHeader?cbseq:NULL,mpeg2info->cbSequenceHeader);
      }
			std::auto_ptr<CBaseSplitterOutputPin> pPinOut(DNew CMpegSplitterOutputPin(mts, name, this, this, &hr));
			if (i == CMpegSplitterFile::subpic)
				(static_cast<CMpegSplitterOutputPin*>(pPinOut.get())->SetMaxShift (_I64_MAX));
			if(S_OK == AddOutputPin(s, pPinOut))
				break;
      it++;
		}
	}

	if(m_pFile->IsRandomAccess() && m_pFile->m_rate)
	{
		m_rtNewStop = m_rtStop = m_rtDuration = 10000000i64 * m_pFile->GetLength() / m_pFile->m_rate;
	}

	return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CMpegSplitterFilter::DemuxInit()
{
	if(!m_pFile.get()) return(false);

	m_rtStartOffset = 0;

	return(true);
}

void CMpegSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	std::vector<CMpegSplitterFile::stream>* pMasterStream = m_pFile->GetMasterStream();

	if(!pMasterStream)
	{
		ASSERT(0);
		return;
	}

	if(m_pFile->IsStreaming())
	{
		m_pFile->Seek(dsmax(0, m_pFile->GetLength() - 100*1024));
		m_rtStartOffset = m_pFile->m_rtMin + m_pFile->NextPTS(pMasterStream->front());
		return;
	}

	REFERENCE_TIME rtPreroll = 10000000;
	
	if(rt <= rtPreroll || m_rtDuration <= 0)
	{
		m_pFile->Seek(0);
	}
	else
	{
		__int64 len = m_pFile->GetLength();
		__int64 seekpos = (__int64)(1.0*rt/m_rtDuration*len);
		__int64 minseekpos = _I64_MAX;

		REFERENCE_TIME rtmax = rt - rtPreroll;
		REFERENCE_TIME rtmin = rtmax - 5000000;

		if(m_rtStartOffset == 0)
		for(int i = 0; i < countof(m_pFile->m_streams)-1; i++)
		{
      std::vector<CMpegSplitterFile::stream>::iterator it = m_pFile->m_streams[i].begin();
    
		  while(it != m_pFile->m_streams[i].end())
      {
			DWORD TrackNum = *it;//m_pFile->m_streams[i].GetNext(pos);

				CBaseSplitterOutputPin* pPin = GetOutputPin(TrackNum);
				if(pPin && pPin->IsConnected())
				{
					m_pFile->Seek(seekpos);

					REFERENCE_TIME pdt = _I64_MIN;

					for(int j = 0; j < 10; j++)
					{
						REFERENCE_TIME rt = m_pFile->NextPTS(TrackNum);

						if(rt < 0) break;

						REFERENCE_TIME dt = rt - rtmax;
						if(dt > 0 && dt == pdt) dt = 10000000i64;


						if(rtmin <= rt && rt <= rtmax || pdt > 0 && dt < 0)
						{
							minseekpos = min(minseekpos, m_pFile->GetPos());
							break;
						}

						m_pFile->Seek(m_pFile->GetPos() - (__int64)(1.0*dt/m_rtDuration*len));
		
						pdt = dt;
					}
        }
        it++;
      }
		}

		if(minseekpos != _I64_MAX)
		{
			seekpos = minseekpos;
		}
		else
		{
			// this file is probably screwed up, try plan B, seek simply by bitrate

			rt -= rtPreroll;
			seekpos = (__int64)(1.0*rt/m_rtDuration*len);
			m_pFile->Seek(seekpos);
			m_rtStartOffset = m_pFile->m_rtMin + m_pFile->NextPTS(pMasterStream->front()) - rt;
		}

		m_pFile->Seek(seekpos);
	}
}

bool CMpegSplitterFilter::DemuxLoop()
{
	REFERENCE_TIME rtStartOffset = m_rtStartOffset ? m_rtStartOffset : m_pFile->m_rtMin;

	HRESULT hr = S_OK;
	while(SUCCEEDED(hr) && !CheckRequest(NULL))
	{
		if((hr = m_pFile->HasMoreData(1024*500)) == S_OK)
			if((hr = DemuxNextPacket(rtStartOffset)) == S_FALSE)
				Sleep(1);
	}

	return(true);
}



// IAMStreamSelect

STDMETHODIMP CMpegSplitterFilter::Count(DWORD* pcStreams)
{
	CheckPointer(pcStreams, E_POINTER);

	*pcStreams = 0;

	for(int i = 0; i < countof(m_pFile->m_streams); i++)
		(*pcStreams) += m_pFile->m_streams[i].size();

	return S_OK;
}

STDMETHODIMP CMpegSplitterFilter::Enable(long lIndex, DWORD dwFlags)
{
	if(!(dwFlags & AMSTREAMSELECTENABLE_ENABLE))
		return E_NOTIMPL;

	for(int i = 0, j = 0; i < countof(m_pFile->m_streams); i++)
	{
		int cnt = m_pFile->m_streams[i].size();
		
		if(lIndex >= j && lIndex < j+cnt)
		{
			lIndex -= j;

			//POSITION pos = m_pFile->m_streams[i].FindIndex(lIndex);
      //
      if (m_pFile->m_streams[i].at(lIndex) != NULL)
        return E_UNEXPECTED;
			//if(!pos) 

			CMpegSplitterFile::stream& to = m_pFile->m_streams[i].at(lIndex);//m_pFile->m_streams[i].GetAt(pos);

			//pos = m_pFile->m_streams[i].GetHeadPosition();
      std::vector<CMpegSplitterFile::stream>::iterator it =  m_pFile->m_streams[i].begin();
			while(it !=  m_pFile->m_streams[i].end())
      {
				CMpegSplitterFile::stream& from = *it;//m_pFile->m_streams[i].GetNext(pos);
				if(!GetOutputPin(from)) continue;

				HRESULT hr;
				if(FAILED(hr = RenameOutputPin(from, to, &to.mt)))
					return hr;

				// Don't rename other pin for Hdmv!
				int iProgram;
				const CHdmvClipInfo::Stream *pClipInfo;
				const CMpegSplitterFile::program* p = m_pFile->FindProgram(to.pid, iProgram, pClipInfo);

				if(p!=NULL && !m_ClipInfo.IsHdmv() && !m_pFile->IsHdmv())
				{
					for(int k = 0; k < countof(m_pFile->m_streams); k++)
					{
						if(k == i) continue;
            
            std::vector<CMpegSplitterFile::stream>::iterator itt =  m_pFile->m_streams[i].begin();
            while (itt != m_pFile->m_streams[k].end())
            {
              CMpegSplitterFile::stream& from = *it;
              if(!GetOutputPin(from)) continue;

							for(int l = 0; l < countof(p->streams); l++)
							{
								if(const CMpegSplitterFile::stream* s = m_pFile->m_streams[k].FindStream(p->streams[l].pid))
								{
									if(from != *s)
										hr = RenameOutputPin(from, *s, &s->mt);
									break;
								}
							}
            it++;
            }
						
					}
				}
        it++;
				return S_OK;
			}
		}

		j += cnt;
	}

	return S_FALSE;
}



static int GetHighestBitSet32(unsigned long _Value)
{
	unsigned long Ret;
	unsigned char bNonZero = _BitScanReverse(&Ret, _Value);
	if (bNonZero)
		return Ret;
	else
		return -1;
}

CStdString FormatBitrate(double _Bitrate)
{
	CStdString Temp;
	if (_Bitrate > 20000000) // More than 2 mbit
		Temp.Format("%.2f mbit/s", double(_Bitrate)/1000000.0);
	else
		Temp.Format("%.1f kbit/s", double(_Bitrate)/1000.0);

	return Temp;
}

CStdString FormatString(const wchar_t *pszFormat, ... )
{
	CStdStringW Temp;
	//ASSERT( AtlIsValidString( pszFormat ) );

	va_list argList;
	va_start( argList, pszFormat );
	Temp.FormatV( pszFormat, argList );
	va_end( argList );

	return Temp;
}



bool CMpegSplitterFile::stream::operator < (const stream &_Other) const
{

	if (mt.majortype == MEDIATYPE_Audio && _Other.mt.majortype == MEDIATYPE_Audio)
	{
		int iProgram0;
		const CHdmvClipInfo::Stream *pClipInfo0;
		const CMpegSplitterFile::program * pProgram0 = m_pFile->FindProgram(pid, iProgram0, pClipInfo0);
		int StreamType0 = pClipInfo0 ? pClipInfo0->m_Type : pProgram0 ? pProgram0->streams[iProgram0].type : 0;
		int iProgram1;
		const CHdmvClipInfo::Stream *pClipInfo1;
		const CMpegSplitterFile::program * pProgram1 = m_pFile->FindProgram(_Other.pid, iProgram1, pClipInfo1);
		int StreamType1 = pClipInfo1 ? pClipInfo1->m_Type : pProgram1 ? pProgram1->streams[iProgram1].type : 0;
		
		if (mt.formattype == FORMAT_WaveFormatEx && _Other.mt.formattype != FORMAT_WaveFormatEx)
			return true;
		if (mt.formattype != FORMAT_WaveFormatEx && _Other.mt.formattype == FORMAT_WaveFormatEx)
			return false;

		LONGLONG Quality0 = GetMediaTypeQuality(&mt, StreamType0);
		LONGLONG Quality1 = GetMediaTypeQuality(&_Other.mt, StreamType1);
		if (Quality0 > Quality1)
			return true;
		if (Quality0 < Quality1)
			return false;
	}
	DWORD DefaultFirst = *this;
	DWORD DefaultSecond = _Other;
	return DefaultFirst < DefaultSecond;
}

CStdString GetMediaTypeDesc(const CMediaType *_pMediaType, const CHdmvClipInfo::Stream *pClipInfo, int _PresentationType)
{
	const WCHAR *pPresentationDesc = NULL;

	if (pClipInfo)
    pPresentationDesc = DShowUtil::StreamTypeToName(pClipInfo->m_Type);
	else
    pPresentationDesc = DShowUtil::StreamTypeToName((PES_STREAM_TYPE)_PresentationType);

	CStdString MajorType;
	std::list<CStdString> Infos;

	if (_pMediaType->majortype == MEDIATYPE_Video)
	{
		MajorType = "Video";

		if (pClipInfo)
		{
      CStdString name = DShowUtil::ISO6392ToLanguage(pClipInfo->m_LanguageCode);

			if (!name.IsEmpty())
				Infos.push_back(name);
		}

		const VIDEOINFOHEADER *pVideoInfo = NULL;
		const VIDEOINFOHEADER2 *pVideoInfo2 = NULL;

		if (_pMediaType->formattype == FORMAT_MPEGVideo)
		{
			Infos.push_back(L"MPEG");

			const MPEG1VIDEOINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);

			pVideoInfo = &pInfo->hdr;

		}
		else if (_pMediaType->formattype == FORMAT_MPEG2_VIDEO)
		{
			const MPEG2VIDEOINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);

			pVideoInfo2 = &pInfo->hdr;

			bool bIsAVC = false;

			if (pInfo->hdr.bmiHeader.biCompression == '1CVA')
			{
				bIsAVC = true;
				Infos.push_back(L"AVC (H.264)");
			}
			else if (pInfo->hdr.bmiHeader.biCompression == 0)
				Infos.push_back(L"MPEG2");
			else
			{
				WCHAR Temp[5];
				memset(Temp, 0, sizeof(Temp));
				Temp[0] = (pInfo->hdr.bmiHeader.biCompression >> 0) & 0xFF;
				Temp[1] = (pInfo->hdr.bmiHeader.biCompression >> 0) & 0xFF;
				Temp[2] = (pInfo->hdr.bmiHeader.biCompression >> 0) & 0xFF;
				Temp[3] = (pInfo->hdr.bmiHeader.biCompression >> 0) & 0xFF;
				Infos.push_back(Temp);
			}

			switch (pInfo->dwProfile)
			{
			case AM_MPEG2Profile_Simple:			Infos.push_back(L"Simple Profile"); break;
			case AM_MPEG2Profile_Main:				Infos.push_back(L"Main Profile"); break;
			case AM_MPEG2Profile_SNRScalable:		Infos.push_back(L"SNR Scalable Profile"); break;
			case AM_MPEG2Profile_SpatiallyScalable:	Infos.push_back(L"Spatially Scalable Profile"); break;
			case AM_MPEG2Profile_High:				Infos.push_back(L"High Profile"); break;
			default:
				if (pInfo->dwProfile)
				{
					if (bIsAVC)
					{
						switch (pInfo->dwProfile)
						{
							case 44:	Infos.push_back(L"CAVLC Profile"); break;
							case 66:	Infos.push_back(L"Baseline Profile"); break;
							case 77:	Infos.push_back(L"Main Profile"); break;
							case 88:	Infos.push_back(L"Extended Profile"); break;
							case 100:	Infos.push_back(L"High Profile"); break;
							case 110:	Infos.push_back(L"High 10 Profile"); break;
							case 122:	Infos.push_back(L"High 4:2:2 Profile"); break;
							case 244:	Infos.push_back(L"High 4:4:4 Profile"); break;

							default:	Infos.push_back(FormatString(L"Profile %d", pInfo->dwProfile)); break;
						}
					}
					else
						Infos.push_back(FormatString(L"Profile %d", pInfo->dwProfile));
				}
				break;
			}

			switch (pInfo->dwLevel)
			{
			case AM_MPEG2Level_Low:			Infos.push_back(L"Low Level"); break;
			case AM_MPEG2Level_Main:		Infos.push_back(L"Main Level"); break;
			case AM_MPEG2Level_High1440:	Infos.push_back(L"High1440 Level"); break;
			case AM_MPEG2Level_High:		Infos.push_back(L"High Level"); break;
			default:
				if (pInfo->dwLevel)
				{
					if (bIsAVC)
						Infos.push_back(FormatString(L"Level %1.1f", double(pInfo->dwLevel)/10.0));
					else
						Infos.push_back(FormatString(L"Level %d", pInfo->dwLevel));
				}
				break;
			}
		}
		else if (_pMediaType->formattype == FORMAT_VIDEOINFO2)
		{
			const VIDEOINFOHEADER2 *pInfo = GetFormatHelper(pInfo, _pMediaType);

			pVideoInfo2 = pInfo;
			bool bIsVC1 = false;

			DWORD CodecType = pInfo->bmiHeader.biCompression;
			if (CodecType == '1CVW')
			{
				bIsVC1 = true;
				Infos.push_back(L"VC-1");
			}
			else if (CodecType)
			{
				WCHAR Temp[5];
				memset(Temp, 0, sizeof(Temp));
				Temp[0] = (CodecType >> 0) & 0xFF;
				Temp[1] = (CodecType >> 0) & 0xFF;
				Temp[2] = (CodecType >> 0) & 0xFF;
				Temp[3] = (CodecType >> 0) & 0xFF;
				Infos.push_back(Temp);
			}
		}
		else if (_pMediaType->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
		{
			Infos.push_back(L"DVD Sub Picture");
		}
		else if (_pMediaType->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE)
		{
			Infos.push_back(L"SVCD Sub Picture");
		}
		else if (_pMediaType->subtype == MEDIASUBTYPE_CVD_SUBPICTURE)
		{
			Infos.push_back(L"CVD Sub Picture");
		}

		if (pVideoInfo2)
		{
			if (pVideoInfo2->bmiHeader.biWidth && pVideoInfo2->bmiHeader.biHeight)
				Infos.push_back(FormatString(L"%dx%d", pVideoInfo2->bmiHeader.biWidth, pVideoInfo2->bmiHeader.biHeight));
			if (pVideoInfo2->AvgTimePerFrame)
				Infos.push_back(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo2->AvgTimePerFrame)));
			if (pVideoInfo2->dwBitRate)
				Infos.push_back(FormatBitrate(pVideoInfo2->dwBitRate));
		}
		else if (pVideoInfo)
		{
			if (pVideoInfo->bmiHeader.biWidth && pVideoInfo->bmiHeader.biHeight)
				Infos.push_back(FormatString(L"%dx%d", pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight));
			if (pVideoInfo->AvgTimePerFrame)
				Infos.push_back(FormatString(L"%.3f fps", 10000000.0/double(pVideoInfo->AvgTimePerFrame)));
			if (pVideoInfo->dwBitRate)
				Infos.push_back(FormatBitrate(pVideoInfo->dwBitRate));
		}
		
	}
	else if (_pMediaType->majortype == MEDIATYPE_Audio)
	{
		MajorType = "Audio";
		if (pClipInfo)
		{
			CStdString name = DShowUtil::ISO6392ToLanguage(pClipInfo->m_LanguageCode);
			if (!name.IsEmpty())
				Infos.push_back(name);
		}
		if (_pMediaType->formattype == FORMAT_WaveFormatEx)
		{
			const WAVEFORMATEX *pInfo = GetFormatHelper(pInfo, _pMediaType);

			if (_pMediaType->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
			{
				Infos.push_back(L"DVD LPCM");
			}
			else if (_pMediaType->subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO)
			{
				const WAVEFORMATEX_HDMV_LPCM *pInfoHDMV = GetFormatHelper(pInfoHDMV, _pMediaType);
				Infos.push_back(L"HDMV LPCM");
			}
			else
			{
				switch (pInfo->wFormatTag)
				{
				case WAVE_FORMAT_PS2_PCM:
					{
						Infos.push_back(L"PS2 PCM");
					}
					break;
				case WAVE_FORMAT_PS2_ADPCM:
					{
						Infos.push_back(L"PS2 ADPCM");
					}
					break;
				case WAVE_FORMAT_DVD_DTS:
					{
						if (pPresentationDesc)
							Infos.push_back(pPresentationDesc);
						else
							Infos.push_back(L"DTS");
					}
					break;
				case WAVE_FORMAT_DOLBY_AC3:
					{
						if (pPresentationDesc)
							Infos.push_back(pPresentationDesc);
						else
							Infos.push_back(L"Dolby Digital");
					}
					break;
				case WAVE_FORMAT_AAC:
					{
						Infos.push_back(L"AAC");
					}
					break;
				case WAVE_FORMAT_MP3:
					{
						Infos.push_back(L"MP3");
					}
					break;
				case WAVE_FORMAT_MPEG:
					{
						const MPEG1WAVEFORMAT* pInfoMPEG1 = GetFormatHelper(pInfoMPEG1, _pMediaType);

						int layer = GetHighestBitSet32(pInfoMPEG1->fwHeadLayer) + 1;
						Infos.push_back(FormatString(L"MPEG1 - Layer %d", layer));
					}
					break;
				}
			}

			if (pClipInfo && (pClipInfo->m_SampleRate == BDVM_SampleRate_48_192 || pClipInfo->m_SampleRate == BDVM_SampleRate_48_96))
			{
				switch (pClipInfo->m_SampleRate)
				{
				case BDVM_SampleRate_48_192:
					Infos.push_back(FormatString(L"192(48) kHz"));
					break;
				case BDVM_SampleRate_48_96:
					Infos.push_back(FormatString(L"96(48) kHz"));
					break;
				}
			}
			else if (pInfo->nSamplesPerSec)
				Infos.push_back(FormatString(L"%.1f kHz", double(pInfo->nSamplesPerSec)/1000.0));
			if (pInfo->nChannels)
				Infos.push_back(FormatString(L"%d chn", pInfo->nChannels));
			if (pInfo->wBitsPerSample)
				Infos.push_back(FormatString(L"%d bit", pInfo->wBitsPerSample));
			if (pInfo->nAvgBytesPerSec)
				Infos.push_back(FormatBitrate(pInfo->nAvgBytesPerSec * 8));

		}
	}
	else if (_pMediaType->majortype == MEDIATYPE_Subtitle)
	{
		MajorType = "Subtitle";

		if (pPresentationDesc)
			Infos.push_back(pPresentationDesc);

		if (_pMediaType->cbFormat == sizeof(SUBTITLEINFO))
		{
			const SUBTITLEINFO *pInfo = GetFormatHelper(pInfo, _pMediaType);
			CStdString name = DShowUtil::ISO6392ToLanguage(pInfo->IsoLang);

			if (pInfo->TrackName[0])
				Infos.push_front(pInfo->TrackName);
			if (!name.IsEmpty())
				Infos.push_front(name);
		}
		else
		{
			if (pClipInfo)
			{
        CStdString name = DShowUtil::ISO6392ToLanguage(pClipInfo->m_LanguageCode);
				if (!name.IsEmpty())
					Infos.push_front(name);
			}
		}
	}

	if (!Infos.empty())
	{
		CStdString Ret;

		Ret += MajorType;
		Ret += " - ";

		bool bFirst = true;

    for(std::list<CStdString>::iterator it = Infos.begin();it != Infos.end(); it++)
		{
			CStdString& String = *it;

			if (bFirst)
				Ret += String;
			else
				Ret += L", " + String;

			bFirst = false;
		}

		return Ret;
	}
	return CStdString();
}



STDMETHODIMP CMpegSplitterFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	for(int i = 0, j = 0; i < countof(m_pFile->m_streams); i++)
	{
		int cnt = m_pFile->m_streams[i].size();
		
		if(lIndex >= j && lIndex < j+cnt)
		{
			lIndex -= j;
			
			CMpegSplitterFile::stream&	s = m_pFile->m_streams[i].at(lIndex);
			if(!s) 
        return E_UNEXPECTED;

			//CMpegSplitterFile::stream&	s = m_pFile->m_streams[i].GetAt(pos);
			CHdmvClipInfo::Stream*		pStream = m_ClipInfo.FindStream (s.pid);

			if(ppmt) *ppmt = CreateMediaType(&s.mt);
			if(pdwFlags) *pdwFlags = GetOutputPin(s) ? (AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE) : 0;
			if(plcid) *plcid = pStream ? pStream->m_LCID : 0;
			if(pdwGroup) *pdwGroup = i;
			if(ppObject) *ppObject = NULL;
			if(ppUnk) *ppUnk = NULL;

			
			if(ppszName)
			{
				CStdStringW name = CMpegSplitterFile::CStreamList::ToString(i);

				CStdStringW str;

				if (i == CMpegSplitterFile::subpic && s.pid == NO_SUBTITLE_PID)
				{
					str		= _T("No subtitles");
					*plcid	= LCID_NOSUBTITLES;
				}
				else
				{
					int iProgram;
					const CHdmvClipInfo::Stream *pClipInfo;
					const CMpegSplitterFile::program * pProgram = m_pFile->FindProgram(s.pid, iProgram, pClipInfo);
					const wchar_t *pStreamName = NULL;
					int StreamType = pClipInfo ? pClipInfo->m_Type : pProgram ? pProgram->streams[iProgram].type : 0;
          pStreamName = DShowUtil::StreamTypeToName((PES_STREAM_TYPE)StreamType);

					CStdString FormatDesc = GetMediaTypeDesc(&s.mt, pClipInfo, StreamType);

					//if (!FormatDesc.IsEmpty())
					//	str.Format(L"%s (%04x,%02x,%02x)", FormatDesc.c_str(), s.pid, s.pesid, s.ps1id); // TODO: make this nicer
          if (!FormatDesc.IsEmpty())
          {
            str.Format(L"%s (%04x,", FormatDesc.c_str(), s.pid);
            str.AppendFormat(L"%02x,%02x)", s.pesid, s.ps1id);
          }
					else if (pStreamName)
						str.Format(L"%s - %s (%04x,%02x,%02x)", name, pStreamName, s.pid, s.pesid, s.ps1id); // TODO: make this nicer
					else
						str.Format(L"%s (%04x,%02x,%02x)", name, s.pid, s.pesid, s.ps1id); // TODO: make this nicer
				}

				*ppszName = (WCHAR*)CoTaskMemAlloc((str.GetLength()+1)*sizeof(WCHAR));
				if(*ppszName == NULL) return E_OUTOFMEMORY;

				wcscpy_s(*ppszName, str.GetLength()+1, str);
			}
		}

		j += cnt;
	}

	return S_OK;
}

//
// CMpegSourceFilter
//

CMpegSourceFilter::CMpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CMpegSplitterFilter(pUnk, phr, clsid)
{
	m_pInput.release();
}

//
// CMpegSplitterOutputPin
//

CMpegSplitterOutputPin::CMpegSplitterOutputPin(std::vector<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(mts, pName, pFilter, pLock, phr)
	, m_fHasAccessUnitDelimiters(false)
	, m_rtMaxShift(50000000)
{
}

CMpegSplitterOutputPin::~CMpegSplitterOutputPin()
{
}

HRESULT CMpegSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	{
		CAutoLock cAutoLock(this);
		m_rtPrev = Packet::INVALID_TIME;
		m_rtOffset = 0;
	}

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CMpegSplitterOutputPin::DeliverEndFlush()
{
	{
		CAutoLock cAutoLock(this);
		m_p.release();
		m_pl.clear();
	}

	return __super::DeliverEndFlush();
}

HRESULT CMpegSplitterOutputPin::DeliverPacket(std::auto_ptr<Packet> p)
{
	CAutoLock cAutoLock(this);

	if(p->rtStart != Packet::INVALID_TIME)
	{
		REFERENCE_TIME rt = p->rtStart + m_rtOffset;

		// Filter invalid PTS (if too different from previous packet)
		if(m_rtPrev != Packet::INVALID_TIME)
		if(_abs64(rt - m_rtPrev) > m_rtMaxShift)
			m_rtOffset += m_rtPrev - rt;

		p->rtStart += m_rtOffset;
		p->rtStop += m_rtOffset;

		m_rtPrev = p->rtStart;
	}

	
	if (p->pmt)
	{
		if (*((CMediaType *)p->pmt) != m_mt)
			SetMediaType ((CMediaType*)p->pmt);
	}


	if(m_mt.subtype == MEDIASUBTYPE_AAC) // special code for aac, the currently available decoders only like whole frame samples
	{
		if(m_p.get() && m_p->size() == 1 && m_p->at(0) == 0xff	&& !(!p->empty() && (p->at(0) & 0xf6) == 0xf0))
			m_p.release();

		if(!m_p.get())
		{
			BYTE* base = &p->at(0);
			BYTE* s = base;
			BYTE* e = s + p->size();

			for(; s < e; s++)
			{
				if(*s != 0xff) continue;

				if(s == e-1 || (s[1]&0xf6) == 0xf0)
				{
					memmove(base, s, e - s);
					p->resize(e - s);
					m_p = p;
					break;
				}
			}
		}
		else
		{
      //*p.get()->at(0)
      /*int packetnewsize = m_p->size();
      packetnewsize += *p->size();
      m_p->resize(packetnewsize);
      (*p).front()
      m_p->insert(m_p->end(),&.at(0));*/
      
			
      //replacing Append with push_back work???
      //m_p->Append(*p);
		}

		while(m_p.get() && m_p->size() > 9)
		{
			BYTE* base = &m_p->at(0);//GetData();
			BYTE* s = base;
			BYTE* e = s + m_p->size();
			int len = ((s[3]&3)<<11)|(s[4]<<3)|(s[5]>>5);
			bool crc = !(s[1]&1);
			s += 7; len -= 7;
			if(crc) s += 2, len -= 2;

			if(e - s < len)
			{
				break;
			}

			if(len <= 0 || e - s >= len + 2 && (s[len] != 0xff || (s[len+1]&0xf6) != 0xf0))
			{
				m_p.release();
				break;
			}

			std::auto_ptr<Packet> p2(DNew Packet());

			p2->TrackNumber = m_p->TrackNumber;
			p2->bDiscontinuity |= m_p->bDiscontinuity;
			m_p->bDiscontinuity = false;

			p2->bSyncPoint = m_p->rtStart != Packet::INVALID_TIME;
			p2->rtStart = m_p->rtStart;
			m_p->rtStart = Packet::INVALID_TIME;

			p2->rtStop = m_p->rtStop;
			m_p->rtStop = Packet::INVALID_TIME;
			p2->pmt = m_p->pmt; m_p->pmt = NULL;
			p2->SetData(s, len);

			s += len;
			memmove(base, s, e - s);
			m_p->resize(e - s);

			HRESULT hr = __super::DeliverPacket(p2);
			if(hr != S_OK) return hr;
		}

		if(m_p.get() && p.get())
		{
			if(!m_p->bDiscontinuity) m_p->bDiscontinuity = p->bDiscontinuity;
			if(!m_p->bSyncPoint) m_p->bSyncPoint = p->bSyncPoint;
			if(m_p->rtStart == Packet::INVALID_TIME) m_p->rtStart = p->rtStart, m_p->rtStop = p->rtStop;
			if(m_p->pmt) DeleteMediaType(m_p->pmt);
			
			m_p->pmt = p->pmt;
			p->pmt = NULL;
		}

		return S_OK;
	}
	else if(m_mt.subtype == FOURCCMap('1CVA') || m_mt.subtype == FOURCCMap('1cva')) // just like aac, this has to be starting nalus, more can be packed together
	{
		if(!m_p.get())
		{
			m_p.reset(new Packet());
			m_p->TrackNumber = p->TrackNumber;
			m_p->bDiscontinuity = p->bDiscontinuity;
			p->bDiscontinuity = FALSE;

			m_p->bSyncPoint = p->bSyncPoint;
			p->bSyncPoint = FALSE;

			m_p->rtStart = p->rtStart;
			p->rtStart = Packet::INVALID_TIME;

			m_p->rtStop = p->rtStop;
			p->rtStop = Packet::INVALID_TIME;
		}
//FIX THIS
		//m_p->Append(*p);
    /*test*/
    m_p->insert(m_p->end(),(BYTE)&p->at(0));
    /*end test*/

		BYTE* start = &m_p->at(0);
		BYTE* end = start + m_p->size();
    //FIX THIS
		
		while(start <= end-4 && *(DWORD*)start != 0x01000000) start++;

		while(start <= end-4)
		{
			BYTE* next = start+1;

			while(next <= end-4 && *(DWORD*)next != 0x01000000) next++;

			if(next >= end-4) break;

			int size = next - start;

			CH264Nalu			Nalu;
			Nalu.SetBuffer (start, size, 0);

			std::auto_ptr<Packet> p2;

			while (Nalu.ReadNext())
			{
				DWORD	dwNalLength = 
					((Nalu.GetDataLength() >> 24) & 0x000000ff) |
					((Nalu.GetDataLength() >>  8) & 0x0000ff00) |
					((Nalu.GetDataLength() <<  8) & 0x00ff0000) |
					((Nalu.GetDataLength() << 24) & 0xff000000);

				std::auto_ptr<Packet> p3(DNew Packet());
				
				p3->resize (Nalu.GetDataLength()+sizeof(dwNalLength));
				memcpy (&p3->at(0), &dwNalLength, sizeof(dwNalLength));
				memcpy (&p3->at(0)+sizeof(dwNalLength), Nalu.GetDataBuffer(), Nalu.GetDataLength());
				
				if (p2.get() == NULL)
					p2 = p3;
				else
					p2->insert(p2->end(),(BYTE)&p3->at(0));
			}

			p2->TrackNumber = m_p->TrackNumber;
			p2->bDiscontinuity = m_p->bDiscontinuity;
			m_p->bDiscontinuity = FALSE;

			p2->bSyncPoint = m_p->bSyncPoint;
			m_p->bSyncPoint = FALSE;

			p2->rtStart = m_p->rtStart; m_p->rtStart = Packet::INVALID_TIME;
			p2->rtStop = m_p->rtStop;
			m_p->rtStop = Packet::INVALID_TIME;

			p2->pmt = m_p->pmt; m_p->pmt = NULL;

			m_pl.push_back(p2);

			if(p->rtStart != Packet::INVALID_TIME)
			{
				m_p->rtStart = p->rtStart;
				m_p->rtStop = p->rtStop;
				p->rtStart = Packet::INVALID_TIME;
			}
			if(p->bDiscontinuity)
			{
				m_p->bDiscontinuity = p->bDiscontinuity;
				p->bDiscontinuity = FALSE;
			}
			if(p->bSyncPoint)
			{
				m_p->bSyncPoint = p->bSyncPoint;
				p->bSyncPoint = FALSE;
			}
			if(m_p->pmt)
				DeleteMediaType(m_p->pmt);
			
			m_p->pmt = p->pmt;
			p->pmt = NULL;

			start = next;
		}
		if(start > &m_p->at(0))
		{
			//m_p->RemoveAt(0, start - &m_p->at(0));
      m_p->erase(m_p->begin(),m_p->begin() + countof(start));
		}
    //for(POSITION pos = m_pl.GetHeadPosition(); pos; m_pl.GetNext(pos))
    for (std::list<boost::shared_ptr<Packet>>::iterator it = m_pl.begin(); it != m_pl.end(); it++)
		{
      //this is not making any sense for me
			//if(pos == m_pl.GetHeadPosition()) 
		  //  continue;

			Packet* pPacket = (*it).get();//m_pl.GetAt(pos);
			BYTE* pData = &pPacket->at(0);//GetData();

			if((pData[4]&0x1f) == 0x09) m_fHasAccessUnitDelimiters = true;

			if((pData[4]&0x1f) == 0x09 || !m_fHasAccessUnitDelimiters && pPacket->rtStart != Packet::INVALID_TIME)
			{
				p.reset(m_pl.front().get());
        m_pl.pop_front();//m_pl.RemoveHead();
        while (*it != m_pl.front())
        {
          boost::shared_ptr<Packet> p2 = m_pl.front();
          m_pl.pop_front();
          p->insert(p->end(),(BYTE)&p2->at(0));
        }
				/*while(pos != m_pl.GetHeadPosition())
				{
					std::auto_ptr<Packet> p2 = m_pl.RemoveHead();
					p->Append(*p2);
				}*/

				HRESULT hr = __super::DeliverPacket(p);
				if(hr != S_OK) return hr;
			}
		}

		return S_OK;
	}
	else if(m_mt.subtype == FOURCCMap('1CVW') || m_mt.subtype == FOURCCMap('1cvw')) // just like aac, this has to be starting nalus, more can be packed together
	{
		if(!m_p.get())
		{
			m_p.reset(DNew Packet());
			m_p->TrackNumber = p->TrackNumber;
			m_p->bDiscontinuity = p->bDiscontinuity;
			p->bDiscontinuity = FALSE;

			m_p->bSyncPoint = p->bSyncPoint;
			p->bSyncPoint = FALSE;

			m_p->rtStart = p->rtStart;
			p->rtStart = Packet::INVALID_TIME;

			m_p->rtStop = p->rtStop;
			p->rtStop = Packet::INVALID_TIME;
		}

		m_p->insert(m_p->end(),(BYTE)(*p).at(0));

		BYTE* start = &m_p->at(0);
		BYTE* end = start + m_p->size();

		bool bSeqFound = false;
		while(start <= end-4)
		{
			if (*(DWORD*)start == 0x0D010000)
			{
				bSeqFound = true;
				break;
			}
			else if (*(DWORD*)start == 0x0F010000)
				break;
			start++;
		}

		while(start <= end-4)
		{
			BYTE* next = start+1;

			while(next <= end-4)
			{
				if (*(DWORD*)next == 0x0D010000)
				{
					if (bSeqFound) break;
					bSeqFound = true;
				}
				else if (*(DWORD*)next == 0x0F010000)
					break;
				next++;
			}

			if(next >= end-4) break;

			int size = next - start - 4;


			std::auto_ptr<Packet> p2(DNew Packet());
			p2->TrackNumber = m_p->TrackNumber;
			p2->bDiscontinuity = m_p->bDiscontinuity;
			m_p->bDiscontinuity = FALSE;

			p2->bSyncPoint = m_p->bSyncPoint;
			m_p->bSyncPoint = FALSE;

			p2->rtStart = m_p->rtStart;
			m_p->rtStart = Packet::INVALID_TIME;

			p2->rtStop = m_p->rtStop;
			m_p->rtStop = Packet::INVALID_TIME;

			p2->pmt = m_p->pmt;
			m_p->pmt = NULL;

			p2->SetData(start, next - start);

			HRESULT hr = __super::DeliverPacket(p2);
			if(hr != S_OK) return hr;

			if(p->rtStart != Packet::INVALID_TIME)
			{
				m_p->rtStart = p->rtStart;
				m_p->rtStop = p->rtStop;
				p->rtStart = Packet::INVALID_TIME;
			}
			if(p->bDiscontinuity)
			{
				m_p->bDiscontinuity = p->bDiscontinuity;
				p->bDiscontinuity = FALSE;
			}
			if(p->bSyncPoint)
			{
				m_p->bSyncPoint = p->bSyncPoint;
				p->bSyncPoint = FALSE;
			}
			if(m_p->pmt)
				DeleteMediaType(m_p->pmt);
			
			m_p->pmt = p->pmt;
			p->pmt = NULL;

			start = next;
			bSeqFound = (*(DWORD*)start == 0x0D010000);
		}

		if(start > &m_p->at(0))
		{
			//m_p->RemoveAt(0, start - m_p->GetData());
      m_p->erase(m_p->begin(),m_p->begin() + countof(start));
		}

		return S_OK;
	}
	else if (m_mt.subtype == MEDIASUBTYPE_DTS || m_mt.subtype == MEDIASUBTYPE_WAVE_DTS) // DTS HD MA data is causing trouble, lets just remove it
	{
#if 0
		BYTE* start = p->GetData();
		BYTE* end = start + p->size();
		if (end - start < 4 && !p->pmt)
			return S_OK;  // Should be invalid packet

		BYTE* hdr = start;


		int Type;
		  // 16 bits big endian bitstream
		  if      (hdr[0] == 0x7f && hdr[1] == 0xfe &&
				   hdr[2] == 0x80 && hdr[3] == 0x01)
			Type = 16 + 32;

		  // 16 bits low endian bitstream
		  else if (hdr[0] == 0xfe && hdr[1] == 0x7f &&
				   hdr[2] == 0x01 && hdr[3] == 0x80)
			Type = 16;

		  // 14 bits big endian bitstream
		  else if (hdr[0] == 0x1f && hdr[1] == 0xff &&
				   hdr[2] == 0xe8 && hdr[3] == 0x00 &&
				   hdr[4] == 0x07 && (hdr[5] & 0xf0) == 0xf0)
			Type = 14 + 32;

		  // 14 bits low endian bitstream
		  else if (hdr[0] == 0xff && hdr[1] == 0x1f &&
				   hdr[2] == 0x00 && hdr[3] == 0xe8 &&
				  (hdr[4] & 0xf0) == 0xf0 && hdr[5] == 0x07)
			Type = 14;

		  // no sync
		  else if (!p->pmt)
		  {
			  return S_OK;
		  }

#endif

	}
	else if (m_mt.subtype == MEDIASUBTYPE_HDMV_LPCM_AUDIO)
	{
		BYTE* start = &p->at(0);
		p->SetData(start + 4, p->size() - 4);
	}
	else
	{
		m_p.release();
		m_pl.clear();
	}

	return __super::DeliverPacket(p);
}


STDMETHODIMP CMpegSplitterOutputPin::Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt)
{
	HRESULT		hr;
	PIN_INFO	PinInfo;
	GUID		FilterClsid;

	if (SUCCEEDED (pReceivePin->QueryPinInfo (&PinInfo)))
	{
		if (SUCCEEDED (PinInfo.pFilter->GetClassID(&FilterClsid)) && (FilterClsid == CLSID_DMOWrapperFilter))
			(static_cast<CMpegSplitterFilter*>(m_pFilter))->SetPipo(true);
		PinInfo.pFilter->Release();
	}

	hr = __super::Connect (pReceivePin, pmt);
	(static_cast<CMpegSplitterFilter*>(m_pFilter))->SetPipo(false);
	return hr;
}
