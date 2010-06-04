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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include <list>
#include "FgFilter.h"
#include "FileItem.h"
#include "DShowUtil/smartptr.h"

enum DIRECTSHOW_RENDERER
{
    DIRECTSHOW_RENDERER_VMR9 = 1,
    DIRECTSHOW_RENDERER_EVR = 2,
    DIRECTSHOW_RENDERER_UNDEF = 3
};

struct SFilterInfos
{
  SFilterInfos()
  {
    Clear();
  }
  
  void Clear()
  {
    pBF = NULL;
    osdname = "";
    guid = GUID_NULL;
  }

  Com::SmartPtr<IBaseFilter> pBF;
  CStdString osdname;
  GUID guid;
};

struct SVideoRendererFilterInfos: SFilterInfos
{
  SVideoRendererFilterInfos()
    : SFilterInfos()
  {
    Clear();
  }

  void Clear()
  {
    pQualProp = NULL;
    __super::Clear();
  }
  Com::SmartPtr<IQualProp> pQualProp;
};

struct SDVDFilters
{
  SDVDFilters()
  {
    Clear();
  }

  void Clear()
  {
    dvdControl.Release();
    dvdInfo.Release();
  }

  Com::SmartQIPtr<IDvdControl2> dvdControl;
  Com::SmartQIPtr<IDvdInfo2> dvdInfo;
};


struct SFilters
{
  SFilterInfos Source;
  SFilterInfos Splitter;
  SFilterInfos Video;
  SFilterInfos Audio;
  SFilterInfos AudioRenderer;
  SVideoRendererFilterInfos VideoRenderer;
  std::vector<SFilterInfos> Extras;
  SDVDFilters DVD;
  bool isDVD;

  void Clear()
  {
    isDVD = false;
    DVD.Clear();
    Source.Clear();
    Splitter.Clear();
    Video.Clear();
    AudioRenderer.Clear();
    Audio.Clear();
    VideoRenderer.Clear();
    for (std::vector<SFilterInfos>::iterator it = Extras.begin();
      it != Extras.end(); ++it)
      (*it).Clear();
  }

  SFilters()
  {
    Clear();
  }
};

enum ESettingsType
{
  MEDIAS,
  FILTERS
};

class CFGLoader : public CCriticalSection
{
public:
  CFGLoader();
  virtual ~CFGLoader();

  static SFilters Filters;

  HRESULT    LoadConfig();
  bool       LoadFilterCoreFactorySettings(const CStdString& item, ESettingsType type, bool clear);

  HRESULT    LoadFilterRules(const CFileItem& pFileItem);
  HRESULT    InsertSourceFilter(const CFileItem& pFileItem, const CStdString& filterName);
  HRESULT    InsertSplitter(const CFileItem& pFileItem, const CStdString& filterName);
  HRESULT    InsertAudioRenderer(const CStdString& filterName);
  HRESULT    InsertVideoRenderer();
  HRESULT    InsertAutoLoad();
  HRESULT    InsertFilter(const CStdString& filterName, SFilterInfos& f);

  static DIRECTSHOW_RENDERER GetCurrentRenderer() { return m_CurrentRenderer; }
  static bool         IsUsingDXVADecoder() { return m_UsingDXVADecoder; }

protected:
  CStdString                m_xbmcConfigFilePath;

  static bool               m_UsingDXVADecoder;

  static DIRECTSHOW_RENDERER m_CurrentRenderer;
  CFGFilterVideoRenderer*   m_pFGF;

};

