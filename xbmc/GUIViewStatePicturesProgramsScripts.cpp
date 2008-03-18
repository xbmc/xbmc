/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIViewStatePicturesProgramsScripts.h"
#include "GUIBaseContainer.h"

using namespace DIRECTORY;

CGUIViewStateWindowPictures::CGUIViewStateWindowPictures(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS());
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 564, LABEL_MASKS());
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
    AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

    SetSortMethod((SORT_METHOD)g_guiSettings.GetInt("pictures.sortmethod"));
    SetViewAsControl(g_guiSettings.GetInt("pictures.viewmode"));
    SetSortOrder((SORT_ORDER)g_guiSettings.GetInt("pictures.sortorder"));
  }
  LoadViewState(items.m_strPath, WINDOW_PICTURES);
}

void CGUIViewStateWindowPictures::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_PICTURES);
}

CStdString CGUIViewStateWindowPictures::GetLockType()
{
  return "pictures";
}

bool CGUIViewStateWindowPictures::UnrollArchives()
{
  return g_guiSettings.GetBool("filelists.unrollarchives");
}

CStdString CGUIViewStateWindowPictures::GetExtensions()
{
  return g_stSettings.m_pictureExtensions;
}

VECSHARES& CGUIViewStateWindowPictures::GetShares()
{
  return g_settings.m_pictureSources;
}

CGUIViewStateWindowPrograms::CGUIViewStateWindowPrograms(const CFileItemList& items) : CGUIViewState(items)
{
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%K", "%J", "%L", "%J"));  // Titel, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_PROGRAM_COUNT, 565, LABEL_MASKS("%K", "%C", "%L", ""));  // Titel, Count | Foldername, empty
  AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%K", "%I", "%K", "%I"));  // Filename, Size | Foldername, Size
  AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty

  SetSortMethod((SORT_METHOD)g_guiSettings.GetInt("programfiles.sortmethod"));
  SetViewAsControl(g_guiSettings.GetInt("programfiles.viewmode"));
  SetSortOrder((SORT_ORDER)g_guiSettings.GetInt("programfiles.sortorder"));

  LoadViewState(items.m_strPath, WINDOW_PROGRAMS);
}

void CGUIViewStateWindowPrograms::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_PROGRAMS);
}

CStdString CGUIViewStateWindowPrograms::GetLockType()
{
  return "programs";
}

CStdString CGUIViewStateWindowPrograms::GetExtensions()
{
  return ".xbe|.cut";
}

VECSHARES& CGUIViewStateWindowPrograms::GetShares()
{
  return g_settings.m_programSources;
}

CGUIViewStateWindowScripts::CGUIViewStateWindowScripts(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
  AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty
  SetSortMethod(SORT_METHOD_LABEL);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_ASC);
  LoadViewState(items.m_strPath, WINDOW_SCRIPTS);
}

void CGUIViewStateWindowScripts::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_SCRIPTS);
}

CStdString CGUIViewStateWindowScripts::GetExtensions()
{
  return ".py";
}

VECSHARES& CGUIViewStateWindowScripts::GetShares()
{
  m_shares.clear();

  CShare share;
  if (g_settings.m_vecProfiles.size() > 1)
  {
    if (CDirectory::Exists("P:\\scripts"))
    {
      CShare share2;
      share2.strName = "Profile Scripts";
      share2.strPath = "P:\\scripts";
      share2.m_iDriveType = SHARE_TYPE_LOCAL;
      m_shares.push_back(share2);
    }
    share.strName = "Shared Scripts";
  }
  else
    share.strName = "Scripts";

  share.strPath = "Q:\\scripts";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);

  return CGUIViewState::GetShares();
}


CGUIViewStateWindowGameSaves::CGUIViewStateWindowGameSaves(const CFileItemList& items) : CGUIViewState(items)
{
  //
  ///////////////////////////////
  /// NOTE:  GAME ID is saved to %T  (aka TITLE) and t         // Date is %J     %L is Label1
  /////////////
  AddSortMethod(SORT_METHOD_LABEL, 551,  LABEL_MASKS("%L", "%T", "%L", ""));  // Filename, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_TITLE, 560, LABEL_MASKS("%L", "%T", "%L", "%T"));  // Filename, TITLE | Foldername, TITLE
  AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty
  SetSortMethod(SORT_METHOD_LABEL);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_ASC);
  LoadViewState(items.m_strPath, WINDOW_GAMESAVES);
}

void CGUIViewStateWindowGameSaves::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_GAMESAVES);
}


VECSHARES& CGUIViewStateWindowGameSaves::GetShares()
{
  m_shares.clear();
  CShare share;
  share.strName = "Local GameSaves";
  share.strPath = "E:\\udata";
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  m_shares.push_back(share);
  return CGUIViewState::GetShares();
}
