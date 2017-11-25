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

struct plane
{
  drmModePlane *plane;
  drmModeObjectProperties *props;
  drmModePropertyRes **props_info;
};

struct connector
{
  drmModeConnector *connector;
  drmModeObjectProperties *props;
  drmModePropertyRes **props_info;
};

struct encoder
{
  drmModeEncoder *encoder;
};

struct crtc
{
  drmModeCrtc *crtc;
  drmModeObjectProperties *props;
  drmModePropertyRes **props_info;
};

struct drm
{
  int fd;

  struct plane *primary_plane;
  struct plane *overlay_plane;
  struct connector *connector;
  struct encoder *encoder;
  struct crtc *crtc;

  drmModeModeInfo *mode;

  int crtc_index;
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
  static bool SetMode(RESOLUTION_INFO res);
  static void WaitVBlank();
  static struct drm *GetDrm();

protected:
  static drm_fb * DrmFbGetFromBo(struct gbm_bo *bo);

private:
  static bool GetResources();
  static bool GetConnector();
  static bool GetEncoder();
  static bool GetCrtc();
  static bool GetPlanes();
  static bool GetPreferredMode();
  static int Open(const char* device);
  static bool RestoreOriginalMode();
  static void DrmFbDestroyCallback(struct gbm_bo *bo, void *data);
};
