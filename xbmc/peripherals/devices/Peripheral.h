/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/joysticks/interfaces/IInputProvider.h"
#include "input/keyboard/interfaces/IKeyboardInputProvider.h"
#include "input/mouse/interfaces/IMouseInputProvider.h"
#include "peripherals/PeripheralTypes.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class CDateTime;
class CSetting;

namespace KODI
{
namespace JOYSTICK
{
class IButtonMapper;
class IDriverHandler;
class IDriverReceiver;
class IInputHandler;
} // namespace JOYSTICK

namespace KEYBOARD
{
class IKeyboardDriverHandler;
} // namespace KEYBOARD

namespace KEYMAP
{
class IKeymap;
} // namespace KEYMAP

namespace MOUSE
{
class IMouseDriverHandler;
} // namespace MOUSE
} // namespace KODI

namespace PERIPHERALS
{
class CAddonButtonMapping;
class CGUIDialogPeripheralSettings;
class CPeripheralBus;
class CPeripherals;

/*!
 * \ingroup peripherals
 */
typedef enum
{
  STATE_SWITCH_TOGGLE,
  STATE_ACTIVATE_SOURCE,
  STATE_STANDBY
} CecStateChange;

/*!
 * \ingroup peripherals
 */
class CPeripheral : public KODI::JOYSTICK::IInputProvider,
                    public KODI::KEYBOARD::IKeyboardInputProvider,
                    public KODI::MOUSE::IMouseInputProvider
{
  friend class CGUIDialogPeripheralSettings;

public:
  CPeripheral(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus);
  ~CPeripheral(void) override;

  bool operator==(const CPeripheral& right) const;
  bool operator!=(const CPeripheral& right) const;
  bool operator==(const PeripheralScanResult& right) const;
  bool operator!=(const PeripheralScanResult& right) const;

  const std::string& FileLocation(void) const { return m_strFileLocation; }
  const std::string& Location(void) const { return m_strLocation; }
  int VendorId(void) const { return m_iVendorId; }
  const char* VendorIdAsString(void) const { return m_strVendorId.c_str(); }
  int ProductId(void) const { return m_iProductId; }
  const char* ProductIdAsString(void) const { return m_strProductId.c_str(); }
  PeripheralType Type(void) const { return m_type; }
  PeripheralBusType GetBusType(void) const { return m_busType; }
  const std::string& DeviceName(void) const { return m_strDeviceName; }
  bool IsHidden(void) const { return m_bHidden; }
  void SetHidden(bool bSetTo = true) { m_bHidden = bSetTo; }
  const std::string& GetVersionInfo(void) const { return m_strVersionInfo; }

  /*!
   * @brief Get an icon for this peripheral
   * @return Path to an icon, or skin icon file name
   */
  virtual std::string GetIcon() const;

  /*!
   * @brief Check whether this device has the given feature.
   * @param feature The feature to check for.
   * @return True when the device has the feature, false otherwise.
   */
  bool HasFeature(const PeripheralFeature feature) const;

  /*!
   * @brief Get all features that are supported by this device.
   * @param features The features.
   */
  void GetFeatures(std::vector<PeripheralFeature>& features) const;

  /*!
   * @brief Initialises the peripheral.
   * @return True when the peripheral has been initialised successfully, false otherwise.
   */
  bool Initialise(void);

  /*!
   * @brief Initialise one of the features of this peripheral.
   * @param feature The feature to initialise.
   * @return True when the feature has been initialised successfully, false otherwise.
   */
  virtual bool InitialiseFeature(const PeripheralFeature feature) { return true; }

  /*!
   * @brief Briefly activate a feature to notify the user
   */
  virtual void OnUserNotification() {}

  /*!
   * @brief Briefly test one of the features of this peripheral.
   * @param feature The feature to test.
   * @return True if the test succeeded, false otherwise.
   */
  virtual bool TestFeature(PeripheralFeature feature) { return false; }

  /*!
   * @brief Called when a setting changed.
   * @param strChangedSetting The changed setting.
   */
  virtual void OnSettingChanged(const std::string& strChangedSetting) {}

  /*!
   * @brief Called when this device is removed, before calling the destructor.
   */
  virtual void OnDeviceRemoved(void) {}

  /*!
   * @brief Get all subdevices if this device is multifunctional.
   * @param subDevices The subdevices.
   */
  virtual void GetSubdevices(PeripheralVector& subDevices) const;

  /*!
   * @return True when this device is multifunctional, false otherwise.
   */
  virtual bool IsMultiFunctional(void) const;

  /*!
   * @brief Add a setting to this peripheral. This will overwrite a previous setting with the same
   * key.
   * @param strKey The key of the setting.
   * @param setting The setting.
   */
  virtual void AddSetting(const std::string& strKey,
                          const std::shared_ptr<const CSetting>& setting,
                          int order);

  /*!
   * @brief Check whether a setting is known with the given key.
   * @param strKey The key to search.
   * @return True when found, false otherwise.
   */
  virtual bool HasSetting(const std::string& strKey) const;

  /*!
   * @return True when this device has any settings, false otherwise.
   */
  virtual bool HasSettings(void) const;

  /*!
   * @return True when this device has any configurable settings, false otherwise.
   */
  virtual bool HasConfigurableSettings(void) const;

  /*!
   * @brief Get the value of a setting.
   * @param strKey The key to search.
   * @return The value or an empty string if it wasn't found.
   */
  virtual const std::string GetSettingString(const std::string& strKey) const;
  virtual bool SetSetting(const std::string& strKey, const std::string& strValue);
  virtual void SetSettingVisible(const std::string& strKey, bool bSetTo);
  virtual bool IsSettingVisible(const std::string& strKey) const;

  virtual int GetSettingInt(const std::string& strKey) const;
  virtual bool SetSetting(const std::string& strKey, int iValue);

  virtual bool GetSettingBool(const std::string& strKey) const;
  virtual bool SetSetting(const std::string& strKey, bool bValue);

  virtual float GetSettingFloat(const std::string& strKey) const;
  virtual bool SetSetting(const std::string& strKey, float fValue);

  virtual void PersistSettings(bool bExiting = false);
  virtual void LoadPersistedSettings(void);
  virtual void ResetDefaultSettings(void);

  virtual std::vector<std::shared_ptr<CSetting>> GetSettings(void) const;

  virtual bool ErrorOccured(void) const { return m_bError; }

  virtual void RegisterJoystickDriverHandler(KODI::JOYSTICK::IDriverHandler* handler,
                                             bool bPromiscuous)
  {
  }
  virtual void UnregisterJoystickDriverHandler(KODI::JOYSTICK::IDriverHandler* handler) {}

  virtual void RegisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler,
                                             bool bPromiscuous)
  {
  }
  virtual void UnregisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler) {}

  virtual void RegisterMouseDriverHandler(KODI::MOUSE::IMouseDriverHandler* handler,
                                          bool bPromiscuous)
  {
  }
  virtual void UnregisterMouseDriverHandler(KODI::MOUSE::IMouseDriverHandler* handler) {}

  // implementation of IInputProvider
  void RegisterInputHandler(KODI::JOYSTICK::IInputHandler* handler, bool bPromiscuous) override;
  void UnregisterInputHandler(KODI::JOYSTICK::IInputHandler* handler) override;

  // implementation of IKeyboardInputProvider
  void RegisterKeyboardHandler(KODI::KEYBOARD::IKeyboardInputHandler* handler,
                               bool bPromiscuous,
                               bool forceDefaultMap) override;
  void UnregisterKeyboardHandler(KODI::KEYBOARD::IKeyboardInputHandler* handler) override;

  // implementation of IMouseInputProvider
  void RegisterMouseHandler(KODI::MOUSE::IMouseInputHandler* handler,
                            bool bPromiscuous,
                            bool forceDefaultMap) override;
  void UnregisterMouseHandler(KODI::MOUSE::IMouseInputHandler* handler) override;

  virtual void RegisterJoystickButtonMapper(KODI::JOYSTICK::IButtonMapper* mapper);
  virtual void UnregisterJoystickButtonMapper(KODI::JOYSTICK::IButtonMapper* mapper);

  virtual KODI::JOYSTICK::IDriverReceiver* GetDriverReceiver() { return nullptr; }

  virtual KODI::KEYMAP::IKeymap* GetKeymap(const std::string& controllerId) { return nullptr; }

  /*!
   * \brief Return the last time this peripheral was active
   *
   * \return The time of last activation, or invalid if unknown/never active
   */
  virtual CDateTime LastActive() const;

  /*!
   * \brief Get the controller profile that best represents this peripheral
   *
   * \return The controller profile, or empty if unknown
   */
  virtual KODI::GAME::ControllerPtr ControllerProfile() const { return m_controllerProfile; }

  /*!
   * \brief Set the controller profile for this peripheral
   *
   * \param controller The new controller profile
   */
  virtual void SetControllerProfile(const KODI::GAME::ControllerPtr& controller)
  {
    m_controllerProfile = controller;
  }

protected:
  virtual void ClearSettings(void);

  CPeripherals& m_manager;
  PeripheralType m_type;
  PeripheralBusType m_busType;
  PeripheralBusType m_mappedBusType;
  std::string m_strLocation;
  std::string m_strDeviceName;
  std::string m_strSettingsFile;
  std::string m_strFileLocation;
  int m_iVendorId;
  std::string m_strVendorId;
  int m_iProductId;
  std::string m_strProductId;
  std::string m_strVersionInfo;
  bool m_bInitialised = false;
  bool m_bHidden = false;
  bool m_bError = false;
  std::vector<PeripheralFeature> m_features;
  PeripheralVector m_subDevices;
  std::map<std::string, PeripheralDeviceSetting> m_settings;
  std::set<std::string> m_changedSettings;
  CPeripheralBus* m_bus;
  std::map<KODI::JOYSTICK::IInputHandler*, std::unique_ptr<KODI::JOYSTICK::IDriverHandler>>
      m_inputHandlers;
  std::map<KODI::KEYBOARD::IKeyboardInputHandler*,
           std::unique_ptr<KODI::KEYBOARD::IKeyboardDriverHandler>>
      m_keyboardHandlers;
  std::map<KODI::MOUSE::IMouseInputHandler*, std::unique_ptr<KODI::MOUSE::IMouseDriverHandler>>
      m_mouseHandlers;
  std::map<KODI::JOYSTICK::IButtonMapper*, std::unique_ptr<CAddonButtonMapping>> m_buttonMappers;
  KODI::GAME::ControllerPtr m_controllerProfile;
};
} // namespace PERIPHERALS
