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

#include "utils/StdString.h"
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
   \param returnDDS if we're allowed to return a DDS version, defaults to true
   \return cached url of this image
   \sa GetCachedImage
   */
  CStdString CheckAndCacheImage(const CStdString &image, bool returnDDS = true);

  /*! \brief retrieve the cached version of the given image (if it exists)
   \param image url of the image
   \return cached url of this image, empty if none exists
   \sa ClearCachedImage
   */
  CStdString GetCachedImage(const CStdString &image);

  /*! \brief clear the cached version of the given image
   \param image url of the image
   \sa GetCachedImage
   */
  void ClearCachedImage(const CStdString &image, bool deleteSource = false);

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

  /*! \brief retrieve a wrapped URL for a thumb file
   \param image name of the file
   \return full wrapped URL of the thumb file
   */
  static CStdString GetWrappedThumbURL(const CStdString &image);

  /*! \brief get a unique image path to associate with the given URL, useful for caching images
   \param url path to retrieve a unique image for
   \param extension type of file we want
   \return a "unique" path to an image with the appropriate extension
   */
  static CStdString GetUniqueImage(const CStdString &url, const CStdString &extension);

private:
  /* \brief Job class for creating .dds versions of textures
   */
  class CDDSJob : public CJob
  {
  public:
    CDDSJob(const CStdString &original);

    virtual const char* GetType() const { return "ddscompress"; };
    virtual bool operator==(const CJob *job) const;
    virtual bool DoWork();

    CStdString m_original;
  };

  /*! \brief Job class for caching textures
   */
  class CCacheJob : public CJob
  {
  public:
    CCacheJob(const CStdString &url, const CStdString &oldHash);

    virtual const char* GetType() const { return "cacheimage"; };
    virtual bool operator==(const CJob *job) const;
    virtual bool DoWork();

    /*! \brief Cache an image either full size or thumb sized
     \param url URL of image to cache
     \param original URL of cached version
     \param oldHash hash of any previously cached version - if the hashes match, we don't cache the image
     \return hash of the image that we cached, empty on failure
     */
    static CStdString CacheImage(const CStdString &url, const CStdString &original, const CStdString &oldHash = "");

    CStdString m_url;
    CStdString m_original;
    CStdString m_hash;
    CStdString m_oldHash;
  };

  // private construction, and no assignements; use the provided singleton methods
  CTextureCache();
  CTextureCache(const CTextureCache&);
  CTextureCache const& operator=(CTextureCache const&);
  virtual ~CTextureCache();

  /*! \brief Check if the given image is a cached image
   \param image url of the image
   \return true if this is a cached image, false otherwise.
   */
  bool IsCachedImage(const CStdString &image) const;

  /*! \brief Add this image to the database
   Thread-safe wrapper of CTextureDatabase::AddCachedTexture
   \param image url of the original image
   \param cacheFile url of the cached image
   \param hash hash of the original image
   \return true if we successfully added to the database, false otherwise.
   */
  bool AddCachedTexture(const CStdString &image, const CStdString &cacheFile, const CStdString &hash);

  /*! \brief Get an image from the database
   Thread-safe wrapper of CTextureDatabase::GetCachedTexture
   \param image url of the original image
   \param cacheFile [out] url of the cached original (if available)
   \return true if we have a cached version of this image, false otherwise.
   */
  bool GetCachedTexture(const CStdString &url, CStdString &cacheFile);

  /*! \brief Clear an image from the database
   Thread-safe wrapper of CTextureDatabase::ClearCachedTexture
   \param image url of the original image
   \param cacheFile [out] url of the cached original (if available)
   \return true if we had a cached version of this image, false otherwise.
   */
  bool ClearCachedTexture(const CStdString &url, CStdString &cacheFile);

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

