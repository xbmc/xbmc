/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryHistory.h"

#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>

const std::string& CDirectoryHistory::CPathHistoryItem::GetPath(bool filter /* = false */) const
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

void CDirectoryHistory::RemoveSelectedItem(const std::string& strDirectory)
{
  HistoryMap::iterator iter = m_vecHistory.find(preparePath(strDirectory));
  if (iter != m_vecHistory.end())
    m_vecHistory.erase(iter);
}

void CDirectoryHistory::SetSelectedItem(const std::string& strSelectedItem,
                                        const std::string& strDirectory,
                                        const int indexItem)
{
  if (strSelectedItem.empty())
    return;

  std::string strDir = preparePath(strDirectory);
  std::string strItem = preparePath(strSelectedItem, false);

  HistoryMap::iterator iter = m_vecHistory.find(strDir);
  if (iter != m_vecHistory.end())
  {
    iter->second.m_strItem = strItem;
    iter->second.m_indexItem = indexItem;
    return;
  }

  CHistoryItem item;
  item.m_strItem = strItem;
  item.m_indexItem = indexItem;
  m_vecHistory[strDir] = item;
}

const std::string& CDirectoryHistory::GetSelectedItem(const std::string& strDirectory) const
{
  HistoryMap::const_iterator iter = m_vecHistory.find(preparePath(strDirectory));
  if (iter != m_vecHistory.end())
    return iter->second.m_strItem;

  return StringUtils::Empty;
}

int CDirectoryHistory::GetSelectedItemIndex(const std::string& strDirectory) const
{
  HistoryMap::const_iterator iter = m_vecHistory.find(preparePath(strDirectory));
  if (iter != m_vecHistory.end())
    return iter->second.m_indexItem;

  return -1;
}

void CDirectoryHistory::AddPath(const std::string& strPath, const std::string &strFilterPath /* = "" */)
{
  if (!m_vecPathHistory.empty() && m_vecPathHistory.back().m_strPath == strPath)
  {
    if (!strFilterPath.empty())
      m_vecPathHistory.back().m_strFilterPath = strFilterPath;
    return;
  }

  CPathHistoryItem item;
  item.m_strPath = strPath;
  item.m_strFilterPath = strFilterPath;
  m_vecPathHistory.push_back(item);
}

void CDirectoryHistory::AddPathFront(const std::string& strPath, const std::string &strFilterPath /* = "" */)
{
  CPathHistoryItem item;
  item.m_strPath = strPath;
  item.m_strFilterPath = strFilterPath;
  m_vecPathHistory.insert(m_vecPathHistory.begin(), item);
}

std::string CDirectoryHistory::GetParentPath(bool filter /* = false */)
{
  if (m_vecPathHistory.empty())
    return "";

  return m_vecPathHistory.back().GetPath(filter);
}

bool CDirectoryHistory::IsInHistory(const std::string &path) const
{
  std::string slashEnded(path);
  URIUtils::AddSlashAtEnd(slashEnded);
  for (std::vector<CPathHistoryItem>::const_iterator i = m_vecPathHistory.begin(); i != m_vecPathHistory.end(); ++i)
  {
    std::string testPath(i->GetPath());
    URIUtils::AddSlashAtEnd(testPath);
    if (slashEnded == testPath)
      return true;
  }
  return false;
}

std::string CDirectoryHistory::RemoveParentPath(bool filter /* = false */)
{
  if (m_vecPathHistory.empty())
    return "";

  std::string strParent = GetParentPath(filter);
  m_vecPathHistory.pop_back();
  return strParent;
}

void CDirectoryHistory::ClearPathHistory()
{
  m_vecPathHistory.clear();
}

bool CDirectoryHistory::IsMusicSearchUrl(CPathHistoryItem &i)
{
  return StringUtils::StartsWith(i.GetPath(), "musicsearch://");
}

void CDirectoryHistory::ClearSearchHistory()
{
  m_vecPathHistory.erase(remove_if(m_vecPathHistory.begin(), m_vecPathHistory.end(), IsMusicSearchUrl), m_vecPathHistory.end());
}

void CDirectoryHistory::DumpPathHistory()
{
  // debug log
  CLog::Log(LOGDEBUG,"Current m_vecPathHistory:");
  for (int i = 0; i < (int)m_vecPathHistory.size(); ++i)
    CLog::Log(LOGDEBUG, "  {:02}.[{}; {}]", i, m_vecPathHistory[i].m_strPath,
              m_vecPathHistory[i].m_strFilterPath);
}

std::string CDirectoryHistory::preparePath(const std::string &strDirectory, bool tolower /* = true */)
{
  std::string strDir = strDirectory;
  if (tolower)
    StringUtils::ToLower(strDir);

  URIUtils::RemoveSlashAtEnd(strDir);

  return strDir;
}
