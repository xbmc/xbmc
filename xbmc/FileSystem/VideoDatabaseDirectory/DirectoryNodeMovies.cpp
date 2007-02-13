#include "../../stdafx.h"
#include "DirectoryNodeMovies.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeMovies::CDirectoryNodeMovies(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MOVIES, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMovies::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_GENRE;
  else if (GetName()=="2")
    return NODE_TYPE_TITLE;
  else if (GetName()=="3")
    return NODE_TYPE_YEAR;
  else if (GetName()=="4")
    return NODE_TYPE_ACTOR;
  else if (GetName()=="5")
    return NODE_TYPE_DIRECTOR;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeMovies::GetContent(CFileItemList& items)
{
  CStdStringArray vecRoot;
  vecRoot.push_back(g_localizeStrings.Get(135));  // Genres
  vecRoot.push_back(g_localizeStrings.Get(369));  // Title
  vecRoot.push_back(g_localizeStrings.Get(345));  // Year
  vecRoot.push_back(g_localizeStrings.Get(344));  // Actors
  vecRoot.push_back(g_localizeStrings.Get(20348));  // Directors

  for (int i = 0; i < (int)vecRoot.size(); ++i)
  {
    CFileItem* pItem = new CFileItem(vecRoot[i]);
    CStdString strDir;
    strDir.Format("%i/", i+1);
    pItem->m_strPath = BuildPath() + strDir;
    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
