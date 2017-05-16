/*
 *      Copyright (C) 2014-2017 Team Kodi
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
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_peripheral_types.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_peripheral_utils.hpp"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/PeripheralTypes.h"
#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"

#include <map>
#include <memory>
#include <vector>

namespace KODI
{
namespace JOYSTICK
{
  class IButtonMap;
  class IDriverHandler;
}
}

namespace PERIPHERALS
{
  class CPeripheral;
  class CPeripheralJoystick;

  typedef std::vector<ADDON::DriverPrimitive> PrimitiveVector;
  typedef std::map<KODI::JOYSTICK::FeatureName, ADDON::JoystickFeature> FeatureMap;

  class CPeripheralAddon : public ADDON::CAddonDll
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

    /*!
     * \brief Deinitialize the instance of this add-on
     */
    void DestroyAddon();

    bool         Register(unsigned int peripheralIndex, const PeripheralPtr& peripheral);
    void         UnregisterRemovedDevices(const PeripheralScanResults &results, PeripheralVector& removedPeripherals);
    void         GetFeatures(std::vector<PeripheralFeature> &features) const;
    bool         HasFeature(const PeripheralFeature feature) const;
    PeripheralPtr GetPeripheral(unsigned int index) const;
    PeripheralPtr GetByPath(const std::string &strPath) const;
    bool         SupportsFeature(PeripheralFeature feature) const;
    int          GetPeripheralsWithFeature(PeripheralVector &results, const PeripheralFeature feature) const;
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
    bool MapFeature(const CPeripheral* device, const std::string& strControllerId, const ADDON::JoystickFeature& feature);
    bool GetIgnoredPrimitives(const CPeripheral* device, PrimitiveVector& primitives);
    bool SetIgnoredPrimitives(const CPeripheral* device, const PrimitiveVector& primitives);
    void SaveButtonMap(const CPeripheral* device);
    void RevertButtonMap(const CPeripheral* device);
    void ResetButtonMap(const CPeripheral* device, const std::string& strControllerId);
    void PowerOffJoystick(unsigned int index);
    //@}

    void RegisterButtonMap(CPeripheral* device, KODI::JOYSTICK::IButtonMap* buttonMap);
    void UnregisterButtonMap(KODI::JOYSTICK::IButtonMap* buttonMap);
    void RefreshButtonMaps(const std::string& strDeviceName = "");

  private:
    void UnregisterButtonMap(CPeripheral* device);

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

    bool LogError(const PERIPHERAL_ERROR error, const char *strMethod) const;

    static std::string GetDeviceName(PeripheralType type);
    static std::string GetProvider(PeripheralType type);

    /* @brief Cache for const char* members in PERIPHERAL_PROPERTIES */
    std::string         m_strUserPath;    /*!< @brief translated path to the user profile */
    std::string         m_strClientPath;  /*!< @brief translated path to this add-on */

    /* @brief Add-on properties */
    ADDON::AddonVersion m_apiVersion;
    bool                m_bProvidesJoysticks;
    bool                m_bSupportsJoystickRumble;
    bool                m_bSupportsJoystickPowerOff;
    bool                m_bProvidesButtonMaps;

    /* @brief Map of peripherals belonging to the add-on */
    std::map<unsigned int, PeripheralPtr>  m_peripherals;

    /* @brief Button map observers */
    std::vector<std::pair<CPeripheral*, KODI::JOYSTICK::IButtonMap*> > m_buttonMaps;
    CCriticalSection m_buttonMapMutex;

    /* @brief Thread synchronization */
    CCriticalSection    m_critSection;
    
    PERIPHERAL_PROPERTIES m_info;
    KodiToAddonFuncTable_Peripheral m_struct;

    CSharedSection      m_dllSection;
  };
}
