/*
 *      Copyright (C) 2012 Team XBMC
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

#include "utils/StdString.h"
#include "utils/Job.h"

/*!
 \ingroup textures
 \brief Job class for caching textures
 
 Handles loading and caching of textures.
 */
class CTextureCacheJob : public CJob
{
public:
  CTextureCacheJob(const CStdString &url, const CStdString &oldHash);

  virtual const char* GetType() const { return "cacheimage"; };
  virtual bool operator==(const CJob *job) const;
  virtual bool DoWork();

  /*! \brief Cache an image either full size or thumb sized
   \param url URL of image to cache
   \param relativeCacheFile relative URL of cached version
   \param oldHash hash of any previously cached version - if the hashes match, we don't cache the image
   \return hash of the image that we cached, empty on failure
   */
  static CStdString CacheImage(const CStdString &url, const CStdString &relativeCacheFile, const CStdString &oldHash = "");

  CStdString m_url;
  CStdString m_relativeCacheFile;
  CStdString m_hash;
  CStdString m_oldHash;
private:
  /*! \brief retrieve a hash for the given image
   Combines the size, ctime and mtime of the image file into a "unique" hash
   \param url location of the image
   \return a hash string for this image
   */
  static CStdString GetImageHash(const CStdString &url);
};

/* \brief Job class for creating .dds versions of textures
 */
class CTextureDDSJob : public CJob
{
public:
  CTextureDDSJob(const CStdString &original);

  virtual const char* GetType() const { return "ddscompress"; };
  virtual bool operator==(const CJob *job) const;
  virtual bool DoWork();

  CStdString m_original;
};
