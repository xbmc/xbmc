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

#include "StdString.h"
#include "FileSystem/Directory.h"
#include <vector>

class CURL;

namespace ADDON
{

enum AddonType
{
  ADDON_UNKNOWN           = -1,
  ADDON_MULTITYPE         = 0,
  ADDON_VIZ               = 1,
  ADDON_SKIN              = 2,
  ADDON_PVRDLL            = 3,
  ADDON_SCRIPT            = 4,
  ADDON_SCRAPER           = 5,
  ADDON_SCREENSAVER       = 6,
  ADDON_PLUGIN_PVR        = 7,
  ADDON_PLUGIN_MUSIC      = 8,
  ADDON_PLUGIN_VIDEO      = 9,
  ADDON_PLUGIN_PROGRAM    = 10,
  ADDON_PLUGIN_PICTURES   = 11
};

const CStdString ADDON_PVRDLL_EXT = "*.pvr";
const CStdString ADDON_GUID_RE = "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";
const CStdString ADDON_VERSION_RE = "(?<Major>\\d*)\\.?(?<Minor>\\d*)?\\.?(?<Build>\\d*)?\\.?(?<Revision>\\d*)?";


/*!
\ingroup windows
\brief Represents an Addon.
\sa VECAddon, IVECADDONS
*/
class CAddon
{
public:
  CAddon();
  virtual ~CAddon() {};
  bool operator==(const CAddon &rhs) const;

  static void LoadAddonStrings(const CURL &url);
  static void ClearAddonStrings();

  CStdString m_guid;       ///< Unique identifier for this addon, chosen by developer
  CStdString m_strName;    ///< Name of the addon, can be chosen freely.
  CStdString m_strVersion; ///< Version of the addon, must be in form 
  CStdString m_summary;    ///< Short summary of addon
  CStdString m_strDesc;    ///< Description of addon
  CStdString m_strPath;    ///< Path to the addon
  CStdString m_strCreator; ///< Author(s) of the addon
  CStdString m_icon;       ///< Path to icon for the addon, or blank by default
  int        m_stars;      ///< Rating
  CStdString m_disclaimer; ///< if exists, user needs to confirm before installation
  bool       m_disabled;   ///< Is this addon disabled?

  AddonType m_addonType;

  
};

/*!
\ingroup windows
\brief A vector to hold CAddon objects.
\sa CAddon, IVECADDONS
*/
typedef std::vector<CAddon> VECADDONS;

/*!
\ingroup windows
\brief Iterator of VECADDONS.
\sa CAddon, VECADDONS
*/
typedef std::vector<CAddon>::iterator IVECADDONS;

}; /* namespace ADDON */