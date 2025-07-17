/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtils.h"

using namespace KODI::SHADER;

std::string CShaderUtils::StripParameterPragmas(std::string source)
{
  size_t pragmaPosition;
  size_t newlinePosition;

  while ((pragmaPosition = source.find("#pragma parameter")) != std::string::npos)
  {
    newlinePosition = source.find_first_of('\n', pragmaPosition + 17);

    if (newlinePosition != std::string::npos)
      source.erase(pragmaPosition, newlinePosition - pragmaPosition + 1);
  }

  return source;
}

float2 CShaderUtils::GetOptimalTextureSize(float2 videoSize)
{
  unsigned int textureWidth = 1;
  unsigned int textureHeight = 1;

  // Find smallest possible power-of-two sized width that can contain the input texture
  while (true)
  {
    if (textureWidth >= videoSize.x)
      break;
    textureWidth *= 2;
  }

  // Find smallest possible power-of-two sized height that can contain the input texture
  while (true)
  {
    if (textureHeight >= videoSize.y)
      break;
    textureHeight *= 2;
  }

  return float2(textureWidth, textureHeight);
}
