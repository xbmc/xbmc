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

#include "WinSystemWin32.h" //g_hwnd
#include "WindowingFactory.h"
#include "CharsetConverter.h"
#include "DShowUtil/dshowutil.h"
#include "DShowUtil/DshowCommon.h"
#include "SystemInfo.h" //g_sysinfo
#include "GUISettings.h"//g_guiSettings


#include "filters/VMR9AllocatorPresenter.h"
#include "filters/evrAllocatorPresenter.h"
#include <initguid.h>
#include "moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>
#include <Vmr9.h>
#include "Filters/IMpaDecFilter.h"
#include "Log.h"
#include "FileSystem/SpecialProtocol.h"
//XML CONFIG HEADERS
#include "tinyXML/tinyxml.h"
#include "XMLUtils.h"
//END XML CONFIG HEADERS

using namespace std;

//
// CFGManager
//

CFGManager::CFGManager(LPCTSTR pName, LPUNKNOWN pUnk)
  : CUnknown(pName, pUnk)
  , m_dwRegister(0)
  , m_pUnkInner(NULL)
  , m_pFM(NULL)
{
  CoCreateInstance(CLSID_FilterGraph,GetOwner(),CLSCTX_ALL,__uuidof(IUnknown),(void**) &m_pUnkInner);
  CoCreateInstance(CLSID_FilterMapper2,NULL,CLSCTX_ALL,__uuidof(m_pFM),(void**) &m_pFM);
  //m_pFM.CoCreateInstance(CLSID_FilterMapper2);
}

CFGManager::~CFGManager()
{
  CAutoLock cAutoLock(this);
  while(!m_source.empty()) 
    m_source.pop_back();
  while(!m_transform.empty()) 
    m_transform.pop_back();
  while(!m_override.empty()) 
    m_override.pop_back();

  
  SAFE_RELEASE(m_pUnkInner);
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
  push_back(p);
}

bool CFGManager::CStreamPath::Compare(const CStreamPath& path)
{
  PathListIter pos1 = begin();
  PathListConstIter pos2 = path.begin();
  while((pos1 != end()) && (pos2 != path.end()))
  {
    const path_t& p1 = *pos1;
    const path_t& p2 = *pos2;
    if(p1.filter != p2.filter) return true;
    else if(p1.pin != p2.pin) return false;
    pos1++;
    pos2++;
  }

  return true;
}

HRESULT CFGManager::CreateFilter(CFGFilter* pFGF, IBaseFilter** ppBF)
{
  CheckPointer(pFGF, E_POINTER);
  CheckPointer(ppBF, E_POINTER);

  IBaseFilter* pBF;
  if(FAILED(pFGF->Create(&pBF)))
    return E_FAIL;

  *ppBF = pBF;
  pBF = NULL;

  return S_OK;
}

// IFilterGraph

STDMETHODIMP CFGManager::AddFilter(IBaseFilter* pFilter, LPCWSTR pName)
{
  CAutoLock cAutoLock(this);

  HRESULT hr;
  IFilterGraph2* pIFG;
  hr = m_pUnkInner->QueryInterface(__uuidof(pIFG),(void**)&pIFG);
  if(FAILED(hr))
    return hr;
  hr = pIFG->AddFilter(pFilter, pName);
  if(FAILED(hr))
    return hr;

  // TODO
  hr = pFilter->JoinFilterGraph(NULL, NULL);
  hr = pFilter->JoinFilterGraph(this, pName);
  
  SAFE_RELEASE(pIFG);
  return hr;
}

STDMETHODIMP CFGManager::RemoveFilter(IBaseFilter* pFilter)
{
  if(!m_pUnkInner) 
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);
  IFilterGraph2* pIFG;
  if (SUCCEEDED(m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG)))
    return pIFG->RemoveFilter(pFilter);
  return E_FAIL;
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->RemoveFilter(pFilter);
}

STDMETHODIMP CFGManager::EnumFilters(IEnumFilters** ppEnum)
{
  if(!m_pUnkInner) 
    return E_UNEXPECTED;
  //Locking make crash reclock
  //solution comming from mpc-hc
  //CAutoLock cAutoLock(this);
  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->EnumFilters(ppEnum);
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->EnumFilters(ppEnum);
}

STDMETHODIMP CFGManager::FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter)
{
  if(!m_pUnkInner) 
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);
  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->FindFilterByName(pName, ppFilter);
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->FindFilterByName(pName, ppFilter);
}

STDMETHODIMP CFGManager::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
  if(!m_pUnkInner) 
    return E_UNEXPECTED;

	CAutoLock cAutoLock(this);

  IBaseFilter* pBF = DShowUtil::GetFilterFromPin(pPinIn);
	CLSID clsid = DShowUtil::GetCLSID(pBF);

	// TODO: GetUpStreamFilter goes up on the first input pin only
	for(IBaseFilter* pBFUS = DShowUtil::GetFilterFromPin(pPinOut); pBFUS; pBFUS = DShowUtil::GetUpStreamFilter(pBFUS))
	{
		if(pBFUS == pBF) 
      return VFW_E_CIRCULAR_GRAPH;
    if(DShowUtil::GetCLSID(pBFUS) == clsid) 
      return VFW_E_CANNOT_CONNECT;
	}
  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->ConnectDirect(pPinOut, pPinIn, pmt);
	//return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ConnectDirect(pPinOut, pPinIn, pmt);
}

STDMETHODIMP CFGManager::Reconnect(IPin* ppin)
{
  if(!m_pUnkInner) 
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);

  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->Reconnect(ppin);
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Reconnect(ppin);
}

STDMETHODIMP CFGManager::Disconnect(IPin* ppin)
{
  if(!m_pUnkInner) 
    return E_UNEXPECTED;
  CAutoLock cAutoLock(this);

  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->Disconnect(ppin);
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Disconnect(ppin);
}

STDMETHODIMP CFGManager::SetDefaultSyncSource()
{
  CAutoLock cAutoLock(this);

  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->SetDefaultSyncSource();
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->SetDefaultSyncSource();
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

    IPin* pPinTo;
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
    IStreamBuilder* pSB;
    if (SUCCEEDED(pPinOut->QueryInterface(__uuidof(pSB),(void**)&pSB)))
    {
      if(SUCCEEDED(hr = pSB->Render(pPinOut, this)))
        return hr;

      pSB->Backout(pPinOut, this);
    }
  }

  // 2. Try cached filters
  IGraphConfig* pGC;
  if (SUCCEEDED((IGraphBuilder2*)this->QueryInterface(__uuidof(pGC),(void**)&pGC)))
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
    std::list<IBaseFilter*> pBFs;
    BeginEnumFilters(this, pEF, pBF)
    {
      if(pPinIn && (DShowUtil::IsStreamEnd(pBF) || DShowUtil::GetFilterFromPin(pPinIn) == pBF)
      || DShowUtil::GetFilterFromPin(pPinOut) == pBF)
        continue;

      // HACK: ffdshow - audio capture filter
      if(DShowUtil::GetCLSID(pPinOut) == DShowUtil::GUIDFromCString(_T("{04FE9017-F873-410E-871E-AB91661A4EF7}"))
      && DShowUtil::GetCLSID(pBF) == DShowUtil::GUIDFromCString(_T("{E30629D2-27E5-11CE-875D-00608CB78066}")))
        continue;
      pBFs.push_back(pBF);
    }
    EndEnumFilters

    IBaseFilter* pBF;
    for (list<IBaseFilter*>::iterator it = pBFs.begin() ; it != pBFs.end(); it++)
    {
      pBF = *it;// ((IBaseFilter*)it);
      if(SUCCEEDED(hr = ConnectFilterDirect(pPinOut, pBF, NULL)))
      {
        if(! DShowUtil::IsStreamEnd(pBF)) 
          fDeadEnd = false;

        if(SUCCEEDED(hr = ConnectFilter(pBF, pPinIn)))
          return hr;
      }

      EXECUTE_ASSERT(Disconnect(pPinOut));
    }
  }

  // 4. Look up filters in the registry
  
  {
    CFGFilterList* fl = new CFGFilterList();

    std::vector<GUID> types;
    DShowUtil::ExtractMediaTypes(pPinOut, types);

    
    CFGFilter* pFGF;
    for(FilterListIter it = m_transform.begin() ; it != m_transform.end(); it++ )
    {
      pFGF = *it;
      if(pFGF->GetMerit() < MERIT64_DO_USE || pFGF->CheckTypes(types, false)) 
        fl->Insert(pFGF, 0, pFGF->CheckTypes(types, true), false);
    }

    
    for(FilterListIter it = m_override.begin() ; it != m_override.end() ; it++)
    {
      pFGF = *it;
      if(pFGF->GetMerit() < MERIT64_DO_USE || pFGF->CheckTypes(types, false)) 
        fl->Insert(pFGF, 0, pFGF->CheckTypes(types, true), false);
    }

    IEnumMoniker* pEM;
    //Not sure its the best way to do it but at least its not throwing unhandled exception
    std::vector<GUID>::iterator it = types.begin();
    if((types.size() > 0 )
    && SUCCEEDED(m_pFM->EnumMatchingFilters(
      &pEM, 0, false, MERIT_DO_NOT_USE+1,
      true, types.size()/2, &(*it), NULL, NULL, false, 
      !!pPinIn, 0, NULL, NULL, NULL)))
    {
      for(IMoniker* pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
      {
        CFGFilterRegistry* pFGF = new CFGFilterRegistry(pMoniker);
        fl->Insert(pFGF, 0, pFGF->CheckTypes(types, true));
      }
    }
//crashing right there on mkv
//The subtitle might be the reason
    std::list<CFGFilter*> theList;
    theList = fl->GetSortedList();
    for (list<CFGFilter*>::iterator it = theList.begin() ; it != theList.end() ; it++)
    {
      //CFGFilter* pFGF = fl.GetNext(pos);

      IBaseFilter* pBF;
      if(FAILED(CreateFilter(*it, &pBF)))
        continue;

      if(pPinIn &&  DShowUtil::IsStreamEnd(pBF))
        continue;
      if(FAILED(hr = AddFilter(pBF, (LPCWSTR)(*it)->GetName().c_str())))
        continue;

      hr = E_FAIL;

      if(types.size() == 2 && types[0] == MEDIATYPE_Stream && types[1] != MEDIATYPE_NULL)
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
        if(! DShowUtil::IsStreamEnd(pBF)) 
          fDeadEnd = false;

        hr = ConnectFilter(pBF, pPinIn);

        if(SUCCEEDED(hr))
        {
          IMixerPinConfig* pMPC;
          if (SUCCEEDED(pBF->QueryInterface(IID_IMixerPinConfig,(void **)&pMPC)))
          {  
            pMPC->SetAspectRatioMode(AM_ARMODE_STRETCHED);
            SAFE_RELEASE(pMPC);
          }
          IVMRAspectRatioControl* pARC;
          if (SUCCEEDED(pBF->QueryInterface(__uuidof(IVMRAspectRatioControl),(void **)&pARC)))
          {  
            pARC->SetAspectRatioMode(VMR_ARMODE_NONE);
            SAFE_RELEASE(pARC);
          }

          IVMRAspectRatioControl9* pARC9;
          if (SUCCEEDED(pBF->QueryInterface(__uuidof(IVMRAspectRatioControl9),(void **)&pARC9)))
          {  
            pARC9->SetAspectRatioMode(VMR_ARMODE_NONE);
            SAFE_RELEASE(pARC9);
          }
          return hr;
        }
      }

      EXECUTE_ASSERT(SUCCEEDED(RemoveFilter(pBF)));
    }
  }
  
  
  /*if(fDeadEnd)
  {
    CStreamDeadEnd* psde;
    psde = new CStreamDeadEnd();
    //CAutoPtr<CStreamDeadEnd> psde(new CStreamDeadEnd());
    //The one under wasnt used already commented
    //psde->AddTailList(&m_streampath);

    path_t pth;
    for (PathListIter it = m_streampath.begin();it != m_streampath.end(); it++)
    {
      psde->push_back(*it);
    }
    BeginEnumMediaTypes(pPinOut, pEM, pmt)
      psde->mts.push_back(CMediaType(*pmt));
    EndEnumMediaTypes(pmt)
    m_deadends.push_back(psde);
  }*/

  return pPinIn ? VFW_E_CANNOT_CONNECT : VFW_E_CANNOT_RENDER;
}

STDMETHODIMP CFGManager::Render(IPin* pPinOut)
{return E_NOTIMPL;
  /*CAutoLock cAutoLock(this);

  return RenderEx(pPinOut, 0, NULL);*/
}

STDMETHODIMP CFGManager::RenderFileXbmc(const CFileItem& pFileItem)
{
  CAutoLock cAutoLock(this);
  HRESULT hr = S_OK;
  if (FAILED(m_CfgLoader->LoadFilterRules(pFileItem) ))
    return E_FAIL;
  DShowUtil::RemoveUnconnectedFilters(this);
  
  return hr;
  
}

STDMETHODIMP CFGManager::GetFileInfo(CStdString* sourceInfo,CStdString* splitterInfo,CStdString* audioInfo,CStdString* videoInfo,CStdString* audioRenderer)
{
  *sourceInfo = m_CfgLoader->GetSourceFilterInfo();
  *splitterInfo = m_CfgLoader->GetSplitterFilterInfo();
  *audioInfo = m_CfgLoader->GetAudioDecInfo();
  *videoInfo = m_CfgLoader->GetVideoDecInfo();
  *audioRenderer = m_CfgLoader->GetAudioRenderer();
  return S_OK;
}

STDMETHODIMP CFGManager::Abort()
{
  CAutoLock cAutoLock(this);
  
  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->Abort();
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->Abort();
}

STDMETHODIMP CFGManager::ShouldOperationContinue()
{
  CAutoLock cAutoLock(this);
  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->ShouldOperationContinue();
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ShouldOperationContinue();
}

// IFilterGraph2

STDMETHODIMP CFGManager::AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
  CAutoLock cAutoLock(this);
  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->AddSourceFilterForMoniker(pMoniker, pCtx, lpcwstrFilterName, ppFilter);
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->AddSourceFilterForMoniker(pMoniker, pCtx, lpcwstrFilterName, ppFilter);
}

STDMETHODIMP CFGManager::ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt)
{
  CAutoLock cAutoLock(this);
  IFilterGraph2* pIFG;
  m_pUnkInner->QueryInterface(__uuidof(IFilterGraph2),(void**)&pIFG);
  return pIFG->ReconnectEx(ppin, pmt);
  //return CComQIPtr<IFilterGraph2>(m_pUnkInner)->ReconnectEx(ppin, pmt);
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

  IPin* pPinTo;
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
        for (vector<CStreamDeadEnd>::iterator it = m_deadends.end() ; it != m_deadends.begin() ;it-- )
        {
          if((*it).Compare(m_streampath))
            m_deadends.erase(it);
        }
        //for(int i = m_deadends.size()-1; i >= 0; i--)
        //  if(m_deadends[i]->Compare(m_streampath))
        //    m_deadends.RemoveAt(i);

        nRendered++;
      }

      nTotal++;

      m_streampath.pop_back();

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

  IBaseFilter* pBF;
  IPin* pPin;
  if (SUCCEEDED(pUnk->QueryInterface(__uuidof(pBF),(void**)&pBF)))
  {
    BeginEnumPins(pBF, pEP, pPin)
    {
      NukeDownstream(pPin);
    }
    EndEnumPins
  }
  else if (SUCCEEDED(pUnk->QueryInterface(__uuidof(pPin),(void**)&pPin)))
  {
    IPin* pPinTo;
    if(S_OK == IsPinDirection(pPin, PINDIR_OUTPUT)
    && SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo)
    {
      if(IBaseFilter* pBF = DShowUtil::GetFilterFromPin(pPinTo))
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

STDMETHODIMP CFGManager::AddToROT()
{
  CAutoLock cAutoLock(this);

    HRESULT hr;

  if(m_dwRegister)
    return S_FALSE;
    IRunningObjectTable* pROT;
  IMoniker* pMoniker;
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

  IRunningObjectTable* pROT;
    if(SUCCEEDED(hr = GetRunningObjectTable(0, &pROT))
  && SUCCEEDED(hr = pROT->Revoke(m_dwRegister)))
    m_dwRegister = 0;

  return hr;
}

// IGraphBuilderDeadEnd

STDMETHODIMP_(size_t) CFGManager::GetCount()
{
  return m_deadends.size();
}

STDMETHODIMP CFGManager::GetDeadEnd(int iIndex, std::list<CStdStringW>& path, std::list<CMediaType>& mts)
{
  CAutoLock cAutoLock(this);

  if(iIndex < 0 || iIndex >= m_deadends.size()) 
    return E_FAIL;

  while (!path.empty())
    path.pop_back();
  while (!mts.empty())
    mts.pop_back();

  path_t p;
  
  for (list<path_t>::iterator it = m_deadends.at(iIndex).begin(); it != m_deadends.at(iIndex).end(); it++)
  {
    p = *it;
    CStdStringW str;
    str.Format(L"%s::%s", p.filter, p.pin);
    path.push_back(str);
  }

  for (list<CMediaType>::iterator it = m_deadends.at(iIndex).mts.begin(); it != m_deadends.at(iIndex).mts.begin(); it++)
    mts.push_back(*it);

  return S_OK;
}

//
//   CFGManagerCustom
//

CFGManagerCustom::CFGManagerCustom(LPCTSTR pName, LPUNKNOWN pUnk)
  : CFGManager(pName, pUnk)
  , m_vrmerit(MERIT64(MERIT_PREFERRED))
  , m_armerit(MERIT64(MERIT_PREFERRED))

{
  WORD merit_low = 1;
  ULONGLONG merit;
  merit = MERIT64_ABOVE_DSHOW + merit_low++;


// Blocked filters
  
// "Subtitle Mixer" makes an access violation around the 
// 11-12th media type when enumerating them on its output.
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{00A95963-3BE5-48C0-AD9F-3356D67EA09D}")), MERIT64_DO_NOT_USE));

// DiracSplitter.ax is crashing MPC-HC when opening invalid files...
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{09E7F58E-71A1-419D-B0A0-E524AE1454A9}")), MERIT64_DO_NOT_USE));
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{5899CFB9-948F-4869-A999-5544ECB38BA5}")), MERIT64_DO_NOT_USE));
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{F78CF248-180E-4713-B107-B13F7B5C31E1}")), MERIT64_DO_NOT_USE));

// ISCR suxx
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}")), MERIT64_DO_NOT_USE));

// Samsung's "mpeg-4 demultiplexor" can even open matroska files, amazing...
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{99EC0C72-4D1B-411B-AB1F-D561EE049D94}")), MERIT64_DO_NOT_USE));

// LG Video Renderer (lgvid.ax) just crashes when trying to connect it
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9F711C60-0668-11D0-94D4-0000C02BA972}")), MERIT64_DO_NOT_USE));

// palm demuxer crashes (even crashes graphedit when dropping an .ac3 onto it)
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{BE2CF8A7-08CE-4A2C-9A25-FD726A999196}")), MERIT64_DO_NOT_USE));

// mainconcept color space converter
  m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{272D77A0-A852-4851-ADA4-9091FEAD4C86}")), MERIT64_DO_NOT_USE));

//Block if vmr9 is used
  if(m_CurrentRenderer == DIRECTSHOW_RENDERER_VMR9 )
    m_transform.push_back(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9852A670-F845-491B-9BE6-EBD841B8A613}")), MERIT64_DO_NOT_USE));
  
}

STDMETHODIMP CFGManagerCustom::AddFilter(IBaseFilter* pBF, LPCWSTR pName)
{
  CAutoLock cAutoLock(this);

  HRESULT hr;

  if(FAILED(hr = __super::AddFilter(pBF, pName)))
    return hr;


  if(DShowUtil::GetCLSID(pBF) == CLSID_DMOWrapperFilter)
  {
    IPropertyBag* pPB;
    if (SUCCEEDED(pBF->QueryInterface(__uuidof(IPropertyBag),(void**) &pPB)))
    {
      tagVARIANT var;
      var.boolVal = true;
      pPB->Write(LPCOLESTR(L"_HIRESOUTPUT"), &var);
    }
    SAFE_RELEASE(pPB);
  }

  IMpaDecFilter* m_pMDF;
  if (SUCCEEDED(pBF->QueryInterface(__uuidof(IMpaDecFilter),(void**) &m_pMDF)))
  {
    //m_pMDF->SetSampleFormat((SampleFormat)s.mpasf);
    m_pMDF->SetNormalize(1);
    //m_pMDF->SetSpeakerConfig(IMpaDecFilter::ac3, s.ac3sc);
    //m_pMDF->SetDynamicRangeControl(IMpaDecFilter::ac3, s.ac3drc);
    //m_pMDF->SetSpeakerConfig(IMpaDecFilter::dts, s.dtssc);
    //m_pMDF->SetDynamicRangeControl(IMpaDecFilter::dts, s.dtsdrc);
    //m_pMDF->SetSpeakerConfig(IMpaDecFilter::aac, s.aacsc);
    m_pMDF->SetBoost(1);
  }
  SAFE_RELEASE(m_pMDF);
  return hr;
}

//
//   CFGManagerPlayer
//

CFGManagerPlayer::CFGManagerPlayer(LPCTSTR pName, LPUNKNOWN pUnk, HWND hWnd)
  : CFGManagerCustom(pName, pUnk)
  , m_hWnd(hWnd)
  , m_vrmerit(MERIT64(MERIT_PREFERRED))
  , m_armerit(MERIT64(MERIT_PREFERRED))
{
  if(m_pFM)
  {
    IEnumMoniker* pEM;

    GUID guids[] = {MEDIATYPE_Video, MEDIASUBTYPE_NULL};

    if(SUCCEEDED(m_pFM->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
      TRUE, 1, guids, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL)))
    {
      for(IMoniker* pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
      {
        CFGFilterRegistry f(pMoniker);
        m_vrmerit = max(m_vrmerit, f.GetMerit());
      }
    }

    m_vrmerit += 0x100;
  }

  if(m_pFM)
  {
    IEnumMoniker* pEM;

    GUID guids[] = {MEDIATYPE_Audio, MEDIASUBTYPE_NULL};

    if(SUCCEEDED(m_pFM->EnumMatchingFilters(&pEM, 0, FALSE, MERIT_DO_NOT_USE+1,
      TRUE, 1, guids, NULL, NULL, TRUE, FALSE, 0, NULL, NULL, NULL)))
    {
      for(IMoniker* pMoniker; S_OK == pEM->Next(1, &pMoniker, NULL); pMoniker = NULL)
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
  fileconfigtmp = _P("special://xbmc/system/players/dsplayer/dsfilterconfig.xml");
  //Load the config for the xml
  m_CfgLoader = new CFGLoader(this);

  if (SUCCEEDED(m_CfgLoader->LoadConfig(fileconfigtmp)))
    CLog::Log(LOGNOTICE,"Successfully loaded %s",fileconfigtmp.c_str());
  else
    CLog::Log(LOGERROR,"Failed loading %s",fileconfigtmp.c_str());
  if (g_sysinfo.IsVistaOrHigher())
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_CurrentRenderer = DIRECTSHOW_RENDERER_VMR9;
    else 
      m_CurrentRenderer = DIRECTSHOW_RENDERER_EVR;
  else
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_CurrentRenderer = DIRECTSHOW_RENDERER_EVR;
    else 
      m_CurrentRenderer = DIRECTSHOW_RENDERER_VMR9;
    

  // Renderers
  if (m_CurrentRenderer == DIRECTSHOW_RENDERER_EVR)
    m_transform.push_back(new CFGFilterVideoRenderer(__uuidof(CEVRAllocatorPresenter), L"Xbmc EVR", m_vrmerit));
  else
    m_transform.push_back(new CFGFilterVideoRenderer(__uuidof(CVMR9AllocatorPresenter), L"Xbmc VMR9 (Renderless)", m_vrmerit));
  
}

HRESULT CFGManagerPlayer::CreateFilter(CFGFilter* pFGF, IBaseFilter** ppBF)
{
  HRESULT hr;

  if(FAILED(hr = __super::CreateFilter(pFGF, ppBF)))
    return hr;
  return hr;
}

STDMETHODIMP CFGManagerPlayer::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
  CAutoLock cAutoLock(this);

  if(DShowUtil::GetCLSID(pPinOut) == CLSID_MPEG2Demultiplexer)
  {
    IMediaSeeking* pMS = NULL;
    pPinOut->QueryInterface(__uuidof(IMediaSeeking),(void**)&pMS);
    REFERENCE_TIME rtDur = 0;
    if(!pMS || FAILED(pMS->GetDuration(&rtDur)) || rtDur <= 0)
       return E_FAIL;
    SAFE_RELEASE(pMS);
  }

  return __super::ConnectDirect(pPinOut, pPinIn, pmt);
}
