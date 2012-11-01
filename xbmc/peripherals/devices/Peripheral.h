#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include <set>
#include "utils/StdString.h"
#include "peripherals/PeripheralTypes.h"

class TiXmlDocument;

namespace PERIPHERALS
{
  class CGUIDialogPeripheralSettings;

  class CPeripheral
  {
    friend class CGUIDialogPeripheralSettings;

  public:
    CPeripheral(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId);
    CPeripheral(void);
    virtual ~CPeripheral(void);

    bool operator ==(const CPeripheral &right) const;
    bool operator !=(const CPeripheral &right) const;

    const CStdString &FileLocation(void) const     { return m_strFileLocation; }
    const CStdString &Location(void) const         { return m_strLocation; }
    int VendorId(void) const                       { return m_iVendorId; }
    const char *VendorIdAsString(void) const       { return m_strVendorId.c_str(); }
    int ProductId(void) const                      { return m_iProductId; }
    const char *ProductIdAsString(void) const      { return m_strProductId.c_str(); }
    const PeripheralType Type(void) const          { return m_type; }
    const PeripheralBusType GetBusType(void) const { return m_busType; };
    const CStdString &DeviceName(void) const       { return m_strDeviceName; }
    bool IsHidden(void) const                      { return m_bHidden; }
    void SetHidden(bool bSetTo = true)             { m_bHidden = bSetTo; }
    const CStdString &GetVersionInfo(void) const   { return m_strVersionInfo; }

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
    void GetFeatures(std::vector<PeripheralFeature> &features) const;

    /*!
     * @brief Initialises the peripheral.
     * @return True when the peripheral has been initialised succesfully, false otherwise.
     */
    bool Initialise(void);

    /*!
     * @brief Initialise one of the features of this peripheral.
     * @param feature The feature to initialise.
     * @return True when the feature has been initialised succesfully, false otherwise.
     */
    virtual bool InitialiseFeature(const PeripheralFeature feature) { return true; }

    /*!
     * @brief Called when a setting changed.
     * @param strChangedSetting The changed setting.
     */
    virtual void OnSettingChanged(const CStdString &strChangedSetting) {};

    /*!
     * @brief Called when this device is removed, before calling the destructor.
     */
    virtual void OnDeviceRemoved(void) {}

    /*!
     * @brief Get all subdevices if this device is multifunctional.
     * @param subDevices The subdevices.
     */
    virtual void GetSubdevices(std::vector<CPeripheral *> &subDevices) const;

    /*!
     * @return True when this device is multifunctional, false otherwise.
     */
    virtual bool IsMultiFunctional(void) const;

    /*!
     * @brief Add a setting to this peripheral. This will overwrite a previous setting with the same key.
     * @param strKey The key of the setting.
     * @param setting The setting.
     */
    virtual void AddSetting(const CStdString &strKey, const CSetting *setting);

    /*!
     * @brief Check whether a setting is known with the given key.
     * @param strKey The key to search.
     * @return True when found, false otherwise.
     */
    virtual bool HasSetting(const CStdString &strKey) const;

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
    virtual const CStdString GetSettingString(const CStdString &strKey) const;
    virtual bool SetSetting(const CStdString &strKey, const CStdString &strValue);
    virtual void SetSettingVisible(const CStdString &strKey, bool bSetTo);
    virtual bool IsSettingVisible(const CStdString &strKey) const;

    virtual int GetSettingInt(const CStdString &strKey) const;
    virtual bool SetSetting(const CStdString &strKey, int iValue);

    virtual bool GetSettingBool(const CStdString &strKey) const;
    virtual bool SetSetting(const CStdString &strKey, bool bValue);

    virtual float GetSettingFloat(const CStdString &strKey) const;
    virtual bool SetSetting(const CStdString &strKey, float fValue);

    virtual void PersistSettings(bool bExiting = false);
    virtual void LoadPersistedSettings(void);
    virtual void ResetDefaultSettings(void);

    virtual std::vector<CSetting *> GetSettings(void) const;

    virtual bool ErrorOccured(void) const { return m_bError; }

  protected:
    virtual void ClearSettings(void);

    PeripheralType                   m_type;
    PeripheralBusType                m_busType;
    CStdString                       m_strLocation;
    CStdString                       m_strDeviceName;
    CStdString                       m_strSettingsFile;
    CStdString                       m_strFileLocation;
    int                              m_iVendorId;
    CStdString                       m_strVendorId;
    int                              m_iProductId;
    CStdString                       m_strProductId;
    CStdString                       m_strVersionInfo;
    bool                             m_bInitialised;
    bool                             m_bHidden;
    bool                             m_bError;
    std::vector<PeripheralFeature>   m_features;
    std::vector<CPeripheral *>       m_subDevices;
    std::map<CStdString, CSetting *> m_settings;
    std::set<CStdString>             m_changedSettings;
  };
}
