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

#include "include.h"
#include "GUIImage.h"
#include "TextureManager.h"

using namespace std;

CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, bool largeTexture)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_texture(posX, posY, width, height, texture.GetInfo(largeTexture)) // FIXME
{
  m_image = texture;
  // a constant image never needs updating
  if (m_image.file.IsConstant())
    m_texture.SetFileName(m_image.file.GetLabel(0));

  ControlType = GUICONTROL_IMAGE;
}

CGUIImage::CGUIImage(const CGUIImage &left)
    : CGUIControl(left), m_texture(left.m_texture)
{
  m_aspect = left.m_aspect;
  m_image = left.m_image;
  // defaults
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_texturesAllocated = false;
}

CGUIImage::~CGUIImage(void)
{

}

void CGUIImage::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  // check for conditional information before we free and
  // alloc as this does free and allocation as well
  if (!m_pushedUpdates)
    UpdateInfo(item);

  AllocateOnDemand();
}

void CGUIImage::UpdateInfo(const CGUIListItem *item)
{
  if (m_image.file.IsConstant())
    return; // nothing to do

  // don't allow image to change while animating out
  if (HasRendered() && IsAnimating(ANIM_TYPE_HIDDEN) && !IsVisibleFromSkin())
    return;

  if (item)
    SetFileName(m_image.file.GetItemLabel(item, true));
  else
    SetFileName(m_image.file.GetLabel(m_dwParentID, true));
}

void CGUIImage::AllocateOnDemand()
{
  // if we're hidden, we can free our resources and return
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && IsAllocated())
      FreeResourcesButNotAnims();
    return;
  }

  // either visible or delayed - we need the resources allocated in either case
  if (!m_texture.IsAllocated())
    AllocResources();
}

void CGUIImage::Render()
{
  // we need the checks for visibility and resource allocation here as
  // most controls use CGUIImage's to do their rendering (where UpdateVisibility doesn't apply)
  AllocateOnDemand();
  
  if (!IsVisible()) return;

  if (m_bInvalidated)
    CalculateSize();

  // see if we need to clip the image
  if (m_fNW > m_width || m_fNH > m_height)
  {
    if (g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
    {
      m_texture.Render();
      g_graphicsContext.RestoreClipRegion();
    }
  }
  else
    m_texture.Render();
  CGUIControl::Render();
}

bool CGUIImage::OnAction(const CAction &action)
{
  return false;
}

bool CGUIImage::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_THUMBS)
  {
    if (!m_image.file.IsConstant())
      FreeTextures(true); // true as we want to free the texture immediately
    return true;
  }
  return CGUIControl::OnMessage(message);
}

void CGUIImage::PreAllocResources()
{
  FreeResources();
  m_texture.PreAllocResources();
}

void CGUIImage::AllocResources()
{
  if (m_texture.GetFileName().IsEmpty())
    return;
  FreeTextures();
  CGUIControl::AllocResources();

  m_texture.AllocResources();
  m_texturesAllocated = true;

  CalculateSize();

  if (!m_aspect.scaleDiffuse) // stretch'ing diffuse
  { // scale diffuse up or down to match output rect size, rather than image size
    //(m_fX, mfY) -> (m_fX + m_fNW, m_fY + m_fNH)
    //(0,0) -> (m_fU*m_diffuseScaleU, m_fV*m_diffuseScaleV)
    // x = u/(m_fU*m_diffuseScaleU)*m_fNW + m_fX
    // -> u = (m_posX - m_fX) * m_fU * m_diffuseScaleU / m_fNW
    float scaleU = m_fNW / m_width;
    float scaleV = m_fNH / m_height;
    float offsetU = (m_fX - m_posX) / m_fNW;
    float offsetV = (m_fY - m_posY) / m_fNH;
    m_texture.SetDiffuseScaling(scaleU, scaleV, offsetU, offsetV);
  }
}

void CGUIImage::FreeTextures(bool immediately /* = false */)
{
  m_texture.FreeResources();
  m_texturesAllocated = false;
}

void CGUIImage::FreeResources()
{
  FreeTextures();
  CGUIControl::FreeResources();
}

// WORKAROUND - we are currently resetting all animations when this is called, which shouldn't be the case
//              see CGUIControl::FreeResources() - this needs remedying.
void CGUIImage::FreeResourcesButNotAnims()
{
  FreeTextures();
  m_bAllocated=false;
  m_hasRendered = false;
}

void CGUIImage::DynamicResourceAlloc(bool bOnOff)
{
  m_texture.DynamicResourceAlloc(bOnOff);
  CGUIControl::DynamicResourceAlloc(bOnOff);
}

void CGUIImage::CalculateSize()
{
  m_fX = m_posX;
  m_fY = m_posY;

  if (m_width == 0)
    m_width = (float)m_texture.GetTextureWidth();
  if (m_height == 0)
    m_height = (float)m_texture.GetTextureHeight();

  m_fNW = m_width;
  m_fNH = m_height;

  if (m_aspect.ratio != CAspectRatio::AR_STRETCH && m_texture.GetTextureWidth() && m_texture.GetTextureHeight())
  {
    // to get the pixel ratio, we must use the SCALED output sizes
    float pixelRatio = g_graphicsContext.GetScalingPixelRatio();

    float fSourceFrameRatio = ((float)m_texture.GetTextureWidth()) / ((float)m_texture.GetTextureHeight());
    if (m_texture.GetOrientation() & 4)
      fSourceFrameRatio = ((float)m_texture.GetTextureHeight()) / ((float)m_texture.GetTextureWidth());
    float fOutputFrameRatio = fSourceFrameRatio / pixelRatio;

    // maximize the thumbnails width
    m_fNW = m_width;
    m_fNH = m_fNW / fOutputFrameRatio;

    if ((m_aspect.ratio == CAspectRatio::AR_SCALE && m_fNH < m_height) ||
        (m_aspect.ratio == CAspectRatio::AR_KEEP && m_fNH > m_height))
    {
      m_fNH = m_height;
      m_fNW = m_fNH * fOutputFrameRatio;
    }
    if (m_aspect.ratio == CAspectRatio::AR_CENTER)
    { // keep original size + center
      m_fNW = (float)m_texture.GetTextureWidth();
      m_fNH = (float)m_texture.GetTextureHeight();
    }

    // calculate placement
    if (m_aspect.align & ASPECT_ALIGN_LEFT)
      m_fX = m_posX;
    else if (m_aspect.align & ASPECT_ALIGN_RIGHT)
      m_fX = m_posX + m_width - m_fNW;
    else
      m_fX = m_posX + (m_width - m_fNW) * 0.5f;
    if (m_aspect.align & ASPECT_ALIGNY_TOP)
      m_fY = m_posY;
    else if (m_aspect.align & ASPECT_ALIGNY_BOTTOM)
      m_fY = m_posY + m_height - m_fNH;
    else
      m_fY = m_posY + (m_height - m_fNH) * 0.5f;

    // and update our texture size
    m_texture.SetPosition(m_fX, m_fY);
    m_texture.SetWidth(m_fNW);
    m_texture.SetHeight(m_fNH);
  }
}

bool CGUIImage::CanFocus() const
{
  return false;
}

int CGUIImage::GetTextureWidth() const
{
  return m_texture.GetTextureWidth();
}

int CGUIImage::GetTextureHeight() const
{
  return m_texture.GetTextureHeight();
}

const CStdString &CGUIImage::GetFileName() const
{
  return m_texture.GetFileName();
}

void CGUIImage::SetAspectRatio(const CAspectRatio &aspect)
{
  if (m_aspect != aspect)
  {
    m_aspect = aspect;
    SetInvalid();
  }
}

void CGUIImage::SetFileName(const CStdString& strFileName, bool setConstant)
{
  if (setConstant)
    m_image.file.SetLabel(strFileName, "");
  m_texture.SetFileName(strFileName);
}

void CGUIImage::SetAlpha(unsigned char alpha)
{
  m_texture.SetAlpha(alpha);
}

bool CGUIImage::IsAllocated() const
{
  if (!m_texturesAllocated) return false;
  return CGUIControl::IsAllocated();
}

#ifdef _DEBUG
void CGUIImage::DumpTextureUse()
{
  if (m_texture.IsAllocated())
  {
    if (GetID())
      CLog::Log(LOGDEBUG, "Image control %u using texture %s",
                GetID(), m_texture.GetFileName().c_str());
    else
      CLog::Log(LOGDEBUG, "Using texture %s", m_texture.GetFileName().c_str());
  }
}
#endif

void CGUIImage::SetWidth(float width)
{
  m_texture.SetWidth(width);
  CGUIControl::SetWidth(m_texture.GetWidth());
}

void CGUIImage::SetHeight(float height)
{
  m_texture.SetHeight(height);
  CGUIControl::SetHeight(m_texture.GetHeight());
}

void CGUIImage::SetPosition(float posX, float posY)
{
  m_texture.SetPosition(posX, posY);
  CGUIControl::SetPosition(posX, posY);
}