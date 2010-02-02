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

// IAMExtendedSeeking
#include <qnetwork.h>

#include "igraphbuilder2.h"
#include "Filters/IMpaDecFilter.h"
#include "Filters/IMPCVideoDecFilter.h"
#include "Filters/IffdshowDecVideo.h"

struct IAMStreamSelectInfos
{
  CStdString name;
  DWORD flags;
  IUnknown *pObj; // Output pin of the splitter
  IUnknown *pUnk; // Intput pin of the filter
  LCID  lcid;
  DWORD group;
};

struct ChapterInfos
{
  CStdString name;
  double time; // in ms
};


class CDSConfig
{
public:
  CDSConfig(void);
  virtual ~CDSConfig(void);
  virtual HRESULT LoadGraph(IFilterGraph2* pGB, IBaseFilter * splitter);
  virtual HRESULT UnloadGraph();

  CStdString GetDxvaMode()  { return m_pStdDxva; };
  std::map<long, IAMStreamSelectInfos *> GetAudioStreams() { return m_pAudioStreams; }
  std::map<long, ChapterInfos *> GetChapters() { return m_pChapters; }
  IAMStreamSelect * GetStreamSelector() { return m_pIAMStreamSelect; }


// AudioStream
  virtual int  GetAudioStreamCount();
  virtual int  GetAudioStream();
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName);

  virtual int  GetSubtitleCount();
  virtual int  GetSubtitle();
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void SetSubtitle(int iStream);

// Chapters
  virtual int  GetChapterCount();
  virtual int  GetChapter();
  virtual void GetChapterName(CStdString& strChapterName);
  void UpdateChapters( __int64 currentTime );
  
// Filters Property Pages
  virtual std::vector<IBaseFilter *> GetFiltersWithPropertyPages() { return m_pPropertiesFilters; };
protected:
  bool LoadStreams();
  bool LoadPropertiesPage(IBaseFilter *pBF);
  bool LoadChapters();
  bool GetMpaDec(IBaseFilter* pBF);
  bool GetMpcVideoDec(IBaseFilter* pBF);
  bool GetffdshowVideo(IBaseFilter* pBF);
  void LoadFilters();
  CCritSec m_pLock;
  
private:
  //Direct Show Filters
  long                           m_lCurrentChapter;
  IFilterGraph2*                 m_pGraphBuilder;
  IMPCVideoDecFilter*         	 m_pIMpcDecFilter;
  IMpaDecFilter*                 m_pIMpaDecFilter;
  IAMStreamSelect*               m_pIAMStreamSelect;
  IAMExtendedSeeking*            m_pIAMExtendedSeeking;
  IBaseFilter*                   m_pSplitter;
  CStdString                     m_pStdDxva;
  std::map<long, IAMStreamSelectInfos *>     m_pAudioStreams;
  std::map<long, IAMStreamSelectInfos *>     m_pEmbedSubtitles;
  std::vector<IBaseFilter *>                 m_pPropertiesFilters;
  std::map<long, ChapterInfos *>             m_pChapters;
};

extern class CDSConfig g_dsconfig;