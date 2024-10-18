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
    m_textureCurrent(CGUITexture::CreateTexture(posX, posY, width, height, texture)),
    m_textureNext(CGUITexture::CreateTexture(posX, posY, width, height, texture))
{
  m_crossFadeTime = 0;
  m_currentFadeTime = 0;
  m_lastRenderTime = 0;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_textureNext->SetFileName("");
}

CGUIImage::CGUIImage(const CGUIImage& left)
  : CGUIControl(left),
    m_image(left.m_image),
    m_info(left.m_info),
    m_textureCurrent(left.m_textureCurrent->Clone()),
    m_textureNext(left.m_textureNext->Clone()),
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
  if (m_textureCurrent->SetDiffuseColor(m_diffuseColor, item))
    MarkDirtyRegion();
  m_textureNext->SetDiffuseColor(m_diffuseColor, item);
}

void CGUIImage::UpdateInfo(const CGUIListItem *item)
{
  // The texture may also depend on info conditions. Update the diffuse color in that case.
  if (m_textureCurrent->GetDiffuseColor().HasInfo())
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
  if (!IsVisible() && m_visible != DELAYED && m_bDynamicResourceAlloc)
  {
    FreeResourcesButNotAnims();
    return;
  }

  // either visible or delayed - we need the resources allocated in either case
  if (!m_textureCurrent->IsAllocated())
    AllocResources();
}

void CGUIImage::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  ProcessState();

  ProcessAllocation();

  if (!m_isTransitioning ||
      (!m_textureNext->ReadyToRender() && !m_textureNext->GetFileName().empty()))
    ProcessNoTransition(currentTime);
  else if (!m_crossFadeTime)
    ProcessInstantTransition(currentTime);
  else
    ProcessFadingTransition(currentTime);

  if (!m_textureCurrent->GetDiffuseColor().HasInfo())
    UpdateDiffuseColor(nullptr);

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIImage::ProcessState()
{
  if (!m_hasNewStagingTexture)
    return;

  std::string fileName = m_nameStaging;
  if (fileName.empty())
    fileName = GetFallback(fileName);

  if (m_nameCurrent == fileName || m_textureCurrent->GetFileName() == fileName)
  {
    // the current texture might be a fallback from a image which failed to
    // load, and it might be the texture we want.
    if (m_textureCurrent->GetFileName() == fileName && m_nameCurrent != fileName)
      m_nameCurrent = fileName;

    if (m_isTransitioning)
    {
      if (m_textureNext->ReadyToRender() || m_textureNext->GetFileName().empty())
      {
        // if the current texture (which is fading out) is our desired texture,
        // reverse the animation.
        std::swap(m_textureCurrent, m_textureNext);
        std::swap(m_nameCurrent, m_nameNext);
        m_currentFadeTime = m_crossFadeTime - m_currentFadeTime;
      }
      else
      {
        // if we are about to fade but the new texture is not ready, we want to
        // keep the current texture, and cancel the new texture.
        m_isTransitioning = false;
        m_textureNext->SetFileName("");
        m_nameNext = "";
      }
    }

    m_hasNewStagingTexture = false;
    return;
  }

  // our fading-in texture is already set
  if (m_nameNext == fileName || m_textureNext->GetFileName() == fileName)
  {
    // the next texture might be a fallback from a image which failed to load,
    // and it might be the texture we want.
    if (m_textureNext->GetFileName() == fileName && m_nameNext != fileName)
      m_nameNext = fileName;

    // ensure that the transition is on its way.
    if (!m_isTransitioning &&
        (m_textureNext->ReadyToRender() || m_textureNext->GetFileName().empty()))
      m_isTransitioning = true;

    m_hasNewStagingTexture = false;
    return;
  }

  // can't set new texture during animation.
  if (m_isTransitioning && (m_textureNext->ReadyToRender() || m_textureNext->GetFileName().empty()))
    return;

  // finally, we can request a new image.
  m_textureNext->SetFileName(fileName);
  m_nameNext = fileName;
  m_isTransitioning = true;
  m_hasNewStagingTexture = false;
}

void CGUIImage::ProcessAllocation()
{
  m_textureCurrent->AllocResources();
  m_textureNext->AllocResources();

  if (m_isTransitioning && m_textureNext->FailedToAlloc())
  {
    if (m_textureNext->GetFileName() != m_info.GetFallback())
      m_textureNext->SetFileName(GetFallback(m_nameNext));
    else
      m_textureNext->SetFileName("");

    m_textureNext->AllocResources();
  }

  if (m_textureCurrent->FailedToAlloc())
  {
    if (m_textureCurrent->GetFileName() != m_info.GetFallback())
      m_textureCurrent->SetFileName(GetFallback(m_nameCurrent));
    else
      m_textureCurrent->SetFileName("");

    m_textureCurrent->AllocResources();
  }
}

void CGUIImage::ProcessNoTransition(unsigned int currentTime)
{
  if (m_textureCurrent->Process(currentTime))
    MarkDirtyRegion();
}

void CGUIImage::ProcessInstantTransition(unsigned int currentTime)
{
  std::swap(m_textureCurrent, m_textureNext);
  std::swap(m_nameCurrent, m_nameNext);

  m_nameNext = "";
  m_textureNext->SetFileName("");

  m_textureCurrent->Process(currentTime);

  m_isTransitioning = false;

  MarkDirtyRegion();
}

void CGUIImage::ProcessFadingTransition(unsigned int currentTime)
{
  unsigned int frameTime = 0;
  if (m_lastRenderTime)
    frameTime = currentTime - m_lastRenderTime;
  if (!frameTime)
    frameTime = (unsigned int)(1000 / CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS());
  m_lastRenderTime = currentTime;

  m_currentFadeTime += frameTime;
  if (m_currentFadeTime > m_crossFadeTime ||
      frameTime == 0) // for if we allocate straight away on creation
    m_currentFadeTime = m_crossFadeTime;

  if (m_currentFadeTime < m_crossFadeTime)
  {
    m_textureCurrent->SetAlpha(GetFadeLevel(m_crossFadeTime - m_currentFadeTime));

    m_textureNext->SetAlpha(GetFadeLevel(m_currentFadeTime));
    m_textureNext->Process(currentTime);
  }
  else
  {
    std::swap(m_textureCurrent, m_textureNext);
    std::swap(m_nameCurrent, m_nameNext);

    m_textureCurrent->SetAlpha(0xff);

    m_nameNext = "";
    m_textureNext->SetFileName("");

    m_currentFadeTime = 0;
    m_lastRenderTime = 0;
    m_isTransitioning = false;
  }

  m_textureCurrent->Process(currentTime);

  MarkDirtyRegion();
}

void CGUIImage::Render()
{
  if (!IsVisible())
    return;

  if (m_isTransitioning)
    m_textureNext->Render();

  m_textureCurrent->Render();

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
  if (m_textureCurrent->GetFileName().empty())
    return;

  CGUIControl::AllocResources();
  m_textureCurrent->AllocResources();
}

void CGUIImage::FreeTextures(bool immediately /* = false */)
{
  m_textureNext->FreeResources(immediately);
  m_textureNext->SetFileName("");
  m_nameNext = "";

  m_textureCurrent->FreeResources(immediately);
  if (!m_info.IsConstant()) // constant textures never change
  {
    m_textureCurrent->SetFileName("");
    m_nameCurrent = "";
  }

  m_isTransitioning = false;
  m_lastRenderTime = 0;
  m_currentFadeTime = 0;
}

void CGUIImage::FreeResources(bool immediately)
{
  FreeTextures(immediately);
  CGUIControl::FreeResources(immediately);
}

void CGUIImage::SetInvalid()
{
  m_textureCurrent->SetInvalid();
  m_textureNext->SetInvalid();
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
  m_textureCurrent->DynamicResourceAlloc(bOnOff);
  m_textureNext->DynamicResourceAlloc(bOnOff);
  CGUIControl::DynamicResourceAlloc(bOnOff);
}

bool CGUIImage::CanFocus() const
{
  return false;
}

float CGUIImage::GetTextureWidth() const
{
  return m_textureCurrent->GetTextureWidth();
}

float CGUIImage::GetTextureHeight() const
{
  return m_textureCurrent->GetTextureHeight();
}

CRect CGUIImage::CalcRenderRegion() const
{
  CRect region = m_textureCurrent->GetRenderRect();

  if (m_isTransitioning && m_textureNext->ReadyToRender())
    region.Union(m_textureNext->GetRenderRect());

  return CGUIControl::CalcRenderRegion().Intersect(region);
}

const std::string &CGUIImage::GetFileName() const
{
  return m_textureCurrent->GetFileName();
}

void CGUIImage::SetAspectRatio(const CAspectRatio &aspect)
{
  m_textureCurrent->SetAspectRatio(aspect);
  m_textureNext->SetAspectRatio(aspect);
}

void CGUIImage::SetCrossFade(unsigned int time)
{
  m_crossFadeTime = time;
}

void CGUIImage::SetFileName(const std::string& strFileName, bool setConstant, const bool useCache)
{
  if (setConstant)
    m_info.SetLabel(strFileName, "", GetParentID());
  m_nameStaging = strFileName;
  m_hasNewStagingTexture = true;
}

#ifdef _DEBUG
void CGUIImage::DumpTextureUse()
{
  if (m_textureCurrent->IsAllocated())
  {
    if (GetID())
      CLog::Log(LOGDEBUG, "Image control {} using texture {}", GetID(),
                m_textureCurrent->GetFileName());
    else
      CLog::Log(LOGDEBUG, "Using texture {}", m_textureCurrent->GetFileName());
  }
}
#endif

void CGUIImage::SetWidth(float width)
{
  m_textureCurrent->SetWidth(width);
  m_textureNext->SetWidth(width);
  CGUIControl::SetWidth(m_textureCurrent->GetWidth());
}

void CGUIImage::SetHeight(float height)
{
  m_textureCurrent->SetHeight(height);
  m_textureNext->SetHeight(height);
  CGUIControl::SetHeight(m_textureCurrent->GetHeight());
}

void CGUIImage::SetPosition(float posX, float posY)
{
  m_textureCurrent->SetPosition(posX, posY);
  m_textureNext->SetPosition(posX, posY);
  CGUIControl::SetPosition(posX, posY);
}

void CGUIImage::SetInfo(const GUIINFO::CGUIInfoLabel &info)
{
  m_info = info;
  // a constant image never needs updating
  if (m_info.IsConstant())
  {
    m_textureCurrent->SetFileName(m_info.GetLabel(0));
    m_nameCurrent = m_info.GetLabel(0);
  }
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

std::string CGUIImage::GetFallback(const std::string& currentName)
{
  if (!m_currentFallback.empty() && currentName != m_currentFallback)
    return m_currentFallback;
  else
    return m_info.GetFallback();
}

std::string CGUIImage::GetDescription(void) const
{
  return GetFileName();
}

