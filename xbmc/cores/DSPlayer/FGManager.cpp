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

#include "FGManager.h"

#include <mpconfig.h> //IMixerConfig


#include "DShowUtil/dshowutil.h"
#include "DShowUtil/DshowCommon.h"

#include "filters/DX9AllocatorPresenter.h"
#include "filters/xbmcfilesource.h"
#include "filters/asyncflt.h"
#include "filters/asyncio.h"
#include <initguid.h>
#include "helpers/moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>
#include <Vmr9.h>

//XML CONFIG HEADERS
#include "Log.h"
#include "tinyXML/tinyxml.h"
#include "XMLUtils.h"
//END XML CONFIG HEADERS

enum
{
  SRC_CDDA      = 1, 
  SRC_CDXA      = SRC_CDDA<<1,
  SRC_VTS       = SRC_CDXA<<1,
  SRC_FLIC      = SRC_VTS<<1,
  SRC_D2V       = SRC_FLIC<<1,
  SRC_DTSAC3    = SRC_D2V<<1,
  SRC_MATROSKA  = SRC_DTSAC3<<1,
  SRC_SHOUTCAST = SRC_MATROSKA<<1,
  SRC_REALMEDIA = SRC_SHOUTCAST<<1,
  SRC_AVI       = SRC_REALMEDIA<<1,
  SRC_RADGT     = SRC_AVI<<1,
  SRC_ROQ       = SRC_RADGT<<1,
  SRC_OGG       = SRC_ROQ<<1,
  SRC_NUT       = SRC_OGG<<1,
  SRC_MPEG      = SRC_NUT<<1,
  SRC_DIRAC     = SRC_MPEG<<1,
  SRC_MPA       = SRC_DIRAC<<1,
  SRC_DSM       = SRC_MPA<<1,
  SRC_SUBS      = SRC_DSM<<1,
  SRC_MP4       = SRC_SUBS<<1,
  SRC_FLV       = SRC_MP4<<1,  
  SRC_FLAC      = SRC_FLV<<1,
  SRC_LAST      = SRC_FLAC<<1
};

using namespace std;

//
// CFGManager
//

CFGManager::CFGManager(LPCTSTR pName, LPUNKNOWN pUnk)
  : CUnknown(pName, pUnk)
  , m_dwRegister(0)
{
  m_pUnkInner.CoCreateInstance(CLSID_FilterGraph, GetOwner());
  m_pFM.CoCreateInstance(CLSID_FilterMapper2);
}

CFGManager::~CFGManager()
{
  CAutoLock cAutoLock(this);
  while(!m_source.IsEmpty()) delete m_source.RemoveHead();
  while(!m_transform.IsEmpty()) delete m_transform.RemoveHead();
  while(!m_override.IsEmpty()) delete m_override.RemoveHead();
  while(!m_configfilter.IsEmpty()) delete m_configfilter.RemoveHead();
  while(!m_splitter.IsEmpty()) delete m_splitter.RemoveHead();
  m_FileSource.Release();
  m_Splitter.Release();
  m_XbmcVideoDec.Release();
  m_pUnks.RemoveAll();
  m_pUnkInner.Release();
}

STDMETHODIMP CFGManager::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

  return
    QI(IFilterGraph)
    QI(IGraphBuilder)
    QI(IFilterGraph2)
    QI(IGraphBuilder2)
    QI(IGraphBuilderDeadEnd)
    m_pUnkInner && (riid != IID_IUnknown && SUCCEEDED(m_pUnkInner->QueryInterface(riid, ppv))) ? S_OK :
    __super::NonDelegatingQueryInterface(riid, ppv);
}

//

void CFGManager::CStreamPath::Append(IBaseFilter* pBF, IPin* pPin)
{
  path_t p;
  p.clsid = DShowUtil::GetCLSID(pBF);
  p.filter = DShowUtil::GetFilterName(pBF);
  p.pin = DShowUtil::GetPinName(pPin);
  AddTail(p);
}

bool CFGManager::CStreamPath::Compare(const CStreamPath& path)
{
  POSITION pos1 = GetHeadPosition();
  POSITION pos2 = path.GetHeadPosition();

  while(pos1 && pos2)
  {
    const path_t& p1 = GetNext(pos1);
    const path_t& p2 = path.GetNext(pos2);

    if(p1.filter != p2.filter) return true;
    else if(p1.pin != p2.pin) return false;
  }

  return true;
}

bool CFGManager::CheckBytes(HANDLE hFile, CStdString chkbytes)
{
  
  CAtlList<CStdString> sl;
  Explode(chkbytes, sl, ',');

  if(sl.GetCount() < 4)
    return false;

  LARGE_INTEGER size = {0, 0};
  size.LowPart = GetFileSize(hFile, (DWORD*)&size.HighPart);

  POSITION pos = sl.GetHeadPosition();
  while(sl.GetCount() >= 4)
  {
    CStdString offsetstr = sl.RemoveHead();
    CStdString cbstr = sl.RemoveHead();
    CStdString maskstr = sl.RemoveHead();
    CStdString valstr = sl.RemoveHead();

    long cb = _ttol(cbstr);

    if(offsetstr.IsEmpty() || cbstr.IsEmpty() 
    || valstr.IsEmpty() || (valstr.GetLength() & 1)
    || cb*2 != valstr.GetLength())
      return false;

    LARGE_INTEGER offset;
    offset.QuadPart = _ttoi64(offsetstr);
    if(offset.QuadPart < 0) offset.QuadPart = size.QuadPart - offset.QuadPart;
    SetFilePointer(hFile, offset.LowPart, &offset.HighPart, FILE_BEGIN);

    // LAME
    while(maskstr.GetLength() < valstr.GetLength())
      maskstr += 'F';

    CAtlArray<BYTE> mask, val;
    DShowUtil::CStringToBin(maskstr, mask);
    DShowUtil::CStringToBin(valstr, val);

    for(size_t i = 0; i < val.GetCount(); i++)
    {
      BYTE b;
      DWORD r;
      if(!ReadFile(hFile, &b, 1, &r, NULL) || (b & mask[i]) != val[i])
        return false;
    }
  }

  return true;
}

HRESULT CFGManager::CreateFilter(CFGFilter* pFGF, IBaseFilter** ppBF, IUnknown** ppUnk)
{
  CheckPointer(pFGF, E_POINTER);
  CheckPointer(ppBF, E_POINTER);
  CheckPointer(ppUnk, E_POINTER);

  CComPtr<IBaseFilter> pBF;
  //CComPtr<IUnknown> pUnk;
  CInterfaceList<IUnknown, &IID_IUnknown> pUnk;
  if(FAILED(pFGF->Create(&pBF, pUnk)))
    return E_FAIL;

  *ppBF = pBF.Detach();
  //if(pUnk) *ppUnk = pUnk.Detach();
  m_pUnks.AddTailList(&pUnk);

  return S_OK;
}

HRESULT CFGManager::AddSourceFilter(CFGFilter* pFGF, LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppBF)
{
  //TRACE(_T("FGM: AddSourceFilter trying '%s'\n"), CStringFromGUID(pFGF->GetCLSID()));

  CheckPointer(lpcwstrFileName, E_POINTER);
  CheckPointer(ppBF, E_POINTER);

  ASSERT(*ppBF == NULL);

  HRESULT hr;

  CComPtr<IBaseFilter> pBF;
  CComPtr<IUnknown> pUnk;
  if(FAILED(hr = CreateFilter(pFGF, &pBF, &pUnk)))
    return hr;

  CComQIPtr<IFileSourceFilter> pFSF = pBF;
  if(!pFSF) return E_NOINTERFACE;

  if(FAILED(hr = AddFilter(pBF, lpcwstrFilterName)))
    return hr;

  const AM_MEDIA_TYPE* pmt = NULL;

  CMediaType mt;
  const CAtlList<GUID>& types = pFGF->GetTypes();
  if(types.GetCount() == 2 && (types.GetHead() != GUID_NULL || types.GetTail() != GUID_NULL))
  {
    mt.majortype = types.GetHead();
    mt.subtype = types.GetTail();
    pmt = &mt;
  }

  if(FAILED(hr = pFSF->Load(lpcwstrFileName, pmt)))
  {
    RemoveFilter(pBF);
    return hr;
  }

  *ppBF = pBF.Detach();

  if(pUnk) m_pUnks.AddTail(pUnk);

  return S_OK;
}

// IFilterGraph

STDMETHODIMP CFGManager::AddFilter(IBaseFilter* pFilter, LPCWSTR pName)
{
  CAutoLock cAutoLock(this);

  HRESULT hr;

  if(FAILED(hr = CComQIPtr<IFilterGraph2>(m_pUnkInner)->AddFilter(pFilter, pName)))
    return hr;

  // TODO
  hr = pFilter->JoinFilterGraph(NULL, NULL);
  hr = pFilter->JoinFilterGraph(this, pName);

  return hr;
}

STDMETHODIMP CFGManager::RemoveFilter(IBaseFilter* pFilter)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->RemoveFilter(pFilter);
}

STDMETHODIMP CFGManager::EnumFilters(IEnumFilters** ppEnum)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->EnumFilters(ppEnum);
}

STDMETHODIMP CFGManager::FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->FindFilterByName(pName, ppFilter);
}

STDMETHODIMP CFGManager::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ConnectDirect(pPinOut, pPinIn, pmt);
}

STDMETHODIMP CFGManager::Reconnect(IPin* ppin)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Reconnect(ppin);
}

STDMETHODIMP CFGManager::Disconnect(IPin* ppin)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Disconnect(ppin);
}

STDMETHODIMP CFGManager::SetDefaultSyncSource()
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->SetDefaultSyncSource();
}

// IGraphBuilder

STDMETHODIMP CFGManager::Connect(IPin* pPinOut, IPin* pPinIn)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPinOut, E_POINTER);

  HRESULT hr;

  {
    PIN_DIRECTION dir;
    if(FAILED(hr = pPinOut->QueryDirection(&dir)) || dir != PINDIR_OUTPUT
    || pPinIn && (FAILED(hr = pPinIn->QueryDirection(&dir)) || dir != PINDIR_INPUT))
      return VFW_E_INVALID_DIRECTION;

    CComPtr<IPin> pPinTo;
    if(SUCCEEDED(hr = pPinOut->ConnectedTo(&pPinTo)) || pPinTo
    || pPinIn && (SUCCEEDED(hr = pPinIn->ConnectedTo(&pPinTo)) || pPinTo))
      return VFW_E_ALREADY_CONNECTED;
  }

  bool fDeadEnd = true;

  if(pPinIn)
  {
    // 1. Try a direct connection between the filters, with no intermediate filters

    if(SUCCEEDED(hr = ConnectDirect(pPinOut, pPinIn, NULL)))
      return hr;

    // Since pPinIn is our target we must not allow:
    // - intermediate filters with no output pins 
    // - input pins being on by the same filter, that would lead to circular graph
  }
  else
  {
    // 1. Use IStreamBuilder

    if(CComQIPtr<IStreamBuilder> pSB = pPinOut)
    {
      if(SUCCEEDED(hr = pSB->Render(pPinOut, this)))
        return hr;

      pSB->Backout(pPinOut, this);
    }
  }

  // 2. Try cached filters

  if(CComQIPtr<IGraphConfig> pGC = (IGraphBuilder2*)this)
  {
    BeginEnumCachedFilters(pGC, pEF, pBF)
    {
      if(pPinIn && (DShowUtil::IsStreamEnd(pBF) || DShowUtil::GetFilterFromPin(pPinIn) == pBF))
        continue;

      hr = pGC->RemoveFilterFromCache(pBF);

      if(SUCCEEDED(hr = ConnectFilterDirect(pPinOut, pBF, NULL)))
      {
        if(!DShowUtil::IsStreamEnd(pBF)) fDeadEnd = false;

        if(SUCCEEDED(hr = ConnectFilter(pBF, pPinIn)))
          return hr;
      }

      hr = pGC->AddFilterToCache(pBF);
    }
    EndEnumCachedFilters
  }

  // 3. Try filters in the graph

  {
    CInterfaceList<IBaseFilter> pBFs;

    BeginEnumFilters(this, pEF, pBF)
    {
      if(pPinIn && (DShowUtil::IsStreamEnd(pBF) || DShowUtil::GetFilterFromPin(pPinIn) == pBF)
      || DShowUtil::GetFilterFromPin(pPinOut) == pBF)
        continue;

      // HACK: ffdshow - audio capture filter
      if(DShowUtil::GetCLSID(pPinOut) == DShowUtil::GUIDFromCString(_T("{04FE9017-F873-410E-871E-AB91661A4EF7}"))
      && DShowUtil::GetCLSID(pBF) == DShowUtil::GUIDFromCString(_T("{E30629D2-27E5-11CE-875D-00608CB78066}")))
        continue;

      pBFs.AddTail(pBF);
    }
    EndEnumFilters

    POSITION pos = pBFs.GetHeadPosition();
    while(pos)
    {
      IBaseFilter* pBF = pBFs.GetNext(pos);

      if(SUCCEEDED(hr = ConnectFilterDirect(pPinOut, pBF, NULL)))
      {
        if(! DShowUtil::IsStreamEnd(pBF)) fDeadEnd = false;

        if(SUCCEEDED(hr = ConnectFilter(pBF, pPinIn)))
          return hr;
      }

      EXECUTE_ASSERT(Disconnect(pPinOut));
    }
  }

  // 4. Look up filters in the registry
  
  {
    CFGFilterList fl;

    CAtlArray<GUID> types;
     DShowUtil::ExtractMediaTypes(pPinOut, types);

    POSITION pos = m_transform.GetHeadPosition();
    while(pos)
    {
      CFGFilter* pFGF = m_transform.GetNext(pos);
      if(pFGF->GetMerit() < MERIT64_DO_USE || pFGF->CheckTypes(types, false)) 
        fl.Insert(pFGF, 0, pFGF->CheckTypes(types, true), false);
    }

    pos = m_override.GetHeadPosition();
    while(pos)
    {
      CFGFilter* pFGF = m_override.GetNext(pos);
      if(pFGF->GetMerit() < MERIT64_DO_USE || pFGF->CheckTypes(types, false)) 
        fl.Insert(pFGF, 0, pFGF->CheckTypes(types, true), false);
    }

    CComPtr<IEnumMoniker> pEM;
    if(types.GetCount() > 0 
    && SUCCEEDED(m_pFM->EnumMatchingFilters(
      &pEM, 0, FALSE, MERIT_DO_NOT_USE+1, 
      TRUE, types.GetCount()/2, types.GetData(), NULL, NULL, FALSE,
      !!pPinIn, 0, NULL, NULL, NULL)))
    {
      for(CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
      {
        CFGFilterRegistry* pFGF = new CFGFilterRegistry(pMoniker);
        fl.Insert(pFGF, 0, pFGF->CheckTypes(types, true));
      }
    }
//crashing right there on mkv
//The subtitle might be the reason  
    pos = fl.GetHeadPosition();
    while(pos)
    {
      CFGFilter* pFGF = fl.GetNext(pos);

      //TRACE(_T("FGM: Connecting '%s'\n"), pFGF->GetName());

      CComPtr<IBaseFilter> pBF;
      CComPtr<IUnknown> pUnk;
      if(FAILED(CreateFilter(pFGF, &pBF, &pUnk)))
        continue;

      if(pPinIn &&  DShowUtil::IsStreamEnd(pBF))
        continue;
      if(FAILED(hr = AddFilter(pBF, (LPCWSTR)pFGF->GetName().c_str())))
        continue;

      hr = E_FAIL;

      if(types.GetCount() == 2 && types[0] == MEDIATYPE_Stream && types[1] != MEDIATYPE_NULL)
      {
        CMediaType mt;
        
        mt.majortype = types[0];
        mt.subtype = types[1];
        mt.formattype = FORMAT_None;
        if(FAILED(hr)) hr = ConnectFilterDirect(pPinOut, pBF, &mt);

        mt.formattype = GUID_NULL;
        if(FAILED(hr)) hr = ConnectFilterDirect(pPinOut, pBF, &mt);
      }

      if(FAILED(hr)) hr = ConnectFilterDirect(pPinOut, pBF, NULL);

      if(SUCCEEDED(hr))
      {
        if(! DShowUtil::IsStreamEnd(pBF)) fDeadEnd = false;

        hr = ConnectFilter(pBF, pPinIn);

        if(SUCCEEDED(hr))
        {
          if(pUnk) m_pUnks.AddTail(pUnk);

          // maybe the application should do this...
          if(CComQIPtr<IMixerPinConfig, &IID_IMixerPinConfig> pMPC = pUnk)
            pMPC->SetAspectRatioMode(AM_ARMODE_STRETCHED);
          if(CComQIPtr<IVMRAspectRatioControl> pARC = pBF)
            pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
          if(CComQIPtr<IVMRAspectRatioControl9> pARC = pBF)
            pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
          return hr;
        }
      }

      EXECUTE_ASSERT(SUCCEEDED(RemoveFilter(pBF)));

      //TRACE(_T("FGM: Connecting '%s' FAILED!\n"), pFGF->GetName());
    }
  }
  
  
  if(fDeadEnd)
  {
    CAutoPtr<CStreamDeadEnd> psde(new CStreamDeadEnd());
    psde->AddTailList(&m_streampath);
    BeginEnumMediaTypes(pPinOut, pEM, pmt)
      psde->mts.AddTail(CMediaType(*pmt));
    EndEnumMediaTypes(pmt)
    m_deadends.Add(psde);
  }

  return pPinIn ? VFW_E_CANNOT_CONNECT : VFW_E_CANNOT_RENDER;
}

STDMETHODIMP CFGManager::Render(IPin* pPinOut)
{return E_NOTIMPL;
  /*CAutoLock cAutoLock(this);

  return RenderEx(pPinOut, 0, NULL);*/
}

STDMETHODIMP CFGManager::GetXbmcVideoDecFilter(IMPCVideoDecFilter** pBF)
{
  CheckPointer(pBF,E_POINTER);
  *pBF = NULL;
  m_XbmcVideoDec.QueryInterface(pBF);
}

STDMETHODIMP CFGManager::RenderFileXbmc(const CFileItem& pFileItem)
{
  CAutoLock cAutoLock(this);
  HRESULT hr;
  if(m_File.Open(pFileItem.GetAsUrl().GetFileName().c_str(), READ_TRUNCATED | READ_BUFFERED))
  {
    CXBMCFileStream* pXBMCStream = new CXBMCFileStream(&m_File);
    CXBMCFileReader* pXBMCReader = new CXBMCFileReader(pXBMCStream, NULL, &hr);
    if (!pXBMCReader)
      CLog::Log(LOGERROR,"%s Failed Loading XBMC File Source filter",__FUNCTION__);
    m_FileSource = pXBMCReader;
    this->AddFilter(m_FileSource, L"XBMC File Source");
  }

  //Connecting the splitter for the current filetype
  POSITION Pos = m_splitter.GetHeadPosition();
  CComPtr<IBaseFilter> ppSBF;
  while (Pos)
  {
    CFGFilterFile* pFGF = m_splitter.GetNext(Pos);
	if (pFGF->GetXFileType().Equals(pFileItem.GetAsUrl().GetFileType().c_str(),false))
	{ 
	  if (SUCCEEDED(::AddFilterByCLSID(this,pFGF->GetCLSID(),&ppSBF, L"XBMC Splitter" )))
	  {
        hr = ::ConnectFilters(this,m_FileSource,ppSBF);
        if ( SUCCEEDED( hr ) )
		{
          //Now the source filter is connected to the splitter lets load the rules from the xml
          break;
        }
	    else
		{
          CLog::Log(LOGERROR,"%s Failed to connect the source to the spliter",__FUNCTION__);
          return hr;
        }
	  }
	}
  }
  
  //Load the rules from the xml
  CInterfaceList<IUnknown, &IID_IUnknown> pUnk;
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(m_xbmcConfigFilePath))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;

  TiXmlElement *pRules = graphConfigRoot->FirstChildElement("rules");
  pRules = pRules->FirstChildElement("rule");
  while (pRules)
  {
    if (((CStdString)pRules->Attribute("filetypes")).Equals(pFileItem.GetAsUrl().GetFileType().c_str(),false))
	{
	  POSITION pos = m_configfilter.GetHeadPosition();
      while(pos)
      {
        CFGFilterFile* pFGF = m_configfilter.GetNext(pos);
		if ( ((CStdString)pRules->Attribute("videodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
		{
		  CComPtr<IBaseFilter> ppBF;
		  if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
		  {
            //TODO Fix the name of the filters to make it look cleaner in the graph
            this->AddFilter(ppBF,L"video decoder");
		  }
		  ppBF.Release();
		}
		else if ( ((CStdString)pRules->Attribute("audiodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
		{
		  CComPtr<IBaseFilter> ppBF;
		  if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
		  {
            //TODO Fix the name of the filters to make it look cleaner in the graph
		    this->AddFilter(ppBF,L"audio decoder");
		  }
		  ppBF.Release();
		}
	  }
	}
    pRules = pRules->NextSiblingElement();
  }
  this->ConnectFilter(ppSBF,NULL);
  //Getting the mpc video dec filter.
  //This filter interface is required to get the info for the current dxva rendering status
  CLSID mpcvid;
  BeginEnumFilters(this, pEF, pBF)
  {
    if (SUCCEEDED(pBF->GetClassID(&mpcvid)))
  {
    if (mpcvid == m_mpcVideoDecGuid)
    {
      m_XbmcVideoDec = pBF;
      break;
    }
  
  }
  }
  EndEnumFilters
  
return hr;
  
}
bool CFGManager::LoadFiltersFromXml(CStdString configFile, CStdString xbmcPath)
{
  m_xbmcConfigFilePath = configFile;
  xbmcPath.Replace("\\","\\\\");
  if (!CFile::Exists(configFile))
    return false;
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(configFile))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;
  CStdString strFPath;
  CStdString strFGuid;
  CStdString strFileType;
  CStdString strTrimedPath;
  CStdString strTmpFilterName;
  CStdString strTmpFilterType;
  CFGFilterFile* pFGF;
  TiXmlElement *pFilters = graphConfigRoot->FirstChildElement("filters");
  pFilters = pFilters->FirstChildElement("filter");
  while (pFilters)
  {
    strTmpFilterName = pFilters->Attribute("name");
    strTmpFilterType = pFilters->Attribute("type");
    XMLUtils::GetString(pFilters,"path",strFPath);
    if (!CFile::Exists(strFPath))
      strFPath.Format("%s\\\\system\\\\players\\\\dsplayer\\\\%s",xbmcPath.c_str(),strFPath.c_str());
    XMLUtils::GetString(pFilters,"guid",strFGuid);
    XMLUtils::GetString(pFilters,"filetype",strFileType);
    pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,L"",MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
    if (strTmpFilterName.Equals("mpcvideodec",false))
      m_mpcVideoDecGuid = DShowUtil::GUIDFromCString(strFGuid);

  if (pFGF)
  {
    if (strTmpFilterType.Equals("source",false))
        m_source.AddTail(pFGF);
    else if (strTmpFilterType.Equals("splitter",false))
        m_splitter.AddTail(pFGF);
    else
      m_configfilter.AddTail(pFGF);
  }
  pFilters = pFilters->NextSiblingElement();
  
  }
}

bool CFGManager::ConnectFiltersXbmc()
{
  CAtlList<IPin*> pinlist;
  if (m_Splitter)
  {
    if (SUCCEEDED(ConnectFilter(m_Splitter,NULL)))
      return true;
    IEnumPins *epins;
  IPin *pin;
  ULONG    f;
    if (FAILED(m_Splitter->EnumPins(&epins)))
      return false;
  epins->Reset();
  while (epins->Next(1, &pin, &f) == NOERROR) 
  {
      try
    {
        PIN_DIRECTION  dir;
        PIN_INFO    info;
        pin->QueryDirection(&dir);
      pin->QueryPinInfo(&info);
        if (dir == PINDIR_OUTPUT)
    {
          this->Render(pin);
      }
    }
    catch (...)
    {
    }
      pin->Release();
  }
  }
}

STDMETHODIMP CFGManager::RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList)
{
  CAutoLock cAutoLock(this);

  m_streampath.RemoveAll();
  m_deadends.RemoveAll();

  HRESULT hr;

  CComPtr<IBaseFilter> pBF;
  hr = this->AddSourceFilter(lpcwstrFile,lpcwstrFile,&pBF);
  //hr = ::AddSourceFilter(this,lpcwstrFile,DS_AVISOURCE,&pBF);
  if (FAILED(hr))
    return hr;

  return ConnectFilter(pBF, NULL);
}

STDMETHODIMP CFGManager::AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
  CAutoLock cAutoLock(this);

  // TODO: use overrides

  CheckPointer(lpcwstrFileName, E_POINTER);
  CheckPointer(ppFilter, E_POINTER);

  HRESULT hr;

  CStdString fn = CStdString(lpcwstrFileName).TrimLeft();
  //CStdString protocol = fn.Left(fn.Find(':')+1).TrimRight(':').MakeLower();
  CStdString protocol = fn.Left(fn.Find(':')+1).TrimRight(':');
  //CStdString ext = CPathW(fn).GetExtension();
  CStdString ext = CPath(fn.c_str()).GetExtension();

  TCHAR buff[256], buff2[256];
  ULONG len, len2;

  CFGFilterList fl;

  HANDLE hFile = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

  // internal / protocol

  if( protocol.GetLength() > 1 && (!protocol.Equals("file",false)) )
  {
    POSITION pos = m_source.GetHeadPosition();
    while(pos)
    {
      CFGFilter* pFGF = m_source.GetNext(pos);
      if(pFGF->m_protocols.Find(CStdString(protocol)))
        fl.Insert(pFGF, 0, false, false);
    }
  }

  // internal / check bytes

  if(hFile != INVALID_HANDLE_VALUE)
  {
    POSITION pos = m_source.GetHeadPosition();
    while(pos)
    {
      CFGFilter* pFGF = m_source.GetNext(pos);

      POSITION pos2 = pFGF->m_chkbytes.GetHeadPosition();
      while(pos2)
      {
        if(CheckBytes(hFile, pFGF->m_chkbytes.GetNext(pos2)))
        {
          fl.Insert(pFGF, 1, false, false);
          break;
        }
      }
    }
  }

  // insernal / file extension

  if(!ext.IsEmpty())
  {
    POSITION pos = m_source.GetHeadPosition();
    while(pos)
    {
      CFGFilter* pFGF = m_source.GetNext(pos);
      if(pFGF->m_extensions.Find(CStdString(ext)))
        fl.Insert(pFGF, 2, false, false);
    }
  }

  // internal / the rest

  {
    POSITION pos = m_source.GetHeadPosition();
    while(pos)
    {
      CFGFilter* pFGF = m_source.GetNext(pos);
      if(pFGF->m_protocols.IsEmpty() && pFGF->m_chkbytes.IsEmpty() && pFGF->m_extensions.IsEmpty())
        fl.Insert(pFGF, 3, false, false);
    }
  }

  // protocol

  if(protocol.GetLength() > 1 && (!protocol.Equals("file",false)) )
  {
    CRegKey key;
    if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, protocol, KEY_READ))
    {
      CRegKey exts;
      if(ERROR_SUCCESS == exts.Open(key, _T("Extensions"), KEY_READ))
      {
        len = countof(buff);
        if(ERROR_SUCCESS == exts.QueryStringValue(CString(ext), buff, &len))
          fl.Insert(new CFGFilterRegistry( DShowUtil::GUIDFromCString(buff)), 4);
      }

      len = countof(buff);
      if(ERROR_SUCCESS == key.QueryStringValue(_T("Source Filter"), buff, &len))
        fl.Insert(new CFGFilterRegistry( DShowUtil::GUIDFromCString(buff)), 5);
    }

    fl.Insert(new CFGFilterRegistry(CLSID_URLReader), 6);
  }

  // check bytes

  if(hFile != INVALID_HANDLE_VALUE)
  {
    CRegKey key;
    if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("Media Type"), KEY_READ))
    {
      FILETIME ft;
      len = countof(buff);
      for(DWORD i = 0; ERROR_SUCCESS == key.EnumKey(i, buff, &len, &ft); i++, len = countof(buff))
      {
        GUID majortype;
        if(FAILED( DShowUtil::GUIDFromCString(buff, majortype)))
          continue;

        CRegKey majorkey;
        if(ERROR_SUCCESS == majorkey.Open(key, buff, KEY_READ))
        {
          len = countof(buff);
          for(DWORD j = 0; ERROR_SUCCESS == majorkey.EnumKey(j, buff, &len, &ft); j++, len = countof(buff))
          {
            GUID subtype;
            if(FAILED( DShowUtil::GUIDFromCString(buff, subtype)))
              continue;

            CRegKey subkey;
            if(ERROR_SUCCESS == subkey.Open(majorkey, buff, KEY_READ))
            {
              len = countof(buff);
              if(ERROR_SUCCESS != subkey.QueryStringValue(_T("Source Filter"), buff, &len))
                continue;

              GUID clsid = DShowUtil::GUIDFromCString(buff);

              len = countof(buff);
              len2 = sizeof(buff2);
              for(DWORD k = 0, type; 
                clsid != GUID_NULL && ERROR_SUCCESS == RegEnumValue(subkey, k, buff2, &len2, 0, &type, (BYTE*)buff, &len); 
                k++, len = countof(buff), len2 = sizeof(buff2))
              {
                if(CheckBytes(hFile, CStdString(buff)))
                {
                  CFGFilter* pFGF = new CFGFilterRegistry(clsid);
                  pFGF->AddType(majortype, subtype);
                  fl.Insert(pFGF, 7);
                  break;
                }
              }
            }
          }
        }
      }
    }
  }

  // file extension

  if(!ext.IsEmpty())
  {
    CRegKey key;
    if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions\\") + CString(ext), KEY_READ))
    {
      ULONG len = countof(buff);
      if(ERROR_SUCCESS == key.QueryStringValue(_T("Source Filter"), buff, &len))
      {
        GUID clsid = DShowUtil::GUIDFromCString(buff);
        GUID majortype = GUID_NULL;
        GUID subtype = GUID_NULL;

        len = countof(buff);
        if(ERROR_SUCCESS == key.QueryStringValue(_T("Media Type"), buff, &len))
          majortype = DShowUtil::GUIDFromCString(buff);

        len = countof(buff);
        if(ERROR_SUCCESS == key.QueryStringValue(_T("Subtype"), buff, &len))
          subtype = DShowUtil::GUIDFromCString(buff);

        CFGFilter* pFGF = new CFGFilterRegistry(clsid);
        pFGF->AddType(majortype, subtype);
        fl.Insert(pFGF, 8);
      }
    }
  }

  if(hFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(hFile);
  }

  CFGFilter* pFGF = new CFGFilterRegistry(CLSID_AsyncReader);
  pFGF->AddType(MEDIATYPE_Stream, MEDIASUBTYPE_None);
  fl.Insert(pFGF, 9);

  POSITION pos = fl.GetHeadPosition();
  while(pos)
  {
    if(SUCCEEDED(hr = AddSourceFilter(fl.GetNext(pos), lpcwstrFileName, lpcwstrFilterName, ppFilter)))
      return hr;
  }

  return hFile == INVALID_HANDLE_VALUE ? VFW_E_NOT_FOUND : VFW_E_CANNOT_LOAD_SOURCE_FILTER;
}

STDMETHODIMP CFGManager::SetLogFile(DWORD_PTR hFile)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->SetLogFile(hFile);
}

STDMETHODIMP CFGManager::Abort()
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Abort();
}

STDMETHODIMP CFGManager::ShouldOperationContinue()
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ShouldOperationContinue();
}

// IFilterGraph2

STDMETHODIMP CFGManager::AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->AddSourceFilterForMoniker(pMoniker, pCtx, lpcwstrFilterName, ppFilter);
}

STDMETHODIMP CFGManager::ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt)
{
  CAutoLock cAutoLock(this);

  return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ReconnectEx(ppin, pmt);
}

STDMETHODIMP CFGManager::RenderEx(IPin* pPinOut, DWORD dwFlags, DWORD* pvContext)
{
  CAutoLock cAutoLock(this);

  if(!pPinOut || dwFlags > AM_RENDEREX_RENDERTOEXISTINGRENDERERS || pvContext)
    return E_INVALIDARG;

  HRESULT hr;

  if(dwFlags & AM_RENDEREX_RENDERTOEXISTINGRENDERERS)
  {
    CInterfaceList<IBaseFilter> pBFs;

    BeginEnumFilters(this, pEF, pBF)
    {
      if(DShowUtil::IsStreamEnd(pBF))
      {
        pBFs.AddTail(pBF);
      }
    }
    EndEnumFilters

    POSITION pos = pBFs.GetHeadPosition();
    while(pos)
    {
      if(SUCCEEDED(hr = ConnectFilter(pPinOut, pBFs.GetNext(pos))))
        return hr;
    }

    return VFW_E_CANNOT_RENDER;
  }

  return Connect(pPinOut, (IPin*)NULL);
}

// IGraphBuilder2

STDMETHODIMP CFGManager::IsPinDirection(IPin* pPin, PIN_DIRECTION dir1)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPin, E_POINTER);

  PIN_DIRECTION dir2;
  if(FAILED(pPin->QueryDirection(&dir2)))
    return E_FAIL;

  return dir1 == dir2 ? S_OK : S_FALSE;
}

STDMETHODIMP CFGManager::IsPinConnected(IPin* pPin)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPin, E_POINTER);

  CComPtr<IPin> pPinTo;
  return SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo ? S_OK : S_FALSE;
}

STDMETHODIMP CFGManager::ConnectFilter(IBaseFilter* pBF, IPin* pPinIn)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pBF, E_POINTER);

  if(pPinIn && S_OK != IsPinDirection(pPinIn, PINDIR_INPUT))
    return VFW_E_INVALID_DIRECTION;

  int nTotal = 0, nRendered = 0;

  BeginEnumPins(pBF, pEP, pPin)
  {
    if(DShowUtil::GetPinName(pPin)[0] != '~'
    && S_OK == IsPinDirection(pPin, PINDIR_OUTPUT)
    && S_OK != IsPinConnected(pPin))
    {
      m_streampath.Append(pBF, pPin);

      HRESULT hr = Connect(pPin, pPinIn);

      if(SUCCEEDED(hr))
      {
        for(int i = m_deadends.GetCount()-1; i >= 0; i--)
          if(m_deadends[i]->Compare(m_streampath))
            m_deadends.RemoveAt(i);

        nRendered++;
      }

      nTotal++;

      m_streampath.RemoveTail();

      if(SUCCEEDED(hr) && pPinIn) 
        return S_OK;
    }
  }
  EndEnumPins

  return 
    nRendered == nTotal ? (nRendered > 0 ? S_OK : S_FALSE) :
    nRendered > 0 ? VFW_S_PARTIAL_RENDER :
    VFW_E_CANNOT_RENDER;
}

HRESULT CFGManager::ConnectFilter(IPin* pPinOut, IBaseFilter* pBF)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPinOut, E_POINTER);
  CheckPointer(pBF, E_POINTER);

  HRESULT hr;

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir;
    if(DShowUtil::GetPinName(pPin)[0] != '~'
    && SUCCEEDED(hr = pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
    && SUCCEEDED(hr = Connect(pPinOut, pPin)))
      return hr;
  }
  EndEnumPins

  return VFW_E_CANNOT_CONNECT;
}

HRESULT CFGManager::ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt)
{
  CAutoLock cAutoLock(this);

  CheckPointer(pPinOut, E_POINTER);
  CheckPointer(pBF, E_POINTER);

  HRESULT hr;

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir;
    if(DShowUtil::GetPinName(pPin)[0] != '~'
    && SUCCEEDED(hr = pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
    && SUCCEEDED(hr = ConnectDirect(pPinOut, pPin, pmt)))
      return hr;
  }
  EndEnumPins

  return VFW_E_CANNOT_CONNECT;
}

STDMETHODIMP CFGManager::NukeDownstream(IUnknown* pUnk)
{
  CAutoLock cAutoLock(this);

  if(CComQIPtr<IBaseFilter> pBF = pUnk)
  {
    BeginEnumPins(pBF, pEP, pPin)
    {
      NukeDownstream(pPin);
    }
    EndEnumPins
  }
  else if(CComQIPtr<IPin> pPin = pUnk)
  {
    CComPtr<IPin> pPinTo;
    if(S_OK == IsPinDirection(pPin, PINDIR_OUTPUT)
    && SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo)
    {
      if(CComPtr<IBaseFilter> pBF = DShowUtil::GetFilterFromPin(pPinTo))
      {
        NukeDownstream(pBF);
        Disconnect(pPinTo);
        Disconnect(pPin);
        RemoveFilter(pBF);
      }
    }
  }
  else
  {
    return E_INVALIDARG;
  }

  return S_OK;
}


STDMETHODIMP CFGManager::FindInterface(REFIID iid, void** ppv, BOOL bRemove)
{
  CAutoLock cAutoLock(this);

  CheckPointer(ppv, E_POINTER);

  for(POSITION pos = m_pUnks.GetHeadPosition(); pos; m_pUnks.GetNext(pos))
  {
    if(SUCCEEDED(m_pUnks.GetAt(pos)->QueryInterface(iid, ppv)))
    {
      if(bRemove) m_pUnks.RemoveAt(pos);
      return S_OK;
    }
  }

  return E_NOINTERFACE;
}

STDMETHODIMP CFGManager::AddToROT()
{
  CAutoLock cAutoLock(this);

    HRESULT hr;

  if(m_dwRegister) return S_FALSE;

    CComPtr<IRunningObjectTable> pROT;
  CComPtr<IMoniker> pMoniker;
  WCHAR wsz[256];
    swprintf(wsz, L"FilterGraph %08p pid %08x (XBMC)", (DWORD_PTR)this, GetCurrentProcessId());
    if(SUCCEEDED(hr = GetRunningObjectTable(0, &pROT))
  && SUCCEEDED(hr = CreateItemMoniker(L"!", wsz, &pMoniker)))
        hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, (IGraphBuilder2*)this, pMoniker, &m_dwRegister);

  return hr;
}

STDMETHODIMP CFGManager::RemoveFromROT()
{
  CAutoLock cAutoLock(this);

  HRESULT hr;

  if(!m_dwRegister) return S_FALSE;

  CComPtr<IRunningObjectTable> pROT;
    if(SUCCEEDED(hr = GetRunningObjectTable(0, &pROT))
  && SUCCEEDED(hr = pROT->Revoke(m_dwRegister)))
    m_dwRegister = 0;

  return hr;
}

// IGraphBuilderDeadEnd

STDMETHODIMP_(size_t) CFGManager::GetCount()
{
  return m_deadends.GetCount();
}

STDMETHODIMP CFGManager::GetDeadEnd(int iIndex, CAtlList<CStdStringW>& path, CAtlList<CMediaType>& mts)
{
  CAutoLock cAutoLock(this);

  if(iIndex < 0 || iIndex >= m_deadends.GetCount()) return E_FAIL;

  path.RemoveAll();
  mts.RemoveAll();

  POSITION pos = m_deadends[iIndex]->GetHeadPosition();
  while(pos)
  {
    const path_t& p = m_deadends[iIndex]->GetNext(pos);

    CStdStringW str;
    str.Format(L"%s::%s", p.filter, p.pin);
    path.AddTail(str);
  }

  mts.AddTailList(&m_deadends[iIndex]->mts);

  return S_OK;
}

//
//   CFGManagerCustom
//

CFGManagerCustom::CFGManagerCustom(LPCTSTR pName, LPUNKNOWN pUnk,CStdString pXbmcPath)
  : CFGManager(pName, pUnk)
    ,m_pXbmcPath(pXbmcPath)
  , m_vrmerit(MERIT64(MERIT_PREFERRED))
  , m_armerit(MERIT64(MERIT_PREFERRED))

{
  CFGFilter* pFGF;
  WORD merit_low = 1;
  ULONGLONG merit;
  merit = MERIT64_ABOVE_DSHOW + merit_low++;
// Source filters

//WMAsfReader
  pFGF = new CFGFilterRegistry(CLSID_WMAsfReader);
  pFGF->m_chkbytes.AddTail(_T("0,4,,3026B275"));
  pFGF->m_chkbytes.AddTail(_T("0,4,,D129E2D6"));    
  m_source.AddTail(pFGF);

  //Skipping this part of CFGManagerCustom Since the use of dsfilterconfig.xml
  return;
// Blocked filters

// "Subtitle Mixer" makes an access violation around the 
// 11-12th media type when enumerating them on its output.
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{00A95963-3BE5-48C0-AD9F-3356D67EA09D}")), MERIT64_DO_NOT_USE));

// DiracSplitter.ax is crashing MPC-HC when opening invalid files...
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{09E7F58E-71A1-419D-B0A0-E524AE1454A9}")), MERIT64_DO_NOT_USE));
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{5899CFB9-948F-4869-A999-5544ECB38BA5}")), MERIT64_DO_NOT_USE));
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{F78CF248-180E-4713-B107-B13F7B5C31E1}")), MERIT64_DO_NOT_USE));

// ISCR suxx
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}")), MERIT64_DO_NOT_USE));

// Samsung's "mpeg-4 demultiplexor" can even open matroska files, amazing...
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{99EC0C72-4D1B-411B-AB1F-D561EE049D94}")), MERIT64_DO_NOT_USE));

// LG Video Renderer (lgvid.ax) just crashes when trying to connect it
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9F711C60-0668-11D0-94D4-0000C02BA972}")), MERIT64_DO_NOT_USE));

// palm demuxer crashes (even crashes graphedit when dropping an .ac3 onto it)
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{BE2CF8A7-08CE-4A2C-9A25-FD726A999196}")), MERIT64_DO_NOT_USE));

// mainconcept color space converter
  m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{272D77A0-A852-4851-ADA4-9091FEAD4C86}")), MERIT64_DO_NOT_USE));
//TODO
//Block if vmr9 is used
  if(1)
    m_transform.AddTail(DNew CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9852A670-F845-491B-9BE6-EBD841B8A613}")), MERIT64_DO_NOT_USE));
  
}

STDMETHODIMP CFGManagerCustom::AddFilter(IBaseFilter* pBF, LPCWSTR pName)
{
  CAutoLock cAutoLock(this);

  HRESULT hr;

  if(FAILED(hr = __super::AddFilter(pBF, pName)))
    return hr;

  //AppSettings& s = AfxGetAppSettings();

  if(DShowUtil::GetCLSID(pBF) == CLSID_DMOWrapperFilter)
  {
    if(CComQIPtr<IPropertyBag> pPB = pBF)
    {
      CComVariant var(true);
      pPB->Write(CComBSTR(L"_HIRESOUTPUT"), &var);
    }
  }

  /*if(CComQIPtr<IMpeg2DecFilter> m_pM2DF = pBF)
  {
    m_pM2DF->SetDeinterlaceMethod((ditype)s.mpegdi);
    m_pM2DF->SetBrightness(s.mpegbright);
    m_pM2DF->SetContrast(s.mpegcont);
    m_pM2DF->SetHue(s.mpeghue);
    m_pM2DF->SetSaturation(s.mpegsat);
    m_pM2DF->EnableForcedSubtitles(s.mpegforcedsubs);
    m_pM2DF->EnablePlanarYUV(s.mpegplanaryuv);
  }

  if(CComQIPtr<IMpeg2DecFilter2> m_pM2DF2 = pBF)
  {
    m_pM2DF2->EnableInterlaced(s.mpeginterlaced);
  }

  if(CComQIPtr<IMpaDecFilter> m_pMDF = pBF)
  {
    m_pMDF->SetSampleFormat((SampleFormat)s.mpasf);
    m_pMDF->SetNormalize(s.mpanormalize);
    m_pMDF->SetSpeakerConfig(IMpaDecFilter::ac3, s.ac3sc);
    m_pMDF->SetDynamicRangeControl(IMpaDecFilter::ac3, s.ac3drc);
    m_pMDF->SetSpeakerConfig(IMpaDecFilter::dts, s.dtssc);
    m_pMDF->SetDynamicRangeControl(IMpaDecFilter::dts, s.dtsdrc);
    m_pMDF->SetSpeakerConfig(IMpaDecFilter::aac, s.aacsc);
    m_pMDF->SetBoost(s.mpaboost);
  }*/

  return hr;
}

//
//   CFGManagerPlayer
//

CFGManagerPlayer::CFGManagerPlayer(LPCTSTR pName, LPUNKNOWN pUnk, HWND hWnd,CStdString pXbmcPath)
  : CFGManagerCustom(pName, pUnk,pXbmcPath)
  , m_hWnd(hWnd)
  , m_vrmerit(MERIT64(MERIT_PREFERRED))
  , m_armerit(MERIT64(MERIT_PREFERRED))
{
  CFGFilter* pFGF;

  //AppSettings& s = AfxGetAppSettings();

  if(m_pFM)
  {
    CComPtr<IEnumMoniker> pEM;

    GUID guids[] = {MEDIATYPE_Video, MEDIASUBTYPE_NULL};

    if(SUCCEEDED(m_pFM->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
      TRUE, 1, guids, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL)))
    {
      for(CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
      {
        CFGFilterRegistry f(pMoniker);
        m_vrmerit = max(m_vrmerit, f.GetMerit());
      }
    }

    m_vrmerit += 0x100;
  }

  if(m_pFM)
  {
    CComPtr<IEnumMoniker> pEM;

    GUID guids[] = {MEDIATYPE_Audio, MEDIASUBTYPE_NULL};

    if(SUCCEEDED(m_pFM->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
      TRUE, 1, guids, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL)))
    {
      for(CComPtr<IMoniker> pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
      {
        CFGFilterRegistry f(pMoniker);
        m_armerit = max(m_armerit, f.GetMerit());
      }
    }

    BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
    {
      CFGFilterRegistry f(pMoniker);
      m_armerit = max(m_armerit, f.GetMerit());
    }
    EndEnumSysDev

    m_armerit += 0x100;
  }

  CStdString fileconfigtmp;
  fileconfigtmp.Format("%s\\system\\players\\dsplayer\\dsfilterconfig.xml",pXbmcPath.c_str());
  //Load the config for the xml
  bool testtest;
  if (LoadFiltersFromXml(fileconfigtmp,pXbmcPath))
    CLog::Log(LOGNOTICE,"Successfully loaded %s",fileconfigtmp.c_str());
  else
    CLog::Log(LOGERROR,"Failed loading %s",fileconfigtmp.c_str());

  // Renderers
  m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, CLSID_VMR9AllocatorPresenter, L"Xbmc VMR9 (Renderless)", m_vrmerit));
/*
  {
    pFGF = new CFGFilterCustom<CNullVideoRenderer>(L"Null Video Renderer (Any)", MERIT64_ABOVE_DSHOW+2);
    pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
    m_transform.AddTail(pFGF);
  }
  else if(s.iDSVideoRendererType == VIDRNDT_DS_NULL_UNCOMP)
  {
    pFGF = new CFGFilterCustom<CNullUVideoRenderer>(L"Null Video Renderer (Uncompressed)", MERIT64_ABOVE_DSHOW+2);
    pFGF->AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
    m_transform.AddTail(pFGF);
  }

  if(s.AudioRendererDisplayName == AUDRNDT_NULL_COMP)
  {
    pFGF = new CFGFilterCustom<CNullAudioRenderer>(AUDRNDT_NULL_COMP, MERIT64_ABOVE_DSHOW+2);
    pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
    m_transform.AddTail(pFGF);
  }
  else if(s.AudioRendererDisplayName == AUDRNDT_NULL_UNCOMP)
  {
    pFGF = new CFGFilterCustom<CNullUAudioRenderer>(AUDRNDT_NULL_UNCOMP, MERIT64_ABOVE_DSHOW+2);
    pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
    m_transform.AddTail(pFGF);
  }
  else if(!s.AudioRendererDisplayName.IsEmpty())
  {
    pFGF = new CFGFilterRegistry(s.AudioRendererDisplayName, m_armerit);
    pFGF->AddType(MEDIATYPE_Audio, MEDIASUBTYPE_NULL);
    m_transform.AddTail(pFGF);
  }*/
}

HRESULT CFGManagerPlayer::CreateFilter(CFGFilter* pFGF, IBaseFilter** ppBF, IUnknown** ppUnk)
{
  HRESULT hr;

  if(FAILED(hr = __super::CreateFilter(pFGF, ppBF, ppUnk)))
    return hr;

  if(ppBF && *ppBF && ppUnk && !*ppUnk)
  {
    //AppSettings& s = AfxGetAppSettings();

    /*if(CComQIPtr<IAudioSwitcherFilter> pASF = *ppBF)
    {
      pASF->SetSpeakerConfig(s.fCustomChannelMapping, s.pSpeakerToChannelMap);
      pASF->EnableDownSamplingTo441(s.fDownSampleTo441);
      pASF->SetAudioTimeShift(s.fAudioTimeShift ? 10000i64*s.tAudioTimeShift : 0);
      *ppUnk = pASF.Detach();
    }*/
  }

  return hr;
}

STDMETHODIMP CFGManagerPlayer::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
  CAutoLock cAutoLock(this);

  if(DShowUtil::GetCLSID(pPinOut) == CLSID_MPEG2Demultiplexer)
  {
    CComQIPtr<IMediaSeeking> pMS = pPinOut;
    REFERENCE_TIME rtDur = 0;
    if(!pMS || FAILED(pMS->GetDuration(&rtDur)) || rtDur <= 0)
       return E_FAIL;
  }

  return __super::ConnectDirect(pPinOut, pPinIn, pmt);
}

