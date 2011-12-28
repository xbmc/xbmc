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

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#define _DSPLAYER_DEBUG

#include "FGFilter.h"
#include "DSConfig.h"
#include "Filters/IffdshowDecVideo.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "FGLoader.h"

#include <list>

typedef std::list<CFGFilter*> FilterList;
typedef std::list<CFGFilter*>::iterator FilterListIter;
typedef std::list<CMediaType> MediaTypeList;
typedef std::list<CMediaType>::iterator MediaTypeListIter;

struct path_t {CLSID clsid; CStdStringW filter, pin;};
typedef std::list<path_t> PathList;
typedef std::list<path_t>::iterator PathListIter;
typedef std::list<path_t>::const_iterator PathListConstIter;

using namespace XFILE;


class CFGLoader;

class CFGManager:
  public CCriticalSection
{
public:
  
  class CStreamPath : public std::list<path_t> 
  {
  public: 
    void Append(IBaseFilter* pBF, IPin* pPin); 
    bool Compare(const CStreamPath& path);
  };

  class CStreamDeadEnd : public CStreamPath 
  {
  public: 
    std::list<CMediaType> mts;
  };
  
private:
  DWORD m_dwRegister;
  CStreamPath m_streampath;
  std::vector<CStreamDeadEnd> m_deadends;

protected:
  Com::SmartPtr<IFilterMapper2> m_pFM;
  Com::SmartPtr<IUnknown> m_pUnkInner;
  Com::SmartPtr<IDvdGraphBuilder> m_pDvdGraph;

  bool m_audioPinConnected;
  bool m_videoPinConnected;

  //std::list<CFGFilter*> m_source, m_transform, m_override;

  virtual HRESULT CreateFilter(CFGFilter* pFGF, IBaseFilter** ppBF);

  CFile m_File;
  CFGLoader* m_CfgLoader;

  CStdString m_xbmcConfigFilePath;

  // IFilterGraph
  STDMETHODIMP AddFilter(IBaseFilter* pFilter, LPCWSTR pName);
  STDMETHODIMP RemoveFilter(IBaseFilter* pFilter);
  STDMETHODIMP EnumFilters(IEnumFilters** ppEnum);
  STDMETHODIMP FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter);
  STDMETHODIMP Reconnect(IPin* ppin);
  STDMETHODIMP SetDefaultSyncSource();

  // IGraphBuilder
  STDMETHODIMP Connect(IPin* pPinOut, IPin* pPinIn);
  STDMETHODIMP Render(IPin* pPinOut);
  STDMETHODIMP RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList) { return E_NOTIMPL; };
  STDMETHODIMP AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter) { return E_NOTIMPL; };
  STDMETHODIMP SetLogFile(DWORD_PTR hFile) {return E_NOTIMPL; };
  STDMETHODIMP Abort();
  STDMETHODIMP ShouldOperationContinue();

  // IFilterGraph2
  STDMETHODIMP AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);
  STDMETHODIMP ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt);
  STDMETHODIMP RenderEx(IPin* pPinOut, DWORD dwFlags, DWORD* pvContext) { return E_NOTIMPL; };

  // IGraphBuilderDeadEnd

  STDMETHODIMP_(size_t) GetCount();
  STDMETHODIMP GetDeadEnd(int iIndex, std::list<CStdStringW>& path, std::list<CMediaType>& mts);
  //CfgManagerCustom
  UINT64 m_vrmerit, m_armerit;
  // IFilterGraph
public:
  CFGManager();
  virtual ~CFGManager();
  void InitManager();
  void UpdateRegistry();
  void ForceStableCodecs();
#ifdef _DSPLAYER_DEBUG
  void LogFilterGraph(void);
#endif

  STDMETHODIMP ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt);
  STDMETHODIMP Disconnect(IPin* ppin);
  
  // IGraphBuilder2
  HRESULT IsPinDirection(IPin* pPin, PIN_DIRECTION dir);
  HRESULT IsPinConnected(IPin* pPin);
  HRESULT ConnectFilter(IBaseFilter* pBF, IPin* pPinIn);
  HRESULT ConnectFilter(IPin* pPinOut, IBaseFilter* pBF);
  HRESULT ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt);
  //HRESULT NukeDownstream(IUnknown* pUnk);
  HRESULT AddToROT();
  HRESULT RemoveFromROT();
  HRESULT RenderFileXbmc(const CFileItem& pFileItem);

  HRESULT QueryInterface(REFIID iid , void** ppv);

  HRESULT RecoverFromGraphError(const CFileItem& pFileItem);
};