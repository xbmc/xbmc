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

#include "PlaylistOperations.h"
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"

using namespace Json;
using namespace JSONRPC;
using namespace PLAYLIST;
using namespace std;

JSON_STATUS CPlaylistOperations::GetItems(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CPlayList *playlist = NULL;
  bool current;
  if (!GetPlaylist(parameterObject, &playlist, current))
    return InvalidParams;

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

  if (current)
    result["current"] = g_playlistPlayer.GetCurrentSong();

  return OK;
}

JSON_STATUS CPlaylistOperations::Add(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!parameterObject.isMember("file"))
    return InvalidParams;

  CPlayList *playlist = NULL;
  bool current;
  if (!GetPlaylist(parameterObject, &playlist, current))
    return InvalidParams;

  if (parameterObject["file"].isString())
  {
    CStdString file = parameterObject["file"].asString();
    CFileItemPtr item = CFileItemPtr(new CFileItem(file, CUtil::HasSlashAtEnd(file)));
    playlist->Add(item);
  }
  else
    return InvalidParams;

  NotifyAll();
  return ACK;
}

JSON_STATUS CPlaylistOperations::Remove(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!parameterObject.isMember("item"))
    return InvalidParams;

  CPlayList *playlist = NULL;
  bool current;
  if (!GetPlaylist(parameterObject, &playlist, current))
    return InvalidParams;

  if (parameterObject["item"].isInt())
    playlist->Remove(parameterObject["item"].asInt());
  else if (parameterObject["item"].isString())
    playlist->Remove(parameterObject["item"].asString());
  else
    return InvalidParams;

  NotifyAll();
  return ACK;
}

JSON_STATUS CPlaylistOperations::Swap(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!parameterObject.isMember("item1") && !parameterObject.isMember("item2"))
    return InvalidParams;

  CPlayList *playlist = NULL;
  bool current;
  if (!GetPlaylist(parameterObject, &playlist, current))
    return InvalidParams;

  if (parameterObject["item1"].isInt() && parameterObject["item2"].isInt())
    playlist->Swap(parameterObject["item1"].asInt(), parameterObject["item1"].asInt());
  else
    return InvalidParams;

  NotifyAll();
  return ACK;
}

JSON_STATUS CPlaylistOperations::Clear(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CPlayList *playlist = NULL;
  bool current;
  if (!GetPlaylist(parameterObject, &playlist, current))
    return InvalidParams;

  playlist->Clear();

  NotifyAll();
  return ACK;
}

JSON_STATUS CPlaylistOperations::Shuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CPlayList *playlist = NULL;
  bool current;
  if (!GetPlaylist(parameterObject, &playlist, current))
    return InvalidParams;

  playlist->Shuffle();

  NotifyAll();
  return ACK;
}

JSON_STATUS CPlaylistOperations::UnShuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CPlayList *playlist = NULL;
  bool current;
  if (!GetPlaylist(parameterObject, &playlist, current))
    return InvalidParams;

  playlist->UnShuffle();

  NotifyAll();
  return ACK;
}

int CPlaylistOperations::PlaylistFromString(const string &id)
{
  CStdString tempID = id;
  tempID = tempID.ToLower();
  if (tempID.Equals("video"))
    return PLAYLIST_VIDEO;
  else if (tempID.Equals("music"))
    return PLAYLIST_MUSIC;
  else
    return PLAYLIST_NONE;
}

bool CPlaylistOperations::GetPlaylist(const Value &parameterObject, CPlayList **playlist, bool &current)
{
  const Value id = (parameterObject.isObject() && parameterObject.isMember("playlist")) ? parameterObject["playlist"] : Value(nullValue);
  int nbr;
  if (id.isNull() || id.isString())
  {
    nbr = id.isNull() ? g_playlistPlayer.GetCurrentPlaylist() : PlaylistFromString(id.asString());
    *playlist = &g_playlistPlayer.GetPlaylist(nbr);

    current = g_playlistPlayer.GetCurrentPlaylist() == nbr;
    return *playlist != NULL;
  }
  else
    return false;
}

void CPlaylistOperations::NotifyAll()
{
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}
