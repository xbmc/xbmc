
#include "../stdafx.h"
#include "DirectoryHistory.h"
#include "../util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CDirectoryHistory::CDirectoryHistory()
{
  m_strNull = "";
}

CDirectoryHistory::~CDirectoryHistory()
{}

void CDirectoryHistory::Remove(const CStdString& strDirectory)
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

void CDirectoryHistory::Set(const CStdString& strSelectedItem, const CStdString& strDirectory)
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

const CStdString& CDirectoryHistory::Get(const CStdString& strDirectory) const
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
