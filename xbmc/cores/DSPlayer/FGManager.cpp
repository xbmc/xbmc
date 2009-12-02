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
  m_FileSource.Release();
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

HRESULT CFGManager::CreateFilter(CFGFilter* pFGF, IBaseFilter** ppBF, IUnknown** ppUnk)
{
  CheckPointer(pFGF, E_POINTER);
  CheckPointer(ppBF, E_POINTER);
  CheckPointer(ppUnk, E_POINTER);

  CComPtr<IBaseFilter> pBF;
  CInterfaceList<IUnknown, &IID_IUnknown> pUnk;
  if(FAILED(pFGF->Create(&pBF, pUnk)))
    return E_FAIL;

  *ppBF = pBF.Detach();
  m_pUnks.AddTailList(&pUnk);

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

STDMETHODIMP CFGManager::GetXbmcVideoDecFilter(IMPCVideoDecFilter** ppBF)
{
  CheckPointer(ppBF,E_POINTER);
  IMPCVideoDecFilter* mpcvfilter;
  *ppBF = NULL;
  //Getting the mpc video dec filter.
  //This filter interface is required to get the info for the current dxva rendering status
  CLSID mpcvid;
  BeginEnumFilters(this, pEF, pBF)
  {
    if (SUCCEEDED(pBF->GetClassID(&mpcvid)))
    {
    if (mpcvid == DShowUtil::GUIDFromCString("{008BAC12-FBAF-497B-9670-BC6F6FBAE2C4}"))
      {
        if ( SUCCEEDED(pBF->QueryInterface(__uuidof(IMPCVideoDecFilter),(void**) &mpcvfilter)))
          *ppBF = mpcvfilter;
      break;
      }
    }
  }
  EndEnumFilters
}

STDMETHODIMP CFGManager::GetFfdshowVideoDecFilter(IffdshowDecVideoA** ppBF)
{
  CheckPointer(ppBF,E_POINTER);
  IffdshowDecVideoA* mpcvfilter;
  *ppBF = NULL;
  //Getting the mpc video dec filter.
  //This filter interface is required to get the info for the current dxva rendering status
  CLSID mpcvid;
  BeginEnumFilters(this, pEF, pBF)
  {
    if (SUCCEEDED(pBF->GetClassID(&mpcvid)))
    {
    if (mpcvid == DShowUtil::GUIDFromCString("{04FE9017-F873-410E-871E-AB91661A4EF7}"))
      {
        if ( SUCCEEDED(pBF->QueryInterface(IID_IffdshowDecVideoA,(void**) &mpcvfilter)))
          *ppBF = mpcvfilter;
      break;
      }
    }
  }
  EndEnumFilters
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
  WORD merit_low = 1;
  ULONGLONG merit;
  merit = MERIT64_ABOVE_DSHOW + merit_low++;


// Blocked filters
  
// "Subtitle Mixer" makes an access violation around the 
// 11-12th media type when enumerating them on its output.
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{00A95963-3BE5-48C0-AD9F-3356D67EA09D}")), MERIT64_DO_NOT_USE));

// DiracSplitter.ax is crashing MPC-HC when opening invalid files...
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{09E7F58E-71A1-419D-B0A0-E524AE1454A9}")), MERIT64_DO_NOT_USE));
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{5899CFB9-948F-4869-A999-5544ECB38BA5}")), MERIT64_DO_NOT_USE));
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{F78CF248-180E-4713-B107-B13F7B5C31E1}")), MERIT64_DO_NOT_USE));

// ISCR suxx
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}")), MERIT64_DO_NOT_USE));

// Samsung's "mpeg-4 demultiplexor" can even open matroska files, amazing...
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{99EC0C72-4D1B-411B-AB1F-D561EE049D94}")), MERIT64_DO_NOT_USE));

// LG Video Renderer (lgvid.ax) just crashes when trying to connect it
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9F711C60-0668-11D0-94D4-0000C02BA972}")), MERIT64_DO_NOT_USE));

// palm demuxer crashes (even crashes graphedit when dropping an .ac3 onto it)
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{BE2CF8A7-08CE-4A2C-9A25-FD726A999196}")), MERIT64_DO_NOT_USE));

// mainconcept color space converter
  m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{272D77A0-A852-4851-ADA4-9091FEAD4C86}")), MERIT64_DO_NOT_USE));
//TODO
//Block if vmr9 is used
  if(!DShowUtil::IsVistaOrAbove())
    m_transform.AddTail(new CFGFilterRegistry(DShowUtil::GUIDFromCString(_T("{9852A670-F845-491B-9BE6-EBD841B8A613}")), MERIT64_DO_NOT_USE));
  
}

STDMETHODIMP CFGManagerCustom::AddFilter(IBaseFilter* pBF, LPCWSTR pName)
{
  CAutoLock cAutoLock(this);

  HRESULT hr;

  if(FAILED(hr = __super::AddFilter(pBF, pName)))
    return hr;


  if(DShowUtil::GetCLSID(pBF) == CLSID_DMOWrapperFilter)
  {
    if(CComQIPtr<IPropertyBag> pPB = pBF)
    {
      CComVariant var(true);
      pPB->Write(CComBSTR(L"_HIRESOUTPUT"), &var);
    }
  }
  if(CComQIPtr<IMpaDecFilter> m_pMDF = pBF)
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
  fileconfigtmp = _P("special://xbmc/system/players/dsplayer/dsfilterconfig.xml");
  //Load the config for the xml
  m_CfgLoader = new CFGLoader(this);
  //if (LoadFiltersFromXml(fileconfigtmp,pXbmcPath))
  if (SUCCEEDED(m_CfgLoader->LoadConfig(fileconfigtmp)))
    CLog::Log(LOGNOTICE,"Successfully loaded %s",fileconfigtmp.c_str());
  else
    CLog::Log(LOGERROR,"Failed loading %s",fileconfigtmp.c_str());

  // Renderers
  if (DShowUtil::IsVistaOrAbove())
    m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, __uuidof(CEVRAllocatorPresenter), L"Xbmc EVR", m_vrmerit));
  else
    m_transform.AddTail(new CFGFilterVideoRenderer(m_hWnd, __uuidof(CDX9AllocatorPresenter), L"Xbmc VMR9 (Renderless)", m_vrmerit));
  
}

HRESULT CFGManagerPlayer::CreateFilter(CFGFilter* pFGF, IBaseFilter** ppBF, IUnknown** ppUnk)
{
  HRESULT hr;

  if(FAILED(hr = __super::CreateFilter(pFGF, ppBF, ppUnk)))
    return hr;
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

