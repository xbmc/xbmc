/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef PROGRAMS_GUIVIEWSTATEPROGRAMS_H_INCLUDED
#define PROGRAMS_GUIVIEWSTATEPROGRAMS_H_INCLUDED
#include "GUIViewStatePrograms.h"
#endif

#ifndef PROGRAMS_FILEITEM_H_INCLUDED
#define PROGRAMS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef PROGRAMS_VIEW_VIEWSTATE_H_INCLUDED
#define PROGRAMS_VIEW_VIEWSTATE_H_INCLUDED
#include "view/ViewState.h"
#endif

#ifndef PROGRAMS_SETTINGS_MEDIASOURCESETTINGS_H_INCLUDED
#define PROGRAMS_SETTINGS_MEDIASOURCESETTINGS_H_INCLUDED
#include "settings/MediaSourceSettings.h"
#endif

#ifndef PROGRAMS_FILESYSTEM_DIRECTORY_H_INCLUDED
#define PROGRAMS_FILESYSTEM_DIRECTORY_H_INCLUDED
#include "filesystem/Directory.h"
#endif

#ifndef PROGRAMS_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#define PROGRAMS_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#include "guilib/LocalizeStrings.h"
#endif

#ifndef PROGRAMS_GUILIB_WINDOWIDS_H_INCLUDED
#define PROGRAMS_GUILIB_WINDOWIDS_H_INCLUDED
#include "guilib/WindowIDs.h"
#endif

#ifndef PROGRAMS_SETTINGS_SETTINGS_H_INCLUDED
#define PROGRAMS_SETTINGS_SETTINGS_H_INCLUDED
#include "settings/Settings.h"
#endif

#ifndef PROGRAMS_VIEW_VIEWSTATESETTINGS_H_INCLUDED
#define PROGRAMS_VIEW_VIEWSTATESETTINGS_H_INCLUDED
#include "view/ViewStateSettings.h"
#endif


using namespace XFILE;

CGUIViewStateWindowPrograms::CGUIViewStateWindowPrograms(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%K", "%I", "%L", ""),  // Titel, Size | Foldername, empty
    CSettings::Get().GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone);

  const CViewState *viewState = CViewStateSettings::Get().Get("programs");
  SetSortMethod(viewState->m_sortDescription);
  SetViewAsControl(viewState->m_viewMode);
  SetSortOrder(viewState->m_sortDescription.sortOrder);

  LoadViewState(items.GetPath(), WINDOW_PROGRAMS);
}

void CGUIViewStateWindowPrograms::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAMS, CViewStateSettings::Get().Get("programs"));
}

CStdString CGUIViewStateWindowPrograms::GetLockType()
{
  return "programs";
}

CStdString CGUIViewStateWindowPrograms::GetExtensions()
{
  return ".xbe|.cut";
}

VECSOURCES& CGUIViewStateWindowPrograms::GetSources()
{
  AddAddonsSource("executable", g_localizeStrings.Get(1043), "DefaultAddonProgram.png");
#if defined(TARGET_ANDROID)
  AddAndroidSource("apps", g_localizeStrings.Get(20244), "DefaultProgram.png");
#endif

  VECSOURCES *programSources = CMediaSourceSettings::Get().GetSources("programs");
  AddOrReplace(*programSources, CGUIViewState::GetSources());
  return *programSources;
}

