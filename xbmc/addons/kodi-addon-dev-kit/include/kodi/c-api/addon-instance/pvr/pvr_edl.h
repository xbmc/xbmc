/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr_defines.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef enum PVR_EDL_TYPE
  {
    PVR_EDL_TYPE_CUT = 0,
    PVR_EDL_TYPE_MUTE = 1,
    PVR_EDL_TYPE_SCENE = 2,
    PVR_EDL_TYPE_COMBREAK = 3
  } PVR_EDL_TYPE;

  typedef struct PVR_EDL_ENTRY
  {
    int64_t start;
    int64_t end;
    enum PVR_EDL_TYPE type;
  } PVR_EDL_ENTRY;

#ifdef __cplusplus
}
#endif /* __cplusplus */
