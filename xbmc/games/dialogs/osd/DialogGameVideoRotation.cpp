/*
 *      Copyright (C) 2018 Team Kodi
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

#include "DialogGameVideoRotation.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/Variant.h"
#include "FileItem.h"

using namespace KODI;
using namespace GAME;

CDialogGameVideoRotation::CDialogGameVideoRotation() :
  CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIDEO_ROTATION)
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

void CDialogGameVideoRotation::GetItems(CFileItemList &items)
{
  for (unsigned int rotation : m_rotations)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(GetRotationLabel(rotation));
    item->SetProperty("game.videorotation", CVariant{ rotation });
    items.Add(std::move(item));
  }
}

void CDialogGameVideoRotation::OnItemFocus(unsigned int index)
{
  if (index < m_rotations.size())
  {
    const unsigned int rotationDegCCW = m_rotations[index];

    CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();
    if (gameSettings.RotationDegCCW() != rotationDegCCW)
    {
      gameSettings.SetRotationDegCCW(rotationDegCCW);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);
    }
  }
}

unsigned int CDialogGameVideoRotation::GetFocusedItem() const
{
  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

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
