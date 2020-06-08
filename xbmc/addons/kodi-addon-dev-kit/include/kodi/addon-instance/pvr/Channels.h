/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class PVRChannel : public CStructHdl<PVRChannel, PVR_CHANNEL>
{
public:
  PVRChannel() { memset(m_cStructure, 0, sizeof(PVR_CHANNEL)); }
  PVRChannel(const PVRChannel& channel) : CStructHdl(channel) {}
  PVRChannel(const PVR_CHANNEL* channel) : CStructHdl(channel) {}
  PVRChannel(PVR_CHANNEL* channel) : CStructHdl(channel) {}

  void SetUniqueId(unsigned int uniqueId) { m_cStructure->iUniqueId = uniqueId; }
  unsigned int GetUniqueId() const { return m_cStructure->iUniqueId; }

  void SetIsRadio(bool isRadio) { m_cStructure->bIsRadio = isRadio; }
  bool GetIsRadio() const { return m_cStructure->bIsRadio; }

  void SetChannelNumber(unsigned int channelNumber)
  {
    m_cStructure->iChannelNumber = channelNumber;
  }
  unsigned int GetChannelNumber() const { return m_cStructure->iChannelNumber; }

  void SetSubChannelNumber(unsigned int subChannelNumber)
  {
    m_cStructure->iSubChannelNumber = subChannelNumber;
  }
  unsigned int GetSubChannelNumber() const { return m_cStructure->iSubChannelNumber; }

  void SetChannelName(const std::string& channelName)
  {
    strncpy(m_cStructure->strChannelName, channelName.c_str(),
            sizeof(m_cStructure->strChannelName) - 1);
  }
  std::string GetChannelName() const { return m_cStructure->strChannelName; }

  void SetMimeType(const std::string& inputFormat)
  {
    strncpy(m_cStructure->strMimeType, inputFormat.c_str(), sizeof(m_cStructure->strMimeType) - 1);
  }
  std::string GetMimeType() const { return m_cStructure->strMimeType; }

  void SetEncryptionSystem(unsigned int encryptionSystem)
  {
    m_cStructure->iEncryptionSystem = encryptionSystem;
  }
  unsigned int GetEncryptionSystem() const { return m_cStructure->iEncryptionSystem; }

  void SetIconPath(const std::string& iconPath)
  {
    strncpy(m_cStructure->strIconPath, iconPath.c_str(), sizeof(m_cStructure->strIconPath) - 1);
  }
  std::string GetIconPath() const { return m_cStructure->strIconPath; }

  void SetIsHidden(bool isHidden) { m_cStructure->bIsHidden = isHidden; }
  bool GetIsHidden() const { return m_cStructure->bIsHidden; }

  void SetHasArchive(bool hasArchive) { m_cStructure->bHasArchive = hasArchive; }
  bool GetHasArchive() const { return m_cStructure->bHasArchive; }

  void SetOrder(bool order) { m_cStructure->iOrder = order; }
  bool GetOrder() const { return m_cStructure->iOrder; }
};

class PVRChannelsResultSet
{
public:
  PVRChannelsResultSet() = delete;
  PVRChannelsResultSet(const AddonInstance_PVR* instance, ADDON_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }

  void Add(const kodi::addon::PVRChannel& tag)
  {
    m_instance->toKodi->TransferChannelEntry(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const ADDON_HANDLE m_handle;
};

class PVRSignalStatus : public CStructHdl<PVRSignalStatus, PVR_SIGNAL_STATUS>
{
public:
  PVRSignalStatus() = default;
  PVRSignalStatus(const PVRSignalStatus& type) : CStructHdl(type) {}
  PVRSignalStatus(const PVR_SIGNAL_STATUS* type) : CStructHdl(type) {}
  PVRSignalStatus(PVR_SIGNAL_STATUS* type) : CStructHdl(type) {}

  void SetAdapterName(const std::string& adapterName)
  {
    strncpy(m_cStructure->strAdapterName, adapterName.c_str(),
            sizeof(m_cStructure->strAdapterName) - 1);
  }
  std::string GetAdapterName() const { return m_cStructure->strAdapterName; }

  void SetAdapterStatus(const std::string& adapterStatus)
  {
    strncpy(m_cStructure->strAdapterStatus, adapterStatus.c_str(),
            sizeof(m_cStructure->strAdapterStatus) - 1);
  }
  std::string GetAdapterStatus() const { return m_cStructure->strAdapterStatus; }

  void SetServiceName(const std::string& serviceName)
  {
    strncpy(m_cStructure->strServiceName, serviceName.c_str(),
            sizeof(m_cStructure->strServiceName) - 1);
  }
  std::string GetServiceName() const { return m_cStructure->strServiceName; }

  void SetProviderName(const std::string& providerName)
  {
    strncpy(m_cStructure->strProviderName, providerName.c_str(),
            sizeof(m_cStructure->strProviderName) - 1);
  }
  std::string GetProviderName() const { return m_cStructure->strProviderName; }

  void SetMuxName(const std::string& muxName)
  {
    strncpy(m_cStructure->strMuxName, muxName.c_str(), sizeof(m_cStructure->strMuxName) - 1);
  }
  std::string GetMuxName() const { return m_cStructure->strMuxName; }

  void SetSNR(int snr) { m_cStructure->iSNR = snr; }
  int GetSNR() const { return m_cStructure->iSNR; }

  void SetSignal(int signal) { m_cStructure->iSignal = signal; }
  int GetSignal() const { return m_cStructure->iSignal; }

  void SetBER(long ber) { m_cStructure->iBER = ber; }
  long GetBER() const { return m_cStructure->iBER; }

  void SetUNC(long unc) { m_cStructure->iUNC = unc; }
  long GetUNC() const { return m_cStructure->iUNC; }
};

class PVRDescrambleInfo : public CStructHdl<PVRDescrambleInfo, PVR_DESCRAMBLE_INFO>
{
public:
  PVRDescrambleInfo()
  {
    m_cStructure->iPid = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
    m_cStructure->iCaid = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
    m_cStructure->iProvid = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
    m_cStructure->iEcmTime = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
    m_cStructure->iHops = PVR_DESCRAMBLE_INFO_NOT_AVAILABLE;
  }
  PVRDescrambleInfo(const PVRDescrambleInfo& type) : CStructHdl(type) {}
  PVRDescrambleInfo(const PVR_DESCRAMBLE_INFO* type) : CStructHdl(type) {}
  PVRDescrambleInfo(PVR_DESCRAMBLE_INFO* type) : CStructHdl(type) {}

  void SetPID(int pid) { m_cStructure->iPid = pid; }
  int GetPID() const { return m_cStructure->iPid; }

  void SetCAID(int iCaid) { m_cStructure->iCaid = iCaid; }
  int GetCAID() const { return m_cStructure->iCaid; }

  void SetProviderID(int provid) { m_cStructure->iProvid = provid; }
  int GetProviderID() const { return m_cStructure->iProvid; }

  void SetECMTime(int ecmTime) { m_cStructure->iEcmTime = ecmTime; }
  int GetECMTime() const { return m_cStructure->iEcmTime; }

  void SetHops(int hops) { m_cStructure->iHops = hops; }
  int GetHops() const { return m_cStructure->iHops; }

  void SetCardSystem(const std::string& cardSystem)
  {
    strncpy(m_cStructure->strCardSystem, cardSystem.c_str(),
            sizeof(m_cStructure->strCardSystem) - 1);
  }
  std::string GetCardSystem() const { return m_cStructure->strCardSystem; }

  void SetReader(const std::string& reader)
  {
    strncpy(m_cStructure->strReader, reader.c_str(), sizeof(m_cStructure->strReader) - 1);
  }
  std::string GetReader() const { return m_cStructure->strReader; }

  void SetFrom(const std::string& from)
  {
    strncpy(m_cStructure->strFrom, from.c_str(), sizeof(m_cStructure->strFrom) - 1);
  }
  std::string GetFrom() const { return m_cStructure->strFrom; }

  void SetProtocol(const std::string& protocol)
  {
    strncpy(m_cStructure->strProtocol, protocol.c_str(), sizeof(m_cStructure->strProtocol) - 1);
  }
  std::string GetProtocol() const { return m_cStructure->strProtocol; }
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
