/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <vector>

#include "guilib/Resolution.h"
#include "GBMUtils.h"

struct crtc
{
  drmModeCrtc *crtc;
  drmModeObjectProperties *props;
  drmModePropertyRes **props_info;
};

struct connector
{
  drmModeConnector *connector;
  drmModeObjectProperties *props;
  drmModePropertyRes **props_info;
};

struct drm
{
  int fd;

  struct crtc *crtc;
  struct connector *connector;
  int crtc_index;

  drmModeModeInfo *mode;
  uint32_t crtc_id;
  uint32_t connector_id;
};

struct drm_fb
{
  struct gbm_bo *bo;
  uint32_t fb_id;
};

class CDRMUtils
{
public:
  static bool InitDrm(drm *drm);
  static void DestroyDrm();
  static bool GetModes(std::vector<RESOLUTION_INFO> &resolutions);

protected:
  static bool GetMode(RESOLUTION_INFO res);
  static drm_fb * DrmFbGetFromBo(struct gbm_bo *bo);

private:
  static bool GetResources();
  static bool GetConnector();
  static bool GetEncoder();
  static bool GetPreferredMode();
  static bool RestoreOriginalMode();
  static void DrmFbDestroyCallback(struct gbm_bo *bo, void *data);
};
