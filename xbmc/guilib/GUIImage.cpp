/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIImage.h"

#include "FileItem.h"
#include "GUIMessage.h"
#include "ImageSettings.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <cassert>
#include <memory>

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
    m_imageFilterInfo(left.m_imageFilterInfo),
    m_imageFilter(left.m_imageFilter),
    m_diffuseFilterInfo(left.m_diffuseFilterInfo),
    m_diffuseFilter(left.m_diffuseFilter)
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
  if (item != nullptr)
  {
    std::string imageFilter = m_imageFilterInfo.GetItemLabel(item);
    if (!imageFilter.empty())
    {
      m_imageFilter = ImageSettings::TranslateImageFilter(imageFilter);
      UpdateImageFilter(m_imageFilter);
    }

    std::string diffuseFilter = m_diffuseFilterInfo.GetItemLabel(item);
    if (!diffuseFilter.empty())
    {
      m_diffuseFilter = ImageSettings::TranslateImageFilter(diffuseFilter);
      UpdateDiffuseFilter(m_diffuseFilter);
    }
  }

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

  // the clock is never reset, so a fade pending across a processing gap
  // settles on resume instead of replaying stale content
  unsigned int frameTime = 0;
  if (m_lastRenderTime)
    frameTime = currentTime - m_lastRenderTime;
  if (!frameTime)
    frameTime = (unsigned int)(1000 / CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS());
  m_lastRenderTime = currentTime;

  // swap the new image in once it has something to show (or clears the image)
  if (m_isTransitioning && (m_textureNext->ReadyToRender() || m_textureNext->GetFileName().empty()))
  {
    if (m_crossFadeTime)
      StartFadeTransition();
    else
      ProcessInstantTransition();
  }

  if (m_crossFadeTime)
    ProcessFades(currentTime, frameTime);

  if (!m_textureCurrent->GetDiffuseColor().HasInfo())
    UpdateDiffuseColor(nullptr);

  if (m_textureCurrent->Process(currentTime))
    MarkDirtyRegion();

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

    // the current texture is already what we want, so cancel the incoming one
    if (m_isTransitioning)
    {
      m_textureNext->SetFileName("");
      m_nameNext = "";
      m_isTransitioning = false;
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

    m_isTransitioning = true;
    m_hasNewStagingTexture = false;
    return;
  }

  // replace the in-flight texture, so the latest request always wins
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

void CGUIImage::ProcessInstantTransition()
{
  std::swap(m_textureCurrent, m_textureNext);
  std::swap(m_nameCurrent, m_nameNext);

  m_nameNext = "";
  m_textureNext->SetFileName("");

  m_textureCurrent->SetAlpha(0xff);
  m_currentFadeTime = 0;
  m_isTransitioning = false;

  m_fadingTextures.clear();

  MarkDirtyRegion();
}

void CGUIImage::StartFadeTransition()
{
  if (m_textureCurrent->ReadyToRender())
  { // save the current image, so it can fade out on its own clock
    // the oldest texture is the most faded out, so kill that one off first
    if (m_fadingTextures.size() >= MAX_FADING_TEXTURES)
      m_fadingTextures.erase(m_fadingTextures.begin());

    m_fadingTextures.emplace_back(
        std::make_unique<CFadingTexture>(m_textureCurrent.get(), m_currentFadeTime));
  }

  std::swap(m_textureCurrent, m_textureNext);
  std::swap(m_nameCurrent, m_nameNext);

  m_nameNext = "";
  m_textureNext->SetFileName("");

  m_currentFadeTime = 0;
  m_isTransitioning = false;

  MarkDirtyRegion();
}

void CGUIImage::ProcessFades(unsigned int currentTime, unsigned int frameTime)
{
  if (!m_fadingTextures.empty())
  { // have some fading images
    // anything other than the last old texture needs to be faded out as per usual
    for (auto i = m_fadingTextures.begin(); i != m_fadingTextures.end() - 1;)
    {
      if (!ProcessFading(**i, frameTime, currentTime))
        i = m_fadingTextures.erase(i);
      else
        ++i;
    }

    if (m_textureCurrent->ReadyToRender() || m_textureCurrent->GetFileName().empty())
    { // fade out the last one as well
      if (!ProcessFading(*m_fadingTextures.back(), frameTime, currentTime))
        m_fadingTextures.pop_back();
    }
    else
    { // keep the last one fading in
      CFadingTexture& texture = *m_fadingTextures.back();
      texture.m_fadeTime += frameTime;
      if (texture.m_fadeTime > m_crossFadeTime)
        texture.m_fadeTime = m_crossFadeTime;

      if (texture.m_texture->SetAlpha(GetFadeLevel(texture.m_fadeTime)))
        MarkDirtyRegion();
      if (texture.m_texture->SetDiffuseColor(m_diffuseColor))
        MarkDirtyRegion();
      if (texture.m_texture->Process(currentTime))
        MarkDirtyRegion();
    }
  }

  if (m_textureCurrent->ReadyToRender() || m_textureCurrent->GetFileName().empty())
  { // fade the new one in
    m_currentFadeTime += frameTime;
    if (m_currentFadeTime > m_crossFadeTime ||
        frameTime == 0) // for if we allocate straight away on creation
      m_currentFadeTime = m_crossFadeTime;
  }

  if (m_textureCurrent->SetAlpha(GetFadeLevel(m_currentFadeTime)))
    MarkDirtyRegion();
}

bool CGUIImage::ProcessFading(CFadingTexture& texture,
                              unsigned int frameTime,
                              unsigned int currentTime)
{
  if (texture.m_fadeTime <= frameTime)
  { // time to kill off the texture
    MarkDirtyRegion();
    return false;
  }
  // render this texture
  texture.m_fadeTime -= frameTime;

  if (texture.m_texture->SetAlpha(GetFadeLevel(texture.m_fadeTime)))
    MarkDirtyRegion();
  if (texture.m_texture->SetDiffuseColor(m_diffuseColor))
    MarkDirtyRegion();
  if (texture.m_texture->Process(currentTime))
    MarkDirtyRegion();

  return true;
}

void CGUIImage::Render()
{
  if (!IsVisible())
    return;

  // what is left of the old images hides behind the new one
  for (const auto& fadingTexture : m_fadingTextures)
    fadingTexture->m_texture->Render();

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
  m_fadingTextures.clear();

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

  for (const auto& fadingTexture : m_fadingTextures)
    region.Union(fadingTexture->m_texture->GetRenderRect());

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

void CGUIImage::SetScalingMethod(TEXTURE_SCALING scalingMethod)
{
  m_textureCurrent->SetScalingMethod(scalingMethod);
  m_textureNext->SetScalingMethod(scalingMethod);
}

void CGUIImage::SetDiffuseScalingMethod(TEXTURE_SCALING scalingMethod)
{
  m_textureCurrent->SetDiffuseScalingMethod(scalingMethod);
  m_textureNext->SetDiffuseScalingMethod(scalingMethod);
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

void CGUIImage::SetImageFilter(const GUIINFO::CGUIInfoLabel& imageFilter)
{
  m_imageFilterInfo = imageFilter;

  // Check if an image filter is available without a listitem
  static const CFileItem empty;
  const std::string strImageFilter = m_imageFilterInfo.GetItemLabel(&empty);
  if (!strImageFilter.empty())
  {
    m_imageFilter = ImageSettings::TranslateImageFilter(strImageFilter);
    UpdateImageFilter(m_imageFilter);
  }
}

void CGUIImage::SetDiffuseFilter(const GUIINFO::CGUIInfoLabel& diffuseFilter)
{
  m_diffuseFilterInfo = diffuseFilter;

  // Check if a diffuse filter is available without a listitem
  static const CFileItem empty;
  const std::string strDiffuseFilter = m_diffuseFilterInfo.GetItemLabel(&empty);
  if (!strDiffuseFilter.empty())
  {
    m_diffuseFilter = ImageSettings::TranslateImageFilter(strDiffuseFilter);
    UpdateDiffuseFilter(m_diffuseFilter);
  }
}

void CGUIImage::UpdateImageFilter(IMAGE_FILTER imageFilter)
{
  switch (imageFilter)
  {
    case IMAGE_FILTER::LINEAR:
    {
      SetScalingMethod(TEXTURE_SCALING::LINEAR);
      break;
    }
    case IMAGE_FILTER::NEAREST:
    {
      SetScalingMethod(TEXTURE_SCALING::NEAREST);
      break;
    }
    default:
      break;
  }
}

void CGUIImage::UpdateDiffuseFilter(IMAGE_FILTER diffuseFilter)
{
  switch (diffuseFilter)
  {
    case IMAGE_FILTER::LINEAR:
    {
      SetDiffuseScalingMethod(TEXTURE_SCALING::LINEAR);
      break;
    }
    case IMAGE_FILTER::NEAREST:
    {
      SetDiffuseScalingMethod(TEXTURE_SCALING::NEAREST);
      break;
    }
    default:
      break;
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
  // report the incoming texture as soon as it resolves so Control.GetLabel doesn't lag the fade
  if (m_isTransitioning && (m_textureNext->ReadyToRender() || m_textureNext->GetFileName().empty()))
    return m_textureNext->GetFileName();
  return GetFileName();
}

