/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EGLUtils.h"

#include "ServiceBroker.h"
#include "StringUtils.h"
#include "guilib/IDirtyRegionSolver.h"
#include "log.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

#include <map>

#include <EGL/eglext.h>

namespace
{

#define X(VAL) std::make_pair(VAL, #VAL)
std::map<EGLint, const char*> eglAttributes =
{
  // please keep attributes in accordance to:
  // https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglGetConfigAttrib.xhtml
  X(EGL_ALPHA_SIZE),
  X(EGL_ALPHA_MASK_SIZE),
  X(EGL_BIND_TO_TEXTURE_RGB),
  X(EGL_BIND_TO_TEXTURE_RGBA),
  X(EGL_BLUE_SIZE),
  X(EGL_BUFFER_SIZE),
  X(EGL_COLOR_BUFFER_TYPE),
  X(EGL_CONFIG_CAVEAT),
  X(EGL_CONFIG_ID),
  X(EGL_CONFORMANT),
  X(EGL_DEPTH_SIZE),
  X(EGL_GREEN_SIZE),
  X(EGL_LEVEL),
  X(EGL_LUMINANCE_SIZE),
  X(EGL_MAX_PBUFFER_WIDTH),
  X(EGL_MAX_PBUFFER_HEIGHT),
  X(EGL_MAX_PBUFFER_PIXELS),
  X(EGL_MAX_SWAP_INTERVAL),
  X(EGL_MIN_SWAP_INTERVAL),
  X(EGL_NATIVE_RENDERABLE),
  X(EGL_NATIVE_VISUAL_ID),
  X(EGL_NATIVE_VISUAL_TYPE),
  X(EGL_RED_SIZE),
  X(EGL_RENDERABLE_TYPE),
  X(EGL_SAMPLE_BUFFERS),
  X(EGL_SAMPLES),
  X(EGL_STENCIL_SIZE),
  X(EGL_SURFACE_TYPE),
  X(EGL_TRANSPARENT_TYPE),
  X(EGL_TRANSPARENT_RED_VALUE),
  X(EGL_TRANSPARENT_GREEN_VALUE),
  X(EGL_TRANSPARENT_BLUE_VALUE)
};

std::map<EGLenum, const char*> eglErrors =
{
  // please keep errors in accordance to:
  // https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglGetError.xhtml
  X(EGL_SUCCESS),
  X(EGL_NOT_INITIALIZED),
  X(EGL_BAD_ACCESS),
  X(EGL_BAD_ALLOC),
  X(EGL_BAD_ATTRIBUTE),
  X(EGL_BAD_CONFIG),
  X(EGL_BAD_CONTEXT),
  X(EGL_BAD_CURRENT_SURFACE),
  X(EGL_BAD_DISPLAY),
  X(EGL_BAD_MATCH),
  X(EGL_BAD_NATIVE_PIXMAP),
  X(EGL_BAD_NATIVE_WINDOW),
  X(EGL_BAD_PARAMETER),
  X(EGL_BAD_SURFACE),
  X(EGL_CONTEXT_LOST),
};

std::map<EGLint, const char*> eglErrorType =
{
  X(EGL_DEBUG_MSG_CRITICAL_KHR),
  X(EGL_DEBUG_MSG_ERROR_KHR),
  X(EGL_DEBUG_MSG_WARN_KHR),
  X(EGL_DEBUG_MSG_INFO_KHR),
};
#undef X

} // namespace

void EglErrorCallback(EGLenum error,
                      const char* command,
                      EGLint messageType,
                      EGLLabelKHR threadLabel,
                      EGLLabelKHR objectLabel,
                      const char* message)
{
  std::string errorStr;
  std::string typeStr;

  auto eglError = eglErrors.find(error);
  if (eglError != eglErrors.end())
  {
    errorStr = eglError->second;
  }

  auto eglType = eglErrorType.find(messageType);
  if (eglType != eglErrorType.end())
  {
    typeStr = eglType->second;
  }

  CLog::Log(LOGDEBUG, "EGL Debugging:\nError: {}\nCommand: {}\nType: {}\nMessage: {}", errorStr,
            command, typeStr, message ? message : "");
}

std::set<std::string> CEGLUtils::GetClientExtensions()
{
  const char* extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  if (!extensions)
  {
    return {};
  }
  std::set<std::string> result;
  StringUtils::SplitTo(std::inserter(result, result.begin()), extensions, " ");
  return result;
}

std::set<std::string> CEGLUtils::GetExtensions(EGLDisplay eglDisplay)
{
  const char* extensions = eglQueryString(eglDisplay, EGL_EXTENSIONS);
  if (!extensions)
  {
    throw std::runtime_error("Could not query EGL for extensions");
  }
  std::set<std::string> result;
  StringUtils::SplitTo(std::inserter(result, result.begin()), extensions, " ");
  return result;
}

bool CEGLUtils::HasExtension(EGLDisplay eglDisplay, const std::string& name)
{
  auto exts = GetExtensions(eglDisplay);
  return (exts.find(name) != exts.end());
}

bool CEGLUtils::HasClientExtension(const std::string& name)
{
  auto exts = GetClientExtensions();
  return (exts.find(name) != exts.end());
}

void CEGLUtils::Log(int logLevel, const std::string& what)
{
  EGLenum error = eglGetError();
  std::string errorStr = StringUtils::Format("{:#04X}", error);

  auto eglError = eglErrors.find(error);
  if (eglError != eglErrors.end())
  {
    errorStr = eglError->second;
  }

  CLog::Log(logLevel, "{} ({})", what, errorStr);
}

CEGLContextUtils::CEGLContextUtils(EGLenum platform, std::string const& platformExtension)
: m_platform{platform}
{
  if (CEGLUtils::HasClientExtension("EGL_KHR_debug"))
  {
    auto eglDebugMessageControl = CEGLUtils::GetRequiredProcAddress<PFNEGLDEBUGMESSAGECONTROLKHRPROC>("eglDebugMessageControlKHR");

    EGLAttrib eglDebugAttribs[] = {EGL_DEBUG_MSG_CRITICAL_KHR, EGL_TRUE,
                                   EGL_DEBUG_MSG_ERROR_KHR, EGL_TRUE,
                                   EGL_DEBUG_MSG_WARN_KHR, EGL_TRUE,
                                   EGL_DEBUG_MSG_INFO_KHR, EGL_TRUE,
                                   EGL_NONE};

    eglDebugMessageControl(EglErrorCallback, eglDebugAttribs);
  }

  m_platformSupported = CEGLUtils::HasClientExtension("EGL_EXT_platform_base") && CEGLUtils::HasClientExtension(platformExtension);
}

bool CEGLContextUtils::IsPlatformSupported() const
{
  return m_platformSupported;
}

CEGLContextUtils::~CEGLContextUtils()
{
  Destroy();
}

bool CEGLContextUtils::CreateDisplay(EGLNativeDisplayType nativeDisplay)
{
  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    throw std::logic_error("Do not call CreateDisplay when display has already been created");
  }

  m_eglDisplay = eglGetDisplay(nativeDisplay);
  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    CEGLUtils::Log(LOGERROR, "failed to get EGL display");
    return false;
  }

  return true;
}

bool CEGLContextUtils::CreatePlatformDisplay(void* nativeDisplay, EGLNativeDisplayType nativeDisplayLegacy)
{
  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    throw std::logic_error("Do not call CreateDisplay when display has already been created");
  }

#if defined(EGL_EXT_platform_base)
  if (IsPlatformSupported())
  {
    // Theoretically it is possible to use eglGetDisplay() and eglCreateWindowSurface,
    // but then the EGL library basically has to guess which platform we want
    // if it supports multiple which is usually the case -
    // it's better and safer to make it explicit

    auto getPlatformDisplayEXT = CEGLUtils::GetRequiredProcAddress<PFNEGLGETPLATFORMDISPLAYEXTPROC>("eglGetPlatformDisplayEXT");
    m_eglDisplay = getPlatformDisplayEXT(m_platform, nativeDisplay, nullptr);

    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      CEGLUtils::Log(LOGERROR, "failed to get platform display");
      return false;
    }
  }
#endif

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    return CreateDisplay(nativeDisplayLegacy);
  }

  return true;
}

bool CEGLContextUtils::InitializeDisplay(EGLint renderingApi)
{
  if (!eglInitialize(m_eglDisplay, nullptr, nullptr))
  {
    CEGLUtils::Log(LOGERROR, "failed to initialize EGL display");
    Destroy();
    return false;
  }

  const char* value;
  value = eglQueryString(m_eglDisplay, EGL_VERSION);
  CLog::Log(LOGINFO, "EGL_VERSION = {}", value ? value : "NULL");

  value = eglQueryString(m_eglDisplay, EGL_VENDOR);
  CLog::Log(LOGINFO, "EGL_VENDOR = {}", value ? value : "NULL");

  value = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
  CLog::Log(LOGINFO, "EGL_EXTENSIONS = {}", value ? value : "NULL");

  value = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  CLog::Log(LOGINFO, "EGL_CLIENT_EXTENSIONS = {}", value ? value : "NULL");

  if (eglBindAPI(renderingApi) != EGL_TRUE)
  {
    CEGLUtils::Log(LOGERROR, "failed to bind EGL API");
    Destroy();
    return false;
  }

  return true;
}

bool CEGLContextUtils::ChooseConfig(EGLint renderableType, EGLint visualId, bool hdr)
{
  EGLint numMatched{0};

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    throw std::logic_error("Choosing an EGLConfig requires an EGL display");
  }

  EGLint surfaceType = EGL_WINDOW_BIT;
  // for the non-trivial dirty region modes, we need the EGL buffer to be preserved across updates
  int guiAlgorithmDirtyRegions = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAlgorithmDirtyRegions;
  if (guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_COST_REDUCTION ||
      guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_UNION)
    surfaceType |= EGL_SWAP_BEHAVIOR_PRESERVED_BIT;

  CEGLAttributesVec attribs;
  attribs.Add({{EGL_RED_SIZE, 8},
               {EGL_GREEN_SIZE, 8},
               {EGL_BLUE_SIZE, 8},
               {EGL_ALPHA_SIZE, 2},
               {EGL_DEPTH_SIZE, 16},
               {EGL_STENCIL_SIZE, 0},
               {EGL_SAMPLE_BUFFERS, 0},
               {EGL_SAMPLES, 0},
               {EGL_SURFACE_TYPE, surfaceType},
               {EGL_RENDERABLE_TYPE, renderableType}});

  EGLConfig* currentConfig(hdr ? &m_eglHDRConfig : &m_eglConfig);

  if (hdr)
#if EGL_EXT_pixel_format_float
    attribs.Add({{EGL_COLOR_COMPONENT_TYPE_EXT, EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT}});
#else
    return false;
#endif

  const char* errorMsg = nullptr;

  if (eglChooseConfig(m_eglDisplay, attribs.Get(), nullptr, 0, &numMatched) != EGL_TRUE)
    errorMsg = "failed to query number of EGL configs";

  std::vector<EGLConfig> eglConfigs(numMatched);
  if (eglChooseConfig(m_eglDisplay, attribs.Get(), eglConfigs.data(), numMatched, &numMatched) != EGL_TRUE)
    errorMsg = "failed to find EGL configs with appropriate attributes";

  if (errorMsg)
  {
    if (!hdr)
    {
      CEGLUtils::Log(LOGERROR, errorMsg);
      Destroy();
    }
    else
      CEGLUtils::Log(LOGINFO, errorMsg);
    return false;
  }

  EGLint id{0};
  for (const auto &eglConfig: eglConfigs)
  {
    *currentConfig = eglConfig;

    if (visualId == 0)
      break;

    if (eglGetConfigAttrib(m_eglDisplay, *currentConfig, EGL_NATIVE_VISUAL_ID, &id) != EGL_TRUE)
      CEGLUtils::Log(LOGERROR, "failed to query EGL attribute EGL_NATIVE_VISUAL_ID");

    if (visualId == id)
      break;
  }

  if (visualId != 0 && visualId != id)
  {
    CLog::Log(LOGDEBUG, "failed to find EGL config with EGL_NATIVE_VISUAL_ID={}", visualId);
    return false;
  }

  CLog::Log(LOGDEBUG, "EGL {}Config Attributes:", hdr ? "HDR " : "");

  for (const auto &eglAttribute : eglAttributes)
  {
    EGLint value{0};
    if (eglGetConfigAttrib(m_eglDisplay, *currentConfig, eglAttribute.first, &value) != EGL_TRUE)
      CEGLUtils::Log(LOGERROR,
                     StringUtils::Format("failed to query EGL attribute {}", eglAttribute.second));

    // we only need to print the hex value if it's an actual EGL define
    CLog::Log(LOGDEBUG, "  {}: {}", eglAttribute.second,
              (value >= 0x3000 && value <= 0x3200) ? StringUtils::Format("{:#04x}", value)
                                                   : std::to_string(value));
  }

  return true;
}

EGLint CEGLContextUtils::GetConfigAttrib(EGLint attribute) const
{
  EGLint value{0};
  if (eglGetConfigAttrib(m_eglDisplay, m_eglConfig, attribute, &value) != EGL_TRUE)
    CEGLUtils::Log(LOGERROR, "failed to query EGL attribute");
  return value;
}

bool CEGLContextUtils::CreateContext(CEGLAttributesVec contextAttribs)
{
  if (m_eglContext != EGL_NO_CONTEXT)
  {
    throw std::logic_error("Do not call CreateContext when context has already been created");
  }

  EGLConfig eglConfig{m_eglConfig};

  if (CEGLUtils::HasExtension(m_eglDisplay, "EGL_KHR_no_config_context"))
    eglConfig = EGL_NO_CONFIG_KHR;

  if (CEGLUtils::HasExtension(m_eglDisplay, "EGL_IMG_context_priority"))
    contextAttribs.Add({{EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_HIGH_IMG}});

  if (CEGLUtils::HasExtension(m_eglDisplay, "EGL_KHR_create_context") &&
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_openGlDebugging)
  {
    contextAttribs.Add({{EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR}});
  }

  m_eglContext = eglCreateContext(m_eglDisplay, eglConfig,
                                  EGL_NO_CONTEXT, contextAttribs.Get());

  if (CEGLUtils::HasExtension(m_eglDisplay, "EGL_IMG_context_priority"))
  {
    EGLint value{EGL_CONTEXT_PRIORITY_MEDIUM_IMG};

    if (eglQueryContext(m_eglDisplay, m_eglContext, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &value) != EGL_TRUE)
      CEGLUtils::Log(LOGERROR, "failed to query EGL context attribute EGL_CONTEXT_PRIORITY_LEVEL_IMG");

    if (value != EGL_CONTEXT_PRIORITY_HIGH_IMG)
      CLog::Log(LOGDEBUG, "Failed to obtain a high priority EGL context");
  }

  if (m_eglContext == EGL_NO_CONTEXT)
  {
    // This is expected to fail under some circumstances, so log as debug
    CLog::Log(LOGDEBUG, "Failed to create EGL context (EGL error {})", eglGetError());
    return false;
  }

  return true;
}

bool CEGLContextUtils::BindContext()
{
  if (m_eglDisplay == EGL_NO_DISPLAY || m_eglSurface == EGL_NO_SURFACE || m_eglContext == EGL_NO_CONTEXT)
  {
    throw std::logic_error("Activating an EGLContext requires display, surface, and context");
  }

  if (eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext) != EGL_TRUE)
  {
    CLog::Log(LOGERROR, "Failed to make context current {} {} {}", fmt::ptr(m_eglDisplay),
              fmt::ptr(m_eglSurface), fmt::ptr(m_eglContext));
    return false;
  }

  return true;
}

void CEGLContextUtils::SurfaceAttrib()
{
  if (m_eglDisplay == EGL_NO_DISPLAY || m_eglSurface == EGL_NO_SURFACE)
  {
    throw std::logic_error("Setting surface attributes requires a surface");
  }

  // for the non-trivial dirty region modes, we need the EGL buffer to be preserved across updates
  int guiAlgorithmDirtyRegions = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAlgorithmDirtyRegions;
  if (guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_COST_REDUCTION ||
      guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_UNION)
  {
    if (eglSurfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED) != EGL_TRUE)
    {
      CEGLUtils::Log(LOGERROR, "failed to set EGL_BUFFER_PRESERVED swap behavior");
    }
  }
}

void CEGLContextUtils::SurfaceAttrib(EGLint attribute, EGLint value)
{
  if (eglSurfaceAttrib(m_eglDisplay, m_eglSurface, attribute, value) != EGL_TRUE)
  {
    CEGLUtils::Log(LOGERROR, "failed to set EGL_BUFFER_PRESERVED swap behavior");
  }
}

bool CEGLContextUtils::CreateSurface(EGLNativeWindowType nativeWindow, EGLint HDRcolorSpace /* = EGL_NONE */)
{
  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    throw std::logic_error("Creating a surface requires a display");
  }
  if (m_eglSurface != EGL_NO_SURFACE)
  {
    throw std::logic_error("Do not call CreateSurface when surface has already been created");
  }

  CEGLAttributesVec attribs;
  EGLConfig config = m_eglConfig;

#ifdef EGL_GL_COLORSPACE
  if (HDRcolorSpace != EGL_NONE)
  {
    attribs.Add({{EGL_GL_COLORSPACE, HDRcolorSpace}});
    config = m_eglHDRConfig;
  }
#endif

  m_eglSurface = eglCreateWindowSurface(m_eglDisplay, config, nativeWindow, attribs.Get());

  if (m_eglSurface == EGL_NO_SURFACE)
  {
    CEGLUtils::Log(LOGERROR, "failed to create window surface");
    return false;
  }

  SurfaceAttrib();

  return true;
}

bool CEGLContextUtils::CreatePlatformSurface(void* nativeWindow, EGLNativeWindowType nativeWindowLegacy)
{
  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    throw std::logic_error("Creating a surface requires a display");
  }
  if (m_eglSurface != EGL_NO_SURFACE)
  {
    throw std::logic_error("Do not call CreateSurface when surface has already been created");
  }

#if defined(EGL_EXT_platform_base)
  if (IsPlatformSupported())
  {
    auto createPlatformWindowSurfaceEXT = CEGLUtils::GetRequiredProcAddress<PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC>("eglCreatePlatformWindowSurfaceEXT");
    m_eglSurface = createPlatformWindowSurfaceEXT(m_eglDisplay, m_eglConfig, nativeWindow, nullptr);

    if (m_eglSurface == EGL_NO_SURFACE)
    {
      CEGLUtils::Log(LOGERROR, "failed to create platform window surface");
      return false;
    }
  }
#endif

  if (m_eglSurface == EGL_NO_SURFACE)
  {
    return CreateSurface(nativeWindowLegacy);
  }

  SurfaceAttrib();

  return true;
}

void CEGLContextUtils::Destroy()
{
  DestroyContext();
  DestroySurface();

  if (m_eglDisplay != EGL_NO_DISPLAY)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = EGL_NO_DISPLAY;
  }
}

void CEGLContextUtils::DestroyContext()
{
  if (m_eglContext != EGL_NO_CONTEXT)
  {
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(m_eglDisplay, m_eglContext);
    m_eglContext = EGL_NO_CONTEXT;
  }
}

void CEGLContextUtils::DestroySurface()
{
  if (m_eglSurface != EGL_NO_SURFACE)
  {
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }
}


bool CEGLContextUtils::SetVSync(bool enable)
{
  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    return false;
  }

  return (eglSwapInterval(m_eglDisplay, enable) == EGL_TRUE);
}

bool CEGLContextUtils::TrySwapBuffers()
{
  if (m_eglDisplay == EGL_NO_DISPLAY || m_eglSurface == EGL_NO_SURFACE)
  {
    return false;
  }

  return (eglSwapBuffers(m_eglDisplay, m_eglSurface) == EGL_TRUE);
}
