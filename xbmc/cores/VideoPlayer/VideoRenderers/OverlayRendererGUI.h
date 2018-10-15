/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "OverlayRenderer.h"
#include "utils/Color.h"
#include <string>

enum SubtitleAlign
{
  SUBTITLE_ALIGN_MANUAL         = 0,
  SUBTITLE_ALIGN_BOTTOM_INSIDE,
  SUBTITLE_ALIGN_BOTTOM_OUTSIDE,
  SUBTITLE_ALIGN_TOP_INSIDE,
  SUBTITLE_ALIGN_TOP_OUTSIDE
};

class CGUITextLayout;
class CDVDOverlayText;

namespace OVERLAY {

class COverlayText : public COverlay
{
public:
  COverlayText() = default;
  explicit COverlayText(CDVDOverlayText* src);
  ~COverlayText() override;
  void Render(SRenderState& state) override;
  using COverlay::PrepareRender;
  void PrepareRender(const std::string &font, int color, int height, int style, const std::string &fontcache,
                     const std::string &fontbordercache, const UTILS::Color bgcolor, const CRect &rectView);
  virtual CGUITextLayout* GetFontLayout(const std::string &font, int color, int height, int style,
                                        const std::string &fontcache, const std::string &fontbordercache);

  CGUITextLayout* m_layout;
  std::string m_text;
  int m_subalign;
  UTILS::Color m_bgcolor = UTILS::COLOR::NONE;
protected:
  // target Rect for subtitles (updated on PrepareRender)
  CRect m_rv = CRect(0, 0, 0, 0);
};

}
