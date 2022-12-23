/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace RETRO
{

// NOTE: Only append
enum class SCALINGMETHOD
{
  AUTO = 0,
  NEAREST = 1,
  LINEAR = 2,
  MAX = LINEAR
};

/*!
 * \ingroup games
 * \brief Methods for stretching the game to the viewing area
 */
enum class STRETCHMODE
{
  /*!
   * \brief Show the game at its normal aspect ratio
   */
  Normal,

  /*!
   * \brief Stretch the game to maintain a 4:3 aspect ratio
   */
  Stretch4x3,

  /*!
   * \brief Stretch the game to fill the viewing area
   */
  Fullscreen,

  /*!
   * \brief Show the game at its original size (humorous for old consoles
   *        on 4K TVs)
   */
  Original,

  /*!
   * \brief Show the game at its normal aspect ratio but zoom to fill the
   * viewing area
   */
  Zoom,
};

constexpr const char* STRETCHMODE_NORMAL_ID = "normal";
constexpr const char* STRETCHMODE_STRETCH_4_3_ID = "4:3";
constexpr const char* STRETCHMODE_FULLSCREEN_ID = "fullscreen";
constexpr const char* STRETCHMODE_ORIGINAL_ID = "original";
constexpr const char* STRETCHMODE_ZOOM_ID = "zoom";

enum class RENDERFEATURE
{
  ROTATION,
  STRETCH,
  ZOOM,
  PIXEL_RATIO,
};

}
}
