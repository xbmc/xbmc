/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

namespace KODI::WINDOWING
{

/*!
 * \brief The largest rectangle of display-space ratio \p aspect that fits centred in \p area,
 * whose pixels are \p pixelRatio wide. Edges are whole pixels.
 *
 * Exact: a rectangle of a ratio the area very nearly already has still comes back at the ratio
 * asked for, not rounded up to the area. \p area when \p aspect is zero or below.
 */
CRect ComputeAspectRect(const CRect& area, float aspect, float pixelRatio);

} // namespace KODI::WINDOWING
