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
  constexpr std::string_view PRAGMA = "#pragma parameter";

  // Only a line that starts with the pragma declares a parameter. The words
  // can also appear in a comment, and the line after it must survive.
  size_t lineStart = 0;
  while (lineStart < source.size())
  {
    size_t lineEnd = source.find('\n', lineStart);
    lineEnd = (lineEnd == std::string::npos) ? source.size() : lineEnd + 1;

    const size_t textStart = source.find_first_not_of(" \t", lineStart);
    if (textStart < lineEnd && source.compare(textStart, PRAGMA.size(), PRAGMA) == 0)
      source.erase(lineStart, lineEnd - lineStart);
    else
      lineStart = lineEnd;
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
