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
#include "cores/RetroPlayer/rendering/IRenderSettingsCallback.h"
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
        if (m_callback != nullptr)
        {
          bSupported = m_callback->SupportsRenderFeature(RENDERFEATURE_STRETCH) ||
                       m_callback->SupportsRenderFeature(RENDERFEATURE_PIXEL_RATIO);
        }
        break;

      case ViewModeStretch16x9Nonlin:
        if (m_callback != nullptr)
        {
          bSupported = m_callback->SupportsRenderFeature(RENDERFEATURE_NONLINSTRETCH);
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

      if (m_callback != nullptr)
        m_callback->SetRenderViewMode(viewMode);
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
