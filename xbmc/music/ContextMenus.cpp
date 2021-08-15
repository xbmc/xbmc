/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "tags/MusicInfoTag.h"
#include "utils/QueueAndPlayUtils.h"

#include <utility>

namespace CONTEXTMENU
{

CMusicInfo::CMusicInfo(MediaType mediaType)
  : CStaticContextMenuAction(19033), m_mediaType(std::move(mediaType))
{
}
namespace
{

void DoToastNotification(const CFileItemPtr& pItem, const int idString)
{
  std::string sTitle = pItem->GetMusicInfoTag()->GetTitle();
  if (!sTitle.empty())
  {
    if (pItem->IsAlbum())
      sTitle = g_localizeStrings.Get(558) + ": " + sTitle; // album
    else if (pItem->GetMusicInfoTag()->GetType() == MediaTypeSong)
      sTitle = g_localizeStrings.Get(179) + ": " + sTitle; // song
  }
  else
  {
    sTitle = pItem->GetMusicInfoTag()->GetArtistString();
    if (!sTitle.empty())
      sTitle = g_localizeStrings.Get(557) + ": " + sTitle; // artist
  }
  if (sTitle.empty())
    sTitle = pItem->GetMusicInfoTag()->GetType() + ": " + pItem->GetLabel(); // year, genre etc
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(idString),
                                        sTitle);
}

bool IsContextButtonValid(const CFileItem& itemIn)
{
  // Exclude plugins, scripts and addons
  if (itemIn.IsPlugin() || itemIn.IsScript() || itemIn.IsAddonsPath())
    return false;
  const int winID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  if (winID != WINDOW_MUSIC_NAV && winID != WINDOW_HOME)
    return false;
  XFILE::CMusicDatabaseDirectory dir;
  // Exclude top level nodes - eg can't play 'genres' just a specific genre etc
  XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE Node = dir.GetDirectoryParentType(itemIn.GetPath());
  if (Node == XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_OVERVIEW)
    return false;
  if (itemIn.HasMusicInfoTag() && itemIn.CanQueue() && !itemIn.IsParentFolder())
    return true;
  else if (itemIn.IsPlayList() && itemIn.IsAudio())
    return true;
  else if (!itemIn.m_bIsShareOrDrive && itemIn.m_bIsFolder && !itemIn.IsParentFolder())
    return true;
  return false;
}
} // unnamed namespace

bool CMusicInfo::IsVisible(const CFileItem& item) const
{
  return (item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == m_mediaType) ||
         (m_mediaType == MediaTypeArtist && item.IsVideoDb() &&
          item.HasProperty("artist_musicid")) ||
         (m_mediaType == MediaTypeAlbum && item.IsVideoDb() && item.HasProperty("album_musicid"));
}

bool CMusicInfo::Execute(const CFileItemPtr& item) const
{
  CGUIDialogMusicInfo::ShowFor(item.get());
  return true;
}

std::string CMusicPlay::GetLabel(const CFileItem& itemIn) const
{
  return g_localizeStrings.Get(208); // Play
}

bool CMusicPlay::IsVisible(const CFileItem& itemIn) const
{
  return IsContextButtonValid(itemIn);
}

bool CMusicPlay::Execute(const CFileItemPtr& itemIn) const
{
  CQueueAndPlayUtils queueAndPlayUtils;
  queueAndPlayUtils.PlayItem(itemIn);
  DoToastNotification(itemIn, 208); // play
  return true;
}

std::string CMusicQueue::GetLabel(const CFileItem& itemIn) const
{
  return g_localizeStrings.Get(13347); // queue item
}

bool CMusicQueue::IsVisible(const CFileItem& itemIn) const
{
  return IsContextButtonValid(itemIn);
}

bool CMusicQueue::Execute(const CFileItemPtr& itemIn) const
{
  if (itemIn->IsParentFolder() || itemIn->IsRAR() || itemIn->IsZIP())
    return false;
  CQueueAndPlayUtils queueAndPlayUtils;
  queueAndPlayUtils.QueueItem(itemIn);
  DoToastNotification(itemIn, 38082); // Added to playlist
  return true;
}

std::string CMusicBrowse::GetLabel(const CFileItem& itemIn) const
{
  return g_localizeStrings.Get(37015); // Browse into
}

bool CMusicBrowse::IsVisible(const CFileItem& itemIn) const
{
  return (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_HOME &&
          itemIn.HasMusicInfoTag() && itemIn.m_bIsFolder);
}

bool CMusicBrowse::Execute(const CFileItemPtr& itemIn) const
{
  // Call the window so that we can return directly back to the window we called it from
  const std::string target = StringUtils::Format("ActivateWindow({},{},return)", WINDOW_MUSIC_NAV,
                                                 StringUtils::Paramify(itemIn->GetPath()));
  CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
  message.SetStringParam(target);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
  return true;
}

std::string CMusicPlayNext::GetLabel(const CFileItem& itemIn) const
{
  return g_localizeStrings.Get(10008); // play next
}

bool CMusicPlayNext::IsVisible(const CFileItem& itemIn) const
{
  return IsContextButtonValid(itemIn);
}

bool CMusicPlayNext::Execute(const CFileItemPtr& itemIn) const
{
  if (itemIn->IsParentFolder() || itemIn->IsRAR() || itemIn->IsZIP())
    return false;
  CQueueAndPlayUtils queueAndPlayUtils;
  queueAndPlayUtils.QueueItem(itemIn, true); // true - add item at head of queue so will play next
  DoToastNotification(itemIn, 38083); // Added to playlist to play next
  return true;
}
} //namespace CONTEXTMENU
