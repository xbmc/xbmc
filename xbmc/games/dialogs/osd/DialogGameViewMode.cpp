/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "DialogGameViewMode.h"
#include "cores/RetroPlayer/guibridge/GUIGameVideoHandle.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/Variant.h"
#include "FileItem.h"

using namespace KODI;
using namespace GAME;

const std::vector<CDialogGameViewMode::ViewModeProperties> CDialogGameViewMode::m_allViewModes =
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

std::string CDialogGameViewMode::GetHeading()
{
  return g_localizeStrings.Get(629); // "View mode"
}

void CDialogGameViewMode::PreInit()
{
  m_viewModes.clear();

  for (const auto &viewMode : m_allViewModes)
  {
    bool bSupported = false;

    switch (viewMode.viewMode)
    {
      case ViewModeNormal:
      case ViewModeOriginal:
        bSupported = true;
        break;

      case ViewModeStretch4x3:
      case ViewModeStretch16x9:
        if (m_gameVideoHandle)
        {
          bSupported = m_gameVideoHandle->SupportsRenderFeature(RENDERFEATURE_STRETCH) ||
                       m_gameVideoHandle->SupportsRenderFeature(RENDERFEATURE_PIXEL_RATIO);
        }
        break;

      case ViewModeStretch16x9Nonlin:
        if (m_gameVideoHandle)
        {
          bSupported = m_gameVideoHandle->SupportsRenderFeature(RENDERFEATURE_NONLINSTRETCH);
        }
        break;

      default:
        break;
    }

    if (bSupported)
      m_viewModes.emplace_back(viewMode);
  }
}

void CDialogGameViewMode::GetItems(CFileItemList &items)
{
  for (const auto &viewMode : m_viewModes)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(viewMode.stringIndex));
    item->SetProperty("game.viewmode", CVariant{ viewMode.viewMode });
    items.Add(std::move(item));
  }
}

void CDialogGameViewMode::OnItemFocus(unsigned int index)
{
  if (index < m_viewModes.size())
  {
    const ViewMode viewMode = m_viewModes[index].viewMode;

    CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();
    if (gameSettings.ViewMode() != viewMode)
    {
      gameSettings.SetViewMode(viewMode);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);
    }
  }
}

unsigned int CDialogGameViewMode::GetFocusedItem() const
{
  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (unsigned int i = 0; i < m_viewModes.size(); i++)
  {
    const ViewMode viewMode = m_viewModes[i].viewMode;
    if (viewMode == gameSettings.ViewMode())
      return i;
  }

  return 0;
}

void CDialogGameViewMode::PostExit()
{
  m_viewModes.clear();
}
