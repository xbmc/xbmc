#include "stdafx.h"
#include "GUIViewStatePicturesProgramsScripts.h"
#include "AutoSwitch.h"

CGUIViewStateWindowPictures::CGUIViewStateWindowPictures(const CFileItemList& items) : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 103);
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 498);
    SetSortMethod(g_stSettings.m_MyPicturesRootSortMethod);

    AddViewAsControl(VIEW_METHOD_LIST, 101);
    AddViewAsControl(VIEW_METHOD_ICONS, 100);
    AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
    SetViewAsControl(g_stSettings.m_MyPicturesRootViewMethod);

    SetSortOrder(g_stSettings.m_MyPicturesRootSortOrder);
  }
  else
  {
    AddSortMethod(SORT_METHOD_LABEL, 103);
    AddSortMethod(SORT_METHOD_SIZE, 105);
    AddSortMethod(SORT_METHOD_DATE, 104);
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

    SetSortOrder(g_stSettings.m_MyVideoSortOrder);
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

CGUIViewStateWindowPrograms::CGUIViewStateWindowPrograms(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 103);
  AddSortMethod(SORT_METHOD_DATE, 105);
  AddSortMethod(SORT_METHOD_PROGRAM_COUNT, 507);
  SetSortMethod(g_stSettings.m_MyProgramsSortMethod);

  AddViewAsControl(VIEW_METHOD_LIST, 101);
  AddViewAsControl(VIEW_METHOD_ICONS, 100);
  AddViewAsControl(VIEW_METHOD_LARGE_ICONS, 417);
  SetViewAsControl(g_stSettings.m_MyProgramsViewMethod);

  SetSortOrder(g_stSettings.m_MyProgramsSortOrder);
}

void CGUIViewStateWindowPrograms::SaveViewState()
{
  g_stSettings.m_MyProgramsSortMethod=GetSortMethod();
  g_stSettings.m_MyProgramsViewMethod=GetViewAsControl();
  g_stSettings.m_MyProgramsSortOrder=GetSortOrder();
  g_settings.Save();
}

CGUIViewStateWindowScripts::CGUIViewStateWindowScripts(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 103);
  AddSortMethod(SORT_METHOD_DATE, 104);
  AddSortMethod(SORT_METHOD_SIZE, 105);
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
