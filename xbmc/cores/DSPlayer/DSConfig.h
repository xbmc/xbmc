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

struct IAMStreamSelectInfos
{
  CStdString name;
  DWORD flags;
  IUnknown *pObj;
  IUnknown *pUnk;
  LCID  lcid;
  DWORD group;
};

class CDSConfig
{
public:
  CDSConfig();
  virtual ~CDSConfig();
  virtual HRESULT LoadGraph(IFilterGraph2* pGB);

  CStdString GetDxvaMode()  { return m_pStdDxva; };
  std::map<long, IAMStreamSelectInfos *> GetAudioStreams() { return m_pAudioStreams; }
  IAMStreamSelect * GetStreamSelector() { return m_pIAMStreamSelect; }

//AudioStream
  virtual int  GetAudioStreamCount();
  virtual int  GetAudioStream();
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName);
  virtual void SetAudioStream(int iStream);

  virtual int  GetSubtitleCount();
  virtual int  GetSubtitle();
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void SetSubtitle(int iStream);
  
protected:
  bool LoadAudioStreams(IBaseFilter* pBF);
  bool GetMpaDec(IBaseFilter* pBF);
  bool GetMpcVideoDec(IBaseFilter* pBF);
  bool GetffdshowVideo(IBaseFilter* pBF);
  void LoadFilters();
  CCritSec m_pLock;
  //
  
private:
  //Direct Show Filters
  IFilterGraph2*                 m_pGraphBuilder;
  IMPCVideoDecFilter*         	 m_pIMpcDecFilter;
  IMpaDecFilter*                 m_pIMpaDecFilter;
  IAMStreamSelect*               m_pIAMStreamSelect;
  IBaseFilter*                   m_pSplitter;
  CStdString                     m_pStdDxva;
  std::map<long, IAMStreamSelectInfos *>     m_pAudioStreams;
  std::map<long, IAMStreamSelectInfos *>     m_pEmbedSubtitles;
};
