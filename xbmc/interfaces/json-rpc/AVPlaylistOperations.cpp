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

#include "AVPlaylistOperations.h"
#include "PlayListPlayer.h"
#include "playlists/PlayListFactory.h"
#include "Util.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "Application.h"

using namespace JSONRPC;
using namespace PLAYLIST;
using namespace std;

JSON_STATUS CAVPlaylistOperations::Play(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  bool status = true;
  int playlist = GetPlaylist(method);
  if (g_playlistPlayer.GetCurrentPlaylist() != playlist)
    g_playlistPlayer.SetCurrentPlaylist(playlist);

  int item = parameterObject["item"].asInteger();
  int songId = parameterObject["songid"].asInteger();

  if (item >= 0)
    g_application.getApplicationMessenger().PlayListPlayerPlay(item);
  else if (playlist == PLAYLIST_MUSIC && songId > 0)
    status = g_application.getApplicationMessenger().PlayListPlayerPlaySongId(songId);
  else
    g_application.getApplicationMessenger().PlayListPlayerPlay();

  result["success"] = status;
  NotifyAll();
  return OK;
}

JSON_STATUS CAVPlaylistOperations::SkipPrevious(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (g_playlistPlayer.GetCurrentPlaylist() != GetPlaylist(method))
    return FailedToExecute;

  g_application.getApplicationMessenger().PlayListPlayerPrevious();

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::SkipNext(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (g_playlistPlayer.GetCurrentPlaylist() != GetPlaylist(method))
    return FailedToExecute;

  g_application.getApplicationMessenger().PlayListPlayerNext();

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::GetItems(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CFileItemList list;
  int playlist = GetPlaylist(method);

  g_application.getApplicationMessenger().PlayListPlayerGetItems(playlist, list);

  HandleFileItemList("id", true, "items", list, parameterObject, result);

  if (g_playlistPlayer.GetCurrentPlaylist() == GetPlaylist(method))
  {
    result["state"]["current"] = g_playlistPlayer.GetCurrentSong();
    result["state"]["playing"] = g_application.IsPlaying();
    result["state"]["paused"] = g_application.IsPaused();
  }
  return OK;
}

JSON_STATUS CAVPlaylistOperations::Add(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(method);
  CFileItemList list;
  CVariant params = parameterObject;
  if (playlist == PLAYLIST_VIDEO)
    params["item"]["media"] = "video";
  else if (playlist == PLAYLIST_MUSIC)
    params["item"]["media"] = "music";

  if (!FillFileItemList(params["item"], list))
    return InvalidParams;

  g_application.getApplicationMessenger().PlayListPlayerAdd(playlist, list);

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::Insert(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(method);
  CFileItemList list;
  CVariant params = parameterObject;
  if (playlist == PLAYLIST_VIDEO)
    params["item"]["media"] = "video";
  else if (playlist == PLAYLIST_MUSIC)
    params["item"]["media"] = "music";

  if (!FillFileItemList(params["item"], list))
    return InvalidParams;

  g_application.getApplicationMessenger().PlayListPlayerInsert(GetPlaylist(method), list, parameterObject["index"].asInteger());

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::Remove(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  g_application.getApplicationMessenger().PlayListPlayerRemove(GetPlaylist(method), parameterObject["item"].asInteger());

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::Clear(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  g_application.getApplicationMessenger().PlayListPlayerClear(GetPlaylist(method));

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::Shuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  g_application.getApplicationMessenger().PlayListPlayerShuffle(GetPlaylist(method), true);

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::UnShuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  g_application.getApplicationMessenger().PlayListPlayerShuffle(GetPlaylist(method), false);

  NotifyAll();
  return ACK;
}

int CAVPlaylistOperations::GetPlaylist(const CStdString &method)
{
  CStdString methodStart = method.Left(5);
  if (methodStart.Equals("video"))
    return PLAYLIST_VIDEO;
  else if (methodStart.Equals("audio"))
    return PLAYLIST_MUSIC;
  else
    return PLAYLIST_NONE;
}

void CAVPlaylistOperations::NotifyAll()
{
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}
