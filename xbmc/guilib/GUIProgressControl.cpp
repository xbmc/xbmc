/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIProgressControl.h"
#include "GUIInfoManager.h"
#include "GUIListItem.h"
#include "GUIWindowManager.h"
#include "FileItem.h"

CGUIProgressControl::CGUIProgressControl(int parentID, int controlID,
                                         float posX, float posY, float width,
                                         float height, const CTextureInfo& backGroundTexture,
                                         const CTextureInfo& leftTexture,
                                         const CTextureInfo& midTexture,
                                         const CTextureInfo& rightTexture,
                                         const CTextureInfo& overlayTexture,
                                         bool reveal)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_guiBackground(posX, posY, width, height, backGroundTexture)
    , m_guiLeft(posX, posY, width, height, leftTexture)
    , m_guiMid(posX, posY, width, height, midTexture)
    , m_guiRight(posX, posY, width, height, rightTexture)
    , m_guiOverlay(posX, posY, width, height, overlayTexture)
{
  m_fPercent = 0;
  m_iInfoCode = 0;
  ControlType = GUICONTROL_PROGRESS;
  m_bReveal = reveal;
  m_bChanged = false;
}

CGUIProgressControl::~CGUIProgressControl(void)
{
}

void CGUIProgressControl::SetPosition(float posX, float posY)
{
  // everything is positioned based on the background image position
  CGUIControl::SetPosition(posX, posY);
  m_guiBackground.SetPosition(posX, posY);
}

void CGUIProgressControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool changed = false;

  if (!IsDisabled())
    changed |= UpdateLayout();
  changed |= m_guiBackground.Process(currentTime);
  changed |= m_guiMid.Process(currentTime);
  changed |= m_guiLeft.Process(currentTime);
  changed |= m_guiRight.Process(currentTime);
  changed |= m_guiOverlay.Process(currentTime);

  if (changed)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIProgressControl::Render()
{
  if (!IsDisabled())
  {
    m_guiBackground.Render();

    if (m_guiLeft.GetFileName().IsEmpty() && m_guiRight.GetFileName().IsEmpty())
    {
      if (m_bReveal && !m_guiMidClipRect.IsEmpty())
      {
        bool restore = g_graphicsContext.SetClipRegion(m_guiMidClipRect.x1, m_guiMidClipRect.y1, m_guiMidClipRect.Width(), m_guiMidClipRect.Height());
        m_guiMid.Render();
        if (restore)
          g_graphicsContext.RestoreClipRegion();
      }
      else if (!m_bReveal && m_guiMid.GetWidth() > 0)
        m_guiMid.Render();
    }
    else
    {
      m_guiLeft.Render();

      if (m_bReveal && !m_guiMidClipRect.IsEmpty())
      {
        bool restore = g_graphicsContext.SetClipRegion(m_guiMidClipRect.x1, m_guiMidClipRect.y1, m_guiMidClipRect.Width(), m_guiMidClipRect.Height());
        m_guiMid.Render();
        if (restore)
          g_graphicsContext.RestoreClipRegion();
      }
      else if (!m_bReveal && m_guiMid.GetWidth() > 0)
        m_guiMid.Render();

      m_guiRight.Render();
    }

    m_guiOverlay.Render();
  }

  CGUIControl::Render();
}


bool CGUIProgressControl::CanFocus() const
{
  return false;
}


bool CGUIProgressControl::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}

void CGUIProgressControl::SetPercentage(float fPercent)
{
  if (m_fPercent != fPercent)
    SetInvalid();
  m_fPercent = fPercent;
}

float CGUIProgressControl::GetPercentage() const
{
  return m_fPercent;
}
void CGUIProgressControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_guiBackground.FreeResources(immediately);
  m_guiMid.FreeResources(immediately);
  m_guiRight.FreeResources(immediately);
  m_guiLeft.FreeResources(immediately);
  m_guiOverlay.FreeResources(immediately);
}

void CGUIProgressControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_guiBackground.DynamicResourceAlloc(bOnOff);
  m_guiMid.DynamicResourceAlloc(bOnOff);
  m_guiRight.DynamicResourceAlloc(bOnOff);
  m_guiLeft.DynamicResourceAlloc(bOnOff);
  m_guiOverlay.DynamicResourceAlloc(bOnOff);
}

void CGUIProgressControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground.AllocResources();
  m_guiMid.AllocResources();
  m_guiRight.AllocResources();
  m_guiLeft.AllocResources();
  m_guiOverlay.AllocResources();

  m_guiBackground.SetHeight(25);
  m_guiRight.SetHeight(20);
  m_guiLeft.SetHeight(20);
  m_guiMid.SetHeight(20);
  m_guiOverlay.SetHeight(20);
}

void CGUIProgressControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_guiBackground.SetInvalid();
  m_guiMid.SetInvalid();
  m_guiRight.SetInvalid();
  m_guiLeft.SetInvalid();
  m_guiOverlay.SetInvalid();
}

void CGUIProgressControl::SetInfo(int iInfo)
{
  m_iInfoCode = iInfo;
}

bool CGUIProgressControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_guiBackground.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiRight.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiLeft.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiMid.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiOverlay.SetDiffuseColor(m_diffuseColor);

  return changed;
}

CStdString CGUIProgressControl::GetDescription() const
{
  CStdString percent;
  percent.Format("%2.f", m_fPercent);
  return percent;
}

bool CGUIProgressControl::UpdateLayout(void)
{
  bool bChanged(false);

  if (m_width == 0)
    m_width = m_guiBackground.GetTextureWidth();
  if (m_height == 0)
    m_height = m_guiBackground.GetTextureHeight();

  bChanged |= m_guiBackground.SetHeight(m_height);
  bChanged |= m_guiBackground.SetWidth(m_width);

  float fScaleX, fScaleY;
  fScaleY = m_guiBackground.GetTextureHeight() ? m_height / m_guiBackground.GetTextureHeight() : 1.0f;
  fScaleX = m_guiBackground.GetTextureWidth() ? m_width / m_guiBackground.GetTextureWidth() : 1.0f;

  float posX = m_guiBackground.GetXPosition();
  float posY = m_guiBackground.GetYPosition();

  if (m_guiLeft.GetFileName().IsEmpty() && m_guiRight.GetFileName().IsEmpty())
  { // rendering without left and right image - fill the mid image completely
    float width = m_fPercent * m_width * 0.01f;
    float offset = fabs(fScaleY * 0.5f * (m_guiMid.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
    if (offset > 0)  //  Center texture to the background if necessary
      bChanged |= m_guiMid.SetPosition(posX, posY + offset);
    else
      bChanged |= m_guiMid.SetPosition(posX, posY);
    bChanged |= m_guiMid.SetHeight(fScaleY * m_guiMid.GetTextureHeight());
    if (m_bReveal)
    {
      bChanged |= m_guiMid.SetWidth(m_width);
      float x = posX, y = posY + offset, w = width, h = fScaleY * m_guiMid.GetTextureHeight();
      m_guiMidClipRect = CRect(x, y, x + w, y + h);
    }
    else
    {
      bChanged |= m_guiMid.SetWidth(width);
      m_guiMidClipRect = CRect();
    }
  }
  else
  {
    float fWidth = m_fPercent;
    float fFullWidth = m_guiBackground.GetTextureWidth() - m_guiLeft.GetTextureWidth() - m_guiRight.GetTextureWidth();
    fWidth /= 100.0f;
    fWidth *= fFullWidth;

    float offset = fabs(fScaleY * 0.5f * (m_guiLeft.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
    if (offset > 0)  //  Center texture to the background if necessary
      bChanged |= m_guiLeft.SetPosition(posX, posY + offset);
    else
      bChanged |= m_guiLeft.SetPosition(posX, posY);
    bChanged |= m_guiLeft.SetHeight(fScaleY * m_guiLeft.GetTextureHeight());
    bChanged |= m_guiLeft.SetWidth(fScaleX * m_guiLeft.GetTextureWidth());

    posX += fScaleX * m_guiLeft.GetTextureWidth();
    offset = fabs(fScaleY * 0.5f * (m_guiMid.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
    if (offset > 0)  //  Center texture to the background if necessary
      bChanged |= m_guiMid.SetPosition(posX, posY + offset);
    else
      bChanged |= m_guiMid.SetPosition(posX, posY);
    bChanged |= m_guiMid.SetHeight(fScaleY * m_guiMid.GetTextureHeight());
    if (m_bReveal)
    {
      bChanged |= m_guiMid.SetWidth(fScaleX * fFullWidth);
      float x = posX, y = posY + offset, w =  fScaleX * fWidth, h = fScaleY * m_guiMid.GetTextureHeight();
      m_guiMidClipRect = CRect(x, y, x + w, y + h);
    }
    else
    {
      bChanged |= m_guiMid.SetWidth(fScaleX * fWidth);
      m_guiMidClipRect = CRect();
    }

    posX += fWidth * fScaleX;

    offset = fabs(fScaleY * 0.5f * (m_guiRight.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
    if (offset > 0)  //  Center texture to the background if necessary
      bChanged |= m_guiRight.SetPosition(posX, posY + offset);
    else
      bChanged |= m_guiRight.SetPosition(posX, posY);
    bChanged |= m_guiRight.SetHeight(fScaleY * m_guiRight.GetTextureHeight());
    bChanged |= m_guiRight.SetWidth(fScaleX * m_guiRight.GetTextureWidth());
  }
  float offset = fabs(fScaleY * 0.5f * (m_guiOverlay.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
  if (offset > 0)  //  Center texture to the background if necessary
    bChanged |= m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(), m_guiBackground.GetYPosition() + offset);
  else
    bChanged |= m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(), m_guiBackground.GetYPosition());
  bChanged |= m_guiOverlay.SetHeight(fScaleY * m_guiOverlay.GetTextureHeight());
  bChanged |= m_guiOverlay.SetWidth(fScaleX * m_guiOverlay.GetTextureWidth());

  return bChanged;
}

void CGUIProgressControl::UpdateInfo(const CGUIListItem *item)
{
  if (!IsDisabled())
  {
    if (m_iInfoCode)
    {
      int value;
      if (g_infoManager.GetInt(value, m_iInfoCode, m_parentID, item))
        m_fPercent = (float)value;

      if (m_fPercent < 0.0f) m_fPercent = 0.0f;
      if (m_fPercent > 100.0f) m_fPercent = 100.0f;
    }
  }
}
