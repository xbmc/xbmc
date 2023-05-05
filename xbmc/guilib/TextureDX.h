/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "D3DResource.h"
#include "Texture.h"

/************************************************************************/
/*    CDXTexture                                                       */
/************************************************************************/
class CDXTexture : public CTexture
{
public:
  CDXTexture(unsigned int width = 0, unsigned int height = 0, XB_FMT format = XB_FMT_UNKNOWN);
  virtual ~CDXTexture();

  void CreateTextureObject();
  void DestroyTextureObject();
  virtual void LoadToGPU();
  void BindToUnit(unsigned int unit);

  ID3D11Texture2D* GetTextureObject()
  {
    return m_texture.Get();
  };

  ID3D11ShaderResourceView* GetShaderResource()
  {
    return m_texture.GetShaderResource();
  };

private:
  CD3DTexture m_texture;
  DXGI_FORMAT GetFormat();
};
