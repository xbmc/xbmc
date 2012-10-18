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

#include "DirectoryHistory.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace std;

const CStdString& CDirectoryHistory::CPathHistoryItem::GetPath(bool filter /* = false */) const
{
  if (filter && !m_strFilterPath.empty())
    return m_strFilterPath;

  return m_strPath;
}

CDirectoryHistory::~CDirectoryHistory()
{
  m_vecHistory.clear();
  m_vecPathHistory.clear();
}

void CDirectoryHistory::RemoveSelectedItem(const CStdString& strDirectory)
{
  HistoryMap::iterator iter = m_vecHistory.find(preparePath(strDirectory));
  if (iter != m_vecHistory.end())
    m_vecHistory.erase(iter);
}

void CDirectoryHistory::SetSelectedItem(const CStdString& strSelectedItem, const CStdString& strDirectory)
{
  if (strSelectedItem.empty())
    return;
  
  CStdString strDir = preparePath(strDirectory);
  CStdString strItem = preparePath(strSelectedItem, false);
  
  HistoryMap::iterator iter = m_vecHistory.find(strDir);
  if (iter != m_vecHistory.end())
  {
    iter->second.m_strItem = strItem;
    return;
  }

  CHistoryItem item;
  item.m_strItem = strItem;
  item.m_strDirectory = strDir;
  m_vecHistory[strDir] = item;
}

const CStdString& CDirectoryHistory::GetSelectedItem(const CStdString& strDirectory) const
{
  HistoryMap::const_iterator iter = m_vecHistory.find(preparePath(strDirectory));
  if (iter != m_vecHistory.end())
    return iter->second.m_strItem;

  return StringUtils::EmptyString;
}

void CDirectoryHistory::AddPath(const CStdString& strPath, const CStdString &strFilterPath /* = "" */)
{
  if (!m_vecPathHistory.empty() && m_vecPathHistory.back().m_strPath == strPath)
    return;

  CPathHistoryItem item;
  item.m_strPath = strPath;
  item.m_strFilterPath = strFilterPath;
  m_vecPathHistory.push_back(item);
}

void CDirectoryHistory::AddPathFront(const CStdString& strPath, const CStdString &strFilterPath /* = "" */)
{
  CPathHistoryItem item;
  item.m_strPath = strPath;
  item.m_strFilterPath = strFilterPath;
  m_vecPathHistory.insert(m_vecPathHistory.begin(), item);
}

CStdString CDirectoryHistory::GetParentPath(bool filter /* = false */)
{
  if (m_vecPathHistory.empty())
    return StringUtils::EmptyString;

  return m_vecPathHistory.back().GetPath(filter);
}

CStdString CDirectoryHistory::RemoveParentPath(bool filter /* = false */)
{
  if (m_vecPathHistory.empty())
    return StringUtils::EmptyString;

  CStdString strParent = GetParentPath(filter);
  m_vecPathHistory.pop_back();
  return strParent;
}

void CDirectoryHistory::ClearPathHistory()
{
  m_vecPathHistory.clear();
}

void CDirectoryHistory::DumpPathHistory()
{
  // debug log
  CLog::Log(LOGDEBUG,"Current m_vecPathHistory:");
  for (int i = 0; i < (int)m_vecPathHistory.size(); ++i)
    CLog::Log(LOGDEBUG, "  %02i.[%s; %s]", i, m_vecPathHistory[i].m_strPath.c_str(), m_vecPathHistory[i].m_strFilterPath.c_str());
}

CStdString CDirectoryHistory::preparePath(const CStdString &strDirectory, bool tolower /* = true */)
{
  CStdString strDir = strDirectory;
  if (tolower)
    strDir.ToLower();
  URIUtils::RemoveSlashAtEnd(strDir);

  return strDir;
}
