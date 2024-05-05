/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <array>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "system_egl.h"

class CEGLUtils
{
public:
  static std::set<std::string> GetClientExtensions();
  static std::set<std::string> GetExtensions(EGLDisplay eglDisplay);
  static bool HasExtension(EGLDisplay eglDisplay, std::string const & name);
  static bool HasClientExtension(std::string const& name);
  static void Log(int logLevel, std::string const& what);
  template<typename T>
  static T GetRequiredProcAddress(const char * procname)
  {
    T p = reinterpret_cast<T>(eglGetProcAddress(procname));
    if (!p)
    {
      throw std::runtime_error(std::string("Could not get EGL function \"") + procname + "\" - maybe a required extension is not supported?");
    }
    return p;
  }

private:
  CEGLUtils();
};

/**
 * Convenience wrapper for heap-allocated EGL attribute arrays
 *
 * The wrapper makes sure that the key/value pairs are always written in actual
 * pairs and  that the array is always terminated with EGL_NONE.
 */
class CEGLAttributesVec
{
public:
  struct EGLAttribute
  {
    EGLint key;
    EGLint value;
  };

  /**
   * Add multiple attributes
   *
   * The array is automatically terminated with EGL_NONE
   */
  void Add(std::initializer_list<EGLAttribute> const& attributes)
  {
    for (auto const& attribute : attributes)
    {
      m_attributes.insert(m_attributes.begin(), attribute.value);
      m_attributes.insert(m_attributes.begin(), attribute.key);
    }
  }

  /**
   * Add one attribute
   *
   * The array is automatically terminated with EGL_NONE
   */
  void Add(EGLAttribute const& attribute)
  {
    Add({attribute});
  }

  EGLint const * Get() const
  {
    return m_attributes.data();
  }

private:
  std::vector<EGLint> m_attributes{EGL_NONE};
};

/**
 * Convenience wrapper for stack-allocated EGL attribute arrays
 *
 * The wrapper makes sure that the key/value pairs are always written in actual
 * pairs, that the array is always terminated with EGL_NONE, and that the bounds
 * of the array are not exceeded (checked on runtime).
 *
 * \tparam AttributeCount maximum number of attributes that can be added.
 *                        Determines the size of the storage array.
 */
template<std::size_t AttributeCount>
class CEGLAttributes
{
public:
  struct EGLAttribute
  {
    EGLint key;
    EGLint value;
  };

  CEGLAttributes()
  {
    m_attributes[0] = EGL_NONE;
  }

  /**
   * Add multiple attributes
   *
   * The array is automatically terminated with EGL_NONE
   *
   * \throws std::out_of_range if more than AttributeCount attributes are added
   *                           in total
   */
  void Add(std::initializer_list<EGLAttribute> const& attributes)
  {
    if (m_writePosition + attributes.size() * 2 + 1 > m_attributes.size())
    {
      throw std::out_of_range("CEGLAttributes::Add");
    }

    for (auto const& attribute : attributes)
    {
      m_attributes[m_writePosition++] = attribute.key;
      m_attributes[m_writePosition++] = attribute.value;
    }
    m_attributes[m_writePosition] = EGL_NONE;
  }

  /**
   * Add one attribute
   *
   * The array is automatically terminated with EGL_NONE
   *
   * \throws std::out_of_range if more than AttributeCount attributes are added
   *                           in total
   */
  void Add(EGLAttribute const& attribute)
  {
    Add({attribute});
  }

  EGLint const * Get() const
  {
    return m_attributes.data();
  }

  int Size() const
  {
    return m_writePosition;
  }

private:
  std::array<EGLint, AttributeCount * 2 + 1> m_attributes;
  int m_writePosition{};
};

class CEGLContextUtils final
{
public:
  CEGLContextUtils() = default;
  /**
   * \param platform platform as constant from an extension building on EGL_EXT_platform_base
   */
  CEGLContextUtils(EGLenum platform, std::string const& platformExtension);
  ~CEGLContextUtils();

  bool CreateDisplay(EGLNativeDisplayType nativeDisplay);
  /**
   * Create EGLDisplay with EGL_EXT_platform_base
   *
   * Falls back to \ref CreateDisplay (with nativeDisplayLegacy) on failure.
   * The native displays to use with the platform-based and the legacy approach
   * may be defined to have different types and/or semantics, so this function takes
   * both as separate parameters.
   *
   * \param nativeDisplay native display to use with eglGetPlatformDisplayEXT
   * \param nativeDisplayLegacy native display to use with eglGetDisplay
   */
  bool CreatePlatformDisplay(void* nativeDisplay, EGLNativeDisplayType nativeDisplayLegacy);

  void SurfaceAttrib(EGLint attribute, EGLint value);
  bool CreateSurface(EGLNativeWindowType nativeWindow, EGLint HDRcolorSpace = EGL_NONE);
  bool CreatePlatformSurface(void* nativeWindow, EGLNativeWindowType nativeWindowLegacy);
  bool InitializeDisplay(EGLint renderingApi);
  bool ChooseConfig(EGLint renderableType, EGLint visualId = 0, bool hdr = false);
  bool CreateContext(CEGLAttributesVec contextAttribs);
  bool BindContext();
  void Destroy();
  void DestroySurface();
  void DestroyContext();
  bool SetVSync(bool enable);
  bool TrySwapBuffers();
  bool IsPlatformSupported() const;
  EGLint GetConfigAttrib(EGLint attribute) const;

  EGLDisplay GetEGLDisplay() const
  {
    return m_eglDisplay;
  }
  EGLSurface GetEGLSurface() const
  {
    return m_eglSurface;
  }
  EGLContext GetEGLContext() const
  {
    return m_eglContext;
  }
  EGLConfig GetEGLConfig() const
  {
    return m_eglConfig;
  }

  bool BindTextureUploadContext();
  bool UnbindTextureUploadContext();
  bool HasContext();

private:
  void SurfaceAttrib();

  EGLenum m_platform{EGL_NONE};
  bool m_platformSupported{false};

  EGLDisplay m_eglDisplay{EGL_NO_DISPLAY};
  EGLSurface m_eglSurface{EGL_NO_SURFACE};
  EGLContext m_eglContext{EGL_NO_CONTEXT};
  EGLConfig m_eglConfig{}, m_eglHDRConfig{};
  EGLContext m_eglUploadContext{EGL_NO_CONTEXT};
  mutable CCriticalSection m_textureUploadLock;
};
