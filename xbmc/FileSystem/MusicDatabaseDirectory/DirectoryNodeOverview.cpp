#include "../../stdafx.h"
#include "DirectoryNodeOverview.h"

using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeOverview::CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_GENRE;
  else if (GetName()=="2")
    return NODE_TYPE_ARTIST;
  else if (GetName()=="3")
    return NODE_TYPE_ALBUM;
  else if (GetName()=="4")
    return NODE_TYPE_SONG;
  else if (GetName()=="5")
    return NODE_TYPE_TOP100;
  else if (GetName()=="6")
    return NODE_TYPE_ALBUM_RECENTLY_ADDED;
  else if (GetName()=="7")
    return NODE_TYPE_ALBUM_RECENTLY_PLAYED;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items)
{
  CStdStringArray vecRoot;
  vecRoot.push_back(g_localizeStrings.Get(135));  // Genres
  vecRoot.push_back(g_localizeStrings.Get(133));  // Artists
  vecRoot.push_back(g_localizeStrings.Get(132));  // Albums
  vecRoot.push_back(g_localizeStrings.Get(134));  // Songs
  vecRoot.push_back(g_localizeStrings.Get(271));  // Top 100
  vecRoot.push_back(g_localizeStrings.Get(359));  // Recently Added Albums
  vecRoot.push_back(g_localizeStrings.Get(517));  // Recently Played Albums

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
