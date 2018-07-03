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

#include "DialogGameStretchMode.h"
#include "cores/RetroPlayer/guibridge/GUIGameVideoHandle.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/Variant.h"
#include "FileItem.h"

using namespace KODI;
using namespace GAME;

const std::vector<CDialogGameStretchMode::ViewModeProperties> CDialogGameStretchMode::m_allViewModes =
{
  { 630,   RETRO::VIEWMODE::Normal },
//  { 631,   RETRO::VIEWMODE::Zoom }, //! @todo RetroArch allows trimming some outer pixels
  { 632,   RETRO::VIEWMODE::Stretch4x3 },
  { 35232, RETRO::VIEWMODE::Fullscreen },
  { 635,   RETRO::VIEWMODE::Original },
};

CDialogGameStretchMode::CDialogGameStretchMode() :
  CDialogGameVideoSelect(WINDOW_DIALOG_GAME_STRETCH_MODE)
{
}

std::string CDialogGameStretchMode::GetHeading()
{
  return g_localizeStrings.Get(35233); // "Stretch mode"
}

void CDialogGameStretchMode::PreInit()
{
  m_viewModes.clear();

  for (const auto &viewMode : m_allViewModes)
  {
    bool bSupported = false;

    switch (viewMode.viewMode)
    {
      case RETRO::VIEWMODE::Normal:
      case RETRO::VIEWMODE::Original:
        bSupported = true;
        break;

      case RETRO::VIEWMODE::Stretch4x3:
      case RETRO::VIEWMODE::Fullscreen:
        if (m_gameVideoHandle)
        {
          bSupported = m_gameVideoHandle->SupportsRenderFeature(RETRO::RENDERFEATURE::STRETCH) ||
                       m_gameVideoHandle->SupportsRenderFeature(RETRO::RENDERFEATURE::PIXEL_RATIO);
        }
        break;

      default:
        break;
    }

    if (bSupported)
      m_viewModes.emplace_back(viewMode);
  }
}

void CDialogGameStretchMode::GetItems(CFileItemList &items)
{
  for (const auto &viewMode : m_viewModes)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(viewMode.stringIndex));
    item->SetProperty("game.viewmode", CVariant{ static_cast<int>(viewMode.viewMode) });
    items.Add(std::move(item));
  }
}

void CDialogGameStretchMode::OnItemFocus(unsigned int index)
{
  if (index < m_viewModes.size())
  {
    const RETRO::VIEWMODE viewMode = m_viewModes[index].viewMode;

    CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();
    if (gameSettings.ViewMode() != viewMode)
    {
      gameSettings.SetViewMode(viewMode);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);
    }
  }
}

unsigned int CDialogGameStretchMode::GetFocusedItem() const
{
  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (unsigned int i = 0; i < m_viewModes.size(); i++)
  {
    const RETRO::VIEWMODE viewMode = m_viewModes[i].viewMode;
    if (viewMode == gameSettings.ViewMode())
      return i;
  }

  return 0;
}

void CDialogGameStretchMode::PostExit()
{
  m_viewModes.clear();
}
