/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"
#include "windowing/Resolution.h"

namespace KODI::WINDOWING
{

//! \brief The largest rectangle of \p aspect centred in the calibrated area, or the whole
//! display when \p aspect is zero or below.
CRect ComputeRasterRect(const RESOLUTION_INFO& info, float aspect);

} // namespace KODI::WINDOWING
