/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "OverlayRenderer.h"

#include <string>

#define MAX_LINES 6

struct SInfo
{
  std::string line[MAX_LINES];
};

class CDVDOverlayText;

class CDebugRenderer
{
public:
  CDebugRenderer();
  virtual ~CDebugRenderer();
  void SetInfo(SInfo& info);
  void Render(CRect &src, CRect &dst, CRect &view);
  void Flush();

protected:

  class CRenderer : public OVERLAY::CRenderer
  {
  public:
    CRenderer();
    void Render(int idx) override;
  };

  std::string m_strDebug[MAX_LINES];
  CDVDOverlayText* m_overlay[MAX_LINES];
  CRenderer m_overlayRenderer;
};
