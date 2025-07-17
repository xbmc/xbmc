/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTypes.h"

namespace KODI::SHADER
{
class CShaderUtils
{
public:
  /*!
   * \brief Strip shader parameter pragmas from the source code
   */
  static std::string StripParameterPragmas(std::string source);

  /*!
   * \brief Returns smallest possible power-of-two sized texture
   */
  static float2 GetOptimalTextureSize(float2 videoSize);
};
} // namespace KODI::SHADER
