/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/addon-instance/inputstream/timing_constants.h"

#ifdef __cplusplus

// Unset the on timing_constants.h given defines
#undef STREAM_TIME_TO_MSEC
#undef STREAM_SEC_TO_TIME
#undef STREAM_MSEC_TO_TIME

/// @ingroup cpp_kodi_addon_inputstream_Defs_TimingConstants
/// @brief Converts a stream time to milliseconds as an integer value.
///
/// @param[in] x Stream time
/// @return Milliseconds
///
/// @note Within "C" code this is used as `#define`.
///
constexpr int STREAM_TIME_TO_MSEC(double x)
{
  return static_cast<int>(x * 1000 / STREAM_TIME_BASE);
}

/// @ingroup cpp_kodi_addon_inputstream_Defs_TimingConstants
/// @brief Converts a time in seconds to the used stream time format.
///
/// @param[in] x Seconds
/// @return Stream time
///
/// @note Within "C" code this is used as `#define`.
///
constexpr double STREAM_SEC_TO_TIME(double x)
{
  return x * STREAM_TIME_BASE;
}

/// @ingroup cpp_kodi_addon_inputstream_Defs_TimingConstants
/// @brief Converts a time in milliseconds to the used stream time format.
///
/// @param[in] x Milliseconds
/// @return Stream time
///
/// @note Within "C" code this is used as `#define`.
///
constexpr double STREAM_MSEC_TO_TIME(double x)
{
  return x * STREAM_TIME_BASE / 1000;
}

#endif /* __cplusplus */
