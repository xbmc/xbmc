/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "PlayListPlayer.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "GUIUserMessages.h"
#include "ApplicationMessenger.h"
#include "pictures/GUIWindowSlideShow.h"
#include "pictures/PictureInfoTag.h"

using namespace JSONRPC;
using namespace PLAYLIST;
using namespace std;

JSONRPC_STATUS CPlaylistOperations::GetPlaylists(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CPlaylistOperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(playlist, propertyName, property)) != OK)
      return ret;

    result[propertyName] = property;
  }

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::GetItems(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

bool CPlaylistOperations::CheckMediaParameter(int playlist, const CVariant &itemObject)
{
  if (itemObject.isMember("media") && itemObject["media"].asString().compare("files") != 0)
  {
    if (playlist == PLAYLIST_VIDEO && itemObject["media"].asString().compare("video") != 0)
      return false;
    if (playlist == PLAYLIST_MUSIC && itemObject["media"].asString().compare("music") != 0)
      return false;
    if (playlist == PLAYLIST_PICTURE && itemObject["media"].asString().compare("video") != 0 && itemObject["media"].asString().compare("pictures") != 0)
      return false;
  }
  return true;
}

JSONRPC_STATUS CPlaylistOperations::Add(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);

  CGUIWindowSlideShow *slideshow = NULL;
  if (playlist == PLAYLIST_PICTURE)
  {
    slideshow = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (slideshow == NULL)
      return FailedToExecute;
  }

  CFileItemList list;
  if (!HandleItemsParameter(playlist, parameterObject["item"], list))
    return InvalidParams;

  switch (playlist)
  {
    case PLAYLIST_VIDEO:
    case PLAYLIST_MUSIC:
      CApplicationMessenger::Get().PlayListPlayerAdd(playlist, list);
      break;

    case PLAYLIST_PICTURE:
      for (int index = 0; index < list.Size(); index++)
      {
        CPictureInfoTag picture = CPictureInfoTag();
        if (!picture.Load(list[index]->GetPath()))
          continue;

        *list[index]->GetPictureInfoTag() = picture;
        slideshow->Add(list[index].get());
      }
      break;

    default:
      return InvalidParams;
  }
  
  NotifyAll();
  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Insert(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  if (playlist == PLAYLIST_PICTURE)
    return FailedToExecute;

  CFileItemList list;
  if (!HandleItemsParameter(playlist, parameterObject["item"], list))
    return InvalidParams;

  CApplicationMessenger::Get().PlayListPlayerInsert(GetPlaylist(parameterObject["playlistid"]), list, (int)parameterObject["position"].asInteger());

  NotifyAll();
  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Remove(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CPlaylistOperations::Clear(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CPlaylistOperations::Swap(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CPlaylistOperations::GetPropertyValue(int playlist, const std::string &property, CVariant &result)
{
  if (property == "type")
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
  else if (property == "size")
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

bool CPlaylistOperations::HandleItemsParameter(int playlistid, const CVariant &itemParam, CFileItemList &items)
{
  vector<CVariant> vecItems;
  if (itemParam.isArray())
    vecItems.assign(itemParam.begin_array(), itemParam.end_array());
  else
    vecItems.push_back(itemParam);

  bool success = false;
  for (vector<CVariant>::iterator itemIt = vecItems.begin(); itemIt != vecItems.end(); ++itemIt)
  {
    if (!CheckMediaParameter(playlistid, *itemIt))
      continue;

    switch (playlistid)
    {
    case PLAYLIST_VIDEO:
      (*itemIt)["media"] = "video";
      break;
    case PLAYLIST_MUSIC:
      (*itemIt)["media"] = "music";
      break;
    case PLAYLIST_PICTURE:
      (*itemIt)["media"] = "pictures";
      break;
    }

    success |= FillFileItemList(*itemIt, items);
  }

  return success;
}
