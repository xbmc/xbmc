/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>

typedef int DisplayMode;
#define DM_WINDOWED     -1
#define DM_FULLSCREEN    0

enum RESOLUTION
{
  RES_INVALID        = -1,
  RES_WINDOW         = 15,
  RES_DESKTOP        = 16,          // Desktop resolution
  RES_CUSTOM         = 16 + 1,      // First additional resolution
};

struct OVERSCAN
{
  int left;
  int top;
  int right;
  int bottom;
public:
  OVERSCAN()
  {
    left = top = right = bottom = 0;
  }
  OVERSCAN(const OVERSCAN& os)
  {
    left = os.left; top = os.top;
    right = os.right; bottom = os.bottom;
  }
  OVERSCAN& operator=(const OVERSCAN&) = default;

  bool operator==(const OVERSCAN& other)
  {
    return left == other.left && right == other.right && top == other.top && bottom == other.bottom;
  }
  bool operator!=(const OVERSCAN& other)
  {
    return left != other.left || right != other.right || top != other.top || bottom != other.bottom;
  }
};

struct EdgeInsets
{
  float left = 0.0f;
  float top = 0.0f;
  float right = 0.0f;
  float bottom = 0.0f;

  EdgeInsets() = default;
  EdgeInsets(float l, float t, float r, float b);
};

//! @brief Provide info of a resolution
struct RESOLUTION_INFO
{
  //!< Screen overscan boundary
  OVERSCAN Overscan;

  //!< Edge insets to scale the GUI to prevent the display notch from hiding a part of the GUI
  EdgeInsets guiInsets;

  //!< Specify if it is a fullscreen resolution, otherwise windowed
  bool bFullScreen{true};

  //!< Width GUI resolution (pixels), may differ from the screen value if GUI resolution limit, 3D is set or in HiDPI screens
  int iWidth{0};

  //!< Height GUI resolution (pixels), may differ from the screen value if GUI resolution limit, 3D is set or in HiDPI screens
  int iHeight{0};

  //!< Number of pixels of padding between stereoscopic frames
  int iBlanking{0};

  //!< Screen width (logical width in pixels)
  int iScreenWidth{0};

  //!< Screen height (logical height in pixels)
  int iScreenHeight{0};

  //!< The vertical subtitle baseline position, may be changed by Video calibration
  int iSubtitles{0};

  //!< Properties of the resolution e.g. interlaced mode
  uint32_t dwFlags{0};

  //!< Pixel aspect ratio
  float fPixelRatio{1.0f};

  //!< Refresh rate
  float fRefreshRate{0.0f};

  //!< Resolution mode description
  std::string strMode;

  //!< Resolution output description
  std::string strOutput;

  //!< Resolution ID
  std::string strId;

public:
  RESOLUTION_INFO(int width = 1280, int height = 720, float aspect = 0, std::string mode = "");
  RESOLUTION_INFO(const RESOLUTION_INFO& res) = default;
  RESOLUTION_INFO& operator=(const RESOLUTION_INFO&) = default;

  float DisplayRatio() const;
};

class CResolutionUtils
{
public:
  static RESOLUTION ChooseBestResolution(float fps, int width, int height, bool is3D);
  static bool HasWhitelist();
  static void PrintWhitelist();

  /*!
   * \brief Get the max allowed screen resolution, if fullscreen
   * \param width [OUT] Max width resolution
   * \param height [OUT] Max height resolution
   */
  static void GetMaxAllowedScreenResolution(unsigned int& width, unsigned int& height);

private:
  static void FindResolutionFromWhitelist(float fps, int width, int height, bool is3D, RESOLUTION &resolution);
  static bool FindResolutionFromOverride(float fps, RESOLUTION& resolution, bool fallback);
};
