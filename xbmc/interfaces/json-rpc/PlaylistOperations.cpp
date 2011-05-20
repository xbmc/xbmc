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

using namespace JSONRPC;
using namespace PLAYLIST;
using namespace std;

#define PLAYLIST_MEMBER_VIRTUAL "id"
#define PLAYLIST_MEMBER_FILE    "file"

map<CStdString, CPlayListPtr> CPlaylistOperations::VirtualPlaylists;
CCriticalSection CPlaylistOperations::VirtualCriticalSection;

JSON_STATUS CPlaylistOperations::Create(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString file = "";
  CStdString id   = "";

  if (parameterObject["playlist"].isMember(PLAYLIST_MEMBER_FILE) && parameterObject["playlist"][PLAYLIST_MEMBER_FILE].isString())
      file = parameterObject["playlist"][PLAYLIST_MEMBER_FILE].asString();

  if (parameterObject["playlist"].isMember(PLAYLIST_MEMBER_VIRTUAL) && parameterObject["playlist"][PLAYLIST_MEMBER_VIRTUAL].isString())
    id = parameterObject["playlist"][PLAYLIST_MEMBER_VIRTUAL].asString();

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
  result["playlistid"] = id;

  return OK;
}

JSON_STATUS CPlaylistOperations::Destroy(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CSingleLock lock(VirtualCriticalSection);
  if (VirtualPlaylists.erase(parameterObject["playlistid"].asString()) <= 0)
    return InvalidParams;

  return ACK;
}

JSON_STATUS CPlaylistOperations::GetItems(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

    HandleFileItemList("id", true, "items", items, parameterObject, result);

    return OK;
  }

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::Add(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CPlaylistOperations::Remove(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);

  if (playlist)
  {
    if (parameterObject["item"].isInteger())
      playlist->Remove((int)parameterObject["item"].asInteger());
    else if (parameterObject["item"].isString())
      playlist->Remove(parameterObject["item"].asString());

    return ACK;
  }

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::Swap(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CSingleLock lock(VirtualCriticalSection);
  CPlayListPtr playlist = GetPlaylist(parameterObject);

  if (playlist && playlist->Swap((int)parameterObject["item1"].asInteger(), (int)parameterObject["item2"].asInteger()))
    return ACK;

  return InvalidParams;
}

JSON_STATUS CPlaylistOperations::Clear(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CPlaylistOperations::Shuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSON_STATUS CPlaylistOperations::UnShuffle(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

bool CPlaylistOperations::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  bool found = false;

  if (parameterObject["playlist"].isMember(PLAYLIST_MEMBER_FILE) && parameterObject["playlist"][PLAYLIST_MEMBER_FILE].isString())
  {
    CStdString file = parameterObject["playlist"][PLAYLIST_MEMBER_FILE].asString();
    CPlayListPtr playlist = CPlayListPtr(CPlayListFactory::Create(file));
    if (playlist && playlist->Load(file))
    {
      for (int i = 0; i < playlist->size(); i++)
        list.Add((*playlist)[i]);

      found = true;
    }
  }

  CSingleLock lock(VirtualCriticalSection);
  if (parameterObject["playlist"].isMember(PLAYLIST_MEMBER_VIRTUAL) && parameterObject["playlist"][PLAYLIST_MEMBER_VIRTUAL].isString())
  {
    CStdString id = parameterObject["playlist"][PLAYLIST_MEMBER_VIRTUAL].asString();
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

CPlayListPtr CPlaylistOperations::GetPlaylist(const CVariant &parameterObject)
{
  if (parameterObject["playlist"].isMember(PLAYLIST_MEMBER_VIRTUAL) && parameterObject["playlist"][PLAYLIST_MEMBER_VIRTUAL].isString())
  {
    CStdString id = parameterObject["playlist"][PLAYLIST_MEMBER_VIRTUAL].asString();
    return VirtualPlaylists[id];
  }
  else if (parameterObject["playlist"].isMember(PLAYLIST_MEMBER_FILE) && parameterObject["playlist"][PLAYLIST_MEMBER_FILE].isString())
  {
    CStdString file = parameterObject["playlist"][PLAYLIST_MEMBER_FILE].asString();
    CPlayListPtr playlist = CPlayListPtr(CPlayListFactory::Create(file));
    if (playlist && playlist->Load(file))
      return playlist;
  }

  return CPlayListPtr();
}
