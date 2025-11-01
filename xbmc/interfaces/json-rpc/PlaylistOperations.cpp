/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlaylistOperations.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "pictures/PictureInfoTag.h"
#include "pictures/SlideShowDelegator.h"
#include "utils/Variant.h"

using namespace JSONRPC;
using namespace KODI;

JSONRPC_STATUS CPlaylistOperations::GetPlaylists(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  result = CVariant(CVariant::VariantTypeArray);
  CVariant playlist = CVariant(CVariant::VariantTypeObject);

  playlist["playlistid"] = static_cast<int>(PLAYLIST::Id::TYPE_MUSIC);
  playlist["type"] = "audio";
  result.append(playlist);

  playlist["playlistid"] = static_cast<int>(PLAYLIST::Id::TYPE_VIDEO);
  playlist["type"] = "video";
  result.append(playlist);

  playlist["playlistid"] = static_cast<int>(PLAYLIST::Id::TYPE_PICTURE);
  playlist["type"] = "picture";
  result.append(playlist);

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(playlistId, propertyName, property)) != OK)
      return ret;

    result[propertyName] = property;
  }

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::GetItems(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CFileItemList list;
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);

  switch (playlistId)
  {
    case PLAYLIST::Id::TYPE_VIDEO:
    case PLAYLIST::Id::TYPE_MUSIC:
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_GET_ITEMS,
                                                 static_cast<int>(playlistId), -1,
                                                 static_cast<void*>(&list));
      break;

    case PLAYLIST::Id::TYPE_PICTURE:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
      slideShow.GetSlideShowContents(list);
      break;
    }
    default:
      break;
  }

  HandleFileItemList("id", true, "items", list, parameterObject, result);

  return OK;
}

bool CPlaylistOperations::CheckMediaParameter(PLAYLIST::Id playlistId, const CVariant& itemObject)
{
  if (itemObject.isMember("media") && itemObject["media"].asString().compare("files") != 0)
  {
    if (playlistId == PLAYLIST::Id::TYPE_VIDEO &&
        itemObject["media"].asString().compare("video") != 0)
      return false;
    if (playlistId == PLAYLIST::Id::TYPE_MUSIC &&
        itemObject["media"].asString().compare("music") != 0)
      return false;
    if (playlistId == PLAYLIST::Id::TYPE_PICTURE &&
        itemObject["media"].asString().compare("video") != 0 &&
        itemObject["media"].asString().compare("pictures") != 0)
      return false;
  }
  return true;
}

JSONRPC_STATUS CPlaylistOperations::Add(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);

  CFileItemList list;
  if (!HandleItemsParameter(playlistId, parameterObject["item"], list))
    return InvalidParams;

  switch (playlistId)
  {
    case PLAYLIST::Id::TYPE_VIDEO:
    case PLAYLIST::Id::TYPE_MUSIC:
    {
      auto tmpList = new CFileItemList();
      tmpList->Copy(list);
      CServiceBroker::GetAppMessenger()->PostMsg(
          TMSG_PLAYLISTPLAYER_ADD, static_cast<int>(playlistId), -1, static_cast<void*>(tmpList));
      break;
    }
    case PLAYLIST::Id::TYPE_PICTURE:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
      for (int index = 0; index < list.Size(); index++)
      {
        CPictureInfoTag picture = CPictureInfoTag();
        if (!picture.Load(list[index]->GetPath()))
          continue;

        *list[index]->GetPictureInfoTag() = picture;
        slideShow.Add(list[index].get());
      }
      break;
    }
    default:
      return InvalidParams;
  }

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Insert(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PLAYLIST::Id::TYPE_PICTURE)
    return FailedToExecute;

  CFileItemList list;
  if (!HandleItemsParameter(playlistId, parameterObject["item"], list))
    return InvalidParams;

  auto tmpList = new CFileItemList();
  tmpList->Copy(list);
  CServiceBroker::GetAppMessenger()->PostMsg(
      TMSG_PLAYLISTPLAYER_INSERT, static_cast<int>(playlistId),
      static_cast<int>(parameterObject["position"].asInteger()), static_cast<void*>(tmpList));

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Remove(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PLAYLIST::Id::TYPE_PICTURE)
    return FailedToExecute;

  int position = (int)parameterObject["position"].asInteger();
  if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == playlistId &&
      CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx() == position)
    return InvalidParams;

  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_REMOVE,
                                             static_cast<int>(playlistId), position);

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Clear(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  switch (playlistId)
  {
    case PLAYLIST::Id::TYPE_MUSIC:
    case PLAYLIST::Id::TYPE_VIDEO:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_CLEAR,
                                                 static_cast<int>(playlistId));
      break;

    case PLAYLIST::Id::TYPE_PICTURE:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
      //! @todo: Stop should be a delegator method to void GUI coupling! Same goes for other player controls.
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_SLIDESHOW, -1,
                                                 static_cast<void*>(new CAction(ACTION_STOP)));
      slideShow.Reset();
      break;
    }
    default:
      break;
  }

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Swap(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PLAYLIST::Id::TYPE_PICTURE)
    return FailedToExecute;

  auto tmpVec = new std::vector<int>();
  tmpVec->push_back(static_cast<int>(parameterObject["position1"].asInteger()));
  tmpVec->push_back(static_cast<int>(parameterObject["position2"].asInteger()));
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_SWAP, static_cast<int>(playlistId),
                                             -1, static_cast<void*>(tmpVec));

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::SetShuffle(const std::string& method,
                                               ITransportLayer* transport,
                                               IClient* client,
                                               const CVariant& parameterObject,
                                               CVariant& result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  bool shuffle = parameterObject["shuffle"].isBoolean() && parameterObject["shuffle"].asBoolean();
  bool unshuffle =
      parameterObject["shuffle"].isBoolean() && !parameterObject["shuffle"].asBoolean();
  bool toggle =
      parameterObject["shuffle"].isString() && parameterObject["shuffle"].asString() == "toggle";

  switch (playlist)
  {
    case PLAYLIST_MUSIC:
    case PLAYLIST_VIDEO:
    {
      if (CServiceBroker::GetPlaylistPlayer().IsShuffled(playlist))
      {
        if (unshuffle || toggle)
          CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_SHUFFLE, playlist, 0);
      }
      else
      {
        if (shuffle || toggle)
          CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_SHUFFLE, playlist, 1);
      }
      break;
    }

    case PLAYLIST_PICTURE:
    {
      CGUIWindowSlideShow* slideshow =
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(
              WINDOW_SLIDESHOW);
      if (!slideshow)
        return FailedToExecute;
      if (slideshow->IsShuffled())
      {
        if (unshuffle || toggle)
          return FailedToExecute;
      }
      else
      {
        if (shuffle || toggle)
          slideshow->Shuffle();
      }
      break;
    }

    default:
      return FailedToExecute;
  }
  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::SetRepeat(const std::string& method,
                                              ITransportLayer* transport,
                                              IClient* client,
                                              const CVariant& parameterObject,
                                              CVariant& result)
{
  int playlist = GetPlaylist(parameterObject["playlistid"]);
  if (playlist == PLAYLIST_PICTURE)
    return FailedToExecute;

  std::string repeat = parameterObject["repeat"].asString();
  REPEAT_STATE state = REPEAT_NONE;
  if (repeat == "cycle")
  {
    REPEAT_STATE statePrev = CServiceBroker::GetPlaylistPlayer().GetRepeat(playlist);
    switch (statePrev)
    {
      case REPEAT_NONE:
        state = REPEAT_ALL;
        break;

      case REPEAT_ALL:
        state = REPEAT_ONE;
        break;

      default:
        state = REPEAT_NONE;
    }
  }
  else if (repeat == "one")
    state = REPEAT_ONE;
  else if (repeat == "all")
    state = REPEAT_ALL;

  CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_REPEAT, playlist, state);

  return ACK;
}

PLAYLIST::Id CPlaylistOperations::GetPlaylist(const CVariant& playlist)
{
  PLAYLIST::Id playlistId =
      PLAYLIST::Id{playlist.asInteger32(static_cast<int>(PLAYLIST::Id::TYPE_NONE))};
  if (playlistId != PLAYLIST::Id::TYPE_NONE)
    return playlistId;

  return PLAYLIST::Id::TYPE_NONE;
}

JSONRPC_STATUS CPlaylistOperations::GetPropertyValue(PLAYLIST::Id playlistId,
                                                     const std::string& property,
                                                     CVariant& result)
{
  if (property == "type")
  {
    switch (playlistId)
    {
      case PLAYLIST::Id::TYPE_MUSIC:
        result = "audio";
        break;

      case PLAYLIST::Id::TYPE_VIDEO:
        result = "video";
        break;

      case PLAYLIST::Id::TYPE_PICTURE:
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
    switch (playlistId)
    {
      case PLAYLIST::Id::TYPE_MUSIC:
      case PLAYLIST::Id::TYPE_VIDEO:
      {
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_GET_ITEMS,
                                                   static_cast<int>(playlistId), -1,
                                                   static_cast<void*>(&list));
        result = list.Size();
        break;
      }
      case PLAYLIST::Id::TYPE_PICTURE:
      {
        CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
        const int numSlides = slideShow.NumSlides();
        if (numSlides < 0)
          result = 0;
        else
          result = numSlides;
        break;
      }
      default:
      {
        result = 0;
        break;
      }
    }
  }
  else if (property == "shuffled")
  {
    switch (playlist)
    {
      case PLAYLIST_MUSIC:
      case PLAYLIST_VIDEO:
      {
        bool shuffled;
        CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_IS_SHUFFLED, playlist, -1,
                                                     static_cast<void*>(&shuffled));
        result = shuffled;
        break;
      }

      case PLAYLIST_PICTURE:
      {
        CGUIWindowSlideShow* slideshow =
            CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(
                WINDOW_SLIDESHOW);
        if (slideshow)
          result = slideshow->IsShuffled();
        else
          result = -1;
        break;
      }

      default:
        result = -1;
    }
  }
  else if (property == "repeat")
  {
    switch (playlist)
    {
      case PLAYLIST_MUSIC:
      case PLAYLIST_VIDEO:
      {
        REPEAT_STATE state;
        CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_GET_REPEAT, playlist, -1,
                                                     static_cast<void*>(&state));
        switch (state)
        {
          case REPEAT_ONE:
            result = "one";
            break;
          case REPEAT_ALL:
            result = "all";
            break;
          default:
            result = "off";
        }
        break;
      }

      case PLAYLIST_PICTURE:
      default:
        result = "off";
    }
  }
  else
    return InvalidParams;

  return OK;
}

bool CPlaylistOperations::HandleItemsParameter(PLAYLIST::Id playlistId,
                                               const CVariant& itemParam,
                                               CFileItemList& items)
{
  std::vector<CVariant> vecItems;
  if (itemParam.isArray())
    vecItems.assign(itemParam.begin_array(), itemParam.end_array());
  else
    vecItems.push_back(itemParam);

  bool success = false;
  for (auto& itemIt : vecItems)
  {
    if (!CheckMediaParameter(playlistId, itemIt))
      continue;

    switch (playlistId)
    {
      case PLAYLIST::Id::TYPE_VIDEO:
        itemIt["media"] = "video";
        break;
      case PLAYLIST::Id::TYPE_MUSIC:
        itemIt["media"] = "music";
        break;
      case PLAYLIST::Id::TYPE_PICTURE:
        itemIt["media"] = "pictures";
        break;
      default:
        break;
    }

    success |= FillFileItemList(itemIt, items);
  }

  return success;
}
