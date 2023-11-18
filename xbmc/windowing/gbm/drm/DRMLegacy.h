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

class CDRMLegacy : public CDRMUtils
{
public:
  CDRMLegacy() = default;
  ~CDRMLegacy() override = default;
  void FlipPage(struct gbm_bo* bo, bool rendered, bool videoLayer, bool async) override;
  bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo* bo) override;
  bool SetActive(bool active) override;
  bool InitDrm() override;

private:
  bool WaitingForFlip();
  bool QueueFlip(struct gbm_bo *bo);
  static void PageFlipHandler(int fd, unsigned int frame, unsigned int sec,
                              unsigned int usec, void *data);
};

}
}
}
