/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "playlists/PlayListTypes.h"
#include "threads/CriticalSection.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

class CApplicationPlayLists;
class CFileItem;

class CGUIListItem;

class CGUIControl;
class CGUIMediaWindow;
class CGUIWindow;

namespace KODI::GUILIB::GUIINFO
{

class CGUIInfo;

CGUIWindow* GetWindow(int contextWindow);
CGUIControl* GetActiveContainer(int containerId, int contextWindow);
CGUIMediaWindow* GetMediaWindow(int contextWindow);
std::shared_ptr<CGUIListItem> GetCurrentListItem(int contextWindow,
                                                 int containerId = 0,
                                                 int itemOffset = 0,
                                                 unsigned int itemFlags = 0);

std::string GetFileInfoLabelValueFromPath(int info, const std::string& filenameAndPath);

/*!
 * \brief Fill value from the item's label or path, for the labels any item answers when its tag
 * has none: the Player path and file name, and the Player, MusicPlayer and VideoPlayer titles.
 * \return false if info is not one of those labels.
 */
bool GetFileFallbackLabel(std::string& value, const CFileItem& item, int info);

/*!
 * \brief Whether info is an offset or position label, naming a playlist entry rather than the
 * item asked about. \p first and \p last are the asking player's own offset range.
 */
bool IsPlayListEntryInfo(const CGUIInfo& info, int first, int last);

struct PlayListEntryLabel
{
  PLAYLIST::EntryId entry;
  std::shared_ptr<CFileItem> item;
};

/*!
 * \brief The entry of this playlist an offset or position label names. With data1 1, data2 counts
 * from the playing entry; otherwise data2 is a position. A Player label names only the playing
 * playlist.
 */
std::optional<PlayListEntryLabel> GetPlayListEntry(const CApplicationPlayLists& playLists,
                                                   PLAYLIST::Type type,
                                                   const CGUIInfo& info);

/*!
 * \brief The playlist items whose details a label has looked up. An entry whose item is replaced,
 * as a rebuilt playlist does, is looked up again; one whose item is gone is forgotten.
 */
class CLookedUpItems
{
public:
  //! Whether this item, the entry's own, still needs its details looked up.
  bool NeedsLookUp(PLAYLIST::EntryId entry, const std::shared_ptr<const CFileItem>& item) const;
  //! The entry's item now carries its details.
  void Add(PLAYLIST::EntryId entry, const std::shared_ptr<const CFileItem>& item);

private:
  //! Labels are read from more than one thread.
  mutable CCriticalSection m_section;
  std::unordered_map<PLAYLIST::EntryId, std::weak_ptr<const CFileItem>> m_items;
};

/*!
 * \brief The entry's item with its details. Labels are asked every frame, so the first time an
 * entry is asked about \p load fills in a copy, which replaces the entry's item on the playlist.
 */
std::shared_ptr<CFileItem> LookUpOnce(CApplicationPlayLists& playLists,
                                      PLAYLIST::Type type,
                                      const PlayListEntryLabel& found,
                                      CLookedUpItems& lookedUp,
                                      const std::function<void(CFileItem&)>& load);

std::string GetPlayListLengthLabel(const CApplicationPlayLists& playLists,
                                   std::optional<PLAYLIST::Type> type);

/*!
 * \return The playing entry's place in play order, counted from 1, or empty while the playlist is
 * not playing.
 */
std::string GetPlayListPositionLabel(const CApplicationPlayLists& playLists,
                                     std::optional<PLAYLIST::Type> type);

} // namespace KODI::GUILIB::GUIINFO
