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
#include "streams.h"
#include "FgFilter.h"
#include "File.h"
#include "FileItem.h"
#include "tinyXML/tinyxml.h"
#include <list>
#include "SystemInfo.h" //g_sysinfo
#include "GUISettings.h"//g_guiSettings

enum DIRECTSHOW_RENDERER
{
    DIRECTSHOW_RENDERER_VMR9 = 1,
    DIRECTSHOW_RENDERER_EVR = 2,
    DIRECTSHOW_RENDERER_UNDEF = 3
};

class CFGLoader : public CCritSec
{
public:
  CFGLoader();
  virtual ~CFGLoader();


  HRESULT    LoadConfig(IFilterGraph2* fg,CStdString configFile);
  HRESULT    LoadFilterRules(const CFileItem& pFileItem);
  HRESULT    InsertSourceFilter(const CFileItem& pFileItem, const CStdString& filterName);
  HRESULT    InsertSplitter(const CStdString& filterName);
  HRESULT    InsertAudioDecoder(const CStdString& filterName);
  HRESULT    InsertVideoDecoder(const CStdString& filterName);
  HRESULT    InsertExtraFilter(const CStdString& filterName);
  HRESULT    InsertAudioRenderer();
  HRESULT    InsertVideoRenderer();
  HRESULT    InsertAutoLoad();

  static DIRECTSHOW_RENDERER GetCurrentRenderer() { return m_CurrentRenderer; }
  
  IBaseFilter* GetSplitter() { return m_SplitterF; };
  IBaseFilter* GetSource() { return m_SourceF; }
  IBaseFilter* GetVideoDec() { return m_VideoDecF; }
  IBaseFilter* GetAudioDec() { return m_AudioDecF; }
  std::vector<IBaseFilter*>& GetExtras() { return m_extraFilters; }
  IBaseFilter* GetAudioRenderer() { return m_AudioRendererF; }
  IBaseFilter* GetVideoRenderer() {return m_VideoRendererF; }

  CStdString GetVideoDecInfo(){return  m_pStrVideodec;};
  CStdString GetAudioDecInfo(){return  m_pStrAudiodec;};
  CStdString GetSourceFilterInfo(){return  m_pStrSource;};
  CStdString GetSplitterFilterInfo(){return  m_pStrSplitter;};
  CStdString GetAudioRendererInfo(){return  m_pStrAudioRenderer;};
protected:
  IFilterGraph2*            m_pGraphBuilder;
  CStdString                m_xbmcConfigFilePath;
  CStdString                m_pStrVideodec;
  CStdString                m_pStrAudiodec;
  CStdString                m_pStrAudioRenderer;
  CStdString                m_pStrSource;
  CStdString                m_pStrSplitter;
  std::list<CFGFilterFile*> m_configFilter;
  XFILE::CFile              m_File;

  IBaseFilter*              m_SourceF;
  IBaseFilter*              m_SplitterF;
  IBaseFilter*              m_VideoDecF;
  IBaseFilter*              m_AudioDecF;
  std::vector<IBaseFilter *> m_extraFilters;
  IBaseFilter*              m_AudioRendererF;
  IBaseFilter*              m_VideoRendererF;

  static DIRECTSHOW_RENDERER m_CurrentRenderer;
  CFGFilterVideoRenderer*   m_pFGF;

};

