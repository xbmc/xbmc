/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtils.h"

#include <string_view>

using namespace KODI::SHADER;

std::string CShaderUtils::StripParameterPragmas(std::string source)
{
  constexpr std::string_view pragma = "#pragma parameter";

  size_t pragmaPosition = 0;
  while ((pragmaPosition = source.find(pragma, pragmaPosition)) != std::string::npos)
  {
    const size_t previousNewline = source.rfind('\n', pragmaPosition);
    const size_t lineStart = previousNewline == std::string::npos ? 0 : previousNewline + 1;

    // Only a directive that starts its line is removed. Removing the words from a
    // comment would take its newline too and comment out the next line.
    if (source.find_first_not_of(" \t", lineStart) != pragmaPosition)
    {
      pragmaPosition += pragma.size();
      continue;
    }

    const size_t newlinePosition = source.find('\n', pragmaPosition);
    source.erase(lineStart, newlinePosition == std::string::npos ? std::string::npos
                                                                 : newlinePosition - lineStart + 1);
    pragmaPosition = lineStart;
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
