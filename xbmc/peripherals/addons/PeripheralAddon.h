/*
 *      Copyright (C) 2014-2016 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "addons/AddonDll.h"
#include "addons/DllPeripheral.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_peripheral_types.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_peripheral_utils.hpp"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/PeripheralTypes.h"

#include <map>
#include <memory>
#include <vector>

namespace JOYSTICK
{
  class IButtonMap;
  class IDriverHandler;
}

namespace PERIPHERALS
{
  class CPeripheral;
  class CPeripheralJoystick;

  typedef std::map<JOYSTICK::FeatureName, ADDON::JoystickFeature> FeatureMap;

  class CPeripheralAddon : public ADDON::CAddonDll<DllPeripheral, PeripheralAddon, PERIPHERAL_PROPERTIES>
  {
  public:
    static std::unique_ptr<CPeripheralAddon> FromExtension(ADDON::AddonProps props, const cp_extension_t* ext);

    CPeripheralAddon(ADDON::AddonProps props, bool bProvidesJoysticks, bool bProvidesButtonMaps);

    virtual ~CPeripheralAddon(void);

    // implementation of IAddon
    virtual ADDON::AddonPtr GetRunningInstance(void) const override;

    /*!
     * @brief Initialise the instance of this add-on
     */
    ADDON_STATUS CreateAddon(void);

    bool         Register(unsigned int peripheralIndex, CPeripheral* peripheral);
    void         UnregisterRemovedDevices(const PeripheralScanResults &results, std::vector<CPeripheral*>& removedPeripherals);
    void         GetFeatures(std::vector<PeripheralFeature> &features) const;
    bool         HasFeature(const PeripheralFeature feature) const;
    CPeripheral* GetPeripheral(unsigned int index) const;
    CPeripheral* GetByPath(const std::string &strPath) const;
    int          GetPeripheralsWithFeature(std::vector<CPeripheral*> &results, const PeripheralFeature feature) const;
    size_t       GetNumberOfPeripherals(void) const;
    size_t       GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const;
    void         GetDirectory(const std::string &strPath, CFileItemList &items) const;

    /** @name Peripheral add-on methods */
    //@{
    bool PerformDeviceScan(PeripheralScanResults &results);
    bool ProcessEvents(void);
    bool SendRumbleEvent(unsigned int index, unsigned int driverIndex, float magnitude);
    //@}

    /** @name Joystick methods */
    //@{
    bool GetJoystickProperties(unsigned int index, CPeripheralJoystick& joystick);
    bool HasButtonMaps(void) const { return m_bProvidesButtonMaps; }
    bool GetFeatures(const CPeripheral* device, const std::string& strControllerId, FeatureMap& features);
    bool MapFeatures(const CPeripheral* device, const std::string& strControllerId, const FeatureMap& features);
    void ResetButtonMap(const CPeripheral* device, const std::string& strControllerId);
    void PowerOffJoystick(unsigned int index);
    //@}

    void RegisterButtonMap(CPeripheral* device, JOYSTICK::IButtonMap* buttonMap);
    void UnregisterButtonMap(JOYSTICK::IButtonMap* buttonMap);
    void RefreshButtonMaps(const std::string& strDeviceName = "", const std::string& strControllerId = "");

  protected:
    /*!
     * @brief Request the API version from the add-on, and check if it's compatible
     * @return True when compatible, false otherwise.
     * @remark Implementation of CAddonDll
     */
    virtual bool CheckAPIVersion(void) override;

  private:
    /*!
     * @brief Helper functions
     */
    static void GetPeripheralInfo(const CPeripheral* device, ADDON::Peripheral& peripheralInfo);

    static void GetJoystickInfo(const CPeripheral* device, ADDON::Joystick& joystickInfo);
    static void SetJoystickInfo(CPeripheralJoystick& joystick, const ADDON::Joystick& joystickInfo);

    /*!
     * @brief Reset all class members to their defaults. Called by the constructors
     */
    void ResetProperties(void);

    /*!
     * @brief Retrieve add-on properties from the add-on
     */
    bool GetAddonProperties(void);

    /*!
     * @brief Checks whether the provided API version is compatible with XBMC
     * @param minVersion The add-on's XBMC_PERIPHERAL_MIN_API_VERSION version
     * @param version The add-on's XBMC_PERIPHERAL_API_VERSION version
     * @return True when compatible, false otherwise
     */
    static bool IsCompatibleAPIVersion(const ADDON::AddonVersion &minVersion, const ADDON::AddonVersion &version);

    bool LogError(const PERIPHERAL_ERROR error, const char *strMethod) const;
    void LogException(const std::exception &e, const char *strFunctionName) const;

    /* @brief Cache for const char* members in PERIPHERAL_PROPERTIES */
    std::string         m_strUserPath;    /*!< @brief translated path to the user profile */
    std::string         m_strClientPath;  /*!< @brief translated path to this add-on */

    /* @brief Add-on properties */
    ADDON::AddonVersion m_apiVersion;
    bool                m_bProvidesJoysticks;
    bool                m_bProvidesButtonMaps;

    /* @brief Map of peripherals belonging to the add-on */
    std::map<unsigned int, CPeripheral*>  m_peripherals;

    /* @brief Button map observers */
    std::vector<std::pair<CPeripheral*, JOYSTICK::IButtonMap*> > m_buttonMaps;

    /* @brief Thread synchronization */
    CCriticalSection    m_critSection;
  };
}
