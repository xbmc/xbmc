/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *      Test patterns designed by Ofer LaOr - hometheater.co.il
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/windows/GUIWindowTestPattern.h"
#include "guilib/GUIShaderDX.h"

#include <wrl/client.h>

class CGUIWindowTestPatternDX : public CGUIWindowTestPattern
{
public:
  CGUIWindowTestPatternDX(void);
  virtual ~CGUIWindowTestPatternDX(void);

protected:
  void DrawVerticalLines(int top, int left, int bottom, int right) override;
  void DrawHorizontalLines(int top, int left, int bottom, int right) override;
  void DrawCheckers(int top, int left, int bottom, int right) override;
  void DrawBouncingRectangle(int top, int left, int bottom, int right) override;
  void DrawContrastBrightnessPattern(int top, int left, int bottom, int right) override;
  void DrawCircle(int originX, int originY, int radius) override;
  void BeginRender() override;
  void EndRender() override;

private:
  void UpdateVertexBuffer(Vertex *vertices, unsigned count);
  void DrawRectangle(float x, float y, float x2, float y2, DWORD color);
  void DrawCircleEx(float originX, float originY, float radius, DWORD color);

  unsigned m_bufferWidth;
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_vb;
};

