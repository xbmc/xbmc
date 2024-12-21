/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUICameraControl.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "smarthome/SmartHomeServices.h"
#include "smarthome/guibridge/GUIRenderHandle.h"
#include "smarthome/guibridge/GUIRenderSettings.h"
#include "smarthome/guibridge/SmartHomeGuiBridge.h"
#include "smarthome/guicontrols/GUICameraConfig.h"
#include "smarthome/ros2/IRos2.h"
#include "utils/Geometry.h"

#include <sstream>

using namespace KODI;
using namespace SMART_HOME;

void CGUICameraControl::RenderSettingsDeleter::operator()(CGUIRenderSettings* obj)
{
  delete obj;
};

CGUICameraControl::CGUICameraControl(
    int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_cameraConfig(std::make_unique<CGUICameraConfig>()),
    m_renderSettings(new CGUIRenderSettings(*this))
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_CAMERA;

  m_renderSettings->SetDimensions(CRect(CPoint(posX, posY), CSize(width, height)));
}

CGUICameraControl::CGUICameraControl(const CGUICameraControl& other)
  : CGUIControl(other),
    m_cameraConfig(std::make_unique<CGUICameraConfig>(*other.m_cameraConfig)),
    m_renderSettings(new CGUIRenderSettings(*other.m_renderSettings)),
    m_bHasStretchMode(other.m_bHasStretchMode),
    m_bHasRotation(other.m_bHasRotation),
    m_stretchModeInfo(other.m_stretchModeInfo),
    m_rotationInfo(other.m_rotationInfo)
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(m_width, m_height)));

  SetPubSubTopic(other.m_topicInfo);
}

CGUICameraControl::~CGUICameraControl()
{
  UnregisterControl();
}

void CGUICameraControl::SetPubSubTopic(const GUILIB::GUIINFO::CGUIInfoLabel& topic)
{
  m_topicInfo = topic;

  // Check if a topic is available without a listitem
  CFileItem empty;
  const std::string strTopic = m_topicInfo.GetItemLabel(&empty);
  if (!strTopic.empty())
    UpdateTopic(strTopic);
}

void CGUICameraControl::SetStretchMode(const GUILIB::GUIINFO::CGUIInfoLabel& stretchMode)
{
  m_stretchModeInfo = stretchMode;

  // Check if a stretch mode is available without a listitem
  CFileItem empty;
  const std::string strStretchMode = m_stretchModeInfo.GetItemLabel(&empty);
  if (!strStretchMode.empty())
    UpdateStretchMode(strStretchMode);
}

void CGUICameraControl::SetRotation(const GUILIB::GUIINFO::CGUIInfoLabel& rotation)
{
  m_rotationInfo = rotation;

  // Check if a rotation is available without a listitem
  CFileItem empty;
  const std::string strRotation = m_rotationInfo.GetItemLabel(&empty);
  if (!strRotation.empty())
    UpdateRotation(strRotation);
}

void CGUICameraControl::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  //! @todo Proper processing which marks when its actually changed
  if (m_renderHandle && m_renderHandle->IsDirty())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUICameraControl::Render()
{
  if (m_renderHandle)
    m_renderHandle->Render();

  CGUIControl::Render();
}

bool CGUICameraControl::CanFocus() const
{
  // Unfocusable
  return false;
}

void CGUICameraControl::SetPosition(float posX, float posY)
{
  m_renderSettings->SetDimensions(CRect(CPoint(posX, posY), CSize(m_width, m_height)));

  CGUIControl::SetPosition(posX, posY);
}

void CGUICameraControl::SetWidth(float width)
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(width, m_height)));

  CGUIControl::SetWidth(width);
}

void CGUICameraControl::SetHeight(float height)
{
  m_renderSettings->SetDimensions(CRect(CPoint(m_posX, m_posY), CSize(m_width, height)));

  CGUIControl::SetHeight(height);
}

void CGUICameraControl::UpdateInfo(const CGUIListItem* item /* = nullptr */)
{
  if (item != nullptr)
  {
    ResetInfo();

    std::string strTopic = m_topicInfo.GetItemLabel(item);
    if (!strTopic.empty())
      UpdateTopic(strTopic);

    std::string strStretchMode = m_stretchModeInfo.GetItemLabel(item);
    if (!strStretchMode.empty())
      UpdateStretchMode(strStretchMode);

    std::string strRotation = m_rotationInfo.GetItemLabel(item);
    if (!strRotation.empty())
      UpdateRotation(strRotation);
  }
}

void CGUICameraControl::UpdateTopic(const std::string& topic)
{
  if (topic != m_cameraConfig->GetTopic())
  {
    m_cameraConfig->SetTopic(topic);

    RegisterControl(topic);
  }
}

void CGUICameraControl::UpdateStretchMode(const std::string& strStretchMode)
{
  const RETRO::STRETCHMODE stretchMode =
      RETRO::CRetroPlayerUtils::IdentifierToStretchMode(strStretchMode);
  m_renderSettings->SetStretchMode(stretchMode);
  m_bHasStretchMode = true;
}

void CGUICameraControl::UpdateRotation(const std::string& strRotation)
{
  const unsigned int rotationDegCCW = std::stoul(strRotation);
  m_renderSettings->SetRotationDegCCW(rotationDegCCW);
  m_bHasRotation = true;
}

void CGUICameraControl::ResetInfo()
{
  m_bHasStretchMode = false;
  m_bHasRotation = false;
}

void CGUICameraControl::RegisterControl(const std::string& topic)
{
  UnregisterControl();

  IRos2* ros2 = CServiceBroker::GetSmartHomeServices().Ros2();
  if (ros2 != nullptr)
    CServiceBroker::GetSmartHomeServices().Ros2()->RegisterImageTopic(topic);

  m_renderHandle = CServiceBroker::GetSmartHomeServices().GuiBridge(topic).RegisterControl(*this);
}

void CGUICameraControl::UnregisterControl()
{
  m_renderHandle.reset();
}
