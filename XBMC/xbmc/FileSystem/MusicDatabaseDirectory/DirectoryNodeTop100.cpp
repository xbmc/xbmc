#include "stdafx.h"
#include "DirectoryNodeTop100.h"

using namespace std;
using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeTop100::CDirectoryNodeTop100(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TOP100, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeTop100::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_SONG_TOP100;
  else if (GetName()=="2")
    return NODE_TYPE_ALBUM_TOP100;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeTop100::GetContent(CFileItemList& items)
{
  vector<CStdString> vecRoot;
  vecRoot.push_back(g_localizeStrings.Get(10504));  // Top 100 Songs
  vecRoot.push_back(g_localizeStrings.Get(10505));  // Top 100 Albums

  for (int i = 0; i < (int)vecRoot.size(); ++i)
  {
    CFileItem* pItem = new CFileItem(vecRoot[i]);
    CStdString strDir;
    strDir.Format("%i/", i+1);
    pItem->m_strPath += BuildPath() + strDir;
    pItem->m_bIsFolder = true;
    items.Add(pItem);
  }

  return true;
}
