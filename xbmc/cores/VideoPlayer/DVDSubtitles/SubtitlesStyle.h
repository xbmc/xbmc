/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Color.h"

#include <string>

namespace KODI
{
namespace SUBTITLES
{

constexpr double VIEWPORT_HEIGHT = 1080.0;
constexpr double VIEWPORT_WIDTH = 1920.0;

enum class FontAlignment
{
  TOP_LEFT = 0,
  TOP_CENTER,
  TOP_RIGHT,
  MIDDLE_LEFT,
  MIDDLE_CENTER,
  MIDDLE_RIGHT,
  SUB_LEFT,
  SUB_CENTER,
  SUB_RIGHT
};

enum class FontStyle
{
  NORMAL = 0,
  BOLD,
  ITALIC,
  BOLD_ITALIC
};

enum class BorderStyle
{
  OUTLINE,
  OUTLINE_BOX
};

enum class AssOverrideStyles
{
  DISABLED = 0,
  POSITIONS,
  STYLES,
  STYLES_POSITIONS
};

struct style
{
  std::string fontName;
  double fontSize;
  FontStyle fontStyle = FontStyle::NORMAL;
  UTILS::Color fontColor = UTILS::COLOR::WHITE;
  int fontOpacity = 100;
  BorderStyle borderStyle = BorderStyle::OUTLINE;
  UTILS::Color outlineColor = UTILS::COLOR::BLACK;
  UTILS::Color backgroundColor = UTILS::COLOR::BLACK;
  int backgroundOpacity;
  FontAlignment alignment = FontAlignment::TOP_LEFT;
  AssOverrideStyles assOverrideStyles = AssOverrideStyles::DISABLED;
  bool assOverrideFont = false;
  bool drawWithinBlackBars = false;
};

const UTILS::Color colors[9] = {
    UTILS::COLOR::YELLOW,      UTILS::COLOR::WHITE,       UTILS::COLOR::BLUE,
    UTILS::COLOR::BRIGHTGREEN, UTILS::COLOR::YELLOWGREEN, UTILS::COLOR::CYAN,
    UTILS::COLOR::LIGHTGREY,   UTILS::COLOR::GREY,        UTILS::COLOR::DARKGREY};

const UTILS::Color bgColors[5] = {UTILS::COLOR::BLACK, UTILS::COLOR::YELLOW, UTILS::COLOR::WHITE,
                                  UTILS::COLOR::LIGHTGREY, UTILS::COLOR::GREY};

struct renderOpts
{
  int frameWidth;
  int frameHeight;
  int videoWidth;
  int videoHeight;
  int sourceWidth;
  int sourceHeight;
  bool usePosition = false;
  // position: vertical line position of subtitles in percent. 0 = no change (bottom), 100 = on top.
  double position = 0;
};

} // namespace SUBTITLES
} // namespace KODI
