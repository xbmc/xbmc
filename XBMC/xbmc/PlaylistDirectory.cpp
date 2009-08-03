/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "PlaylistDirectory.h"
#include "Settings.h"
#include "FileSystem/HDDirectory.h"
#include "PlayListFactory.h"
#include "Util.h"

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

