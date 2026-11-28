/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListsMessageHandler.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "PartyMode.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayLists.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationStackHelper.h"
#include "application/PlayListsGUIListener.h"
#include "filesystem/PluginDirectory.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/MessengerPayload.h"
#include "music/MusicFileItemClassify.h"
#include "playlists/PlayListFileItemClassify.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"

#include <memory>

using namespace KODI;
using namespace KODI::MESSAGING;
using namespace KODI::PLAYLIST;

namespace
{
void WakeScreen()
{
  CServiceBroker::GetAppComponents().GetComponent<CApplicationPowerHandling>()->WakeScreen();
}

//! A message's position: -1 for none.
std::optional<int> PositionFromInt(int position)
{
  return position < 0 ? std::nullopt : std::optional<int>(position);
}
} // namespace

namespace KODI::APPLICATION
{
void PostPlayItems(std::unique_ptr<CFileItemList> items,
                   std::optional<int> start,
                   const std::string& player)
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY_ITEMS, start.value_or(-1), -1,
                                             TransferToMessenger(std::move(items)), player);
}
} // namespace KODI::APPLICATION

CPlayListsMessageHandler::CPlayListsMessageHandler(CApplicationPlayLists& playLists)
  : m_playLists(playLists)
{
}

int CPlayListsMessageHandler::GetMessageMask()
{
  return TMSG_MASK_PLAYLISTPLAYER;
}

void CPlayListsMessageHandler::OnApplicationMessage(ThreadMessage* pMsg)
{
  using Persist = CApplicationPlayLists::Persist;

  switch (pMsg->dwMessage)
  {
    case TMSG_PLAYLISTPLAYER_PLAY:
      if (const std::optional<Type> type = NamedOrPlaying(pMsg->param2); type)
        m_playLists.PlayFrom(*type, PositionFromInt(pMsg->param1));
      break;

    case TMSG_PLAYLISTPLAYER_NEXT:
    {
      const std::optional<Type> type = TypeFromInt(pMsg->param1);
      APPLICATION::NotifyIfNothingThere(type ? m_playLists.PlayNext(*type) : m_playLists.PlayNext(),
                                        APPLICATION::Direction::Forward);
      break;
    }

    case TMSG_PLAYLISTPLAYER_PREV:
    {
      const std::optional<Type> type = TypeFromInt(pMsg->param1);
      APPLICATION::NotifyIfNothingThere(type ? m_playLists.PlayPrevious(*type)
                                             : m_playLists.PlayPrevious(),
                                        APPLICATION::Direction::Back);
      break;
    }

    case TMSG_PLAYLISTPLAYER_SHUFFLE:
      if (const std::optional<Type> type = TypeFromInt(pMsg->param1); type)
      {
        if (pMsg->param2 < 0)
          m_playLists.ToggleShuffle(*type, Persist::Yes);
        else
          m_playLists.SetShuffle(*type, pMsg->param2 > 0, Persist::Yes);
      }
      break;

    case TMSG_PLAYLISTPLAYER_REPEAT:
    {
      const std::optional<Type> type = TypeFromInt(pMsg->param1);
      const std::optional<PLAYLIST::Repeat> repeat = PLAYLIST::RepeatFromName(pMsg->strParam);
      if (type && repeat)
        m_playLists.SetRepeat(*type, *repeat, Persist::Yes);
      else if (type && pMsg->strParam == "cycle")
        m_playLists.CycleRepeat(*type, Persist::Yes);
      break;
    }

    case TMSG_MEDIA_PLAY_ITEM:
      OnPlayItem(pMsg);
      break;

    case TMSG_MEDIA_PLAY_ITEMS:
      OnPlayItems(pMsg);
      break;

    case TMSG_MEDIA_PLAY_PLAYLIST:
    {
      const std::unique_ptr<const APPLICATION::PlayOptions> options{
          static_cast<const APPLICATION::PlayOptions*>(pMsg->lpVoid)};
      if (const std::optional<Type> type = TypeFromInt(pMsg->param1); type)
      {
        WakeScreen();
        m_playLists.PlayFrom(*type, PositionFromInt(pMsg->param2),
                             options ? *options : APPLICATION::PlayOptions{});
        if (const std::optional<PLAYLIST::Repeat> repeat = PLAYLIST::RepeatFromName(pMsg->strParam);
            repeat)
          m_playLists.SetRepeat(*type, *repeat, Persist::No);
      }
      break;
    }

    case TMSG_PLAYLISTPLAYER_PARTYMODE:
      PARTYMODE::Stop();
      if (pMsg->param1 == 0)
        break;
      if (const std::optional<Type> type = TypeFromInt(pMsg->param2); type)
        PARTYMODE::Start(*type);
      else if (!pMsg->strParam.empty())
        PARTYMODE::Start(pMsg->strParam);
      else
        PARTYMODE::Start(PLAYLIST::Audio);
      break;

    default:
      break;
  }
}

std::optional<Type> CPlayListsMessageHandler::NamedOrPlaying(int type) const
{
  if (const std::optional<Type> named = TypeFromInt(type); named)
    return named;
  return m_playLists.GetPlayingType();
}

void CPlayListsMessageHandler::OnPlayItem(ThreadMessage* pMsg)
{
  const std::shared_ptr<CFileItem> item{static_cast<CFileItem*>(pMsg->lpVoid)};
  if (!item)
    return;

  WakeScreen();
  const bool restart = pMsg->param1 == 1;
  // a single item replaces its playlist, unless it is a stack moving to another of its parts
  if (CServiceBroker::GetAppComponents().GetComponent<CApplicationStackHelper>()->IsSeekingParts())
    g_application.PlayFile(*item, "",
                           restart ? CApplication::Reopen::Yes : CApplication::Reopen::No);
  else
    m_playLists.PlayItem(
        std::nullopt, item,
        {.reopen = restart ? CApplication::Reopen::Yes : CApplication::Reopen::No});
}

void CPlayListsMessageHandler::OnPlayItems(ThreadMessage* pMsg)
{
  using Persist = CApplicationPlayLists::Persist;

  const std::unique_ptr<CFileItemList> list{static_cast<CFileItemList*>(pMsg->lpVoid)};
  if (!list || list->IsEmpty())
    return;

  WakeScreen();
  // shuffle picks the entry that starts; repeat applies once it plays. New contents end a party
  // before either is set, as ending it restores the saved settings.
  const auto playWithOptions = [this, &list](Type type, const auto& play)
  {
    if (list->HasProperty("shuffled") && list->GetProperty("shuffled").isBoolean())
      m_playLists.SetShuffle(type, list->GetProperty("shuffled").asBoolean(), Persist::No);
    play();
    if (const std::optional<PLAYLIST::Repeat> repeat =
            PLAYLIST::RepeatFromName(list->GetProperty("repeat").asString());
        repeat)
      m_playLists.SetRepeat(type, *repeat, Persist::No);
  };
  const std::optional<int> position = PositionFromInt(pMsg->param1);

  if (list->Size() > 1)
  {
    const Type type = TypeFor(*list);
    m_playLists.DropFeed();
    playWithOptions(type, [&]
                    { m_playLists.PlayItems(type, *list, position, {.player = pMsg->strParam}); });
    return;
  }

  const std::shared_ptr<CFileItem> item = (*list)[0];
  if (HoldsEntries(*item))
  {
    // which playlist the entries belong on is only known once they have been read
    CFileItemList entries;
    if (!APPLICATION::ExpandAsListed(item, entries) || entries.IsEmpty())
      return;
    const Type type = TypeFor(entries, *item);
    m_playLists.DropFeed();
    playWithOptions(type,
                    [&]
                    {
                      m_playLists.PlayItems(type, entries, position, {.player = pMsg->strParam},
                                            item->GetPath());
                    });
    return;
  }

  // if the item is a plugin we need to resolve the URL to ensure the infotags are filled.
  if (URIUtils::HasPluginPath(*item) && !XFILE::CPluginDirectory::GetResolvedPluginResult(*item))
  {
    m_playLists.ReportFailed(item, PLAYLIST::FailReason::Unresolved);
    return;
  }
  const bool isVideo{VIDEO::IsVideo(*item)};
  const bool isAudio{MUSIC::IsAudio(*item)};
  if (!isAudio && !isVideo)
  {
    g_application.PlayMedia(*item, pMsg->strParam);
    return;
  }
  if ((isVideo && !g_passwordManager.IsVideoUnlocked()) ||
      (isAudio && !g_passwordManager.IsMusicUnlocked()))
  {
    CLog::LogF(LOGERROR, "MasterCode or MediaSource-code is wrong: {} will not be played.",
               item->GetPath());
    m_playLists.ReportFailed(item, PLAYLIST::FailReason::Locked);
    return;
  }
  const Type type = TypeFor(*item);
  playWithOptions(type, [&] { m_playLists.PlayItem(type, item, {.player = pMsg->strParam}); });
}
