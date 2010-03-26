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
#include "utils/JobManager.h"
#include "TextureDatabase.h"

/*!
 \ingroup textures
 \brief Texture cache class for handling the caching of images.

 Manages the caching of images for use as control textures. Images are cached
 both as originals (direct copies) and as .dds textures for fast loading. Images
 may be periodically checked for updates and may be purged from the cache if
 unused for a set period of time.
 
 */
class CTextureCache : public CJobQueue
{
public:
  /*!
   \brief The only way through which the global instance of the CTextureCache should be accessed.
   \return the global instance.
   */
  static CTextureCache &Get();

  /*! \brief Initalize the texture cache
   */
  void Initialize();

  /*! \brief Deinitialize the texture cache
   */
  void Deinitialize();

  /*! \brief check whether we have this image cached, and change it's url if so

   Checks firstly whether the image should be cached (i.e. is the image a .dds file?).
   If the image isn't a .dds file, we lookup the database for a cached version and use
   that if available. If the image is not yet in the database it is cached and added
   to the database.

   \param image url of the image to cache
   \return cached url of this image
   \sa CacheImageToDB
   */
  CStdString CheckAndCacheImage(const CStdString &image);

  /*! \brief clear the cached version of the given image
   \param image url of the image to cache
   \return cached url of this image
   \sa CacheImageToDB
   */
  void ClearCachedImage(const CStdString &image);

  /*! \brief retrieve the cache file to associate with the given image
   \param url location of the image
   \return a "unique" filename for the associated cache file.
   */
  static CStdString GetCacheFile(const CStdString &url);

  /*! \brief retrieve the full path of the given cached file
   \param file name of the file
   \return full path of the cached file
   */
  static CStdString GetCachedPath(const CStdString &file);

private:
  /* \brief Job class for creating .dds versions of textures
   */
  class CDDSJob : public CJob
  {
  public:
    CDDSJob(const CStdString &url, const CStdString &original);
    
    virtual const char* GetType() const { return "ddscache"; };
    virtual bool operator==(const CJob *job) const;
    virtual bool DoWork();
    
    CStdString m_url;
    CStdString m_original;
    CStdString m_dds;
  };

  // private construction, and no assignements; use the provided singleton methods
  CTextureCache();
  CTextureCache(const CTextureCache&);
  CTextureCache const& operator=(CTextureCache const&);
  virtual ~CTextureCache();

  /*! \brief Add this image to the database
   Thread-safe wrapper of CTextureDatabase::AddCachedTexture
   \param image url of the original image
   \param cacheFile url of the cached image
   \param ddsFile url of the dds version of the cached image
   \param hash hash of the original image
   \return true if we successfully added to the database, false otherwise.
   */
  bool AddCachedTexture(const CStdString &image, const CStdString &cacheFile, const CStdString &ddsFile, const CStdString &hash);

  /*! \brief Get an image from the database
   Thread-safe wrapper of CTextureDatabase::GetCachedTexture
   \param image url of the original image
   \param cacheFile [out] url of the cached original (if available)
   \param ddsFile [out] url of the dds original (if available)
   \return true if we have a cached version of this image, false otherwise.
   */
  bool GetCachedTexture(const CStdString &url, CStdString &cacheFile, CStdString &ddsFile);

  /*! \brief Clear an image from the database
   Thread-safe wrapper of CTextureDatabase::ClearCachedTexture
   \param image url of the original image
   \param cacheFile [out] url of the cached original (if available)
   \param ddsFile [out] url of the dds original (if available)
   \return true if we had a cached version of this image, false otherwise.
   */
  bool ClearCachedTexture(const CStdString &url, CStdString &cacheFile, CStdString &ddsFile);

  /*! \brief retrieve a hash for the given image
   Combines the size, ctime and mtime of the image file into a "unique" hash
   \param url location of the image
   \return a hash string for this image
   */
  CStdString GetImageHash(const CStdString &url) const;

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  CCriticalSection m_databaseSection;
  CTextureDatabase m_database;
};

