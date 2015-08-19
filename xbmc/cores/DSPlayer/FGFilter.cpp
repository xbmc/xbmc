/*
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
 *
 *	Copyright (C) 2010-2013 Eduard Kytmanov
 *	http://www.avmedia.su
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

#include "FGFilter.h"
#include "DSUtil/SmartPtr.h"


#include "Filters/VMR9AllocatorPresenter.h"
#include "Filters/EVRAllocatorPresenter.h"
#include "Filters/madVRAllocatorPresenter.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/charsetconverter.h"
#include "utils/XMLUtils.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "Dmodshow.h"
#include "Dmoreg.h"
#include "settings/Settings.h"
#include "../../filesystem/SpecialProtocol.h "
#include "cores/DSPlayer/DSPlayer.h"
#pragma comment(lib, "Dmoguids.lib")
//
// CFGFilter
//
#define _P(x)     CSpecialProtocol::TranslatePath(x)

CFGFilter::CFGFilter(const CLSID& clsid, Type type, CStdString name)
  : m_clsid(clsid)
  , m_name(name)
  , m_type(type)
{
}

void CFGFilter::AddType(const GUID& majortype, const GUID& subtype)
{
  m_types.push_back(majortype);
  m_types.push_back(subtype);
}

//
// CFGFilterRegistry
//

CFGFilterRegistry::CFGFilterRegistry(IMoniker* pMoniker)
  : CFGFilter(GUID_NULL, CFGFilter::REGISTRY, L"")
  , m_pMoniker(pMoniker)
{
  if (!m_pMoniker) return;

  LPOLESTR str = NULL;
  if (FAILED(m_pMoniker->GetDisplayName(0, 0, &str))) return;
  m_DisplayName = m_name = str;
  CoTaskMemFree(str), str = NULL;

  IPropertyBag* pPB;
  if (SUCCEEDED(m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
  {
    tagVARIANT var;
    if (SUCCEEDED(pPB->Read(LPCOLESTR(L"FriendlyName"), &var, NULL)))
    {
      m_name = var.bstrVal;
      VariantClear(&var);
    }

    if (SUCCEEDED(pPB->Read(LPCOLESTR(L"CLSID"), &var, NULL)))
    {
      CLSIDFromString(var.bstrVal, &m_clsid);
      VariantClear(&var);
    }

    if (SUCCEEDED(pPB->Read(LPCOLESTR(L"FilterData"), &var, NULL)))
    {
      BSTR* pstr;
      if (SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pstr)))
      {
        ExtractFilterData((BYTE*)pstr, var.parray->cbElements*(var.parray->rgsabound[0].cElements));
        SafeArrayUnaccessData(var.parray);
      }

      VariantClear(&var);
    }
  }
}

CFGFilterRegistry::CFGFilterRegistry(CStdString DisplayName)
  : CFGFilter(GUID_NULL, CFGFilter::REGISTRY, L"")
  , m_DisplayName(DisplayName)
{
  if (m_DisplayName.IsEmpty()) return;

  IBindCtx* pBC;
  CreateBindCtx(0, &pBC);

  ULONG chEaten;
  if (S_OK != MkParseDisplayName(pBC, AToW(DisplayName), &chEaten, &m_pMoniker))
    return;

  IPropertyBag* pPB;
  if (SUCCEEDED(m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
  {
    tagVARIANT var;
    if (SUCCEEDED(pPB->Read(LPCOLESTR(L"FriendlyName"), &var, NULL)))
    {
      m_name = var.bstrVal;
      VariantClear(&var);
    }

    if (SUCCEEDED(pPB->Read(LPCOLESTR(L"CLSID"), &var, NULL)))
    {
      CLSIDFromString(var.bstrVal, &m_clsid);
      VariantClear(&var);
    }

    if (SUCCEEDED(pPB->Read(LPCOLESTR(L"FilterData"), &var, NULL)))
    {
      BSTR* pstr;
      if (SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pstr)))
      {
        ExtractFilterData((BYTE*)pstr, var.parray->cbElements*(var.parray->rgsabound[0].cElements));
        SafeArrayUnaccessData(var.parray);
      }

      VariantClear(&var);
    }
  }
}

CFGFilterRegistry::CFGFilterRegistry(const CLSID& clsid)
  : CFGFilter(clsid, CFGFilter::REGISTRY, L"")
{
  m_pMoniker = NULL;
  if (m_clsid == GUID_NULL) return;

  CStdString guid = StringFromGUID(m_clsid);

}

HRESULT CFGFilterRegistry::Create(IBaseFilter** ppBF)
{
  CheckPointer(ppBF, E_POINTER);

  HRESULT hr = E_FAIL;

  if (m_pMoniker)
  {
    if (SUCCEEDED(hr = m_pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)ppBF)))
      m_clsid = ::GetCLSID(*ppBF);
  }
  else if (m_clsid != GUID_NULL)
    hr = CoCreateInstance(m_clsid, NULL, CLSCTX_ALL, IID_IBaseFilter, (void**)ppBF);

  return hr;
};

[uuid("97f7c4d4-547b-4a5f-8332-536430ad2e4d")]
interface IAMFilterData : public IUnknown
{
  STDMETHOD(ParseFilterData) (BYTE* rgbFilterData, ULONG cb, BYTE** prgbRegFilter2) PURE;
  STDMETHOD(CreateFilterData) (REGFILTER2* prf2, BYTE** prgbFilterData, ULONG* pcb) PURE;
};

void CFGFilterRegistry::ExtractFilterData(BYTE* p, UINT len)
{
  IAMFilterData* pFD;
  BYTE* ptr = NULL;

  if (SUCCEEDED(CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_ALL, __uuidof(pFD), (void**)&pFD))
    && SUCCEEDED(pFD->ParseFilterData(p, len, (BYTE**)&ptr)))
  {
    REGFILTER2* prf = (REGFILTER2*)*(DWORD*)ptr; // this is f*cked up

    if (prf->dwVersion == 1)
    {
      for (UINT i = 0; i < prf->cPins; i++)
      {
        if (prf->rgPins[i].bOutput)
          continue;

        for (UINT j = 0; j < prf->rgPins[i].nMediaTypes; j++)
        {
          if (!prf->rgPins[i].lpMediaType[j].clsMajorType || !prf->rgPins[i].lpMediaType[j].clsMinorType)
            break;

          const REGPINTYPES& rpt = prf->rgPins[i].lpMediaType[j];
          AddType(*rpt.clsMajorType, *rpt.clsMinorType);
        }
      }
    }
    else if (prf->dwVersion == 2)
    {
      for (UINT i = 0; i < prf->cPins2; i++)
      {
        if (prf->rgPins2[i].dwFlags&REG_PINFLAG_B_OUTPUT)
          continue;

        for (UINT j = 0; j < prf->rgPins2[i].nMediaTypes; j++)
        {
          if (!prf->rgPins2[i].lpMediaType[j].clsMajorType || !prf->rgPins2[i].lpMediaType[j].clsMinorType)
            break;

          const REGPINTYPES& rpt = prf->rgPins2[i].lpMediaType[j];
          AddType(*rpt.clsMajorType, *rpt.clsMinorType);
        }
      }
    }

    CoTaskMemFree(prf);
  }
  else
  {
    BYTE* base = p;

#define ChkLen(size) if(p - base + size > (int)len) return;

    ChkLen(4)
      if (*(DWORD*)p != 0x00000002) return; // only version 2 supported, no samples found for 1
    p += 4;

    ChkLen(4)
      p += 4;

    while (!m_types.empty())
      m_types.pop_back();

    ChkLen(8)
      DWORD nPins = *(DWORD*)p; p += 8;
    while (nPins-- > 0)
    {
      ChkLen(1)
        BYTE n = *p - 0x30; p++;

      ChkLen(2)
        WORD pi = *(WORD*)p; p += 2;
      ASSERT(pi == 'ip');

      ChkLen(1)
        BYTE x33 = *p; p++;
      ASSERT(x33 == 0x33);

      ChkLen(8)
        bool fOutput = !!(*p&REG_PINFLAG_B_OUTPUT);
      p += 8;

      ChkLen(12)
        DWORD nTypes = *(DWORD*)p; p += 12;
      while (nTypes-- > 0)
      {
        ChkLen(1)
          BYTE n = *p - 0x30; p++;

        ChkLen(2)
          WORD ty = *(WORD*)p; p += 2;
        ASSERT(ty == 'yt');

        ChkLen(5)
          BYTE x33 = *p; p++;
        ASSERT(x33 == 0x33);
        p += 4;

        ChkLen(8)
          if (*(DWORD*)p < (DWORD)(p - base + 8) || *(DWORD*)p >= len
            || *(DWORD*)(p + 4) < (DWORD)(p - base + 8) || *(DWORD*)(p + 4) >= len)
          {
          p += 8;
          continue;
          }

        GUID majortype, subtype;
        memcpy(&majortype, &base[*(DWORD*)p], sizeof(GUID)); p += 4;
        if (!fOutput) AddType(majortype, subtype);
      }
    }

#undef ChkLen
  }
}

//
// CFGFilterFile
//

CFGFilterFile::CFGFilterFile(const CLSID& clsid, CStdString path, CStdStringW name, CStdString internalName, CStdString filetype)
  : CFGFilter(clsid, CFGFilter::FILE, name),
  m_path(path),
  m_xFileType(filetype),
  m_internalName(internalName),
  m_hInst(NULL),
  m_isDMO(false)
{
}

CFGFilterFile::CFGFilterFile(TiXmlElement *pFilter)
  : CFGFilter(CFGFilter::FILE)
  , m_isDMO(false)
  , m_catDMO(GUID_NULL)
  , m_hInst(NULL)
  , m_path("")
{
  bool m_filterFound = true;

  m_internalName = pFilter->Attribute("name");
  m_xFileType = pFilter->Attribute("type"); // Currently not used

  CStdString guid = "";
  XMLUtils::GetString(pFilter, "guid", guid);
  const CLSID clsid = GUIDFromString(guid);

  CStdString osdname = "";
  XMLUtils::GetString(pFilter, "osdname", osdname);

  //This is needed for a correct insertion of dmo filters into a graph
  XMLUtils::GetBoolean(pFilter, "isdmo", m_isDMO);

  CStdString strDmoGuid = "";
  if (XMLUtils::GetString(pFilter, "guid_category_dmo", strDmoGuid))
  {
    const CLSID dmoclsid = GUIDFromString(strDmoGuid);
    m_catDMO = dmoclsid;
  }

  if (!XMLUtils::GetString(pFilter, "path", m_path) || m_path.empty())
  {
    m_filterFound = false;
  }
  else {
    if (!XFILE::CFile::Exists(m_path))
    {
      CStdString path(m_path);
      m_path = CProfilesManager::GetInstance().GetUserDataItem("dsplayer/" + path);
      if (!XFILE::CFile::Exists(m_path))
      {
        m_path.Format("special://xbmc/system/players/dsplayer/%s", path.c_str());
        if (!XFILE::CFile::Exists(m_path))
        {
          m_filterFound = false;
        }
      }
    }
  }

  m_path = m_filterFound ? _P(m_path) : GetFilterPath(clsid);

  // Call super constructor
  m_clsid = clsid;
  m_name = osdname;
}

HRESULT CFGFilterFile::Create(IBaseFilter** ppBF)
{
  CheckPointer(ppBF, E_POINTER);

  HRESULT hr = E_FAIL;
  if ((m_isDMO) && (m_catDMO != GUID_NULL))
  {
    Com::SmartPtr<IBaseFilter> pBFDmo = NULL;
    Com::SmartQIPtr<IDMOWrapperFilter> pDMOWrapper;

    hr = pBFDmo.CoCreateInstance(CLSID_DMOWrapperFilter, NULL);
    if (SUCCEEDED(hr))
    {
      pDMOWrapper = pBFDmo;
      if (pDMOWrapper)
        hr = pDMOWrapper->Init(m_clsid, m_catDMO);

      *ppBF = pBFDmo.Detach();
    }
  }
  else
  {
    hr = LoadExternalFilter(m_path, m_clsid, ppBF);
    if (FAILED(hr))
    {
      CStdString guid;
      g_charsetConverter.wToUTF8(StringFromGUID(m_clsid), guid);
      CLog::Log(LOGINFO, "%s Failed to load external filter (clsid:%s path:%s). Trying with CoCreateInstance", __FUNCTION__,
        guid.c_str(), m_path.c_str());

      /* If LoadExternalFilter failed, maybe we will have more chance
       * using CoCreateInstance directly. Will only works if the filter
       * is registered! */
      Com::SmartQIPtr<IBaseFilter> pBF;
      hr = pBF.CoCreateInstance(m_clsid);
      if (FAILED(hr))
      {
        CLog::Log(LOGFATAL, "%s CoCreateInstance failed!", __FUNCTION__);
        return hr;
      }
      *ppBF = pBF.Detach();
    }

  }

  CStdString guid;
  g_charsetConverter.wToUTF8(StringFromGUID(m_clsid), guid);

  if (FAILED(hr))
    CLog::Log(LOGERROR, "%s Failed to load external filter (clsid:%s path:%s)", __FUNCTION__,
    guid.c_str(), m_path.c_str());
  else
    CLog::Log(LOGDEBUG, "%s Successfully loaded external filter (clsid:%s path:%s)", __FUNCTION__,
    guid.c_str(), m_path.c_str());

  return hr;
}

//
// CFGFilterVideoRenderer
//

CFGFilterVideoRenderer::CFGFilterVideoRenderer(const CLSID& clsid, CStdStringW name)
  : CFGFilter(clsid, VIDEORENDERER, name)
{
  AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
}

CFGFilterVideoRenderer::~CFGFilterVideoRenderer()
{
}

HRESULT CFGFilterVideoRenderer::Create(IBaseFilter** ppBF)
{
  CheckPointer(ppBF, E_POINTER);

  HRESULT hr = S_OK;
  Com::SmartPtr<ISubPicAllocatorPresenter> pCAP;

  CStdString __err;
  if (m_clsid == CLSID_VMR9AllocatorPresenter)
    CreateAP9(m_clsid, g_hWnd, &pCAP);
  else if (m_clsid == CLSID_EVRAllocatorPresenter)
    CreateEVR(m_clsid, g_hWnd, &pCAP);
  else if (m_clsid == CLSID_madVRAllocatorPresenter)
    CreateMadVR(m_clsid, g_hWnd, &pCAP);

  if (pCAP == NULL)
  {
    CLog::Log(LOGERROR, "%s Failed to create the allocater presenter (error: %s)", __FUNCTION__, __err.c_str());
    return E_FAIL;
  }

  /*
  if (SUCCEEDED(hr = pCAP->CreateRenderer((IUnknown **)ppBF)))
  {
    CLog::Log(LOGDEBUG, "%s Allocator presenter successfully created", __FUNCTION__);
  }
 */
  
  Com::SmartPtr<IUnknown> pRenderer;
  if (SUCCEEDED(hr = pCAP->CreateRenderer(&pRenderer))) 
  {
    *ppBF = Com::SmartQIPtr<IBaseFilter>(pRenderer).Detach();
    // madVR supports calling IVideoWindow::put_Owner before the pins are connected
    if (m_clsid == CLSID_madVRAllocatorPresenter) 
    {
      if (Com::SmartQIPtr<IMadVRSubclassReplacement> pMVRSR = pCAP)
        VERIFY(SUCCEEDED(pMVRSR->DisableSubclassing()));

      if (Com::SmartQIPtr<IVideoWindow> pVW = pCAP)
        pVW->put_Owner((OAHWND)CDSPlayer::GetDShWnd());

      // Go out from Kodi exclusive fullscreen mode if needed
      if (!CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN))
      {
        CMadvrCallback::Get()->SetInitMadvr(true);
        CSettings::GetInstance().SetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN, true);
        CGraphFilters::Get()->SetKodiRealFS(true);
        CMadvrCallback::Get()->SetInitMadvr(false);
      }
      else 
      {
        CGraphFilters::Get()->SetKodiRealFS(false);
      }
    }
    CLog::Log(LOGDEBUG, "%s Allocator presenter successfully created", __FUNCTION__);
  }
  
  if (!*ppBF) hr = E_FAIL;

  return hr;
}

#endif