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
constexpr int MARGIN_VERTICAL = 30;


enum class HorizontalAlignment
{
  DISABLED = 0,
  LEFT,
  CENTER,
  RIGHT
};

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
  OUTLINE_NO_SHADOW,
  OUTLINE,
  BOX,
  SQUARE_BOX
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
  int fontBorderSize = 15; // In %
  UTILS::Color fontBorderColor = UTILS::COLOR::BLACK;
  int fontOpacity = 100; // In %
  BorderStyle borderStyle = BorderStyle::OUTLINE;
  UTILS::Color backgroundColor = UTILS::COLOR::BLACK;
  int backgroundOpacity = 0; // In %
  int shadowSize = 0; // In %
  UTILS::Color shadowColor = UTILS::COLOR::BLACK;
  int shadowOpacity = 100; // In %
  FontAlignment alignment = FontAlignment::TOP_LEFT;
  AssOverrideStyles assOverrideStyles = AssOverrideStyles::DISABLED;
  bool assOverrideFont = false;
  bool drawWithinBlackBars = false;
  int marginVertical = MARGIN_VERTICAL;
  int blur = 0; // In %
};

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
  HorizontalAlignment horizontalAlignment = HorizontalAlignment::DISABLED;
};

} // namespace SUBTITLES
} // namespace KODI
