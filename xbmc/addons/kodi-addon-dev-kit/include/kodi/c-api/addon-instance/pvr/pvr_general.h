/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr_defines.h"

#ifdef BUILD_KODI_ADDON
#include "../../../InputStreamConstants.h"
#else
#include "cores/VideoPlayer/Interface/Addon/InputStreamConstants.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef enum PVR_ERROR
  {
    PVR_ERROR_NO_ERROR = 0,
    PVR_ERROR_UNKNOWN = -1,
    PVR_ERROR_NOT_IMPLEMENTED = -2,
    PVR_ERROR_SERVER_ERROR = -3,
    PVR_ERROR_SERVER_TIMEOUT = -4,
    PVR_ERROR_REJECTED = -5,
    PVR_ERROR_ALREADY_PRESENT = -6,
    PVR_ERROR_INVALID_PARAMETERS = -7,
    PVR_ERROR_RECORDING_RUNNING = -8,
    PVR_ERROR_FAILED = -9,
  } PVR_ERROR;

  typedef enum PVR_CONNECTION_STATE
  {
    PVR_CONNECTION_STATE_UNKNOWN = 0,
    PVR_CONNECTION_STATE_SERVER_UNREACHABLE = 1,
    PVR_CONNECTION_STATE_SERVER_MISMATCH = 2,
    PVR_CONNECTION_STATE_VERSION_MISMATCH = 3,
    PVR_CONNECTION_STATE_ACCESS_DENIED = 4,
    PVR_CONNECTION_STATE_CONNECTED = 5,
    PVR_CONNECTION_STATE_DISCONNECTED = 6,
    PVR_CONNECTION_STATE_CONNECTING = 7,
  } PVR_CONNECTION_STATE;

  #define PVR_STREAM_PROPERTY_STREAMURL "streamurl"
  #define PVR_STREAM_PROPERTY_INPUTSTREAM STREAM_PROPERTY_INPUTSTREAM
  #define PVR_STREAM_PROPERTY_INPUTSTREAM_INSTANCE_ID STREAM_PROPERTY_INPUTSTREAM_INSTANCE_ID
  #define PVR_STREAM_PROPERTY_MIMETYPE "mimetype"
  #define PVR_STREAM_PROPERTY_ISREALTIMESTREAM STREAM_PROPERTY_ISREALTIMESTREAM
  #define PVR_STREAM_PROPERTY_EPGPLAYBACKASLIVE "epgplaybackaslive"
  #define PVR_STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG

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

    unsigned int iRecordingsLifetimesSize;
    struct PVR_ATTRIBUTE_INT_VALUE recordingsLifetimeValues[PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE];
  } PVR_ADDON_CAPABILITIES;

#ifdef __cplusplus
}
#endif /* __cplusplus */
