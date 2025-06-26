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
   * \brief Show the game at 1:1 PAR
   */
  Normal1x1,

  /*!
   * \brief Stretch the game to maintain a 4:3 aspect ratio
   */
  Stretch4x3,

  /*!
   * \brief Stretch the game to maintain a 16:9 aspect ratio
   */
  Stretch16x9,

  /*!
   * \brief Stretch the game to fill the viewing area
   */
  Fullscreen,

  /*!
   * \brief Stretch the game by multiplying the original vertical size
   *        by an integer value while keeping the original PAR
   */
  Integer,

  /*!
   * \brief Stretch the game by multiplying the original pixel dimensions
   *        by an integer value at 1:1 PAR
   */
  Integer1x1,

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
constexpr const char* STRETCHMODE_NORMAL_1_1_ID = "normal1:1";
constexpr const char* STRETCHMODE_STRETCH_4_3_ID = "4:3";
constexpr const char* STRETCHMODE_STRETCH_16_9_ID = "16:9";
constexpr const char* STRETCHMODE_FULLSCREEN_ID = "fullscreen";
constexpr const char* STRETCHMODE_INTEGER_ID = "integer";
constexpr const char* STRETCHMODE_INTEGER_1_1_ID = "integer1:1";
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
