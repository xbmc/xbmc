/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUITexture.h"

#include "GUILargeTextureManager.h"
#include "Texture.h"
#include "TextureManager.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "windowing/GraphicContext.h"

#include <stdexcept>

CreateGUITextureFunc CGUITexture::m_createGUITextureFunc;
DrawQuadFunc CGUITexture::m_drawQuadFunc;

CTextureInfo::CTextureInfo()
{
  orientation = 0;
  useLarge = false;
}

CTextureInfo::CTextureInfo(const std::string &file):
  filename(file)
{
  orientation = 0;
  useLarge = false;
}

void CGUITexture::Register(const CreateGUITextureFunc& createFunction,
                           const DrawQuadFunc& drawQuadFunction)
{
  m_createGUITextureFunc = createFunction;
  m_drawQuadFunc = drawQuadFunction;
}

CGUITexture* CGUITexture::CreateTexture(
    float posX, float posY, float width, float height, const CTextureInfo& texture)
{
  if (!m_createGUITextureFunc)
    throw std::runtime_error(
        "No GUITexture Create function available. Did you forget to register?");

  return m_createGUITextureFunc(posX, posY, width, height, texture);
}

void CGUITexture::DrawQuad(const CRect& coords,
                           UTILS::COLOR::Color color,
                           CTexture* texture,
                           const CRect* texCoords,
                           const float depth,
                           const bool blending)
{
  // bail for now if we render front to back
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_FRONT_TO_BACK)
    return;

  if (!m_drawQuadFunc)
    throw std::runtime_error(
        "No GUITexture DrawQuad function available. Did you forget to register?");

  m_drawQuadFunc(coords, color, texture, texCoords, depth, blending);
}

CGUITexture::CGUITexture(
    float posX, float posY, float width, float height, const CTextureInfo& texture)
  : m_height(height), m_info(texture)
{
  m_posX = posX;
  m_posY = posY;
  m_width = width;

  // defaults
  m_visible = true;
  m_diffuseColor = 0xffffffff;
  m_alpha = 0xff;

  m_vertex.SetRect(m_posX, m_posY, m_posX + m_width, m_posY + m_height);

  m_frameWidth = 0;
  m_frameHeight = 0;

  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;
  m_diffuseU = 1.0f;
  m_diffuseV = 1.0f;
  m_diffuseScaleU = 1.0f;
  m_diffuseScaleV = 1.0f;

  // anim gifs
  ResetAnimState();

  m_allocateDynamically = false;
  m_isAllocated = NO;
  m_invalid = true;
  m_use_cache = true;
}

CGUITexture::CGUITexture(const CGUITexture& right)
  : m_visible(right.m_visible),
    m_diffuseColor(right.m_diffuseColor),
    m_posX(right.m_posX),
    m_posY(right.m_posY),
    m_width(right.m_width),
    m_height(right.m_height),
    m_use_cache(right.m_use_cache),
    m_alpha(right.m_alpha),
    m_allocateDynamically(right.m_allocateDynamically),
    m_info(right.m_info),
    m_aspect(right.m_aspect)
{
  // defaults
  m_vertex.SetRect(m_posX, m_posY, m_posX + m_width, m_posY + m_height);

  m_frameWidth = 0;
  m_frameHeight = 0;

  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;
  m_diffuseU = 1.0f;
  m_diffuseV = 1.0f;
  m_diffuseScaleU = 1.0f;
  m_diffuseScaleV = 1.0f;

  ResetAnimState();

  m_isAllocated = NO;
  m_invalid = true;
}

bool CGUITexture::AllocateOnDemand()
{
  if (m_visible)
  { // visible, so make sure we're allocated
    if (!IsAllocated() || (m_isAllocated == LARGE && !m_texture.size()))
      return AllocResources();
  }
  else
  { // hidden, so deallocate as applicable
    if (m_allocateDynamically && IsAllocated())
      FreeResources();
    // reset animated textures (animgifs)
    ResetAnimState();
  }

  return false;
}

bool CGUITexture::Process(unsigned int currentTime)
{
  bool changed = false;
  // check if we need to allocate our resources
  changed |= AllocateOnDemand();

  if (m_texture.size() > 1)
    changed |= UpdateAnimFrame(currentTime);

  if (m_invalid)
    changed |= CalculateSize();

  if (m_isAllocated)
    changed |= !ReadyToRender();

  return changed;
}

void CGUITexture::Render(int32_t depthOffset, int32_t overrideDepth)
{
  if (!m_visible || !m_texture.size())
    return;

  // set our draw color
  #define MIX_ALPHA(a,c) (((a * (c >> 24)) / 255) << 24) | (c & 0x00ffffff)

  // diffuse color
  UTILS::COLOR::Color color =
      (m_info.diffuseColor) ? (UTILS::COLOR::Color)m_info.diffuseColor : m_diffuseColor;
  // clang-format off
  if (m_alpha != 0xFF)
    color = MIX_ALPHA(m_alpha, color);
  // clang-format on

  color = CServiceBroker::GetWinSystem()->GetGfxContext().MergeColor(color);

  if (overrideDepth >= 0)
  {
    m_depth = CServiceBroker::GetWinSystem()->GetGfxContext().GetNormalizedDepth(overrideDepth +
                                                                                 depthOffset);
  }
  else
  {
    m_depth = CServiceBroker::GetWinSystem()->GetGfxContext().GetTransformDepth(depthOffset);
  }

  bool hasAlpha =
      (((color >> 24) & 0xFF) != 0xFF || m_texture.m_textures[m_currentFrame]->HasAlpha());
  if (m_diffuse.size())
    hasAlpha |= m_diffuse.m_textures[0]->HasAlpha();

  // bail if it is not the appropriate render pass
  RENDER_ORDER renderOrder = CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder();
  if (hasAlpha && renderOrder == RENDER_ORDER_FRONT_TO_BACK)
    return;
  if (!hasAlpha && renderOrder == RENDER_ORDER_BACK_TO_FRONT)
    return;

  // see if we need to clip the image
  if (m_vertex.Width() > m_width || m_vertex.Height() > m_height)
  {
    if (!CServiceBroker::GetWinSystem()->GetGfxContext().SetClipRegion(m_posX, m_posY, m_width,
                                                                       m_height))
      return;
  }

  // setup our renderer
  Begin(color);

  // compute the texture coordinates
  float u1, u2, u3, v1, v2, v3;
  u1 = m_info.border.x1;
  u2 = m_frameWidth - m_info.border.x2;
  u3 = m_frameWidth;
  v1 = m_info.border.y1;
  v2 = m_frameHeight - m_info.border.y2;
  v3 = m_frameHeight;

  if (!m_texture.m_texCoordsArePixels)
  {
    u1 *= m_texCoordsScaleU;
    u2 *= m_texCoordsScaleU;
    u3 *= m_texCoordsScaleU;
    v1 *= m_texCoordsScaleV;
    v2 *= m_texCoordsScaleV;
    v3 *= m_texCoordsScaleV;
  }

  //! @todo The diffuse coloring applies to all vertices, which will
  //!      look weird for stuff with borders, as will the -ve height/width
  //!       for flipping

  // left segment (0,0,u1,v3)
  if (m_info.border.x1)
  {
    if (m_info.border.y1)
      Render(m_vertex.x1, m_vertex.y1, m_vertex.x1 + m_info.border.x1, m_vertex.y1 + m_info.border.y1, 0, 0, u1, v1, u3, v3);
    Render(m_vertex.x1, m_vertex.y1 + m_info.border.y1, m_vertex.x1 + m_info.border.x1, m_vertex.y2 - m_info.border.y2, 0, v1, u1, v2, u3, v3);
    if (m_info.border.y2)
      Render(m_vertex.x1, m_vertex.y2 - m_info.border.y2, m_vertex.x1 + m_info.border.x1, m_vertex.y2, 0, v2, u1, v3, u3, v3);
  }
  // middle segment (u1,0,u2,v3)
  if (m_info.border.y1)
    Render(m_vertex.x1 + m_info.border.x1, m_vertex.y1, m_vertex.x2 - m_info.border.x2, m_vertex.y1 + m_info.border.y1, u1, 0, u2, v1, u3, v3);
  if (m_info.m_infill)
    Render(m_vertex.x1 + m_info.border.x1, m_vertex.y1 + m_info.border.y1,
           m_vertex.x2 - m_info.border.x2, m_vertex.y2 - m_info.border.y2, u1, v1, u2, v2, u3, v3);
  if (m_info.border.y2)
    Render(m_vertex.x1 + m_info.border.x1, m_vertex.y2 - m_info.border.y2, m_vertex.x2 - m_info.border.x2, m_vertex.y2, u1, v2, u2, v3, u3, v3);
  // right segment
  if (m_info.border.x2)
  { // have a left border
    if (m_info.border.y1)
      Render(m_vertex.x2 - m_info.border.x2, m_vertex.y1, m_vertex.x2, m_vertex.y1 + m_info.border.y1, u2, 0, u3, v1, u3, v3);
    Render(m_vertex.x2 - m_info.border.x2, m_vertex.y1 + m_info.border.y1, m_vertex.x2, m_vertex.y2 - m_info.border.y2, u2, v1, u3, v2, u3, v3);
    if (m_info.border.y2)
      Render(m_vertex.x2 - m_info.border.x2, m_vertex.y2 - m_info.border.y2, m_vertex.x2, m_vertex.y2, u2, v2, u3, v3, u3, v3);
  }

  // close off our renderer
  End();

  if (m_vertex.Width() > m_width || m_vertex.Height() > m_height)
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreClipRegion();
}

void CGUITexture::Render(float left,
                         float top,
                         float right,
                         float bottom,
                         float u1,
                         float v1,
                         float u2,
                         float v2,
                         float u3,
                         float v3)
{
  CRect diffuse(u1, v1, u2, v2);
  CRect texture(u1, v1, u2, v2);
  CRect vertex(left, top, right, bottom);
  CServiceBroker::GetWinSystem()->GetGfxContext().ClipRect(vertex, texture, m_diffuse.size() ? &diffuse : NULL);

  if (vertex.IsEmpty())
    return; // nothing to render

  int orientation = GetOrientation();
  OrientateTexture(texture, u3, v3, orientation);

  if (m_diffuse.size())
  {
    // flip the texture as necessary.  Diffuse just gets flipped according to m_info.orientation.
    // Main texture gets flipped according to GetOrientation().
    diffuse.x1 *= m_diffuseScaleU / u3; diffuse.x2 *= m_diffuseScaleU / u3;
    diffuse.y1 *= m_diffuseScaleV / v3; diffuse.y2 *= m_diffuseScaleV / v3;
    diffuse += m_diffuseOffset;
    OrientateTexture(diffuse, m_diffuseU, m_diffuseV, m_info.orientation);
  }

  float x[4], y[4], z[4];

#define ROUND_TO_PIXEL(x) static_cast<float>(MathUtils::round_int(static_cast<double>(x)))

  x[0] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(vertex.x1, vertex.y1));
  y[0] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(vertex.x1, vertex.y1));
  z[0] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalZCoord(vertex.x1, vertex.y1));
  x[1] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(vertex.x2, vertex.y1));
  y[1] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(vertex.x2, vertex.y1));
  z[1] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalZCoord(vertex.x2, vertex.y1));
  x[2] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(vertex.x2, vertex.y2));
  y[2] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(vertex.x2, vertex.y2));
  z[2] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalZCoord(vertex.x2, vertex.y2));
  x[3] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(vertex.x1, vertex.y2));
  y[3] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(vertex.x1, vertex.y2));
  z[3] = ROUND_TO_PIXEL(CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalZCoord(vertex.x1, vertex.y2));

  if (y[2] == y[0]) y[2] += 1.0f;
  if (x[2] == x[0]) x[2] += 1.0f;
  if (y[3] == y[1]) y[3] += 1.0f;
  if (x[3] == x[1]) x[3] += 1.0f;

  Draw(x, y, z, texture, diffuse, orientation);
}

bool CGUITexture::AllocResources()
{
  if (m_info.filename.empty())
    return false;

  if (m_texture.size())
    return false; // already have our texture

  // reset our animstate
  ResetAnimState();

  bool changed = false;
  bool useLarge = m_info.useLarge || !CServiceBroker::GetGUI()->GetTextureManager().CanLoad(m_info.filename);
  if (useLarge)
  { // we want to use the large image loader, but we first check for bundled textures
    if (!IsAllocated())
    {
      CTextureArray texture;
      texture = CServiceBroker::GetGUI()->GetTextureManager().Load(m_info.filename, true);
      if (texture.size())
      {
        m_isAllocated = NORMAL;
        m_texture = texture;
        changed = true;
      }
    }
    if (m_isAllocated != NORMAL)
    { // use our large image background loader
      CTextureArray texture;
      if (CServiceBroker::GetGUI()->GetLargeTextureManager().GetImage(m_info.filename, texture, !IsAllocated(), m_use_cache))
      {
        m_isAllocated = LARGE;

        if (!texture.size()) // not ready as yet
          return false;

        m_texture = texture;

        changed = true;
      }
      else
        m_isAllocated = LARGE_FAILED;
    }
  }
  else if (!IsAllocated())
  {
    CTextureArray texture = CServiceBroker::GetGUI()->GetTextureManager().Load(m_info.filename);

    // set allocated to true even if we couldn't load the image to save
    // us hitting the disk every frame
    m_isAllocated = texture.size() ? NORMAL : NORMAL_FAILED;
    if (!texture.size())
      return false;
    m_texture = texture;
    changed = true;
  }
  m_frameWidth = (float)m_texture.m_width;
  m_frameHeight = (float)m_texture.m_height;

  // load the diffuse texture (if necessary)
  if (!m_info.diffuse.empty())
  {
    m_diffuse = CServiceBroker::GetGUI()->GetTextureManager().Load(m_info.diffuse);
  }

  CalculateSize();

  // call our implementation
  Allocate();

  return changed;
}

bool CGUITexture::CalculateSize()
{
  if (m_currentFrame >= m_texture.size())
    return false;

  m_texCoordsScaleU = 1.0f / m_texture.m_texWidth;
  m_texCoordsScaleV = 1.0f / m_texture.m_texHeight;

  if (m_width == 0)
    m_width = m_frameWidth;
  if (m_height == 0)
    m_height = m_frameHeight;

  float newPosX = m_posX;
  float newPosY = m_posY;
  float newWidth = m_width;
  float newHeight = m_height;

  if (m_aspect.ratio != CAspectRatio::AR_STRETCH && m_frameWidth && m_frameHeight)
  {
    // to get the pixel ratio, we must use the SCALED output sizes
    float pixelRatio = CServiceBroker::GetWinSystem()->GetGfxContext().GetScalingPixelRatio();

    float fSourceFrameRatio = m_frameWidth / m_frameHeight;
    if (GetOrientation() & 4)
      fSourceFrameRatio = m_frameHeight / m_frameWidth;
    float fOutputFrameRatio = fSourceFrameRatio / pixelRatio;

    // maximize the width
    newHeight = m_width / fOutputFrameRatio;

    if ((m_aspect.ratio == CAspectRatio::AR_SCALE && newHeight < m_height) ||
        (m_aspect.ratio == CAspectRatio::AR_KEEP && newHeight > m_height))
    {
      newHeight = m_height;
      newWidth = newHeight * fOutputFrameRatio;
    }
    if (m_aspect.ratio == CAspectRatio::AR_CENTER)
    { // keep original size + center
      newWidth = m_frameWidth / sqrt(pixelRatio);
      newHeight = m_frameHeight * sqrt(pixelRatio);
    }

    if (m_aspect.align & ASPECT_ALIGN_LEFT)
      newPosX = m_posX;
    else if (m_aspect.align & ASPECT_ALIGN_RIGHT)
      newPosX = m_posX + m_width - newWidth;
    else
      newPosX = m_posX + (m_width - newWidth) * 0.5f;
    if (m_aspect.align & ASPECT_ALIGNY_TOP)
      newPosY = m_posY;
    else if (m_aspect.align & ASPECT_ALIGNY_BOTTOM)
      newPosY = m_posY + m_height - newHeight;
    else
      newPosY = m_posY + (m_height - newHeight) * 0.5f;
  }

  m_vertex.SetRect(newPosX, newPosY, newPosX + newWidth, newPosY + newHeight);

  // scale the diffuse coords as well
  if (m_diffuse.size())
  { // calculate scaling for the texcoords
    if (m_diffuse.m_texCoordsArePixels)
    {
      m_diffuseU = float(m_diffuse.m_width);
      m_diffuseV = float(m_diffuse.m_height);
    }
    else
    {
      m_diffuseU = float(m_diffuse.m_width) / float(m_diffuse.m_texWidth);
      m_diffuseV = float(m_diffuse.m_height) / float(m_diffuse.m_texHeight);
    }

    if (m_aspect.scaleDiffuse)
    {
      m_diffuseScaleU = m_diffuseU;
      m_diffuseScaleV = m_diffuseV;
      m_diffuseOffset = CPoint(0,0);
    }
    else // stretching diffuse
    { // scale diffuse up or down to match output rect size, rather than image size
      //(m_fX, mfY) -> (m_fX + m_fNW, m_fY + m_fNH)
      //(0,0) -> (m_fU*m_diffuseScaleU, m_fV*m_diffuseScaleV)
      // x = u/(m_fU*m_diffuseScaleU)*m_fNW + m_fX
      // -> u = (m_posX - m_fX) * m_fU * m_diffuseScaleU / m_fNW
      m_diffuseScaleU = m_diffuseU * m_vertex.Width() / m_width;
      m_diffuseScaleV = m_diffuseV * m_vertex.Height() / m_height;
      m_diffuseOffset = CPoint((m_vertex.x1 - m_posX) / m_vertex.Width() * m_diffuseScaleU, (m_vertex.y1 - m_posY) / m_vertex.Height() * m_diffuseScaleV);
    }
  }

  m_invalid = false;
  return true;
}

void CGUITexture::FreeResources(bool immediately /* = false */)
{
  if (m_isAllocated == LARGE || m_isAllocated == LARGE_FAILED)
    CServiceBroker::GetGUI()->GetLargeTextureManager().ReleaseImage(m_info.filename, immediately || (m_isAllocated == LARGE_FAILED));
  else if (m_isAllocated == NORMAL && m_texture.size())
    CServiceBroker::GetGUI()->GetTextureManager().ReleaseTexture(m_info.filename, immediately);

  if (m_diffuse.size())
    CServiceBroker::GetGUI()->GetTextureManager().ReleaseTexture(m_info.diffuse, immediately);
  m_diffuse.Reset();

  m_texture.Reset();

  ResetAnimState();

  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;

  // call our implementation
  Free();

  m_isAllocated = NO;
}

void CGUITexture::DynamicResourceAlloc(bool allocateDynamically)
{
  m_allocateDynamically = allocateDynamically;
}

void CGUITexture::SetInvalid()
{
  m_invalid = true;
}

bool CGUITexture::UpdateAnimFrame(unsigned int currentTime)
{
  bool changed = false;
  unsigned int delay = m_texture.m_delays[m_currentFrame];

  if (m_lasttime == 0)
  {
    m_lasttime = currentTime;
  }
  else
  {
    if ((currentTime - m_lasttime) >= delay)
    {
      if (m_currentFrame + 1 >= m_texture.size())
      {
        if (m_texture.m_loops > 0)
        {
          if (m_currentLoop + 1 < m_texture.m_loops)
          {
            m_currentLoop++;
            m_currentFrame = 0;
            m_lasttime = currentTime;
            changed = true;
          }
        }
        else
        {
          // 0 == loop forever
          m_currentFrame = 0;
          m_lasttime = currentTime;
          changed = true;
        }
      }
      else
      {
        m_currentFrame++;
        m_lasttime = currentTime;
        changed = true;
      }
    }
  }

  return changed;
}

bool CGUITexture::SetVisible(bool visible)
{
  bool changed = m_visible != visible;
  m_visible = visible;
  return changed;
}

bool CGUITexture::SetAlpha(unsigned char alpha)
{
  bool changed = m_alpha != alpha;
  m_alpha = alpha;
  return changed;
}

bool CGUITexture::SetDiffuseColor(UTILS::COLOR::Color color,
                                  const CGUIListItem* item /* = nullptr */)
{
  bool changed = m_diffuseColor != color;
  m_diffuseColor = color;
  changed |= m_info.diffuseColor.Update(item);
  return changed;
}

bool CGUITexture::ReadyToRender() const
{
  return m_texture.size() > 0;
}

void CGUITexture::OrientateTexture(CRect& rect, float width, float height, int orientation)
{
  switch (orientation & 3)
  {
  case 0:
    // default
    break;
  case 1:
    // flip in X direction
    rect.x1 = width - rect.x1;
    rect.x2 = width - rect.x2;
    break;
  case 2:
    // rotate 180 degrees
    rect.x1 = width - rect.x1;
    rect.x2 = width - rect.x2;
    rect.y1 = height - rect.y1;
    rect.y2 = height - rect.y2;
    break;
  case 3:
    // flip in Y direction
    rect.y1 = height - rect.y1;
    rect.y2 = height - rect.y2;
    break;
  }
  if (orientation & 4)
  {
    // we need to swap x and y coordinates but only within the width,height block
    float temp = rect.x1;
    rect.x1 = rect.y1 * width/height;
    rect.y1 = temp * height/width;
    temp = rect.x2;
    rect.x2 = rect.y2 * width/height;
    rect.y2 = temp * height/width;
  }
}

void CGUITexture::ResetAnimState()
{
  m_lasttime = 0;
  m_currentFrame = 0;
  m_currentLoop = 0;
}

bool CGUITexture::SetWidth(float width)
{
  if (width < m_info.border.x1 + m_info.border.x2)
    width = m_info.border.x1 + m_info.border.x2;
  if (m_width != width)
  {
    m_width = width;
    m_invalid = true;
    return true;
  }
  else
    return false;
}

bool CGUITexture::SetHeight(float height)
{
  if (height < m_info.border.y1 + m_info.border.y2)
    height = m_info.border.y1 + m_info.border.y2;
  if (m_height != height)
  {
    m_height = height;
    m_invalid = true;
    return true;
  }
  else
    return false;
}

bool CGUITexture::SetPosition(float posX, float posY)
{
  if (m_posX != posX || m_posY != posY)
  {
    m_posX = posX;
    m_posY = posY;
    m_invalid = true;
    return true;
  }
  else
    return false;
}

bool CGUITexture::SetAspectRatio(const CAspectRatio& aspect)
{
  if (m_aspect != aspect)
  {
    m_aspect = aspect;
    m_invalid = true;
    return true;
  }
  else
    return false;
}

bool CGUITexture::SetFileName(const std::string& filename)
{
  if (m_info.filename == filename) return false;
  // Don't completely free resources here - we may be just changing
  // filenames mid-animation
  FreeResources();
  m_info.filename = filename;

  // disable large loader and cache for gifs
  if (StringUtils::EndsWithNoCase(m_info.filename, ".gif"))
  {
    m_info.useLarge = false;
    SetUseCache(false);
  }

  // Don't allocate resources here as this is done at render time
  return true;
}

void CGUITexture::SetUseCache(const bool useCache)
{
  m_use_cache = useCache;
}

int CGUITexture::GetOrientation() const
{
  // multiply our orientations
  static char orient_table[] = { 0, 1, 2, 3, 4, 5, 6, 7,
                                 1, 0, 3, 2, 5, 4, 7, 6,
                                 2, 3, 0, 1, 6, 7, 4, 5,
                                 3, 2, 1, 0, 7, 6, 5, 4,
                                 4, 7, 6, 5, 0, 3, 2, 1,
                                 5, 6, 7, 4, 1, 2, 3, 0,
                                 6, 5, 4, 7, 2, 1, 0, 3,
                                 7, 4, 5, 6, 3, 0, 1, 2 };
  return (int)orient_table[8 * m_info.orientation + m_texture.m_orientation];
}
