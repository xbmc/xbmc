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

#pragma once
#include <atlcoll.h>
#include "streams.h"
#include "IGraphBuilder2.h"
#include "FgFilter.h"
class CFGLoader
{
public:
  CFGLoader(IGraphBuilder2* gb,CStdString xbmcPath);
  virtual ~CFGLoader();
  HRESULT LoadConfig(CStdString configFile);
  HRESULT LoadFilterRules(CStdString fileType,CComPtr<IBaseFilter> fileSource,IBaseFilter** pBFSplitter);
protected:
  CComPtr<IGraphBuilder2>  m_pGraphBuilder;
  CStdString               m_xbmcPath;
  CStdString               m_xbmcConfigFilePath;
  GUID                     m_mpcVideoDecGuid;
  CAtlList<CFGFilterFile*> m_configFilter;
};

