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
#include "smartptr.h"
#include "igraphbuilder2.h"
#include "Filters/IMpaDecFilter.h"
#include "Filters/IMPCVideoDecFilter.h"
#include "Filters/IffdshowDecVideo.h"


class CDSConfig
{
public:
  CDSConfig();
  virtual ~CDSConfig();
  virtual HRESULT LoadGraph(SmartPtr<IGraphBuilder2> pGB);

  CStdString GetDxvaMode()  { return m_pStdDxva; };
//AudioStream
  virtual int  GetAudioStreamCount();
  virtual int  GetAudioStream();
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName);
  virtual void SetAudioStream(int iStream);
  
protected:
  bool GetStreamSelector(SmartPtr<IBaseFilter> pBF);
  bool GetMpaDec(SmartPtr<IBaseFilter> pBF);
  bool GetMpcVideoDec(SmartPtr<IBaseFilter> pBF);
  bool GetffdshowVideo(SmartPtr<IBaseFilter> pBF);
  void LoadFilters();
  CCritSec m_pLock;
  //
  
private:
  //Direct Show Filters
  SmartPtr<IGraphBuilder2>        m_pGraphBuilder;
  SmartPtr<IMPCVideoDecFilter>	 m_pIMpcDecFilter;//CComQIPtr
  //No guid defined for ffdshow video
  //SmartPtr<IffdshowDecVideoA>	 m_pIffdDecFilter;
  SmartPtr<IMpaDecFilter>       m_pIMpaDecFilter;//CComQIPtr
  SmartPtr<IAMStreamSelect>     m_pIAMStreamSelect;//CComQIPtr
  CStdString                     m_pStdDxva;
};
