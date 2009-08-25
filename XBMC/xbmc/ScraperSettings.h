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
#include "settings/ISettingsProvider.h"
class CScraperUrl;

class CScraperSettings : ADDON::CAddon
{
public:
  CScraperSettings();
  virtual ~CScraperSettings();
  virtual bool HasSettings() { return false; }
  virtual bool LoadSettings() { return false; }
  virtual void SaveSettings() {}
  virtual void SaveFromDefault() {}
  virtual void UpdateSetting(const CStdString& key, const CStdString& value) {}
  virtual CStdString GetSetting(const CStdString& key) { return ""; }
  virtual TiXmlElement* GetAddonRoot() { return NULL; }
private:
	TiXmlDocument m_addonXmlDoc;
	TiXmlDocument m_userXmlDoc;
};

struct CScraper
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
//     settings.Clear();
  }
};

#endif

