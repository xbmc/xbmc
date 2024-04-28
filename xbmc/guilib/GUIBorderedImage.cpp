/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIBorderedImage.h"

CGUIBorderedImage::CGUIBorderedImage(int parentID,
                                     int controlID,
                                     float posX,
                                     float posY,
                                     float width,
                                     float height,
                                     const CTextureInfo& texture,
                                     const CTextureInfo& borderTexture,
                                     const CRect& borderSize)
  : CGUIImage(parentID,
              controlID,
              posX + borderSize.x1,
              posY + borderSize.y1,
              width - borderSize.x1 - borderSize.x2,
              height - borderSize.y1 - borderSize.y2,
              texture),
    m_borderImage(CGUITexture::CreateTexture(posX, posY, width, height, borderTexture)),
    m_borderSize(borderSize)
{
  ControlType = GUICONTROL_BORDEREDIMAGE;
}

CGUIBorderedImage::CGUIBorderedImage(const CGUIBorderedImage& right)
  : CGUIImage(right), m_borderImage(right.m_borderImage->Clone()), m_borderSize(right.m_borderSize)
{
  ControlType = GUICONTROL_BORDEREDIMAGE;
}

void CGUIBorderedImage::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CGUIImage::Process(currentTime, dirtyregions);
  if (!m_borderImage->GetFileName().empty() && m_texture->ReadyToRender())
  {
    CRect rect = CRect(m_texture->GetXPosition(), m_texture->GetYPosition(),
                       m_texture->GetXPosition() + m_texture->GetWidth(),
                       m_texture->GetYPosition() + m_texture->GetHeight());
    rect.Intersect(m_texture->GetRenderRect());
    m_borderImage->SetPosition(rect.x1 - m_borderSize.x1, rect.y1 - m_borderSize.y1);
    m_borderImage->SetWidth(rect.Width() + m_borderSize.x1 + m_borderSize.x2);
    m_borderImage->SetHeight(rect.Height() + m_borderSize.y1 + m_borderSize.y2);
    m_borderImage->SetDiffuseColor(m_diffuseColor);
    if (m_borderImage->Process(currentTime))
      MarkDirtyRegion();
  }
}

void CGUIBorderedImage::Render()
{
  bool renderFrontToBack = CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
                           RENDER_ORDER_FRONT_TO_BACK;

  if (renderFrontToBack)
    CGUIImage::Render();

  if (!m_borderImage->GetFileName().empty() && m_texture->ReadyToRender())
    m_borderImage->Render(-1);

  if (!renderFrontToBack)
    CGUIImage::Render();
}

CRect CGUIBorderedImage::CalcRenderRegion() const
{
  // have to union the image as well as fading images may still exist that are bigger than our current border image
  return CGUIImage::CalcRenderRegion().Union(m_borderImage->GetRenderRect());
}

void CGUIBorderedImage::AllocResources()
{
  m_borderImage->AllocResources();
  CGUIImage::AllocResources();
}

void CGUIBorderedImage::FreeResources(bool immediately)
{
  m_borderImage->FreeResources(immediately);
  CGUIImage::FreeResources(immediately);
}

void CGUIBorderedImage::DynamicResourceAlloc(bool bOnOff)
{
  m_borderImage->DynamicResourceAlloc(bOnOff);
  CGUIImage::DynamicResourceAlloc(bOnOff);
}
