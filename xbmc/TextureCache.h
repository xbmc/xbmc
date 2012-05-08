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

#include <set>
#include "utils/StdString.h"
#include "utils/JobManager.h"
#include "TextureDatabase.h"
#include "threads/Event.h"

class CBaseTexture;

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

  /*! \brief Check whether we already have this image cached

   Check and return URL to cached image if it exists; If not, return empty string.
   If the image is cached, return URL (for original image or .dds version if requested)
   Creates a .dds of image if requested via returnDDS and the image doesn't need recaching.

   \param image url of the image to check
   \param returnDDS if we're allowed to return a DDS version, defaults to true
   \param needsRecaching [out] whether the image needs recaching.
   \return cached url of this image
   \sa GetCachedImage
   */ 
  CStdString CheckCachedImage(const CStdString &image, bool returnDDS, bool &needsRecaching);

  /*! \brief This function is a wrapper around CheckCacheImage and CacheImageFile.
  
   Checks firstly whether an image is already cached, and return URL if so [see CheckCacheImage]
   If the image is not yet in the database it is cached and added to the database [see CacheImageFile]

   \param image url of the image to check and cache
   \param returnDDS if we're allowed to return a DDS version, defaults to true
   \return cached url of this image
   \sa CheckCacheImage
   */
  CStdString CheckAndCacheImage(const CStdString &image, bool returnDDS = true);

  /*! \brief Cache image (if required) using a background job

   Essentially a threaded version of CheckAndCacheImage.

   \param image url of the image to cache
   \sa CheckAndCacheImage
   */
  void BackgroundCacheImage(const CStdString &image);

  /*! \brief Cache an image to image cache, optionally return the texture

   Caches the given image, returning the texture if the caller wants it.

   \param image url of the image to cache
   \param texture [out] the loaded image
   \return cached url of this image
   \sa CTextureCacheJob::CacheTexture
   */
  CStdString CacheTexture(const CStdString &url, CBaseTexture **texture = NULL);

  /*! \brief Take image URL and add it to image cache

   Takes the URL to an image. Caches and adds to the database.

   \param image url of the image to cache
   \return cached url of this image
   \sa CTextureCacheJob::DoWork
   */  
  CStdString CacheImageFile(const CStdString &url);

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

  /*! \brief retrieve a cache file (relative to the cache path) to associate with the given image, excluding extension
   Use GetCachedPath(GetCacheFile(url)+extension) for the full path to the file.
   \param url location of the image
   \return a "unique" filename for the associated cache file, excluding extension
   */
  static CStdString GetCacheFile(const CStdString &url);

  /*! \brief retrieve the full path of the given cached file
   \param file name of the file
   \return full path of the cached file
   */
  static CStdString GetCachedPath(const CStdString &file);

  /*! \brief retrieve a wrapped URL for a image file
   \param image name of the file
   \param type signifies a special type of image (eg embedded video thumb, picture folder thumb)
   \param options which options we need (eg size=thumb)
   \return full wrapped URL of the image file
   */
  static CStdString GetWrappedImageURL(const CStdString &image, const CStdString &type, const CStdString &options = "");
  static CStdString GetWrappedThumbURL(const CStdString &image);

  /*! \brief get a unique image path to associate with the given URL, useful for caching images
   \param url path to retrieve a unique image for
   \param extension type of file we want
   \return a "unique" path to an image with the appropriate extension
   */
  static CStdString GetUniqueImage(const CStdString &url, const CStdString &extension);

  /*! \brief Add this image to the database
   Thread-safe wrapper of CTextureDatabase::AddCachedTexture
   \param image url of the original image
   \param details the texture details to add
   \return true if we successfully added to the database, false otherwise.
   */
  bool AddCachedTexture(const CStdString &image, const CTextureDetails &details);

  /*! \brief Export a (possibly) cached image to a file
   \param image url of the original image
   \param destination url of the destination image
   \return true if we successfully exported the file, false otherwise.
   */
  bool Export(const CStdString &image, const CStdString &destination);
private:
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

  /*! \brief retrieve the cached version of the given image (if it exists)
   \param image url of the image
   \param cacheHash [out] set to the hash of the cached image if it needs checking
   \return cached url of this image, empty if none exists
   \sa ClearCachedImage
   */
  CStdString GetCachedImage(const CStdString &image, CStdString &cacheHash);

  /*! \brief Get an image from the database
   Thread-safe wrapper of CTextureDatabase::GetCachedTexture
   \param image url of the original image
   \param details [out] texture details from the database (if available)
   \return true if we have a cached version of this image, false otherwise.
   */
  bool GetCachedTexture(const CStdString &url, CTextureDetails &details);

  /*! \brief Clear an image from the database
   Thread-safe wrapper of CTextureDatabase::ClearCachedTexture
   \param image url of the original image
   \param cacheFile [out] url of the cached original (if available)
   \return true if we had a cached version of this image, false otherwise.
   */
  bool ClearCachedTexture(const CStdString &url, CStdString &cacheFile);

  /*! \brief Increment the use count of a texture in the database
   Thread-safe wrapper of CTextureDatabase::IncrementUseCount
   \param details the texture to increment the use count
   \return true on success, false otherwise
   */
  bool IncrementUseCount(const CTextureDetails &details);

  /*! \brief Set a previously cached texture as valid in the database
   Thread-safe wrapper of CTextureDatabase::SetCachedTextureValid
   \param image url of the original image
   \param updateable whether this image should be checked for updates
   \return true if successful, false otherwise.
   */
  bool SetCachedTextureValid(const CStdString &url, bool updateable);

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  virtual void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job);

  CCriticalSection m_databaseSection;
  CTextureDatabase m_database;
  std::set<CStdString> m_processing; ///< currently processing list to avoid 2 jobs being processed at once
  CCriticalSection     m_processingSection;
  CEvent               m_completeEvent; ///< Set whenever a job has finished
};

