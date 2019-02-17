/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureGLBase.h"

#include "ServiceBroker.h"
#include "guilib/TextureManager.h"
#include "rendering/RenderSystem.h"

CTextureGLBase::CTextureGLBase(unsigned int width, unsigned int height, unsigned int format)
  : CBaseTexture(width, height, format)
{
  m_maxSize = CServiceBroker::GetRenderSystem()->GetMaxTextureSize();
}

CTextureGLBase::~CTextureGLBase()
{
  DestroyTextureObject();
}

void CTextureGLBase::CreateTextureObject()
{
  glGenTextures(1, &m_texture);
}

void CTextureGLBase::DestroyTextureObject()
{
  if (m_texture)
  {
    CServiceBroker::GetGUI()->GetTextureManager().ReleaseHwTexture(m_texture);
  }
}

void CTextureGLBase::BindToUnit(unsigned int unit)
{
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, m_texture);
}
