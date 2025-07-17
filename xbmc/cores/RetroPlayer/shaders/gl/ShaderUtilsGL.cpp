/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtilsGL.h"

#include "ServiceBroker.h"
#include "rendering/gl/RenderSystemGL.h"

using namespace KODI::SHADER;

GLint CShaderUtilsGL::TranslateWrapType(WrapType wrapType)
{
  GLint glWrap;

  switch (wrapType)
  {
    case WrapType::EDGE:
      glWrap = GL_CLAMP_TO_EDGE;
      break;
    case WrapType::REPEAT:
      glWrap = GL_REPEAT;
      break;
    case WrapType::MIRRORED_REPEAT:
      glWrap = GL_MIRRORED_REPEAT;
      break;
    case WrapType::BORDER:
    default:
      glWrap = GL_CLAMP_TO_BORDER;
      break;
  }

  return glWrap;
}

std::string CShaderUtilsGL::GetGLSLVersion(std::string& source)
{
  unsigned int version;
  std::string versionString;

  const size_t sourceVersionPosition = source.find("#version ");

  // Extract the version from the source
  if (sourceVersionPosition != std::string::npos)
  {
    size_t sourceVersionSize;
    std::string sourceVersion = source.substr(sourceVersionPosition + 9);

    if (std::isdigit(sourceVersion.at(0)))
    {
      version = std::stoul(sourceVersion, &sourceVersionSize, 10);

      // Keep only source code after the version define
      source = sourceVersion.substr(sourceVersionSize);
    }
    else
    {
      return "\n";
    }

    versionString = "#version " + std::to_string(version) + "\n";
  }
  // Set GLSL version according to GL version
  else
  {
    unsigned int major{0};
    unsigned int minor{0};
    CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
    version = major * 100 + minor * 10;

    if (version == 200)
      versionString = "110";
    else if (version == 210)
      versionString = "120";
    else if (version == 300)
      versionString = "130";
    else if (version == 310)
      versionString = "140";
    else if (version == 320)
      versionString = "150";
    else
      versionString = std::to_string(version);

    versionString = "#version " + versionString + "\n";
  }
  return versionString;
}
