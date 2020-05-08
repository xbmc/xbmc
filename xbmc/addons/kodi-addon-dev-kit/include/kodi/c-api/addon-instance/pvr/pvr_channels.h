/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr_defines.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  #define PVR_CHANNEL_INVALID_UID -1

  typedef struct PVR_CHANNEL
  {
    unsigned int iUniqueId;
    bool bIsRadio;
    unsigned int iChannelNumber;
    unsigned int iSubChannelNumber;
    char strChannelName[PVR_ADDON_NAME_STRING_LENGTH];
    char strMimeType[PVR_ADDON_INPUT_FORMAT_STRING_LENGTH];
    unsigned int iEncryptionSystem;
    char strIconPath[PVR_ADDON_URL_STRING_LENGTH];
    bool bIsHidden;
    bool bHasArchive;
    int iOrder;
  } PVR_CHANNEL;

  typedef struct PVR_SIGNAL_STATUS
  {
    char strAdapterName[PVR_ADDON_NAME_STRING_LENGTH];
    char strAdapterStatus[PVR_ADDON_NAME_STRING_LENGTH];
    char strServiceName[PVR_ADDON_NAME_STRING_LENGTH];
    char strProviderName[PVR_ADDON_NAME_STRING_LENGTH];
    char strMuxName[PVR_ADDON_NAME_STRING_LENGTH];
    int iSNR;
    int iSignal;
    long iBER;
    long iUNC;
  } PVR_SIGNAL_STATUS;

  #define PVR_DESCRAMBLE_INFO_NOT_AVAILABLE -1

  typedef struct PVR_DESCRAMBLE_INFO
  {
    int iPid;
    int iCaid;
    int iProvid;
    int iEcmTime;
    int iHops;
    char strCardSystem[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strReader[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strFrom[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
    char strProtocol[PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH];
  } PVR_DESCRAMBLE_INFO;

#ifdef __cplusplus
}
#endif /* __cplusplus */
