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

#include "AddonManager.h"
#include "utils/Thread.h"
#include "FileSystem/Directory.h"
#include "../addons/include/xbmc_addon_types.h"
#include <vector>

class CURL;

namespace ADDON
{

  class CAddon
  {
  public:
    CAddon(void);
    ~CAddon() {};

    void Reset();
    bool operator==(const CAddon &rhs) const;

    virtual void Remove() {};

    virtual bool HasSettings() { return false; }
    virtual ADDON_STATUS SetSetting(const char *settingName, const void *settingValue) { return STATUS_UNKNOWN; };

    /* Add-on language functions */
    static void LoadAddonStrings(const CURL &url);
    static void ClearAddonStrings();

    /* Copy existing add-on and reuse it again with another GUID */
    static bool CreateChildAddon(const CAddon &parent, CAddon &child);

    /* Beginning of Add-on data fields (readed from info.xml) */
    CStdString  m_guid;       ///< Unique identifier for this addon, chosen by developer
    CStdString  m_guid_parent;///< Unique identifier of the parent for this child addon, chosen by developer
    CStdString  m_strName;    ///< Name of the addon, can be chosen freely.
    CStdString  m_strVersion; ///< Version of the addon, must be in form
    CStdString  m_summary;    ///< Short summary of addon
    CStdString  m_strDesc;    ///< Description of addon
    CStdString  m_strPath;    ///< Path to the addon
    CStdString  m_strLibName; ///< Name of the library
    CStdString  m_strCreator; ///< Author(s) of the addon
    CStdString  m_icon;       ///< Path to icon for the addon, or blank by default
    int         m_stars;      ///< Rating
    CStdString  m_disclaimer; ///< if exists, user needs to confirm before installation
    bool        m_disabled;   ///< Is this addon disabled?
    AddonType   m_addonType;  ///< Type identifier of this Add-on
    int         m_childs;     ///< How many child add-on's are present
  };

}; /* namespace ADDON */

