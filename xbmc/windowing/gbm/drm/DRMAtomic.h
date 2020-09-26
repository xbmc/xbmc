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

class CDRMAtomic : public CDRMUtils
{
public:
  CDRMAtomic() = default;
  ~CDRMAtomic() override { DestroyDrm(); };
  void FlipPage(struct gbm_bo* bo, bool rendered, bool videoLayer) override;
  bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo* bo) override;
  bool SetActive(bool active) override;
  bool InitDrm() override;
  void DestroyDrm() override;
  bool AddProperty(struct drm_object* object, const char* name, uint64_t value) override;
  static bool DislayHardwareScalingEnabled();

private:
  void DrmAtomicCommit(int fb_id, int flags, bool rendered, bool videoLayer);
  bool ResetPlanes();

  uint32_t GetScalingFilterType(const char* type);
  uint32_t GetScalingFactor(uint32_t srcWidth,
                            uint32_t srcHeight,
                            uint32_t destWidth,
                            uint32_t destHeight);
  bool SetScalingFilter(struct drm_object *object, const char *name, const char *type);

  static const std::string SETTING_VIDEOSCREEN_HW_SCALING_FILTER;

  bool m_need_modeset;
  bool m_active = true;
  drmModeAtomicReq *m_req = nullptr;
};

}
}
}
