/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#ifdef HAS_DS_PLAYER

#include "GraphFilters.h"

CGraphFilters *CGraphFilters::m_pSingleton = NULL;

CGraphFilters::CGraphFilters():
  m_isDVD(false), m_UsingDXVADecoder(false), m_CurrentRenderer(DIRECTSHOW_RENDERER_UNDEF)
{
}

CGraphFilters::~CGraphFilters()
{
  if (Source.pBF)
  {
    delete Source.pData;
    if (Source.isinternal)
      Source.pBF.Release();
    else
      Source.pBF.FullRelease();
  }

  if (Splitter.pBF)
  {
    delete Splitter.pData;
    if (Splitter.isinternal)
      Splitter.pBF.Release();
    else
      Splitter.pBF.FullRelease();
  }

  if (AudioRenderer.pBF)
    AudioRenderer.pBF.FullRelease();

  VideoRenderer.pQualProp = NULL;

  if (VideoRenderer.pBF)
    VideoRenderer.pBF.FullRelease();

  if (Audio.pBF)
  {
    delete Audio.pData;
    if (Audio.isinternal)
      Audio.pBF.Release();
    else
      Audio.pBF.FullRelease();
  }
  
  if (Video.pBF)
  {
    delete Video.pData;
    if (Video.isinternal)
      Video.pBF.Release();
    else
      Video.pBF.FullRelease();
  }
  
  while (! Extras.empty())
  {
    if (Extras.back().pBF)
      Extras.back().pBF.FullRelease();

    Extras.pop_back();
  }
}

CGraphFilters* CGraphFilters::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGraphFilters());
}

#endif