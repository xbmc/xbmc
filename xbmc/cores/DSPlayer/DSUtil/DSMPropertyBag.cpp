#include "StdAfx.h"
#include "DSUtil.h"
#include "DSMPropertyBag.h"

//
// IDSMPropertyBagImpl
//

IDSMPropertyBagImpl::IDSMPropertyBagImpl()
{
}

IDSMPropertyBagImpl::~IDSMPropertyBagImpl()
{
}

// IPropertyBag

STDMETHODIMP IDSMPropertyBagImpl::Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog)
{
  CheckPointer(pVar, E_POINTER);
  if(pVar->vt != VT_EMPTY) return E_INVALIDARG;
  std::map<CStdStringW, CStdStringW>::iterator it = find(pszPropName);
  if (it == end())
    return E_FAIL;
  CStdStringW value = it->second;
  memcpy(pVar, value, sizeof(VARIANT));
  return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::Write(LPCOLESTR pszPropName, VARIANT* pVar)
{
  return SetProperty(pszPropName, pVar);
}

// IPropertyBag2

STDMETHODIMP IDSMPropertyBagImpl::Read(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError)
{
  CheckPointer(pPropBag, E_POINTER);
  CheckPointer(pvarValue, E_POINTER);
  CheckPointer(phrError, E_POINTER);
  for(ULONG i = 0; i < cProperties; phrError[i] = S_OK, i++)
  {
    std::map<CStdStringW, CStdStringW>::iterator it = find(pPropBag[i].pstrName);
    memcpy(pvarValue, it->second, sizeof(VARIANT));
  }
  return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::Write(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue)
{
  CheckPointer(pPropBag, E_POINTER);
  CheckPointer(pvarValue, E_POINTER);
  for(ULONG i = 0; i < cProperties; i++)
    SetProperty(pPropBag[i].pstrName, &pvarValue[i]);
  return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::CountProperties(ULONG* pcProperties)
{
  CheckPointer(pcProperties, E_POINTER);
  *pcProperties = size();
  return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2* pPropBag, ULONG* pcProperties)
{
  CheckPointer(pPropBag, E_POINTER);
  CheckPointer(pcProperties, E_POINTER);
  for(ULONG i = 0; i < cProperties; i++, iProperty++, (*pcProperties)++) 
  {
    std::map<CStdStringW, CStdStringW>::iterator it = begin(); std::advance(it, iProperty);
    CStdStringW key = it->first;
    pPropBag[i].pstrName = (BSTR)CoTaskMemAlloc((key.GetLength()+1)*sizeof(WCHAR));
    if(!pPropBag[i].pstrName) return E_FAIL;
        wcscpy_s(pPropBag[i].pstrName, key.GetLength()+1, key);
  }
  return S_OK;
}

STDMETHODIMP IDSMPropertyBagImpl::LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown* pUnkObject, IErrorLog* pErrLog)
{
  return E_NOTIMPL;
}

// IDSMProperyBag

HRESULT IDSMPropertyBagImpl::SetProperty(LPCWSTR key, LPCWSTR value)
{
  CheckPointer(key, E_POINTER);
  CheckPointer(value, E_POINTER);
  (*this)[key] = value;

  return S_OK;
}

HRESULT IDSMPropertyBagImpl::SetProperty(LPCWSTR key, VARIANT* var)
{
  CheckPointer(key, E_POINTER);
  CheckPointer(var, E_POINTER);
  if((var->vt & (VT_BSTR | VT_BYREF)) != VT_BSTR) return E_INVALIDARG;
  return SetProperty(key, var->bstrVal);
}

HRESULT IDSMPropertyBagImpl::GetProperty(LPCWSTR key, BSTR* value)
{
  CheckPointer(key, E_POINTER);
  CheckPointer(value, E_POINTER);
  if (find(key) == end()) return E_FAIL;
  *value = (*this)[key].AllocSysString();
  return S_OK;
}

HRESULT IDSMPropertyBagImpl::DelAllProperties()
{
  clear();
  return S_OK;
}

HRESULT IDSMPropertyBagImpl::DelProperty(LPCWSTR key)
{
  std::map<CStdStringW, CStdStringW>::iterator it = find(key);
  if (it == end())
    return S_FALSE;

  erase(it);
  
  return S_OK;
}

//
// CDSMResource
//

CCritSec CDSMResource::m_csResources;
std::map<DWORD, CDSMResource*> CDSMResource::m_resources;

CDSMResource::CDSMResource() 
  : mime(_T("application/octet-stream"))
  , tag(0)
{
  CAutoLock cAutoLock(&m_csResources);
  m_resources[(DWORD)this] = this;
}

CDSMResource::CDSMResource(const CDSMResource& r)
{
  *this = r;

  CAutoLock cAutoLock(&m_csResources);
  m_resources[(DWORD)this] = this;
}

CDSMResource::CDSMResource(LPCWSTR name, LPCWSTR desc, LPCWSTR mime, BYTE* pData, int len, DWORD_PTR tag)
{
  this->name = name;
  this->desc = desc;
  this->mime = mime;
  data.resize(len);
  memcpy(&data[0], pData, data.size());
  this->tag = tag;

  CAutoLock cAutoLock(&m_csResources);
  m_resources[(DWORD)this] = this;
}

CDSMResource::~CDSMResource()
{
  CAutoLock cAutoLock(&m_csResources);
  m_resources.erase( m_resources.find((DWORD)this) );
}

void CDSMResource::operator = (const CDSMResource& r)
{
  tag = r.tag;
  name = r.name;
  desc = r.desc;
  mime = r.mime;
  data = r.data;
}

//
// IDSMResourceBagImpl
//

IDSMResourceBagImpl::IDSMResourceBagImpl()
{
}

// IDSMResourceBag

STDMETHODIMP_(DWORD) IDSMResourceBagImpl::ResGetCount()
{
  return m_resources.size();
}

STDMETHODIMP IDSMResourceBagImpl::ResGet(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag)
{
  if(ppData) CheckPointer(pDataLen, E_POINTER);

  if((INT_PTR)iIndex >= m_resources.size())
    return E_INVALIDARG;

  CDSMResource& r = m_resources[iIndex];

  if(ppName) *ppName = r.name.AllocSysString();
  if(ppDesc) *ppDesc = r.desc.AllocSysString();
  if(ppMime) *ppMime = r.mime.AllocSysString();
  if(ppData) {*pDataLen = r.data.size(); memcpy(*ppData = (BYTE*)CoTaskMemAlloc(*pDataLen), &r.data[0], *pDataLen);}
  if(pTag) *pTag = r.tag;

  return S_OK;
}

STDMETHODIMP IDSMResourceBagImpl::ResSet(DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag)
{
  if((INT_PTR)iIndex >= m_resources.size())
    return E_INVALIDARG;

  CDSMResource& r = m_resources[iIndex];

  if(pName) r.name = pName;
  if(pDesc) r.desc = pDesc;
  if(pMime) r.mime = pMime;
  if(pData || len == 0) {r.data.resize(len); if(pData) memcpy(&r.data[0], pData, r.data.size());}
  r.tag = tag;

  return S_OK;
}

STDMETHODIMP IDSMResourceBagImpl::ResAppend(LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag)
{
  m_resources.push_back(CDSMResource());
  return ResSet(m_resources.size() - 1, pName, pDesc, pMime, pData, len, tag);
}

STDMETHODIMP IDSMResourceBagImpl::ResRemoveAt(DWORD iIndex)
{
  if((INT_PTR)iIndex >= m_resources.size())
    return E_INVALIDARG;

  m_resources.erase(m_resources.begin() + iIndex);

  return S_OK;
}

STDMETHODIMP IDSMResourceBagImpl::ResRemoveAll(DWORD_PTR tag)
{
  if(tag)
  {
    for(int i = m_resources.size() - 1; i >= 0; i--)
      if(m_resources[i].tag == tag)
        m_resources.erase(m_resources.begin() + i);
  }
  else
  {
    m_resources.clear();
  }

  return S_OK;
}

//
// CDSMChapter
//

CDSMChapter::CDSMChapter()
{
  order = counter++;
  rt = 0;
}

CDSMChapter::CDSMChapter(REFERENCE_TIME rt, LPCWSTR name)
{
  order = counter++;
  this->rt = rt;
  this->name = name;
}

void CDSMChapter::operator = (const CDSMChapter& c)
{
  order = c.counter;
  rt = c.rt;
  name = c.name;
}

int CDSMChapter::counter = 0;

int CDSMChapter::Compare(CDSMChapter a, CDSMChapter b)
{

  if(a.rt > b.rt) return 1;
  else if(a.rt < b.rt) return -1;

  return a.order - b.order;
}

//
// IDSMChapterBagImpl
//

IDSMChapterBagImpl::IDSMChapterBagImpl()
{
  m_fSorted = false;
}

// IDSMRChapterBag

STDMETHODIMP_(DWORD) IDSMChapterBagImpl::ChapGetCount()
{
  return m_chapters.size();
}

STDMETHODIMP IDSMChapterBagImpl::ChapGet(DWORD iIndex, REFERENCE_TIME* prt, BSTR* ppName)
{
  if((INT_PTR)iIndex >= m_chapters.size())
    return E_INVALIDARG;

  CDSMChapter& c = m_chapters[iIndex];

  if(prt) *prt = c.rt;
  if(ppName) *ppName = c.name.AllocSysString();

  return S_OK;
}

STDMETHODIMP IDSMChapterBagImpl::ChapSet(DWORD iIndex, REFERENCE_TIME rt, LPCWSTR pName)
{
  if((INT_PTR)iIndex >= m_chapters.size())
    return E_INVALIDARG;

  CDSMChapter& c = m_chapters[iIndex];

  c.rt = rt;
  if(pName) c.name = pName;

  m_fSorted = false;

  return S_OK;
}

STDMETHODIMP IDSMChapterBagImpl::ChapAppend(REFERENCE_TIME rt, LPCWSTR pName)
{
  m_chapters.push_back(CDSMChapter());
  return ChapSet(m_chapters.size() - 1, rt, pName);
}

STDMETHODIMP IDSMChapterBagImpl::ChapRemoveAt(DWORD iIndex)
{
  if((INT_PTR)iIndex >= m_chapters.size())
    return E_INVALIDARG;

  m_chapters.erase(m_chapters.begin() + iIndex);

  return S_OK;
}

STDMETHODIMP IDSMChapterBagImpl::ChapRemoveAll()
{
  m_chapters.clear();

  m_fSorted = false;

  return S_OK;
}

STDMETHODIMP_(long) IDSMChapterBagImpl::ChapLookup(REFERENCE_TIME* prt, BSTR* ppName)
{
  CheckPointer(prt, -1);

  ChapSort();

  int i = range_bsearch(m_chapters, *prt);
  if(i < 0) return -1;

  *prt = m_chapters[i].rt;
  if(ppName) *ppName = m_chapters[i].name.AllocSysString();

  return i;
}

STDMETHODIMP IDSMChapterBagImpl::ChapSort()
{
  if(m_fSorted) return S_FALSE;
  std::sort(m_chapters.begin(), m_chapters.end(), CDSMChapter::Compare);
  m_fSorted = true;
  return S_OK;
}

//
// CDSMChapterBag
//

CDSMChapterBag::CDSMChapterBag(LPUNKNOWN pUnk, HRESULT* phr) 
  : CUnknown(_T("CDSMChapterBag"), NULL)
{
}

STDMETHODIMP CDSMChapterBag::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

  return
    QI(IDSMChapterBag)
     __super::NonDelegatingQueryInterface(riid, ppv);
}
