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
#include "ServiceBroker.h"
#include "application/ApplicationPlayLists.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementV14.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/MessengerPayload.h"
#include "pictures/PictureInfoTag.h"
#include "pictures/SlideShowDelegator.h"
#include "playlists/PlayList.h"
#include "utils/Variant.h"

using namespace JSONRPC;
using namespace KODI;
using ANNOUNCEMENT::V14::PlaylistId;
using KODI::MESSAGING::TransferToMessenger;

JSONRPC_STATUS CPlaylistOperations::GetPlaylists(const CVariant& parameterObject, CVariant& result)
{
  result = CVariant(CVariant::VariantTypeArray);
  CVariant playlist = CVariant(CVariant::VariantTypeObject);

  playlist["playlistid"] = static_cast<int>(PlaylistId::TYPE_MUSIC);
  playlist["type"] = "audio";
  result.append(playlist);

  playlist["playlistid"] = static_cast<int>(PlaylistId::TYPE_VIDEO);
  playlist["type"] = "video";
  result.append(playlist);

  playlist["playlistid"] = static_cast<int>(PlaylistId::TYPE_PICTURE);
  playlist["type"] = "picture";
  result.append(playlist);

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::GetProperties(const CVariant& parameterObject, CVariant& result)
{
  PlaylistId playlistId = GetPlaylist(parameterObject["playlistid"]);
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

JSONRPC_STATUS CPlaylistOperations::GetItems(const CVariant& parameterObject, CVariant& result)
{
  CFileItemList list;
  PlaylistId playlistId = GetPlaylist(parameterObject["playlistid"]);

  switch (playlistId)
  {
    case PlaylistId::TYPE_VIDEO:
    case PlaylistId::TYPE_MUSIC:
      for (const PLAYLIST::PlayListEntry& entry :
           CServiceBroker::GetPlayLists()
               ->GetPlayList(*ANNOUNCEMENT::V14::PlayListFromId(playlistId))
               .GetEntries())
        list.Add(std::make_shared<CFileItem>(*entry.item));
      break;

    case PlaylistId::TYPE_PICTURE:
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

bool CPlaylistOperations::CheckMediaParameter(PlaylistId playlistId, const CVariant& itemObject)
{
  if (itemObject.isMember("media") && itemObject["media"].asString().compare("files") != 0)
  {
    if (playlistId == PlaylistId::TYPE_VIDEO &&
        itemObject["media"].asString().compare("video") != 0)
      return false;
    if (playlistId == PlaylistId::TYPE_MUSIC &&
        itemObject["media"].asString().compare("music") != 0)
      return false;
    if (playlistId == PlaylistId::TYPE_PICTURE &&
        itemObject["media"].asString().compare("video") != 0 &&
        itemObject["media"].asString().compare("pictures") != 0)
      return false;
  }
  return true;
}

JSONRPC_STATUS CPlaylistOperations::Add(const CVariant& parameterObject, CVariant& result)
{
  PlaylistId playlistId = GetPlaylist(parameterObject["playlistid"]);

  CFileItemList list;
  if (!HandleItemsParameter(playlistId, parameterObject["item"], list))
    return InvalidParams;

  switch (playlistId)
  {
    case PlaylistId::TYPE_VIDEO:
    case PlaylistId::TYPE_MUSIC:
      CServiceBroker::GetPlayLists()->Insert(*ANNOUNCEMENT::V14::PlayListFromId(playlistId), list,
                                             -1);
      break;
    case PlaylistId::TYPE_PICTURE:
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

JSONRPC_STATUS CPlaylistOperations::Insert(const CVariant& parameterObject, CVariant& result)
{
  PlaylistId playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PlaylistId::TYPE_PICTURE)
    return FailedToExecute;

  CFileItemList list;
  if (!HandleItemsParameter(playlistId, parameterObject["item"], list))
    return InvalidParams;

  if (const auto type = ANNOUNCEMENT::V14::PlayListFromId(playlistId); type)
    CServiceBroker::GetPlayLists()->Insert(
        *type, list, static_cast<int>(parameterObject["position"].asInteger()));

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Remove(const CVariant& parameterObject, CVariant& result)
{
  PlaylistId playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PlaylistId::TYPE_PICTURE)
    return FailedToExecute;

  const auto type = ANNOUNCEMENT::V14::PlayListFromId(playlistId);
  if (!type)
    return ACK;

  int position = (int)parameterObject["position"].asInteger();
  const auto playLists = CServiceBroker::GetPlayLists();
  if (playLists->GetPlayingType() == type && playLists->GetPlayingPosition(*type) == position)
    return InvalidParams;

  playLists->Remove(*type, position);

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Clear(const CVariant& parameterObject, CVariant& result)
{
  PlaylistId playlistId = GetPlaylist(parameterObject["playlistid"]);
  switch (playlistId)
  {
    case PlaylistId::TYPE_MUSIC:
    case PlaylistId::TYPE_VIDEO:
      CServiceBroker::GetPlayLists()->Clear(*ANNOUNCEMENT::V14::PlayListFromId(playlistId));
      break;

    case PlaylistId::TYPE_PICTURE:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
      //! @todo: Stop should be a delegator method to void GUI coupling! Same goes for other player controls.
      CServiceBroker::GetAppMessenger()->PostMsg(
          TMSG_GUI_ACTION, WINDOW_SLIDESHOW, -1,
          TransferToMessenger(std::make_unique<CAction>(ACTION_STOP)));
      slideShow.Reset();
      break;
    }
    default:
      break;
  }

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Swap(const CVariant& parameterObject, CVariant& result)
{
  PlaylistId playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PlaylistId::TYPE_PICTURE)
    return FailedToExecute;

  if (const auto type = ANNOUNCEMENT::V14::PlayListFromId(playlistId); type)
    CServiceBroker::GetPlayLists()->Swap(
        *type, static_cast<int>(parameterObject["position1"].asInteger()),
        static_cast<int>(parameterObject["position2"].asInteger()));

  return ACK;
}

PlaylistId CPlaylistOperations::GetPlaylist(const CVariant& playlist)
{
  PlaylistId playlistId = PlaylistId{playlist.asInteger32(static_cast<int>(PlaylistId::TYPE_NONE))};
  if (playlistId != PlaylistId::TYPE_NONE)
    return playlistId;

  return PlaylistId::TYPE_NONE;
}

JSONRPC_STATUS CPlaylistOperations::GetPropertyValue(PlaylistId playlistId,
                                                     const std::string& property,
                                                     CVariant& result)
{
  if (property == "type")
  {
    switch (playlistId)
    {
      case PlaylistId::TYPE_MUSIC:
        result = "audio";
        break;

      case PlaylistId::TYPE_VIDEO:
        result = "video";
        break;

      case PlaylistId::TYPE_PICTURE:
        result = "pictures";
        break;

      default:
        result = "unknown";
        break;
    }
  }
  else if (property == "size")
  {
    switch (playlistId)
    {
      case PlaylistId::TYPE_MUSIC:
      case PlaylistId::TYPE_VIDEO:
      {
        result = CServiceBroker::GetPlayLists()
                     ->GetPlayList(*ANNOUNCEMENT::V14::PlayListFromId(playlistId))
                     .Size();
        break;
      }
      case PlaylistId::TYPE_PICTURE:
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
  else
    return InvalidParams;

  return OK;
}

bool CPlaylistOperations::HandleItemsParameter(PlaylistId playlistId,
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
      case PlaylistId::TYPE_VIDEO:
        itemIt["media"] = "video";
        break;
      case PlaylistId::TYPE_MUSIC:
        itemIt["media"] = "music";
        break;
      case PlaylistId::TYPE_PICTURE:
        itemIt["media"] = "pictures";
        break;
      default:
        break;
    }

    success |= FillFileItemList(itemIt, items);
  }

  return success;
}
