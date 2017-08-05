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
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "settings/VideoSettings.h" //! @todo
#include "Application.h"
#include "ApplicationPlayer.h"
#include "FileItem.h"

using namespace KODI;
using namespace GAME;

const std::vector<CDialogGameVideoFilter::VideoFilterProperties> CDialogGameVideoFilter::m_allVideoFilters =
{
  { 16301,  VS_SCALINGMETHOD_NEAREST },
  { 16302,  VS_SCALINGMETHOD_LINEAR },
  { 16303,  VS_SCALINGMETHOD_CUBIC },
  { 16304,  VS_SCALINGMETHOD_LANCZOS2 },
  { 16323,  VS_SCALINGMETHOD_SPLINE36_FAST },
  { 16315,  VS_SCALINGMETHOD_LANCZOS3_FAST },
  { 16322,  VS_SCALINGMETHOD_SPLINE36 },
  { 16305,  VS_SCALINGMETHOD_LANCZOS3 },
  { 16306,  VS_SCALINGMETHOD_SINC8 },
  { 16307,  VS_SCALINGMETHOD_BICUBIC_SOFTWARE },
  { 16308,  VS_SCALINGMETHOD_LANCZOS_SOFTWARE },
  { 16309,  VS_SCALINGMETHOD_SINC_SOFTWARE },
  { 13120,  VS_SCALINGMETHOD_VDPAU_HARDWARE },
  { 16319,  VS_SCALINGMETHOD_DXVA_HARDWARE },
};

CDialogGameVideoFilter::CDialogGameVideoFilter() :
  CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIDEO_FILTER)
{
}

void CDialogGameVideoFilter::PreInit()
{
  m_videoFilters.clear();

  for (const auto &videoFilter : m_allVideoFilters)
  {
    if (g_application.m_pPlayer->Supports(videoFilter.scalingMethod))
      m_videoFilters.emplace_back(videoFilter);
  }
}

void CDialogGameVideoFilter::GetItems(CFileItemList &items)
{
  for (const auto &videoFilter : m_videoFilters)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(videoFilter.stringIndex));
    items.Add(std::move(item));
  }

  if (items.Size() == 0)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(16316)); // "Auto"
    items.Add(std::move(item));
  }
}

void CDialogGameVideoFilter::OnItemFocus(unsigned int index)
{
  if (index < m_videoFilters.size())
  {
    const ESCALINGMETHOD scalingMethod = m_videoFilters[index].scalingMethod;

    CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();
    if (gameSettings.ScalingMethod() != scalingMethod)
    {
      gameSettings.SetScalingMethod(scalingMethod);

      //! @todo
      CVideoSettings &videoSettings = CMediaSettings::GetInstance().GetCurrentVideoSettings();
      videoSettings.m_ScalingMethod = scalingMethod;
    }
  }
}

unsigned int CDialogGameVideoFilter::GetFocusedItem() const
{
  CGameSettings &gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (unsigned int i = 0; i < m_videoFilters.size(); i++)
  {
    const ESCALINGMETHOD scalingMethod = m_videoFilters[i].scalingMethod;
    if (scalingMethod == gameSettings.ScalingMethod())
      return i;
  }

  return 0;
}

void CDialogGameVideoFilter::PostExit()
{
  m_videoFilters.clear();
}
