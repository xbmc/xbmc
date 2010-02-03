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

#include "StdString.h"
#include <streams.h>
#include <map>

#include "igraphbuilder2.h"
#include "Filters/IMpaDecFilter.h"
#include "Filters/IMPCVideoDecFilter.h"
#include "Filters/IffdshowDecVideo.h"

class CDSConfig
{
public:
  CDSConfig(void);
  virtual ~CDSConfig(void);
  virtual HRESULT ConfigureFilters(IFilterGraph2* pGB, IBaseFilter * splitter);

  CStdString GetDxvaMode()  { return m_pStdDxva; };

// Filters Property Pages
  virtual std::vector<IBaseFilter *> GetFiltersWithPropertyPages() { return m_pPropertiesFilters; };
protected:
  bool LoadPropertiesPage(IBaseFilter *pBF);
  void CreatePropertiesXml();
  bool GetMpaDec(IBaseFilter* pBF);
  bool GetMpcVideoDec(IBaseFilter* pBF);
  bool GetffdshowVideo(IBaseFilter* pBF);
  void ConfigureFilters();
  CCritSec m_pLock;
  
private:
  //Direct Show Filters
  IFilterGraph2*                 m_pGraphBuilder;
  IMPCVideoDecFilter*         	 m_pIMpcDecFilter;
  IMpaDecFilter*                 m_pIMpaDecFilter;
  IBaseFilter*                   m_pSplitter;
  CStdString                     m_pStdDxva;
  std::vector<IBaseFilter *>     m_pPropertiesFilters;
};

extern class CDSConfig g_dsconfig;