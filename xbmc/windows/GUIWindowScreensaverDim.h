/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIWindowScreensaverDim : public CGUIDialog
{
public:
  CGUIWindowScreensaverDim();

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;

protected:
  void UpdateVisibility() override;

private:
  float m_dimLevel = 100.0f;
  float m_newDimLevel = 100.0f;
  bool m_visible = false;
};
