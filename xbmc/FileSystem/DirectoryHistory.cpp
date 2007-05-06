
#include "stdafx.h"
#include "DirectoryHistory.h"
#include "../Util.h"


CDirectoryHistory::CDirectoryHistory()
{
  m_strNull = "";
}

CDirectoryHistory::~CDirectoryHistory()
{}

void CDirectoryHistory::RemoveSelectedItem(const CStdString& strDirectory)
{
  CStdString strDir = strDirectory;
  strDir.ToLower();
  while (CUtil::HasSlashAtEnd(strDir) )
    strDir = strDir.Left(strDir.size() - 1);

  vector<CHistoryItem>::iterator Iter;
  for (Iter = m_vecHistory.begin();Iter != m_vecHistory.end(); Iter++)
  {
    if ( strDir == Iter->m_strDirectory)
    {
      m_vecHistory.erase(Iter);
      return ;
    }
  }
}

void CDirectoryHistory::SetSelectedItem(const CStdString& strSelectedItem, const CStdString& strDirectory)
{
  if (strSelectedItem.size() == 0) return ;
  // if (strDirectory.size()==0) return;
  CStdString strDir = strDirectory;
  strDir.ToLower();
  while (CUtil::HasSlashAtEnd(strDir) )
  {
    strDir = strDir.Left(strDir.size() - 1);
  }

  CStdString strItem = strSelectedItem;
  while (CUtil::HasSlashAtEnd(strItem) )
  {
    strItem = strItem.Left(strItem.size() - 1);
  }


  for (int i = 0; i < (int)m_vecHistory.size(); ++i)
  {
    CHistoryItem& item = m_vecHistory[i];
    if ( strDir == item.m_strDirectory)
    {
      item.m_strItem = strItem;
      return ;
    }
  }

  CHistoryItem item;
  item.m_strItem = strItem;
  item.m_strDirectory = strDir;
  m_vecHistory.push_back(item);
}

const CStdString& CDirectoryHistory::GetSelectedItem(const CStdString& strDirectory) const
{
  CStdString strDir = strDirectory;
  strDir.ToLower();
  while (CUtil::HasSlashAtEnd(strDir) )
  {
    strDir = strDir.Left(strDir.size() - 1);
  }
  for (int i = 0; i < (int)m_vecHistory.size(); ++i)
  {
    const CHistoryItem& item = m_vecHistory[i];
    if ( strDir == item.m_strDirectory)
    {

      return item.m_strItem;
    }
  }
  return m_strNull;
}

void CDirectoryHistory::AddPath(const CStdString& strPath)
{
  if ((m_vecPathHistory.size() == 0) || m_vecPathHistory.back() != strPath)
  {
    m_vecPathHistory.push_back(strPath);
  }
}

void CDirectoryHistory::AddPathFront(const CStdString& strPath)
{
  m_vecPathHistory.insert(m_vecPathHistory.begin(), strPath);
}

CStdString CDirectoryHistory::GetParentPath()
{
  CStdString strParent;
  if (m_vecPathHistory.size() > 0)
  {
    strParent = m_vecPathHistory.back();
  }

  return strParent;
}

CStdString CDirectoryHistory::RemoveParentPath()
{
  CStdString strParent;
  if (m_vecPathHistory.size() > 0)
  {
    strParent = m_vecPathHistory.back();
    m_vecPathHistory.pop_back();
  }

  return strParent;
}

void CDirectoryHistory::ClearPathHistory()
{
  m_vecPathHistory.clear();
}

void CDirectoryHistory::DumpPathHistory()
{
  // debug log
  CStdString strTemp;
  CLog::Log(LOGDEBUG,"Current m_vecPathHistory:");
  for (int i = 0; i < (int)m_vecPathHistory.size(); ++i)
  {
    strTemp.Format("%02i.[%s]", i, m_vecPathHistory[i]);
    CLog::Log(LOGDEBUG, "  %s", strTemp.c_str());
  }
}
