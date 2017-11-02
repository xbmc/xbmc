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

#include "DialogGameVideoFilter.h"
#include "cores/RetroPlayer/rendering/GUIGameVideoHandle.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

namespace
{
  struct ScalingMethodProperties
  {
    int nameIndex;
    int categoryIndex;
    int descriptionIndex;
    ESCALINGMETHOD scalingMethod;
  };

  const std::vector<ScalingMethodProperties> scalingMethods =
  {
    { 16301, 16296, 16298, VS_SCALINGMETHOD_NEAREST },
    { 16302, 16297, 16299, VS_SCALINGMETHOD_LINEAR },
  };
}

CDialogGameVideoFilter::CDialogGameVideoFilter() :
  CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIDEO_FILTER)
{
}

std::string CDialogGameVideoFilter::GetHeading()
{
  return g_localizeStrings.Get(35225); // "Video filter"
}

void CDialogGameVideoFilter::PreInit()
{
  m_items.Clear();

  InitScalingMethods();

  if (m_items.Size() == 0)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(231)); // "None"
    m_items.Add(std::move(item));
  }

  m_bHasDescription = false;
}

void CDialogGameVideoFilter::InitScalingMethods()
{
  if (m_gameVideoHandle)
  {
    for (const auto &scalingMethodProps : scalingMethods)
    {
      if (m_gameVideoHandle->SupportsScalingMethod(scalingMethodProps.scalingMethod))
      {
        CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(scalingMethodProps.nameIndex));
        item->SetLabel2(g_localizeStrings.Get(scalingMethodProps.categoryIndex));
        item->SetProperty("game.scalingmethod", CVariant{ scalingMethodProps.scalingMethod });
        item->SetProperty("game.videofilterdescription", CVariant{ g_localizeStrings.Get(scalingMethodProps.descriptionIndex) });
        m_items.Add(std::move(item));
      }
    }
  }
}

void CDialogGameVideoFilter::GetItems(CFileItemList &items)
{
  for (const auto &item : m_items)
    items.Add(item);
}

void CDialogGameVideoFilter::OnItemFocus(unsigned int index)
{
  if (static_cast<int>(index) < m_items.Size())
  {
    CFileItemPtr item = m_items[index];

    ESCALINGMETHOD scalingMethod;
    std::string description;
    GetProperties(*item, scalingMethod, description);

    CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

    if (gameSettings.ScalingMethod() != scalingMethod)
    {
      gameSettings.SetScalingMethod(scalingMethod);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);

      OnDescriptionChange(description);
      m_bHasDescription = true;
    }
    else if (!m_bHasDescription)
    {
      OnDescriptionChange(description);
      m_bHasDescription = true;
    }
  }
}

unsigned int CDialogGameVideoFilter::GetFocusedItem() const
{
  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (int i = 0; i < m_items.Size(); i++)
  {
    ESCALINGMETHOD scalingMethod;
    std::string description;
    GetProperties(*m_items[i], scalingMethod, description);

    if (scalingMethod == gameSettings.ScalingMethod())
    {
      return i;
    }
  }

  return 0;
}

void CDialogGameVideoFilter::PostExit()
{
  m_items.Clear();
}

void CDialogGameVideoFilter::GetProperties(const CFileItem &item, ESCALINGMETHOD &scalingMethod, std::string &description)
{
  scalingMethod = VS_SCALINGMETHOD_AUTO;
  description = item.GetProperty("game.videofilterdescription").asString();

  std::string strScalingMethod = item.GetProperty("game.scalingmethod").asString();
  if (StringUtils::IsNaturalNumber(strScalingMethod))
    scalingMethod = static_cast<ESCALINGMETHOD>(item.GetProperty("game.scalingmethod").asUnsignedInteger());
}
