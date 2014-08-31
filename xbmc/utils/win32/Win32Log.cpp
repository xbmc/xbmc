/*
*      Copyright (C) 2014 Team XBMC
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

#include "Win32Log.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"

void CWin32Log::LogW(int loglevel, const wchar_t* format, ...)
{
  if (IsLogLevelLogged(loglevel))
  {
    va_list va;
    va_start(va, format);
    std::wstring strDataW(StringUtils::FormatV(format, va));
    va_end(va);
    if (!strDataW.empty())
    {
      std::string strDataUtf8;
      if (g_charsetConverter.wToUTF8(strDataW, strDataUtf8, false) && !strDataUtf8.empty())
        LogString(loglevel, strDataUtf8);
      else
        PrintDebugString(__FUNCTION__ ": Can't convert log wide string to UTF-8");
    }
  }
}

void CWin32Log::LogFunctionW(int loglevel, const char* functionName, const wchar_t* format, ...)
{
  if (IsLogLevelLogged(loglevel))
  {
    va_list va;
    va_start(va, format);
    std::wstring strDataW(StringUtils::FormatV(format, va));
    va_end(va);
    if (!strDataW.empty())
    {
      std::string funcNameStr;
      if (functionName && functionName[0])
        funcNameStr.assign(functionName).append(": ");

      std::string strDataUtf8;
      if (g_charsetConverter.wToUTF8(strDataW, strDataUtf8, false) && !strDataUtf8.empty())
        LogString(loglevel, funcNameStr + strDataUtf8);
      else
        PrintDebugString(__FUNCTION__ ": Can't convert log wide string to UTF-8");
    }
  }
}
