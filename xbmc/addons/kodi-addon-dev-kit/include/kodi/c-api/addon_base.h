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
  /// @defgroup cpp_kodi_Defs_AddonLog enum AddonLog
  /// @ingroup cpp_kodi_Defs
  /// @brief **Log file type definitions**\n
  /// These define the types of log entries given with @ref kodi::Log() to Kodi.
  ///
  /// -------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/General.h>
  ///
  /// kodi::Log(ADDON_LOG_ERROR, "%s: There is an error occurred!", __func__);
  ///
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  typedef enum AddonLog
  {
    /// @brief **0** : To include debug information in the log file.
    ADDON_LOG_DEBUG = 0,

    /// @brief **1** : To include information messages in the log file.
    ADDON_LOG_INFO = 1,

    /// @brief **2** : To write warnings in the log file.
    ADDON_LOG_WARNING = 2,

    /// @brief **3** : To report error messages in the log file.
    ADDON_LOG_ERROR = 3,

    /// @brief **4** : To notify fatal unrecoverable errors, which can may also indicate
    /// upcoming crashes.
    ADDON_LOG_FATAL = 4
  } AddonLog;
  ///@}
  //----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif /* __cplusplus */
