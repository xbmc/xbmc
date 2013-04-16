/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "GUIDropPolicy.h"
#include "FileItem.h"
#include "windows/GUIWindowFileManager.h"
#include "filesystem/FavouritesDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "utils/FileOperationJob.h"
#include "GUIInfoManager.h"
#include "playlists/PlayList.h"
#include "PlayListPlayer.h"

IGUIDropPolicy* IGUIDropPolicy::Create(CArchive& ar)
{
  int dummy;
  
  ar >> dummy;
  DropPolicyType type = (DropPolicyType)dummy;
  
  CStdString str;
  switch(type)
  {
    case DPT_NONE:
      return NULL;
  }
  return NULL;
}

bool IGUIDropPolicy::IsDuplicate(const CFileItemPtr& item, const CFileItemList& list)
{
  CFileItemPtr dummy = list.Get(item->GetPath());
  return dummy;
}

CFileItemPtr IGUIDropPolicy::CreateDummy(const CFileItemPtr item) 
{ 
  return CFileItemPtr(new CFileItem(*item)); 
}
