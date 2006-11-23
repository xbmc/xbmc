#include "stdafx.h"
#include "GUIViewStatePicturesProgramsScripts.h"
#include "AutoSwitch.h"

CGUIViewStateWindowPictures::CGUIViewStateWindowPictures(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS());
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 498, LABEL_MASKS());
    SetSortMethod(g_stSettings.m_MyPicturesRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyPicturesRootViewMethod);

    SetSortOrder(g_stSettings.m_MyPicturesRootSortOrder);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
    SetSortMethod(g_stSettings.m_MyPicturesSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    if (g_guiSettings.GetBool("pictures.useautoswitching"))
    {
      SetViewAsControl(CAutoSwitch::GetView(items));
    }
    else
    {
      SetViewAsControl(g_stSettings.m_MyPicturesViewMethod);
    }

    SetSortOrder(g_stSettings.m_MyPicturesSortOrder);
  }
}

void CGUIViewStateWindowPictures::SaveViewState()
{
  if (m_items.IsVirtualDirectoryRoot())
  {
    g_stSettings.m_MyPicturesRootSortMethod=GetSortMethod();
    g_stSettings.m_MyPicturesRootViewMethod=GetViewAsControl();
    g_stSettings.m_MyPicturesRootSortOrder=GetSortOrder();
  }
  else
  {
    g_stSettings.m_MyPicturesSortMethod=GetSortMethod();
    g_stSettings.m_MyPicturesViewMethod=GetViewAsControl();
    g_stSettings.m_MyPicturesSortOrder=GetSortOrder();
  }
  g_settings.Save();
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
  return g_settings.m_vecMyPictureShares;
}

CGUIViewStateWindowPrograms::CGUIViewStateWindowPrograms(const CFileItemList& items) : CGUIViewState(items)
{
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 103, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS("%K", "%J", "%L", "%J"));  // Titel, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_PROGRAM_COUNT, 507, LABEL_MASKS("%K", "%C", "%L", ""));  // Titel, Count | Foldername, empty
  AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS("%K", "%I", "%K", "%I"));  // Filename, Size | Foldername, Size
  SetSortMethod(g_stSettings.m_MyProgramsSortMethod);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  if (g_guiSettings.GetBool("programfiles.useautoswitching"))
  {
    SetViewAsControl(CAutoSwitch::GetView(items));
  }
  else
  {
    SetViewAsControl(g_stSettings.m_MyProgramsViewMethod);
  }

  SetSortOrder(g_stSettings.m_MyProgramsSortOrder);
}

void CGUIViewStateWindowPrograms::SaveViewState()
{
  g_stSettings.m_MyProgramsSortMethod=GetSortMethod();
  g_stSettings.m_MyProgramsViewMethod=GetViewAsControl();
  g_stSettings.m_MyProgramsSortOrder=GetSortOrder();
  g_settings.Save();
}

CStdString CGUIViewStateWindowPrograms::GetLockType()
{
  return "myprograms";
}

CStdString CGUIViewStateWindowPrograms::GetExtensions()
{
  return ".xbe|.cut";
}

VECSHARES& CGUIViewStateWindowPrograms::GetShares()
{
  return g_settings.m_vecMyProgramsShares;
}

CGUIViewStateWindowScripts::CGUIViewStateWindowScripts(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Filename, Size | Foldername, Size
  SetSortMethod(g_stSettings.m_ScriptsSortMethod);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  SetViewAsControl(g_stSettings.m_ScriptsViewMethod);

  SetSortOrder(g_stSettings.m_ScriptsSortOrder);
}

void CGUIViewStateWindowScripts::SaveViewState()
{
  g_stSettings.m_ScriptsSortMethod=GetSortMethod();
  g_stSettings.m_ScriptsViewMethod=GetViewAsControl();
  g_stSettings.m_ScriptsSortOrder=GetSortOrder();
  g_settings.Save();
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
  AddSortMethod(SORT_METHOD_LABEL, 103,  LABEL_MASKS("%L", "%T", "%L", ""));  // Filename, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_TITLE, 20316, LABEL_MASKS("%L", "%T", "%L", "%T"));  // Filename, TITLE | Foldername, TITLE
  AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Filename, Date | Foldername, Date
  SetSortMethod(g_stSettings.m_GameSavesSortMethod);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  SetViewAsControl(g_stSettings.m_GameSavesViewMethod);

  SetSortOrder(g_stSettings.m_GameSavesSortOrder);
}

void CGUIViewStateWindowGameSaves::SaveViewState()
{
  g_stSettings.m_GameSavesSortMethod=GetSortMethod();
  g_stSettings.m_GameSavesViewMethod=GetViewAsControl();
  g_stSettings.m_GameSavesSortOrder=GetSortOrder();
  g_settings.Save();
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
