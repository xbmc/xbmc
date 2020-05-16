/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  ///
  ///
  typedef enum AddonLog
  {
    ///
    ADDON_LOG_DEBUG = 0,

    ///
    ADDON_LOG_INFO = 1,

    ///
    ADDON_LOG_WARNING = 2,

    ///
    ADDON_LOG_ERROR = 3,

    ///
    ADDON_LOG_FATAL = 4
  } AddonLog;
  //----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif /* __cplusplus */
