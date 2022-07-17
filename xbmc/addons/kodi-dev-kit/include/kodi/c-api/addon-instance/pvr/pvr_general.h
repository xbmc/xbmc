/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_GENERAL_H
#define C_API_ADDONINSTANCE_PVR_GENERAL_H

#include "../inputstream/stream_constants.h"
#include "pvr_defines.h"

#include <stdbool.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 1 - General PVR
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_General_PVR_ERROR enum PVR_ERROR
  /// @ingroup cpp_kodi_addon_pvr_Defs_General
  /// @brief **PVR add-on error codes**\n
  /// Used as return values on most PVR related functions.
  ///
  /// In this way, a PVR instance signals errors in its processing and, under
  /// certain conditions, allows Kodi to make corrections.
  ///
  ///@{
  typedef enum PVR_ERROR
  {
    /// @brief __0__ : No error occurred.
    PVR_ERROR_NO_ERROR = 0,

    /// @brief __-1__ : An unknown error occurred.
    PVR_ERROR_UNKNOWN = -1,

    /// @brief __-2__ : The method that Kodi called is not implemented by the add-on.
    PVR_ERROR_NOT_IMPLEMENTED = -2,

    /// @brief __-3__ : The backend reported an error, or the add-on isn't connected.
    PVR_ERROR_SERVER_ERROR = -3,

    /// @brief __-4__ : The command was sent to the backend, but the response timed out.
    PVR_ERROR_SERVER_TIMEOUT = -4,

    /// @brief __-5__ : The command was rejected by the backend.
    PVR_ERROR_REJECTED = -5,

    /// @brief __-6__ : The requested item can not be added, because it's already present.
    PVR_ERROR_ALREADY_PRESENT = -6,

    /// @brief __-7__ : The parameters of the method that was called are invalid for this
    /// operation.
    PVR_ERROR_INVALID_PARAMETERS = -7,

    /// @brief __-8__ : A recording is running, so the timer can't be deleted without
    /// doing a forced delete.
    PVR_ERROR_RECORDING_RUNNING = -8,

    /// @brief __-9__ : The command failed.
    PVR_ERROR_FAILED = -9,
  } PVR_ERROR;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_General_PVR_CONNECTION_STATE enum PVR_CONNECTION_STATE
  /// @ingroup cpp_kodi_addon_pvr_Defs_General
  /// @brief **PVR backend connection states**\n
  /// Used with @ref kodi::addon::CInstancePVRClient::ConnectionStateChange() callback.
  ///
  /// With this, a PVR instance signals that Kodi should perform special
  /// operations.
  ///
  ///@{
  typedef enum PVR_CONNECTION_STATE
  {
    /// @brief __0__ : Unknown state (e.g. not yet tried to connect).
    PVR_CONNECTION_STATE_UNKNOWN = 0,

    /// @brief __1__ : Backend server is not reachable (e.g. server not existing or
    /// network down).
    PVR_CONNECTION_STATE_SERVER_UNREACHABLE = 1,

    /// @brief __2__ : Backend server is reachable, but there is not the expected type of
    /// server running (e.g. HTSP required, but FTP running at given server:port).
    PVR_CONNECTION_STATE_SERVER_MISMATCH = 2,

    /// @brief __3__ : Backend server is reachable, but server version does not match
    /// client requirements.
    PVR_CONNECTION_STATE_VERSION_MISMATCH = 3,

    /// @brief __4__ : Backend server is reachable, but denies client access (e.g. due
    /// to wrong credentials).
    PVR_CONNECTION_STATE_ACCESS_DENIED = 4,

    /// @brief __5__ : Connection to backend server is established.
    PVR_CONNECTION_STATE_CONNECTED = 5,

    /// @brief __6__ : No connection to backend server (e.g. due to network errors or
    /// client initiated disconnect).
    PVR_CONNECTION_STATE_DISCONNECTED = 6,

    /// @brief __7__ : Connecting to backend.
    PVR_CONNECTION_STATE_CONNECTING = 7,
  } PVR_CONNECTION_STATE;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_General_PVR_STREAM_PROPERTY definition PVR_STREAM_PROPERTY
  /// @ingroup cpp_kodi_addon_pvr_Defs_General_Inputstream
  /// @brief **PVR related stream property values**\n
  /// This is used to pass additional data to Kodi on a given PVR stream.
  ///
  /// Then transferred to livestream, recordings or EPG Tag stream using the
  /// properties.
  ///
  /// This defines are used by:
  /// - @ref kodi::addon::CInstancePVRClient::GetChannelStreamProperties()
  /// - @ref kodi::addon::CInstancePVRClient::GetEPGTagStreamProperties()
  /// - @ref kodi::addon::CInstancePVRClient::GetRecordingStreamProperties()
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  ///
  /// PVR_ERROR CMyPVRInstance::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
  ///                                                      std::vector<PVRStreamProperty>& properties)
  /// {
  ///   ...
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, "inputstream.adaptive");
  ///   properties.emplace_back("inputstream.adaptive.manifest_type", "mpd");
  ///   properties.emplace_back("inputstream.adaptive.manifest_update_parameter", "full");
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, "application/xml+dash");
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  ///
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  ///@{

  /// @brief the URL of the stream that should be played.
  ///
#define PVR_STREAM_PROPERTY_STREAMURL "streamurl"

  /// @brief To define in stream properties the name of the inputstream add-on
  /// that should be used.
  ///
  /// Leave blank to use Kodi's built-in playing capabilities or to allow ffmpeg
  /// to handle directly set to @ref PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG.
  ///
#define PVR_STREAM_PROPERTY_INPUTSTREAM STREAM_PROPERTY_INPUTSTREAM

  /// @brief To define in stream properties the player the inputstream add-on
  /// should use.
  ///
  /// Leave blank to use Kodi's built-in player selection mechanism.
  /// Permitted values are:
  /// - "videodefaultplayer"
  /// - "audiodefaultplayer"
  ///
#define PVR_STREAM_PROPERTY_INPUTSTREAM_PLAYER STREAM_PROPERTY_INPUTSTREAM_PLAYER

  /// @brief Identification string for an input stream.
  ///
  /// This value can be used in addition to @ref PVR_STREAM_PROPERTY_INPUTSTREAM.
  /// It is used to provide the respective inpustream addon with additional
  /// identification.
  ///
  /// The difference between this and other stream properties is that it is also
  /// passed in the associated @ref kodi::addon::CAddonBase::CreateInstance()
  /// call.
  ///
  /// This makes it possible to select different processing classes within the
  /// associated add-on.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  ///
  /// // On PVR instance of addon
  /// PVR_ERROR CMyPVRInstance::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
  ///                                                      std::vector<PVRStreamProperty>& properties)
  /// {
  ///   ...
  ///   // For here on example the inpustream is also inside the PVR addon
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, "pvr.my_one");
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM_INSTANCE_ID, "my_special_id_1");
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  ///
  /// ...
  ///
  /// // On CAddonBase part of addon
  /// ADDON_STATUS CMyAddon::CreateInstanceEx(int instanceType,
  ///                                         std::string instanceID,
  ///                                         KODI_HANDLE instance,
  ///                                         KODI_HANDLE& addonInstance
  ///                                         const std::string& version)
  /// {
  ///   if (instanceType == ADDON_INSTANCE_INPUTSTREAM)
  ///   {
  ///     kodi::Log(ADDON_LOG_INFO, "Creating my special inputstream");
  ///     if (instanceID == "my_special_id_1")
  ///       addonInstance = new CMyPVRClientInstance_Type1(instance, version);
  ///     else if (instanceID == "my_special_id_2")
  ///       addonInstance = new CMyPVRClientInstance_Type2(instance, version);
  ///     return ADDON_STATUS_OK;
  ///   }
  ///   else if (...)
  ///   {
  ///     ...
  ///   }
  ///   return ADDON_STATUS_UNKNOWN;
  /// }
  ///
  /// ...
  /// ~~~~~~~~~~~~~
  ///
#define PVR_STREAM_PROPERTY_INPUTSTREAM_INSTANCE_ID STREAM_PROPERTY_INPUTSTREAM_INSTANCE_ID

  /// @brief the MIME type of the stream that should be played.
  ///
#define PVR_STREAM_PROPERTY_MIMETYPE "mimetype"

  /// @brief <b>"true"</b> to denote that the stream that should be played is a
  /// realtime stream.
  ///
  /// Any other value indicates that this is no realtime stream.
  ///
#define PVR_STREAM_PROPERTY_ISREALTIMESTREAM STREAM_PROPERTY_ISREALTIMESTREAM

  /// @brief <b>"true"</b> to denote that if the stream is from an EPG tag.
  ///
  /// It should be played is a live stream. Otherwise if it's a EPG tag it will
  /// play as normal video.
  ///
#define PVR_STREAM_PROPERTY_EPGPLAYBACKASLIVE "epgplaybackaslive"

  /// @brief Special value for @ref PVR_STREAM_PROPERTY_INPUTSTREAM to use
  /// ffmpeg to directly play a stream URL.
#define PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG

  ///@}
  //-----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on capabilities.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRCapabilities for description of values.
   */
  typedef struct PVR_ADDON_CAPABILITIES
  {
    bool bSupportsEPG;
    bool bSupportsEPGEdl;
    bool bSupportsTV;
    bool bSupportsRadio;
    bool bSupportsRecordings;
    bool bSupportsRecordingsUndelete;
    bool bSupportsTimers;
    bool bSupportsChannelGroups;
    bool bSupportsChannelScan;
    bool bSupportsChannelSettings;
    bool bHandlesInputStream;
    bool bHandlesDemuxing;
    bool bSupportsRecordingPlayCount;
    bool bSupportsLastPlayedPosition;
    bool bSupportsRecordingEdl;
    bool bSupportsRecordingsRename;
    bool bSupportsRecordingsLifetimeChange;
    bool bSupportsDescrambleInfo;
    bool bSupportsAsyncEPGTransfer;
    bool bSupportsRecordingSize;
    bool bSupportsProviders;
    bool bSupportsRecordingsDelete;

    unsigned int iRecordingsLifetimesSize;
    struct PVR_ATTRIBUTE_INT_VALUE recordingsLifetimeValues[PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE];
  } PVR_ADDON_CAPABILITIES;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_GENERAL_H */
