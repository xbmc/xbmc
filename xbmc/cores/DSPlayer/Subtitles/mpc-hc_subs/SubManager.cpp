#include "stdafx.h"
#include <d3d9.h>
#include "..\subpic\ISubPic.h"
#include "..\subpic\DX9SubPic.h"
#include <moreuuids.h>
#include "..\subtitles\VobSubFile.h"
#include "..\subtitles\RTS.h"
#include "..\DSUtil\NullRenderers.h"
#include "SubManager.h"
#include "TextPassThruFilter.h"

STSStyle g_style;
BOOL g_overrideUserStyles;
int g_subPicsBufferAhead(3);
Com::SmartSize g_textureSize(800, 600);
bool g_pow2tex(true);
BOOL g_disableAnim(TRUE);

CSubManager::CSubManager(IDirect3DDevice9* d3DDev, SIZE size, HRESULT& hr)
	: m_d3DDev(d3DDev),
	m_isSetTime(false),
	m_iSubtitleSel(-1),
	m_rtNow(-1),
	m_delay(0),
	m_lastSize(size)
{
  g__rtTimePerFrame = 0; // Variable set on IPinHook on XBMC side
  g__tSampleStart = 0;
  g__tSegmentStart = 0;

	//ATLTRACE("CSubManager constructor: texture size %dx%d, buffer ahead: %d, pow2tex: %d", g_textureSize.cx, g_textureSize.cy, g_subPicsBufferAhead, g_pow2tex);
	m_pAllocator = new CDX9SubPicAllocator(d3DDev, g_textureSize, g_pow2tex/*AfxGetAppSettings().fSPCPow2Tex*/);
	hr = S_OK;
	if (g_subPicsBufferAhead > 0)
		m_pSubPicQueue = new CSubPicQueue(g_subPicsBufferAhead, g_disableAnim, m_pAllocator, &hr);
	else
		m_pSubPicQueue = new CSubPicQueueNoThread(m_pAllocator, &hr);
	if (FAILED(hr))
	{
    //ATLTRACE("CSubPicQueue creation error: %x", hr);
	}
}

CSubManager::~CSubManager(void)
{
	//ATLTRACE("CSubManager destructor");
}

void CSubManager::ApplyStyle(CRenderedTextSubtitle* pRTS) {
	/*if (g_overrideUserStyles)
	{
		if (pRTS->m_styles.size() > 1)
		{ //remove all styles besides Default
			CStdString def = _T("Default");
			STSStyle* defStyle = NULL;
      std::map<CStdString, STSStyle*>::iterator it = pRTS->m_styles.find(def);
			if (it != pRTS->m_styles.end())
			{
				pRTS->m_styles.erase(it);
			}
			pRTS->m_styles.clear();
			pRTS->m_styles[def] = defStyle;
		}
		//m_dstScreenSize defined by PlayResX, PlayResY params of ass
		//need to set to MPC default which is 384x288 (CSimpleTextSubtitle::Open)
		pRTS->m_dstScreenSize = Com::SmartSize(384, 288);
		pRTS->SetDefaultStyle(g_style);
	}
	else if (pRTS->m_fUsingAutoGeneratedDefaultStyle)
	{
		pRTS->SetDefaultStyle(g_style);
	}
	pRTS->Deinit();*/
}

void CSubManager::ApplyStyleSubStream(ISubStream* pSubStream)
{
	CLSID clsid;
	if(FAILED(pSubStream->GetClassID(&clsid)))
		return;
	if(clsid == __uuidof(CRenderedTextSubtitle))
	{
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)pSubStream;
		ApplyStyle(pRTS);
	}
}

void CSubManager::SetSubPicProvider(ISubStream* pSubStream)
{
	m_pSubStream = pSubStream;
	m_pSubPicQueue->SetSubPicProvider(Com::SmartQIPtr<ISubPicProvider>(pSubStream));
	m_delay = 0;
	m_subresync.RemoveAll();
}

void CSubManager::ReplaceSubtitle(ISubStream* pSubStreamOld, ISubStream* pSubStreamNew)
{
	ApplyStyleSubStream(pSubStreamNew);
	m_intSubStream = pSubStreamNew;
	if (m_iSubtitleSel >= GetExtCount())
	{
		//ATLTRACE("ReplaceSubtitle: SetSubPicProvider");
		SetSubPicProvider(pSubStreamNew);
	}
}

void CSubManager::InvalidateSubtitle(DWORD_PTR nSubtitleId, REFERENCE_TIME rtInvalidate)
{
	if(m_iSubtitleSel >= GetExtCount())
	{
		//ATLTRACE("InvalidateSubtitle");
		m_pSubPicQueue->Invalidate(rtInvalidate);
	}
}

void  CSubManager::SelectStream(int i)
{
	if (i < (int) m_intSubs.size())
	{
		//ATLTRACE("SelectStream %d", i);
		int index = m_intSubs[i];
		DWORD dwFlags = 0;
		if (SUCCEEDED(m_pSS->Info(index, NULL, &dwFlags, NULL, NULL, NULL, NULL, NULL)))
		{
			if(dwFlags&(AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE)) 
			{
				SetSubPicProvider(m_intSubStream);
			}
			else
			{
				m_pSS->Enable(index, AMSTREAMSELECTENABLE_ENABLE);
			}
		}
	}
	else
	{
		m_pSubPicQueue->SetSubPicProvider(NULL);
	}
}

void CSubManager::UpdateSubtitle()
{
	//ATLTRACE("UpdateSubtitle");
	int i = m_iSubtitleSel;

  std::list<ISubStream *>::iterator it = m_pSubStreams.begin();
  while(it != m_pSubStreams.end() && i >= 0)
	{
		ISubStream* pSubStream = *it; it++;
		if(i < pSubStream->GetStreamCount()) 
		{
			CAutoLock cAutoLock(&m_csSubLock);
			pSubStream->SetStream(i);
			//m_nSubtitleId = (DWORD_PTR)pSubStream.p;
			SetSubPicProvider(pSubStream);
			return;
		}
		i -= pSubStream->GetStreamCount();
	}

	if (i >= 0)
	{
		SelectStream(i);
	}
	else
	{
		m_pSubPicQueue->SetSubPicProvider(NULL);
	}
}

int CSubManager::GetExtCount() 
{
	int cnt = 0;
	std::list<ISubStream *>::iterator pos = m_pSubStreams.begin();
	while(pos != m_pSubStreams.end())
  {
    cnt += (*pos)->GetStreamCount(); pos++;
  }
	return cnt;
}

int CSubManager::GetCount() 
{
	return GetExtCount() + m_intSubs.size();
}

BSTR CSubManager::GetLanguage(int i)
{
	std::list<ISubStream *>::iterator pos = m_pSubStreams.begin();
	while(pos != m_pSubStreams.end() && i >= 0)
	{
		ISubStream* pSubStream = *pos; pos++;

		if(i < pSubStream->GetStreamCount()) 
		{
			WCHAR* pName = NULL;
			if(SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, NULL)))
			{
				_bstr_t res(pName);
				CoTaskMemFree(pName);
				return res.Detach();
			}
			return 0;
		}
		i -= pSubStream->GetStreamCount();
	}

	return (i >= 0 && i < (int)m_intNames.size()) ? m_intNames[i].AllocSysString() : NULL;
}

int CSubManager::GetCurrent()
{
	return m_iSubtitleSel&0x7fffffff;
}

void CSubManager::SetCurrent(int current)
{
	m_iSubtitleSel = current | (m_iSubtitleSel&0x80000000); 
	UpdateSubtitle();
}

void CSubManager::SetEnable(BOOL enable)
{
	if ((enable && m_iSubtitleSel < 0) || (!enable && m_iSubtitleSel>= 0))
	{
		m_iSubtitleSel ^= 0x80000000;
		UpdateSubtitle();
	}
}

BOOL CSubManager::GetEnable()
{
	return m_iSubtitleSel >= 0;
}

void CSubManager::SetTime(REFERENCE_TIME nsSampleTime)
{
	m_rtNow = g__tSegmentStart + nsSampleTime - m_delay;
	m_pSubPicQueue->SetTime(m_rtNow);
	m_isSetTime = true;
}

HRESULT CSubManager::GetTexture(Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest)
{
  if (m_iSubtitleSel < 0)
    return E_INVALIDARG;

  if (!m_isSetTime)
  {
    m_rtNow = g__tSegmentStart + g__tSampleStart - m_delay;
    m_pSubPicQueue->SetTime(m_rtNow);
  }

  m_fps = 10000000.0 / g__rtTimePerFrame;
  m_pSubPicQueue->SetFPS(m_fps);

  Com::SmartPtr<ISubPic> pSubPic;
  if(m_pSubPicQueue->LookupSubPic(m_rtNow, pSubPic)) 
  {
    Com::SmartSize size(1440, 900);
    if (SUCCEEDED (pSubPic->GetSourceAndDest(&size, pSrc, pDest)))
    {
      return pSubPic->GetTexture(pTexture);      
    }
  }

  return E_FAIL;
}

void CSubManager::Render(int x, int y, int width, int height)
{
	if (m_iSubtitleSel < 0)
		return;

	if (!m_isSetTime)
  {
    m_rtNow = g__tSegmentStart + g__tSampleStart - m_delay;
    m_pSubPicQueue->SetTime(m_rtNow);
  }

	m_fps = 10000000.0 / g__rtTimePerFrame;
	m_pSubPicQueue->SetFPS(m_fps);

	Com::SmartSize size(width, height);
	if (m_lastSize != size && width > 0 && height > 0)
	{ //adjust texture size
		//ATLTRACE("Size change from %dx%d to %dx%d", m_lastSize.cx, m_lastSize.cy, size.cx, size.cy);
		m_pAllocator->ChangeDevice(m_d3DDev);
		//m_pAllocator->SetMaxTextureSize(g_textureSize);
		m_pAllocator->SetCurVidRect(Com::SmartRect(Com::SmartPoint(0,0), size));
		m_pSubPicQueue->Invalidate(m_rtNow+1000000);
		m_lastSize = size;
	}

	Com::SmartPtr<ISubPic> pSubPic;
	if(m_pSubPicQueue->LookupSubPic(m_rtNow, pSubPic)) 
	{
 		Com::SmartRect rcSource, rcDest;
		if (SUCCEEDED (pSubPic->GetSourceAndDest(&size, rcSource, rcDest))) {
			rcDest.OffsetRect(x, y);
			DWORD fvf, alphaTest, colorOp;
			m_d3DDev->GetFVF(&fvf);
			m_d3DDev->GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest); 
			m_d3DDev->GetTextureStageState(0, D3DTSS_COLOROP, &colorOp); //change to it causes "white" osd artifact  

			m_d3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 

			pSubPic->AlphaBlt(rcSource, rcDest, NULL/*pTarget*/);

			m_d3DDev->SetFVF(fvf);
			m_d3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest); 
			m_d3DDev->SetTextureStageState(0, D3DTSS_COLOROP, colorOp);
		}
	}
}

static bool IsTextPin(IPin* pPin)
{
	bool isText = false;
	BeginEnumMediaTypes(pPin, pEMT, pmt)
	{
		if (pmt->majortype == MEDIATYPE_Text || pmt->majortype == MEDIATYPE_Subtitle)
		{
			isText = true;
			break;
		}
	}
	EndEnumMediaTypes(pmt)
	return isText;
}

static bool isTextConnection(IPin* pPin)
{
	AM_MEDIA_TYPE mt;
	if (FAILED(pPin->ConnectionMediaType(&mt)))
		return false;
	bool isText = (mt.majortype == MEDIATYPE_Text || mt.majortype == MEDIATYPE_Subtitle);
	FreeMediaType(mt);
	return isText;
}

//load internal subtitles through TextPassThruFilter
void CSubManager::LoadInternalSubtitles(IGraphBuilder* pGB)
{
	BeginEnumFilters(pGB, pEF, pBF)
	{
		//ATLTRACE(L"Processing filter: %s", GetFilterName(pBF).GetString());
		if(!IsSplitter(pBF)) continue;
		//ATLTRACE("Is splitter!");
		BeginEnumPins(pBF, pEP, pPin)
		{
			PIN_DIRECTION pindir;
			pPin->QueryDirection(&pindir);
			if (pindir != PINDIR_OUTPUT)
				continue;
			Com::SmartPtr<IPin> pPinTo;
			pPin->ConnectedTo(&pPinTo);
			if (pPinTo)
			{
				if (!isTextConnection(pPin))
					continue;
				pGB->Disconnect(pPin);
				pGB->Disconnect(pPinTo);
			}
			else if (!IsTextPin(pPin))
				continue;
			
			Com::SmartQIPtr<IBaseFilter> pTPTF = new CTextPassThruFilter(this);
			CStdStringW name;
			name.Format(L"TextPassThru%08x", pTPTF);
			if(FAILED(pGB->AddFilter(pTPTF, name)))
				continue;

			Com::SmartQIPtr<ISubStream> pSubStream;
			HRESULT hr;
			do
			{
				if (FAILED(hr = pGB->ConnectDirect(pPin, GetFirstPin(pTPTF, PINDIR_INPUT), NULL)))
				{
					//ATLTRACE("connection to TextPassThruFilter failed: %x", hr);
					break;
				}
				Com::SmartQIPtr<IBaseFilter> pNTR = new CNullTextRenderer(NULL, &hr);
				if (FAILED(hr) || FAILED(pGB->AddFilter(pNTR, NULL)))
					break;

				if FAILED(hr = pGB->ConnectDirect(GetFirstPin(pTPTF, PINDIR_OUTPUT), GetFirstPin(pNTR, PINDIR_INPUT), NULL))
					break;
				pSubStream = pTPTF;
			} while(0);

			if (pSubStream)
			{
				ApplyStyleSubStream(pSubStream);
				m_intSubStream = pSubStream;
				//m_pSubStreams.AddTail(pSubStream);
				InitInternalSubs(pBF);
				return;
			}
			else
			{
				//ATLTRACE("TextPassThruFilter removed");
				pGB->RemoveFilter(pTPTF);
			}
		}
		EndEnumPins
	}
	EndEnumFilters
}

void CSubManager::InitInternalSubs(IBaseFilter* pBF)
{
	m_pSS = pBF;
	if(!m_pSS) return;
	DWORD cStreams = 0;
	if(SUCCEEDED(m_pSS->Count(&cStreams)))
	{
		for(int i = 0; i < (int)cStreams; i++)
		{
			DWORD dwFlags = 0;
			LCID lcid = 0;
			DWORD dwGroup = 0;
			WCHAR* pszName = NULL;
			if(FAILED(m_pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
				continue;

			if(dwGroup == 2)
			{
				CStdString lang;
				if (lcid == 0)
				{
					lang = pszName;
					if (lang.Find(L"No ") >= 0)
						lang.Empty();
				}
				else
				{
					int len = GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
					//lang.ReleaseBufferSetLength(max(len-1, 0));
				}
				if (!lang.IsEmpty())
				{
					m_intSubs.push_back(i);
					m_intNames.push_back(lang);
				}
			}

			if(pszName) CoTaskMemFree(pszName);
		}
	}


}

CStdString GetExtension(CStdString&  filename)
{
  const size_t i = filename.rfind('.');
  return filename.substr(i+1, filename.size());
}

void CSubManager::LoadExternalSubtitles(const wchar_t* filename, const wchar_t* subpaths)
{
	m_movieFile = filename;
	std::vector<CStdString> paths;
		
	CStdString allpaths=subpaths;
	CStdString path;
	int start = 0;
	int prev = 0;
		
	while (start != -1)
  {
		start = allpaths.Find(',', start);
		if(start > 0)
		{
		  path=allpaths.Mid(prev,start);
			paths.push_back(path);				
			int end = allpaths.Find(',', start+1);
			if(end > start)
			{  
				path=allpaths.Mid(start+1,end-start-1);
			  paths.push_back(path);
				prev = allpaths.Find(',', end+1);
				if(prev > 0)
				{
					start++;
				  prev = start;
				}
				else
				{
          path=allpaths.Right(allpaths.GetLength()-end-1);
          paths.push_back(path);
					start=-1;
				}
			}
			else
			{
				path=allpaths.Right(allpaths.GetLength()-start-1);
				paths.push_back(path);
				start=-1;
			}			
		}
		else if(allpaths.GetLength() > 0)
		{
      paths.push_back(allpaths);		
			start=-1;		
		}	
	}
	
	if(paths.size() <= 0)
	{
		paths.push_back(_T("."));
	  paths.push_back(_T(".\\subtitles"));
	  paths.push_back(_T("c:\\subtitles"));
	}

	std::vector<SubFile> ret;	
	GetSubFileNames(m_movieFile, paths, ret);	

	for(size_t i = 0; i < ret.size(); i++)
	{
		// TMP: maybe this will catch something for those who get a runtime error dialog when opening subtitles from cds
		try
		{
			ISubStream* pSubStream;

			if(!pSubStream)
			{
        std::auto_ptr<CVobSubFile> pVSF(new CVobSubFile(&m_csSubLock));
				if(CStdString(GetExtension(ret[i].fn).MakeLower()) == _T(".idx") && pVSF.get() && pVSF->Open(ret[i].fn) && pVSF->GetStreamCount() > 0)
					pSubStream = pVSF.release();
			}

			if(!pSubStream)
			{
				std::auto_ptr<CRenderedTextSubtitle> pRTS(new CRenderedTextSubtitle(&m_csSubLock));
				if(pRTS.get() && pRTS->Open(ret[i].fn, DEFAULT_CHARSET) && pRTS->GetStreamCount() > 0) {
					ApplyStyle(pRTS.get());
					pSubStream = pRTS.release();
				}
			}
			if (pSubStream)
			{
				m_pSubStreams.insert(m_pSubStreams.end(), pSubStream);
			}
		}
		catch(... /*CException* e*/)
		{
      //e->Delete();
		}
	}
}

int CSubManager::GetDelay()
{
	return (int)(m_delay/10000);
}

void CSubManager::SetDelay(int delay_ms)
{
	m_subresync.AddShift(m_rtNow + m_delay, delay_ms - GetDelay());
	m_delay = delay_ms*10000;
}

void CSubManager::SaveToDisk()
{
	if (m_iSubtitleSel >= 0)
	{
		m_subresync.SaveToDisk(m_pSubStream, m_fps, m_movieFile);
	}
}

void CSubManager::LoadSubtitlesForFile(const wchar_t* fn, IGraphBuilder* pGB, const wchar_t* paths)
{
	LoadInternalSubtitles(pGB);	
	LoadExternalSubtitles(fn, paths);
	if(GetCount() > 0)
	{
		m_iSubtitleSel = 0x80000000; //stream 0, disabled
	} 
}

void CSubManager::SetTimePerFrame( REFERENCE_TIME timePerFrame )
{
  g__rtTimePerFrame = timePerFrame;
}

void CSubManager::SetSegmentStart( REFERENCE_TIME segmentStart )
{
  g__tSegmentStart = segmentStart;
}

void CSubManager::SetSampleStart( REFERENCE_TIME sampleStart )
{
  g__tSampleStart = sampleStart;
}