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
                        public IEventScannerCallback
  {
  public:
    static CPeripherals &GetInstance();
    virtual ~CPeripherals();

    /*!
     * @brief Initialise the peripherals manager.
     */
    virtual void Initialise();

    /*!
     * @brief Clear all data known by the peripherals manager.
     */
    virtual void Clear();

    /*!
     * @brief Get the instance of the peripheral at the given location.
     * @param strLocation The location.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return The peripheral or NULL if it wasn't found.
     */
    virtual CPeripheral *GetPeripheralAtLocation(const std::string &strLocation, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Check whether a peripheral is present at the given location.
     * @param strLocation The location.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return True when a peripheral was found, false otherwise.
     */
    virtual bool HasPeripheralAtLocation(const std::string &strLocation, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Get the bus that holds the device with the given location.
     * @param strLocation The location.
     * @return The bus or NULL if no device was found.
     */
    virtual PeripheralBusPtr GetBusWithDevice(const std::string &strLocation) const;

    /*!
     * @brief Get all peripheral instances that have the given feature.
     * @param results The list of results.
     * @param feature The feature to search for.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return The number of devices that have been found.
     */
    virtual int GetPeripheralsWithFeature(std::vector<CPeripheral *> &results, const PeripheralFeature feature, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    size_t GetNumberOfPeripherals() const;

    /*!
     * @brief Check whether there is at least one device present with the given feature.
     * @param feature The feature to check for.
     * @param busType The bus to query. Default (PERIPHERAL_BUS_UNKNOWN) searches all busses.
     * @return True when at least one device was found with this feature, false otherwise.
     */
    virtual bool HasPeripheralWithFeature(const PeripheralFeature feature, PeripheralBusType busType = PERIPHERAL_BUS_UNKNOWN) const;

    /*!
     * @brief Called when a device has been added to a bus.
     * @param bus The bus the device was added to.
     * @param peripheral The peripheral that has been added.
     */
    virtual void OnDeviceAdded(const CPeripheralBus &bus, const CPeripheral &peripheral);

    /*!
     * @brief Called when a device has been deleted from a bus.
     * @param bus The bus from which the device removed.
     * @param peripheral The peripheral that has been removed.
     */
    virtual void OnDeviceDeleted(const CPeripheralBus &bus, const CPeripheral &peripheral);

    /*!
     * @brief Creates a new instance of a peripheral.
     * @param bus The bus on which this peripheral is present.
     * @param result The scan result from the device scanning code.
     * @return The new peripheral or NULL if it could not be created.
     */
    CPeripheral *CreatePeripheral(CPeripheralBus &bus, const PeripheralScanResult& result);

    /*!
     * @brief Add the settings that are defined in the mappings file to the peripheral (if there is anything defined).
     * @param peripheral The peripheral to get the settings for.
     */
    void GetSettingsFromMapping(CPeripheral &peripheral) const;

    /*!
     * @brief Trigger a device scan on all known busses
     */
    virtual void TriggerDeviceScan(const PeripheralBusType type = PERIPHERAL_BUS_UNKNOWN);

    /*!
     * @brief Get the instance of a bus given it's type.
     * @param type The bus type.
     * @return The bus or NULL if it wasn't found.
     */
    virtual PeripheralBusPtr GetBusByType(const PeripheralBusType type) const;

    /*!
     * @brief Get all fileitems for a path.
     * @param strPath The path to the directory to get the items from.
     * @param items The item list.
     */
    virtual void GetDirectory(const std::string &strPath, CFileItemList &items) const;

    /*!
     * @brief Get the instance of a peripheral given it's path.
     * @param strPath The path to the peripheral.
     * @return The peripheral or NULL if it wasn't found.
     */
    virtual CPeripheral *GetByPath(const std::string &strPath) const;

    /*!
     * @brief Try to let one of the peripherals handle an action.
     * @param action The change to handle.
     * @return True when this change was handled by a peripheral (and should not be handled by anything else), false otherwise.
     */
    virtual bool OnAction(const CAction &action);

    /*!
     * @brief Check whether there's a peripheral that reports to be muted.
     * @return True when at least one peripheral reports to be muted, false otherwise.
     */
    virtual bool IsMuted();

    /*!
     * @brief Try to toggle the mute status via a peripheral.
     * @return True when this change was handled by a peripheral (and should not be handled by anything else), false otherwise.
     */
    virtual bool ToggleMute();

    /*!
     * @brief Try to toggle the playing device state via a peripheral.
     * @param mode Whether to activate, put on standby or toggle the source.
     * @param iPeripheral Optional CPeripheralCecAdapter pointer to a specific device, instead of iterating through all of them.
     * @return True when the playing device has been switched on, false otherwise.
     */
    virtual bool ToggleDeviceState(const CecStateChange mode = STATE_SWITCH_TOGGLE, const unsigned int iPeripheral = 0);

    /*!
     * @brief Try to mute the audio via a peripheral.
     * @return True when this change was handled by a peripheral (and should not be handled by anything else), false otherwise.
     */
    virtual bool Mute() { return ToggleMute(); } //! @todo CEC only supports toggling the mute status at this time

    /*!
     * @brief Try to unmute the audio via a peripheral.
     * @return True when this change was handled by a peripheral (and should not be handled by anything else), false otherwise.
     */
    virtual bool UnMute() { return ToggleMute(); } //! @todo CEC only supports toggling the mute status at this time

    /*!
     * @brief Try to get a keypress from a peripheral.
     * @param frameTime The current frametime.
     * @param key The fetched key.
     * @return True when a keypress was fetched, false otherwise.
     */
    virtual bool GetNextKeypress(float frameTime, CKey &key);

    /*!
     * @brief Request event scan rate
     * @brief rateHz The rate in Hz
     * @return A handle that unsets its rate when expired
     */
    EventRateHandle SetEventScanRate(float rateHz) { return m_eventScanner.SetRate(rateHz); }

    /*!
     * 
     */
    void OnUserNotification();

    /*!
     * @brief Request peripherals with the specified feature to perform a quick test
     * @return true if any peripherals support the feature, false otherwise
     */
    bool TestFeature(PeripheralFeature feature);

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

    virtual PeripheralAddonPtr GetAddonWithButtonMap(const CPeripheral* device);

    virtual void ResetButtonMaps(const std::string& controllerId);

    void RegisterJoystickButtonMapper(JOYSTICK::IButtonMapper* mapper);
    void UnregisterJoystickButtonMapper(JOYSTICK::IButtonMapper* mapper);

    virtual void OnSettingChanged(const CSetting *setting) override;
    virtual void OnSettingAction(const CSetting *setting) override;

    virtual void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;
    virtual int GetMessageMask() override;

  private:
    CPeripherals();
    bool LoadMappings();
    bool GetMappingForDevice(const CPeripheralBus &bus, PeripheralScanResult& result) const;
    static void GetSettingsFromMappingsFile(TiXmlElement *xmlNode, std::map<std::string, PeripheralDeviceSetting> &m_settings);

    void OnDeviceChanged();

    bool                                 m_bInitialised;
    bool                                 m_bIsStarted;
#if !defined(HAVE_LIBCEC)
    bool                                 m_bMissingLibCecWarningDisplayed;
#endif
    std::vector<PeripheralBusPtr>        m_busses;
    std::vector<PeripheralDeviceMapping> m_mappings;
    CEventScanner                        m_eventScanner;
    CCriticalSection                     m_critSection;
    CCriticalSection                     m_critSectionBusses;
    CCriticalSection                     m_critSectionMappings;
  };
}
