/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "StdString.h"

/*!
 \ingroup textures
 \brief Texture cache class for handling the caching of images.

 Manages the caching of images for use as control textures. Images are cached
 both as originals (direct copies) and as .dds textures for fast loading. Images
 may be periodically checked for updates and may be purged from the cache if
 unused for a set period of time.
 
 */
class CTextureCache
{
public:
  /*!
   \brief The only way through which the global instance of the CTextureCache should be accessed.
   \return the global instance.
   */
  static CTextureCache &Get();

  /*! \brief check whether we have this image cached, and change it's url if so
   Checks firstly whether the image should be cached (i.e. is the image already in the cache).
   If the image is already in the cache, we check whether we have a faster loading (.dds) version,
   and if not, create one.
   \param image url of the image to cache
   \return cached url of this image
   */
   CStdString CheckAndCacheImage(const CStdString &image);

private:
  // private construction, and no assignements; use the provided singleton methods
  CTextureCache();
  CTextureCache(const CTextureCache&);
  CTextureCache const& operator=(CTextureCache const&);
  virtual ~CTextureCache();

  /*! \brief cache a given image as a .dds file
   \param image url of the image to cache
   \param ddsImage url of the resulting dds image
   \param maxMSE maximal mean square error (per pixel) allowed, ignored if 0 (the default)
   \return true if we successfully generated a dds file, false otherwise
   */
  bool CacheDDS(const CStdString &image, const CStdString &ddsImage, double maxMSE = 0) const;
};

