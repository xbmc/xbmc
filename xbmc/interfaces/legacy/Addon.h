/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "addons/IAddon.h"

#include "AddonString.h"
#include "AddonClass.h"
#include "Exception.h"

namespace XBMCAddon
{
  namespace xbmcaddon
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(AddonException);

    ///
    /// \addtogroup python_xbmcaddon
    /// @{
    /// @brief **Kodi's addon class.**
    ///
    /// Offers classes and functions that manipulate the add-on settings,
    /// information and localization.
    ///
    ///-------------------------------------------------------------------------
    ///
    /// \python_class{ xbmcaddon.Addon([id]) }
    ///
    /// Creates a new AddOn class.
    ///
    /// @param id                    [opt] string - id of the addon as
    ///                              specified in [addon.xml](http://kodi.wiki/view/Addon.xml)
    ///
    /// @note Specifying the addon id is not needed.\n
    /// Important however is that the addon folder has the same name as the AddOn
    /// id provided in [addon.xml](http://kodi.wiki/view/Addon.xml).\n
    /// You can optionally specify the addon id from another installed addon to
    /// retrieve settings from it.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// self.Addon = xbmcaddon.Addon()
    /// self.Addon = xbmcaddon.Addon('script.foo.bar')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    class Addon : public AddonClass
    {
      ADDON::AddonPtr pAddon;

      String getDefaultId();

      String getAddonVersion();

    public:
      Addon(const char* id = NULL);
      virtual ~Addon();

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getLocalizedString(id) }
      ///-----------------------------------------------------------------------
      /// Returns an addon's localized 'unicode string'.
      ///
      /// @param id                      integer - id# for string you want to
      ///                                localize.
      /// @return                        Localized 'unicode string'
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// locstr = self.Addon.getLocalizedString(32000)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getLocalizedString(...);
#else
      String getLocalizedString(int id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getSetting(id) }
      ///-----------------------------------------------------------------------
      /// Returns the value of a setting as a unicode string.
      ///
      /// @param id                      string - id of the setting that the module
      ///                                needs to access.
      /// @return                        Setting as a unicode string
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// apikey = self.Addon.getSetting('apikey')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getSetting(...);
#else
      String getSetting(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).setSetting(id, value) }
      ///-----------------------------------------------------------------------
      /// Sets a script setting.
      ///
      /// @param id                  string - id of the setting that the module needs to access.
      /// @param value               string or unicode - value of the setting.
      ///
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.Addon.setSetting(id='username', value='teamkodi')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setSetting(...);
#else
      void setSetting(const char* id, const String& value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).openSettings() }
      ///-----------------------------------------------------------------------
      /// Opens this scripts settings dialog.
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.Addon.openSettings()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      openSettings();
#else
      void openSettings();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// \anchor python_xbmcaddon_Addon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getAddonInfo(id) }
      ///-----------------------------------------------------------------------
      /// Returns the value of an addon property as a string.
      ///
      /// @param id                      string - id of the property that the
      ///                                module needs to access.
      /// @par Choices for the property are
      /// |             |             |             |             |
      /// |:-----------:|:-----------:|:-----------:|:-----------:|
      /// | author      | changelog   | description | disclaimer  |
      /// | fanart      | icon        | id          | name        |
      /// | path        | profile     | stars       | summary     |
      /// | type        | version     |             |             |
      /// @return                        AddOn property as a string
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// version = self.Addon.getAddonInfo('version')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getAddonInfo(...);
#else
      String getAddonInfo(const char* id);
#endif
    };
    //@}
  }
}
