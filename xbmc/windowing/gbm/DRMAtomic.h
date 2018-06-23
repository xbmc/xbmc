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

#include "DRMUtils.h"

class CDRMAtomic : public CDRMUtils
{
public:
  CDRMAtomic() = default;
  ~CDRMAtomic() { DestroyDrm(); };
  virtual void FlipPage(struct gbm_bo *bo, bool rendered, bool videoLayer) override;
  virtual bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo *bo) override;
  virtual bool SetActive(bool active) override;
  virtual bool InitDrm() override;
  virtual void DestroyDrm() override;
  virtual bool AddProperty(struct drm_object *object, const char *name, uint64_t value) override;

private:
  void DrmAtomicCommit(int fb_id, int flags, bool rendered, bool videoLayer);

  bool m_need_modeset;
  bool m_active = true;
  drmModeAtomicReq *m_req = nullptr;
};
