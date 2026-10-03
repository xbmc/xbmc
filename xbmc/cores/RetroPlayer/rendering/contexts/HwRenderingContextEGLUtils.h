/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <charconv>
#include <string_view>

#if defined(HAS_EGL)
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"

#include <algorithm>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

namespace KODI::RETRO
{
struct EGLCapabilities
{
  bool createContext{false};
  bool surfaceless{false};
  bool coreContextAttributes{false};
};

inline EGLCapabilities GetEGLCapabilities(const char* version, const char* extensions)
{
  if (!version)
    return {};
  const std::string_view versionString(version);
  const char* end = version + versionString.size();
  unsigned int major = 0, minor = 0;
  const auto majorResult = std::from_chars(version, end, major);
  if (majorResult.ec != std::errc{} || majorResult.ptr == end || *majorResult.ptr != '.')
    return {};
  const auto minorResult = std::from_chars(majorResult.ptr + 1, end, minor);
  if (minorResult.ec != std::errc{} || (minorResult.ptr != end && *minorResult.ptr != ' '))
    return {};

  // EGL 1.5 incorporates surfaceless binding and the KHR context attributes.
  if (major > 1 || (major == 1 && minor >= 5))
    return {true, true, true};

  bool surfaceless = false, createContext = false;
  std::string_view remaining(extensions ? extensions : "");
  while (!remaining.empty())
  {
    const auto separator = remaining.find(' ');
    const auto extension = remaining.substr(0, separator);
    surfaceless |= extension == "EGL_KHR_surfaceless_context";
    createContext |= extension == "EGL_KHR_create_context";
    if (separator == std::string_view::npos)
      break;
    remaining.remove_prefix(separator + 1);
  }
  if (major != 1 || minor != 4)
    return {};
  return {createContext, surfaceless, false};
}

inline bool SupportsEGLHardwareRendering(const char* version,
                                         const char* extensions,
                                         bool pbufferAvailable = false)
{
  const auto capabilities = GetEGLCapabilities(version, extensions);
  return capabilities.createContext && (capabilities.surfaceless || pbufferAvailable);
}

inline bool SupportsEGLHardwareRendering(const char* version,
                                         const char* extensions,
                                         bool embedded,
                                         unsigned int guiMajor,
                                         unsigned int guiMinor,
                                         bool pbufferAvailable = false)
{
  return SupportsEGLHardwareRendering(version, extensions, pbufferAvailable) &&
         (embedded ? guiMajor >= 3 : guiMajor > 3 || (guiMajor == 3 && guiMinor >= 2));
}

#if defined(HAS_EGL)
struct EGLConfigFunctions
{
  decltype(&eglGetConfigAttrib) getConfigAttrib{eglGetConfigAttrib};
  decltype(&eglChooseConfig) chooseConfig{eglChooseConfig};
};

inline std::vector<EGLConfig> GetEGLClientConfigs(EGLDisplay display,
                                                  EGLConfig guiConfig,
                                                  EGLint renderableType,
                                                  bool surfaceless,
                                                  const EGLConfigFunctions& egl = {})
{
  std::vector<EGLConfig> configs;
  EGLint renderable = 0, surfaces = 0;
  if (guiConfig && egl.getConfigAttrib(display, guiConfig, EGL_RENDERABLE_TYPE, &renderable) &&
      (renderable & renderableType) == renderableType &&
      (surfaceless || (egl.getConfigAttrib(display, guiConfig, EGL_SURFACE_TYPE, &surfaces) &&
                       (surfaces & EGL_PBUFFER_BIT))))
    configs.push_back(guiConfig);

  // The FBO supplies the framebuffer format. Zero overrides EGL's default window-surface requirement.
  const EGLint attributes[]{EGL_RENDERABLE_TYPE, renderableType, EGL_SURFACE_TYPE,
                            surfaceless ? 0 : EGL_PBUFFER_BIT, EGL_NONE};
  EGLint count = 0;
  if (!egl.chooseConfig(display, attributes, nullptr, 0, &count) || count <= 0)
    return configs;
  std::vector<EGLConfig> alternatives(count);
  if (!egl.chooseConfig(display, attributes, alternatives.data(), count, &count) || count <= 0)
    return configs;
  alternatives.resize(count);
  for (const EGLConfig config : alternatives)
  {
    if (std::find(configs.begin(), configs.end(), config) == configs.end())
      configs.push_back(config);
  }
  return configs;
}

inline std::vector<EGLint> BuildEGLContextAttributes(const HwContextProperties& properties,
                                                     unsigned int major,
                                                     unsigned int minor,
                                                     const char* eglVersion)
{
  std::vector<EGLint> attributes;
  if (major != 0)
  {
    attributes.insert(attributes.end(),
                      {EGL_CONTEXT_MAJOR_VERSION_KHR, static_cast<EGLint>(major),
                       EGL_CONTEXT_MINOR_VERSION_KHR, static_cast<EGLint>(minor)});
  }
  if (!properties.embedded)
  {
    attributes.insert(attributes.end(),
                      {EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
                       properties.coreProfile ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
                                              : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR});
  }
  if (properties.debugContext)
  {
    // EGL 1.5 replaces the KHR debug flag with a separate boolean attribute.
    if (GetEGLCapabilities(eglVersion, nullptr).coreContextAttributes)
      attributes.insert(attributes.end(), {EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE});
    else
      attributes.insert(attributes.end(),
                        {EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR});
  }
  attributes.push_back(EGL_NONE);
  return attributes;
}
#endif
} // namespace KODI::RETRO
