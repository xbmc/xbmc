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
#include "MessengerPayload.h"
#include "PlayListPlayer.h"
#include "PlaybackModes.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "pictures/PictureInfoTag.h"
#include "pictures/SlideShowDelegator.h"
#include "playlists/PlayListTypes.h"
#include "utils/Variant.h"

#include <memory>
#include <optional>

using namespace JSONRPC;
using namespace KODI;

namespace
{
const char* ReasonOf(JSONRPC_STATUS status)
{
  switch (status)
  {
    case NotFound:
      return "notfound";
    case Unavailable:
      return "unavailable";
    default:
      return "invalid";
  }
}

CVariant UnresolvedEntry(const CVariant& item, const std::string& reason)
{
  CVariant entry{CVariant::VariantTypeObject};
  entry["item"] = item;
  entry["reason"] = reason;
  return entry;
}

// The error for a call that added nothing. A reference that no longer resolves is NotFound;
// only when every entry was malformed is the request itself at fault.
JSONRPC_STATUS StatusForNothingAdded(const CVariant& unresolved)
{
  for (auto entry = unresolved.begin_array(); entry != unresolved.end_array(); ++entry)
  {
    if ((*entry)["reason"].asString() != "invalid")
      return NotFound;
  }

  return InvalidParams;
}
} // unnamed namespace

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
      CServiceBroker::GetAppMessenger()->SendMsg(
          TMSG_PLAYLISTPLAYER_GET_ITEMS, static_cast<int>(playlistId), -1, LendToMessenger(list));
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
  CVariant unresolved{CVariant::VariantTypeArray};
  HandleItemsParameter(playlistId, parameterObject["item"], list, unresolved);

  int added{0};
  switch (playlistId)
  {
    case PLAYLIST::Id::TYPE_VIDEO:
    case PLAYLIST::Id::TYPE_MUSIC:
    {
      if (list.Size() > 0)
      {
        auto items = std::make_unique<CFileItemList>();
        items->Copy(list);
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_ADD,
                                                   static_cast<int>(playlistId), -1,
                                                   TransferToMessenger(std::move(items)));
      }
      added = list.Size();
      break;
    }
    case PLAYLIST::Id::TYPE_PICTURE:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();
      for (int index = 0; index < list.Size(); index++)
      {
        CPictureInfoTag picture = CPictureInfoTag();
        if (!picture.Load(list[index]->GetPath()))
        {
          // The file resolved but holds no picture, which the item parameter cannot express.
          CVariant item{CVariant::VariantTypeObject};
          item["file"] = list[index]->GetPath();
          unresolved.push_back(UnresolvedEntry(item, "invalid"));
          continue;
        }

        *list[index]->GetPictureInfoTag() = picture;
        slideShow.Add(list[index].get());
        ++added;
      }
      break;
    }
    default:
      return InvalidParams;
  }

  if (added == 0)
    return StatusForNothingAdded(unresolved);

  result["added"] = added;
  result["unresolved"] = unresolved;

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::Insert(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PLAYLIST::Id::TYPE_PICTURE)
    return FailedToExecute;

  CFileItemList list;
  CVariant unresolved{CVariant::VariantTypeArray};
  HandleItemsParameter(playlistId, parameterObject["item"], list, unresolved);

  if (list.Size() == 0)
    return StatusForNothingAdded(unresolved);

  auto items = std::make_unique<CFileItemList>();
  items->Copy(list);
  CServiceBroker::GetAppMessenger()->PostMsg(
      TMSG_PLAYLISTPLAYER_INSERT, static_cast<int>(playlistId),
      static_cast<int>(parameterObject["position"].asInteger()),
      TransferToMessenger(std::move(items)));

  result["added"] = list.Size();
  result["unresolved"] = unresolved;

  return OK;
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

JSONRPC_STATUS CPlaylistOperations::Swap(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PLAYLIST::Id::TYPE_PICTURE)
    return FailedToExecute;

  auto positions = std::make_unique<std::vector<int>>();
  positions->push_back(static_cast<int>(parameterObject["position1"].asInteger()));
  positions->push_back(static_cast<int>(parameterObject["position2"].asInteger()));
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_SWAP, static_cast<int>(playlistId),
                                             -1, TransferToMessenger(std::move(positions)));

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::SetShuffle(const std::string& method,
                                               ITransportLayer* transport,
                                               IClient* client,
                                               const CVariant& parameterObject,
                                               CVariant& result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  const CVariant& shuffle = parameterObject["shuffle"];

  switch (playlistId)
  {
    case PLAYLIST::Id::TYPE_MUSIC:
    case PLAYLIST::Id::TYPE_VIDEO:
      ApplyShuffle(playlistId, shuffle);
      break;

    case PLAYLIST::Id::TYPE_PICTURE:
      if (!CServiceBroker::GetSlideShowDelegator().IsPlaying())
        return FailedToExecute;
      return ShuffleSlideshow(shuffle);

    default:
      return InvalidParams;
  }

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::SetRepeat(const std::string& method,
                                              ITransportLayer* transport,
                                              IClient* client,
                                              const CVariant& parameterObject,
                                              CVariant& result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId != PLAYLIST::Id::TYPE_MUSIC && playlistId != PLAYLIST::Id::TYPE_VIDEO)
    return FailedToExecute;

  ApplyRepeat(playlistId, parameterObject["repeat"]);

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
        CServiceBroker::GetAppMessenger()->SendMsg(
            TMSG_PLAYLISTPLAYER_GET_ITEMS, static_cast<int>(playlistId), -1, LendToMessenger(list));
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
    switch (playlistId)
    {
      case PLAYLIST::Id::TYPE_MUSIC:
      case PLAYLIST::Id::TYPE_VIDEO:
        result = CServiceBroker::GetPlaylistPlayer().IsShuffled(playlistId);
        break;

      case PLAYLIST::Id::TYPE_PICTURE:
        result = CServiceBroker::GetSlideShowDelegator().IsShuffled();
        break;

      default:
        result = false;
        break;
    }
  }
  else if (property == "repeat")
  {
    switch (playlistId)
    {
      case PLAYLIST::Id::TYPE_MUSIC:
      case PLAYLIST::Id::TYPE_VIDEO:
      {
        switch (CServiceBroker::GetPlaylistPlayer().GetRepeat(playlistId))
        {
          case PLAYLIST::RepeatState::ONE:
            result = "one";
            break;

          case PLAYLIST::RepeatState::ALL:
            result = "all";
            break;

          default:
            result = "off";
            break;
        }
        break;
      }

      default:
        result = "off";
        break;
    }
  }
  else
    return InvalidParams;

  return OK;
}

void CPlaylistOperations::HandleItemsParameter(PLAYLIST::Id playlistId,
                                               const CVariant& itemParam,
                                               CFileItemList& items,
                                               CVariant& unresolved)
{
  std::vector<CVariant> vecItems;
  if (itemParam.isArray())
    vecItems.assign(itemParam.begin_array(), itemParam.end_array());
  else
    vecItems.push_back(itemParam);

  for (auto& itemIt : vecItems)
  {
    // Keep the item as the client wrote it; "media" below is added here, not requested.
    const CVariant requested{itemIt};
    bool resolved{false};

    if (CheckMediaParameter(playlistId, itemIt))
    {
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

      // FillFileItemList reports a non-empty list, not whether this item resolved; growth says so.
      const int before{items.Size()};
      FillFileItemList(itemIt, items);
      resolved = items.Size() > before;
    }

    if (!resolved)
    {
      unresolved.push_back(UnresolvedEntry(requested, ReasonOf(DiagnoseUnresolvedItem(requested))));
    }
  }
}
