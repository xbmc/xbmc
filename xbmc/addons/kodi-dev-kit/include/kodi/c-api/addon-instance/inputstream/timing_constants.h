/*
 *  Copyright (C) 2015-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_INPUTSTREAM_TIMINGCONSTANTS_H
#define C_API_ADDONINSTANCE_INPUTSTREAM_TIMINGCONSTANTS_H

/// @ingroup cpp_kodi_addon_inputstream_Defs_TimingConstants
/// @brief Speed value to pause stream in playback.
///
#define STREAM_PLAYSPEED_PAUSE 0 // frame stepping

/// @ingroup cpp_kodi_addon_inputstream_Defs_TimingConstants
/// @brief Speed value to perform stream playback at normal speed.
///
/// See @ref STREAM_PLAYSPEED_PAUSE for pause of stream.
///
#define STREAM_PLAYSPEED_NORMAL 1000

/// @ingroup cpp_kodi_addon_inputstream_Defs_TimingConstants
/// @brief Time base represented as integer.
///
#define STREAM_TIME_BASE 1000000

/// @ingroup cpp_kodi_addon_inputstream_Defs_TimingConstants
/// @brief Undefined timestamp value.
///
/// Usually reported by demuxer that work on containers that do not provide
/// either pts or dts.
///
#define STREAM_NOPTS_VALUE 0xFFF0000000000000

// "C" defines to translate stream times
#define STREAM_TIME_TO_MSEC(x) ((int)((double)(x)*1000 / STREAM_TIME_BASE))
#define STREAM_SEC_TO_TIME(x) ((double)(x)*STREAM_TIME_BASE)
#define STREAM_MSEC_TO_TIME(x) ((double)(x)*STREAM_TIME_BASE / 1000)

#endif /* !C_API_ADDONINSTANCE_INPUTSTREAM_TIMINGCONSTANTS_H */
