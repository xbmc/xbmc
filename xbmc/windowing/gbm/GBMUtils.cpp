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

#include "GBMUtils.h"
#include "utils/log.h"

bool CGBMUtils::CreateDevice(struct gbm *gbm, int fd)
{
  if (gbm->device)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - device already created", __FUNCTION__);

  gbm->device = gbm_create_device(fd);
  if (!gbm->device)
  {
    CLog::Log(LOGERROR, "CGBMUtils::%s - failed to create device", __FUNCTION__);
    return false;
  }

  return true;
}

void CGBMUtils::DestroyDevice(struct gbm *gbm)
{
  if (!gbm->device)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - device already destroyed", __FUNCTION__);

  if (gbm->device)
  {
    gbm_device_destroy(gbm->device);
    gbm->device = nullptr;
  }
}

bool CGBMUtils::CreateSurface(struct gbm *gbm, int width, int height)
{
  if (gbm->surface)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - surface already created", __FUNCTION__);

  gbm->width = width;
  gbm->height = height;

  gbm->surface = gbm_surface_create(gbm->device,
                                    gbm->width,
                                    gbm->height,
                                    GBM_FORMAT_ARGB8888,
                                    GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

  if (!gbm->surface)
  {
    CLog::Log(LOGERROR, "CGBMUtils::%s - failed to create surface", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "CGBMUtils::%s - created surface with size %dx%d", __FUNCTION__,
                                                                         gbm->width,
                                                                         gbm->height);

  return true;
}

void CGBMUtils::DestroySurface(struct gbm *gbm)
{
  if (!gbm->surface)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - surface already destroyed", __FUNCTION__);

  if (gbm->surface)
  {
    ReleaseBuffer(gbm);

    gbm_surface_destroy(gbm->surface);
    gbm->surface = nullptr;
    gbm->width = 0;
    gbm->height = 0;
  }
}

struct gbm_bo *CGBMUtils::LockFrontBuffer(struct gbm *gbm)
{
  if (gbm->next_bo)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - uneven surface buffer usage", __FUNCTION__);

  gbm->next_bo = gbm_surface_lock_front_buffer(gbm->surface);
  return gbm->next_bo;
}

void CGBMUtils::ReleaseBuffer(struct gbm *gbm)
{
  if (gbm->bo)
    gbm_surface_release_buffer(gbm->surface, gbm->bo);

  gbm->bo = gbm->next_bo;
  gbm->next_bo = nullptr;
}
