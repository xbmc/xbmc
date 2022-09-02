/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"

#include <map>
#include <utility>
#include <vector>

class CGUIWindowSettingsScreenCalibration : public CGUIWindow
{
public:
  CGUIWindowSettingsScreenCalibration(void);
  ~CGUIWindowSettingsScreenCalibration(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void FrameMove() override;
  void DoRender() override;
  void AllocResources(bool forceLoad = false) override;
  void FreeResources(bool forceUnLoad = false) override;

protected:
  unsigned int FindCurrentResolution();
  void NextControl();
  void ResetControls();
  void EnableControl(int iControl);
  bool UpdateFromControl(int iControl);
  void ResetCalibration();
  unsigned int m_iCurRes;
  std::vector<RESOLUTION> m_Res;
  int m_iControl;
  float m_fPixelRatioBoxHeight;

private:
  std::map<int, std::pair<float, float>> m_controlsSize;
  int m_subtitlesHalfSpace{0};
  int m_subtitleVerticalMargin{0};
  bool m_isSubtitleBarEnabled{false};
};
