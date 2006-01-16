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
    m_hideParentDirItems = g_guiSettings.GetBool("Pictures.HideParentDirItems");

    AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "%I", "%F", ""));  // Filename, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS("%F", "%I", "%F", "%I"));  // Filename, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS("%F", "%J", "%F", "%J"));  // Filename, Date | Foldername, Date
    SetSortMethod(g_stSettings.m_MyPicturesSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    if (g_guiSettings.GetBool("Pictures.UseAutoSwitching"))
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

bool CGUIViewStateWindowPictures::HandleArchives()
{
  return g_guiSettings.GetBool("Pictures.HandleArchives");
}

CGUIViewStateWindowPrograms::CGUIViewStateWindowPrograms(const CFileItemList& items) : CGUIViewState(items)
{
  if (!items.IsVirtualDirectoryRoot())
    m_hideParentDirItems = g_guiSettings.GetBool("ProgramFiles.HideParentDirItems");

  AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%K", "%I", "%F", ""));  // Titel, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS("%K", "%J", "%F", "%J"));  // Titel, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_PROGRAM_COUNT, 507, LABEL_MASKS("%K", "%C", "%F", ""));  // Titel, Count | Foldername, empty
  SetSortMethod(g_stSettings.m_MyProgramsSortMethod);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  if (g_guiSettings.GetBool("ProgramFiles.UseAutoSwitching"))
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

CGUIViewStateWindowScripts::CGUIViewStateWindowScripts(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 103, LABEL_MASKS("%F", "%I", "%F", ""));  // Filename, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE, 104, LABEL_MASKS("%F", "%J", "%F", "%J"));  // Filename, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_SIZE, 105, LABEL_MASKS("%F", "%I", "%F", "%I"));  // Filename, Size | Foldername, Size
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
