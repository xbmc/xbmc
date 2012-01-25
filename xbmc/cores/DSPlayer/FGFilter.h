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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#pragma once

#include "streams.h"
#include <list>
#include "tinyXML\tinyxml.h"

class CFGFilter
{
public:
  enum Type {
    NONE,
    FILE,
    INTERNAL,
    REGISTRY,
    VIDEORENDERER
  };

  CFGFilter(const CLSID& clsid, Type type, CStdString name = L"");
  CFGFilter(Type type) { m_type = type; };
  virtual ~CFGFilter() {};

  CLSID GetCLSID() {return m_clsid;}
  CStdStringW GetName() {return m_name;}
  Type GetType() const { return m_type; }

  void AddType(const GUID& majortype, const GUID& subtype);

  virtual HRESULT Create(IBaseFilter** ppBF) = 0;
protected:
  CLSID m_clsid;
  CStdString m_name;
  Type m_type;
  std::list<GUID> m_types;
};

class CFGFilterRegistry : public CFGFilter
{
protected:
  CStdString m_DisplayName;
  IMoniker* m_pMoniker;

  void ExtractFilterData(BYTE* p, UINT len);

public:
  CFGFilterRegistry(IMoniker* pMoniker);
  CFGFilterRegistry(CStdString DisplayName);
  CFGFilterRegistry(const CLSID& clsid);

  CStdString GetDisplayName() {return m_DisplayName;}
  IMoniker* GetMoniker() {return m_pMoniker;}

  HRESULT Create(IBaseFilter** ppBF);
};

template<class T>
class CFGFilterInternal : public CFGFilter
{
public:
  CFGFilterInternal(CStdStringW name = L"")
    : CFGFilter(__uuidof(T), INTERNAL, name) {}

  HRESULT Create(IBaseFilter** ppBF)
  {
    CheckPointer(ppBF, E_POINTER);

    HRESULT hr = S_OK;
    IBaseFilter* pBF = new T(NULL, &hr);
    if(FAILED(hr)) return hr;

    (*ppBF = pBF)->AddRef();
    pBF = NULL;

    return hr;
  }
};

class CFGFilterFile : public CFGFilter
{
protected:
  CStdString m_path;
  CStdString m_xFileType;
  CStdString m_internalName;
  HINSTANCE m_hInst;
  bool m_isDMO;
  CLSID m_catDMO;

public:
  CFGFilterFile(const CLSID& clsid, CStdString path, CStdStringW name = L"", CStdString filtername = "", CStdString filetype = "");
  CFGFilterFile(TiXmlElement *pFilter);

  HRESULT Create(IBaseFilter** ppBF);
  CStdString GetXFileType() { return m_xFileType; };
  CStdString GetInternalName() { return m_internalName; };
};

interface IDsRenderer;
class CDSGraph;

class CFGFilterVideoRenderer : public CFGFilter
{
public:
  CFGFilterVideoRenderer(const CLSID& clsid, CStdStringW name = L"");
  ~CFGFilterVideoRenderer();

  HRESULT Create(IBaseFilter** ppBF);
};
