/*
 *      Copyright (C) 2010 Marcel Groothuis
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
 *
 */
#if defined(TARGET_WINDOWS)
#pragma warning(disable:4244) //wchar to char = loss of data
#endif

#include "utils.h"
#include "client.h" //For XBMC->Log
#include "libPlatform/os-dependent.h"
#include <string>
#include <algorithm> // sort

using namespace ADDON;

namespace Json
{
  void printValueTree( const Json::Value& value, const std::string& path)
  {
     switch ( value.type() )
     {
     case Json::nullValue:
        XBMC->Log(LOG_DEBUG, "%s=null\n", path.c_str() );
        break;
     case Json::intValue:
        XBMC->Log(LOG_DEBUG, "%s=%d\n", path.c_str(), value.asInt() );
        break;
     case Json::uintValue:
        XBMC->Log(LOG_DEBUG, "%s=%u\n", path.c_str(), value.asUInt() );
        break;
     case Json::realValue:
        XBMC->Log(LOG_DEBUG, "%s=%.16g\n", path.c_str(), value.asDouble() );
        break;
     case Json::stringValue:
        XBMC->Log(LOG_DEBUG, "%s=\"%s\"\n", path.c_str(), value.asString().c_str() );
        break;
     case Json::booleanValue:
        XBMC->Log(LOG_DEBUG, "%s=%s\n", path.c_str(), value.asBool() ? "true" : "false" );
        break;
     case Json::arrayValue:
        {
           XBMC->Log(LOG_DEBUG, "%s=[]\n", path.c_str() );
           int size = value.size();
           for ( int index =0; index < size; ++index )
           {
              static char buffer[16];
              snprintf( buffer, 16, "[%d]", index );
              printValueTree( value[index], path + buffer );
           }
        }
        break;
     case Json::objectValue:
        {
           XBMC->Log(LOG_DEBUG, "%s={}\n", path.c_str() );
           Json::Value::Members members( value.getMemberNames() );
           std::sort( members.begin(), members.end() );
           std::string suffix = *(path.end()-1) == '.' ? "" : ".";
           for ( Json::Value::Members::iterator it = members.begin(); 
                 it != members.end(); 
                 ++it )
           {
              const std::string &name = *it;
              printValueTree( value[name], path + suffix + name );
           }
        }
        break;
     default:
        break;
     }
  }
} //namespace Json
