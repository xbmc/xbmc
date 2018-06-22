  /*
 *      Copyright (C) 2005-2018 Team XBMC
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

#include "DRMUtils.h"

#ifdef TARGET_POSIX
#include "platform/linux/XTimeUtils.h"
#endif

class COffScreenModeSetting : public CDRMUtils
{
public:
  COffScreenModeSetting() = default;
  ~COffScreenModeSetting() { DestroyDrm(); };
  void FlipPage(struct gbm_bo *bo, bool rendered, bool videoLayer) override {}
  bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo *bo) override { return false; }
  bool SetActive(bool active) override { return false; }
  bool InitDrm() override;
  void DestroyDrm() override;

  RESOLUTION_INFO GetCurrentMode() override;
  std::vector<RESOLUTION_INFO> GetModes() override;
  bool SetMode(const RESOLUTION_INFO& res) override { return true; }
  void WaitVBlank() override { Sleep(20); }

private:
  const int DISPLAY_WIDTH = 1280;
  const int  DISPLAY_HEIGHT= 720;
  const float DISPLAY_REFRESH = 50.0f;
};
