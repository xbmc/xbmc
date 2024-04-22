/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIViewStatePrograms.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/TextureManager.h"
#include "guilib/WindowIDs.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "view/ViewState.h"
#include "view/ViewStateSettings.h"

using namespace XFILE;

CGUIViewStateWindowPrograms::CGUIViewStateWindowPrograms(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SortByLabel, 551, LABEL_MASKS("%K", "%I", "%L", ""),  // Title, Size | Foldername, empty
    CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);

  const CViewState *viewState = CViewStateSettings::GetInstance().Get("programs");
  SetSortMethod(viewState->m_sortDescription);
  SetViewAsControl(viewState->m_viewMode);
  SetSortOrder(viewState->m_sortDescription.sortOrder);

  LoadViewState(items.GetPath(), WINDOW_PROGRAMS);
}

void CGUIViewStateWindowPrograms::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAMS, CViewStateSettings::GetInstance().Get("programs"));
}

std::string CGUIViewStateWindowPrograms::GetLockType()
{
  return "programs";
}

std::string CGUIViewStateWindowPrograms::GetExtensions()
{
  return ".cut";
}

VECSOURCES& CGUIViewStateWindowPrograms::GetSources()
{
#if defined(TARGET_ANDROID)
  {
    CMediaSource source;
    source.strPath = "androidapp://sources/apps/";
    source.strName = g_localizeStrings.Get(20244);
    if (CServiceBroker::GetGUI()->GetTextureManager().HasTexture("DefaultProgram.png"))
      source.m_strThumbnailImage = "DefaultProgram.png";
    source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    source.m_ignore = true;
    m_sources.emplace_back(std::move(source));
  }
#endif

  VECSOURCES *programSources = CMediaSourceSettings::GetInstance().GetSources("programs");
  AddOrReplace(*programSources, CGUIViewState::GetSources());
  return *programSources;
}

