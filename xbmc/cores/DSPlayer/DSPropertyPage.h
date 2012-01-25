#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "streams.h"
#include "threads/Thread.h"
#include "DSUtil/SmartPtr.h"

class CDSPlayerPropertyPageSite: public IPropertyPageSite, public CUnknown
{
public:
  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,__deref_out void **ppv) { return E_NOTIMPL; }

  HRESULT STDMETHODCALLTYPE OnStatusChange(
    /* [in] */ DWORD dwFlags ) {

      if (hwnd == (HWND)NULL)
        return E_UNEXPECTED;
      
      switch ( dwFlags )
      {
        case PROPPAGESTATUS_DIRTY:
          /* dirty */
          SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)(hwnd), 0);
          break;
        case PROPPAGESTATUS_VALIDATE:
          /* validate */
          SendMessage(GetParent(hwnd), PSM_UNCHANGED, (WPARAM)(hwnd), 0);
          break;
        default:
          return E_INVALIDARG;
      }
      
      return S_OK;
  };

  HRESULT STDMETHODCALLTYPE GetLocaleID(
    __RPC__out LCID *pLocaleID) { *pLocaleID = m_lcid; return S_OK; };

  HRESULT STDMETHODCALLTYPE GetPageContainer( 
    /* [out] */ __RPC__deref_out_opt IUnknown **ppUnk) { return E_NOTIMPL; };


  HRESULT STDMETHODCALLTYPE TranslateAccelerator( 
    /* [in] */ __RPC__in MSG *pMsg) { return E_NOTIMPL; };

  CDSPlayerPropertyPageSite(LCID lcid):
    CUnknown("DSPlayer Property Page", NULL),
    m_lcid(lcid),
    hwnd(0)
  { }

  HWND hwnd;

private:
  LCID m_lcid;

};

struct OLEPropertyFrame {
  struct SDialog {
    DLGTEMPLATE dlg;
    WORD menu;
    WORD wclass;
    WORD caption;
  } dialog;
  CDSPlayerPropertyPageSite *pps;
  IPropertyPage  *propPage;
};

class CDSPropertyPage : public CThread
{
public:
  CDSPropertyPage(IBaseFilter* pBF);
  virtual ~CDSPropertyPage();

  virtual bool Initialize();
protected:
  virtual void OnExit();
  virtual void Process();
  IBaseFilter* m_pBF;
};