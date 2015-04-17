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

CGraphFilters *CGraphFilters::m_pSingleton = NULL;

CGraphFilters::CGraphFilters() :
m_isDVD(false), m_UsingDXVADecoder(false), m_CurrentRenderer(DIRECTSHOW_RENDERER_UNDEF), m_hsubfilter(false)
{
  /*MADVR*/
  m_pMadvr = NULL;
  m_isInitMadVr = false;
  m_isKodiRealFS = false;
  m_swappingDevice = false;
}

CGraphFilters::~CGraphFilters()
{
  if (m_isKodiRealFS)
  {
    CSettings::Get().SetBool("videoscreen.fakefullscreen", false);
    m_isKodiRealFS = false;
  }
}

CGraphFilters* CGraphFilters::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGraphFilters());
}

/*MADVR*/
bool CGraphFilters::UsingMadVr()
{
  if (m_pMadvr == NULL)
    return false;
  return true;

}

bool CGraphFilters::ReadyMadVr()
{
  return ((m_pMadvr != NULL) && CGraphFilters::Get()->GetMadvrCallback()->IsDeviceSet());
}

bool CGraphFilters::IsEnteringExclusiveMadVr()
{
  return ((m_pMadvr != NULL) && CGraphFilters::Get()->GetMadvrCallback()->IsEnteringExclusive());
}
#endif