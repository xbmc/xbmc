/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;

namespace KODI::PLAYLIST
{

/*!
 * \brief A playlist file's items, as one of the file formats reads and writes them. The items are
 * only collected: playing them is the business of CPlayList, which takes them from here.
 */
class CPlayListFile
{
public:
  CPlayListFile() = default;
  virtual ~CPlayListFile() = default;
  CPlayListFile(const CPlayListFile&) = delete;
  CPlayListFile& operator=(const CPlayListFile&) = delete;

  virtual bool Load(const std::string& strFileName);
  virtual bool LoadData(std::istream& stream);
  virtual bool LoadData(const std::string& strData);
  virtual void Save(const std::string& strFileName) const {}

  /*!
   * \brief Add a copy of the item, marked playable, and given the file's base path when it has none
   * of its own.
   */
  void Add(const std::shared_ptr<CFileItem>& item);
  void Add(const CFileItemList& items);
  void Add(const CPlayListFile& playlist);
  //! Drop the items and the name.
  void Clear();

  const std::string& GetName() const { return m_strPlayListName; }
  //! The folder relative paths in the file are read against.
  const std::string& GetBasePath() const { return m_strBasePath; }

  int Size() const { return static_cast<int>(m_items.size()); }
  bool IsEmpty() const { return m_items.empty(); }
  std::shared_ptr<CFileItem> operator[](int iItem) const;
  const std::vector<std::shared_ptr<CFileItem>>& GetItems() const { return m_items; }
  //! Append the items to the list, in order.
  void GetItems(CFileItemList& items) const;

protected:
  std::string m_strPlayListName;
  std::string m_strBasePath;
  std::vector<std::shared_ptr<CFileItem>> m_items;
};

} // namespace KODI::PLAYLIST
