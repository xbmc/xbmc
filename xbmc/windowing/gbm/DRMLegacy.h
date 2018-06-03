/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "DRMUtils.h"

class CDRMLegacy : public CDRMUtils
{
public:
  CDRMLegacy() = default;
  ~CDRMLegacy() { DestroyDrm(); };
  virtual void FlipPage(struct gbm_bo *bo, bool rendered, bool videoLayer) override;
  virtual bool SetVideoMode(RESOLUTION_INFO res, struct gbm_bo *bo) override;
  virtual bool SetActive(bool active) override;
  virtual bool InitDrm() override;

private:
  bool WaitingForFlip();
  bool QueueFlip(struct gbm_bo *bo);
  static void PageFlipHandler(int fd, unsigned int frame, unsigned int sec,
                              unsigned int usec, void *data);
};
