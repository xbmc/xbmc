/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DRMUtils.h"

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class COffScreenModeSetting : public CDRMUtils
{
public:
  COffScreenModeSetting() = default;
  ~COffScreenModeSetting() { DestroyDrm(); };
  void FlipPage(struct gbm_bo *bo, bool rendered, bool videoLayer) override {}
  bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo *bo) override { return false; }
  bool SetActive(bool active) override { return false; }
  bool InitDrm() override;
  void DestroyDrm() override {}

  RESOLUTION_INFO GetCurrentMode() override;
  std::vector<RESOLUTION_INFO> GetModes() override;
  bool SetMode(const RESOLUTION_INFO& res) override { return true; }

private:
  const int DISPLAY_WIDTH = 1280;
  const int  DISPLAY_HEIGHT= 720;
  const float DISPLAY_REFRESH = 50.0f;
};

}
}
}
