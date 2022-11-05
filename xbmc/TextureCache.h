/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureCacheJob.h"
#include "TextureDatabase.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/JobManager.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

class CJob;
class CURL;
class CTexture;

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
  CTextureCache();
  ~CTextureCache() override;

  /*! \brief Initialize the texture cache
   */
  void Initialize();

  /*! \brief Deinitialize the texture cache
   */
  void Deinitialize();

  /*! \brief Check whether we already have this image cached

   Check and return URL to cached image if it exists; If not, return empty string.
   If the image is cached, return URL (for original image or .dds version if requested)

   \param image url of the image to check
   \param needsRecaching [out] whether the image needs recaching.
   \return cached url of this image
   \sa GetCachedImage
   */
  std::string CheckCachedImage(const std::string &image, bool &needsRecaching);

  /*! \brief Cache image (if required) using a background job

   Checks firstly whether an image is already cached, and return URL if so [see CheckCacheImage]
   If the image is not yet in the database, a background job is started to
   cache the image and add to the database [see CTextureCacheJob]

   \param image url of the image to cache
   \sa CacheImage
   */
  void BackgroundCacheImage(const std::string &image);

  /*! \brief Updates the in-process list.

   Inserts the image url into the currently processing list 
   to avoid 2 jobs being processed at once

   \param image url of the image to start processing
   \return true if list updated, false otherwise
   \sa CacheImage
   */
  bool StartCacheImage(const std::string& image);

  /*! \brief Cache an image to image cache, optionally return the texture

   Caches the given image, returning the texture if the caller wants it.

   \param image url of the image to cache
   \param texture [out] the loaded image
   \param details [out] details of the cached image
   \return cached url of this image
   \sa CTextureCacheJob::CacheTexture
   */
  std::string CacheImage(const std::string& image,
                         std::unique_ptr<CTexture>* texture = nullptr,
                         CTextureDetails* details = nullptr);

  /*! \brief Cache an image to image cache if not already cached, returning the image details.
   \param image url of the image to cache.
   \param details [out] the image details.
   \return true if the image is in the cache, false otherwise.
   \sa CTextureCacheJob::CacheTexture
   */
  bool CacheImage(const std::string &image, CTextureDetails &details);

  /*! \brief Check whether an image is in the cache
   Note: If the image url won't normally be cached (eg a skin image) this function will return false.
   \param image url of the image
   \return true if the image is cached, false otherwise
   \sa ClearCachedImage
   */
  bool HasCachedImage(const std::string &image);

  /*! \brief clear the cached version of the given image
   \param image url of the image
   \sa GetCachedImage
   */
  void ClearCachedImage(const std::string &image, bool deleteSource = false);

  /*! \brief clear the cached version of the image with given id
   \param database id of the image
   \sa GetCachedImage
   */
  bool ClearCachedImage(int textureID);

  /*! \brief retrieve a cache file (relative to the cache path) to associate with the given image, excluding extension
   Use GetCachedPath(GetCacheFile(url)+extension) for the full path to the file.
   \param url location of the image
   \return a "unique" filename for the associated cache file, excluding extension
   */
  static std::string GetCacheFile(const std::string &url);

  /*! \brief retrieve the full path of the given cached file
   \param file name of the file
   \return full path of the cached file
   */
  static std::string GetCachedPath(const std::string &file);

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
  bool AddCachedTexture(const std::string &image, const CTextureDetails &details);

  /*! \brief Export a (possibly) cached image to a file
   \param image url of the original image
   \param destination url of the destination image, excluding extension.
   \param overwrite whether to overwrite the destination if it exists (TODO: Defaults to false)
   \return true if we successfully exported the file, false otherwise.
   */
  bool Export(const std::string &image, const std::string &destination, bool overwrite);
  bool Export(const std::string &image, const std::string &destination); //! @todo BACKWARD COMPATIBILITY FOR MUSIC THUMBS
private:
  // private construction, and no assignments; use the provided singleton methods
  CTextureCache(const CTextureCache&) = delete;
  CTextureCache const& operator=(CTextureCache const&) = delete;

  /*! \brief Check if the given image is a cached image
   \param image url of the image
   \return true if this is a cached image, false otherwise.
   */
  bool IsCachedImage(const std::string &image) const;

  /*! \brief retrieve the cached version of the given image (if it exists)
   \param image url of the image
   \param details [out] the details of the texture.
   \param trackUsage whether this call should track usage of the image (defaults to false)
   \return cached url of this image, empty if none exists
   \sa ClearCachedImage, CTextureDetails
   */
  std::string GetCachedImage(const std::string &image, CTextureDetails &details, bool trackUsage = false);

  /*! \brief Get an image from the database
   Thread-safe wrapper of CTextureDatabase::GetCachedTexture
   \param image url of the original image
   \param details [out] texture details from the database (if available)
   \return true if we have a cached version of this image, false otherwise.
   */
  bool GetCachedTexture(const std::string &url, CTextureDetails &details);

  /*! \brief Clear an image from the database
   Thread-safe wrapper of CTextureDatabase::ClearCachedTexture
   \param image url of the original image
   \param cacheFile [out] url of the cached original (if available)
   \return true if we had a cached version of this image, false otherwise.
   */
  bool ClearCachedTexture(const std::string &url, std::string &cacheFile);
  bool ClearCachedTexture(int textureID, std::string &cacheFile);

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
  bool SetCachedTextureValid(const std::string &url, bool updateable);

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  /*! \brief Called when a caching job has completed.
   Removes the job from our processing list, updates the database
   and fires a DDS job if appropriate.
   \param success whether the job was successful.
   \param job the caching job.
   */
  void OnCachingComplete(bool success, CTextureCacheJob *job);

  CCriticalSection m_databaseSection;
  CTextureDatabase m_database;
  std::set<std::string> m_processinglist; ///< currently processing list to avoid 2 jobs being processed at once
  CCriticalSection     m_processingSection;
  CEvent               m_completeEvent; ///< Set whenever a job has finished
  std::vector<CTextureDetails> m_useCounts; ///< Use count tracking
  CCriticalSection             m_useCountSection;
};

