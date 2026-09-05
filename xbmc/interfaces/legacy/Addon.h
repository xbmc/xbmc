/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "AddonString.h"
#include "Exception.h"
#include "Settings.h"
#include "addons/IAddon.h"

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
      /// Returns an addon's localized 'string'.
      ///
      /// @param id                      integer - id# for string you want to
      ///                                localize.
      /// @return                        Localized 'string'
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
      /// @brief \python_func{ xbmcaddon.Addon([id]).supportsInstanceSettings() }
      /// Returns true if the requested add-on supports several instances.
      ///
      /// @return Returns true if supported, false otherwise
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v21 New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// supportsInstances = self.Addon('pvr.iptvsimple').supportsInstanceSettings()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      supportsInstanceSettings();
#else
      bool supportsInstanceSettings();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getInstanceIds() }
      /// Returns the list of known available instance ids.
      ///
      /// @return List of used add-on instances
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v21 New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// usedInstances = self.Addon('pvr.iptvsimple').getInstanceIds()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getInstanceIds();
#else
      std::vector<unsigned int> getInstanceIds();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getFreeNewInstanceId() }
      /// Get free add-on instance id to set new independent instance and settings about.
      ///
      /// @note After setting the new instance settings a @ref setInstanceState() call is required.
      ///
      /// @return Next usable and free instance id
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v21 New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// addon = xbmcaddon.Addon('pvr.iptvsimple')
      /// newinstanceid = addon.getFreeNewInstanceId()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getFreeNewInstanceId();
#else
      unsigned int getFreeNewInstanceId();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).setInstanceState(id, enabled, [name]) }
      /// Set changed state on add-on instance system.
      ///
      /// @param instance unsigned integer - Instance identifier to use.
      /// @param enabled boolean - To enable or disable the instance
      /// @param name [opt] string or unicode - The instance name used inside Kodi
      ///             @warning Needs on new added instances set.
      /// @return True if the setting the instance change was successful, false otherwise
      ///
      /// @note To add or Enable/Disable an instance this function needs to be called.
      ///
      ///-----------------------------------------------------------------------
      /// @python_v21 New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      ///
      /// addon = xbmcaddon.Addon('pvr.iptvsimple')
      /// newinstanceid = addon.getFreeNewInstanceId()
      ///
      /// settings = addon.getSettings(newinstanceid)
      /// settings.setInt('m3uPathType', 0)
      /// settings.setString('m3uPath', '/SOME_PATH_OF_YOU')
      /// ...
      /// addon.setInstanceState(newinstanceid, True, 'Example by Python added')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setInstanceState(...);
#else
      bool setInstanceState(unsigned int instance, bool enabled, const String& name = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).deleteInstance(id) }
      /// Delete selected instance settings from storage.
      ///
      /// @param instance unsigned integer - Instance identifier to use.
      /// @return true on success, false otherwise.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v21 New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// instance = 1;
      /// self.Addon('pvr.iptvsimple').deleteInstance(instance)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      deleteInstance(...);
#else
      bool deleteInstance(unsigned int instance);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getSettings([instance]) }
      /// Returns a wrapper around the addon's settings.
      ///
      /// @param instance [opt] unsigned integer - used id of the requested add-on instance setting
      ///                 @note use 0 to access regular settings.xml.
      /// @return @ref python_settings wrapper
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// settings = self.Addon.getSettings()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getSettings(...);
#else
      Settings* getSettings(unsigned int instance = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getSetting(id) }
      /// Returns the value of a setting as string.
      ///
      /// @param id                      string - id of the setting that the module
      ///                                needs to access.
      /// @return                        Setting as a string
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
      /// @python_v20 Deprecated. Use **Settings.getBool()** instead.
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
      /// @python_v20 Deprecated. Use **Settings.getInt()** instead.
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
      /// @python_v20 Deprecated. Use **Settings.getNumber()** instead.
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
      /// Returns the value of a setting as a string.
      ///
      /// @param id                      string - id of the setting that the module
      ///                                needs to access.
      /// @return                        Setting as a string
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18
      /// New function added.
      /// @python_v20 Deprecated. Use **Settings.getString()** instead.
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
      /// Sets a script setting.
      ///
      /// @param id                  string - id of the setting that the module needs to access.
      /// @param value               string - value of the setting.
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
      /// @python_v20 Deprecated. Use **Settings.setBool()** instead.
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
      /// @python_v20 Deprecated. Use **Settings.setInt()** instead.
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
      /// @python_v20 Deprecated. Use **Settings.setNumber()** instead.
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
      /// @python_v20 Deprecated. Use **Settings.setString()** instead.
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
      /// @brief \python_func{ xbmcaddon.Addon([id]).openSettings([instance]) }
      /// Opens this scripts settings dialog.
      ///
      /// @param instance [opt] unsigned integer - used id of the requested add-on instance setting
      ///                 @note use 0 to access regular settings.xml.
      ///
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
      void openSettings(unsigned int instance = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcaddon
      /// \anchor python_xbmcaddon_Addon
      /// @brief \python_func{ xbmcaddon.Addon([id]).getAddonInfo(id) }
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
