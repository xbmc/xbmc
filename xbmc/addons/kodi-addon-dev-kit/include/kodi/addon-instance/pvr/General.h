/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr/pvr_general.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class PVRTypeIntValue : public CStructHdl<PVRTypeIntValue, PVR_ATTRIBUTE_INT_VALUE>
{
public:
  PVRTypeIntValue(const PVRTypeIntValue& data) : CStructHdl(data) {}
  PVRTypeIntValue(const PVR_ATTRIBUTE_INT_VALUE* data) : CStructHdl(data) {}
  PVRTypeIntValue(PVR_ATTRIBUTE_INT_VALUE* data) : CStructHdl(data) {}

  PVRTypeIntValue() = default;
  PVRTypeIntValue(int value, const std::string& description)
  {
    SetValue(value);
    SetDescription(description);
  }

  void SetValue(int value) { m_cStructure->iValue = value; }

  int GetValue() const { return m_cStructure->iValue; }

  void SetDescription(const std::string& description)
  {
    strncpy(m_cStructure->strDescription, description.c_str(),
            sizeof(m_cStructure->strDescription) - 1);
  }

  std::string GetDescription() const { return m_cStructure->strDescription; }
};

class PVRCapabilities
{
public:
  explicit PVRCapabilities() = delete;
  PVRCapabilities(PVR_ADDON_CAPABILITIES* capabilities) : m_capabilities(capabilities) {}

  void SetSupportsEPG(bool supportsEPG) { m_capabilities->bSupportsEPG = supportsEPG; }
  bool GetSupportsEPG() const { return m_capabilities->bSupportsEPG; }

  void SetSupportsEPGEdl(bool supportsEPGEdl) { m_capabilities->bSupportsEPGEdl = supportsEPGEdl; }
  bool GetSupportsEPGEdl() const { return m_capabilities->bSupportsEPGEdl; }

  void SetSupportsTV(bool supportsTV) { m_capabilities->bSupportsTV = supportsTV; }
  bool GetSupportsTV() const { return m_capabilities->bSupportsTV; }

  void SetSupportsRadio(bool supportsRadio) { m_capabilities->bSupportsRadio = supportsRadio; }
  bool GetSupportsRadio() const { return m_capabilities->bSupportsRadio; }

  void SetSupportsRecordings(bool supportsRecordings)
  {
    m_capabilities->bSupportsRecordings = supportsRecordings;
  }
  bool GetSupportsRecordings() const { return m_capabilities->bSupportsRecordings; }

  void SetSupportsRecordingsUndelete(bool supportsRecordingsUndelete)
  {
    m_capabilities->bSupportsRecordingsUndelete = supportsRecordingsUndelete;
  }
  bool GetSupportsRecordingsUndelete() const { return m_capabilities->bSupportsRecordingsUndelete; }

  void SetSupportsTimers(bool supportsTimers) { m_capabilities->bSupportsTimers = supportsTimers; }
  bool GetSupportsTimers() const { return m_capabilities->bSupportsTimers; }

  void SetSupportsChannelGroups(bool supportsChannelGroups)
  {
    m_capabilities->bSupportsChannelGroups = supportsChannelGroups;
  }
  bool GetSupportsChannelGroups() const { return m_capabilities->bSupportsChannelGroups; }

  void SetSupportsChannelScan(bool supportsChannelScan)
  {
    m_capabilities->bSupportsChannelScan = supportsChannelScan;
  }
  bool GetSupportsChannelScan() const { return m_capabilities->bSupportsChannelScan; }

  void SetSupportsChannelSettings(bool supportsChannelSettings)
  {
    m_capabilities->bSupportsChannelSettings = supportsChannelSettings;
  }
  bool GetSupportsChannelSettings() const { return m_capabilities->bSupportsChannelSettings; }

  void SetHandlesInputStream(bool handlesInputStream)
  {
    m_capabilities->bHandlesInputStream = handlesInputStream;
  }
  bool GetHandlesInputStream() const { return m_capabilities->bHandlesInputStream; }

  void SetHandlesDemuxing(bool handlesDemuxing)
  {
    m_capabilities->bHandlesDemuxing = handlesDemuxing;
  }
  bool GetHandlesDemuxing() const { return m_capabilities->bHandlesDemuxing; }

  void SetSupportsRecordingPlayCount(bool supportsRecordingPlayCount)
  {
    m_capabilities->bSupportsRecordingPlayCount = supportsRecordingPlayCount;
  }
  bool GetSupportsRecordingPlayCount() const { return m_capabilities->bSupportsRecordingPlayCount; }

  void SetSupportsLastPlayedPosition(bool supportsLastPlayedPosition)
  {
    m_capabilities->bSupportsLastPlayedPosition = supportsLastPlayedPosition;
  }
  bool GetSupportsLastPlayedPosition() const { return m_capabilities->bSupportsLastPlayedPosition; }

  void SetSupportsRecordingEdl(bool supportsRecordingEdl)
  {
    m_capabilities->bSupportsRecordingEdl = supportsRecordingEdl;
  }
  bool GetSupportsRecordingEdl() const { return m_capabilities->bSupportsRecordingEdl; }

  void SetSupportsRecordingsRename(bool supportsRecordingsRename)
  {
    m_capabilities->bSupportsRecordingsRename = supportsRecordingsRename;
  }
  bool GetSupportsRecordingsRename() const { return m_capabilities->bSupportsRecordingsRename; }

  void SetSupportsRecordingsLifetimeChange(bool supportsRecordingsLifetimeChange)
  {
    m_capabilities->bSupportsRecordingsLifetimeChange = supportsRecordingsLifetimeChange;
  }
  bool GetSupportsRecordingsLifetimeChange() const
  {
    return m_capabilities->bSupportsRecordingsLifetimeChange;
  }

  void SetSupportsDescrambleInfo(bool supportsDescrambleInfo)
  {
    m_capabilities->bSupportsDescrambleInfo = supportsDescrambleInfo;
  }
  bool GetSupportsDescrambleInfo() const { return m_capabilities->bSupportsDescrambleInfo; }

  void SetSupportsAsyncEPGTransfer(bool supportsAsyncEPGTransfer)
  {
    m_capabilities->bSupportsAsyncEPGTransfer = supportsAsyncEPGTransfer;
  }
  bool GetSupportsAsyncEPGTransfer() const { return m_capabilities->bSupportsAsyncEPGTransfer; }

  void SetSupportsRecordingSize(bool supportsRecordingSize)
  {
    m_capabilities->bSupportsRecordingSize = supportsRecordingSize;
  }
  bool GetSupportsRecordingSize() const { return m_capabilities->bSupportsRecordingSize; }

  void SetRecordingsLifetimeValues(
      const std::vector<PVRTypeIntValue>& recordingsLifetimeValues)
  {
    m_capabilities->iRecordingsLifetimesSize = 0;
    for (unsigned int i = 0; i < recordingsLifetimeValues.size() &&
                             i < sizeof(m_capabilities->recordingsLifetimeValues);
         ++i)
    {
      m_capabilities->recordingsLifetimeValues[i].iValue =
          recordingsLifetimeValues[i].GetCStructure()->iValue;
      strncpy(m_capabilities->recordingsLifetimeValues[i].strDescription,
              recordingsLifetimeValues[i].GetCStructure()->strDescription,
              sizeof(m_capabilities->recordingsLifetimeValues[i].strDescription) - 1);
      ++m_capabilities->iRecordingsLifetimesSize;
    }
  }
  std::vector<PVRTypeIntValue> GetRecordingsLifetimeValues() const
  {
    std::vector<PVRTypeIntValue> recordingsLifetimeValues;
    for (unsigned int i = 0; i < m_capabilities->iRecordingsLifetimesSize; ++i)
      recordingsLifetimeValues.emplace_back(
          m_capabilities->recordingsLifetimeValues[i].iValue,
          m_capabilities->recordingsLifetimeValues[i].strDescription);
    return recordingsLifetimeValues;
  }

private:
  PVR_ADDON_CAPABILITIES* m_capabilities;
};

class PVRStreamProperty : public CStructHdl<PVRStreamProperty, PVR_NAMED_VALUE>
{
public:
  PVRStreamProperty(const PVRStreamProperty& data) : CStructHdl(data) {}
  PVRStreamProperty(const PVR_NAMED_VALUE* data) : CStructHdl(data) {}
  PVRStreamProperty(PVR_NAMED_VALUE* data) : CStructHdl(data) {}

  PVRStreamProperty() = default;
  PVRStreamProperty(const std::string& name, const std::string& value)
  {
    SetName(name);
    SetValue(value);
  }

  void SetName(const std::string& name)
  {
    strncpy(m_cStructure->strName, name.c_str(), sizeof(m_cStructure->strName) - 1);
  }

  std::string GetName() const { return m_cStructure->strName; }

  void SetValue(const std::string& value)
  {
    strncpy(m_cStructure->strValue, value.c_str(), sizeof(m_cStructure->strValue) - 1);
  }

  std::string GetValue() const { return m_cStructure->strValue; }
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
