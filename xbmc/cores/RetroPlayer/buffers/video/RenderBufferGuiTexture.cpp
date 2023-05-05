/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferGuiTexture.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferGuiTexture::CRenderBufferGuiTexture(SCALINGMETHOD scalingMethod)
  : m_scalingMethod(scalingMethod)
{
  m_textureFormat = XB_FMT_A8R8G8B8;
}

bool CRenderBufferGuiTexture::Allocate(AVPixelFormat format,
                                       unsigned int width,
                                       unsigned int height)
{
  // Initialize IRenderBuffer
  m_format = TranslateFormat(m_textureFormat);

  if (m_format != AV_PIX_FMT_NONE)
  {
    m_texture = CTexture::CreateTexture(width, height, m_textureFormat);
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

uint8_t* CRenderBufferGuiTexture::GetMemory()
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

AVPixelFormat CRenderBufferGuiTexture::TranslateFormat(XB_FMT textureFormat)
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

TEXTURE_SCALING CRenderBufferGuiTexture::TranslateScalingMethod(SCALINGMETHOD scalingMethod)
{
  switch (scalingMethod)
  {
    case SCALINGMETHOD::NEAREST:
      return TEXTURE_SCALING::NEAREST;
    case SCALINGMETHOD::LINEAR:
      return TEXTURE_SCALING::LINEAR;
    default:
      break;
  }

  return TEXTURE_SCALING::NEAREST;
}
