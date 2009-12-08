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
#include "AddonManager.h"
#include "StdString.h"
#include "LocalizeStrings.h"

class CURL;

namespace ADDON
{

// utils
const CStdString    TranslateContent(const CONTENT_TYPE &content, bool pretty=false);
const CONTENT_TYPE  TranslateContent(const CStdString &string);
const CStdString    TranslateType(const TYPE &type);
const TYPE          TranslateType(const CStdString &string);

class CAddon
{
public:
  CAddon(void);
  ~CAddon() {};
  void Set(const AddonProps &props);
  void Reset();

  virtual void Remove() {};
  virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue) { return STATUS_UNKNOWN; };

  /* Add-on language functions */
  static void LoadAddonStrings(const CURL &url);
  static void ClearAddonStrings();

  /* Copy existing add-on and reuse it again with another GUID */
  static bool CreateChildAddon(const CAddon &parent, CAddon &child);



  /* Beginning of Add-on data fields (readed from info.xml) */
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
  CStdString        m_strPath;     ///< Path to the addon

private:
  TYPE              m_type;
  std::set<CONTENT_TYPE> m_content;///< CONTENT_TYPE type identifier(s) this Add-on supports
  CStdString        m_guid;        ///< Unique identifier for this addon, chosen by developer
  CStdString        m_guid_parent; ///< Unique identifier of the parent for this child addon, chosen by developer
  CStdString        m_strName;     ///< Name of the addon, can be chosen freely.
  CStdString        m_strVersion;  ///< Version of the addon, must be in form
  CStdString        m_summary;     ///< Short summary of addon
  CStdString        m_strDesc;     ///< Description of addon
//  CStdString        m_strPath;     ///< Path to the addon
  CStdString        m_strProfile;  ///< Path to the addon's datastore for this profile
  CStdString        m_strLibName;  ///< Name of the library
  CStdString        m_strAuthor;   ///< Author(s) of the addon
  CStdString        m_icon;        ///< Path to icon for the addon, or blank by default
  int               m_stars;       ///< Rating
  CStdString        m_disclaimer;  ///< if exists, user needs to confirm before installation
  bool              m_disabled;    ///< Is this addon disabled?
  int               m_childs;      ///< How many child add-on's are present
  CLocalizeStrings  m_strings;

private:
  static IAddonCallback *m_cbMultitye;
  static IAddonCallback *m_cbViz;
  static IAddonCallback *m_cbSkin;
  static IAddonCallback *m_cbPVR;
  static IAddonCallback *m_cbScript;
  static IAddonCallback *m_cbScraper;
  static IAddonCallback *m_cbScreensaver;
  static IAddonCallback *m_cbPlugin;
  static IAddonCallback *m_cbDSPAudio;
};

}; /* namespace ADDON */

