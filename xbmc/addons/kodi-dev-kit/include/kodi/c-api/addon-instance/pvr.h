/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_H
#define C_API_ADDONINSTANCE_PVR_H

#include "../../AddonBase.h"
#include "pvr/pvr_channel_groups.h"
#include "pvr/pvr_channels.h"
#include "pvr/pvr_defines.h"
#include "pvr/pvr_edl.h"
#include "pvr/pvr_epg.h"
#include "pvr/pvr_general.h"
#include "pvr/pvr_menu_hook.h"
#include "pvr/pvr_providers.h"
#include "pvr/pvr_recordings.h"
#include "pvr/pvr_stream.h"
#include "pvr/pvr_timers.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" main interface function tables between Kodi and addon
//
// Values related to all parts and not used direct on addon, are to define here.
//
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  /*!
   * @internal
   * @brief PVR "C" basis API interface
   *
   * This field contains things that are exchanged between Kodi and Addon
   * and is the basis of the PVR-side "C" API.
   *
   * @warning Care should be taken when making changes in this fields!\n
   * Changes can destroy API in addons that have already been created. If a
   * necessary change or new feature is added, the version of the PVR
   * at @ref ADDON_INSTANCE_VERSION_PVR_MIN must be increased too.\n
   * \n
   * Conditional changes can be made in some places, without min PVR version
   * increase. The add-on should then use CreateInstanceEx and add partial tests
   * for this in the C++ header.
   *
   * Have by add of new parts a look about **Doxygen** `\\ingroup`, so that
   * added parts included in documentation.
   *
   * If you add addon side related documentation, where his dev need know,
   * use `///`. For parts only for Kodi make it like here.
   *
   * @endinternal
   */

  struct AddonInstance_PVR;

  /*!
   * @brief Structure to define typical standard values
   */
  typedef struct AddonProperties_PVR
  {
    const char* strUserPath;
    const char* strClientPath;
    int iEpgMaxFutureDays;
    int iEpgMaxPastDays;
  } AddonProperties_PVR;

  /*!
   * @brief Structure to transfer the methods from Kodi to addon
   */
  typedef struct AddonToKodiFuncTable_PVR
  {
    // Pointer inside Kodi where used from him to find his class
    KODI_HANDLE kodiInstance;

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // General callback functions
    void (*AddMenuHook)(void* kodiInstance, const struct PVR_MENUHOOK* hook);
    void (*RecordingNotification)(void* kodiInstance,
                                  const char* name,
                                  const char* fileName,
                                  bool on);
    void (*ConnectionStateChange)(void* kodiInstance,
                                  const char* strConnectionString,
                                  enum PVR_CONNECTION_STATE newState,
                                  const char* strMessage);
    void (*EpgEventStateChange)(void* kodiInstance,
                                struct EPG_TAG* tag,
                                enum EPG_EVENT_STATE newState);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Transfer functions where give data back to Kodi, e.g. GetChannels calls TransferChannelEntry
    void (*TransferChannelEntry)(void* kodiInstance,
                                 const PVR_HANDLE handle,
                                 const struct PVR_CHANNEL* chan);
    void (*TransferProviderEntry)(void* kodiInstance,
                                  const PVR_HANDLE handle,
                                  const struct PVR_PROVIDER* chanProvider);
    void (*TransferChannelGroup)(void* kodiInstance,
                                 const PVR_HANDLE handle,
                                 const struct PVR_CHANNEL_GROUP* group);
    void (*TransferChannelGroupMember)(void* kodiInstance,
                                       const PVR_HANDLE handle,
                                       const struct PVR_CHANNEL_GROUP_MEMBER* member);
    void (*TransferEpgEntry)(void* kodiInstance,
                             const PVR_HANDLE handle,
                             const struct EPG_TAG* epgentry);
    void (*TransferRecordingEntry)(void* kodiInstance,
                                   const PVR_HANDLE handle,
                                   const struct PVR_RECORDING* recording);
    void (*TransferTimerEntry)(void* kodiInstance,
                               const PVR_HANDLE handle,
                               const struct PVR_TIMER* timer);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Kodi inform interface functions
    void (*TriggerChannelUpdate)(void* kodiInstance);
    void (*TriggerProvidersUpdate)(void* kodiInstance);
    void (*TriggerChannelGroupsUpdate)(void* kodiInstance);
    void (*TriggerEpgUpdate)(void* kodiInstance, unsigned int iChannelUid);
    void (*TriggerRecordingUpdate)(void* kodiInstance);
    void (*TriggerTimerUpdate)(void* kodiInstance);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Stream demux interface functions
    void (*FreeDemuxPacket)(void* kodiInstance, struct DEMUX_PACKET* pPacket);
    struct DEMUX_PACKET* (*AllocateDemuxPacket)(void* kodiInstance, int iDataSize);
    struct PVR_CODEC (*GetCodecByName)(const void* kodiInstance, const char* strCodecName);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // New functions becomes added below and can be on another API change (where
    // breaks min API version) moved up.
  } AddonToKodiFuncTable_PVR;

  /*!
   * @brief Structure to transfer the methods from addon to Kodi
   */
  typedef struct KodiToAddonFuncTable_PVR
  {
    // Pointer inside addon where used on them to find his instance class (currently unused!)
    KODI_HANDLE addonInstance;

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // General interface functions
    enum PVR_ERROR(__cdecl* GetCapabilities)(const struct AddonInstance_PVR*,
                                             struct PVR_ADDON_CAPABILITIES*);
    enum PVR_ERROR(__cdecl* GetBackendName)(const struct AddonInstance_PVR*, char*, int);
    enum PVR_ERROR(__cdecl* GetBackendVersion)(const struct AddonInstance_PVR*, char*, int);
    enum PVR_ERROR(__cdecl* GetBackendHostname)(const struct AddonInstance_PVR*, char*, int);
    enum PVR_ERROR(__cdecl* GetConnectionString)(const struct AddonInstance_PVR*, char*, int);
    enum PVR_ERROR(__cdecl* GetDriveSpace)(const struct AddonInstance_PVR*, uint64_t*, uint64_t*);
    enum PVR_ERROR(__cdecl* CallSettingsMenuHook)(const struct AddonInstance_PVR*,
                                                  const struct PVR_MENUHOOK*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel interface functions

    enum PVR_ERROR(__cdecl* GetChannelsAmount)(const struct AddonInstance_PVR*, int*);
    enum PVR_ERROR(__cdecl* GetChannels)(const struct AddonInstance_PVR*, PVR_HANDLE, bool);
    enum PVR_ERROR(__cdecl* GetChannelStreamProperties)(const struct AddonInstance_PVR*,
                                                        const struct PVR_CHANNEL*,
                                                        struct PVR_NAMED_VALUE*,
                                                        unsigned int*);
    enum PVR_ERROR(__cdecl* GetSignalStatus)(const struct AddonInstance_PVR*,
                                             int,
                                             struct PVR_SIGNAL_STATUS*);
    enum PVR_ERROR(__cdecl* GetDescrambleInfo)(const struct AddonInstance_PVR*,
                                               int,
                                               struct PVR_DESCRAMBLE_INFO*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Provider interface functions

    enum PVR_ERROR(__cdecl* GetProvidersAmount)(const struct AddonInstance_PVR*, int*);
    enum PVR_ERROR(__cdecl* GetProviders)(const struct AddonInstance_PVR*, PVR_HANDLE);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel group interface functions
    enum PVR_ERROR(__cdecl* GetChannelGroupsAmount)(const struct AddonInstance_PVR*, int*);
    enum PVR_ERROR(__cdecl* GetChannelGroups)(const struct AddonInstance_PVR*, PVR_HANDLE, bool);
    enum PVR_ERROR(__cdecl* GetChannelGroupMembers)(const struct AddonInstance_PVR*,
                                                    PVR_HANDLE,
                                                    const struct PVR_CHANNEL_GROUP*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Channel edit interface functions
    enum PVR_ERROR(__cdecl* DeleteChannel)(const struct AddonInstance_PVR*,
                                           const struct PVR_CHANNEL*);
    enum PVR_ERROR(__cdecl* RenameChannel)(const struct AddonInstance_PVR*,
                                           const struct PVR_CHANNEL*);
    enum PVR_ERROR(__cdecl* OpenDialogChannelSettings)(const struct AddonInstance_PVR*,
                                                       const struct PVR_CHANNEL*);
    enum PVR_ERROR(__cdecl* OpenDialogChannelAdd)(const struct AddonInstance_PVR*,
                                                  const struct PVR_CHANNEL*);
    enum PVR_ERROR(__cdecl* OpenDialogChannelScan)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* CallChannelMenuHook)(const struct AddonInstance_PVR*,
                                                 const PVR_MENUHOOK*,
                                                 const PVR_CHANNEL*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // EPG interface functions
    enum PVR_ERROR(__cdecl* GetEPGForChannel)(
        const struct AddonInstance_PVR*, PVR_HANDLE, int, time_t, time_t);
    enum PVR_ERROR(__cdecl* IsEPGTagRecordable)(const struct AddonInstance_PVR*,
                                                const struct EPG_TAG*,
                                                bool*);
    enum PVR_ERROR(__cdecl* IsEPGTagPlayable)(const struct AddonInstance_PVR*,
                                              const struct EPG_TAG*,
                                              bool*);
    enum PVR_ERROR(__cdecl* GetEPGTagEdl)(const struct AddonInstance_PVR*,
                                          const struct EPG_TAG*,
                                          struct PVR_EDL_ENTRY[],
                                          int*);
    enum PVR_ERROR(__cdecl* GetEPGTagStreamProperties)(const struct AddonInstance_PVR*,
                                                       const struct EPG_TAG*,
                                                       struct PVR_NAMED_VALUE*,
                                                       unsigned int*);
    enum PVR_ERROR(__cdecl* SetEPGMaxPastDays)(const struct AddonInstance_PVR*, int);
    enum PVR_ERROR(__cdecl* SetEPGMaxFutureDays)(const struct AddonInstance_PVR*, int);
    enum PVR_ERROR(__cdecl* CallEPGMenuHook)(const struct AddonInstance_PVR*,
                                             const struct PVR_MENUHOOK*,
                                             const struct EPG_TAG*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Recording interface functions
    enum PVR_ERROR(__cdecl* GetRecordingsAmount)(const struct AddonInstance_PVR*, bool, int*);
    enum PVR_ERROR(__cdecl* GetRecordings)(const struct AddonInstance_PVR*, PVR_HANDLE, bool);
    enum PVR_ERROR(__cdecl* DeleteRecording)(const struct AddonInstance_PVR*,
                                             const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* UndeleteRecording)(const struct AddonInstance_PVR*,
                                               const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* DeleteAllRecordingsFromTrash)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* RenameRecording)(const struct AddonInstance_PVR*,
                                             const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* SetRecordingLifetime)(const struct AddonInstance_PVR*,
                                                  const struct PVR_RECORDING*);
    enum PVR_ERROR(__cdecl* SetRecordingPlayCount)(const struct AddonInstance_PVR*,
                                                   const struct PVR_RECORDING*,
                                                   int);
    enum PVR_ERROR(__cdecl* SetRecordingLastPlayedPosition)(const struct AddonInstance_PVR*,
                                                            const struct PVR_RECORDING*,
                                                            int);
    enum PVR_ERROR(__cdecl* GetRecordingLastPlayedPosition)(const struct AddonInstance_PVR*,
                                                            const struct PVR_RECORDING*,
                                                            int*);
    enum PVR_ERROR(__cdecl* GetRecordingEdl)(const struct AddonInstance_PVR*,
                                             const struct PVR_RECORDING*,
                                             struct PVR_EDL_ENTRY[],
                                             int*);
    enum PVR_ERROR(__cdecl* GetRecordingSize)(const struct AddonInstance_PVR*,
                                              const PVR_RECORDING*,
                                              int64_t*);
    enum PVR_ERROR(__cdecl* GetRecordingStreamProperties)(const struct AddonInstance_PVR*,
                                                          const struct PVR_RECORDING*,
                                                          struct PVR_NAMED_VALUE*,
                                                          unsigned int*);
    enum PVR_ERROR(__cdecl* CallRecordingMenuHook)(const struct AddonInstance_PVR*,
                                                   const struct PVR_MENUHOOK*,
                                                   const struct PVR_RECORDING*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Timer interface functions
    enum PVR_ERROR(__cdecl* GetTimerTypes)(const struct AddonInstance_PVR*,
                                           struct PVR_TIMER_TYPE[],
                                           int*);
    enum PVR_ERROR(__cdecl* GetTimersAmount)(const struct AddonInstance_PVR*, int*);
    enum PVR_ERROR(__cdecl* GetTimers)(const struct AddonInstance_PVR*, PVR_HANDLE);
    enum PVR_ERROR(__cdecl* AddTimer)(const struct AddonInstance_PVR*, const struct PVR_TIMER*);
    enum PVR_ERROR(__cdecl* DeleteTimer)(const struct AddonInstance_PVR*,
                                         const struct PVR_TIMER*,
                                         bool);
    enum PVR_ERROR(__cdecl* UpdateTimer)(const struct AddonInstance_PVR*, const struct PVR_TIMER*);
    enum PVR_ERROR(__cdecl* CallTimerMenuHook)(const struct AddonInstance_PVR*,
                                               const struct PVR_MENUHOOK*,
                                               const struct PVR_TIMER*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Powersaving interface functions
    enum PVR_ERROR(__cdecl* OnSystemSleep)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* OnSystemWake)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* OnPowerSavingActivated)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* OnPowerSavingDeactivated)(const struct AddonInstance_PVR*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Live stream read interface functions
    bool(__cdecl* OpenLiveStream)(const struct AddonInstance_PVR*, const struct PVR_CHANNEL*);
    void(__cdecl* CloseLiveStream)(const struct AddonInstance_PVR*);
    int(__cdecl* ReadLiveStream)(const struct AddonInstance_PVR*, unsigned char*, unsigned int);
    int64_t(__cdecl* SeekLiveStream)(const struct AddonInstance_PVR*, int64_t, int);
    int64_t(__cdecl* LengthLiveStream)(const struct AddonInstance_PVR*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Recording stream read interface functions
    bool(__cdecl* OpenRecordedStream)(const struct AddonInstance_PVR*, const struct PVR_RECORDING*);
    void(__cdecl* CloseRecordedStream)(const struct AddonInstance_PVR*);
    int(__cdecl* ReadRecordedStream)(const struct AddonInstance_PVR*, unsigned char*, unsigned int);
    int64_t(__cdecl* SeekRecordedStream)(const struct AddonInstance_PVR*, int64_t, int);
    int64_t(__cdecl* LengthRecordedStream)(const struct AddonInstance_PVR*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // Stream demux interface functions
    enum PVR_ERROR(__cdecl* GetStreamProperties)(const struct AddonInstance_PVR*,
                                                 struct PVR_STREAM_PROPERTIES*);
    struct DEMUX_PACKET*(__cdecl* DemuxRead)(const struct AddonInstance_PVR*);
    void(__cdecl* DemuxReset)(const struct AddonInstance_PVR*);
    void(__cdecl* DemuxAbort)(const struct AddonInstance_PVR*);
    void(__cdecl* DemuxFlush)(const struct AddonInstance_PVR*);
    void(__cdecl* SetSpeed)(const struct AddonInstance_PVR*, int);
    void(__cdecl* FillBuffer)(const struct AddonInstance_PVR*, bool);
    bool(__cdecl* SeekTime)(const struct AddonInstance_PVR*, double, bool, double*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // General stream interface functions
    bool(__cdecl* CanPauseStream)(const struct AddonInstance_PVR*);
    void(__cdecl* PauseStream)(const struct AddonInstance_PVR*, bool);
    bool(__cdecl* CanSeekStream)(const struct AddonInstance_PVR*);
    bool(__cdecl* IsRealTimeStream)(const struct AddonInstance_PVR*);
    enum PVR_ERROR(__cdecl* GetStreamTimes)(const struct AddonInstance_PVR*,
                                            struct PVR_STREAM_TIMES*);
    enum PVR_ERROR(__cdecl* GetStreamReadChunkSize)(const struct AddonInstance_PVR*, int*);

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    // New functions becomes added below and can be on another API change (where
    // breaks min API version) moved up.
  } KodiToAddonFuncTable_PVR;

  typedef struct AddonInstance_PVR
  {
    struct AddonProperties_PVR* props;
    struct AddonToKodiFuncTable_PVR* toKodi;
    struct KodiToAddonFuncTable_PVR* toAddon;
  } AddonInstance_PVR;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_H */
