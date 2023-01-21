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
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "music/MusicUtils.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "tags/MusicInfoTag.h"

#include <utility>

using namespace CONTEXTMENU;

CMusicInfo::CMusicInfo(MediaType mediaType)
  : CStaticContextMenuAction(19033), m_mediaType(std::move(mediaType))
{
}

bool CMusicInfo::IsVisible(const CFileItem& item) const
{
  return (item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == m_mediaType) ||
         (m_mediaType == MediaTypeArtist && item.IsVideoDb() && item.HasProperty("artist_musicid")) ||
         (m_mediaType == MediaTypeAlbum && item.IsVideoDb() && item.HasProperty("album_musicid"));
}

bool CMusicInfo::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CGUIDialogMusicInfo::ShowFor(item.get());
  return true;
}

bool CMusicBrowse::IsVisible(const CFileItem& item) const
{
  if (item.IsFileFolder(EFILEFOLDER_MASK_ONBROWSE))
    return false; // handled by CMediaWindow

  return item.m_bIsFolder && MUSIC_UTILS::IsItemPlayable(item);
}

bool CMusicBrowse::Execute(const std::shared_ptr<CFileItem>& item) const
{
  auto& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
  if (windowMgr.GetActiveWindow() == WINDOW_MUSIC_NAV)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, WINDOW_MUSIC_NAV, 0, GUI_MSG_UPDATE);
    msg.SetStringParam(item->GetPath());
    windowMgr.SendMessage(msg);
  }
  else
  {
    windowMgr.ActivateWindow(WINDOW_MUSIC_NAV, {item->GetPath(), "return"});
  }
  return true;
}

bool CMusicPlay::IsVisible(const CFileItem& item) const
{
  return MUSIC_UTILS::IsItemPlayable(item);
}

bool CMusicPlay::Execute(const std::shared_ptr<CFileItem>& item) const
{
  MUSIC_UTILS::PlayItem(item);
  return true;
}

bool CMusicPlayNext::IsVisible(const CFileItem& item) const
{
  if (!item.CanQueue())
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
  if (!item.CanQueue())
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
