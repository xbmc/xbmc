/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderUtilsGLES.h"

#include "ServiceBroker.h"
#include "rendering/gles/RenderSystemGLES.h"

using namespace KODI::SHADER;

GLint CShaderUtilsGLES::TranslateWrapType(WrapType wrapType)
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
      glWrap = GL_CLAMP_TO_EDGE;
      break;
  }

  return glWrap;
}

std::string CShaderUtilsGLES::GetGLSLVersion(std::string& source)
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

    if (version >= 130 && version < 330)
      versionString = "300 es";
    else if (version == 330)
      versionString = "310 es";
    else if (version > 330)
      versionString = "320 es";
    else
      versionString = "100";

    versionString = "#version " + versionString + "\n";
  }
  // Set GLSL version according to GL version
  else
  {
    unsigned int major{0};
    unsigned int minor{0};
    CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);
    version = major * 100 + minor * 10;

    if (version == 200)
      versionString = "100";
    else
      versionString = std::to_string(version) + " es";

    // Do not force GLSL version for OpenGL ES
    //versionString = "#version " + versionString + "\n";
    versionString = "\n";
  }
  return versionString;
}
