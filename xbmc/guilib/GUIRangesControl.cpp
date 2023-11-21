/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRangesControl.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cmath>

CGUIRangesControl::CGUIRange::CGUIRange(float fPosX,
                                        float fPosY,
                                        float fWidth,
                                        float fHeight,
                                        const CTextureInfo& lowerTextureInfo,
                                        const CTextureInfo& fillTextureInfo,
                                        const CTextureInfo& upperTextureInfo,
                                        const std::pair<float, float>& percentages)
  : m_guiLowerTexture(CGUITexture::CreateTexture(fPosX, fPosY, fWidth, fHeight, lowerTextureInfo)),
    m_guiFillTexture(CGUITexture::CreateTexture(fPosX, fPosY, fWidth, fHeight, fillTextureInfo)),
    m_guiUpperTexture(CGUITexture::CreateTexture(fPosX, fPosY, fWidth, fHeight, upperTextureInfo)),
    m_percentValues(percentages)
{
}

CGUIRangesControl::CGUIRange::CGUIRange(const CGUIRange& range)
  : m_guiLowerTexture(range.m_guiLowerTexture->Clone()),
    m_guiFillTexture(range.m_guiFillTexture->Clone()),
    m_guiUpperTexture(range.m_guiUpperTexture->Clone()),
    m_percentValues(range.m_percentValues)
{
}

void CGUIRangesControl::CGUIRange::AllocResources()
{
  m_guiFillTexture->AllocResources();
  m_guiUpperTexture->AllocResources();
  m_guiLowerTexture->AllocResources();

  m_guiFillTexture->SetHeight(20);
  m_guiUpperTexture->SetHeight(20);
  m_guiLowerTexture->SetHeight(20);
}

void CGUIRangesControl::CGUIRange::FreeResources(bool bImmediately)
{
  m_guiFillTexture->FreeResources(bImmediately);
  m_guiUpperTexture->FreeResources(bImmediately);
  m_guiLowerTexture->FreeResources(bImmediately);
}

void CGUIRangesControl::CGUIRange::DynamicResourceAlloc(bool bOnOff)
{
  m_guiFillTexture->DynamicResourceAlloc(bOnOff);
  m_guiUpperTexture->DynamicResourceAlloc(bOnOff);
  m_guiLowerTexture->DynamicResourceAlloc(bOnOff);
}

void CGUIRangesControl::CGUIRange::SetInvalid()
{
  m_guiFillTexture->SetInvalid();
  m_guiUpperTexture->SetInvalid();
  m_guiLowerTexture->SetInvalid();
}

bool CGUIRangesControl::CGUIRange::SetDiffuseColor(const KODI::GUILIB::GUIINFO::CGUIInfoColor& color)
{
  bool bChanged = false;
  bChanged |= m_guiFillTexture->SetDiffuseColor(color);
  bChanged |= m_guiUpperTexture->SetDiffuseColor(color);
  bChanged |= m_guiLowerTexture->SetDiffuseColor(color);
  return bChanged;
}

bool CGUIRangesControl::CGUIRange::Process(unsigned int currentTime)
{
  bool bChanged = false;
  bChanged |= m_guiFillTexture->Process(currentTime);
  bChanged |= m_guiUpperTexture->Process(currentTime);
  bChanged |= m_guiLowerTexture->Process(currentTime);
  return bChanged;
}

void CGUIRangesControl::CGUIRange::Render()
{
  if (m_guiLowerTexture->GetFileName().empty() && m_guiUpperTexture->GetFileName().empty())
  {
    if (m_guiFillTexture->GetWidth() > 0)
      m_guiFillTexture->Render();
  }
  else
  {
    m_guiLowerTexture->Render();

    if (m_guiFillTexture->GetWidth() > 0)
      m_guiFillTexture->Render();

    m_guiUpperTexture->Render();
  }
}

bool CGUIRangesControl::CGUIRange::UpdateLayout(float fBackgroundTextureHeight,
                                                float fPosX, float fPosY, float fWidth,
                                                float fScaleX, float fScaleY)
{
  bool bChanged = false;

  if (m_guiLowerTexture->GetFileName().empty() && m_guiUpperTexture->GetFileName().empty())
  {
    // rendering without left and right image - fill the mid image completely
    float width = (m_percentValues.second - m_percentValues.first) * fWidth * 0.01f;
    float offsetX = m_percentValues.first * fWidth * 0.01f;
    float offsetY = std::fabs(fScaleY * 0.5f *
                              (m_guiFillTexture->GetTextureHeight() - fBackgroundTextureHeight));
    bChanged |= m_guiFillTexture->SetPosition(fPosX + (offsetX > 0 ? offsetX : 0),
                                              fPosY + (offsetY > 0 ? offsetY : 0));
    bChanged |= m_guiFillTexture->SetHeight(fScaleY * m_guiFillTexture->GetTextureHeight());
    bChanged |= m_guiFillTexture->SetWidth(width);
  }
  else
  {
    float offsetX =
        m_percentValues.first * fWidth * 0.01f - m_guiLowerTexture->GetTextureWidth() * 0.5f;
    float offsetY = std::fabs(fScaleY * 0.5f *
                              (m_guiLowerTexture->GetTextureHeight() - fBackgroundTextureHeight));
    bChanged |= m_guiLowerTexture->SetPosition(fPosX + (offsetX > 0 ? offsetX : 0),
                                               fPosY + (offsetY > 0 ? offsetY : 0));
    bChanged |= m_guiLowerTexture->SetHeight(fScaleY * m_guiLowerTexture->GetTextureHeight());
    bChanged |= m_guiLowerTexture->SetWidth(m_percentValues.first == 0.0f
                                                ? m_guiLowerTexture->GetTextureWidth() * 0.5f
                                                : m_guiLowerTexture->GetTextureWidth());

    if (m_percentValues.first != m_percentValues.second)
    {
      float width = (m_percentValues.second - m_percentValues.first) * fWidth * 0.01f -
                    m_guiLowerTexture->GetTextureWidth() * 0.5f -
                    m_guiUpperTexture->GetTextureWidth() * 0.5f;

      offsetX += m_guiLowerTexture->GetTextureWidth();
      offsetY = std::fabs(fScaleY * 0.5f *
                          (m_guiFillTexture->GetTextureHeight() - fBackgroundTextureHeight));
      bChanged |=
          m_guiFillTexture->SetPosition(fPosX + offsetX, fPosY + (offsetY > 0 ? offsetY : 0));
      bChanged |= m_guiFillTexture->SetHeight(fScaleY * m_guiFillTexture->GetTextureHeight());
      bChanged |= m_guiFillTexture->SetWidth(width);

      offsetX += width;
      offsetY = std::fabs(fScaleY * 0.5f *
                          (m_guiUpperTexture->GetTextureHeight() - fBackgroundTextureHeight));
      bChanged |=
          m_guiUpperTexture->SetPosition(fPosX + offsetX, fPosY + (offsetY > 0 ? offsetY : 0));
      bChanged |= m_guiUpperTexture->SetHeight(fScaleY * m_guiUpperTexture->GetTextureHeight());
      bChanged |= m_guiUpperTexture->SetWidth(m_percentValues.first == 100.0f
                                                  ? m_guiUpperTexture->GetTextureWidth() * 0.5f
                                                  : m_guiUpperTexture->GetTextureWidth());
    }
    else
    {
      // render only lower texture if range is zero
      bChanged |= m_guiFillTexture->SetVisible(false);
      bChanged |= m_guiUpperTexture->SetVisible(false);
    }
  }
  return bChanged;
}

CGUIRangesControl::CGUIRangesControl(int iParentID,
                                     int iControlID,
                                     float fPosX,
                                     float fPosY,
                                     float fWidth,
                                     float fHeight,
                                     const CTextureInfo& backGroundTextureInfo,
                                     const CTextureInfo& lowerTextureInfo,
                                     const CTextureInfo& fillTextureInfo,
                                     const CTextureInfo& upperTextureInfo,
                                     const CTextureInfo& overlayTextureInfo,
                                     int iInfo)
  : CGUIControl(iParentID, iControlID, fPosX, fPosY, fWidth, fHeight),
    m_guiBackground(
        CGUITexture::CreateTexture(fPosX, fPosY, fWidth, fHeight, backGroundTextureInfo)),
    m_guiOverlay(CGUITexture::CreateTexture(fPosX, fPosY, fWidth, fHeight, overlayTextureInfo)),
    m_guiLowerTextureInfo(lowerTextureInfo),
    m_guiFillTextureInfo(fillTextureInfo),
    m_guiUpperTextureInfo(upperTextureInfo),
    m_iInfoCode(iInfo)
{
  ControlType = GUICONTROL_RANGES;
}

CGUIRangesControl::CGUIRangesControl(const CGUIRangesControl& control)
  : CGUIControl(control),
    m_guiBackground(control.m_guiBackground->Clone()),
    m_guiOverlay(control.m_guiOverlay->Clone()),
    m_guiLowerTextureInfo(control.m_guiLowerTextureInfo),
    m_guiFillTextureInfo(control.m_guiFillTextureInfo),
    m_guiUpperTextureInfo(control.m_guiUpperTextureInfo),
    m_ranges(control.m_ranges),
    m_iInfoCode(control.m_iInfoCode),
    m_prevRanges(control.m_prevRanges)
{
}

void CGUIRangesControl::SetPosition(float fPosX, float fPosY)
{
  // everything is positioned based on the background image position
  CGUIControl::SetPosition(fPosX, fPosY);
  m_guiBackground->SetPosition(fPosX, fPosY);
}

void CGUIRangesControl::Process(unsigned int iCurrentTime, CDirtyRegionList& dirtyregions)
{
  bool bChanged = false;

  if (!IsDisabled())
    bChanged |= UpdateLayout();

  bChanged |= m_guiBackground->Process(iCurrentTime);
  bChanged |= m_guiOverlay->Process(iCurrentTime);

  for (auto& range : m_ranges)
    bChanged |= range.Process(iCurrentTime);

  if (bChanged)
    MarkDirtyRegion();

  CGUIControl::Process(iCurrentTime, dirtyregions);
}

void CGUIRangesControl::Render()
{
  if (!IsDisabled())
  {
    m_guiBackground->Render();

    for (auto& range : m_ranges)
      range.Render();

    m_guiOverlay->Render();
  }

  CGUIControl::Render();
}


bool CGUIRangesControl::CanFocus() const
{
  return false;
}


void CGUIRangesControl::SetRanges(const std::vector<std::pair<float, float>>& ranges)
{
  ClearRanges();
  for (const auto& range : ranges)
    m_ranges.emplace_back(m_posX, m_posY, m_width, m_height, m_guiLowerTextureInfo,
                          m_guiFillTextureInfo, m_guiUpperTextureInfo, range);

  for (auto& range : m_ranges)
    range.AllocResources(); // note: we need to alloc the instance actually inserted into the vector; hence the second loop.
}

void CGUIRangesControl::ClearRanges()
{
  for (auto& range : m_ranges)
    range.FreeResources(true);

  m_ranges.clear();
  m_prevRanges.clear();
}

void CGUIRangesControl::FreeResources(bool bImmediately /* = false */)
{
  CGUIControl::FreeResources(bImmediately);

  m_guiBackground->FreeResources(bImmediately);
  m_guiOverlay->FreeResources(bImmediately);

  for (auto& range : m_ranges)
    range.FreeResources(bImmediately);
}

void CGUIRangesControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);

  m_guiBackground->DynamicResourceAlloc(bOnOff);
  m_guiOverlay->DynamicResourceAlloc(bOnOff);

  for (auto& range : m_ranges)
    range.DynamicResourceAlloc(bOnOff);
}

void CGUIRangesControl::AllocResources()
{
  CGUIControl::AllocResources();

  m_guiBackground->AllocResources();
  m_guiOverlay->AllocResources();

  m_guiBackground->SetHeight(25);
  m_guiOverlay->SetHeight(20);

  for (auto& range : m_ranges)
    range.AllocResources();
}

void CGUIRangesControl::SetInvalid()
{
  CGUIControl::SetInvalid();

  m_guiBackground->SetInvalid();
  m_guiOverlay->SetInvalid();

  for (auto& range : m_ranges)
    range.SetInvalid();
}

bool CGUIRangesControl::UpdateColors(const CGUIListItem* item)
{
  bool bChanged = CGUIControl::UpdateColors(nullptr);

  bChanged |= m_guiBackground->SetDiffuseColor(m_diffuseColor);
  bChanged |= m_guiOverlay->SetDiffuseColor(m_diffuseColor);

  for (auto& range : m_ranges)
    bChanged |= range.SetDiffuseColor(m_diffuseColor);

  return bChanged;
}

bool CGUIRangesControl::UpdateLayout()
{
  bool bChanged = false;

  if (m_width == 0)
    m_width = m_guiBackground->GetTextureWidth();

  if (m_height == 0)
    m_height = m_guiBackground->GetTextureHeight();

  bChanged |= m_guiBackground->SetHeight(m_height);
  bChanged |= m_guiBackground->SetWidth(m_width);

  float fScaleX =
      m_guiBackground->GetTextureWidth() ? m_width / m_guiBackground->GetTextureWidth() : 1.0f;
  float fScaleY =
      m_guiBackground->GetTextureHeight() ? m_height / m_guiBackground->GetTextureHeight() : 1.0f;

  float posX = m_guiBackground->GetXPosition();
  float posY = m_guiBackground->GetYPosition();

  for (auto& range : m_ranges)
    bChanged |= range.UpdateLayout(m_guiBackground->GetTextureHeight(), posX, posY, m_width,
                                   fScaleX, fScaleY);

  float offset = std::fabs(
      fScaleY * 0.5f * (m_guiOverlay->GetTextureHeight() - m_guiBackground->GetTextureHeight()));
  if (offset > 0)  //  Center texture to the background if necessary
    bChanged |= m_guiOverlay->SetPosition(m_guiBackground->GetXPosition(),
                                          m_guiBackground->GetYPosition() + offset);
  else
    bChanged |=
        m_guiOverlay->SetPosition(m_guiBackground->GetXPosition(), m_guiBackground->GetYPosition());

  bChanged |= m_guiOverlay->SetHeight(fScaleY * m_guiOverlay->GetTextureHeight());
  bChanged |= m_guiOverlay->SetWidth(fScaleX * m_guiOverlay->GetTextureWidth());

  return bChanged;
}

void CGUIRangesControl::UpdateInfo(const CGUIListItem* item /* = nullptr */)
{
  if (!IsDisabled() && m_iInfoCode)
  {
    const std::string value = CServiceBroker::GetGUI()->GetInfoManager().GetLabel(m_iInfoCode, m_parentID);
    if (value != m_prevRanges)
    {
      std::vector<std::pair<float, float>> ranges;

      // Parse csv string into ranges...
      const std::vector<std::string> values = StringUtils::Split(value, ',');

      // we must have an even number of values
      if (values.size() % 2 == 0)
      {
        for (auto it = values.begin(); it != values.end();)
        {
          float first = std::stof(*it, nullptr);
          ++it;
          float second = std::stof(*it, nullptr);
          ++it;

          if (first <= second)
            ranges.emplace_back(first, second);
          else
            CLog::Log(LOGERROR, "CGUIRangesControl::UpdateInfo - malformed ranges csv string (end element must be larger or equal than start element)");
        }
      }
      else
        CLog::Log(LOGERROR, "CGUIRangesControl::UpdateInfo - malformed ranges csv string (string must contain even number of elements)");

      SetRanges(ranges);
      m_prevRanges = value;
    }
  }
}
