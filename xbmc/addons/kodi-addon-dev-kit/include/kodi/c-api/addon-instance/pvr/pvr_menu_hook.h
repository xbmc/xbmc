/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr_defines.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef enum PVR_MENUHOOK_CAT
  {
    PVR_MENUHOOK_UNKNOWN = -1,
    PVR_MENUHOOK_ALL = 0,
    PVR_MENUHOOK_CHANNEL = 1,
    PVR_MENUHOOK_TIMER = 2,
    PVR_MENUHOOK_EPG = 3,
    PVR_MENUHOOK_RECORDING = 4,
    PVR_MENUHOOK_DELETED_RECORDING = 5,
    PVR_MENUHOOK_SETTING = 6,
  } PVR_MENUHOOK_CAT;

  typedef struct PVR_MENUHOOK
  {
    unsigned int iHookId;
    unsigned int iLocalizedStringId;
    enum PVR_MENUHOOK_CAT category;
  } PVR_MENUHOOK;

#ifdef __cplusplus
}
#endif /* __cplusplus */
