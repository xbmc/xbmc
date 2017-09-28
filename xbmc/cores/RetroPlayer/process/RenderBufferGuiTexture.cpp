/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RenderBufferGuiTexture.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferGuiTexture::CRenderBufferGuiTexture(ESCALINGMETHOD scalingMethod) :
  m_scalingMethod(scalingMethod)
{
  m_textureFormat = XB_FMT_A8R8G8B8;
}

bool CRenderBufferGuiTexture::Allocate(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int size)
{
  // Initialize IRenderBuffer
  m_format = TranslateFormat(m_textureFormat);

  if (m_format != AV_PIX_FMT_NONE)
  {
    m_texture.reset(new CTexture(width, height, m_textureFormat));
    m_texture->SetScalingMethod(TranslateScalingMethod(m_scalingMethod));
    m_texture->SetCacheMemory(true);

    // Update IRenderBuffer
    m_width = m_texture->GetTextureWidth();
    m_height = m_texture->GetTextureHeight();

    return true;
  }

  return false;
}

size_t CRenderBufferGuiTexture::GetFrameSize() const
{
  if (m_texture)
    return m_texture->GetPitch() * m_texture->GetRows();

  return 0;
}

uint8_t *CRenderBufferGuiTexture::GetMemory()
{
  if (m_texture)
    return m_texture->GetPixels();

  return nullptr;
}

bool CRenderBufferGuiTexture::UploadTexture()
{
  bool bLoaded = false;

  if (m_texture)
  {
    m_texture->LoadToGPU();
    bLoaded = true;
  }

  return bLoaded;
}

void CRenderBufferGuiTexture::BindToUnit(unsigned int unit)
{
  if (m_texture)
    m_texture->BindToUnit(unit);
}

AVPixelFormat CRenderBufferGuiTexture::TranslateFormat(unsigned int textureFormat)
{
  switch (textureFormat)
  {
  case XB_FMT_RGBA8:
  case XB_FMT_A8R8G8B8:
    return AV_PIX_FMT_BGRA;
  default:
    break;
  }

  return AV_PIX_FMT_NONE;
}

TEXTURE_SCALING CRenderBufferGuiTexture::TranslateScalingMethod(ESCALINGMETHOD scalingMethod)
{
  switch (scalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
    return TEXTURE_SCALING::NEAREST;
  case VS_SCALINGMETHOD_LINEAR:
    return TEXTURE_SCALING::LINEAR;
  default:
    break;
  }

  return TEXTURE_SCALING::NEAREST;
}
