/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#pragma once

#include "swighelper.h"
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
     * Addon(id) -- Creates a new Addon class.
     * 
     * id          : string - id of the addon.
     * 
     * *Note, You can use the above as a keyword.
     * 
     * example:
     *  - self.Addon = xbmcaddon.Addon(id='script.recentlyadded')
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
       * Addon(id) -- Creates a new Addon class.
       * 
       * id          : string - id of the addon.
       * 
       * example:
       *  - self.Addon = xbmcaddon.Addon(id='script.recentlyadded')
       */
      Addon(const char* id = NULL) throw (AddonException);

      virtual ~Addon();

      /**
       * getLocalizedString(id) -- Returns an addon's localized 'unicode string'.
       * 
       * id             : integer - id# for string you want to localize.
       * 
       * example:
       *   - locstr = self.Addon.getLocalizedString(id=6)
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
       * *Note, choices are (author, changelog, description, disclaimer, fanart. icon, id, name, path
       *                     profile, stars, summary, type, version)
       * 
       * example:
       *   - version = self.Addon.getAddonInfo('version')
       */
      String getAddonInfo(const char* id) throw (AddonException);

    };
  }
}
