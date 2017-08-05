/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DialogGameViewMode.h"
#include "cores/IPlayer.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "Application.h"
#include "ApplicationPlayer.h"
#include "FileItem.h"

using namespace KODI;
using namespace GAME;

struct ViewModeProperties
{
  int stringIndex;
  ViewMode viewMode;
};

/*!
 * \brief The list of all the view modes along with their properties
 */
static const std::vector<ViewModeProperties> viewModes =
{
  { 630,   ViewModeNormal },
//  { 631,   ViewModeZoom }, //! @todo RetroArch allows trimming some outer pixels
  { 632,   ViewModeStretch4x3 },
  { 634,   ViewModeStretch16x9 },
  { 644,   ViewModeStretch16x9Nonlin },
  { 635,   ViewModeOriginal },
};

CDialogGameViewMode::CDialogGameViewMode() :
  CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIEW_MODE)
{
}

void CDialogGameViewMode::GetItems(CFileItemList &items)
{
  for (const auto &viewMode : viewModes)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(viewMode.stringIndex));
    items.Add(std::move(item));
  }
}

void CDialogGameViewMode::OnItemFocus(unsigned int index)
{
  if (index < viewModes.size())
  {
    const ViewMode viewMode = viewModes[index].viewMode;

    CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();
    if (gameSettings.ViewMode() != viewMode)
    {
      gameSettings.SetViewMode(viewMode);

      g_application.m_pPlayer->SetRenderViewMode(viewMode);
    }
  }
}

unsigned int CDialogGameViewMode::GetFocusedItem() const
{
  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (unsigned int i = 0; i < viewModes.size(); i++)
  {
    const ViewMode viewMode = viewModes[i].viewMode;
    if (viewMode == gameSettings.ViewMode())
      return i;
  }

  return 0;
}

bool CDialogGameViewMode::HasViewModes()
{
  if (g_application.m_pPlayer->Supports(RENDERFEATURE_STRETCH))
    return true;

  if (g_application.m_pPlayer->Supports(RENDERFEATURE_PIXEL_RATIO))
    return true;

  return false;
}
