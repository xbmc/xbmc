#ifndef PLUGINSETTINGS_H_
#define PLUGINSETTINGS_H_
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

#include "tinyXML/tinyxml.h"
#include "URL.h"
#include "Settings.h"

class CBasicSettings
{
public:
  CBasicSettings();
  virtual ~CBasicSettings();

  bool SaveFromDefault(void);
  virtual bool Load(const CURL& url)  { return false; }
  virtual bool Save(void) { return false; }
  void Clear();

  void Set(const CStdString& key, const CStdString& value);
  CStdString Get(const CStdString& key);

  TiXmlElement* GetAddonRoot();
protected:
  TiXmlDocument   m_userXmlDoc;
  TiXmlDocument   m_addonXmlDoc;
};

class CAddonSettings : public CBasicSettings
{
public:
  CAddonSettings();
  virtual ~CAddonSettings();
  bool Load(const CURL& url);
  bool Save(void);
  static bool SettingsExist(const CStdString &strPath);

  CAddonSettings& operator =(const CBasicSettings&);
private:
  CStdString      m_id;
  CURL            m_url;
  CStdString      m_userFileName;
};

extern CAddonSettings g_currentPluginSettings;

#endif
