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
#include "playlists/PlayListFactory.h"
#include "Util.h"
#include "GUIUserMessages.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"

using namespace Json;
using namespace JSONRPC;
using namespace PLAYLIST;
using namespace std;

#define PLAYLIST_MEMBER_VIRTUAL "playlist-virtual"
#define PLAYLIST_MEMBER_FILE    "playlist-file"

map<CStdString, CPlayListPtr> CPlaylistOperations::VirtualPlaylists;
CCriticalSection CPlaylistOperations::VirtualCriticalSection;

JSON_STATUS CPlaylistOperations::Create(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isString() || parameterObject.isNull() || parameterObject.isObject()))
    return InvalidParams;

  CStdString file = "";
  CStdString id   = "";

  if (parameterObject.isObject())
  {
    if (parameterObject.isMember(PLAYLIST_MEMBER_FILE) && parameterObject[PLAYLIST_MEMBER_FILE].isString())
      file = parameterObject[PLAYLIST_MEMBER_FILE].asString();

    if (parameterObject.isMember(PLAYLIST_MEMBER_VIRTUAL) && parameterObject[PLAYLIST_MEMBER_VIRTUAL].isString())
      id = parameterObject[PLAYLIST_MEMBER_VIRTUAL].asString();
  }

  CPlayListPtr playlist;

  if (file.size() > 0)
  {
    CPlayListPtr playlist = CPlayListPtr(CPlayListFactory::Create(file));
    if (playlist == NULL || !playlist->Load(file))
      return playlist == NULL ? InvalidParams : InternalError;
  }
  else
    playlist = CPlayListPtr(new CPlayList());

  if (id.size() == 0)
  {
    do
    {
      id = StringUtils::CreateUUID();
    } while (VirtualPlaylists.find(id) != VirtualPlaylists.end());
  }

  CSingleLock lock(VirtualCriticalSection);
  VirtualPlaylists[id] = playlist;
  result[PLAYLIST_MEMBER_VIRTUAL] = id;

  return OK;
}

JSON_STATUS CPlaylistOperations::Destroy(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!parameterObject.isString())
    return InvalidParams;

  CSingleLock lock(VirtualCriticalSection);
  VirtualPlaylists.erase(parameterObject.asString());

  return ACK;
}

JSON_STATUS CPlaylistOperations::GetItems(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);

  if (playlist)
  {
    CFileItemList items;
    for (int i = 0; i < playlist->size(); i++)
      items.Add((*playlist)[i]);

    CStdString name = playlist->GetName();
    if (!name.IsEmpty())
      result["name"] = playlist->GetName();

    HandleFileItemList(NULL, true, "items", items, parameterObject, result);

    return OK;
  }

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::Add(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);
  //parameterObject.removeMember(PLAYLIST_MEMBER_VIRTUAL);

  if (playlist)
  {
    CFileItemList list;
    if (CFileItemHandler::FillFileItemList(parameterObject, list) && list.Size() > 0)
      playlist->Add(list);

    return ACK;
  }

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::Remove(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject["item"].isInt() || parameterObject["item"].isString()))
    return InvalidParams;

  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);

  if (playlist)
  {
    if (parameterObject["item"].isInt())
      playlist->Remove(parameterObject["item"].asInt());
    else if (parameterObject["item"].isString())
      playlist->Remove(parameterObject["item"].asString());
    return ACK;
  }

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::Swap(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!parameterObject["item1"].isInt() && !parameterObject["item2"].isInt())
    return InvalidParams;

  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);

  if (playlist && playlist->Swap(parameterObject["item1"].asInt(), parameterObject["item2"].asInt()))
    return ACK;

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::Clear(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);

  if (playlist)
  {
    playlist->Clear();
    return ACK;
  }

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::Shuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);

  if (playlist)
  {
    playlist->Shuffle();
    return ACK;
  }

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::UnShuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);

  if (playlist)
  {
    playlist->UnShuffle();
    return ACK;
  }

  return InvalidParams;
}

bool CPlaylistOperations::FillFileItemList(const Value &parameterObject, CFileItemList &list)
{
  bool found = false;

  if (parameterObject[PLAYLIST_MEMBER_FILE].isString())
  {
    CStdString file = parameterObject[PLAYLIST_MEMBER_FILE].asString();
    CPlayListPtr playlist = CPlayListPtr(CPlayListFactory::Create(file));
    if (playlist && playlist->Load(file))
    {
      for (int i = 0; i < playlist->size(); i++)
        list.Add((*playlist)[i]);

      found = true;
    }
  }

  CSingleLock lock(VirtualCriticalSection);
  if (parameterObject[PLAYLIST_MEMBER_VIRTUAL].isString())
  {
    CStdString id = parameterObject[PLAYLIST_MEMBER_VIRTUAL].asString();
    CPlayListPtr playlist = VirtualPlaylists[id];
    if (playlist)
    {
      for (int i = 0; i < playlist->size(); i++)
        list.Add((*playlist)[i]);

      found = true;
    }
  }

  return found;
}

CPlayListPtr CPlaylistOperations::GetPlaylist(const Value &parameterObject)
{
  if (parameterObject[PLAYLIST_MEMBER_VIRTUAL].isString())
  {
    CStdString id = parameterObject[PLAYLIST_MEMBER_VIRTUAL].asString();
    return VirtualPlaylists[id];
  }
  else if (parameterObject[PLAYLIST_MEMBER_FILE].isString())
  {
    CStdString file = parameterObject[PLAYLIST_MEMBER_FILE].asString();
    CPlayListPtr playlist = CPlayListPtr(CPlayListFactory::Create(file));
    if (playlist && playlist->Load(file))
      return playlist;
  }

  return CPlayListPtr();
}
