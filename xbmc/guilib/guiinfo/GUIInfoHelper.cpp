/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIInfoHelper.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/ApplicationPlayLists.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/IGUIContainer.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "playlists/PlayList.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#include "windows/GUIMediaWindow.h"

#include <mutex>

namespace KODI::GUILIB::GUIINFO
{

// conditions for window retrieval
static const int WINDOW_CONDITION_HAS_LIST_ITEMS = 1;
static const int WINDOW_CONDITION_IS_MEDIA_WINDOW = 2;

namespace
{

bool CheckWindowCondition(const CGUIWindow* window, int condition)
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

  CGUIWindow* window = windowMgr.GetWindow(contextWindow);
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
  CGUIWindow* window = GetWindow(contextWindow);
  if (!window)
    return nullptr;

  CGUIControl* control = nullptr;
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
  // Add-ons ask for list items from their own threads, and the window and its
  // controls can change under them unless the GUI is locked
  std::unique_lock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  std::shared_ptr<CGUIListItem> item;

  if (containerId == 0 && itemOffset == 0 && !(itemFlags & INFOFLAG_LISTITEM_CONTAINER) &&
      !(itemFlags & INFOFLAG_LISTITEM_ABSOLUTE) && !(itemFlags & INFOFLAG_LISTITEM_POSITION))
    item = GetCurrentListItemFromWindow(contextWindow);

  if (!item)
  {
    CGUIControl* activeContainer = GetActiveContainer(containerId, contextWindow);
    if (activeContainer)
      item = static_cast<IGUIContainer*>(activeContainer)->GetListItem(itemOffset, itemFlags);
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

bool GetFileFallbackLabel(std::string& value, const CFileItem& item, int info)
{
  switch (info)
  {
    case PLAYER_PATH:
    case PLAYER_FILENAME:
    case PLAYER_FILEPATH:
      value = GetFileInfoLabelValueFromPath(info, item.GetPath());
      return true;
    case PLAYER_TITLE:
    case MUSICPLAYER_TITLE:
    case VIDEOPLAYER_TITLE:
      value = item.GetLabel();
      if (value.empty())
        value = CUtil::GetTitleFromPath(item.GetPath());
      return true;
    default:
      return false;
  }
}

bool IsPlayListEntryInfo(const CGUIInfo& info, int first, int last)
{
  return info.GetData1() && ((info.GetInfo() >= first && info.GetInfo() <= last) ||
                             (info.GetInfo() >= PLAYER_OFFSET_POSITION_FIRST &&
                              info.GetInfo() <= PLAYER_OFFSET_POSITION_LAST));
}

std::optional<PlayListEntryLabel> GetPlayListEntry(const CApplicationPlayLists& playLists,
                                                   PLAYLIST::Type type,
                                                   const CGUIInfo& info)
{
  if (info.GetInfo() >= PLAYER_OFFSET_POSITION_FIRST &&
      info.GetInfo() <= PLAYER_OFFSET_POSITION_LAST && playLists.GetPlayingType() != type)
    return std::nullopt;

  const int position =
      info.GetData1() == 1 ? playLists.GetPlayingPosition(type, info.GetData2()) : info.GetData2();
  const PLAYLIST::CPlayList& playList = playLists.GetPlayList(type);
  const PLAYLIST::EntryId entry = playList.GetEntryId(position);
  std::shared_ptr<CFileItem> item = playList.GetItem(entry);
  if (!item)
    return std::nullopt;
  return PlayListEntryLabel{entry, std::move(item)};
}

bool CLookedUpItems::NeedsLookUp(PLAYLIST::EntryId entry,
                                 const std::shared_ptr<const CFileItem>& item) const
{
  std::unique_lock lock(m_section);
  const auto it = m_items.find(entry);
  return it == m_items.end() || it->second.lock() != item;
}

void CLookedUpItems::Add(PLAYLIST::EntryId entry, const std::shared_ptr<const CFileItem>& item)
{
  std::unique_lock lock(m_section);
  std::erase_if(m_items, [](const auto& looked) { return looked.second.expired(); });
  m_items[entry] = item;
}

std::shared_ptr<CFileItem> LookUpOnce(CApplicationPlayLists& playLists,
                                      PLAYLIST::Type type,
                                      const PlayListEntryLabel& found,
                                      CLookedUpItems& lookedUp,
                                      const std::function<void(CFileItem&)>& load)
{
  if (!lookedUp.NeedsLookUp(found.entry, found.item))
    return found.item;

  auto item = std::make_shared<CFileItem>(*found.item);
  load(*item);
  playLists.ReplaceItem(type, found.entry, *item);
  lookedUp.Add(found.entry, playLists.GetPlayList(type).GetItem(found.entry));
  return item;
}

std::string GetPlayListLengthLabel(const CApplicationPlayLists& playLists,
                                   std::optional<PLAYLIST::Type> type)
{
  return std::to_string(type ? playLists.GetPlayList(*type).Size() : 0);
}

std::string GetPlayListPositionLabel(const CApplicationPlayLists& playLists,
                                     std::optional<PLAYLIST::Type> type)
{
  const int position = type ? playLists.GetPlayingDisplayPosition(*type) : -1;
  return position < 0 ? std::string{} : std::to_string(position + 1);
}

} // namespace KODI::GUILIB::GUIINFO
