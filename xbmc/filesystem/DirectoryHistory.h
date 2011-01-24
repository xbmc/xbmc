#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

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
  CDirectoryHistory();
  virtual ~CDirectoryHistory();

  void SetSelectedItem(const CStdString& strSelectedItem, const CStdString& strDirectory);
  const CStdString& GetSelectedItem(const CStdString& strDirectory) const;
  void RemoveSelectedItem(const CStdString& strDirectory);

  void AddPath(const CStdString& strPath);
  void AddPathFront(const CStdString& strPath);
  CStdString GetParentPath();
  CStdString RemoveParentPath();
  void ClearPathHistory();
  void DumpPathHistory();
private:
  std::vector<CHistoryItem> m_vecHistory;
  std::vector<CStdString> m_vecPathHistory; ///< History of traversed directories
  CStdString m_strNull;
};
