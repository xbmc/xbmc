/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "RendererBase.h"

class CRendererHQ : public CRendererBase
{
public:
  bool Supports(ESCALINGMETHOD method) const override;

protected:
  explicit CRendererHQ(CVideoSettings& videoSettings) : CRendererBase(videoSettings) {}
  virtual ~CRendererHQ() = default;

  void OnOutputReset() override;
  void CheckVideoParameters() override;
  void UpdateVideoFilters() override;
  void FinalOutput(CD3DTexture& source, CD3DTexture& target, const CRect& sourceRect, const CPoint(&destPoints)[4]) override;

  void SelectPSVideoFilter();
  bool HasHQScaler() const;

  ESCALINGMETHOD m_scalingMethod = VS_SCALINGMETHOD_AUTO;
  ESCALINGMETHOD m_scalingMethodGui = VS_SCALINGMETHOD_AUTO;
  std::unique_ptr<CConvolutionShader> m_scalerShader = nullptr;
  bool m_bUseHQScaler = false;
};
