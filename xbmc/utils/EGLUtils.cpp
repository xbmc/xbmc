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

#include "EGLUtils.h"
#include "log.h"

std::set<std::string> CEGLUtils::GetClientExtensions()
{
  const char* extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  if (!extensions)
  {
    throw std::runtime_error("Could not query EGL for client extensions, maybe EGL_EXT_client_extensions is not supported?");
  }
  std::set<std::string> result;
  StringUtils::SplitTo(std::inserter(result, result.begin()), extensions, " ");
  return result;
}

void CEGLUtils::LogError(const std::string& what)
{
  CLog::Log(LOGERROR, "%s (EGL error %d)", what.c_str(), eglGetError());
}
