/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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