/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameVideoFilter.h"

#include "cores/RetroPlayer/guibridge/GUIGameVideoHandle.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
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
  RETRO::SCALINGMETHOD scalingMethod;
};

const std::vector<ScalingMethodProperties> scalingMethods = {
    {16301, 16296, 16298, RETRO::SCALINGMETHOD::NEAREST},
    {16302, 16297, 16299, RETRO::SCALINGMETHOD::LINEAR},
};
} // namespace

CDialogGameVideoFilter::CDialogGameVideoFilter()
  : CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIDEO_FILTER)
{
}

std::string CDialogGameVideoFilter::GetHeading()
{
  return g_localizeStrings.Get(35225); // "Video filter"
}

void CDialogGameVideoFilter::PreInit()
{
  m_items.Clear();

  InitVideoFilters();

  if (m_items.Size() == 0)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(231)); // "None"
    m_items.Add(std::move(item));
  }

  m_bHasDescription = false;
}

void CDialogGameVideoFilter::InitVideoFilters()
{
  if (m_gameVideoHandle)
  {
    for (const auto& scalingMethodProps : scalingMethods)
    {
      if (m_gameVideoHandle->SupportsScalingMethod(scalingMethodProps.scalingMethod))
      {
        RETRO::CRenderVideoSettings videoSettings;
        videoSettings.SetScalingMethod(scalingMethodProps.scalingMethod);

        CFileItemPtr item =
            std::make_shared<CFileItem>(g_localizeStrings.Get(scalingMethodProps.nameIndex));
        item->SetLabel2(g_localizeStrings.Get(scalingMethodProps.categoryIndex));
        item->SetProperty("game.videofilter", CVariant{videoSettings.GetVideoFilter()});
        item->SetProperty("game.videofilterdescription",
                          CVariant{g_localizeStrings.Get(scalingMethodProps.descriptionIndex)});
        m_items.Add(std::move(item));
      }
    }
  }
}

void CDialogGameVideoFilter::GetItems(CFileItemList& items)
{
  for (const auto& item : m_items)
    items.Add(item);
}

void CDialogGameVideoFilter::OnItemFocus(unsigned int index)
{
  if (static_cast<int>(index) < m_items.Size())
  {
    CFileItemPtr item = m_items[index];

    std::string videoFilter;
    std::string description;
    GetProperties(*item, videoFilter, description);

    CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

    if (gameSettings.VideoFilter() != videoFilter)
    {
      gameSettings.SetVideoFilter(videoFilter);
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
  CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (int i = 0; i < m_items.Size(); i++)
  {
    std::string videoFilter;
    std::string description;
    GetProperties(*m_items[i], videoFilter, description);

    if (videoFilter == gameSettings.VideoFilter())
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

bool CDialogGameVideoFilter::OnClickAction()
{
  Close();
  return true;
}

void CDialogGameVideoFilter::GetProperties(const CFileItem& item,
                                           std::string& videoFilter,
                                           std::string& description)
{
  videoFilter = item.GetProperty("game.videofilter").asString();
  description = item.GetProperty("game.videofilterdescription").asString();
}
