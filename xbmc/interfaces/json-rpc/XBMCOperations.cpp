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
#include "utils/log.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CXBMCOperations::GetVolume(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  Value val = g_application.GetVolume();
  result.swap(val);
  return OK;
}

JSON_STATUS CXBMCOperations::SetVolume(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.SetVolume(parameterObject["value"].asInt());
  return GetVolume(method, transport, client, parameterObject, result);
}

JSON_STATUS CXBMCOperations::ToggleMute(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_MUTE));
  return GetVolume(method, transport, client, parameterObject, result);
}

JSON_STATUS CXBMCOperations::Play(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CFileItemList list;
  if (FillFileItemList(parameterObject, list) && list.Size() > 0)
  {
    g_application.getApplicationMessenger().MediaPlay(list);
    return ACK;
  }
  else
    return InvalidParams;
}

JSON_STATUS CXBMCOperations::StartSlideshow(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CStdString exec = "slideShow(";

  exec += parameterObject["directory"].asString();

  if (parameterObject["random"].asBool())
    exec += ", random";
  else
    exec += ", notrandom";

  if (parameterObject["recursive"].asBool())
    exec += ", recursive";

  exec += ")";
  ThreadMessage msg = { TMSG_EXECUTE_BUILT_IN, (DWORD)0, (DWORD)0, exec };
  g_application.getApplicationMessenger().SendMessage(msg);

  return ACK;
}

JSON_STATUS CXBMCOperations::Log(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CStdString message = parameterObject["message"].asString();
  if (message.IsEmpty())
    return InvalidParams;

  CStdString strLevel = parameterObject["level"].asString();
  int level = ParseLogLevel(strLevel.ToLower().c_str());

  CLog::Log(level, "%s", message.c_str());

  return ACK;
}

JSON_STATUS CXBMCOperations::Quit(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().Quit();
  return ACK;
}

int CXBMCOperations::ParseLogLevel(const char *level)
{
  if (strcmp(level, "debug") == 0)
    return LOGDEBUG;
  else if (strcmp(level, "info") == 0)
    return LOGINFO;
  else if (strcmp(level, "notice") == 0)
    return LOGNOTICE;
  else if (strcmp(level, "warning") == 0)
    return LOGWARNING;
  else if (strcmp(level, "error") == 0)
    return LOGERROR;
  else if (strcmp(level, "severe") == 0)
    return LOGSEVERE;
  else if (strcmp(level, "fatal") == 0)
    return LOGFATAL;
  else if (strcmp(level, "none") == 0)
    return LOGNONE;
  else
    return LOGNONE;
}
