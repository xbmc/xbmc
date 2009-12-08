/* 
 *	Copyright (C) 2003-2006 Gabest
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

#pragma once
#include <atlcomcli.h>
#include <atlcoll.h>
#include "streams.h"
#define MERIT64(merit) (((UINT64)(merit))<<16)
#define MERIT64_DO_NOT_USE MERIT64(MERIT_DO_NOT_USE)
#define MERIT64_DO_USE MERIT64(MERIT_DO_NOT_USE+1)
#define MERIT64_UNLIKELY (MERIT64(MERIT_UNLIKELY))
#define MERIT64_NORMAL (MERIT64(MERIT_NORMAL))
#define MERIT64_PREFERRED (MERIT64(MERIT_PREFERRED))
#define MERIT64_ABOVE_DSHOW (MERIT64(1)<<32)

class CFGFilter
{
protected:
	CLSID m_clsid;
	CStdString m_name;
	struct {union {UINT64 val; struct {UINT64 low:16, mid:32, high:16;};};} m_merit;
	CAtlList<GUID> m_types;

public:
	CFGFilter(const CLSID& clsid, CStdString name = L"", UINT64 merit = MERIT64_DO_USE);
	virtual ~CFGFilter() {}

	CLSID GetCLSID() {return m_clsid;}
	CStdStringW GetName() {return m_name;}
	UINT64 GetMerit() {return m_merit.val;}
	DWORD GetMeritForDirectShow() {return m_merit.mid;}
	const CAtlList<GUID>& GetTypes() const;
	void SetTypes(const CAtlList<GUID>& types);
	void AddType(const GUID& majortype, const GUID& subtype);
	bool CheckTypes(const CAtlArray<GUID>& types, bool fExactMatch);

	CAtlList<CStdString> m_protocols, m_extensions, m_chkbytes; // TODO: subtype?

	virtual HRESULT Create(IBaseFilter** ppBF, CInterfaceList<IUnknown, &IID_IUnknown>& pUnks) = 0;
};

class CFGFilterRegistry : public CFGFilter
{
protected:
	CStdString m_DisplayName;
	CComPtr<IMoniker> m_pMoniker;

	void ExtractFilterData(BYTE* p, UINT len);

public:
	CFGFilterRegistry(IMoniker* pMoniker, UINT64 merit = MERIT64_DO_USE);
	CFGFilterRegistry(CStdString DisplayName, UINT64 merit = MERIT64_DO_USE);
	CFGFilterRegistry(const CLSID& clsid, UINT64 merit = MERIT64_DO_USE);

	CStdString GetDisplayName() {return m_DisplayName;}
	IMoniker* GetMoniker() {return m_pMoniker;}

	HRESULT Create(IBaseFilter** ppBF, CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
};

template<class T>
class CFGFilterInternal : public CFGFilter
{
public:
	CFGFilterInternal(CStdStringW name = L"", UINT64 merit = MERIT64_DO_USE) : CFGFilter(__uuidof(T), name, merit) {}

	HRESULT Create(IBaseFilter** ppBF, CInterfaceList<IUnknown, &IID_IUnknown>& pUnks)
	{
		CheckPointer(ppBF, E_POINTER);

		HRESULT hr = S_OK;
		CComPtr<IBaseFilter> pBF = new T(NULL, &hr);
		if(FAILED(hr)) return hr;

		*ppBF = pBF.Detach();

		return hr;
	}
};

class CFGFilterFile : public CFGFilter
{
protected:
  CStdString m_path;
  CStdString m_xFileType;
  CStdString m_xFilterName;
  HINSTANCE m_hInst;
  bool      m_autoload;
public:
  CFGFilterFile(const CLSID& clsid, CStdString path, CStdStringW name = L"", UINT64 merit = MERIT64_DO_USE,CStdString filtername = "",CStdString filetype = "");

  HRESULT Create(IBaseFilter** ppBF, CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
  CStdString GetXFileType() { return m_xFileType; };
  CStdString GetXFilterName() { return m_xFilterName; };
  bool GetAutoLoad() { return m_autoload; };
  void SetAutoLoad(bool autoload);
  
};

class CFGFilterVideoRenderer : public CFGFilter
{
public:
	CFGFilterVideoRenderer(const CLSID& clsid, CStdStringW name = L"", UINT64 merit = MERIT64_DO_USE);

	HRESULT Create(IBaseFilter** ppBF, CInterfaceList<IUnknown, &IID_IUnknown>& pUnks);
};

class CFGFilterList
{
	struct filter_t {int index; CFGFilter* pFGF; int group; bool exactmatch, autodelete;};
	static int filter_cmp(const void* a, const void* b);
	CAtlList<filter_t> m_filters;
	CAtlList<CFGFilter*> m_sortedfilters;

public:
	CFGFilterList();
	virtual ~CFGFilterList();

	void RemoveAll();
	void Insert(CFGFilter* pFGF, int group, bool exactmatch = false, bool autodelete = true);

	POSITION GetHeadPosition();
	CFGFilter* GetNext(POSITION& pos);
};
