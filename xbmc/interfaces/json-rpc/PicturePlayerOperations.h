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

#include "utils/StdString.h"
#include "JSONRPC.h"

namespace JSONRPC
{
  class CPicturePlayerOperations
  {
  public:
    static JSON_STATUS PlayPause(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS Stop(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS SkipPrevious(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS SkipNext(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);

    static JSON_STATUS MoveLeft(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS MoveRight(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS MoveDown(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS MoveUp(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);

    static JSON_STATUS ZoomOut(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS ZoomIn(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS Zoom(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
    static JSON_STATUS Rotate(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);

  private:
    static inline JSON_STATUS SendAction(int actionID);
//    static JSON_STATUS Move(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result);
  };
}
