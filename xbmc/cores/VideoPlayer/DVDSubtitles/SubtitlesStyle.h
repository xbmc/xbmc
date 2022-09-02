/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/ColorUtils.h"

#include <string>

namespace KODI
{
namespace SUBTITLES
{
namespace STYLE
{

constexpr double VIEWPORT_HEIGHT = 1080.0;
constexpr double VIEWPORT_WIDTH = 1920.0;
constexpr int MARGIN_VERTICAL = 75;

enum class HorizontalAlign
{
  DISABLED = 0,
  LEFT,
  CENTER,
  RIGHT
};

enum class FontAlign
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

enum class BorderType
{
  OUTLINE_NO_SHADOW,
  OUTLINE,
  BOX,
  SQUARE_BOX
};

enum class OverrideStyles
{
  DISABLED = 0,
  POSITIONS,
  STYLES,
  STYLES_POSITIONS
};

enum class MarginsMode
{
  // Use style margins only
  DEFAULT,
  // Apply margins to position text within the video area (cropped videos)
  INSIDE_VIDEO,
  // Disable any kind of margin
  DISABLED
};

struct style
{
  std::string fontName; // Font family name
  double fontSize; // Font size in pixel
  FontStyle fontStyle = FontStyle::NORMAL;
  UTILS::COLOR::Color fontColor = UTILS::COLOR::WHITE;
  int fontBorderSize = 15; // In %
  UTILS::COLOR::Color fontBorderColor = UTILS::COLOR::BLACK;
  int fontOpacity = 100; // In %
  BorderType borderStyle = BorderType::OUTLINE;
  UTILS::COLOR::Color backgroundColor = UTILS::COLOR::BLACK;
  int backgroundOpacity = 0; // In %
  int shadowSize = 0; // In %
  UTILS::COLOR::Color shadowColor = UTILS::COLOR::BLACK;
  int shadowOpacity = 100; // In %
  FontAlign alignment = FontAlign::TOP_LEFT;
  // Override styles to native ASS/SSA format type only
  OverrideStyles assOverrideStyles = OverrideStyles::DISABLED;
  // Override fonts to native ASS/SSA format type only
  bool assOverrideFont = false;
  // Vertical margin value in pixels scaled for VIEWPORT_HEIGHT
  int marginVertical = MARGIN_VERTICAL;
  int blur = 0; // In %
};

struct subtitleOpts
{
  bool useMargins = false;
  int marginLeft;
  int marginRight;
  int marginVertical;
};

struct renderOpts
{
  float frameWidth;
  float frameHeight;
  // Video size width, may be influenced by video settings (e.g. zoom)
  float videoWidth;
  // Video size height, may be influenced by video settings (e.g. zoom)
  float videoHeight;
  float sourceWidth;
  float sourceHeight;
  float m_par; // Set the pixel aspect ratio
  MarginsMode marginsMode = MarginsMode::DEFAULT;
  // Vertical line position of subtitles in percentage
  // only for bottom alignment, 0 = bottom (no change), 100 = on top
  double position = 0;
  HorizontalAlign horizontalAlignment = HorizontalAlign::DISABLED;
};

} // namespace STYLE
} // namespace SUBTITLES
} // namespace KODI
