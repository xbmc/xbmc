/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess.h"
#include KITINCLUDE(ADDON_API_LEVEL, AddonLib.hpp)

#include "../version.h"

#include <string>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <stdio.h>

API_NAMESPACE

namespace KodiAPI
{
extern "C"
{

int KODI_API_Level = ADDON_API_LEVEL;
const char* KODI_API_Version  = ADDON_API_VERSION;

int InitLibAddon(void* hdl)
{
  return g_interProcess.InitLibAddon(hdl);
}

int Finalize()
{
  return g_interProcess.Finalize();
}

void Log(const addon_log loglevel, const char* format, ... )
{
  char buffer[16384];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  g_interProcess.Log(loglevel, buffer);
}

} /* extern "C" */
} /* namespace KodiAPI */

END_NAMESPACE()
