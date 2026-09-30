/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "music/MusicDatabase.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicUtils.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "playlists/PlayListTypes.h"
#include "tags/MusicInfoTag.h"
#include "utils/ItemProperties.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "video/VideoFileItemClassify.h"

#include <utility>

using namespace CONTEXTMENU;
using namespace KODI;

CMusicInfoBase::CMusicInfoBase(MediaType mediaType)
  : CStaticContextMenuAction(19033), m_mediaType(std::move(mediaType))
{
}

bool CMusicInfoBase::IsVisible(const CFileItem& item) const
{
  return (item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == m_mediaType) ||
         (m_mediaType == MediaTypeArtist && VIDEO::IsVideoDb(item) &&
          item.HasProperty(ITEM::PROPERTY::ARTIST_MUSICID)) ||
         (m_mediaType == MediaTypeAlbum && VIDEO::IsVideoDb(item) &&
          item.HasProperty(ITEM::PROPERTY::ALBUM_MUSICID));
}

bool CMusicInfoBase::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CGUIDialogMusicInfo::ShowFor(item.get());
  return true;
}

bool CMusicInfo::IsVisible(const CFileItem& item) const
{
  if (CMusicInfoBase::IsVisible(item))
    return true;

  if (item.IsFolder())
    return false;

  const auto* tag{item.GetMusicInfoTag()};
  return tag && tag->GetType() == MediaTypeNone && !tag->GetTitle().empty() && MUSIC::IsAudio(item);
}

bool CMusicBrowse::IsVisible(const CFileItem& item) const
{
  return ((item.IsFolder() || item.IsFileFolder(FileFolderType::MASK_ONBROWSE)) &&
          MUSIC_UTILS::IsItemPlayable(item));
}

bool CMusicBrowse::Execute(const std::shared_ptr<CFileItem>& item) const
{
  // For file directory browsing, we need item's dyn path, for everything else the path.
  const std::string path{item->IsFileFolder(FileFolderType::MASK_ONBROWSE) ? item->GetDynPath()
                                                                           : item->GetPath()};

  auto& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
  if (windowMgr.GetActiveWindow() == WINDOW_MUSIC_NAV)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, WINDOW_MUSIC_NAV, 0, GUI_MSG_UPDATE);
    msg.SetStringParam(path);
    windowMgr.SendMessage(msg);
  }
  else
  {
    windowMgr.ActivateWindow(WINDOW_MUSIC_NAV, {path, "return"});
  }
  return true;
}

bool CMusicGoToArtist::IsVisible(const CFileItem& item) const
{
  if (!item.HasMusicInfoTag())
    return false;

  const auto& tag = *item.GetMusicInfoTag();
  return tag.GetDatabaseId() > -1 &&
         (tag.GetType() == MediaTypeSong || tag.GetType() == MediaTypeAlbum);
}

namespace
{
int SelectArtist(CMusicDatabase& database, const std::vector<int>& artistIds)
{
  if (artistIds.empty())
    return -1;

  if (artistIds.size() == 1)
    return artistIds[0];

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (!dialog)
    return -1;

  dialog->Reset();
  dialog->SetHeading(40807);

  for (const auto artistId : artistIds)
  {
    CArtist artist;
    if (!database.GetArtist(artistId, artist))
      return -1;

    dialog->Add(artist.strArtist);
  }

  dialog->Open();

  if (!dialog->IsConfirmed())
    return -1;

  const int selected = dialog->GetSelectedItem();
  if (selected < 0 || selected >= static_cast<int>(artistIds.size()))
    return -1;

  return artistIds[selected];
}
} // unnamed namespace

bool CMusicGoToArtist::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CMusicDatabase database;
  if (!database.Open())
    return false;

  const auto& tag = *item->GetMusicInfoTag();
  int idArtist = -1;

  if (tag.GetType() == MediaTypeSong)
  {
    std::vector<int> artistIds;
    if (database.GetArtistsBySong(tag.GetDatabaseId(), artistIds))
      idArtist = SelectArtist(database, artistIds);
  }
  else if (tag.GetType() == MediaTypeAlbum &&
           database.GetArtistsByAlbum(tag.GetDatabaseId(), item.get()))
  {
    const auto artistIds = item->GetProperty("albumartistid");
    if (artistIds.isArray() && !artistIds.empty())
    {
      std::vector<int> ids;
      for (unsigned int i = 0; i < artistIds.size(); ++i)
        ids.push_back(artistIds[i].asInteger());

      idArtist = SelectArtist(database, ids);
    }
  }

  if (idArtist < 0)
    return false;

  const std::string path = "musicdb://artists/" + std::to_string(idArtist) + "/";

  auto& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
  if (windowMgr.GetActiveWindow() == WINDOW_MUSIC_NAV)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, WINDOW_MUSIC_NAV, 0, GUI_MSG_UPDATE);
    msg.SetStringParam(path);
    windowMgr.SendMessage(msg);
  }
  else
  {
    windowMgr.ActivateWindow(WINDOW_MUSIC_NAV, {path, "return"});
  }

  return true;
}

bool CMusicPlay::IsVisible(const CFileItem& item) const
{
  return MUSIC_UTILS::IsItemPlayable(item);
}

namespace
{
void Play(const std::shared_ptr<CFileItem>& item, const std::string& player)
{
  item->SetProperty("playlist_type_hint", static_cast<int>(PLAYLIST::Id::TYPE_MUSIC));

  const ContentUtils::PlayMode mode =
      item->GetProperty(ITEM::PROPERTY::CHECK_AUTOPLAY_NEXT_ITEM).asBoolean()
          ? ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM
          : ContentUtils::PlayMode::PLAY_ONLY_THIS;
  MUSIC_UTILS::PlayItem(item, player, mode);
}

std::vector<std::string> GetPlayers(const CPlayerCoreFactory& playerCoreFactory,
                                    const CFileItem& item)
{
  std::vector<std::string> players;
  playerCoreFactory.GetPlayers(item, players);
  return players;
}

bool CanQueue(const CFileItem& item)
{
  if (!item.CanQueue())
    return false;

  const int windowId = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (windowId == WINDOW_MUSIC_PLAYLIST)
    return false; // Already queued

  return true;
}
} // unnamed namespace

bool CMusicPlay::Execute(const std::shared_ptr<CFileItem>& item) const
{
  Play(item, "");
  return true;
}

bool CMusicPlayUsing::IsVisible(const CFileItem& item) const
{
  const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
  return (GetPlayers(playerCoreFactory, item).size() > 1) && MUSIC_UTILS::IsItemPlayable(item);
}

bool CMusicPlayUsing::Execute(const std::shared_ptr<CFileItem>& item) const
{
  const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
  const std::vector<std::string> players{GetPlayers(playerCoreFactory, *item)};
  const std::string player{playerCoreFactory.SelectPlayerDialog(players)};
  if (!player.empty())
  {
    Play(item, player);
    return true;
  }
  return false;
}

bool CMusicPlayNext::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  return MUSIC_UTILS::IsItemPlayable(item);
}

bool CMusicPlayNext::Execute(const std::shared_ptr<CFileItem>& item) const
{
  MUSIC_UTILS::QueueItem(item, MUSIC_UTILS::QueuePosition::POSITION_BEGIN);
  return true;
}

bool CMusicQueue::IsVisible(const CFileItem& item) const
{
  if (!CanQueue(item))
    return false;

  return MUSIC_UTILS::IsItemPlayable(item);
}

namespace
{
void SelectNextItem(int windowID)
{
  auto& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
  CGUIWindow* window = windowMgr.GetWindow(windowID);
  if (window)
  {
    const int viewContainerID = window->GetViewContainerID();
    if (viewContainerID > 0)
    {
      CGUIMessage msg1(GUI_MSG_ITEM_SELECTED, windowID, viewContainerID);
      windowMgr.SendMessage(msg1, windowID);

      CGUIMessage msg2(GUI_MSG_ITEM_SELECT, windowID, viewContainerID, msg1.GetParam1() + 1);
      windowMgr.SendMessage(msg2, windowID);
    }
  }
}
} // unnamed namespace

bool CMusicQueue::Execute(const std::shared_ptr<CFileItem>& item) const
{
  MUSIC_UTILS::QueueItem(item, MUSIC_UTILS::QueuePosition::POSITION_END);

  // Set selection to next item in active window's view.
  const int windowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  SelectNextItem(windowID);

  return true;
}
