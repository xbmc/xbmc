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

#include "IAddon.h"
#include "../addons/include/libaddon.h"
#include "FileSystem/PluginDirectory.h"
#include "tinyXML/tinyxml.h"
#include "Util.h"
#include "URL.h"
#include "LocalizeStrings.h"
#include "boost/shared_ptr.hpp"

class CURL;
class TiXmlElement;

namespace ADDON
{

// utils  
const CStdString    TranslateContent(const CONTENT_TYPE &content, bool pretty=false);
const CONTENT_TYPE  TranslateContent(const CStdString &string);
const CStdString    TranslateType(const TYPE &type, bool pretty=false);
const TYPE          TranslateType(const CStdString &string);

class CAddon : public IAddon
{
public:
  CAddon(const AddonProps &props);
  virtual ~CAddon() {}
  virtual AddonPtr Clone() const;

  // settings & language
  virtual bool HasSettings();
  virtual bool LoadSettings();
  virtual void SaveSettings();
  virtual void SaveFromDefault();
  virtual void UpdateSetting(const CStdString& key, const CStdString& value, const CStdString &type = "");
  virtual CStdString GetSetting(const CStdString& key) const;
  TiXmlElement* GetSettingsXML();
  virtual CStdString GetString(uint32_t id) const;

  /* Beginning of Add-on data fields (read from description.xml) */
  TYPE Type() const { return m_type; }
  CStdString UUID() const { return m_guid; }
  CStdString Parent() const { return m_guid_parent; }
  CStdString Name() const { return m_strName; }
  bool Disabled() const { return m_disabled; }
  CStdString Version() const { return m_strVersion; }
  CStdString Summary() const { return m_summary; }
  CStdString Description() const { return m_strDesc; }
  CStdString Path() const { return m_strPath; }
  CStdString Profile() const { return m_strProfile; }
  CStdString LibName() const { return m_strLibName; }
  CStdString Author() const { return m_strAuthor; }
  CStdString Icon() const { return m_icon; }
  int  Stars() const { return m_stars; }
  CStdString Disclaimer() const { return m_disclaimer; }
  bool Supports(const CONTENT_TYPE &content) const { return (m_content.count(content) == 1); }

protected:
  CAddon(const CAddon&); // protected as all copying is handled by Clone()
  bool LoadUserSettings();
  TiXmlDocument     m_addonXmlDoc;
  TiXmlDocument     m_userXmlDoc;
  CStdString        m_userSettingsPath;

private:
  CStdString GetProfilePath();
  CStdString GetUserSettingsPath();
  void Enable() { LoadStrings(); m_disabled = false; }
  void Disable() { m_disabled = true; ClearStrings();}
  virtual bool LoadStrings();
  virtual void ClearStrings();
  void BuildIconPath();
  void BuildLibName();
  const TYPE m_type; 
  const std::set<CONTENT_TYPE> m_content;     ///< CONTENT_TYPE type identifier(s) this Add-on supports
  const CStdString  m_guid;        ///< Unique identifier for this addon, chosen by developer
  const CStdString  m_guid_parent; ///< Unique identifier of the parent for this child addon, chosen by developer
  CStdString  m_strName;     ///< Name of the addon, can be chosen freely.
  CStdString  m_strVersion;  ///< Version of the addon, must be in form
  CStdString  m_summary;     ///< Short summary of addon
  CStdString  m_strDesc;     ///< Description of addon
  CStdString  m_strPath;     ///< Path to the addon
  CStdString  m_strProfile;  ///< Path to the addon's datastore for this profile
  CStdString  m_strLibName;  ///< Name of the library
  CStdString  m_strAuthor;   ///< Author(s) of the addon
  CStdString  m_icon;        ///< Path to icon for the addon, or blank by default
  int         m_stars;       ///< Rating
  CStdString  m_disclaimer;  ///< if exists, user needs to confirm before installation
  bool        m_disabled;    ///< Is this addon disabled?
  int         m_childs;      ///< How many child add-on's are present
  CLocalizeStrings  m_strings;
};

//template<class Derived>
//AddonPtr CAddon::Clone() const
//{
//  AddonPtr addon(new Derived(dynamic_cast<Derived&>(*this)));
//  return addon;
//}

}; /* namespace ADDON */

