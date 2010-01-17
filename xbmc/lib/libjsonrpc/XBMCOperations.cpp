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

#include "XBMCOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "Util.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CXBMCOperations::GetVolume(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  Value val = g_application.GetVolume();
  result.swap(val);
  return OK;
}

JSON_STATUS CXBMCOperations::SetVolume(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!parameterObject.isInt())
    return InvalidParams;

  int percentage = parameterObject.asInt();
  g_application.SetVolume(percentage);
  return GetVolume(method, transport, client, parameterObject, result);
}

JSON_STATUS CXBMCOperations::ToggleMute(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  CAction action;
  action.id = ACTION_MUTE;
  if (g_application.OnAction(action))
    return GetVolume(method, transport, client, parameterObject, result);
  else
    return FailedToExecute;
}

JSON_STATUS CXBMCOperations::Play(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!parameterObject.isString())
    return InvalidParams;

  CStdString path = parameterObject.asString();
  CFileItem item(path, CUtil::HasSlashAtEnd(path));
  if (g_application.PlayMedia(item, item.IsAudio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO))
  {
    Value val = "OK";
    result.swap(val);
    return OK;
  }
  else
    return FailedToExecute;
}

JSON_STATUS CXBMCOperations::StartSlideshow(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  CStdString exec = "slideShow(";

  if (parameterObject.isString())
    exec += parameterObject.asString();
  else if (parameterObject.isObject() && parameterObject.isMember("directory") && parameterObject["directory"].isString())
  {
    exec += parameterObject["directory"].asString();

    if (parameterObject.get("random", true).asBool())
      exec += ", random";
    else
      exec += ", notrandom";

    if (parameterObject.get("recursive", true).asBool())
      exec += ", recursive";
  }
  else
    return InvalidParams;

  exec += ")";
  ThreadMessage msg = { TMSG_EXECUTE_BUILT_IN, (DWORD)0, (DWORD)0, exec };
  g_application.getApplicationMessenger().SendMessage(msg);

  Value val = "OK";
  result.swap(val);
  return OK;
}

JSON_STATUS CXBMCOperations::Quit(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  g_application.getApplicationMessenger().Quit();
  Value val = "OK";
  result.swap(val);
  return OK;
}
