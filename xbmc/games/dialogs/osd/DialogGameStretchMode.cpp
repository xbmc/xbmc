/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameStretchMode.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "cores/RetroPlayer/guibridge/GUIGameVideoHandle.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

const std::vector<CDialogGameStretchMode::StretchModeProperties>
    CDialogGameStretchMode::m_allStretchModes = {
        {630, RETRO::STRETCHMODE::Normal},
        //  { 631,   RETRO::STRETCHMODE::Zoom }, //! @todo RetroArch allows trimming some outer
        //  pixels
        {632, RETRO::STRETCHMODE::Stretch4x3},
        {35232, RETRO::STRETCHMODE::Fullscreen},
        {635, RETRO::STRETCHMODE::Original},
};

CDialogGameStretchMode::CDialogGameStretchMode()
  : CDialogGameVideoSelect(WINDOW_DIALOG_GAME_STRETCH_MODE)
{
}

std::string CDialogGameStretchMode::GetHeading()
{
  return g_localizeStrings.Get(35233); // "Stretch mode"
}

void CDialogGameStretchMode::PreInit()
{
  m_stretchModes.clear();

  for (const auto& stretchMode : m_allStretchModes)
  {
    bool bSupported = false;

    switch (stretchMode.stretchMode)
    {
      case RETRO::STRETCHMODE::Normal:
      case RETRO::STRETCHMODE::Original:
        bSupported = true;
        break;

      case RETRO::STRETCHMODE::Stretch4x3:
      case RETRO::STRETCHMODE::Fullscreen:
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
      m_stretchModes.emplace_back(stretchMode);
  }
}

void CDialogGameStretchMode::GetItems(CFileItemList& items)
{
  for (const auto& stretchMode : m_stretchModes)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(stretchMode.stringIndex));

    const std::string stretchModeId =
        RETRO::CRetroPlayerUtils::StretchModeToIdentifier(stretchMode.stretchMode);
    if (!stretchModeId.empty())
      item->SetProperty("game.stretchmode", CVariant{stretchModeId});
    items.Add(std::move(item));
  }
}

void CDialogGameStretchMode::OnItemFocus(unsigned int index)
{
  if (index < m_stretchModes.size())
  {
    const RETRO::STRETCHMODE stretchMode = m_stretchModes[index].stretchMode;

    CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();
    if (gameSettings.StretchMode() != stretchMode)
    {
      gameSettings.SetStretchMode(stretchMode);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);
    }
  }
}

unsigned int CDialogGameStretchMode::GetFocusedItem() const
{
  CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (unsigned int i = 0; i < m_stretchModes.size(); i++)
  {
    const RETRO::STRETCHMODE stretchMode = m_stretchModes[i].stretchMode;
    if (stretchMode == gameSettings.StretchMode())
      return i;
  }

  return 0;
}

void CDialogGameStretchMode::PostExit()
{
  m_stretchModes.clear();
}

bool CDialogGameStretchMode::OnClickAction()
{
  Close();
  return true;
}
