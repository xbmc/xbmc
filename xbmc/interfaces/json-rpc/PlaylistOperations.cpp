/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PlaylistOperations.h"
#include "playlists/PlayList.h"
#include "PlayListPlayer.h"
#include "Util.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "ApplicationMessenger.h"
#include "pictures/GUIWindowSlideShow.h"
#include "pictures/PictureInfoTag.h"

using namespace JSONRPC;
using namespace PLAYLIST;
using namespace std;

JSONRPC_STATUS CPlaylistOperations::GetPlaylists(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  result = CVariant(CVariant::VariantTypeArray);
  CVariant playlist = CVariant(CVariant::VariantTypeObject);
  
  playlist["playlistid"] = PLAYLIST_MUSIC;
  playlist["type"] = "audio";
  result.append(playlist);
  
  playlist["playlistid"] = PLAYLIST_VIDEO;
  playlist["type"] = "video";
  result.append(playlist);
  
  playlist["playlistid"] = PLAYLIST_PICTURE;
  playlist["type"] = "picture";
  result.append(playlist);

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::GetProperties(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    CStdString propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(playlist, propertyName, property)) != OK)
      return ret;

    result[propertyName] = property;
  }

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::GetItems(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CFileItemList list;
  int playlist = GetPlaylist(parameterObject["playlistid"]);

  CGUIWindowSlideShow *slideshow = NULL;
  switch (playlist)
  {
    case PLAYLIST_VIDEO:
    case PLAYLIST_MUSIC:
      CApplicationMessenger::Get().PlayListPlayerGetItems(playlist, list);
      break;

    case PLAYLIST_PICTURE:
      slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (slideshow)
        slideshow->GetSlideShowContents(list);
      break;
  }

  HandleFileItemList("id", true, "items", list, parameterObject, result);

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::Add(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  CFileItemList list;
  CVariant params = parameterObject;

  CGUIWindowSlideShow *slideshow = NULL;
  switch (playlist)
  {
    case PLAYLIST_VIDEO:
    case PLAYLIST_MUSIC:
      if (playlist == PLAYLIST_VIDEO)
        params["item"]["media"] = "video";
      else if (playlist == PLAYLIST_MUSIC)
        params["item"]["media"] = "music";

      if (!FillFileItemList(params["item"], list))
        return InvalidParams;

      CApplicationMessenger::Get().PlayListPlayerAdd(playlist, list);
      
      break;

    case PLAYLIST_PICTURE:
      slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (!slideshow)
        return FailedToExecute;
      
      params["item"]["media"] = "pictures";
      if (!FillFileItemList(params["item"], list))
        return InvalidParams;

      for (int index = 0; index < list.Size(); index++)
      {
        CPictureInfoTag picture = CPictureInfoTag();
        if (!picture.Load(list[index]->GetPath()))
          continue;

        *list[index]->GetPictureInfoTag() = picture;
        slideshow->Add(list[index].get());
      }
      break;
  }
  
  NotifyAll();
  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Insert(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  if (playlist == PLAYLIST_PICTURE)
    return FailedToExecute;

  CFileItemList list;
  CVariant params = parameterObject;
  if (playlist == PLAYLIST_VIDEO)
    params["item"]["media"] = "video";
  else if (playlist == PLAYLIST_MUSIC)
    params["item"]["media"] = "music";
  else
    return FailedToExecute;

  if (!FillFileItemList(params["item"], list))
    return InvalidParams;

  CApplicationMessenger::Get().PlayListPlayerInsert(GetPlaylist(parameterObject["playlistid"]), list, (int)parameterObject["position"].asInteger());

  NotifyAll();
  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Remove(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  if (playlist == PLAYLIST_PICTURE)
    return FailedToExecute;
  
  int position = (int)parameterObject["position"].asInteger();
  if (g_playlistPlayer.GetCurrentPlaylist() == playlist && g_playlistPlayer.GetCurrentSong() == position)
    return InvalidParams;

  CApplicationMessenger::Get().PlayListPlayerRemove(playlist, position);

  NotifyAll();
  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Clear(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  CGUIWindowSlideShow *slideshow = NULL;
  switch (playlist)
  {
    case PLAYLIST_MUSIC:
    case PLAYLIST_VIDEO:
      CApplicationMessenger::Get().PlayListPlayerClear(playlist);
      break;

    case PLAYLIST_PICTURE:
       slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
       if (!slideshow)
         return FailedToExecute;
       CApplicationMessenger::Get().SendAction(CAction(ACTION_STOP), WINDOW_SLIDESHOW);
       slideshow->Reset();
       break;
  }

  NotifyAll();
  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Swap(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  if (playlist == PLAYLIST_PICTURE)
    return FailedToExecute;

  CApplicationMessenger::Get().PlayListPlayerSwap(playlist, (int)parameterObject["position1"].asInteger(), (int)parameterObject["position2"].asInteger());

  NotifyAll();
  return ACK;
}

int CPlaylistOperations::GetPlaylist(const CVariant &playlist)
{
  int playlistid = (int)playlist.asInteger();
  if (playlistid > PLAYLIST_NONE && playlistid <= PLAYLIST_PICTURE)
    return playlistid;

  return PLAYLIST_NONE;
}

void CPlaylistOperations::NotifyAll()
{
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  g_windowManager.SendThreadMessage(msg);
}

JSONRPC_STATUS CPlaylistOperations::GetPropertyValue(int playlist, const CStdString &property, CVariant &result)
{
  if (property.Equals("type"))
  {
    switch (playlist)
    {
      case PLAYLIST_MUSIC:
        result = "audio";
        break;
        
      case PLAYLIST_VIDEO:
        result = "video";
        break;
        
      case PLAYLIST_PICTURE:
        result = "pictures";
        break;

      default:
        result = "unknown";
        break;
    }
  }
  else if (property.Equals("size"))
  {
    CFileItemList list;
    CGUIWindowSlideShow *slideshow = NULL;
    switch (playlist)
    {
      case PLAYLIST_MUSIC:
      case PLAYLIST_VIDEO:
        CApplicationMessenger::Get().PlayListPlayerGetItems(playlist, list);
        result = list.Size();
        break;

      case PLAYLIST_PICTURE:
        slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
        if (slideshow)
          result = slideshow->NumSlides();
        else
          result = 0;
        break;

      default:
        result = 0;
        break;
    }
  }
  else
    return InvalidParams;

  return OK;
}
