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

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drm/drm_mode.h>
#include <EGL/egl.h>
#include <unistd.h>

#include "WinSystemGbmGLESContext.h"
#include "guilib/gui3d.h"
#include "utils/log.h"
#include "settings/Settings.h"

#include "GBMUtils.h"

bool CGBMUtils::InitGbm(struct gbm *gbm, int hdisplay, int vdisplay)
{
  gbm->width = hdisplay;
  gbm->height = vdisplay;

  gbm->surface = gbm_surface_create(gbm->dev,
                                    gbm->width,
                                    gbm->height,
                                    GBM_FORMAT_ARGB8888,
                                    GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

  if(!gbm->surface)
  {
    CLog::Log(LOGERROR, "CGBMUtils::%s - failed to create surface", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "CGBMUtils::%s - created surface with size %dx%d", __FUNCTION__,
                                                                         gbm->width,
                                                                         gbm->height);

  return true;
}

void CGBMUtils::DestroyGbm(struct gbm *gbm)
{
  if(gbm->surface)
  {
    gbm_surface_destroy(gbm->surface);
  }

  gbm->surface = nullptr;
}
