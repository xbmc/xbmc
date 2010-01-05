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

#include "StdString.h"
#include "JSONRPC.h"

class CPlayerActions
{
public:
  static JSON_STATUS GetActivePlayers(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetAvailablePlayers(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS PlayPause(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS Stop(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SkipPrevious(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SkipNext(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS BigSkipBackward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS BigSkipForward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SmallSkipBackward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SmallSkipForward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS Rewind(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS Forward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS Record(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
};
