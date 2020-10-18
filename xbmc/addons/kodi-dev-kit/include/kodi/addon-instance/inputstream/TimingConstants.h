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

#undef STREAM_TIME_TO_MSEC
#undef STREAM_SEC_TO_TIME
#undef STREAM_MSEC_TO_TIME

constexpr int STREAM_TIME_TO_MSEC(double x)
{
  return static_cast<int>(x * 1000 / STREAM_TIME_BASE);
}

constexpr double STREAM_SEC_TO_TIME(double x)
{
  return x * STREAM_TIME_BASE;
}

constexpr double STREAM_MSEC_TO_TIME(double x)
{
  return x * STREAM_TIME_BASE / 1000;
}

#endif /* __cplusplus */
