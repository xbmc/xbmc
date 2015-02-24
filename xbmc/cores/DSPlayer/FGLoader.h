/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif
#include "GraphFilters.h"
#include "FgFilter.h"
#include "FileItem.h"
#include "DSUtil/SmartPtr.h"
#include "Utils/StdString.h"

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

  HRESULT    LoadConfig();
  bool	     IsAutoRender(){ return m_bIsAutoRender; };
  bool       LoadFilterCoreFactorySettings(const CStdString& item, ESettingsType type, bool clear);

  HRESULT    LoadFilterRules(const CFileItem& pFileItem);
  HRESULT    InsertSourceFilter(CFileItem& pFileItem, const CStdString& filterName);
  HRESULT    InsertSplitter(const CFileItem& pFileItem, const CStdString& filterName);
  HRESULT    InsertAudioRenderer(const CStdString& filterName);
  HRESULT    InsertVideoRenderer();
  HRESULT    InsertAutoLoad();
  HRESULT    InsertFilter(const CStdString& filterName, SFilterInfos& f);

  static void SettingOptionsDSAudioRendererFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
protected:
  CStdString                m_xbmcConfigFilePath;
  CFGFilterVideoRenderer*   m_pFGF;
  bool					    m_bIsAutoRender;

private:
  void      ParseStreamingType(CFileItem& pFileItem, IBaseFilter* pBF);
};

