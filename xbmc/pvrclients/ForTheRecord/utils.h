#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <json/json.h>
#include "platform/util/StdString.h"

namespace Json
{
  /**
   * \brief  Json support function to print the response from ForTheRecord.
   *         Can be used for debugging purposes.
   * \param  value  Reference to the Json::Value that you want to print
   * \param  path   Optional path
   */
  void printValueTree( const Json::Value& value, const std::string& path = "." );
}

#if defined(TARGET_WINDOWS)
namespace UTF8Util
{
  CStdStringW ConvertUTF8ToUTF16(const char* pszTextUTF8);
  CStdStringA ConvertUTF16ToUTF8(const WCHAR * pszTextUTF16);
}
#endif

/** Delete macros that make the pointer NULL again */
#define SAFE_DELETE(p)       do { delete (p);     (p)=NULL; } while (0)
#define SAFE_DELETE_ARRAY(p) do { delete[] (p);   (p)=NULL; } while (0)
