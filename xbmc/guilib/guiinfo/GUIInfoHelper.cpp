/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoHelper.h"

#include "FileItem.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/IGUIContainer.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "playlists/PlayList.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "windows/GUIMediaWindow.h"

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

// conditions for window retrieval
static const int WINDOW_CONDITION_HAS_LIST_ITEMS  = 1;
static const int WINDOW_CONDITION_IS_MEDIA_WINDOW = 2;

std::string GetPlaylistLabel(int item, PLAYLIST::Id playlistId /* = TYPE_NONE */)
{
  PLAYLIST::CPlayListPlayer& player = CServiceBroker::GetPlaylistPlayer();

  if (playlistId == PLAYLIST::TYPE_NONE)
    playlistId = player.GetCurrentPlaylist();

  switch (item)
  {
    case PLAYLIST_LENGTH:
    {
      return std::to_string(player.GetPlaylist(playlistId).size());
    }
    case PLAYLIST_POSITION:
    {
      int currentSong = player.GetCurrentItemIdx();
      if (currentSong > -1)
        return std::to_string(currentSong + 1);
      break;
    }
    case PLAYLIST_RANDOM:
    {
      if (player.IsShuffled(playlistId))
        return g_localizeStrings.Get(16041); // 16041: On
      else
        return g_localizeStrings.Get(591); // 591: Off
    }
    case PLAYLIST_REPEAT:
    {
      PLAYLIST::RepeatState state = player.GetRepeat(playlistId);
      if (state == PLAYLIST::RepeatState::ONE)
        return g_localizeStrings.Get(592); // 592: One
      else if (state == PLAYLIST::RepeatState::ALL)
        return g_localizeStrings.Get(593); // 593: All
      else
        return g_localizeStrings.Get(594); // 594: Off
    }
  }
  return std::string();
}

namespace
{

bool CheckWindowCondition(CGUIWindow *window, int condition)
{
  // check if it satisfies our condition
  if (!window)
    return false;
  if ((condition & WINDOW_CONDITION_HAS_LIST_ITEMS) && !window->HasListItems())
    return false;
  if ((condition & WINDOW_CONDITION_IS_MEDIA_WINDOW) && !window->IsMediaWindow())
    return false;
  return true;
}

CGUIWindow* GetWindowWithCondition(int contextWindow, int condition)
{
  const CGUIWindowManager& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();

  CGUIWindow *window = windowMgr.GetWindow(contextWindow);
  if (CheckWindowCondition(window, condition))
    return window;

  // try topmost dialog
  window = windowMgr.GetWindow(windowMgr.GetTopmostModalDialog());
  if (CheckWindowCondition(window, condition))
    return window;

  // try active window
  window = windowMgr.GetWindow(windowMgr.GetActiveWindow());
  if (CheckWindowCondition(window, condition))
    return window;

  return nullptr;
}

} // unnamed namespace

CGUIWindow* GetWindow(int contextWindow)
{
  return GetWindowWithCondition(contextWindow, 0);
}

CFileItemPtr GetCurrentListItemFromWindow(int contextWindow)
{
  CGUIWindow* window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
  if (window)
    return window->GetCurrentListItem();

  return CFileItemPtr();
}

CGUIMediaWindow* GetMediaWindow(int contextWindow)
{
  CGUIWindow* window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
  if (window)
    return static_cast<CGUIMediaWindow*>(window);

  return nullptr;
}

CGUIControl* GetActiveContainer(int containerId, int contextWindow)
{
  CGUIWindow *window = GetWindow(contextWindow);
  if (!window)
    return nullptr;

  CGUIControl *control = nullptr;
  if (!containerId) // No container specified, so we lookup the current view container
  {
    if (window->IsMediaWindow())
      containerId = static_cast<CGUIMediaWindow*>(window)->GetViewContainerID();
    else
      control = window->GetFocusedControl();
  }

  if (!control)
    control = window->GetControl(containerId);

  if (control && control->IsContainer())
    return control;

  return nullptr;
}

std::shared_ptr<CGUIListItem> GetCurrentListItem(int contextWindow,
                                                 int containerId /* = 0 */,
                                                 int itemOffset /* = 0 */,
                                                 unsigned int itemFlags /* = 0 */)
{
  std::shared_ptr<CGUIListItem> item;

  if (containerId == 0  &&
      itemOffset == 0 &&
      !(itemFlags & INFOFLAG_LISTITEM_CONTAINER) &&
      !(itemFlags & INFOFLAG_LISTITEM_ABSOLUTE) &&
      !(itemFlags & INFOFLAG_LISTITEM_POSITION))
    item = GetCurrentListItemFromWindow(contextWindow);

  if (!item)
  {
    CGUIControl* activeContainer = GetActiveContainer(containerId, contextWindow);
    if (activeContainer)
      item = static_cast<IGUIContainer *>(activeContainer)->GetListItem(itemOffset, itemFlags);
  }

  return item;
}

std::string GetFileInfoLabelValueFromPath(int info, const std::string& filenameAndPath)
{
  std::string value = filenameAndPath;

  if (info == PLAYER_PATH)
  {
    // do this twice since we want the path outside the archive if this is to be of use.
    if (URIUtils::IsInArchive(value))
      value = URIUtils::GetParentPath(value);

    value = URIUtils::GetParentPath(value);
  }
  else if (info == PLAYER_FILENAME)
  {
    value = URIUtils::GetFileName(value);
  }

  return value;
}

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
