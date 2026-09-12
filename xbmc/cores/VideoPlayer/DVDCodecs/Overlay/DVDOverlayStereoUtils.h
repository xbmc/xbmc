/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDOverlay.h"
#include "rendering/RenderSystemTypes.h"

#include <string_view>

namespace KODI::VIDEO::SUBTITLES
{
/*!
 * \brief Whether the selected output mode separates stereoscopic eye views.
 */
bool IsStereoscopicOutputMode(RenderStereoMode renderMode);

/*!
 * \brief Whether an eye-specific overlay should be rendered in the current view.
 */
bool ShouldRenderStereoOverlay(DVDOverlayStereoView overlayView,
                               RenderStereoView renderView,
                               std::string_view sourceStereoMode);
} // namespace KODI::VIDEO::SUBTITLES
