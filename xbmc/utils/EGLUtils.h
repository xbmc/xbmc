/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <set>
#include <string>
#include <stdexcept>

#include <EGL/egl.h>

#include "StringUtils.h"

class CEGLUtils
{
public:
  static std::set<std::string> GetClientExtensions();
  static std::set<std::string> GetExtensions(EGLDisplay eglDisplay);
  static bool HasExtension(EGLDisplay eglDisplay, std::string const & name);
  static void LogError(std::string const & what);
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

private:
  std::array<EGLint, AttributeCount * 2 + 1> m_attributes;
  int m_writePosition{};
};

class CEGLContextUtils final
{
public:
  CEGLContextUtils();
  ~CEGLContextUtils();

  bool CreateDisplay(EGLDisplay display,
                     EGLint renderable_type,
                     EGLint rendering_api);

  bool CreateSurface(EGLNativeWindowType surface);
  bool CreateContext(const EGLint* contextAttribs);
  bool BindContext();
  bool SurfaceAttrib();
  void Destroy();
  void Detach();
  bool SetVSync(bool enable);
  void SwapBuffers();

  EGLDisplay m_eglDisplay;
  EGLSurface m_eglSurface;
  EGLContext m_eglContext;
  EGLConfig m_eglConfig = 0;
};
