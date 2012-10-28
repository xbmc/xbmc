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

#include <set>
#include "utils/StdString.h"
#include "utils/JobManager.h"
#include "TextureDatabase.h"
#include "threads/Event.h"

class CURL;
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

  /*! \brief Cache image (if required) using a background job

   Checks firstly whether an image is already cached, and return URL if so [see CheckCacheImage]
   If the image is not yet in the database, a background job is started to
   cache the image and add to the database [see CTextureCacheJob]

   \param image url of the image to cache
   \sa CacheImage
   */
  void BackgroundCacheImage(const CStdString &image);

  /*! \brief Cache an image to image cache, optionally return the texture

   Caches the given image, returning the texture if the caller wants it.

   \param image url of the image to cache
   \param texture [out] the loaded image
   \param details [out] details of the cached image
   \return cached url of this image
   \sa CTextureCacheJob::CacheTexture
   */
  CStdString CacheImage(const CStdString &url, CBaseTexture **texture = NULL, CTextureDetails *details = NULL);

  /*! \brief Cache an image to image cache if not already cached, returning the image details.
   \param image url of the image to cache.
   \param details [out] the image details.
   \return true if the image is in the cache, false otherwise.
   \sa CTextureCacheJob::CacheTexture
   */
  bool CacheImage(const CStdString &image, CTextureDetails &details);

  /*! \brief Check whether an image is in the cache
   Note: If the image url won't normally be cached (eg a skin image) this function will return false.
   \param image url of the image
   \return true if the image is cached, false otherwise
   \sa ClearCachedImage
   */
  bool HasCachedImage(const CStdString &image);

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
  static CStdString GetWrappedImageURL(const CStdString &image, const CStdString &type = "", const CStdString &options = "");
  static CStdString GetWrappedThumbURL(const CStdString &image);

  /*! \brief Unwrap an image://<url_encoded_path> style URL
   Such urls are used for art over the webserver or other users of the VFS
   \param image url of the image
   \return the unwrapped URL, or the original URL if unwrapping is inappropriate.
   */
  static CStdString UnwrapImageURL(const CStdString &image);

  /*! \brief check whether an image:// URL may be cached
   \param url the URL to the image
   \return true if the given URL may be cached, false otherwise
   */
  static bool CanCacheImageURL(const CURL &url);

  /*! \brief Add this image to the database
   Thread-safe wrapper of CTextureDatabase::AddCachedTexture
   \param image url of the original image
   \param details the texture details to add
   \return true if we successfully added to the database, false otherwise.
   */
  bool AddCachedTexture(const CStdString &image, const CTextureDetails &details);

  /*! \brief Export a (possibly) cached image to a file
   \param image url of the original image
   \param destination url of the destination image, excluding extension.
   \param overwrite whether to overwrite the destination if it exists (TODO: Defaults to false)
   \return true if we successfully exported the file, false otherwise.
   */
  bool Export(const CStdString &image, const CStdString &destination, bool overwrite);
  bool Export(const CStdString &image, const CStdString &destination); // TODO: BACKWARD COMPATIBILITY FOR MUSIC THUMBS
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
   \param details [out] the details of the texture.
   \param trackUsage whether this call should track usage of the image (defaults to false)
   \return cached url of this image, empty if none exists
   \sa ClearCachedImage, CTextureDetails
   */
  CStdString GetCachedImage(const CStdString &image, CTextureDetails &details, bool trackUsage = false);

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

  /*! \brief Increment the use count of a texture
   Stores locally before calling CTextureDatabase::IncrementUseCount via a CUseCountJob
   \sa CUseCountJob, CTextureDatabase::IncrementUseCount
   */
  void IncrementUseCount(const CTextureDetails &details);

  /*! \brief Set a previously cached texture as valid in the database
   Thread-safe wrapper of CTextureDatabase::SetCachedTextureValid
   \param image url of the original image
   \param updateable whether this image should be checked for updates
   \return true if successful, false otherwise.
   */
  bool SetCachedTextureValid(const CStdString &url, bool updateable);

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  virtual void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job);

  /*! \brief Called when a caching job has completed.
   Removes the job from our processing list, updates the database
   and fires a DDS job if appropriate.
   \param success whether the job was successful.
   \param job the caching job.
   */
  void OnCachingComplete(bool success, CTextureCacheJob *job);

  CCriticalSection m_databaseSection;
  CTextureDatabase m_database;
  std::set<CStdString> m_processing; ///< currently processing list to avoid 2 jobs being processed at once
  CCriticalSection     m_processingSection;
  CEvent               m_completeEvent; ///< Set whenever a job has finished
  std::vector<CTextureDetails> m_useCounts; ///< Use count tracking
  CCriticalSection             m_useCountSection;
};

