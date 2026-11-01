/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

struct VideoPicture;

namespace KODI::RETRO
{
//! Luminance in nits of ordinary white in HDR, the reference white of
//! ITU-R BT.2408
constexpr unsigned int HDR_REFERENCE_WHITE = 203;

//! Luminance in nits of the brightest light when the display doesn't say
constexpr unsigned int HDR_DEFAULT_PEAK = 1000;

/*!
 * \brief The brightest light a game is shown with in HDR
 *
 * \param displayMaxLuminance What the display's EDID asks content to stay
 *        under, in nits, or 0 if it doesn't say
 */
unsigned int GetHdrPeak(float displayMaxLuminance);

/*!
 * \brief Describe the HDR10 output a game is shown in, for the windowing
 *        system's HDR calls
 *
 * \param picture Filled with BT.2020 colours, the PQ transfer at 10 bits and
 *        static metadata for the brightest light the game is shown with
 * \param paperWhite Luminance in nits of the game's ordinary colours
 * \param peak Luminance in nits of its brightest light
 */
void GetHdrOutputPicture(VideoPicture& picture, unsigned int paperWhite, unsigned int peak);
} // namespace KODI::RETRO
