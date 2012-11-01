
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

namespace XFILE
{

struct SNativeIoControl
{
  int   request;
  void* param;
};

struct SCacheStatus
{
  uint64_t forward;  /**< number of bytes cached forward of current position */
  unsigned maxrate;  /**< maximum number of bytes per second cache is allowed to fill */
  unsigned currate;  /**< average read rate from source file since last position change */
  bool     full;     /**< is the cache full */
};

typedef enum {
  IOCTRL_NATIVE        = 1, /**< SNativeIoControl structure, containing what should be passed to native ioctrl */
  IOCTRL_SEEK_POSSIBLE = 2, /**< return 0 if known not to work, 1 if it should work */
  IOCTRL_CACHE_STATUS  = 3, /**< SCacheStatus structure */
  IOCTRL_CACHE_SETRATE = 4, /**< unsigned int with speed limit for caching in bytes per second */
  IOCTRL_SET_CACHE    = 8, /** <CFileCache */
} EIoControl;

}
