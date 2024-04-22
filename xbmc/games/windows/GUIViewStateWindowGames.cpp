/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIViewStateWindowGames.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "games/GameUtils.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/MediaSourceSettings.h"
#include "utils/StringUtils.h"
#include "view/ViewState.h"
#include "view/ViewStateSettings.h"
#include "windowing/GraphicContext.h" // include before ViewState.h

#include <assert.h>
#include <set>

using namespace KODI;
using namespace GAME;

CGUIViewStateWindowGames::CGUIViewStateWindowGames(const CFileItemList& items)
  : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByLabel, 551, LABEL_MASKS());
    AddSortMethod(SortByDriveType, 564, LABEL_MASKS());
    SetSortMethod(SortByLabel);
    SetSortOrder(SortOrderAscending);
    SetViewAsControl(DEFAULT_VIEW_LIST);
  }
  else
  {
    AddSortMethod(SortByFile, 561,
                  LABEL_MASKS("%F", "%I", "%L", "")); // Filename, Size | Label, empty
    AddSortMethod(SortBySize, 553,
                  LABEL_MASKS("%L", "%I", "%L", "%I")); // Filename, Size | Label, Size

    const CViewState* viewState = CViewStateSettings::GetInstance().Get("games");
    if (viewState)
    {
      SetSortMethod(viewState->m_sortDescription);
      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
  }

  LoadViewState(items.GetPath(), WINDOW_GAMES);
}

std::string CGUIViewStateWindowGames::GetLockType()
{
  return "games";
}

std::string CGUIViewStateWindowGames::GetExtensions()
{
  std::set<std::string> exts = CGameUtils::GetGameExtensions();

  // Ensure .zip appears
  exts.insert(".zip");

  return StringUtils::Join(exts, "|");
}

VECSOURCES& CGUIViewStateWindowGames::GetSources()
{
  VECSOURCES* pGameSources = CMediaSourceSettings::GetInstance().GetSources("games");

  // Guard against source type not existing
  if (pGameSources == nullptr)
  {
    static VECSOURCES empty;
    return empty;
  }

  return *pGameSources;
}

void CGUIViewStateWindowGames::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_GAMES, CViewStateSettings::GetInstance().Get("games"));
}
