/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_CHANNEL_GROUPS_H
#define C_API_ADDONINSTANCE_PVR_CHANNEL_GROUPS_H

#include "pvr_defines.h"

#include <stdbool.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 3 - PVR channel group
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  /*!
   * @brief "C" PVR add-on channel group.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRChannelGroup for description of values.
   */
  typedef struct PVR_CHANNEL_GROUP
  {
    const char* strGroupName;
    bool bIsRadio;
    unsigned int iPosition;
  } PVR_CHANNEL_GROUP;

  /*!
   * @brief "C" PVR add-on channel group member.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRChannelGroupMember for description of values.
   */
  typedef struct PVR_CHANNEL_GROUP_MEMBER
  {
    const char* strGroupName;
    unsigned int iChannelUniqueId;
    unsigned int iChannelNumber;
    unsigned int iSubChannelNumber;
    int iOrder;
  } PVR_CHANNEL_GROUP_MEMBER;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_CHANNEL_GROUPS_H */
