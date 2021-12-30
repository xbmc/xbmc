/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

#include <memory>

namespace KODI
{
namespace ADDONS
{
class CScreenSaver;
} // namespace ADDONS
} // namespace KODI

class CGUIWindowScreensaver : public CGUIWindow
{
public:
  CGUIWindowScreensaver(void);

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override { return false; } // We're just a screen saver, nothing to do here
  void Render() override;
  void Process(unsigned int currentTime, CDirtyRegionList &regions) override;

protected:
  EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event) override;

private:
  std::unique_ptr<KODI::ADDONS::CScreenSaver> m_addon;
};

