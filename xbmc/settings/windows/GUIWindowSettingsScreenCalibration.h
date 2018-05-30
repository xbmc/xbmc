/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <vector>

#include "guilib/GUIWindow.h"

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
