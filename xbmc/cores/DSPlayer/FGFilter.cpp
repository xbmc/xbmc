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

#include "FGFilter.h"
#include "dshowutil/dshowutil.h"


#include "Filters/VMR9AllocatorPresenter.h"
#include "Filters/EVRAllocatorPresenter.h"
#include "WindowingFactory.h"
#include "utils/log.h"
#include "charsetconverter.h"
//
// CFGFilter
//

CFGFilter::CFGFilter(const CLSID& clsid, CStdString name, UINT64 merit)
  : m_clsid(clsid)
  , m_name(name)
{
  m_merit.val = merit;
}

const std::list<GUID>& CFGFilter::GetTypes() const
{
  return m_types;
}

void CFGFilter::SetTypes(const std::list<GUID>& types)
{
  while (!m_types.empty())
    m_types.pop_back();
  for (std::list<GUID>::const_iterator it = types.begin(); it != types.end(); it++)
  {
    m_types.push_back((*it));
    
  }
}

void CFGFilter::AddType(const GUID& majortype, const GUID& subtype)
{
  m_types.push_back(majortype);
  m_types.push_back(subtype);
}

bool CFGFilter::CheckTypes(const std::vector<GUID>& types, bool fExactMatch)
{
  for (std::list<GUID>::const_iterator it = m_types.begin(); it != m_types.end(); it++)
  {
    const GUID& majortype = *it;
    it++;
    if(it == m_types.end()) 
    {
      ASSERT(0); 
      break;
    }
    const GUID& subtype = *it;

    for(int i = 0, len = types.size() & ~1; i < len; i += 2)
    {
      if(fExactMatch)
      {
        if(majortype == types[i] && majortype != GUID_NULL
        && subtype == types[i+1] && subtype != GUID_NULL)
                return true;
      }
      else
      {
        if((majortype == GUID_NULL || types[i] == GUID_NULL || majortype == types[i])
        && (subtype == GUID_NULL || types[i+1] == GUID_NULL || subtype == types[i+1]))
          return true;
      }
    }
  }
  return false;

  /*POSITION pos = m_types.GetHeadPosition();
  while(pos)
  {
    const GUID& majortype = m_types.GetNext(pos);
    if(!pos) {ASSERT(0); break;}
    const GUID& subtype = m_types.GetNext(pos);

    for(int i = 0, len = types.size() & ~1; i < len; i += 2)
    {
      if(fExactMatch)
      {
        if(majortype == types[i] && majortype != GUID_NULL
        && subtype == types[i+1] && subtype != GUID_NULL)
                return true;
      }
      else
      {
        if((majortype == GUID_NULL || types[i] == GUID_NULL || majortype == types[i])
        && (subtype == GUID_NULL || types[i+1] == GUID_NULL || subtype == types[i+1]))
          return true;
      }
    }
  }*/

  return false;
}

//
// CFGFilterRegistry
//

CFGFilterRegistry::CFGFilterRegistry(IMoniker* pMoniker, UINT64 merit) 
  : CFGFilter(GUID_NULL, L"", merit)
  , m_pMoniker(pMoniker)
{
  if(!m_pMoniker) return;

  LPOLESTR str = NULL;
  if(FAILED(m_pMoniker->GetDisplayName(0, 0, &str))) return;
  m_DisplayName = m_name = str;
  CoTaskMemFree(str), str = NULL;

  IPropertyBag* pPB;
  if(SUCCEEDED(m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
  {
    tagVARIANT var;
    if(SUCCEEDED(pPB->Read(LPCOLESTR(L"FriendlyName"), &var, NULL)))
    {
      m_name = var.bstrVal;
      VariantClear(&var);
    }

    if(SUCCEEDED(pPB->Read(LPCOLESTR(L"CLSID"), &var, NULL)))
    {
      CLSIDFromString(var.bstrVal, &m_clsid);
      VariantClear(&var);
    }

    if(SUCCEEDED(pPB->Read(LPCOLESTR(L"FilterData"), &var, NULL)))
    {      
      BSTR* pstr;
      if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pstr)))
      {
        ExtractFilterData((BYTE*)pstr, var.parray->cbElements*(var.parray->rgsabound[0].cElements));
        SafeArrayUnaccessData(var.parray);
      }

      VariantClear(&var);
    }
  }

  if(merit != MERIT64_DO_USE) m_merit.val = merit;
}

CFGFilterRegistry::CFGFilterRegistry(CStdString DisplayName, UINT64 merit) 
  : CFGFilter(GUID_NULL, L"", merit)
  , m_DisplayName(DisplayName)
{
  if(m_DisplayName.IsEmpty()) return;

  IBindCtx* pBC;
  CreateBindCtx(0, &pBC);

  ULONG chEaten;
  CStdStringW m_DisplayNameW;
  g_charsetConverter.subtitleCharsetToW(m_DisplayName,m_DisplayNameW);
  if(S_OK != MkParseDisplayName(pBC, LPCOLESTR(m_DisplayNameW.c_str()), &chEaten, &m_pMoniker))
    return;

  IPropertyBag* pPB;
  if(SUCCEEDED(m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
  {
    tagVARIANT var;
    if(SUCCEEDED(pPB->Read(LPCOLESTR(L"FriendlyName"), &var, NULL)))
    {
      m_name = var.bstrVal;
      VariantClear(&var);
    }

    if(SUCCEEDED(pPB->Read(LPCOLESTR(L"CLSID"), &var, NULL)))
    {
      CLSIDFromString(var.bstrVal, &m_clsid);
      VariantClear(&var);
    }

    if(SUCCEEDED(pPB->Read(LPCOLESTR(L"FilterData"), &var, NULL)))
    {      
      BSTR* pstr;
      if(SUCCEEDED(SafeArrayAccessData(var.parray, (void**)&pstr)))
      {
        ExtractFilterData((BYTE*)pstr, var.parray->cbElements*(var.parray->rgsabound[0].cElements));
        SafeArrayUnaccessData(var.parray);
      }

      VariantClear(&var);
    }
  }

  if(merit != MERIT64_DO_USE) m_merit.val = merit;
}

CFGFilterRegistry::CFGFilterRegistry(const CLSID& clsid, UINT64 merit) 
  : CFGFilter(clsid, L"", merit)
{
  m_pMoniker = NULL;
  if(m_clsid == GUID_NULL) return;

  CStdString guid = DShowUtil::CStringFromGUID(m_clsid);
  
  //CRegKey key;
  
  
  /*if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("CLSID\\") + guid, KEY_READ))
  {
    ULONG nChars = 0;
    if(ERROR_SUCCESS == key.QueryStringValue(NULL, NULL, &nChars))
    {
      CStdString name;
      if(ERROR_SUCCESS == key.QueryStringValue(NULL, name.GetBuffer(nChars), &nChars))
      {
        name.ReleaseBuffer(nChars);
        m_name = name;
      }
    }

    key.Close();
  }

  if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, _T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\") + guid, KEY_READ))
  {
    ULONG nChars = 0;
    if(ERROR_SUCCESS == key.QueryStringValue(_T("FriendlyName"), NULL, &nChars))
    {
      CStdString name;
      if(ERROR_SUCCESS == key.QueryStringValue(_T("FriendlyName"), name.GetBuffer(nChars), &nChars))
      {
        name.ReleaseBuffer(nChars);
        m_name = name;
      }
    }

    ULONG nBytes = 0;
    if(ERROR_SUCCESS == key.QueryBinaryValue(_T("FilterData"), NULL, &nBytes))
    {
      CAutoVectorPtr<BYTE> buff;
      if(buff.Allocate(nBytes) && ERROR_SUCCESS == key.QueryBinaryValue(_T("FilterData"), buff, &nBytes))
      {
        ExtractFilterData(buff, nBytes);
      }
    }

    key.Close();
  }

  if(merit != MERIT64_DO_USE) m_merit.val = merit;*/
}

HRESULT CFGFilterRegistry::Create(IBaseFilter** ppBF)
{
  CheckPointer(ppBF, E_POINTER);

  HRESULT hr = E_FAIL;
  
  if(m_pMoniker)
  {
    if(SUCCEEDED(hr = m_pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)ppBF)))
      m_clsid = DShowUtil::GetCLSID(*ppBF);
  }
  else if(m_clsid != GUID_NULL)
  {
    IBaseFilter* pBF;
    if(FAILED(CoCreateInstance(m_clsid,NULL,CLSCTX_ALL,__uuidof(pBF), (void**)&pBF)))
      return E_FAIL;
    *ppBF = pBF;
    pBF = NULL;
    hr = S_OK;
  }

  return hr;
};

[uuid("97f7c4d4-547b-4a5f-8332-536430ad2e4d")]
interface IAMFilterData : public IUnknown
{
  STDMETHOD (ParseFilterData) (BYTE* rgbFilterData, ULONG cb, BYTE** prgbRegFilter2) PURE;
  STDMETHOD (CreateFilterData) (REGFILTER2* prf2, BYTE** prgbFilterData, ULONG* pcb) PURE;
};

void CFGFilterRegistry::ExtractFilterData(BYTE* p, UINT len)
{
  IAMFilterData* pFD;
  BYTE* ptr = NULL;
  
  if(SUCCEEDED(CoCreateInstance(CLSID_FilterMapper2,NULL,CLSCTX_ALL,__uuidof(pFD), (void**)&pFD))
  && SUCCEEDED(pFD->ParseFilterData(p, len, (BYTE**)&ptr)))
  {
    REGFILTER2* prf = (REGFILTER2*)*(DWORD*)ptr; // this is f*cked up

    m_merit.mid = prf->dwMerit;

    if(prf->dwVersion == 1)
    {
      for(UINT i = 0; i < prf->cPins; i++)
      {
        if(prf->rgPins[i].bOutput)
          continue;

        for(UINT j = 0; j < prf->rgPins[i].nMediaTypes; j++)
        {
          if(!prf->rgPins[i].lpMediaType[j].clsMajorType || !prf->rgPins[i].lpMediaType[j].clsMinorType)
            break;

          const REGPINTYPES& rpt = prf->rgPins[i].lpMediaType[j];
          AddType(*rpt.clsMajorType, *rpt.clsMinorType);
        }
      }
    }
    else if(prf->dwVersion == 2)
    {
      for(UINT i = 0; i < prf->cPins2; i++)
      {
        if(prf->rgPins2[i].dwFlags&REG_PINFLAG_B_OUTPUT)
          continue;

        for(UINT j = 0; j < prf->rgPins2[i].nMediaTypes; j++)
        {
          if(!prf->rgPins2[i].lpMediaType[j].clsMajorType || !prf->rgPins2[i].lpMediaType[j].clsMinorType)
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
    if(*(DWORD*)p != 0x00000002) return; // only version 2 supported, no samples found for 1
    p += 4;

    ChkLen(4)
    m_merit.mid = *(DWORD*)p; p += 4;

    while (!m_types.empty())
      m_types.pop_back();

    ChkLen(8)
    DWORD nPins = *(DWORD*)p; p += 8;
    while(nPins-- > 0)
    {
      ChkLen(1)
      BYTE n = *p-0x30; p++;
      
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
      while(nTypes-- > 0)
      {
        ChkLen(1)
        BYTE n = *p-0x30; p++;

        ChkLen(2)
        WORD ty = *(WORD*)p; p += 2;
        ASSERT(ty == 'yt');

        ChkLen(5)
        BYTE x33 = *p; p++;
        ASSERT(x33 == 0x33);
        p += 4;

        ChkLen(8)
        if(*(DWORD*)p < (p-base+8) || *(DWORD*)p >= len 
        || *(DWORD*)(p+4) < (p-base+8) || *(DWORD*)(p+4) >= len)
        {
          p += 8;
          continue;
        }

        GUID majortype, subtype;
        memcpy(&majortype, &base[*(DWORD*)p], sizeof(GUID)); p += 4;
        if(!fOutput) AddType(majortype, subtype); 
      }
    }

    #undef ChkLen
  }
}

//
// CFGFilterFile
//

CFGFilterFile::CFGFilterFile(const CLSID& clsid, CStdString path, CStdStringW name, UINT64 merit,CStdString filtername,CStdString filetype)
  : CFGFilter(clsid, name, merit)
  , m_path(path)
  , m_xFileType(filetype)
  , m_xFilterName(filtername)
  , m_hInst(NULL)
  , m_autoload(false)
{
}
HRESULT CFGFilterFile::Create(IBaseFilter** ppBF)
{
  CheckPointer(ppBF, E_POINTER);

  HRESULT hr = E_FAIL;

  hr = DShowUtil::LoadExternalFilter(m_path, m_clsid, ppBF);

  if (FAILED(hr))
	  CLog::Log(LOGERROR,"%s FAILED clsid:%s path:%s",__FUNCTION__,DShowUtil::CStringFromGUID(m_clsid).c_str(),m_path.c_str());

  return hr;
}

void CFGFilterFile::SetAutoLoad(bool autoload)
{
  m_autoload = autoload;
}

//
// CFGFilterVideoRenderer
//

CFGFilterVideoRenderer::CFGFilterVideoRenderer(const CLSID& clsid, CStdStringW name, UINT64 merit) 
  : CFGFilter(clsid, name, merit), pCAP(0)
{
  AddType(MEDIATYPE_Video, MEDIASUBTYPE_NULL);
}

CFGFilterVideoRenderer::~CFGFilterVideoRenderer()
{
	if (pCAP)
	{
		if (m_clsid == __uuidof(CVMR9AllocatorPresenter))
			delete (CVMR9AllocatorPresenter *) pCAP;
		else if (m_clsid == __uuidof(CEVRAllocatorPresenter))
			delete (CEVRAllocatorPresenter *) pCAP;
		else
			delete pCAP;

		pCAP = NULL;
	}
}

HRESULT CFGFilterVideoRenderer::Create(IBaseFilter** ppBF)
{
  CheckPointer(ppBF, E_POINTER);

  HRESULT hr = S_OK;

  CStdString __err;
  if (m_clsid == __uuidof(CVMR9AllocatorPresenter))
    pCAP = new CVMR9AllocatorPresenter(hr,__err);
  if (m_clsid == __uuidof(CEVRAllocatorPresenter))
    pCAP = new CEVRAllocatorPresenter(hr,__err);

  if(pCAP == NULL)
    return E_FAIL;
  IUnknown* pRenderer;
  if(SUCCEEDED(hr = pCAP->CreateRenderer(&pRenderer)))
  {
    IBaseFilter* pBF;
    pBF = (IBaseFilter*)pRenderer;
    *ppBF = pBF;
    pBF = NULL;
    //*ppBF = CComQIPtr<IBaseFilter>(pRenderer).Detach();
    //pUnks.AddTail(pCAP);
  }
  if(!*ppBF) hr = E_FAIL;

  return hr;
}

//
// CFGFilterList
//

CFGFilterList::CFGFilterList()
{
}

CFGFilterList::~CFGFilterList()
{
  RemoveAll();
}

void CFGFilterList::RemoveAll()
{
  for(std::list<filter_t>::iterator it = m_filters.begin() ;it != m_filters.end() ; it++)
  {
    if ((*it).autodelete)
      delete (*it).pFGF;
  }
  while (!m_filters.empty())
    m_filters.pop_back();
  while (!m_sortedfilters.empty())
    m_sortedfilters.pop_back();
}

void CFGFilterList::Insert(CFGFilter* pFGF, int group, bool exactmatch, bool autodelete)
{
  if(CFGFilterRegistry* f1r = dynamic_cast<CFGFilterRegistry*>(pFGF))
  {
    for(std::list<filter_t>::iterator it = m_filters.begin() ;it != m_filters.end() ; it++)
    {
      

      if(group != (*it).group) continue;

      if(CFGFilterRegistry* f2r = dynamic_cast<CFGFilterRegistry*>((*it).pFGF))
      {
        if(f1r->GetMoniker() && f2r->GetMoniker() && S_OK == f1r->GetMoniker()->IsEqual(f2r->GetMoniker())
        || f1r->GetCLSID() != GUID_NULL && f1r->GetCLSID() == f2r->GetCLSID())
        {

          if(autodelete) 
            delete pFGF;
          return;
        }
      }
    }
  }
  for(std::list<filter_t>::iterator it = m_filters.begin() ;it != m_filters.end() ; it++)
  {
    if((*it).pFGF == pFGF)
    {
      if(autodelete) delete pFGF;
      return;
    }
  }

  filter_t f = {m_filters.size(), pFGF, group, exactmatch, autodelete};
  m_filters.push_back(f);

  while (!m_sortedfilters.empty())
    m_sortedfilters.pop_back();
}

std::list<CFGFilter*> CFGFilterList::GetSortedList()
{
  if(m_sortedfilters.empty())
  {
    vector<filter_t> sort;
    
    for (list<filter_t>::iterator it = m_filters.begin() ; it != m_filters.end() ; it++)
      sort.push_back(*it);
    
    qsort(&sort.at(0), sort.size(), sizeof(sort.at(0)), filter_cmp);
    
    for (vector<filter_t>::iterator it = sort.begin() ; it != sort.end() ; it++)
    {
      filter_t ft = *it;
      
      if ( ft.pFGF->GetMerit() >= MERIT64_DO_USE)
        m_sortedfilters.push_back(ft.pFGF);
    }
  }
  return m_sortedfilters;
}

int CFGFilterList::filter_cmp(const void* a, const void* b)
{
  filter_t* fa = (filter_t*)a;
  filter_t* fb = (filter_t*)b;

  if(fa->group < fb->group) return -1;
  if(fa->group > fb->group) return +1;

  if(fa->pFGF->GetCLSID() == fb->pFGF->GetCLSID())
  {
    CFGFilterFile* fgfa = dynamic_cast<CFGFilterFile*>(fa->pFGF);
    CFGFilterFile* fgfb = dynamic_cast<CFGFilterFile*>(fb->pFGF);

    if(fgfa && !fgfb) return -1;
    if(!fgfa && fgfb) return +1;
  }

  if(fa->pFGF->GetMerit() > fb->pFGF->GetMerit()) return -1;
  if(fa->pFGF->GetMerit() < fb->pFGF->GetMerit()) return +1;

  if(fa->exactmatch && !fb->exactmatch) return -1;
  if(!fa->exactmatch && fb->exactmatch) return +1;

  if(fa->index < fb->index) return -1;
  if(fa->index > fb->index) return +1;

  return 0;
}
