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

#include "PlayerOperations.h"
#include "Application.h"
#include "Builtins.h"
#include "Util.h"
#include "PlayListPlayer.h"
#include "GUIWindowManager.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CPlayerOperations::GetActivePlayers(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  result["players"] = arrayValue;

  if (g_application.IsPlayingVideo())
    result["players"].append("video");
  else if (g_application.IsPlayingAudio())
    result["players"].append("music");

  if (g_windowManager.IsWindowActive(WINDOW_SLIDESHOW))
    result["players"].append("picture");

  return OK;
}

JSON_STATUS CPlayerOperations::PlayPause(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(play)");
  return ACK;
}

JSON_STATUS CPlayerOperations::Stop(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_STOP));
  return ACK;
}

JSON_STATUS CPlayerOperations::SkipPrevious(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_PREV_ITEM));
  return ACK;
}

JSON_STATUS CPlayerOperations::SkipNext(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_NEXT_ITEM));
  return ACK;
}
