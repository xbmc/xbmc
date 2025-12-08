/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RenderCapture.h"
#include "guilib/D3DResource.h"

#include <wrl/client.h>

class CRenderCaptureDX : public CRenderCapture, public ID3DResource
{
public:
  CRenderCaptureDX();
  ~CRenderCaptureDX() override;

  void BeginRender() override;
  void EndRender() override;
  void ReadOut() override;

  void OnDestroyDevice(bool fatal) override;
  void OnCreateDevice() override{};
  CD3DTexture& GetTarget() { return m_renderTex; }

private:
  void SurfaceToBuffer();
  void CleanupDX();

  unsigned int m_surfaceWidth{0};
  unsigned int m_surfaceHeight{0};
  Microsoft::WRL::ComPtr<ID3D11Query> m_query{nullptr};
  CD3DTexture m_renderTex;
  CD3DTexture m_copyTex;
};
