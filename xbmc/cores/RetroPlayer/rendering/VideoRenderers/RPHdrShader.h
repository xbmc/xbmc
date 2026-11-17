/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/Shader.h"
#include "utils/Geometry.h"

#include <array>
#include <string>

namespace KODI::RETRO
{
/*!
 * \brief Draws a game's frame for an HDR10 output
 *
 * The frame, made for an SDR display, is put into BT.2020 and PQ. Its
 * brightest light is raised towards the peak, which wins back the light a
 * CRT filter's scanlines and phosphor mask take away.
 */
class CRPHdrShader : public Shaders::CGLSLShaderProgram
{
public:
  explicit CRPHdrShader(const std::string& prefix);
  ~CRPHdrShader() override = default;

  /*!
   * \brief Set the luminance the frame is shown at
   *
   * \param paperWhite Nits of the frame's ordinary colours
   * \param peak Nits of its brightest light
   */
  void SetLuminance(unsigned int paperWhite, unsigned int peak);

  /*!
   * \brief Set the frame's opacity, from 0 to 1
   */
  void SetAlpha(float alpha) { m_alpha = alpha; }

  /*!
   * \brief Set where the frame goes and which part of the texture it shows
   *
   * \param origin, right, down Corners of the destination: top left, top
   *        right and bottom left, in GUI coordinates
   * \param texCoords The part of the texture that fills it
   */
  void SetQuad(const CPoint& origin,
               const CPoint& right,
               const CPoint& down,
               const CRect& texCoords);

  GLint GetPosLoc() const { return m_hPos; }

protected:
  void OnCompiledAndLinked() override;
  bool OnEnabled() override;

private:
  float m_paperWhite{0.0f}; // PQ-normalised: nits / 10000
  float m_peakRatio{1.0f}; // Peak over paper white
  float m_alpha{1.0f};
  std::array<GLfloat, 16> m_quad{};
  std::array<GLfloat, 16> m_texQuad{};

  GLint m_hPos{-1};
  GLint m_hSamp{-1};
  GLint m_hMatrix{-1};
  GLint m_hQuad{-1};
  GLint m_hTexQuad{-1};
  GLint m_hPaperWhite{-1};
  GLint m_hPeakRatio{-1};
  GLint m_hAlpha{-1};
};
} // namespace KODI::RETRO
