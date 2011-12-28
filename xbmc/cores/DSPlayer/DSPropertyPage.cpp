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
 
 /*
  * most of the code in the file is from the WINE project. Code took here :
  * http://osdir.com/ml/attachments/txtHF6YX5ZgVc.txt
  *
  */

#ifdef HAS_DS_PLAYER

#include "DSPropertyPage.h"
#include "Utils/log.h"
#include "DSUtil/DSUtil.h"
#include "windowing/WindowingFactory.h"
#include "utils/CharsetConverter.h"
#include "guilib/GraphicContext.h"
#include "settings/GUISettings.h"
#include "Application.h"
#include "settings/Settings.h"

LONG GdiGetCharDimensions(HDC hdc, LPTEXTMETRICW lptm, LONG *height)
{
   SIZE sz;
   static const WCHAR alphabet[] = {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0};

    if(lptm && !GetTextMetricsW(hdc, lptm)) return 0;

    if(!GetTextExtentPointW(hdc, alphabet, 52, &sz)) return 0;

    if (height) *height = sz.cy;
    return (sz.cx / 26 + 1) / 2;
}


CDSPropertyPage::CDSPropertyPage(IBaseFilter* pBF)
: m_pBF(pBF)
{
}

CDSPropertyPage::~CDSPropertyPage()
{
}

bool CDSPropertyPage::Initialize()
{
  Create(true); // autodelete = true
  
  return true;
}

static INT_PTR CALLBACK prop_sheet_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                        LPARAM lparam)
{

    switch(msg)
    {
    case WM_INITDIALOG:
      {
        RECT rect;
        HRESULT result;
        PROPSHEETPAGE *psp = (PROPSHEETPAGE *)lparam; 
        OLEPropertyFrame *opf = (OLEPropertyFrame *)psp->lParam;

        opf->pps->hwnd = hwnd;
        ZeroMemory(&rect, sizeof(rect));
        GetClientRect(hwnd, &rect);
        result = opf->propPage->Activate(hwnd, &rect, TRUE);
        if(result == S_OK) {
            result = opf->propPage->Show(SW_SHOW);
            if(result == S_OK) {
                SetWindowLongPtr(hwnd, DWLP_USER, (LONG)opf);
            }
        }
        BringWindowToTop(hwnd);
        break;
      }
    case WM_DESTROY:
      {
        OLEPropertyFrame *opf = (OLEPropertyFrame *)GetWindowLongPtr(hwnd, DWLP_USER);
        if(opf) {
            opf->propPage->Show(SW_HIDE);
            opf->propPage->Deactivate();
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG)NULL);
        }
        break;
      }
    case WM_NOTIFY:
      {
        OLEPropertyFrame *opf = (OLEPropertyFrame *)GetWindowLongPtr(hwnd, DWLP_USER);
        NMHDR *nmhdr = (NMHDR *)lparam;
        if(! opf)
          break;

        switch(nmhdr->code)
        {
        case PSN_APPLY:
            opf->propPage->Apply();
            return TRUE;
        default:
            return FALSE;
        }
      }
    default:
      break;
    }
    return FALSE;
}

void CDSPropertyPage::Process()
{
  bool wasfullscreen = false;
  if (g_Windowing.IsFullScreen() && !g_guiSettings.GetBool("videoscreen.fakefullscreen"))
  {
    wasfullscreen = true;
    CLog::Log(LOGERROR,"true fullscreen and com property page don't mix so switching to windowed");
    SendMessage(g_Windowing.GetHwnd(), WM_SYSKEYDOWN, VK_RETURN, 0);
	  /*g_graphicsContext.Lock();
	  g_graphicsContext.ToggleFullScreenRoot();
    g_graphicsContext.Unlock();*/
    do 
    {
      Sleep(10);
      if (!g_Windowing.IsFullScreen())
        break;
    }
    while (1);
  }

  HRESULT hr;
  Com::SmartPtr<ISpecifyPropertyPages> pProp = NULL;
  CAUUID pPages;
  if ( SUCCEEDED( m_pBF->QueryInterface(IID_ISpecifyPropertyPages, (void **) &pProp) ) )
  {
    
    pProp->GetPages(&pPages);

    hr = OleCreatePropertyFrame(g_Windowing.GetHwnd(), 0, 0, GetFilterName(m_pBF).c_str(),
      1, (LPUNKNOWN *) &m_pBF, pPages.cElems, 
      pPages.pElems, 0, 0, 0);

    if (SUCCEEDED(hr))
      return;

    CLog::Log(LOGERROR, "%s Failed to show property page (result: 0x%X). Trying a custom way", __FUNCTION__, hr);

    OLEPropertyFrame *opf;
    PROPPAGEINFO pPageInfo;
    PROPSHEETHEADER propSheet;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE *hpsp;
    HFONT font = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    HDC hdc = GetDC(NULL);
    LONG xBaseUnit, yBaseUnit;

    ZeroMemory(&propSheet, sizeof(propSheet));
    
    propSheet.dwSize = sizeof(propSheet);
    propSheet.dwFlags = PSH_PROPTITLE;
    propSheet.hwndParent = g_Windowing.GetHwnd();
    
    CStdString filterName;
    g_charsetConverter.wToUTF8(GetFilterName(m_pBF), filterName);
    propSheet.pszCaption = filterName.c_str();
    
    hpsp = (HPROPSHEETPAGE *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                         pPages.cElems * sizeof(HPROPSHEETPAGE));
    opf = (OLEPropertyFrame *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                         pPages.cElems * sizeof(OLEPropertyFrame));
    
    propSheet.phpage = hpsp;
    
    ZeroMemory(&psp, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DLGINDIRECT | PSP_USETITLE;
    psp.pfnDlgProc = prop_sheet_proc;
    
    /* Calculate average character width fo converting pixels to dialog units */
    font = (HFONT) SelectObject(hdc, font);

    xBaseUnit = GdiGetCharDimensions ( hdc, NULL, &yBaseUnit );

    for(unsigned int page = 0; page < pPages.cElems;  page++)
    {
      opf[page].pps = new CDSPlayerPropertyPageSite(LANG_NEUTRAL);
      hr = LoadExternalPropertyPage(m_pBF, pPages.pElems[page], &opf[page].propPage);
      if (FAILED(hr))
        continue;

      hr = opf[page].propPage->SetPageSite(opf[page].pps);

      if (FAILED(hr))
        continue;

      hr = opf[page].propPage->GetPageInfo(&pPageInfo);
      if (FAILED(hr))
        continue;

      hr = opf[page].propPage->SetObjects(1, (LPUNKNOWN *) &m_pBF);
      if (FAILED(hr))
        continue;

      opf[page].dialog.dlg.cx = MulDiv(pPageInfo.size.cx, 4, xBaseUnit);
      opf[page].dialog.dlg.cy = MulDiv(pPageInfo.size.cy, 8, yBaseUnit);
      
      psp.pResource = (DLGTEMPLATE *)&opf[page].dialog;
      psp.lParam = (LPARAM) &opf[page];
         
      CStdString strTitle = "";
      g_charsetConverter.wToUTF8(pPageInfo.pszTitle, strTitle);
      psp.pszTitle = strTitle.c_str();
      hpsp[propSheet.nPages++] = CreatePropertySheetPage(&psp);
      CoTaskMemFree(pPageInfo.pszTitle);
      if (pPageInfo.pszDocString)
        CoTaskMemFree(pPageInfo.pszHelpFile);
    }

    hr = PropertySheet(&propSheet);

    for(unsigned int page = 0; page < pPages.cElems; page++) 
    {
      if(opf[page].propPage) 
      {
        opf[page].propPage->SetPageSite(NULL);
        opf[page].propPage->Release();
      }
    }
    HeapFree(GetProcessHeap(), 0, hpsp);
    HeapFree(GetProcessHeap(), 0, opf);
    CoTaskMemFree(pPages.pElems);  
    if (wasfullscreen)
      SendMessage(g_Windowing.GetHwnd(), WM_SYSKEYDOWN, VK_RETURN, 0);
  }
}

void CDSPropertyPage::OnExit()
{
}


#endif