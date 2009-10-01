#ifndef SCRAPERSETTINGS_H_
#define SCRAPERSETTINGS_H_
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "PluginSettings.h"
#include "StdString.h"

class CScraperUrl;

class CScraperSettings : public CBasicSettings
{
public:
  CScraperSettings();
  virtual ~CScraperSettings();
  bool LoadUserXML(const CStdString& strXML);
  bool LoadSettingsXML(const CStdString& strScraper, const CStdString& strFunction="GetSettings", const CScraperUrl* url=NULL);
  bool Load(const CStdString& strSettings, const CStdString& strSaved);
  CStdString GetSettings() const;
};

struct SScraperInfo
{
  CStdString strTitle;
  CStdString strPath;
  CStdString strThumb;
  CStdString strContent;
  CStdString strLanguage;
  CStdString strFramework;
  CStdString strDate;
  CScraperSettings settings;
  void Reset()
  {
    strTitle.clear();
    strPath.clear();
    strThumb.clear();
    strContent.clear();
    strLanguage.clear();
    settings.Clear();
  }
};

#endif

