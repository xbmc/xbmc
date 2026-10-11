/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/Shader.h"

#include <string>

namespace KODI::PICTURE
{
struct GainMapParams;
}

/*!
 * \brief Draws an SDR picture with an Ultra HDR gain map as PQ, for HDR output.
 *
 * Without a gain map it draws the SDR picture at the same white as the GUI.
 */
class CGainMapShaderGLES : public Shaders::CGLSLShaderProgram
{
public:
  explicit CGainMapShaderGLES(const std::string& prefix);

  //! Set before Enable(); weight 0 draws the picture without its gain map
  void SetGainMap(const KODI::PICTURE::GainMapParams& params, float weight);
  void SetGainMapScale(float x, float y);
  void SetAlpha(float alpha) { m_alpha = alpha; }

  GLint GetPosLoc() const { return m_hPos; }
  GLint GetTexLoc() const { return m_hTex; }

protected:
  void OnCompiledAndLinked() override;
  bool OnEnabled() override;

private:
  float m_min{0.0f};
  float m_max{0.0f};
  float m_gamma{1.0f};
  float m_offsetSdr{0.0f};
  float m_offsetHdr{0.0f};
  float m_weight{0.0f};
  float m_scaleX{1.0f};
  float m_scaleY{1.0f};
  float m_alpha{1.0f};

  GLint m_hPos{-1};
  GLint m_hTex{-1};
  GLint m_hProj{-1};
  GLint m_hModel{-1};
  GLint m_hPicture{-1};
  GLint m_hGainMap{-1};
  GLint m_hMin{-1};
  GLint m_hMax{-1};
  GLint m_hGamma{-1};
  GLint m_hOffsetSdr{-1};
  GLint m_hOffsetHdr{-1};
  GLint m_hWeight{-1};
  GLint m_hScale{-1};
  GLint m_hAlpha{-1};
};
