
#include "stdafx.h"
#include "playlistdirectory.h"
#include "settings.h"
#include "filesystem/HDdirectory.h"
#include "PlayListFactory.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace DIRECTORY;
using namespace PLAYLIST;

CPlayListDirectory::CPlayListDirectory(void)
{}

CPlayListDirectory::~CPlayListDirectory(void)
{}



bool CPlayListDirectory::GetDirectory(const CStdString& strPath, VECFILEITEMS &items)
{
  if ( !CUtil::IsPlayList(strPath) )
  {
    CHDDirectory dirLoader;
    dirLoader.SetMask(".m3u|.b4s|.pls|.strm");
    VECFILEITEMS tmpitems;
    CStdString strDir = g_stSettings.m_szAlbumDirectory;
    strDir += "\\playlists";
    dirLoader.GetDirectory(strDir, tmpitems);


    // for each playlist found
    for (int i = 0; i < (int)tmpitems.size(); ++i)
    {
      CFileItem* pItem = tmpitems[i];
      CStdString strPlayListName, strPlayList;
      strPlayList = CUtil::GetFileName( pItem->m_strPath );
      strPlayListName = CUtil::GetFileName( strPlayList );
      delete pItem;

      CPlayListFactory factory;
      CPlayList* pPlayList = factory.Create(strPlayList);
      if (pPlayList)
      {
        if ( pPlayList->Load(strPlayList) )
        {
          CStdString strPlayListName = pPlayList->GetName();
          if (strPlayListName.size())
          {
            strPlayListName = strPlayListName;
          }
        }
        delete pPlayList;
      }

      //  create an entry....
      pItem = new CFileItem(strPlayListName);
      pItem->m_strPath = strPlayList;
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;

      items.push_back(pItem);
    }
    CUtil::SetThumbs(items);
    CUtil::FillInDefaultIcons(items);

    return true;
  }

  // yes, first add parent path
  {
    CFileItem *pItem = new CFileItem("..");
    pItem->m_strPath = "";
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    items.push_back(pItem);
  }

  // open the playlist
  CPlayListFactory factory;
  CPlayList* pPlayList = factory.Create(strPath);
  if (!pPlayList) return false;

  if ( pPlayList->Load(strPath) )
  {

    for (int i = 0; i < (int)pPlayList->size(); ++i)
    {
      const CPlayList::CPlayListItem& playlistItem = (*pPlayList)[i];

      CStdString strLabel;
      strLabel = CUtil::GetFileName( playlistItem.GetFileName() );
      CFileItem* pItem = new CFileItem(strLabel);
      pItem->m_strPath = playlistItem.GetFileName();
      pItem->m_bIsFolder = false;
      pItem->m_bIsShareOrDrive = false;
      items.push_back(pItem);
    }
  }
  delete pPlayList;
  CUtil::SetThumbs(items);
  CUtil::FillInDefaultIcons(items);

  return true;
}

