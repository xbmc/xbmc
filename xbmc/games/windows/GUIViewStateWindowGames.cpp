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
#include "guilib/WindowIDs.h"
#include "settings/MediaSourceSettings.h"
#include "utils/StringUtils.h"
#include "view/ViewState.h"
#include "view/ViewStateNames.h"
#include "view/ViewStateSettings.h"

#include <assert.h>
#include <set>

using namespace KODI;
using namespace GAME;
using KODI::MEDIA::MediaSection;

CGUIViewStateWindowGames::CGUIViewStateWindowGames(const CFileItemList& items)
  : CGUIViewState(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortBy::LABEL, 551, LABEL_MASKS());
    AddSortMethod(SortBy::DRIVE_TYPE, 564, LABEL_MASKS());
    SetSortMethod(SortBy::LABEL);
    SetSortOrder(SortOrder::ASCENDING);
    SetViewAsControl(DEFAULT_VIEW_LIST);
  }
  else
  {
    AddSortMethod(SortBy::FILE, 561,
                  LABEL_MASKS("%F", "%I", "%L", "")); // Filename, Size | Label, empty
    AddSortMethod(SortBy::SIZE, 553,
                  LABEL_MASKS("%L", "%I", "%L", "%I")); // Filename, Size | Label, Size

    const CViewState* viewState = CViewStateSettings::GetInstance().Get(VIEW_STATE::GAMES);
    if (viewState)
    {
      SetSortMethod(viewState->m_sortDescription);
      SetViewAsControl(viewState->m_viewMode);
      SetSortOrder(viewState->m_sortDescription.sortOrder);
    }
  }

  LoadViewState(items.GetPath(), WINDOW_GAMES);
}

std::optional<KODI::MEDIA::MediaSection> CGUIViewStateWindowGames::GetLockType()
{
  return KODI::MEDIA::MediaSection::GAMES;
}

std::string CGUIViewStateWindowGames::GetExtensions()
{
  std::set<std::string> exts = CGameUtils::GetGameExtensions();

  // Ensure .zip appears
  exts.insert(".zip");

  return StringUtils::Join(exts, "|");
}

std::vector<CMediaSource>& CGUIViewStateWindowGames::GetSources()
{
  return CMediaSourceSettings::GetInstance().GetSources(MediaSection::GAMES);
}

void CGUIViewStateWindowGames::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_GAMES,
               CViewStateSettings::GetInstance().Get(VIEW_STATE::GAMES));
}
