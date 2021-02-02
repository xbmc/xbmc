/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowTestPattern.h"

#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUITexture.h"
#include "guilib/WindowIDs.h"
#include "input/Key.h"
#include "utils/Color.h"
#include "utils/Geometry.h"
#include "utils/Variant.h"
#include "windowing/WinSystem.h"

#include <iostream>


CGUIWindowTestPattern::CGUIWindowTestPattern(void) : CGUIWindow(WINDOW_TEST_PATTERN, "")
{

  std::map<std::string, CVariant> tpgParams;
  tpgParams["r"] = 0.;
  tpgParams["g"] = 0.;
  tpgParams["b"] = 0.;
  tpgParams["bkg_r"] = 0.2;
  tpgParams["bkg_g"] = 0.2;
  tpgParams["bkg_b"] = 0.2;
  tpgParams["x"] = 0.5 * (1. - sqrt(0.1));
  tpgParams["y"] = 0.5 * (1. - sqrt(0.1));
  tpgParams["w"] = sqrt(0.1);
  tpgParams["h"] = sqrt(0.1);

  SetProperty("tpgParams", tpgParams);
}

CGUIWindowTestPattern::~CGUIWindowTestPattern(void) = default;

void CGUIWindowTestPattern::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  //*TODO* Update only on demand
  MarkDirtyRegion();
  CGUIWindow::Process(currentTime, dirtyregions);
  m_renderRegion.SetRect(0, 0, (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
                         (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
}

void CGUIWindowTestPattern::Render()
{
  const CVariant& tpgParams = GetProperty("tpgParams");

  UTILS::Color4f bkgcolor(tpgParams["bkg_r"].asFloat(), tpgParams["bkg_g"].asFloat(),
                          tpgParams["bkg_b"].asFloat(), 1.);
  UTILS::Color4f color(tpgParams["r"].asFloat(), tpgParams["g"].asFloat(), tpgParams["b"].asFloat(),
                       1.);
  float x1 = tpgParams["x"].asFloat();
  float y1 = tpgParams["y"].asFloat();
  float w = tpgParams["w"].asFloat();
  float h = tpgParams["h"].asFloat();
  float x2 = x1 + w;
  float y2 = y1 + h;

  float ww = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
  float wh = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();

  x1 *= ww;
  y1 *= wh;
  x2 *= ww;
  y2 *= wh;

  CRect rect(x1, y1, x2, y2);

  CServiceBroker::GetRenderSystem()->ClearBuffers(bkgcolor);
  CGUITexture::DrawQuad(rect, color);

  CGUIWindow::Render();
}
