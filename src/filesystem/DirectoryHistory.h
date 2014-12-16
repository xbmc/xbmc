#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <string>
#include <vector>

class CDirectoryHistory
{
public:
  class CHistoryItem
  {
  public:
    CHistoryItem(){};
    virtual ~CHistoryItem(){};
    std::string m_strItem;
    std::string m_strDirectory;
  };

  class CPathHistoryItem
  {
  public:
    CPathHistoryItem() { }
    virtual ~CPathHistoryItem() { }

    const std::string& GetPath(bool filter = false) const;

    std::string m_strPath;
    std::string m_strFilterPath;
  };
  
  CDirectoryHistory() { }
  virtual ~CDirectoryHistory();

  void SetSelectedItem(const std::string& strSelectedItem, const std::string& strDirectory);
  const std::string& GetSelectedItem(const std::string& strDirectory) const;
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
