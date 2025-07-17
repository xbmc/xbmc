/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "cores/RetroPlayer/RetroPlayerTypes.h"
#include "guilib/D3DResource.h"

#include <memory>
#include <string>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

namespace KODI::SHADER
{

class CRPWinShader
{
public:
  virtual ~CRPWinShader() = default;

protected:
  virtual bool CreateVertexBuffer(unsigned int vertCount, unsigned int vertSize);
  virtual bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* layout, unsigned numElements);
  virtual bool LockVertexBuffer(void** data);
  virtual bool UnlockVertexBuffer();
  virtual bool LoadEffect(const std::string& filename, DefinesMap* defines);
  virtual bool Execute(const std::vector<CD3DTexture*>& targets, unsigned int vertexIndexStep);
  virtual void SetStepParams(unsigned stepIndex) {}

  CD3DEffect m_effect;
  CD3DTexture* m_target{nullptr};

private:
  void SetTarget(CD3DTexture* target);

  CD3DBuffer m_vb;
  CD3DBuffer m_ib;
  unsigned int m_vbsize{0};
  unsigned int m_vertsize{0};
  Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
};

class CRPWinOutputShader : protected CRPWinShader
{
public:
  ~CRPWinOutputShader() override = default;

  bool Create(RETRO::SCALINGMETHOD scalingMethod);
  void Render(CD3DTexture& sourceTexture,
              CRect sourceRect,
              const KODI::RETRO::ViewportCoordinates& points,
              CRect& viewPort,
              CD3DTexture& target,
              unsigned int range = 0);

private:
  void PrepareParameters(unsigned int sourceWidth,
                         unsigned int sourceHeight,
                         CRect sourceRect,
                         const KODI::RETRO::ViewportCoordinates& points);
  void SetShaderParameters(CD3DTexture& sourceTexture, unsigned int range, CRect& viewPort);

  unsigned int m_sourceWidth{0};
  unsigned int m_sourceHeight{0};
  CRect m_sourceRect{};
  KODI::RETRO::ViewportCoordinates m_destPoints{};
};

} // namespace KODI::SHADER
