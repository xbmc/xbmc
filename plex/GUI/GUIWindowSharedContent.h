#pragma once

/*
 *      Copyright (C) 2011 Plex
 *      http://www.plexapp.com
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

#include "video/windows/GUIWindowVideoBase.h"
#include "PlexTypes.h"

class CGUIWindowSharedContent : public CGUIWindowVideoBase
{
 public:

  CGUIWindowSharedContent()
    : CGUIWindowVideoBase(WINDOW_SHARED_CONTENT, "MySharedContent.xml")
  {
  }
  
  bool OnClick(int iItem)
  {
    if (iItem < 0 || iItem >= (int)m_vecItems->Size()) 
      return true;
    
    CFileItemPtr pItem = m_vecItems->Get(iItem);

    CStdString strWindow;

    switch(pItem->GetPlexDirectoryType())
    {
      case PLEX_DIR_TYPE_ARTIST:
        strWindow = "MyMusicFiles";
        break;
      case PLEX_DIR_TYPE_MOVIE:
      case PLEX_DIR_TYPE_SHOW:
        strWindow = "MyVideoFiles";
        break;
      case PLEX_DIR_TYPE_PHOTOALBUM:
      case PLEX_DIR_TYPE_PHOTO:
        strWindow = "MyPictures";
      default:
        break;
    }
    
    if (strWindow.empty())
      return false;

    CStdString cmd = "XBMC.ActivateWindow(" + strWindow + "," + pItem->GetPath() + ",return)";
    g_application.ExecuteXBMCAction(cmd);

    return true;
  }
};
