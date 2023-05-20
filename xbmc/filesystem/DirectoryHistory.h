/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

class CDirectoryHistory
{
public:
  class CHistoryItem
  {
  public:
    CHistoryItem() = default;
    virtual ~CHistoryItem() = default;
    std::string m_strItem;
    int m_indexItem{-1};
  };

  class CPathHistoryItem
  {
  public:
    CPathHistoryItem() = default;
    virtual ~CPathHistoryItem() = default;

    const std::string& GetPath(bool filter = false) const;

    std::string m_strPath;
    std::string m_strFilterPath;
  };

  CDirectoryHistory() = default;
  virtual ~CDirectoryHistory();

  /*!
   * \brief Store the currently selected item for the navigation path
   * \param strSelectedItem Selected item
   * \param strDirectory Path
   * \param indexItem Index of the selected item (in list, after filtering/sorting).
   * -1 when the index information is not available.
  */
  void SetSelectedItem(const std::string& strSelectedItem,
                       const std::string& strDirectory,
                       const int indexItem = -1);
  const std::string& GetSelectedItem(const std::string& strDirectory) const;
  int GetSelectedItemIndex(const std::string& strDirectory) const;
  void RemoveSelectedItem(const std::string& strDirectory);

  void AddPath(const std::string& strPath, const std::string &m_strFilterPath = "");
  void AddPathFront(const std::string& strPath, const std::string &m_strFilterPath = "");
  std::string GetParentPath(bool filter = false);
  std::string RemoveParentPath(bool filter = false);
  void ClearPathHistory();
  void ClearSearchHistory();
  void DumpPathHistory();

  /*! \brief Returns whether a path is in the history.
   \param path to test
   \return true if the path is in the history, false otherwise.
   */
  bool IsInHistory(const std::string &path) const;

private:
  static std::string preparePath(const std::string &strDirectory, bool tolower = true);

  typedef std::map<std::string, CHistoryItem> HistoryMap;
  HistoryMap m_vecHistory;
  std::vector<CPathHistoryItem> m_vecPathHistory; ///< History of traversed directories
  static bool IsMusicSearchUrl(CPathHistoryItem &i);
};
