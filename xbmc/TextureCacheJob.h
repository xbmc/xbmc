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

class CBaseTexture;

/*!
 \ingroup textures
 \brief Job class for caching textures
 
 Handles loading and caching of textures.
 */
class CTextureCacheJob : public CJob
{
public:
  CTextureCacheJob(const CStdString &url, const CStdString &oldHash);
  CTextureCacheJob(const CStdString &url, const CBaseTexture *texture, unsigned int max_width, unsigned int max_height);
  virtual ~CTextureCacheJob();

  virtual const char* GetType() const { return "cacheimage"; };
  virtual bool operator==(const CJob *job) const;
  virtual bool DoWork();

  /*! \brief Decode an image URL to the underlying image, width, height and orientation
   \param url wrapped URL of the image
   \param width width derived from URL
   \param height height derived from URL
   \param flipped whether the image is flipped horizontally
   \return URL of the underlying image file.
   */
  static CStdString DecodeImageURL(const CStdString &url, unsigned int &width, unsigned int &height, bool &flipped);

  /*! \brief Load an image at a given target size and orientation.

   Doesn't necessarily load the image at the desired size - the loader *may* decide to load it slightly larger
   or smaller than the desired size for speed reasons.

   \param image the URL of the image file.
   \param width the desired maximum width.
   \param height the desired maximum height.
   \param flipped whether the image should be flipped horizontally.
   \return a pointer to a CBaseTexture object, NULL if failed.
   */
  static CBaseTexture *LoadImage(const CStdString &image, unsigned int width, unsigned int height, bool flipped);

  CStdString m_url;
  CStdString m_cacheFile;
  CStdString m_hash;
  CStdString m_oldHash;
private:
  /*! \brief retrieve a hash for the given image
   Combines the size, ctime and mtime of the image file into a "unique" hash
   \param url location of the image
   \return a hash string for this image
   */
  static CStdString GetImageHash(const CStdString &url);

  unsigned int  m_maxWidth;
  unsigned int  m_maxHeight;
  CBaseTexture *m_texture;
  CStdString    m_cachePath;
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
