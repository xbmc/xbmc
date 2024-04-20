/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameVideoRotation.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

CDialogGameVideoRotation::CDialogGameVideoRotation()
  : CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIDEO_ROTATION)
{
}

std::string CDialogGameVideoRotation::GetHeading()
{
  return g_localizeStrings.Get(35227); // "Rotation"
}

void CDialogGameVideoRotation::PreInit()
{
  m_rotations.clear();

  // Present the user with clockwise rotation
  m_rotations.push_back(0);
  m_rotations.push_back(270);
  m_rotations.push_back(180);
  m_rotations.push_back(90);
}

void CDialogGameVideoRotation::GetItems(CFileItemList& items)
{
  for (unsigned int rotation : m_rotations)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(GetRotationLabel(rotation));
    item->SetProperty("game.videorotation", CVariant{rotation});
    items.Add(std::move(item));
  }
}

void CDialogGameVideoRotation::OnItemFocus(unsigned int index)
{
  if (index < m_rotations.size())
  {
    const unsigned int rotationDegCCW = m_rotations[index];

    CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();
    if (gameSettings.RotationDegCCW() != rotationDegCCW)
    {
      gameSettings.SetRotationDegCCW(rotationDegCCW);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);
    }
  }
}

unsigned int CDialogGameVideoRotation::GetFocusedItem() const
{
  CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (unsigned int i = 0; i < m_rotations.size(); i++)
  {
    const unsigned int rotationDegCCW = m_rotations[i];
    if (rotationDegCCW == gameSettings.RotationDegCCW())
      return i;
  }

  return 0;
}

void CDialogGameVideoRotation::PostExit()
{
  m_rotations.clear();
}

bool CDialogGameVideoRotation::OnClickAction()
{
  Close();
  return true;
}

std::string CDialogGameVideoRotation::GetRotationLabel(unsigned int rotationDegCCW)
{
  switch (rotationDegCCW)
  {
    case 0:
      return g_localizeStrings.Get(35228); // 0
    case 90:
      return g_localizeStrings.Get(35231); // 270
    case 180:
      return g_localizeStrings.Get(35230); // 180
    case 270:
      return g_localizeStrings.Get(35229); // 90
    default:
      break;
  }

  return "";
}
