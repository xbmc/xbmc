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

CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& texture)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_texture(posX, posY, width, height, texture)
{
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
}

CGUIImage::CGUIImage(const CGUIImage &left)
    : CGUIControl(left), m_texture(left.m_texture)
{
  m_info = left.m_info;
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
  if (m_info.IsConstant())
    return; // nothing to do

  // don't allow image to change while animating out
  if (HasRendered() && IsAnimating(ANIM_TYPE_HIDDEN) && !IsVisibleFromSkin())
    return;

  if (item)
    SetFileName(m_info.GetItemLabel(item, true));
  else
    SetFileName(m_info.GetLabel(m_dwParentID, true));
}

void CGUIImage::AllocateOnDemand()
{
  // if we're hidden, we can free our resources and return
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && m_texture.IsAllocated())
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

  m_texture.SetDiffuseColor(m_diffuseColor);
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
    if (!m_info.IsConstant())
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
  m_bDynamicResourceAlloc = bOnOff;
  m_texture.DynamicResourceAlloc(bOnOff);
  CGUIControl::DynamicResourceAlloc(bOnOff);
}

bool CGUIImage::CanFocus() const
{
  return false;
}

float CGUIImage::GetTextureWidth() const
{
  return m_texture.GetTextureWidth();
}

float CGUIImage::GetTextureHeight() const
{
  return m_texture.GetTextureHeight();
}

const CStdString &CGUIImage::GetFileName() const
{
  return m_texture.GetFileName();
}

void CGUIImage::SetAspectRatio(const CAspectRatio &aspect)
{
  m_texture.SetAspectRatio(aspect);
}

void CGUIImage::SetFileName(const CStdString& strFileName, bool setConstant)
{
  if (setConstant)
    m_info.SetLabel(strFileName, "");
  m_texture.SetFileName(strFileName);
}

void CGUIImage::SetAlpha(unsigned char alpha)
{
  m_texture.SetAlpha(alpha);
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

void CGUIImage::SetInfo(const CGUIInfoLabel &info)
{
  m_info = info;
  // a constant image never needs updating
  if (m_info.IsConstant())
    m_texture.SetFileName(m_info.GetLabel(0));
}