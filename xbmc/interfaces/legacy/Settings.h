/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "commons/Exception.h"
#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/AddonString.h"
#include "interfaces/legacy/Exception.h"
#include "interfaces/legacy/Tuple.h"
#include "settings/lib/SettingDefinitions.h"

#include <memory>
#include <string>
#include <vector>

class CSettingsBase;

namespace XBMCAddon
{
namespace xbmcaddon
{

XBMCCOMMONS_STANDARD_EXCEPTION(SettingCallbacksNotSupportedException);

//
/// \defgroup python_settings Settings
/// \ingroup python_xbmcaddon
/// @{
/// @brief **Add-on settings**
///
/// \python_class{ Settings() }
///
/// This wrapper provides access to the settings specific to an add-on.
/// It supports reading and writing specific setting values.
///
///-----------------------------------------------------------------------
/// @python_v20 New class added.
///
/// **Example:**
/// ~~~~~~~~~~~~~{.py}
/// ...
/// settings = xbmcaddon.Addon('id').getSettings()
/// ...
/// ~~~~~~~~~~~~~
//
class Settings : public AddonClass
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  std::shared_ptr<CSettingsBase> settings;
  Settings(std::shared_ptr<CSettingsBase> settings);
#endif
  virtual ~Settings() = default;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ getBool(id) }
  /// Returns the value of a setting as a boolean.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @return                       bool - Setting as a boolean
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// enabled = settings.getBool('enabled')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  getBool(...);
#else
  bool getBool(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ getInt(id) }
  /// Returns the value of a setting as an integer.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @return                       integer - Setting as an integer
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// max = settings.getInt('max')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  getInt(...);
#else
  int getInt(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ getNumber(id) }
  /// Returns the value of a setting as a floating point number.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @return                       float - Setting as a floating point number
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// max = settings.getNumber('max')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  getNumber(...);
#else
  double getNumber(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ getString(id) }
  /// Returns the value of a setting as a unicode string.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @return                       string - Setting as a unicode string
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// apikey = settings.getString('apikey')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  getString(...);
#else
  String getString(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ getBoolList(id) }
  /// Returns the value of a setting as a list of booleans.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @return                       list - Setting as a list of booleans
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// enabled = settings.getBoolList('enabled')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  getBoolList(...);
#else
  std::vector<bool> getBoolList(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ getIntList(id) }
  /// Returns the value of a setting as a list of integers.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @return                       list - Setting as a list of integers
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// ids = settings.getIntList('ids')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  getIntList(...);
#else
  std::vector<int> getIntList(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ getNumberList(id) }
  /// Returns the value of a setting as a list of floating point numbers.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @return                       list - Setting as a list of floating point numbers
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// max = settings.getNumberList('max')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  getNumberList(...);
#else
  std::vector<double> getNumberList(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ getStringList(id) }
  /// Returns the value of a setting as a list of unicode strings.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @return                       list - Setting as a list of unicode strings
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// views = settings.getStringList('views')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  getStringList(...);
#else
  std::vector<String> getStringList(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ setBool(id, value) }
  /// Sets the value of a setting.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @param value                  bool - value of the setting.
  ///
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// settings.setBool(id='enabled', value=True)
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  setBool(...);
#else
  void setBool(const char* id, bool value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ setInt(id, value) }
  /// Sets the value of a setting.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @param value                  integer - value of the setting.
  ///
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// settings.setInt(id='max', value=5)
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  setInt(...);
#else
  void setInt(const char* id, int value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ setNumber(id, value) }
  /// Sets the value of a setting.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @param value                  float - value of the setting.
  ///
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// settings.setNumber(id='max', value=5.5)
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  setNumber(...);
#else
  void setNumber(const char* id, double value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ setString(id, value) }
  /// Sets the value of a setting.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @param value                  string or unicode - value of the setting.
  ///
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// settings.setString(id='username', value='teamkodi')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  setString(...);
#else
  void setString(const char* id, const String& value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ setBoolList(id, values) }
  /// Sets the boolean values of a list setting.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @param values                 list of boolean - values of the setting.
  ///
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// settings.setBoolList(id='enabled', values=[ True, False ])
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  setBoolList(...);
#else
  void setBoolList(const char* id, const std::vector<bool>& values);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ setIntList(id, value) }
  /// Sets the integer values of a list setting.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @param values                 list of int - values of the setting.
  ///
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// settings.setIntList(id='max', values=[ 5, 23 ])
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  setIntList(...);
#else
  void setIntList(const char* id, const std::vector<int>& values);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ setNumberList(id, value) }
  /// Sets the floating point values of a list setting.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @param values                 list of float - values of the setting.
  ///
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// settings.setNumberList(id='max', values=[ 5.5, 5.8 ])
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  setNumberList(...);
#else
  void setNumberList(const char* id, const std::vector<double>& values);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
  ///
  /// \ingroup python_settings
  /// @brief \python_func{ setStringList(id, value) }
  /// Sets the string values of a list setting.
  ///
  /// @param id                     string - id of the setting that the module needs to access.
  /// @param values                 list of string or unicode - values of the setting.
  ///
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  ///
  ///-----------------------------------------------------------------------
  /// @python_v20 New function added.
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.py}
  /// ..
  /// settings.setStringList(id='username', values=[ 'team', 'kodi' ])
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  setStringList(...);
#else
  void setStringList(const char* id, const std::vector<String>& values);
#endif
};
//@}

} // namespace xbmcaddon
} // namespace XBMCAddon
