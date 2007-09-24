#include "stdafx.h"
#include "DirectoryNodeTvShowsOverview.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeTvShowsOverview::CDirectoryNodeTvShowsOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TVSHOWS_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeTvShowsOverview::GetChildType()
{
  if (GetName()=="0")
    return NODE_TYPE_EPISODES;
  else if (GetName()=="1")
    return NODE_TYPE_GENRE;
  else if (GetName()=="2")
    return NODE_TYPE_TITLE_TVSHOWS;
  else if (GetName()=="3")
    return NODE_TYPE_YEAR;
  else if (GetName()=="4")
    return NODE_TYPE_ACTOR;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeTvShowsOverview::GetContent(CFileItemList& items)
{
  CStdStringArray vecRoot;
  vecRoot.push_back(g_localizeStrings.Get(135));  // Genres
  vecRoot.push_back(g_localizeStrings.Get(369));  // Title
  vecRoot.push_back(g_localizeStrings.Get(345));  // Year
  vecRoot.push_back(g_localizeStrings.Get(344));  // Actors

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
