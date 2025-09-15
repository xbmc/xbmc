/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 \file FileItemList.h
 \brief
 */

#include "FileItem.h"
#include "threads/CriticalSection.h"

#include <compare>
#include <map>
#include <string>
#include <string_view>
#include <vector>

/*!
  \brief Represents a list of files
  \sa CFileItemList, CFileItem
  */
class CFileItemList : public CFileItem
{
public:
  enum class CacheType
  {
    NEVER = 0,
    IF_SLOW,
    ALWAYS
  };

  enum class StackCandidateType : uint8_t
  {
    FOLDER_CANDIDATE,
    FILE_CANDIDATE
  };

  typedef struct StackCandidate
  {
    StackCandidateType type;
    std::string title;
    std::string volume;
    int64_t size;
    int index; // index in m_items

    auto operator<=>(const StackCandidate&) const = default;
  } StackCandidate;

  typedef struct CountedStackCandidate
  {
    StackCandidateType type;
    std::string title;

    auto operator<=>(const CountedStackCandidate& other) const = default;
  } CountedStackCandidate;

  CFileItemList();
  explicit CFileItemList(const std::string& strPath);
  ~CFileItemList() override;
  void Archive(CArchive& ar) override;
  CFileItemPtr operator[](int iItem) const;
  CFileItemPtr operator[](const std::string& strPath) const;
  void Clear();
  void ClearItems();
  void Add(CFileItemPtr item);
  void Add(CFileItem&& item);
  void AddFront(const CFileItemPtr& pItem, int itemPosition);
  void AddItems(const std::vector<CFileItemPtr>& items);
  void AddItems(std::vector<CFileItemPtr>&& items);
  void Remove(const CFileItem* pItem);
  void Remove(int iItem);
  CFileItemPtr Get(int iItem) const;
  const auto& GetList() const { return m_items; }
  CFileItemPtr Get(const std::string& strPath) const;
  int Size() const;
  bool IsEmpty() const;
  void Append(const CFileItemList& itemlist);
  void Assign(const CFileItemList& itemlist, bool append = false);
  bool Copy(const CFileItemList& item, bool copyItems = true);
  void Reserve(size_t iCount);
  void Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute sortAttributes = SortAttributeNone);
  /* \brief Sorts the items based on the given sorting options

  In contrast to Sort (see above) this does not change the internal
  state by storing the sorting method and order used and therefore
  will always execute the sorting even if the list of items has
  already been sorted with the same options before.
  */
  void Sort(SortDescription sortDescription);
  void Randomize();
  void FillInDefaultIcons();
  int GetFolderCount() const;
  int GetFileCount() const;
  int GetSelectedCount() const;
  int GetObjectCount() const;
  void FilterCueItems();
  void RemoveExtensions();
  void SetIgnoreURLOptions(bool ignoreURLOptions);
  void SetFastLookup(bool fastLookup);
  bool Contains(const std::string& fileName) const;
  bool GetFastLookup() const { return m_fastLookup; }

  /*! \brief stack a CFileItemList
   We stack all items (files and folders) in a CFileItemList
   */
  void Stack();

  SortOrder GetSortOrder() const { return m_sortDescription.sortOrder; }
  SortBy GetSortMethod() const { return m_sortDescription.sortBy; }
  void SetSortOrder(SortOrder sortOrder) { m_sortDescription.sortOrder = sortOrder; }
  void SetSortMethod(SortBy sortBy) { m_sortDescription.sortBy = sortBy; }

  /*! \brief load a CFileItemList out of the cache

   The file list may be cached based on which window we're viewing in, as different
   windows will be listing different portions of the same URL (eg viewing music files
   versus viewing video files)

   \param windowID id of the window that's loading this list (defaults to 0)
   \return true if we loaded from the cache, false otherwise.
   \sa Save,RemoveDiscCache
   */
  bool Load(int windowID = 0);

  /*! \brief save a CFileItemList to the cache

   The file list may be cached based on which window we're viewing in, as different
   windows will be listing different portions of the same URL (eg viewing music files
   versus viewing video files)

   \param windowID id of the window that's saving this list (defaults to 0)
   \return true if successful, false otherwise.
   \sa Load,RemoveDiscCache
   */
  bool Save(int windowID = 0);
  void SetCacheToDisc(CacheType cacheToDisc) { m_cacheToDisc = cacheToDisc; }
  bool CacheToDiscAlways() const { return m_cacheToDisc == CacheType::ALWAYS; }
  bool CacheToDiscIfSlow() const { return m_cacheToDisc == CacheType::IF_SLOW; }
  /*! \brief remove a previously cached CFileItemList from the cache

   The file list may be cached based on which window we're viewing in, as different
   windows will be listing different portions of the same URL (eg viewing music files
   versus viewing video files)

   \param windowID id of the window whose cache we which to remove (defaults to 0)
   \sa Save,Load
   */
  void RemoveDiscCache(int windowID = 0) const;
  void RemoveDiscCache(const std::string& cachefile) const;
  void RemoveDiscCacheCRC(const std::string& crc) const;
  bool AlwaysCache() const;

  void Swap(unsigned int item1, unsigned int item2);

  /*! \brief Update an item in the item list
   \param item the new item, which we match based on path to an existing item in the list
   \return true if the item exists in the list (and was thus updated), false otherwise.
   */
  bool UpdateItem(const CFileItem* item);

  void AddSortMethod(SortBy sortBy,
                     int buttonLabel,
                     const LABEL_MASKS& labelMasks,
                     SortAttribute sortAttributes = SortAttributeNone);
  void AddSortMethod(SortBy sortBy,
                     SortAttribute sortAttributes,
                     int buttonLabel,
                     const LABEL_MASKS& labelMasks);
  void AddSortMethod(const SortDescription& sortDescription,
                     int buttonLabel,
                     const LABEL_MASKS& labelMasks);
  bool HasSortDetails() const { return !m_sortDetails.empty(); }
  const std::vector<GUIViewSortDetails>& GetSortDetails() const { return m_sortDetails; }

  /*! \brief Specify whether this list should be sorted with folders separate from files
   By default we sort with folders listed (and sorted separately) except for those sort modes
   which should be explicitly sorted with folders interleaved with files (eg SORT_METHOD_FILES).
   With this set the folder state will be ignored, allowing folders and files to sort interleaved.
   \param sort whether to ignore the folder state.
   */
  void SetSortIgnoreFolders(bool sort) { m_sortIgnoreFolders = sort; }
  bool GetReplaceListing() const { return m_replaceListing; }
  void SetReplaceListing(bool replace);
  void SetContent(std::string_view content) { m_content = content; }
  const std::string& GetContent() const { return m_content; }

  void ClearSortState();

  auto begin() { return m_items.begin(); }
  auto end() { return m_items.end(); }

  auto begin() const { return m_items.begin(); }
  auto end() const { return m_items.end(); }

  using Iterator = std::vector<std::shared_ptr<CFileItem>>::iterator;
  Iterator erase(Iterator first, Iterator last);

  auto cbegin() const { return m_items.cbegin(); }
  auto cend() const { return m_items.cend(); }

  auto rbegin() const { return m_items.rbegin(); }
  auto rend() const { return m_items.rend(); }

private:
  std::string GetDiscFileCache(int windowID) const;

  /*!
   \brief stack files in a CFileItemList
   \sa Stack
   */
  void StackFiles();

  /*!
   \brief stack folders in a CFileItemList
   \sa Stack
   */
  void StackFolders();

  void AddFastLookupItem(const CFileItemPtr& item);
  void AddFastLookupItems(const std::vector<CFileItemPtr>& items);

  std::vector<std::shared_ptr<CFileItem>> m_items;
  std::map<std::string, std::shared_ptr<CFileItem>, std::less<>> m_map;
  bool m_ignoreURLOptions = false;
  bool m_fastLookup = false;
  SortDescription m_sortDescription;
  bool m_sortIgnoreFolders = false;
  CacheType m_cacheToDisc = CacheType::IF_SLOW;
  bool m_replaceListing = false;
  std::string m_content;

  std::vector<GUIViewSortDetails> m_sortDetails;

  mutable CCriticalSection m_lock;
};
