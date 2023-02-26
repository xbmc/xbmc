/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIImage.h"

#include "GUIMessage.h"
#include "utils/log.h"

#include <cassert>

using namespace KODI::GUILIB;

CGUIImage::CGUIImage(int parentID,
                     int controlID,
                     float posX,
                     float posY,
                     float width,
                     float height,
                     const CTextureInfo& texture)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_texture(CGUITexture::CreateTexture(posX, posY, width, height, texture))
{
  m_crossFadeTime = 0;
  m_currentFadeTime = 0;
  m_lastRenderTime = 0;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
}

CGUIImage::CGUIImage(const CGUIImage& left)
  : CGUIControl(left),
    m_image(left.m_image),
    m_info(left.m_info),
    m_texture(left.m_texture->Clone()),
    m_fadingTextures(),
    m_currentTexture(),
    m_currentFallback()
{
  m_crossFadeTime = left.m_crossFadeTime;
  // defaults
  m_currentFadeTime = 0;
  m_lastRenderTime = 0;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
}

CGUIImage::~CGUIImage(void) = default;

void CGUIImage::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  // now that we've checked for conditional info, we can
  // check for allocation
  AllocateOnDemand();
}

void CGUIImage::UpdateDiffuseColor(const CGUIListItem* item)
{
  if (m_texture->SetDiffuseColor(m_diffuseColor, item))
  {
    MarkDirtyRegion();
  }
}

void CGUIImage::UpdateInfo(const CGUIListItem *item)
{
  // The texture may also depend on info conditions. Update the diffuse color in that case.
  if (m_texture->GetDiffuseColor().HasInfo())
    UpdateDiffuseColor(item);

  if (m_info.IsConstant())
    return; // nothing to do

  // don't allow image to change while animating out
  if (HasProcessed() && IsAnimating(ANIM_TYPE_HIDDEN) && !IsVisibleFromSkin())
    return;

  if (item)
    SetFileName(m_info.GetItemLabel(item, true, &m_currentFallback));
  else
    SetFileName(m_info.GetLabel(m_parentID, true, &m_currentFallback));
}

void CGUIImage::AllocateOnDemand()
{
  // if we're hidden, we can free our resources and return
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && m_texture->IsAllocated())
      FreeResourcesButNotAnims();
    return;
  }

  // either visible or delayed - we need the resources allocated in either case
  if (!m_texture->IsAllocated())
    AllocResources();
}

void CGUIImage::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // check whether our image failed to allocate, and if so drop back to the fallback image
  if (m_texture->FailedToAlloc() && m_texture->GetFileName() != m_info.GetFallback())
  {
    if (!m_currentFallback.empty() && m_texture->GetFileName() != m_currentFallback)
      m_texture->SetFileName(m_currentFallback);
    else
      m_texture->SetFileName(m_info.GetFallback());
  }

  if (m_crossFadeTime)
  {
    // make sure our texture has started allocating
    if (m_texture->AllocResources())
      MarkDirtyRegion();

    // compute the frame time
    unsigned int frameTime = 0;
    if (m_lastRenderTime)
      frameTime = currentTime - m_lastRenderTime;
    if (!frameTime)
      frameTime = (unsigned int)(1000 / CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS());
    m_lastRenderTime = currentTime;

    if (m_fadingTextures.size())  // have some fading images
    { // anything other than the last old texture needs to be faded out as per usual
      for (std::vector<CFadingTexture *>::iterator i = m_fadingTextures.begin(); i != m_fadingTextures.end() - 1;)
      {
        if (!ProcessFading(*i, frameTime, currentTime))
          i = m_fadingTextures.erase(i);
        else
          ++i;
      }

      if (m_texture->ReadyToRender() || m_texture->GetFileName().empty())
      { // fade out the last one as well
        if (!ProcessFading(m_fadingTextures[m_fadingTextures.size() - 1], frameTime, currentTime))
          m_fadingTextures.erase(m_fadingTextures.end() - 1);
      }
      else
      { // keep the last one fading in
        CFadingTexture *texture = m_fadingTextures[m_fadingTextures.size() - 1];
        texture->m_fadeTime += frameTime;
        if (texture->m_fadeTime > m_crossFadeTime)
          texture->m_fadeTime = m_crossFadeTime;

        if (texture->m_texture->SetAlpha(GetFadeLevel(texture->m_fadeTime)))
          MarkDirtyRegion();
        if (texture->m_texture->SetDiffuseColor(m_diffuseColor))
          MarkDirtyRegion();
        if (texture->m_texture->Process(currentTime))
          MarkDirtyRegion();
      }
    }

    if (m_texture->ReadyToRender() || m_texture->GetFileName().empty())
    { // fade the new one in
      m_currentFadeTime += frameTime;
      if (m_currentFadeTime > m_crossFadeTime || frameTime == 0) // for if we allocate straight away on creation
        m_currentFadeTime = m_crossFadeTime;
    }
    if (m_texture->SetAlpha(GetFadeLevel(m_currentFadeTime)))
      MarkDirtyRegion();
  }

  if (!m_texture->GetDiffuseColor().HasInfo())
    UpdateDiffuseColor(nullptr);

  if (m_texture->Process(currentTime))
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIImage::Render()
{
  if (!IsVisible()) return;

  for (auto& itr : m_fadingTextures)
    itr->m_texture->Render();

  m_texture->Render();

  CGUIControl::Render();
}

bool CGUIImage::ProcessFading(CGUIImage::CFadingTexture *texture, unsigned int frameTime, unsigned int currentTime)
{
  assert(texture);
  if (texture->m_fadeTime <= frameTime)
  { // time to kill off the texture
    MarkDirtyRegion();
    delete texture;
    return false;
  }
  // render this texture
  texture->m_fadeTime -= frameTime;

  if (texture->m_texture->SetAlpha(GetFadeLevel(texture->m_fadeTime)))
    MarkDirtyRegion();
  if (texture->m_texture->SetDiffuseColor(m_diffuseColor))
    MarkDirtyRegion();
  if (texture->m_texture->Process(currentTime))
    MarkDirtyRegion();

  return true;
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
  else if (message.GetMessage() == GUI_MSG_SET_FILENAME)
  {
    SetFileName(message.GetLabel());
    return true;
  }
  else if (message.GetMessage() == GUI_MSG_GET_FILENAME)
  {
    message.SetLabel(GetFileName());
    return true;
  }
  return CGUIControl::OnMessage(message);
}

void CGUIImage::AllocResources()
{
  if (m_texture->GetFileName().empty())
    return;

  CGUIControl::AllocResources();
  m_texture->AllocResources();
}

void CGUIImage::FreeTextures(bool immediately /* = false */)
{
  m_texture->FreeResources(immediately);
  for (unsigned int i = 0; i < m_fadingTextures.size(); i++)
    delete m_fadingTextures[i];
  m_fadingTextures.clear();
  m_currentTexture.clear();
  if (!m_info.IsConstant()) // constant textures never change
    m_texture->SetFileName("");
}

void CGUIImage::FreeResources(bool immediately)
{
  FreeTextures(immediately);
  CGUIControl::FreeResources(immediately);
}

void CGUIImage::SetInvalid()
{
  m_texture->SetInvalid();
  CGUIControl::SetInvalid();
}

// WORKAROUND - we are currently resetting all animations when this is called, which shouldn't be the case
//              see CGUIControl::FreeResources() - this needs remedying.
void CGUIImage::FreeResourcesButNotAnims()
{
  FreeTextures();
  m_bAllocated=false;
  m_hasProcessed = false;
}

void CGUIImage::DynamicResourceAlloc(bool bOnOff)
{
  m_bDynamicResourceAlloc = bOnOff;
  m_texture->DynamicResourceAlloc(bOnOff);
  CGUIControl::DynamicResourceAlloc(bOnOff);
}

bool CGUIImage::CanFocus() const
{
  return false;
}

float CGUIImage::GetTextureWidth() const
{
  return m_texture->GetTextureWidth();
}

float CGUIImage::GetTextureHeight() const
{
  return m_texture->GetTextureHeight();
}

CRect CGUIImage::CalcRenderRegion() const
{
  CRect region = m_texture->GetRenderRect();

  for (const auto& itr : m_fadingTextures)
    region.Union(itr->m_texture->GetRenderRect());

  return CGUIControl::CalcRenderRegion().Intersect(region);
}

const std::string &CGUIImage::GetFileName() const
{
  return m_texture->GetFileName();
}

void CGUIImage::SetAspectRatio(const CAspectRatio &aspect)
{
  m_texture->SetAspectRatio(aspect);
}

void CGUIImage::SetCrossFade(unsigned int time)
{
  m_crossFadeTime = time;
  if (!m_crossFadeTime && m_texture->IsLazyLoaded() && !m_info.GetFallback().empty())
    m_crossFadeTime = 1;
}

void CGUIImage::SetFileName(const std::string& strFileName, bool setConstant, const bool useCache)
{
  if (setConstant)
    m_info.SetLabel(strFileName, "", GetParentID());

  // Set whether or not to use cache
  m_texture->SetUseCache(useCache);

  if (m_crossFadeTime)
  {
    // set filename on the next texture
    if (m_currentTexture == strFileName)
      return; // nothing to do - we already have this image

    if (m_texture->ReadyToRender() || m_texture->GetFileName().empty())
    { // save the current image
      m_fadingTextures.push_back(new CFadingTexture(m_texture.get(), m_currentFadeTime));
      MarkDirtyRegion();
    }
    m_currentFadeTime = 0;
  }
  if (m_currentTexture != strFileName)
  { // texture is changing - attempt to load it, and save the name in m_currentTexture.
    // we'll check whether it loaded or not in Render()
    m_currentTexture = strFileName;
    if (m_texture->SetFileName(m_currentTexture))
      MarkDirtyRegion();
  }
}

#ifdef _DEBUG
void CGUIImage::DumpTextureUse()
{
  if (m_texture->IsAllocated())
  {
    if (GetID())
      CLog::Log(LOGDEBUG, "Image control {} using texture {}", GetID(), m_texture->GetFileName());
    else
      CLog::Log(LOGDEBUG, "Using texture {}", m_texture->GetFileName());
  }
}
#endif

void CGUIImage::SetWidth(float width)
{
  m_texture->SetWidth(width);
  CGUIControl::SetWidth(m_texture->GetWidth());
}

void CGUIImage::SetHeight(float height)
{
  m_texture->SetHeight(height);
  CGUIControl::SetHeight(m_texture->GetHeight());
}

void CGUIImage::SetPosition(float posX, float posY)
{
  m_texture->SetPosition(posX, posY);
  CGUIControl::SetPosition(posX, posY);
}

void CGUIImage::SetInfo(const GUIINFO::CGUIInfoLabel &info)
{
  m_info = info;
  // a constant image never needs updating
  if (m_info.IsConstant())
    m_texture->SetFileName(m_info.GetLabel(0));
}

unsigned char CGUIImage::GetFadeLevel(unsigned int time) const
{
  float amount = (float)time / m_crossFadeTime;
  // we want a semi-transparent image, so we need to use a more complicated
  // fade technique.  Assuming a black background (not generally true, but still...)
  // we have
  // b(t) = [a - b(1-t)*a] / a*(1-b(1-t)*a),
  // where a = alpha, and b(t):[0,1] -> [0,1] is the blend function.
  // solving, we get
  // b(t) = [1 - (1-a)^t] / a
  const float alpha = 0.7f;
  return (unsigned char)(255.0f * (1 - pow(1-alpha, amount))/alpha);
}

std::string CGUIImage::GetDescription(void) const
{
  return GetFileName();
}

