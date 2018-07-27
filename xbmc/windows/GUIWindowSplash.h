/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include "guilib/GUIWindow.h"

class CGUITextLayout;
class CGUIImage;

class CGUIWindowSplash : public CGUIWindow
{
public:
  CGUIWindowSplash(void);
  ~CGUIWindowSplash(void) override;
  bool OnAction(const CAction &action) override { return false; };
  void Render() override;
protected:
  void OnInitWindow() override;
private:
  std::unique_ptr<CGUIImage> m_image;
};
