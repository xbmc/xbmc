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

#include "PlayerActions.h"
#include "Application.h"
#include "Builtins.h"
#include "Util.h"
#include "PlayListPlayer.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CPlayerActions::GetActivePlayers(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (g_application.IsPlayingVideo())
    result["players"].append("video");
  else if (g_application.IsPlayingAudio())
    result["players"].append("music");

  return OK;
}

JSON_STATUS CPlayerActions::PlayPause(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CBuiltins::Execute("playercontrol(play)");
  result["playing"] = g_application.IsPlaying();
  result["paused"] = g_application.IsPaused();
  return OK;
}

JSON_STATUS CPlayerActions::Stop(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CAction action;
  action.id = ACTION_STOP;
  return g_application.OnAction(action) ? ACK : FailedToExecute;
}

JSON_STATUS CPlayerActions::SkipPrevious(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CAction action;
  action.id = ACTION_PREV_ITEM;
  return g_application.OnAction(action) ? ACK : FailedToExecute;
}

JSON_STATUS CPlayerActions::SkipNext(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CAction action;
  action.id = ACTION_NEXT_ITEM;
  return g_application.OnAction(action) ? ACK : FailedToExecute;
}

JSON_STATUS CPlayerActions::BigSkipBackward(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CBuiltins::Execute("playercontrol(bigskipbackward)");
  return ACK;
}

JSON_STATUS CPlayerActions::BigSkipForward(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CBuiltins::Execute("playercontrol(bigskipforward)");
  return ACK;
}

JSON_STATUS CPlayerActions::SmallSkipBackward(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CBuiltins::Execute("playercontrol(smallskipbackward)");
  return ACK;
}

JSON_STATUS CPlayerActions::SmallSkipForward(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CBuiltins::Execute("playercontrol(smallskipforward)");
  return ACK;
}

JSON_STATUS CPlayerActions::Rewind(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CAction action;
  action.id = ACTION_PLAYER_REWIND;
  return g_application.OnAction(action) ? ACK : FailedToExecute;
}

JSON_STATUS CPlayerActions::Forward(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CAction action;
  action.id = ACTION_PLAYER_FORWARD;
  return g_application.OnAction(action) ? ACK : FailedToExecute;
}

JSON_STATUS CPlayerActions::Record(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  CBuiltins::Execute("playercontrol(record)");
  return ACK;
}

JSON_STATUS CPlayerActions::GetTime(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  result["time"] = (int)g_application.GetTime();
  result["total"] = (int)g_application.GetTotalTime();
  return OK;
}

JSON_STATUS CPlayerActions::GetTimeMS(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  result["time"] = (int)(g_application.GetTime() * 1000.0);
  result["total"] = (int)(g_application.GetTotalTime() * 1000.0);
  return OK;
}

JSON_STATUS CPlayerActions::GetPercentage(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  Value val = g_application.GetPercentage();
  result.swap(val);
  return OK;
}

JSON_STATUS CPlayerActions::SeekTime(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  if (!parameterObject.isInt())
    return InvalidParams;
  if (!IsCorrectPlayer(method))
    return FailedToExecute;

  g_application.SeekTime(parameterObject.asInt());
  return ACK;
}

JSON_STATUS CPlayerActions::GetPlaylist(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value& parameterObject, Json::Value &result)
{
  int playlist = PLAYLIST_NONE;
  if (method.Left(5).Equals("music"))
    playlist = PLAYLIST_MUSIC;
  else if (method.Left(5).Equals("video"))
    playlist = PLAYLIST_VIDEO;

  Value val = playlist;
  result.swap(val);

  return OK;
}

bool CPlayerActions::IsCorrectPlayer(const CStdString &method)
{
  return (method.Left(5).Equals("music") && g_application.IsPlayingAudio()) || (method.Left(5).Equals("video") && g_application.IsPlayingVideo());
}
