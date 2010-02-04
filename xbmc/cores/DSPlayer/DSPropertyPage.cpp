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

#include "DSPropertyPage.h"
#include "Utils/log.h"
#include "DShowUtil/DShowUtil.h"
#include "WindowingFactory.h"
CDSPropertyPage::CDSPropertyPage(IBaseFilter* pBF)
: m_pBF(pBF)
{
}

CDSPropertyPage::~CDSPropertyPage()
{
}

bool CDSPropertyPage::Initialize()
{
  Create();
  return true;
}

void CDSPropertyPage::Process()
{
  HRESULT hr;
  ISpecifyPropertyPages *pProp = NULL;
  IBaseFilter *cObjects[1]; 
  cObjects[0] = m_pBF;
  CAUUID pPages;
  CStdStringW Caption = "DSPlayer";
  if ( SUCCEEDED( m_pBF->QueryInterface(IID_ISpecifyPropertyPages, (void **) &pProp) ) )
  {
    
    pProp->GetPages(&pPages);

    hr = OleCreatePropertyFrame(g_Windowing.GetHwnd(), 0, 0, L"DSPlayer",
                                1, (LPUNKNOWN *) &m_pBF, pPages.cElems, 
                                pPages.pElems, 0, 0, 0);


	    SAFE_RELEASE(pProp);
      CoTaskMemFree(pPages.pElems);
      
    }
}

void CDSPropertyPage::OnExit()
{
}