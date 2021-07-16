/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/Shader.h"

#include <string>

class CGLESShader : public Shaders::CGLSLShaderProgram
{
public:
  CGLESShader(const char* shader, const std::string& prefix);
  CGLESShader(const char* vshader, const char* fshader, const std::string& prefix);
  void OnCompiledAndLinked() override;
  bool OnEnabled() override;
  void Free();

  GLint GetPosLoc()   { return m_hPos;   }
  GLint GetColLoc()   { return m_hCol;   }
  GLint GetCord0Loc() { return m_hCord0; }
  GLint GetCord1Loc() { return m_hCord1; }
  GLint GetUniColLoc() { return m_hUniCol; }
  GLint GetCoord0MatrixLoc() { return m_hCoord0Matrix; }
  GLint GetFieldLoc() { return m_hField; }
  GLint GetStepLoc() { return m_hStep; }
  GLint GetContrastLoc() { return m_hContrast; }
  GLint GetBrightnessLoc() { return m_hBrightness; }
  GLint GetModelLoc() { return m_hModel; }
  bool HardwareClipIsPossible() { return m_clipPossible; }
  GLfloat GetClipXFactor() { return m_clipXFactor; }
  GLfloat GetClipXOffset() { return m_clipXOffset; }
  GLfloat GetClipYFactor() { return m_clipYFactor; }
  GLfloat GetClipYOffset() { return m_clipYOffset; }

  void SetAlpha(float alpha);

  void SetLayers(int layers);

  void SetYUVMatrix(float (&yuv)[4][4]);

  void SetEnableColorConversion(bool enable);
  void SetPrimaryMatrix(float (&mat)[3][3]);
  void SetGammaSrc(float gamma);
  void SetGammaDstInv(float gamma);

  void SetToneMappingMethod(int method);
  void SetRGBYUVCoefficients(float (&coefs)[3]);
  void SetToneMapParameter(float parameter);

protected:
  GLint m_hTex0 = 0;
  GLint m_hTex1 = 0;
  GLint m_hTex2 = 0;
  GLint m_hUniCol = 0;
  GLint m_hProj = 0;
  GLint m_hModel = 0;
  GLint m_hPos = 0;
  GLint m_hCol = 0;
  GLint m_hCord0 = 0;
  GLint m_hCord1 = 0;
  GLint m_hCoord0Matrix = 0;
  GLint m_hField = 0;
  GLint m_hStep = 0;
  GLint m_hContrast = 0;
  GLint m_hBrightness = 0;

  GLint m_halpha = 0;

  GLint m_hLayers = 0;

  GLint m_hyuvMatrix = 0;

  GLint m_henableColorConversion = 0;
  GLint m_hprimaryMatrix = 0;
  GLint m_hgammaSrc = 0;
  GLint m_hgammaDstInv = 0;

  GLint m_htoneMapMethod = 0;
  GLint m_hcoefs = 0;
  GLint m_htoneP1 = 0;

  const GLfloat *m_proj;
  const GLfloat *m_model;

  bool m_clipPossible;
  GLfloat m_clipXFactor;
  GLfloat m_clipXOffset;
  GLfloat m_clipYFactor;
  GLfloat m_clipYOffset;
};
