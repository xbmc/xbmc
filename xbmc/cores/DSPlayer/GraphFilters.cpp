/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *		Copyright (C) 2010-2013 Eduard Kytmanov
 *		http://www.avmedia.su
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

#ifdef HAS_DS_PLAYER

#include "GraphFilters.h"
#include "settings/Settings.h"
#include "DSPropertyPage.h"
#include "FGFilter.h"
#include "filtercorefactory/filtercorefactory.h"

CGraphFilters *CGraphFilters::m_pSingleton = NULL;

CGraphFilters::CGraphFilters() :
m_isDVD(false), m_UsingDXVADecoder(false), m_CurrentRenderer(DIRECTSHOW_RENDERER_UNDEF), m_hsubfilter(false)
{
  m_isKodiRealFS = false;
}

void CGraphFilters::ShowLavFiltersPage(LAVFILTERS_TYPE type)
{
  std::string filterName;
  if (type == LAVVIDEO)
    filterName = "lavvideo_internal";
  if (type == LAVAUDIO)
    filterName = "lavaudio_internal";
  if (type == LAVSPLITTER)
    filterName = "lavsplitter_internal";
  if (type == XYSUBFILTER)
    filterName = "xysubfilter_internal";

  CFGLoader *pLoader = new CFGLoader();
  pLoader->LoadConfig();

  CFGFilter *pFilter = NULL;
  if (!(pFilter = CFilterCoreFactory::GetFilterFromName(filterName)))
    return;

  IBaseFilter* pBF;
  pFilter->Create(&pBF);
  CDSPropertyPage *pDSPropertyPage = DNew CDSPropertyPage(pBF);
  pDSPropertyPage->Initialize();

  SAFE_DELETE(pLoader);
}

CGraphFilters::~CGraphFilters()
{
  if (m_isKodiRealFS)
  {
    CSettings::GetInstance().SetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN, false);
    m_isKodiRealFS = false;
  }
}

CGraphFilters* CGraphFilters::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGraphFilters());
}
#endif