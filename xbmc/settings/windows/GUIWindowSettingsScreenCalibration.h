/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

#include <vector>

class CGUIWindowSettingsScreenCalibration : public CGUIWindow
{
public:
  CGUIWindowSettingsScreenCalibration(void);
  ~CGUIWindowSettingsScreenCalibration(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void FrameMove() override;
  void DoRender() override;
  void AllocResources(bool forceLoad = false) override;
  void FreeResources(bool forceUnLoad = false) override;

protected:
  unsigned int FindCurrentResolution();
  void NextControl();
  void ResetControls();
  void EnableControl(int iControl);
  void UpdateFromControl(int iControl);
  unsigned int m_iCurRes;
  std::vector<RESOLUTION> m_Res;
  int m_iControl;
  float m_fPixelRatioBoxHeight;
};
