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

/*!
  \brief Represents a list of files
  \sa CFileItemList, CFileItem
  */
class CFileItemList : public CFileItem
{
public:
  enum CACHE_TYPE
  {
    CACHE_NEVER = 0,
    CACHE_IF_SLOW,
    CACHE_ALWAYS
  };

  CFileItemList();
  explicit CFileItemList(const std::string& strPath);
  ~CFileItemList() override;
  void Archive(CArchive& ar) override;
  CFileItemPtr operator[](int iItem);
  const CFileItemPtr operator[](int iItem) const;
  CFileItemPtr operator[](const std::string& strPath);
  const CFileItemPtr operator[](const std::string& strPath) const;
  void Clear();
  void ClearItems();
  void Add(CFileItemPtr item);
  void Add(CFileItem&& item);
  void AddFront(const CFileItemPtr& pItem, int itemPosition);
  void Remove(CFileItem* pItem);
  void Remove(int iItem);
  CFileItemPtr Get(int iItem) const;
  const VECFILEITEMS& GetList() const { return m_items; }
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
   By default we stack all items (files and folders) in a CFileItemList
   \param stackFiles whether to stack all items or just collapse folders (defaults to true)
   \sa StackFiles,StackFolders
   */
  void Stack(bool stackFiles = true);

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
  void SetCacheToDisc(CACHE_TYPE cacheToDisc) { m_cacheToDisc = cacheToDisc; }
  bool CacheToDiscAlways() const { return m_cacheToDisc == CACHE_ALWAYS; }
  bool CacheToDiscIfSlow() const { return m_cacheToDisc == CACHE_IF_SLOW; }
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
  void AddSortMethod(SortDescription sortDescription,
                     int buttonLabel,
                     const LABEL_MASKS& labelMasks);
  bool HasSortDetails() const { return m_sortDetails.size() != 0; }
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
  void SetContent(const std::string& content) { m_content = content; }
  const std::string& GetContent() const { return m_content; }

  void ClearSortState();

  VECFILEITEMS::iterator begin() { return m_items.begin(); }
  VECFILEITEMS::iterator end() { return m_items.end(); }
  VECFILEITEMS::iterator erase(VECFILEITEMS::iterator first, VECFILEITEMS::iterator last);
  VECFILEITEMS::const_iterator begin() const { return m_items.begin(); }
  VECFILEITEMS::const_iterator end() const { return m_items.end(); }
  VECFILEITEMS::const_iterator cbegin() const { return m_items.cbegin(); }
  VECFILEITEMS::const_iterator cend() const { return m_items.cend(); }
  std::reverse_iterator<VECFILEITEMS::const_iterator> rbegin() const { return m_items.rbegin(); }
  std::reverse_iterator<VECFILEITEMS::const_iterator> rend() const { return m_items.rend(); }

private:
  void Sort(FILEITEMLISTCOMPARISONFUNC func);
  void FillSortFields(FILEITEMFILLFUNC func);
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

  VECFILEITEMS m_items;
  MAPFILEITEMS m_map;
  bool m_ignoreURLOptions = false;
  bool m_fastLookup = false;
  SortDescription m_sortDescription;
  bool m_sortIgnoreFolders = false;
  CACHE_TYPE m_cacheToDisc = CACHE_IF_SLOW;
  bool m_replaceListing = false;
  std::string m_content;

  std::vector<GUIViewSortDetails> m_sortDetails;

  mutable CCriticalSection m_lock;
};
