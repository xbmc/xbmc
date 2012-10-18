#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "utils/StdString.h"

class CDirectoryHistory
{
public:
  class CHistoryItem
  {
  public:
    CHistoryItem(){};
    virtual ~CHistoryItem(){};
    CStdString m_strItem;
    CStdString m_strDirectory;
  };

  class CPathHistoryItem
  {
  public:
    CPathHistoryItem() { }
    virtual ~CPathHistoryItem() { }

    const CStdString& GetPath(bool filter = false) const;

    CStdString m_strPath;
    CStdString m_strFilterPath;
  };
  
  CDirectoryHistory() { }
  virtual ~CDirectoryHistory();

  void SetSelectedItem(const CStdString& strSelectedItem, const CStdString& strDirectory);
  const CStdString& GetSelectedItem(const CStdString& strDirectory) const;
  void RemoveSelectedItem(const CStdString& strDirectory);

  void AddPath(const CStdString& strPath, const CStdString &m_strFilterPath = "");
  void AddPathFront(const CStdString& strPath, const CStdString &m_strFilterPath = "");
  CStdString GetParentPath(bool filter = false);
  CStdString RemoveParentPath(bool filter = false);
  void ClearPathHistory();
  void DumpPathHistory();

private:
  static CStdString preparePath(const CStdString &strDirectory, bool tolower = true);
  
  typedef std::map<CStdString, CHistoryItem> HistoryMap;
  HistoryMap m_vecHistory;
  std::vector<CPathHistoryItem> m_vecPathHistory; ///< History of traversed directories
};
