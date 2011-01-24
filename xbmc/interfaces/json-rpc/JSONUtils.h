#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "JSONRPC.h"

namespace JSONRPC
{
  class CJSONUtils
  {
  protected:
    static inline bool ParameterIntOrNull(const Json::Value &parameterObject, const char *key) { return parameterObject[key].isInt() || parameterObject[key].isNull(); }
    static inline int ParameterAsInt(const Json::Value &parameterObject, int fallback, const char *key) { return parameterObject[key].isInt() ? parameterObject[key].asInt() : fallback; }

    static inline const Json::Value ForceObject(const Json::Value &value) { return value.isObject() ? value : Json::Value(Json::objectValue); }
  };
}
