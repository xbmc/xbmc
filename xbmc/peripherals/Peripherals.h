#pragma once
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

#include <vector>

#include "EventScanner.h"
#include "bus/PeripheralBus.h"
#include "devices/Peripheral.h"
#include "interfaces/IAnnouncer.h"
#include "messaging/IMessageTarget.h"
#include "settings/lib/ISettingCallback.h"
#include "system.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

class CFileItemList;
class CSetting;
class CSettingsCategory;
class TiXmlElement;
class CAction;
class CKey;

namespace JOYSTICK
{
  class IButtonMapper;
}

namespace PERIPHERALS
{
  #define g_peripherals CPeripherals::GetInstance()

  class CPeripherals :  public ISettingCallback,
                        public Observable,
                        public KODI::MESSAGING::IMessageTarget,
                        public IEventScannerCallback,
                        public ANNOUNCEMENT::IAnnouncer
  {
  public:
    static CPeripherals &GetInstance();
    virtual ~CPeripherals();

    /*!
     * @brief Initialise the peripherals manager.
     */
    void Initialise();

    /*!
     * @brief Clear all data known by the peripherals manager.
     */
    void Clear();

    /*!
     * @brief Get the instance of the peripheral at the given location.
     * @param strLocation The location.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return The peripheral or NULL if it wasn't found.
     */
    PeripheralPtr GetPeripheralAtLocation(const std::string &strLocation, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Check whether a peripheral is present at the given location.
     * @param strLocation The location.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return True when a peripheral was found, false otherwise.
     */
    bool HasPeripheralAtLocation(const std::string &strLocation, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Get the bus that holds the device with the given location.
     * @param strLocation The location.
     * @return The bus or NULL if no device was found.
     */
    PeripheralBusPtr GetBusWithDevice(const std::string &strLocation) const;

    /*!
     * @brief Check if any busses support the given feature
     * @param feature The feature to check for
     * @return True if a bus supports the feature, false otherwise
     */
    bool SupportsFeature(PeripheralFeature feature) const;

    /*!
     * @brief Get all peripheral instances that have the given feature.
     * @param results The list of results.
     * @param feature The feature to search for.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return The number of devices that have been found.
     */
    int GetPeripheralsWithFeature(PeripheralVector &results, const PeripheralFeature feature, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    size_t GetNumberOfPeripherals() const;

    /*!
     * @brief Check whether there is at least one device present with the given feature.
     * @param feature The feature to check for.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return True when at least one device was found with this feature, false otherwise.
     */
    bool HasPeripheralWithFeature(const PeripheralFeature feature, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Called when a device has been added to a bus.
     * @param bus The bus the device was added to.
     * @param peripheral The peripheral that has been added.
     */
    void OnDeviceAdded(const CPeripheralBus &bus, const CPeripheral &peripheral);

    /*!
     * @brief Called when a device has been deleted from a bus.
     * @param bus The bus from which the device removed.
     * @param peripheral The peripheral that has been removed.
     */
    void OnDeviceDeleted(const CPeripheralBus &bus, const CPeripheral &peripheral);

    /*!
     * @brief Creates a new instance of a peripheral.
     * @param bus The bus on which this peripheral is present.
     * @param result The scan result from the device scanning code.
     * @return The new peripheral or NULL if it could not be created.
     */
    void CreatePeripheral(CPeripheralBus &bus, const PeripheralScanResult& result);

    /*!
     * @brief Add the settings that are defined in the mappings file to the peripheral (if there is anything defined).
     * @param peripheral The peripheral to get the settings for.
     */
    void GetSettingsFromMapping(CPeripheral &peripheral) const;

    /*!
     * @brief Trigger a device scan on all known busses
     */
    void TriggerDeviceScan(const PeripheralBusType type = PERIPHERAL_BUS_UNKNOWN);

    /*!
     * @brief Get the instance of a bus given it's type.
     * @param type The bus type.
     * @return The bus or NULL if it wasn't found.
     */
    PeripheralBusPtr GetBusByType(const PeripheralBusType type) const;

    /*!
     * @brief Get all fileitems for a path.
     * @param strPath The path to the directory to get the items from.
     * @param items The item list.
     */
    void GetDirectory(const std::string &strPath, CFileItemList &items) const;

    /*!
     * @brief Get the instance of a peripheral given it's path.
     * @param strPath The path to the peripheral.
     * @return The peripheral or NULL if it wasn't found.
     */
    PeripheralPtr GetByPath(const std::string &strPath) const;

    /*!
     * @brief Try to let one of the peripherals handle an action.
     * @param action The change to handle.
     * @return True when this change was handled by a peripheral (and should not be handled by anything else), false otherwise.
     */
    bool OnAction(const CAction &action);

    /*!
     * @brief Check whether there's a peripheral that reports to be muted.
     * @return True when at least one peripheral reports to be muted, false otherwise.
     */
    bool IsMuted();

    /*!
     * @brief Try to toggle the mute status via a peripheral.
     * @return True when this change was handled by a peripheral (and should not be handled by anything else), false otherwise.
     */
    bool ToggleMute();

    /*!
     * @brief Try to toggle the playing device state via a peripheral.
     * @param mode Whether to activate, put on standby or toggle the source.
     * @return True when the playing device has been switched on, false otherwise.
     */
    bool ToggleDeviceState(const CecStateChange mode = STATE_SWITCH_TOGGLE);

    /*!
     * @brief Try to mute the audio via a peripheral.
     * @return True when this change was handled by a peripheral (and should not be handled by anything else), false otherwise.
     */
    bool Mute() { return ToggleMute(); } //! @todo CEC only supports toggling the mute status at this time

    /*!
     * @brief Try to unmute the audio via a peripheral.
     * @return True when this change was handled by a peripheral (and should not be handled by anything else), false otherwise.
     */
    bool UnMute() { return ToggleMute(); } //! @todo CEC only supports toggling the mute status at this time

    /*!
     * @brief Try to get a keypress from a peripheral.
     * @param frameTime The current frametime.
     * @param key The fetched key.
     * @return True when a keypress was fetched, false otherwise.
     */
    bool GetNextKeypress(float frameTime, CKey &key);

    /*!
     * @brief Request event scan rate
     * @brief rateHz The rate in Hz
     * @return A handle that unsets its rate when expired
     */
    EventRateHandle SetEventScanRate(double rateHz) { return m_eventScanner.SetRate(rateHz); }

    /*!
     * 
     */
    void OnUserNotification();

    /*!
     * @brief Request peripherals with the specified feature to perform a quick test
     * @return true if any peripherals support the feature, false otherwise
     */
    bool TestFeature(PeripheralFeature feature);

    /*!
     * \brief Request all devices with power-off support to power down
     */
    void PowerOffDevices();

    bool SupportsCEC() const
    {
#if defined(HAVE_LIBCEC)
      return true;
#else
      return false;
#endif
    }

    // implementation of IEventScannerCallback
    virtual void ProcessEvents(void) override;

    /*!
     * \brief Initialize button mapping
     *
     * This command enables button mapping on all busses. Button maps allow
     * connect events from the driver to the higher-level features used by
     * controller profiles.
     *
     * If user input is required, a blocking dialog may be shown.
     *
     * \return True if button mapping is enabled for at least one bus
     */
    bool EnableButtonMapping();

    /*!
     * \brief Get an add-on that can provide button maps for a device
     * \return An add-on that provides button maps, or empty if no add-on is found
     */
    PeripheralAddonPtr GetAddonWithButtonMap(const CPeripheral* device);

    /*!
     * \brief Reset all button maps to the defaults for all devices and the given controller
     * \param controllerId The controller profile to reset
     * @todo Add a device parameter to allow resetting button maps per-device
     */
    void ResetButtonMaps(const std::string& controllerId);

    /*!
     * \brief Register a button mapper interface
     * \param mapper The button mapper
     *
     * Clients implementing the IButtonMapper interface call
     * \ref CPeripherals::RegisterJoystickButtonMapper to register themselves
     * as eligible for button mapping commands.
     *
     * When registering the mapper is forwarded to all peripherals. See
     * \ref CPeripheral::RegisterJoystickButtonMapper for what is done to the
     * mapper after being given to the peripheral.
     */
    void RegisterJoystickButtonMapper(JOYSTICK::IButtonMapper* mapper);

    /*!
     * \brief Unregister a button mapper interface
     * \param mapper The button mapper
     */
    void UnregisterJoystickButtonMapper(JOYSTICK::IButtonMapper* mapper);

    // implementation of ISettingCallback
    virtual void OnSettingChanged(const CSetting *setting) override;
    virtual void OnSettingAction(const CSetting *setting) override;

    // implementation of IMessageTarget
    virtual void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;
    virtual int GetMessageMask() override;

    // implementation of IAnnouncer
    virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;

  private:
    CPeripherals();
    bool LoadMappings();
    bool GetMappingForDevice(const CPeripheralBus &bus, PeripheralScanResult& result) const;
    static void GetSettingsFromMappingsFile(TiXmlElement *xmlNode, std::map<std::string, PeripheralDeviceSetting> &m_settings);

    void OnDeviceChanged();

    bool                                 m_bInitialised;
    bool                                 m_bIsStarted;
#if !defined(HAVE_LIBCEC)
    bool                                 m_bMissingLibCecWarningDisplayed = false;
#endif
    std::vector<PeripheralBusPtr>        m_busses;
    std::vector<PeripheralDeviceMapping> m_mappings;
    CEventScanner                        m_eventScanner;
    CCriticalSection                     m_critSection;
    CCriticalSection                     m_critSectionBusses;
    CCriticalSection                     m_critSectionMappings;
  };
}
