/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Peripheral.h"
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
} // namespace JOYSTICK
} // namespace KODI

namespace PERIPHERALS
{
class CPeripheral;
class CPeripheralJoystick;
class CPeripherals;

typedef std::vector<kodi::addon::DriverPrimitive> PrimitiveVector;
typedef std::map<KODI::JOYSTICK::FeatureName, kodi::addon::JoystickFeature> FeatureMap;

class CPeripheralAddon : public ADDON::IAddonInstanceHandler
{
public:
  explicit CPeripheralAddon(const ADDON::AddonInfoPtr& addonInfo, CPeripherals& manager);
  ~CPeripheralAddon(void) override;

  /*!
   * @brief Initialise the instance of this add-on
   */
  bool CreateAddon(void);

  /*!
   * \brief Deinitialize the instance of this add-on
   */
  void DestroyAddon();

  bool Register(unsigned int peripheralIndex, const PeripheralPtr& peripheral);
  void UnregisterRemovedDevices(const PeripheralScanResults& results,
                                PeripheralVector& removedPeripherals);
  void GetFeatures(std::vector<PeripheralFeature>& features) const;
  bool HasFeature(const PeripheralFeature feature) const;
  PeripheralPtr GetPeripheral(unsigned int index) const;
  PeripheralPtr GetByPath(const std::string& strPath) const;
  bool SupportsFeature(PeripheralFeature feature) const;
  unsigned int GetPeripheralsWithFeature(PeripheralVector& results,
                                         const PeripheralFeature feature) const;
  unsigned int GetNumberOfPeripherals(void) const;
  unsigned int GetNumberOfPeripheralsWithId(const int iVendorId, const int iProductId) const;
  void GetDirectory(const std::string& strPath, CFileItemList& items) const;

  /** @name Peripheral add-on methods */
  //@{
  bool PerformDeviceScan(PeripheralScanResults& results);
  bool ProcessEvents(void);
  bool SendRumbleEvent(unsigned int index, unsigned int driverIndex, float magnitude);
  //@}

  /** @name Joystick methods */
  //@{
  bool GetJoystickProperties(unsigned int index, CPeripheralJoystick& joystick);
  bool HasButtonMaps(void) const { return m_bProvidesButtonMaps; }
  bool GetFeatures(const CPeripheral* device,
                   const std::string& strControllerId,
                   FeatureMap& features);
  bool MapFeature(const CPeripheral* device,
                  const std::string& strControllerId,
                  const kodi::addon::JoystickFeature& feature);
  bool GetIgnoredPrimitives(const CPeripheral* device, PrimitiveVector& primitives);
  bool SetIgnoredPrimitives(const CPeripheral* device, const PrimitiveVector& primitives);
  void SaveButtonMap(const CPeripheral* device);
  void RevertButtonMap(const CPeripheral* device);
  void ResetButtonMap(const CPeripheral* device, const std::string& strControllerId);
  void PowerOffJoystick(unsigned int index);
  //@}

  void RegisterButtonMap(CPeripheral* device, KODI::JOYSTICK::IButtonMap* buttonMap);
  void UnregisterButtonMap(KODI::JOYSTICK::IButtonMap* buttonMap);

  static inline bool ProvidesJoysticks(const ADDON::AddonInfoPtr& addonInfo)
  {
    return addonInfo->Type(ADDON::ADDON_PERIPHERALDLL)->GetValue("@provides_joysticks").asBoolean();
  }

  static inline bool ProvidesButtonMaps(const ADDON::AddonInfoPtr& addonInfo)
  {
    return addonInfo->Type(ADDON::ADDON_PERIPHERALDLL)
        ->GetValue("@provides_buttonmaps")
        .asBoolean();
  }

private:
  void UnregisterButtonMap(CPeripheral* device);

  // Binary add-on callbacks
  void TriggerDeviceScan();
  void RefreshButtonMaps(const std::string& strDeviceName = "");
  unsigned int FeatureCount(const std::string& controllerId, JOYSTICK_FEATURE_TYPE type) const;
  JOYSTICK_FEATURE_TYPE FeatureType(const std::string& controllerId,
                                    const std::string& featureName) const;

  /*!
   * @brief Helper functions
   */
  static void GetPeripheralInfo(const CPeripheral* device, kodi::addon::Peripheral& peripheralInfo);

  static void GetJoystickInfo(const CPeripheral* device, kodi::addon::Joystick& joystickInfo);
  static void SetJoystickInfo(CPeripheralJoystick& joystick,
                              const kodi::addon::Joystick& joystickInfo);

  /*!
   * @brief Reset all class members to their defaults. Called by the constructors
   */
  void ResetProperties(void);

  /*!
   * @brief Retrieve add-on properties from the add-on
   */
  bool GetAddonProperties(void);

  bool LogError(const PERIPHERAL_ERROR error, const char* strMethod) const;

  static std::string GetDeviceName(PeripheralType type);
  static std::string GetProvider(PeripheralType type);

  // Construction parameters
  CPeripherals& m_manager;

  /* @brief Cache for const char* members in PERIPHERAL_PROPERTIES */
  std::string m_strUserPath; /*!< @brief translated path to the user profile */
  std::string m_strClientPath; /*!< @brief translated path to this add-on */

  /*!
   * @brief Callback functions from addon to kodi
   */
  //@{
  static void cb_trigger_scan(void* kodiInstance);
  static void cb_refresh_button_maps(void* kodiInstance,
                                     const char* deviceName,
                                     const char* controllerId);
  static unsigned int cb_feature_count(void* kodiInstance,
                                       const char* controllerId,
                                       JOYSTICK_FEATURE_TYPE type);
  static JOYSTICK_FEATURE_TYPE cb_feature_type(void* kodiInstance,
                                               const char* controllerId,
                                               const char* featureName);
  //@}

  /* @brief Add-on properties */
  bool m_bProvidesJoysticks;
  bool m_bSupportsJoystickRumble;
  bool m_bSupportsJoystickPowerOff;
  bool m_bProvidesButtonMaps;

  /* @brief Map of peripherals belonging to the add-on */
  std::map<unsigned int, PeripheralPtr> m_peripherals;

  /* @brief Button map observers */
  std::vector<std::pair<CPeripheral*, KODI::JOYSTICK::IButtonMap*>> m_buttonMaps;
  CCriticalSection m_buttonMapMutex;

  /* @brief Thread synchronization */
  mutable CCriticalSection m_critSection;

  AddonInstance_Peripheral m_struct;

  CSharedSection m_dllSection;
};
} // namespace PERIPHERALS
