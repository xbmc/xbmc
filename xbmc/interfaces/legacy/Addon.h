/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
    /// @python_v13
    /// **id** is optional as it will be auto detected for this add-on instance.
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

      bool UpdateSettingInActiveDialog(const char* id, const String& value);

    public:
      explicit Addon(const char* id = NULL);
      ~Addon() override;

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
      /// @python_v13
      /// **id** is optional as it will be auto detected for this add-on instance.
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
      /// @python_v13
      /// **id** is optional as it will be auto detected for this add-on instance.
      ///
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
      /// @brief \python_func{ xbmcaddon.Addon([id]).getSettingBool(id) }
      ///-----------------------------------------------------------------------
      /// Returns the value of a setting as a boolean.
      ///
      /// @param id                      string - id of the setting that the module
      ///                                needs to access.
      /// @return                        Setting as a boolean
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// enabled = self.Addon.getSettingBool('enabled')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getSettingBool(...);
#else
      bool getSettingBool(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getSettingInt(id) }
      ///-----------------------------------------------------------------------
      /// Returns the value of a setting as an integer.
      ///
      /// @param id                      string - id of the setting that the module
      ///                                needs to access.
      /// @return                        Setting as an integer
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// max = self.Addon.getSettingInt('max')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getSettingInt(...);
#else
      int getSettingInt(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getSettingNumber(id) }
      ///-----------------------------------------------------------------------
      /// Returns the value of a setting as a floating point number.
      ///
      /// @param id                      string - id of the setting that the module
      ///                                needs to access.
      /// @return                        Setting as a floating point number
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// max = self.Addon.getSettingNumber('max')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getSettingNumber(...);
#else
      double getSettingNumber(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getSettingString(id) }
      ///-----------------------------------------------------------------------
      /// Returns the value of a setting as a unicode string.
      ///
      /// @param id                      string - id of the setting that the module
      ///                                needs to access.
      /// @return                        Setting as a unicode string
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// apikey = self.Addon.getSettingString('apikey')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getSettingString(...);
#else
      String getSettingString(const char* id);
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
      /// @python_v13
      /// **id** is optional as it will be auto detected for this add-on instance.
      ///
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
      /// @brief \python_func{ xbmcaddon.Addon([id]).setSettingBool(id, value) }
      ///-----------------------------------------------------------------------
      /// Sets a script setting.
      ///
      /// @param id                  string - id of the setting that the module needs to access.
      /// @param value               boolean - value of the setting.
      /// @return                    True if the value of the setting was set, false otherwise
      ///
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.Addon.setSettingBool(id='enabled', value=True)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setSettingBool(...);
#else
      bool setSettingBool(const char* id, bool value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).setSettingInt(id, value) }
      ///-----------------------------------------------------------------------
      /// Sets a script setting.
      ///
      /// @param id                  string - id of the setting that the module needs to access.
      /// @param value               integer - value of the setting.
      /// @return                    True if the value of the setting was set, false otherwise
      ///
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.Addon.setSettingInt(id='max', value=5)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setSettingInt(...);
#else
      bool setSettingInt(const char* id, int value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).setSettingNumber(id, value) }
      ///-----------------------------------------------------------------------
      /// Sets a script setting.
      ///
      /// @param id                  string - id of the setting that the module needs to access.
      /// @param value               float - value of the setting.
      /// @return                    True if the value of the setting was set, false otherwise
      ///
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.Addon.setSettingNumber(id='max', value=5.5)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setSettingNumber(...);
#else
      bool setSettingNumber(const char* id, double value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).setSettingString(id, value) }
      ///-----------------------------------------------------------------------
      /// Sets a script setting.
      ///
      /// @param id                  string - id of the setting that the module needs to access.
      /// @param value               string or unicode - value of the setting.
      /// @return                    True if the value of the setting was set, false otherwise
      ///
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.Addon.setSettingString(id='username', value='teamkodi')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setSettingString(...);
#else
      bool setSettingString(const char* id, const String& value);
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
