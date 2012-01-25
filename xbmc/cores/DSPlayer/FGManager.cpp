/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
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

#ifdef HAS_DS_PLAYER

#include "FGManager.h"

#include <mpconfig.h> //IMixerConfig

#include "windowing/windows/WinSystemWin32.h" //g_hwnd
#include "windowing/WindowingFactory.h"
#include "utils/CharsetConverter.h"
#include "DSUtil/DSUtil.h"
#include "DSUtil/DShowCommon.h"
#include "DSUtil/SmartPtr.h"
#include "utils/SystemInfo.h" //g_sysinfo
#include "settings/GUISettings.h"//g_guiSettings


#include "filters/VMR9AllocatorPresenter.h"
#include "filters/EVRAllocatorPresenter.h"

#include <initguid.h>
#include "moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>
#include <Vmr9.h>
#include "utils/Log.h"
#include "FileSystem/SpecialProtocol.h"

//XML CONFIG HEADERS
#include "tinyXML/tinyxml.h"
#include "utils/XMLUtils.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
//END XML CONFIG HEADERS

#include "DSPlayer.h"
#include "FilterCoreFactory/FilterCoreFactory.h"

using namespace std;

//
// CFGManager
//

CFGManager::CFGManager():
  m_dwRegister(0),
  m_audioPinConnected(false),
  m_videoPinConnected(false)
{
  m_pUnkInner.CoCreateInstance(CLSID_FilterGraph, NULL);
  m_pUnkInner.QueryInterface(&g_dsGraph->pFilterGraph);
  m_pFM.CoCreateInstance(CLSID_FilterMapper2);
}

CFGManager::~CFGManager()
{
  CSingleLock CSingleLock(*this);

  SAFE_DELETE(m_CfgLoader);
  m_pFM = NULL;

  m_pUnkInner = NULL;

  CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);
}

HRESULT CFGManager::QueryInterface(const IID &iid, void** ppv)
{
  HRESULT hr;
  CheckPointer(ppv, E_POINTER);
  hr = m_pUnkInner->QueryInterface(iid, ppv);
  if (SUCCEEDED(hr))
    return hr;
  BeginEnumFilters(g_dsGraph->pFilterGraph, pEF, pBF)
  {
    hr = pBF->QueryInterface(iid, ppv);
    if (SUCCEEDED(hr))
      break;
  }
  EndEnumFilters
  
  return hr;
}

void CFGManager::CStreamPath::Append(IBaseFilter* pBF, IPin* pPin)
{
  path_t p;
  p.clsid = GetCLSID(pBF);
  p.filter = GetFilterName(pBF);
  p.pin = GetPinName(pPin);
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
  CSingleLock lock(*this);

  HRESULT hr;

  if (g_dsGraph->pFilterGraph)
  {
    hr = g_dsGraph->pFilterGraph->AddFilter(pFilter, pName);
    if(FAILED(hr))
    return hr;
  }

  // TODO
  hr = pFilter->JoinFilterGraph(NULL, NULL);
  hr = pFilter->JoinFilterGraph(g_dsGraph->pFilterGraph, pName);
  
  //SAFE_RELEASE(pIFG);
  return hr;
}

STDMETHODIMP CFGManager::RemoveFilter(IBaseFilter* pFilter)
{
  if(!g_dsGraph->pFilterGraph) 
    return E_UNEXPECTED;

  CSingleLock CSingleLock(*this);

  return g_dsGraph->pFilterGraph->RemoveFilter(pFilter);
}

STDMETHODIMP CFGManager::EnumFilters(IEnumFilters** ppEnum)
{
  if(!g_dsGraph->pFilterGraph) 
    return E_UNEXPECTED;

  return g_dsGraph->pFilterGraph->EnumFilters(ppEnum);
}

STDMETHODIMP CFGManager::FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter)
{
  if(!g_dsGraph->pFilterGraph) 
    return E_UNEXPECTED;
  CSingleLock CSingleLock(*this);

  return g_dsGraph->pFilterGraph->FindFilterByName(pName, ppFilter);
}

STDMETHODIMP CFGManager::ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt)
{
  if(!m_pUnkInner) return E_UNEXPECTED;

  CSingleLock CSingleLock(*this);

  Com::SmartPtr<IBaseFilter> pBF = GetFilterFromPin(pPinIn);
  CLSID clsid = GetCLSID(pBF);

  // TODO: GetUpStreamFilter goes up on the first input pin only
  for(Com::SmartPtr<IBaseFilter> pBFUS = GetFilterFromPin(pPinOut); pBFUS; pBFUS = GetUpStreamFilter(pBFUS))
  {
    if(pBFUS == pBF) return VFW_E_CIRCULAR_GRAPH;
    if(GetCLSID(pBFUS) == clsid) return VFW_E_CANNOT_CONNECT;
  }

  

  HRESULT hr = Com::SmartQIPtr<IFilterGraph2>(m_pUnkInner)->ConnectDirect(pPinOut, pPinIn, pmt);

#ifdef _DSPLAYER_DEBUG
  CStdString filterNameIn, filterNameOut;
  CStdString pinNameIn, pinNameOut;
  Com::SmartPtr<IBaseFilter> pBFOut = GetFilterFromPin(pPinOut);
  CStdString strPinType;
  strPinType = GetPinMainTypeString(pPinOut);
  g_charsetConverter.wToUTF8(GetFilterName(pBFOut), filterNameOut);
  g_charsetConverter.wToUTF8(GetPinName(pPinOut), pinNameOut);
  g_charsetConverter.wToUTF8(GetFilterName(pBF), filterNameIn);
  g_charsetConverter.wToUTF8(GetPinName(pPinIn), pinNameIn);

  CLog::Log(LOGDEBUG, "%s: %s connecting %s.%s.Type:%s pin to %s.%s", __FUNCTION__,
                                                                    (SUCCEEDED(hr) ? "Succeeded": "Failed"),
                                                                    filterNameOut.c_str(),
                                                                    pinNameOut.c_str(),
                                                                    strPinType.c_str(),
                                                                    filterNameIn.c_str(),
                                                                    pinNameIn.c_str());
#endif

  return hr;
}

STDMETHODIMP CFGManager::Reconnect(IPin* ppin)
{
  if(! g_dsGraph->pFilterGraph) 
    return E_UNEXPECTED;
  CSingleLock CSingleLock(*this);

  return g_dsGraph->pFilterGraph->Reconnect(ppin);
}

STDMETHODIMP CFGManager::Disconnect(IPin* ppin)
{
  if(! g_dsGraph->pFilterGraph) 
    return E_UNEXPECTED;
  CSingleLock CSingleLock(*this);

  return g_dsGraph->pFilterGraph->Disconnect(ppin);
}

STDMETHODIMP CFGManager::SetDefaultSyncSource()
{
  if (! g_dsGraph->pFilterGraph)
    return E_UNEXPECTED;
  CSingleLock CSingleLock(*this);

  return g_dsGraph->pFilterGraph->SetDefaultSyncSource();
}

// IGraphBuilder

STDMETHODIMP CFGManager::Connect(IPin* pPinOut, IPin* pPinIn)
{
  CSingleLock CSingleLock(*this);

  CheckPointer(pPinOut, E_POINTER);

  HRESULT hr;

  if(S_OK != IsPinDirection(pPinOut, PINDIR_OUTPUT) 
    || pPinIn && S_OK != IsPinDirection(pPinIn, PINDIR_INPUT))
    return VFW_E_INVALID_DIRECTION;

  if(S_OK == IsPinConnected(pPinOut)
    || pPinIn && S_OK == IsPinConnected(pPinIn))
    return VFW_E_ALREADY_CONNECTED;

  bool fDeadEnd = true;

  if(pPinIn)
  {
    // 1. Try a direct connection between the filters, with no intermediate filters

    if(SUCCEEDED(hr = ConnectDirect(pPinOut, pPinIn, NULL)))
      return hr;
  }

  {

    std::vector<IBaseFilter *> pBFS;
    BeginEnumFilters(g_dsGraph->pFilterGraph, pEF, pBF)
    {
      if(pPinIn && GetFilterFromPin(pPinIn) == pBF 
        || GetFilterFromPin(pPinOut) == pBF)
        continue;

      pBFS.push_back(pBF);
    }
    EndEnumFilters

    for (unsigned int i = 0; i < pBFS.size(); i++)
    {
      IBaseFilter *pBF = pBFS[i];

      if(SUCCEEDED(hr = ConnectFilterDirect(pPinOut, pBF, NULL)))
      {
        if(! IsStreamEnd(pBF)) fDeadEnd = false;

        if(SUCCEEDED(hr = ConnectFilter(pBF, pPinIn)))
          return hr;
      }

      Disconnect(pPinOut);
    }

  }

  return pPinIn ? VFW_E_CANNOT_CONNECT : VFW_E_CANNOT_RENDER;
}

STDMETHODIMP CFGManager::Render(IPin* pPinOut)
{
  return g_dsGraph->pFilterGraph->Render(pPinOut);
}

HRESULT CFGManager::RenderFileXbmc(const CFileItem& pFileItem)
{
  CSingleLock lock(*this);
  HRESULT hr = S_OK;

  //update ffdshow registry to avoid stupid connection problem
  //TODO move updateregistry to the xbmc gui like setting the stable codecs for ffdshow
  //UpdateRegistry();

  //Clearing config need to be done before the file is loaded. Some config are set when the pin are getting connected
  g_dsconfig.ClearConfig();
  START_PERFORMANCE_COUNTER
  if (FAILED(m_CfgLoader->LoadFilterRules(pFileItem)))
  {
    CLog::Log(LOGERROR, "%s Failed to load filters rules", __FUNCTION__);
    return E_FAIL;
  } 
  else
    CLog::Log(LOGDEBUG, "%s Successfully loaded filters rules", __FUNCTION__);
  END_PERFORMANCE_COUNTER("Loading filters rules");

  START_PERFORMANCE_COUNTER
  hr = ConnectFilter(CGraphFilters::Get()->Splitter.pBF , NULL);
  //In some case its going to failed because the source filter is not the splitter
  if (hr == S_FALSE)
    hr = ConnectFilter(CGraphFilters::Get()->Source.pBF, NULL);
  END_PERFORMANCE_COUNTER("Connecting filters");

  if (hr != S_OK)
  {
    if (FAILED(hr = RecoverFromGraphError(pFileItem)))
      return hr;
  }

  //Apparently the graph don't start with unconnected filters in the graph for wmv files
  //And its also going to be needed for error in the filters set by the user are not getting connected
  RemoveUnconnectedFilters(g_dsGraph->pFilterGraph);

  g_dsconfig.ConfigureFilters();
#ifdef _DSPLAYER_DEBUG
  LogFilterGraph();
#endif

  return hr;  
}

STDMETHODIMP CFGManager::Abort()
{
  if (!g_dsGraph->pFilterGraph)
    return E_UNEXPECTED;
  CSingleLock CSingleLock(*this);
  
  return g_dsGraph->pFilterGraph->Abort();
}

STDMETHODIMP CFGManager::ShouldOperationContinue()
{
  if (! g_dsGraph->pFilterGraph)
    return E_UNEXPECTED;
  CSingleLock CSingleLock(*this);

  return g_dsGraph->pFilterGraph->ShouldOperationContinue();
}

// IFilterGraph2

STDMETHODIMP CFGManager::AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter)
{
  if (! g_dsGraph->pFilterGraph)
    return E_UNEXPECTED;
  CSingleLock CSingleLock(*this);
  
  return g_dsGraph->pFilterGraph->AddSourceFilterForMoniker(pMoniker, pCtx, lpcwstrFilterName, ppFilter);
}

STDMETHODIMP CFGManager::ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt)
{
  if (!g_dsGraph->pFilterGraph)
    return E_UNEXPECTED;
  CSingleLock CSingleLock(*this);
  
  return g_dsGraph->pFilterGraph->ReconnectEx(ppin, pmt);
}

// IGraphBuilder2

HRESULT CFGManager::IsPinDirection(IPin* pPin, PIN_DIRECTION dir1)
{
  CSingleLock CSingleLock(*this);

  CheckPointer(pPin, E_POINTER);

  PIN_DIRECTION dir2;
  if(FAILED(pPin->QueryDirection(&dir2)))
    return E_FAIL;

  return dir1 == dir2 ? S_OK : S_FALSE;
}

HRESULT CFGManager::IsPinConnected(IPin* pPin)
{
  CSingleLock CSingleLock(*this);

  CheckPointer(pPin, E_POINTER);

  IPin* pPinTo;
  return SUCCEEDED(pPin->ConnectedTo(&pPinTo)) && pPinTo ? S_OK : S_FALSE;
}

HRESULT CFGManager::ConnectFilter(IBaseFilter* pBF, IPin* pPinIn)
{
  CSingleLock CSingleLock(*this);

  CheckPointer(pBF, E_POINTER);

  if(pPinIn && S_OK != IsPinDirection(pPinIn, PINDIR_INPUT))
    return VFW_E_INVALID_DIRECTION;

  int nTotal = 0, nRendered = 0;
  GUID mediaType = GUID_NULL;

  /* Only the video pin and one audio pin can be connected
  if the filter is the splitter */
  if (pBF == CGraphFilters::Get()->Splitter.pBF
    && m_audioPinConnected
    && m_videoPinConnected)
    return S_OK;

  BeginEnumPins(pBF, pEP, pPin)
  {
    if(GetPinName(pPin)[0] != '~'
    && S_OK == IsPinDirection(pPin, PINDIR_OUTPUT)
    && S_OK != IsPinConnected(pPin))
    {
      BeginEnumMediaTypes(pPin, pEnum, pMediaType)
      {
        mediaType = pMediaType->majortype;
      }
      EndEnumMediaTypes(pMediaType)

      if (pBF == CGraphFilters::Get()->Splitter.pBF && (mediaType == MEDIATYPE_Video) 
          && m_videoPinConnected)
        continue;                               // A video pin is already connected, continue !
      else if (pBF == CGraphFilters::Get()->Splitter.pBF && (mediaType == MEDIATYPE_Audio)
          && m_audioPinConnected)
        continue;                               // An audio pin is already connected, continue !
      else if (pBF == CGraphFilters::Get()->Splitter.pBF && mediaType == MEDIATYPE_Subtitle)
        continue;                               // We don't connect subtitle pin yet, continue !*/

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

      if (pBF == CGraphFilters::Get()->Splitter.pBF && (mediaType == MEDIATYPE_Video) 
          && SUCCEEDED(hr))
        m_videoPinConnected = true;
      else if (pBF == CGraphFilters::Get()->Splitter.pBF && (mediaType == MEDIATYPE_Audio)
          && SUCCEEDED(hr))
        m_audioPinConnected = true;

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
  CSingleLock CSingleLock(*this);

  CheckPointer(pPinOut, E_POINTER);
  CheckPointer(pBF, E_POINTER);

  HRESULT hr;

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir;
    if(GetPinName(pPin)[0] != '~'
    && SUCCEEDED(hr = pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
    && SUCCEEDED(hr = Connect(pPinOut, pPin)))
    {
      return hr;
    }
  }
  EndEnumPins

  return VFW_E_CANNOT_CONNECT;
}

HRESULT CFGManager::ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt)
{
  CSingleLock CSingleLock(*this);

  CheckPointer(pPinOut, E_POINTER);
  CheckPointer(pBF, E_POINTER);

  if(S_OK != IsPinDirection(pPinOut, PINDIR_OUTPUT))
    return VFW_E_INVALID_DIRECTION;

  HRESULT hr;

  BeginEnumPins(pBF, pEP, pPin)
  {
    if(GetPinName(pPin)[0] != '~'
      && S_OK == IsPinDirection(pPin, PINDIR_INPUT)
      && S_OK != IsPinConnected(pPin)
      && SUCCEEDED(hr = ConnectDirect(pPinOut, pPin, pmt)))
      return hr;

  }
  EndEnumPins

  return VFW_E_CANNOT_CONNECT;
}

/*HRESULT CFGManager::NukeDownstream(IUnknown* pUnk)
{
  CSingleLock CSingleLock(*this);

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
      if(IBaseFilter* pBF = GetFilterFromPin(pPinTo))
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
}*/

HRESULT CFGManager::AddToROT()
{
  CSingleLock CSingleLock(*this);

  HRESULT hr = E_FAIL;

  if(m_dwRegister)
    return S_FALSE;

  Com::SmartPtr<IRunningObjectTable> pROT;
  Com::SmartPtr<IMoniker> pMoniker;
  WCHAR wsz[256] = {0};
  _snwprintf_s(wsz, _countof(wsz), 255, L"FilterGraph %08p pid %08x (XBMC)", (DWORD_PTR)this, GetCurrentProcessId());

  if(SUCCEEDED(hr = GetRunningObjectTable(0, &pROT))
  && SUCCEEDED(hr = CreateItemMoniker(L"!", wsz, &pMoniker)))
        hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, g_dsGraph->pFilterGraph, pMoniker, &m_dwRegister);

  return hr;
}

HRESULT CFGManager::RemoveFromROT()
{
  CSingleLock CSingleLock(*this);

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
  CSingleLock CSingleLock(*this);

  if(iIndex < 0 || (unsigned int) iIndex >= m_deadends.size()) 
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
#ifdef _DSPLAYER_DEBUG
void CFGManager::LogFilterGraph(void)
{
  CStdString buffer;
  CLog::Log(LOGDEBUG, "Starting filters listing ...");
  BeginEnumFilters(g_dsGraph->pFilterGraph, pEF, pBF)
  {
    g_charsetConverter.wToUTF8(GetFilterName(pBF), buffer);
    CLog::Log(LOGDEBUG, "%s", buffer.c_str());
  }
  EndEnumFilters
  CLog::Log(LOGDEBUG, "End of filters listing");
}
#endif

void CFGManager::InitManager()
{

/*  WORD merit_low = 1;
  ULONGLONG merit;
  merit = MERIT64_ABOVE_DSHOW + merit_low++;


// Blocked filters
  
// "Subtitle Mixer" makes an access violation around the 
// 11-12th media type when enumerating them on its output.
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{00A95963-3BE5-48C0-AD9F-3356D67EA09D}")), MERIT64_DO_NOT_USE));

// DiracSplitter.ax is crashing MPC-HC when opening invalid files...
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{09E7F58E-71A1-419D-B0A0-E524AE1454A9}")), MERIT64_DO_NOT_USE));
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{5899CFB9-948F-4869-A999-5544ECB38BA5}")), MERIT64_DO_NOT_USE));
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{F78CF248-180E-4713-B107-B13F7B5C31E1}")), MERIT64_DO_NOT_USE));

// ISCR suxx
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}")), MERIT64_DO_NOT_USE));

// Samsung's "mpeg-4 demultiplexor" can even open matroska files, amazing...
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{99EC0C72-4D1B-411B-AB1F-D561EE049D94}")), MERIT64_DO_NOT_USE));

// LG Video Renderer (lgvid.ax) just crashes when trying to connect it
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{9F711C60-0668-11D0-94D4-0000C02BA972}")), MERIT64_DO_NOT_USE));

// palm demuxer crashes (even crashes graphedit when dropping an .ac3 onto it)
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{BE2CF8A7-08CE-4A2C-9A25-FD726A999196}")), MERIT64_DO_NOT_USE));

// mainconcept color space converter
  m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{272D77A0-A852-4851-ADA4-9091FEAD4C86}")), MERIT64_DO_NOT_USE));

  //Block if vmr9 is used
  if (g_sysinfo.IsVistaOrHigher())
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{9852A670-F845-491B-9BE6-EBD841B8A613}")), MERIT64_DO_NOT_USE));
  else
    if (!g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_transform.push_back(new CFGFilterRegistry(GUIDFromCString(_T("{9852A670-F845-491B-9BE6-EBD841B8A613}")), MERIT64_DO_NOT_USE));

    
  
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
  fileconfigtmp = _P("special://xbmc/system/players/dsplayer/dsfilterconfig.xml");*/

  //Load the config for the xml
  m_CfgLoader = new CFGLoader();

  if (SUCCEEDED(m_CfgLoader->LoadConfig()))
    CLog::Log(LOGNOTICE,"Successfully loaded rules");
  else
    CLog::Log(LOGERROR,"Failed loading rules");
}

HRESULT CFGManager::RecoverFromGraphError(const CFileItem& pFileItem)
{
  Com::SmartPtr<IBaseFilter> pBF = CGraphFilters::Get()->Splitter.pBF;
  int nVideoPin = 0, nAudioPin = 0;
  int nConnectedVideoPin = 0, nConnectedAudioPin = 0;
  bool videoError = false, audioError = false;
  HRESULT hr = S_OK;
  BeginEnumPins(pBF, pEP, pPin)
  {
    BeginEnumMediaTypes(pPin, pEMT, pMT)
    {
      if (pMT->majortype == MEDIATYPE_Video)
      {
        nVideoPin++;
        if (IsPinConnected(pPin))
          nConnectedVideoPin++;
      }
      if (pMT->majortype == MEDIATYPE_Audio)
      {
        nAudioPin++;
        if (IsPinConnected(pPin))
          nConnectedAudioPin++;
      }
      break;
    }
    EndEnumMediaTypes(pMT)
  }
  EndEnumPins

  videoError = (nVideoPin >= 1 && nConnectedVideoPin == 0); // No error if no video stream
  audioError = (nAudioPin >= 1 && nConnectedAudioPin == 0); // No error if no audio stream
  //Work around to fix the audio pipeline
  //I think this should be the best way to render a pin if there an error the audio stream
  //The only problem is the graph is getting locked after that
  if (audioError)
  {
    BeginEnumPins(pBF, pEP, pPin)
    {
      BeginEnumMediaTypes(pPin, pEMT, pMT)
      {
        if (pMT->majortype == MEDIATYPE_Audio)
        {
          if (SUCCEEDED(Render(pPin))) //Todo: The Render method add non desire filters to the graph!
          {
            audioError = false;
          }
        }
      }
      EndEnumMediaTypes(pMT)
    }
    EndEnumPins
  }
  if (videoError)
  {
    if (CGraphFilters::Get()->IsUsingDXVADecoder())
    {
      // We've try to use a dxva decoder. Maybe the file wasn't dxva compliant. Fallback on default video renderer
      g_dsGraph->pFilterGraph->RemoveFilter(CGraphFilters::Get()->Video.pBF);
      CGraphFilters::Get()->Video.pBF.FullRelease();
      CGraphFilters::Get()->Video.Clear();

      CStdString filter = "";
      if ( SUCCEEDED(CFilterCoreFactory::GetVideoFilter(pFileItem, filter, false)))
      {
        if (SUCCEEDED(m_CfgLoader->InsertFilter(filter, CGraphFilters::Get()->Video)))
        {
          /* Default filter is in the graph, connect it */
          hr = ConnectFilter(CGraphFilters::Get()->Splitter.pBF , NULL);
          if (SUCCEEDED(hr))
          {
            CLog::Log(LOGNOTICE, "%s Rendering fails when using DXVA renderer. \"%s\" renderer used instead.", __FUNCTION__, CGraphFilters::Get()->Video.osdname.c_str());
            videoError = false;
          }
        }
      }
    }

    if (videoError)
    {
      //Video renderer cant be changed so we have to force the connection from 
      //the splitter to the video renderer and nothing else
      IBaseFilter *pBFS = CGraphFilters::Get()->Splitter.pBF;

      BeginEnumPins(pBFS, pEP, pPin)
      {
        BeginEnumMediaTypes(pPin, pEMT, pMT)
        {
          if (pMT->majortype == MEDIATYPE_Video)
          {
            if (SUCCEEDED(Render(pPin)))
            {
              videoError = false;
              //Ok got it
            }
          }
        }
        EndEnumMediaTypes(pMT)
      }
      EndEnumPins
    }

    if (!videoError)
    {
      IBaseFilter *pBFV = CGraphFilters::Get()->VideoRenderer.pBF;
      Com::SmartPtr<IPin> pPinV = GetFirstPin(pBFV, PINDIR_INPUT);
      if (IsPinConnected(pPinV))
      {
        CLog::Log(LOGINFO, "%s There were some errors in your rendering chain. Filters have been changed.", __FUNCTION__);
      }
      else
        videoError = true;

      pBFV = NULL;
      pPinV = NULL;
    }
  }
  if (videoError || audioError)
  {
    CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      CStdString strError;

      if (videoError && audioError)
        strError = CStdString("Error in both audio and video rendering chain.");
      else if (videoError)
        strError = CStdString("Error in the video rendering chain.");
      else if (audioError)
        strError = CStdString("Error in the audio rendering chain.");

      CLog::Log(LOGERROR, "%s Audio / Video error \n %s \n Ensure that the audio/video stream is supported by your selected decoder and ensure that the decoder is properly configured.", __FUNCTION__, strError.c_str());
      dialog->SetHeading("Audio / Video error");
      dialog->SetLine(0, strError.c_str());
      dialog->SetLine(1, "more details or see XBMC Wiki - 'DSPlayer codecs'");
      dialog->SetLine(2, "section for more informations.");
      
      CDSPlayer::errorWindow = dialog;

      return E_FAIL;
    }
  }

  return S_OK;
}

#endif