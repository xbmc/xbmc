#include "stdafx.h"
#include "DirectoryNodeMoviesOverview.h"
#include "FileItem.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeMoviesOverview::CDirectoryNodeMoviesOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MOVIES_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMoviesOverview::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_GENRE;
  else if (GetName()=="2")
    return NODE_TYPE_TITLE_MOVIES;
  else if (GetName()=="3")
    return NODE_TYPE_YEAR;
  else if (GetName()=="4")
    return NODE_TYPE_ACTOR;
  else if (GetName()=="5")
    return NODE_TYPE_DIRECTOR;
  else if (GetName()=="6")
    return NODE_TYPE_STUDIO;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeMoviesOverview::GetContent(CFileItemList& items)
{
  CStdStringArray vecRoot;
  vecRoot.push_back(g_localizeStrings.Get(135));  // Genres
  vecRoot.push_back(g_localizeStrings.Get(369));  // Title
  vecRoot.push_back(g_localizeStrings.Get(345));  // Year
  vecRoot.push_back(g_localizeStrings.Get(344));  // Actors
  vecRoot.push_back(g_localizeStrings.Get(20348));  // Directors
  vecRoot.push_back(g_localizeStrings.Get(20388));  // Studios

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
