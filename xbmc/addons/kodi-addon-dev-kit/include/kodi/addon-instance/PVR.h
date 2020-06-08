/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../c-api/addon-instance/pvr.h"
#include "pvr/ChannelGroups.h"
#include "pvr/Channels.h"
#include "pvr/EDL.h"
#include "pvr/EPG.h"
#include "pvr/General.h"
#include "pvr/MenuHook.h"
#include "pvr/Recordings.h"
#include "pvr/Stream.h"
#include "pvr/Timers.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class CInstancePVRClient : public IAddonInstance
{
public:
  CInstancePVRClient() : IAddonInstance(ADDON_INSTANCE_PVR, GetKodiTypeVersion(ADDON_INSTANCE_PVR))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePVRClient: Creation of more as one in single "
                             "instance way is not allowed!");

    SetAddonStruct(CAddonBase::m_interface->firstKodiInstance, m_kodiVersion);
    CAddonBase::m_interface->globalSingleInstance = this;
  }

  explicit CInstancePVRClient(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_PVR,
                     !kodiVersion.empty() ? kodiVersion : GetKodiTypeVersion(ADDON_INSTANCE_PVR))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePVRClient: Creation of multiple together with "
                             "single instance way is not allowed!");

    SetAddonStruct(instance, m_kodiVersion);
  }

  ~CInstancePVRClient() override = default;

  virtual PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) = 0;
  virtual PVR_ERROR GetBackendName(std::string& name) = 0;
  virtual PVR_ERROR GetBackendVersion(std::string& version) = 0;
  virtual PVR_ERROR GetBackendHostname(std::string& hostname) { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR GetConnectionString(std::string& connection)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetDriveSpace(uint64_t& total, uint64_t& used)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR CallSettingsMenuHook(const kodi::addon::PVRMenuhook& menuhook)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  inline void AddMenuHook(kodi::addon::PVRMenuhook& hook)
  {
    m_instanceData->toKodi->AddMenuHook(m_instanceData->toKodi->kodiInstance, hook);
  }
  inline void ConnectionStateChange(const std::string& connectionString,
                                    PVR_CONNECTION_STATE newState,
                                    const std::string& message)
  {
    m_instanceData->toKodi->ConnectionStateChange(
        m_instanceData->toKodi->kodiInstance, connectionString.c_str(), newState, message.c_str());
  }
  inline std::string UserPath() const { return m_instanceData->props->strUserPath; }
  inline std::string ClientPath() const { return m_instanceData->props->strClientPath; }

  virtual PVR_ERROR GetChannelsAmount(int& amount) { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetChannelStreamProperties(
      const kodi::addon::PVRChannel& channel,
      std::vector<kodi::addon::PVRStreamProperty>& properties)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetDescrambleInfo(int channelUid,
                                      kodi::addon::PVRDescrambleInfo& descrambleInfo)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  inline void TriggerChannelUpdate()
  {
    m_instanceData->toKodi->TriggerChannelUpdate(m_instanceData->toKodi->kodiInstance);
  }

  virtual PVR_ERROR GetChannelGroupsAmount(int& amount) { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group,
                                           kodi::addon::PVRChannelGroupMembersResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  inline void TriggerChannelGroupsUpdate()
  {
    m_instanceData->toKodi->TriggerChannelGroupsUpdate(m_instanceData->toKodi->kodiInstance);
  }

  virtual PVR_ERROR DeleteChannel(const kodi::addon::PVRChannel& channel)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR RenameChannel(const kodi::addon::PVRChannel& channel)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR OpenDialogChannelSettings(const kodi::addon::PVRChannel& channel)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR OpenDialogChannelAdd(const kodi::addon::PVRChannel& channel)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR OpenDialogChannelScan() { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR CallChannelMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                        const kodi::addon::PVRChannel& item)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }

  virtual PVR_ERROR GetEPGForChannel(int channelUid,
                                     time_t start,
                                     time_t end,
                                     kodi::addon::PVREPGTagsResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR IsEPGTagRecordable(const kodi::addon::PVREPGTag& tag, bool& isRecordable)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR IsEPGTagPlayable(const kodi::addon::PVREPGTag& tag, bool& isPlayable)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetEPGTagEdl(const kodi::addon::PVREPGTag& tag,
                                 std::vector<kodi::addon::PVREDLEntry>& edl)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetEPGTagStreamProperties(
      const kodi::addon::PVREPGTag& tag, std::vector<kodi::addon::PVRStreamProperty>& properties)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR SetEPGTimeFrame(int days) { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR CallEPGMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                    const kodi::addon::PVREPGTag& tag)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  inline int EpgMaxDays() const { return m_instanceData->props->iEpgMaxDays; }
  inline void TriggerEpgUpdate(unsigned int channelUid)
  {
    m_instanceData->toKodi->TriggerEpgUpdate(m_instanceData->toKodi->kodiInstance, channelUid);
  }
  inline void EpgEventStateChange(kodi::addon::PVREPGTag& tag, EPG_EVENT_STATE newState)
  {
    m_instanceData->toKodi->EpgEventStateChange(m_instanceData->toKodi->kodiInstance, tag.GetTag(),
                                                newState);
  }

  virtual PVR_ERROR GetRecordingsAmount(bool deleted, int& amount)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR DeleteRecording(const kodi::addon::PVRRecording& recording)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR UndeleteRecording(const kodi::addon::PVRRecording& recording)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR RenameRecording(const kodi::addon::PVRRecording& recording)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR SetRecordingLifetime(const kodi::addon::PVRRecording& recording)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR SetRecordingPlayCount(const kodi::addon::PVRRecording& recording, int count)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR SetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording,
                                                   int lastplayedposition)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording,
                                                   int& position)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetRecordingEdl(const kodi::addon::PVRRecording& recording,
                                    std::vector<kodi::addon::PVREDLEntry>& edl)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetRecordingSize(const kodi::addon::PVRRecording& recording, int64_t& size)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetRecordingStreamProperties(
      const kodi::addon::PVRRecording& recording,
      std::vector<kodi::addon::PVRStreamProperty>& properties)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR CallRecordingMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                          const kodi::addon::PVRRecording& item)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  inline void RecordingNotification(const std::string& recordingName,
                                    const std::string& fileName,
                                    bool on)
  {
    m_instanceData->toKodi->RecordingNotification(m_instanceData->toKodi->kodiInstance,
                                                  recordingName.c_str(), fileName.c_str(), on);
  }
  inline void TriggerRecordingUpdate()
  {
    m_instanceData->toKodi->TriggerRecordingUpdate(m_instanceData->toKodi->kodiInstance);
  }

  virtual PVR_ERROR GetTimerTypes(std::vector<kodi::addon::PVRTimerType>& types)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetTimersAmount(int& amount) { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR GetTimers(kodi::addon::PVRTimersResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR AddTimer(const kodi::addon::PVRTimer& timer)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR DeleteTimer(const kodi::addon::PVRTimer& timer, bool forceDelete)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR UpdateTimer(const kodi::addon::PVRTimer& timer)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR CallTimerMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                      const kodi::addon::PVRTimer& item)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  inline void TriggerTimerUpdate()
  {
    m_instanceData->toKodi->TriggerTimerUpdate(m_instanceData->toKodi->kodiInstance);
  }

  virtual PVR_ERROR OnSystemSleep() { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR OnSystemWake() { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR OnPowerSavingActivated() { return PVR_ERROR_NOT_IMPLEMENTED; }
  virtual PVR_ERROR OnPowerSavingDeactivated() { return PVR_ERROR_NOT_IMPLEMENTED; }

  virtual bool OpenLiveStream(const kodi::addon::PVRChannel& channel) { return false; }
  virtual void CloseLiveStream() {}
  virtual int ReadLiveStream(unsigned char* buffer, unsigned int size) { return 0; }
  virtual int64_t SeekLiveStream(int64_t position, int whence) { return 0; }
  virtual int64_t LengthLiveStream() { return 0; }
  virtual PVR_ERROR GetStreamProperties(std::vector<kodi::addon::PVRStreamProperties>& properties)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual DemuxPacket* DemuxRead() { return nullptr; }
  virtual void DemuxReset() {}
  virtual void DemuxAbort() {}
  virtual void DemuxFlush() {}
  virtual void SetSpeed(int speed) {}
  virtual void FillBuffer(bool mode) {}
  virtual bool SeekTime(double time, bool backwards, double& startpts) { return false; }
  inline PVRCodec GetCodecByName(const std::string& codecName) const
  {
    return PVRCodec(m_instanceData->toKodi->GetCodecByName(m_instanceData->toKodi->kodiInstance,
                                                           codecName.c_str()));
  }
  inline DemuxPacket* AllocateDemuxPacket(int iDataSize)
  {
    return m_instanceData->toKodi->AllocateDemuxPacket(m_instanceData->toKodi->kodiInstance,
                                                       iDataSize);
  }
  inline void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    m_instanceData->toKodi->FreeDemuxPacket(m_instanceData->toKodi->kodiInstance, pPacket);
  }

  virtual bool OpenRecordedStream(const kodi::addon::PVRRecording& recording) { return false; }
  virtual void CloseRecordedStream() {}
  virtual int ReadRecordedStream(unsigned char* buffer, unsigned int size) { return 0; }
  virtual int64_t SeekRecordedStream(int64_t position, int whence) { return 0; }
  virtual int64_t LengthRecordedStream() { return 0; }

  virtual bool CanPauseStream() { return false; }
  virtual bool CanSeekStream() { return false; }
  virtual void PauseStream(bool paused) {}
  virtual bool IsRealTimeStream() { return false; }
  virtual PVR_ERROR GetStreamTimes(kodi::addon::PVRStreamTimes& times)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  virtual PVR_ERROR GetStreamReadChunkSize(int& chunksize) { return PVR_ERROR_NOT_IMPLEMENTED; }

private:
  void SetAddonStruct(KODI_HANDLE instance, const std::string& kodiVersion)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstancePVRClient: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_PVR*>(instance);
    m_instanceData->toAddon->addonInstance = this;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->GetCapabilities = ADDON_GetCapabilities;
    m_instanceData->toAddon->GetConnectionString = ADDON_GetConnectionString;
    m_instanceData->toAddon->GetBackendName = ADDON_GetBackendName;
    m_instanceData->toAddon->GetBackendVersion = ADDON_GetBackendVersion;
    m_instanceData->toAddon->GetBackendHostname = ADDON_GetBackendHostname;
    m_instanceData->toAddon->GetDriveSpace = ADDON_GetDriveSpace;
    m_instanceData->toAddon->CallSettingsMenuHook = ADDON_CallSettingsMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->GetChannelsAmount = ADDON_GetChannelsAmount;
    m_instanceData->toAddon->GetChannels = ADDON_GetChannels;
    m_instanceData->toAddon->GetChannelStreamProperties = ADDON_GetChannelStreamProperties;
    m_instanceData->toAddon->GetSignalStatus = ADDON_GetSignalStatus;
    m_instanceData->toAddon->GetDescrambleInfo = ADDON_GetDescrambleInfo;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->GetChannelGroupsAmount = ADDON_GetChannelGroupsAmount;
    m_instanceData->toAddon->GetChannelGroups = ADDON_GetChannelGroups;
    m_instanceData->toAddon->GetChannelGroupMembers = ADDON_GetChannelGroupMembers;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->DeleteChannel = ADDON_DeleteChannel;
    m_instanceData->toAddon->RenameChannel = ADDON_RenameChannel;
    m_instanceData->toAddon->OpenDialogChannelSettings = ADDON_OpenDialogChannelSettings;
    m_instanceData->toAddon->OpenDialogChannelAdd = ADDON_OpenDialogChannelAdd;
    m_instanceData->toAddon->OpenDialogChannelScan = ADDON_OpenDialogChannelScan;
    m_instanceData->toAddon->CallChannelMenuHook = ADDON_CallChannelMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->GetEPGForChannel = ADDON_GetEPGForChannel;
    m_instanceData->toAddon->IsEPGTagRecordable = ADDON_IsEPGTagRecordable;
    m_instanceData->toAddon->IsEPGTagPlayable = ADDON_IsEPGTagPlayable;
    m_instanceData->toAddon->GetEPGTagEdl = ADDON_GetEPGTagEdl;
    m_instanceData->toAddon->GetEPGTagStreamProperties = ADDON_GetEPGTagStreamProperties;
    m_instanceData->toAddon->SetEPGTimeFrame = ADDON_SetEPGTimeFrame;
    m_instanceData->toAddon->CallEPGMenuHook = ADDON_CallEPGMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->GetRecordingsAmount = ADDON_GetRecordingsAmount;
    m_instanceData->toAddon->GetRecordings = ADDON_GetRecordings;
    m_instanceData->toAddon->DeleteRecording = ADDON_DeleteRecording;
    m_instanceData->toAddon->UndeleteRecording = ADDON_UndeleteRecording;
    m_instanceData->toAddon->DeleteAllRecordingsFromTrash = ADDON_DeleteAllRecordingsFromTrash;
    m_instanceData->toAddon->RenameRecording = ADDON_RenameRecording;
    m_instanceData->toAddon->SetRecordingLifetime = ADDON_SetRecordingLifetime;
    m_instanceData->toAddon->SetRecordingPlayCount = ADDON_SetRecordingPlayCount;
    m_instanceData->toAddon->SetRecordingLastPlayedPosition = ADDON_SetRecordingLastPlayedPosition;
    m_instanceData->toAddon->GetRecordingLastPlayedPosition = ADDON_GetRecordingLastPlayedPosition;
    m_instanceData->toAddon->GetRecordingEdl = ADDON_GetRecordingEdl;
    m_instanceData->toAddon->GetRecordingSize = ADDON_GetRecordingSize;
    m_instanceData->toAddon->GetRecordingStreamProperties = ADDON_GetRecordingStreamProperties;
    m_instanceData->toAddon->CallRecordingMenuHook = ADDON_CallRecordingMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->GetTimerTypes = ADDON_GetTimerTypes;
    m_instanceData->toAddon->GetTimersAmount = ADDON_GetTimersAmount;
    m_instanceData->toAddon->GetTimers = ADDON_GetTimers;
    m_instanceData->toAddon->AddTimer = ADDON_AddTimer;
    m_instanceData->toAddon->DeleteTimer = ADDON_DeleteTimer;
    m_instanceData->toAddon->UpdateTimer = ADDON_UpdateTimer;
    m_instanceData->toAddon->CallTimerMenuHook = ADDON_CallTimerMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->OnSystemSleep = ADDON_OnSystemSleep;
    m_instanceData->toAddon->OnSystemWake = ADDON_OnSystemWake;
    m_instanceData->toAddon->OnPowerSavingActivated = ADDON_OnPowerSavingActivated;
    m_instanceData->toAddon->OnPowerSavingDeactivated = ADDON_OnPowerSavingDeactivated;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->OpenLiveStream = ADDON_OpenLiveStream;
    m_instanceData->toAddon->CloseLiveStream = ADDON_CloseLiveStream;
    m_instanceData->toAddon->ReadLiveStream = ADDON_ReadLiveStream;
    m_instanceData->toAddon->SeekLiveStream = ADDON_SeekLiveStream;
    m_instanceData->toAddon->LengthLiveStream = ADDON_LengthLiveStream;
    m_instanceData->toAddon->GetStreamProperties = ADDON_GetStreamProperties;
    m_instanceData->toAddon->GetStreamReadChunkSize = ADDON_GetStreamReadChunkSize;
    m_instanceData->toAddon->IsRealTimeStream = ADDON_IsRealTimeStream;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->OpenRecordedStream = ADDON_OpenRecordedStream;
    m_instanceData->toAddon->CloseRecordedStream = ADDON_CloseRecordedStream;
    m_instanceData->toAddon->ReadRecordedStream = ADDON_ReadRecordedStream;
    m_instanceData->toAddon->SeekRecordedStream = ADDON_SeekRecordedStream;
    m_instanceData->toAddon->LengthRecordedStream = ADDON_LengthRecordedStream;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->DemuxReset = ADDON_DemuxReset;
    m_instanceData->toAddon->DemuxAbort = ADDON_DemuxAbort;
    m_instanceData->toAddon->DemuxFlush = ADDON_DemuxFlush;
    m_instanceData->toAddon->DemuxRead = ADDON_DemuxRead;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    m_instanceData->toAddon->CanPauseStream = ADDON_CanPauseStream;
    m_instanceData->toAddon->PauseStream = ADDON_PauseStream;
    m_instanceData->toAddon->CanSeekStream = ADDON_CanSeekStream;
    m_instanceData->toAddon->SeekTime = ADDON_SeekTime;
    m_instanceData->toAddon->SetSpeed = ADDON_SetSpeed;
    m_instanceData->toAddon->FillBuffer = ADDON_FillBuffer;
    m_instanceData->toAddon->GetStreamTimes = ADDON_GetStreamTimes;
  }

  inline static PVR_ERROR ADDON_GetCapabilities(const AddonInstance_PVR* instance,
                                                PVR_ADDON_CAPABILITIES* capabilities)
  {
    PVRCapabilities cppCapabilities(capabilities);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetCapabilities(cppCapabilities);
  }

  inline static PVR_ERROR ADDON_GetBackendName(const AddonInstance_PVR* instance,
                                               char* str,
                                               int memSize)
  {
    std::string backendName;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetBackendName(backendName);
    if (err == PVR_ERROR_NO_ERROR)
      strncpy(str, backendName.c_str(), memSize);
    return err;
  }

  inline static PVR_ERROR ADDON_GetBackendVersion(const AddonInstance_PVR* instance,
                                                  char* str,
                                                  int memSize)
  {
    std::string backendVersion;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetBackendVersion(backendVersion);
    if (err == PVR_ERROR_NO_ERROR)
      strncpy(str, backendVersion.c_str(), memSize);
    return err;
  }

  inline static PVR_ERROR ADDON_GetBackendHostname(const AddonInstance_PVR* instance,
                                                   char* str,
                                                   int memSize)
  {
    std::string backendHostname;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetBackendHostname(backendHostname);
    if (err == PVR_ERROR_NO_ERROR)
      strncpy(str, backendHostname.c_str(), memSize);
    return err;
  }

  inline static PVR_ERROR ADDON_GetConnectionString(const AddonInstance_PVR* instance,
                                                    char* str,
                                                    int memSize)
  {
    std::string connectionString;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetConnectionString(connectionString);
    if (err == PVR_ERROR_NO_ERROR)
      strncpy(str, connectionString.c_str(), memSize);
    return err;
  }

  inline static PVR_ERROR ADDON_GetDriveSpace(const AddonInstance_PVR* instance,
                                              uint64_t* total,
                                              uint64_t* used)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetDriveSpace(*total, *used);
  }

  inline static PVR_ERROR ADDON_CallSettingsMenuHook(const AddonInstance_PVR* instance,
                                                     const PVR_MENUHOOK* menuhook)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallSettingsMenuHook(menuhook);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetChannelsAmount(const AddonInstance_PVR* instance, int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannelsAmount(*amount);
  }

  inline static PVR_ERROR ADDON_GetChannels(const AddonInstance_PVR* instance,
                                            ADDON_HANDLE handle,
                                            bool radio)
  {
    PVRChannelsResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannels(radio, result);
  }

  inline static PVR_ERROR ADDON_GetChannelStreamProperties(const AddonInstance_PVR* instance,
                                                           const PVR_CHANNEL* channel,
                                                           PVR_NAMED_VALUE* properties,
                                                           unsigned int* propertiesCount)
  {
    *propertiesCount = 0;
    std::vector<PVRStreamProperty> propertiesList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetChannelStreamProperties(channel, propertiesList);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& property : propertiesList)
      {
        strncpy(properties[*propertiesCount].strName, property.GetCStructure()->strName,
                sizeof(properties[*propertiesCount].strName) - 1);
        strncpy(properties[*propertiesCount].strValue, property.GetCStructure()->strValue,
                sizeof(properties[*propertiesCount].strValue) - 1);
        ++*propertiesCount;
        if (*propertiesCount > STREAM_MAX_PROPERTY_COUNT)
          break;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_GetSignalStatus(const AddonInstance_PVR* instance,
                                                int channelUid,
                                                PVR_SIGNAL_STATUS* signalStatus)
  {
    PVRSignalStatus cppSignalStatus(signalStatus);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetSignalStatus(channelUid, cppSignalStatus);
  }

  inline static PVR_ERROR ADDON_GetDescrambleInfo(const AddonInstance_PVR* instance,
                                                  int channelUid,
                                                  PVR_DESCRAMBLE_INFO* descrambleInfo)
  {
    PVRDescrambleInfo cppDescrambleInfo(descrambleInfo);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetDescrambleInfo(channelUid, cppDescrambleInfo);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetChannelGroupsAmount(const AddonInstance_PVR* instance,
                                                       int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannelGroupsAmount(*amount);
  }

  inline static PVR_ERROR ADDON_GetChannelGroups(const AddonInstance_PVR* instance,
                                                 ADDON_HANDLE handle,
                                                 bool radio)
  {
    PVRChannelGroupsResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannelGroups(radio, result);
  }

  inline static PVR_ERROR ADDON_GetChannelGroupMembers(const AddonInstance_PVR* instance,
                                                       ADDON_HANDLE handle,
                                                       const PVR_CHANNEL_GROUP* group)
  {
    PVRChannelGroupMembersResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannelGroupMembers(group, result);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_DeleteChannel(const AddonInstance_PVR* instance,
                                              const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->DeleteChannel(channel);
  }

  inline static PVR_ERROR ADDON_RenameChannel(const AddonInstance_PVR* instance,
                                              const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->RenameChannel(channel);
  }

  inline static PVR_ERROR ADDON_OpenDialogChannelSettings(const AddonInstance_PVR* instance,
                                                          const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenDialogChannelSettings(channel);
  }

  inline static PVR_ERROR ADDON_OpenDialogChannelAdd(const AddonInstance_PVR* instance,
                                                     const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenDialogChannelAdd(channel);
  }

  inline static PVR_ERROR ADDON_OpenDialogChannelScan(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenDialogChannelScan();
  }

  inline static PVR_ERROR ADDON_CallChannelMenuHook(const AddonInstance_PVR* instance,
                                                    const PVR_MENUHOOK* menuhook,
                                                    const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallChannelMenuHook(menuhook, channel);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetEPGForChannel(const AddonInstance_PVR* instance,
                                                 ADDON_HANDLE handle,
                                                 int channelUid,
                                                 time_t start,
                                                 time_t end)
  {
    PVREPGTagsResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetEPGForChannel(channelUid, start, end, result);
  }

  inline static PVR_ERROR ADDON_IsEPGTagRecordable(const AddonInstance_PVR* instance,
                                                   const EPG_TAG* tag,
                                                   bool* isRecordable)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->IsEPGTagRecordable(tag, *isRecordable);
  }

  inline static PVR_ERROR ADDON_IsEPGTagPlayable(const AddonInstance_PVR* instance,
                                                 const EPG_TAG* tag,
                                                 bool* isPlayable)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->IsEPGTagPlayable(tag, *isPlayable);
  }

  inline static PVR_ERROR ADDON_GetEPGTagEdl(const AddonInstance_PVR* instance,
                                             const EPG_TAG* tag,
                                             PVR_EDL_ENTRY* edl,
                                             int* size)
  {
    *size = 0;
    std::vector<PVREDLEntry> edlList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetEPGTagEdl(tag, edlList);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& edlEntry : edlList)
      {
        edl[*size] = *edlEntry;
        ++*size;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_GetEPGTagStreamProperties(const AddonInstance_PVR* instance,
                                                          const EPG_TAG* tag,
                                                          PVR_NAMED_VALUE* properties,
                                                          unsigned int* propertiesCount)
  {
    *propertiesCount = 0;
    std::vector<PVRStreamProperty> propertiesList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetEPGTagStreamProperties(tag, propertiesList);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& property : propertiesList)
      {
        strncpy(properties[*propertiesCount].strName, property.GetCStructure()->strName,
                sizeof(properties[*propertiesCount].strName) - 1);
        strncpy(properties[*propertiesCount].strValue, property.GetCStructure()->strValue,
                sizeof(properties[*propertiesCount].strValue) - 1);
        ++*propertiesCount;
        if (*propertiesCount > STREAM_MAX_PROPERTY_COUNT)
          break;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_SetEPGTimeFrame(const AddonInstance_PVR* instance, int days)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetEPGTimeFrame(days);
  }

  inline static PVR_ERROR ADDON_CallEPGMenuHook(const AddonInstance_PVR* instance,
                                                const PVR_MENUHOOK* menuhook,
                                                const EPG_TAG* tag)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallEPGMenuHook(menuhook, tag);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetRecordingsAmount(const AddonInstance_PVR* instance,
                                                    bool deleted,
                                                    int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetRecordingsAmount(deleted, *amount);
  }

  inline static PVR_ERROR ADDON_GetRecordings(const AddonInstance_PVR* instance,
                                              ADDON_HANDLE handle,
                                              bool deleted)
  {
    PVRRecordingsResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetRecordings(deleted, result);
  }

  inline static PVR_ERROR ADDON_DeleteRecording(const AddonInstance_PVR* instance,
                                                const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->DeleteRecording(recording);
  }

  inline static PVR_ERROR ADDON_UndeleteRecording(const AddonInstance_PVR* instance,
                                                  const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->UndeleteRecording(recording);
  }

  inline static PVR_ERROR ADDON_DeleteAllRecordingsFromTrash(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->DeleteAllRecordingsFromTrash();
  }

  inline static PVR_ERROR ADDON_RenameRecording(const AddonInstance_PVR* instance,
                                                const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->RenameRecording(recording);
  }

  inline static PVR_ERROR ADDON_SetRecordingLifetime(const AddonInstance_PVR* instance,
                                                     const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetRecordingLifetime(recording);
  }

  inline static PVR_ERROR ADDON_SetRecordingPlayCount(const AddonInstance_PVR* instance,
                                                      const PVR_RECORDING* recording,
                                                      int count)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetRecordingPlayCount(recording, count);
  }

  inline static PVR_ERROR ADDON_SetRecordingLastPlayedPosition(const AddonInstance_PVR* instance,
                                                               const PVR_RECORDING* recording,
                                                               int lastplayedposition)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetRecordingLastPlayedPosition(recording, lastplayedposition);
  }

  inline static PVR_ERROR ADDON_GetRecordingLastPlayedPosition(const AddonInstance_PVR* instance,
                                                               const PVR_RECORDING* recording,
                                                               int* position)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetRecordingLastPlayedPosition(recording, *position);
  }

  inline static PVR_ERROR ADDON_GetRecordingEdl(const AddonInstance_PVR* instance,
                                                const PVR_RECORDING* recording,
                                                PVR_EDL_ENTRY* edl,
                                                int* size)
  {
    *size = 0;
    std::vector<PVREDLEntry> edlList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetRecordingEdl(recording, edlList);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& edlEntry : edlList)
      {
        edl[*size] = *edlEntry;
        ++*size;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_GetRecordingSize(const AddonInstance_PVR* instance,
                                                 const PVR_RECORDING* recording,
                                                 int64_t* size)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetRecordingSize(recording, *size);
  }

  inline static PVR_ERROR ADDON_GetRecordingStreamProperties(const AddonInstance_PVR* instance,
                                                             const PVR_RECORDING* recording,
                                                             PVR_NAMED_VALUE* properties,
                                                             unsigned int* propertiesCount)
  {
    *propertiesCount = 0;
    std::vector<PVRStreamProperty> propertiesList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetRecordingStreamProperties(recording, propertiesList);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& property : propertiesList)
      {
        strncpy(properties[*propertiesCount].strName, property.GetCStructure()->strName,
                sizeof(properties[*propertiesCount].strName) - 1);
        strncpy(properties[*propertiesCount].strValue, property.GetCStructure()->strValue,
                sizeof(properties[*propertiesCount].strValue) - 1);
        ++*propertiesCount;
        if (*propertiesCount > STREAM_MAX_PROPERTY_COUNT)
          break;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_CallRecordingMenuHook(const AddonInstance_PVR* instance,
                                                      const PVR_MENUHOOK* menuhook,
                                                      const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallRecordingMenuHook(menuhook, recording);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==


  inline static PVR_ERROR ADDON_GetTimerTypes(const AddonInstance_PVR* instance,
                                              PVR_TIMER_TYPE* types,
                                              int* typesCount)
  {
    *typesCount = 0;
    std::vector<PVRTimerType> timerTypes;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetTimerTypes(timerTypes);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& timerType : timerTypes)
      {
        types[*typesCount] = *timerType;
        ++*typesCount;
        if (*typesCount >= PVR_ADDON_TIMERTYPE_ARRAY_SIZE)
          break;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_GetTimersAmount(const AddonInstance_PVR* instance, int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetTimersAmount(*amount);
  }

  inline static PVR_ERROR ADDON_GetTimers(const AddonInstance_PVR* instance, ADDON_HANDLE handle)
  {
    PVRTimersResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->GetTimers(result);
  }

  inline static PVR_ERROR ADDON_AddTimer(const AddonInstance_PVR* instance, const PVR_TIMER* timer)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->AddTimer(timer);
  }

  inline static PVR_ERROR ADDON_DeleteTimer(const AddonInstance_PVR* instance,
                                            const PVR_TIMER* timer,
                                            bool forceDelete)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->DeleteTimer(timer, forceDelete);
  }

  inline static PVR_ERROR ADDON_UpdateTimer(const AddonInstance_PVR* instance,
                                            const PVR_TIMER* timer)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->UpdateTimer(timer);
  }

  inline static PVR_ERROR ADDON_CallTimerMenuHook(const AddonInstance_PVR* instance,
                                                  const PVR_MENUHOOK* menuhook,
                                                  const PVR_TIMER* timer)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallTimerMenuHook(menuhook, timer);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_OnSystemSleep(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->OnSystemSleep();
  }

  inline static PVR_ERROR ADDON_OnSystemWake(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->OnSystemWake();
  }

  inline static PVR_ERROR ADDON_OnPowerSavingActivated(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OnPowerSavingActivated();
  }

  inline static PVR_ERROR ADDON_OnPowerSavingDeactivated(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OnPowerSavingDeactivated();
  }

  inline static bool ADDON_OpenLiveStream(const AddonInstance_PVR* instance,
                                          const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenLiveStream(channel);
  }

  inline static void ADDON_CloseLiveStream(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->CloseLiveStream();
  }

  inline static int ADDON_ReadLiveStream(const AddonInstance_PVR* instance,
                                         unsigned char* buffer,
                                         unsigned int size)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->ReadLiveStream(buffer, size);
  }

  inline static int64_t ADDON_SeekLiveStream(const AddonInstance_PVR* instance,
                                             int64_t position,
                                             int whence)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SeekLiveStream(position, whence);
  }

  inline static int64_t ADDON_LengthLiveStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->LengthLiveStream();
  }

  inline static PVR_ERROR ADDON_GetStreamProperties(const AddonInstance_PVR* instance,
                                                    PVR_STREAM_PROPERTIES* properties)
  {
    properties->iStreamCount = 0;
    std::vector<PVRStreamProperties> cppProperties;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetStreamProperties(cppProperties);
    if (err == PVR_ERROR_NO_ERROR)
    {
      for (unsigned int i = 0; i < cppProperties.size(); ++i)
      {
        memcpy(&properties->stream[i],
               static_cast<PVR_STREAM_PROPERTIES::PVR_STREAM*>(cppProperties[i]),
               sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
        ++properties->iStreamCount;

        if (properties->iStreamCount >= PVR_STREAM_MAX_STREAMS)
        {
          kodi::Log(
              ADDON_LOG_ERROR,
              "CInstancePVRClient::%s: Addon given with '%li' more allowed streams where '%i'",
              __func__, cppProperties.size(), PVR_STREAM_MAX_STREAMS);
          break;
        }
      }
    }

    return err;
  }

  inline static PVR_ERROR ADDON_GetStreamReadChunkSize(const AddonInstance_PVR* instance,
                                                       int* chunksize)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetStreamReadChunkSize(*chunksize);
  }

  inline static bool ADDON_IsRealTimeStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->IsRealTimeStream();
  }

  inline static bool ADDON_OpenRecordedStream(const AddonInstance_PVR* instance,
                                              const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenRecordedStream(recording);
  }

  inline static void ADDON_CloseRecordedStream(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->CloseRecordedStream();
  }

  inline static int ADDON_ReadRecordedStream(const AddonInstance_PVR* instance,
                                             unsigned char* buffer,
                                             unsigned int size)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->ReadRecordedStream(buffer, size);
  }

  inline static int64_t ADDON_SeekRecordedStream(const AddonInstance_PVR* instance,
                                                 int64_t position,
                                                 int whence)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SeekRecordedStream(position, whence);
  }

  inline static int64_t ADDON_LengthRecordedStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->LengthRecordedStream();
  }

  inline static void ADDON_DemuxReset(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->DemuxReset();
  }

  inline static void ADDON_DemuxAbort(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->DemuxAbort();
  }

  inline static void ADDON_DemuxFlush(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->DemuxFlush();
  }

  inline static DemuxPacket* ADDON_DemuxRead(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->DemuxRead();
  }

  inline static bool ADDON_CanPauseStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->CanPauseStream();
  }

  inline static bool ADDON_CanSeekStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->CanSeekStream();
  }

  inline static void ADDON_PauseStream(const AddonInstance_PVR* instance, bool bPaused)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->PauseStream(bPaused);
  }

  inline static bool ADDON_SeekTime(const AddonInstance_PVR* instance,
                                    double time,
                                    bool backwards,
                                    double* startpts)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SeekTime(time, backwards, *startpts);
  }

  inline static void ADDON_SetSpeed(const AddonInstance_PVR* instance, int speed)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->SetSpeed(speed);
  }

  inline static void ADDON_FillBuffer(const AddonInstance_PVR* instance, bool mode)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->FillBuffer(mode);
  }

  inline static PVR_ERROR ADDON_GetStreamTimes(const AddonInstance_PVR* instance,
                                               PVR_STREAM_TIMES* times)
  {
    PVRStreamTimes cppTimes(times);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetStreamTimes(cppTimes);
  }

  AddonInstance_PVR* m_instanceData = nullptr;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
