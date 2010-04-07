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
#include "PlayListFactory.h"
#include "Util.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "Application.h"

using namespace Json;
using namespace JSONRPC;
using namespace PLAYLIST;
using namespace std;

JSON_STATUS CAVPlaylistOperations::Play(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_playlistPlayer.SetCurrentPlaylist(GetPlaylist(method));

  if (parameterObject.isInt())
    g_application.getApplicationMessenger().PlayListPlayerPlay(parameterObject.asInt());
  else
    g_application.getApplicationMessenger().PlayListPlayerPlay();

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::SkipPrevious(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_playlistPlayer.SetCurrentPlaylist(GetPlaylist(method));
  g_playlistPlayer.PlayPrevious();

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::SkipNext(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_playlistPlayer.SetCurrentPlaylist(GetPlaylist(method));
  g_playlistPlayer.PlayNext();

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::GetItems(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CPlayList *playlist = &g_playlistPlayer.GetPlaylist(GetPlaylist(method));
  if (playlist == NULL)
    return FailedToExecute;

  CFileItemList items;
  if (playlist)
  {
    for (int i = 0; i < playlist->size(); i++)
      items.Add((*playlist)[i]);

    CStdString name = playlist->GetName();
    if (!name.IsEmpty())
      result["name"] = playlist->GetName();
  }

  HandleFileItemList(NULL, "items", items, parameterObject, result);

  if (g_playlistPlayer.GetCurrentPlaylist() == GetPlaylist(method))
    result["current"] = g_playlistPlayer.GetCurrentSong();

  return OK;
}

JSON_STATUS CAVPlaylistOperations::Add(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CFileItemList list;
  if (!FillFileItemList(parameterObject, list))
    return InvalidParams;

  g_playlistPlayer.Add(GetPlaylist(method), list);

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::Clear(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_playlistPlayer.ClearPlaylist(GetPlaylist(method));

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::Shuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_playlistPlayer.SetShuffle(GetPlaylist(method), true);

  NotifyAll();
  return ACK;
}

JSON_STATUS CAVPlaylistOperations::UnShuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_playlistPlayer.SetShuffle(GetPlaylist(method), false);

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
