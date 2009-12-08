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

#ifndef ADDONSETTINGS_H_
#define ADDONSETTINGS_H_

#include "tinyXML/tinyxml.h"
#include "URL.h"
#include "utils/AddonManager.h"
#include "Settings.h"

class CAddonSettings
{
public:
  CAddonSettings() {}
  virtual ~CAddonSettings() {}

  static bool SettingsExist(const CStdString &strPath);

  bool SaveFromDefault(void);
  virtual bool Load(const CURL& url);
  virtual bool Load(const ADDON::IAddon& addon);
  virtual bool Save(void);
  void Clear();

  void Set(const CStdString& key, const CStdString& value);
  CStdString Get(const CStdString& key) const;

  TiXmlElement* GetAddonRoot();

protected:
  TiXmlDocument   m_userXmlDoc;
  TiXmlDocument   m_addonXmlDoc;

private:
  CStdString GetUserDirectory(const CURL& url);

  CStdString      m_id;
  CURL            m_url;
  CStdString      m_userFileName;
};

extern CAddonSettings g_currentAddonSettings;

#endif /* ADDONSETTINGS_H_ */
