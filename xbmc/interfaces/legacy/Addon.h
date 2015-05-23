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

    /**
     * Addon class.
     * 
     * Addon([id]) -- Creates a new Addon class.
     * 
     * id          : [opt] string - id of the addon as specified in addon.xml
     * 
     * *Note, specifying the addon id is not needed.\n
     *  Important however is that the addon folder has the same name as the addon id provided in addon.xml.\n
     *  You can optionally specify the addon id from another installed addon to retrieve settings from it.
     * 
     * example:
     *  - self.Addon = xbmcaddon.Addon()
     *  - self.Addon = xbmcaddon.Addon('script.foo.bar')
     */
    class Addon : public AddonClass
    {
      ADDON::AddonPtr pAddon;

      String getDefaultId();

      String getAddonVersion();

    public:
      /**
       * Addon class.
       * 
       * Addon([id]) -- Creates a new Addon class.
       * 
       * id          : [opt] string - id of the addon as specified in addon.xml\n
       * 
       * *Note, specifying the addon id is not needed.\n
       *  Important however is that the addon folder has the same name as the addon id provided in addon.xml.\n
       *  You can optionally specify the addon id from another installed addon to retrieve settings from it.
       * 
       * example:
       *  - self.Addon = xbmcaddon.Addon()
       *  - self.Addon = xbmcaddon.Addon('script.foo.bar')
       */
      Addon(const char* id = NULL);

      virtual ~Addon();

      /**
       * getLocalizedString(id) -- Returns an addon's localized 'unicode string'.
       * 
       * id        : integer - id# for string you want to localize.
       * 
       * example:
       *   - locstr = self.Addon.getLocalizedString(32000)
       */
      String getLocalizedString(int id);

      /**
       * getSetting(id) -- Returns the value of a setting as a unicode string.
       * 
       * id        : string - id of the setting that the module needs to access.
       * 
       * example:
       *   - apikey = self.Addon.getSetting('apikey')
       */
      String getSetting(const char* id);

      /**
       * setSetting(id, value) -- Sets a script setting.
       * 
       * id        : string - id of the setting that the module needs to access.
       * value     : string or unicode - value of the setting.
       * 
       * *Note, You can use the above as keywords for arguments.
       * 
       * example:
       *   - self.Settings.setSetting(id='username', value='teamxbmc')\n
       */
      void setSetting(const char* id, const String& value);

      /**
       * openSettings() -- Opens this scripts settings dialog.
       * 
       * example:
       *   - self.Settings.openSettings()
       */
      void openSettings();

      /**
       * getAddonInfo(id) -- Returns the value of an addon property as a string.
       * 
       * id        : string - id of the property that the module needs to access.
       * 
       * *Note, choices are (author, changelog, description, disclaimer, fanart. icon, id, name, path,\n
       *                     profile, stars, summary, type, version)
       * 
       * example:
       *   - version = self.Addon.getAddonInfo('version')
       */
      String getAddonInfo(const char* id);
    };
  }
}
