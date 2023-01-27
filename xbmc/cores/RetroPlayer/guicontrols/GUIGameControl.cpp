/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameControl.h"

#include "GUIRenderSettings.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIRenderHandle.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/Geometry.h"
#include "utils/StringUtils.h"

#include <sstream>

using namespace KODI;
using namespace RETRO;

CGUIGameControl::CGUIGameControl(
    int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_renderSettings(new CGUIRenderSettings(*this))
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAME;

  m_renderSettings->SetDimensions(CRect(CPoint(posX, posY), CSize(width, height)));

  RegisterControl();
}

CGUIGameControl::CGUIGameControl(const CGUIGameControl& other)
  : CGUIControl(other),
    m_videoFilterInfo(other.m_videoFilterInfo),
    m_stretchModeInfo(other.m_stretchModeInfo),
    m_rotationInfo(other.m_rotationInfo),
    m_pixelInfo(other.m_pixelInfo),
    m_bHasVideoFilter(other.m_bHasVideoFilter),
    m_bHasStretchMode(other.m_bHasStretchMode),
    m_bHasRotation(other.m_bHasRotation),
    m_bHasPixels(other.m_bHasPixels),
    m_renderSettings(new CGUIRenderSettings(*this))
{
  m_renderSettings->SetSettings(other.m_renderSettings->GetSettings());
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(m_width, m_height)));

  RegisterControl();
}

CGUIGameControl::~CGUIGameControl()
{
  UnregisterControl();
}

void CGUIGameControl::SetVideoFilter(const GUILIB::GUIINFO::CGUIInfoLabel& videoFilter)
{
  m_videoFilterInfo = videoFilter;
}

void CGUIGameControl::SetStretchMode(const GUILIB::GUIINFO::CGUIInfoLabel& stretchMode)
{
  m_stretchModeInfo = stretchMode;
}

void CGUIGameControl::SetRotation(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& rotation)
{
  m_rotationInfo = rotation;
}

void CGUIGameControl::SetPixels(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& pixels)
{
  m_pixelInfo = pixels;
}

IGUIRenderSettings* CGUIGameControl::GetRenderSettings() const
{
  return m_renderSettings.get();
}

void CGUIGameControl::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed
  if (m_renderHandle->IsDirty())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIGameControl::Render()
{
  m_renderHandle->Render();

  CGUIControl::Render();
}

void CGUIGameControl::RenderEx()
{
  m_renderHandle->RenderEx();

  CGUIControl::RenderEx();
}

bool CGUIGameControl::CanFocus() const
{
  // Unfocusable
  return false;
}

void CGUIGameControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);
  m_renderSettings->SetDimensions(CRect(CPoint(posX, posY), CSize(m_width, m_height)));
}

void CGUIGameControl::SetWidth(float width)
{
  CGUIControl::SetWidth(width);
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(width, m_height)));
}

void CGUIGameControl::SetHeight(float height)
{
  CGUIControl::SetHeight(height);
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(m_width, height)));
}

void CGUIGameControl::UpdateInfo(const CGUIListItem* item /* = nullptr */)
{
  if (item)
  {
    Reset();

    std::string strVideoFilter = m_videoFilterInfo.GetItemLabel(item);
    if (!strVideoFilter.empty())
    {
      m_renderSettings->SetVideoFilter(strVideoFilter);
      m_bHasVideoFilter = true;
    }

    std::string strStretchMode = m_stretchModeInfo.GetItemLabel(item);
    if (!strStretchMode.empty())
    {
      STRETCHMODE stretchMode = CRetroPlayerUtils::IdentifierToStretchMode(strStretchMode);
      m_renderSettings->SetStretchMode(stretchMode);
      m_bHasStretchMode = true;
    }

    std::string strRotation = m_rotationInfo.GetItemLabel(item);
    if (StringUtils::IsNaturalNumber(strRotation))
    {
      unsigned int rotation;
      std::istringstream(strRotation) >> rotation;
      m_renderSettings->SetRotationDegCCW(rotation);
      m_bHasRotation = true;
    }

    std::string strPixels = m_pixelInfo.GetItemLabel(item);
    if (!strPixels.empty())
    {
      m_renderSettings->SetPixels(strPixels);
      m_bHasPixels = true;
    }
  }
}

void CGUIGameControl::Reset()
{
  m_bHasVideoFilter = false;
  m_bHasStretchMode = false;
  m_bHasRotation = false;
  m_bHasPixels = false;
  m_renderSettings->Reset();
}

void CGUIGameControl::RegisterControl()
{
  m_renderHandle = CServiceBroker::GetGameRenderManager().RegisterControl(*this);
}

void CGUIGameControl::UnregisterControl()
{
  m_renderHandle.reset();
}
