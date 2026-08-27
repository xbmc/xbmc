/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/Job.h"
#include "pictures/PictureScalingAlgorithm.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CTexture;
namespace IMAGE_FILES
{
class CImageFileURL;
}

/*!
 \ingroup textures
 \brief Simple class for passing texture detail around
 */
class CTextureDetails
{
public:
  bool operator==(const CTextureDetails &right) const
  {
    return (id    == right.id    &&
            file  == right.file  &&
            width == right.width );
  };

  int id{-1};
  std::string file;
  std::string hash;
  unsigned int width{0};
  unsigned int height{0};
  bool updateable{false};
  bool hashRevalidated{false};
};

/*!
 \ingroup textures
 \brief Job class for caching textures

 Handles loading and caching of textures.
 */
class CTextureCacheJob : public CJob
{
public:
  static constexpr const char* JOB_TYPE_CACHE_IMAGE = "cacheimage";

  /*!
   \param url location of the image
   \param oldDetails what the image is already cached as, if anything. Its hash is only set once
          the image is due its periodic check, and an unchanged image is then revalidated rather
          than cached all over again.
   \param knownHash hash of the source file, if the caller already knows it (see GetImageHash())
   */
  CTextureCacheJob(const std::string& url,
                   const CTextureDetails& oldDetails = {},
                   const std::string& knownHash = "");
  ~CTextureCacheJob() override;

  const char* GetType() const override { return JOB_TYPE_CACHE_IMAGE; }
  bool Equals(const CJob* job) const override;
  bool DoWork() override;

  /*! \brief retrieve a hash for the given image
   Combines the size, ctime and mtime of the image file into a "unique" hash
   \param url location of the image
   \return a hash string for this image
   */
  bool CacheTexture(std::unique_ptr<CTexture>* texture = nullptr);

  static bool ResizeTexture(const std::string& url,
                            unsigned int height,
                            unsigned int width,
                            CPictureScalingAlgorithm::Algorithm scalingAlgorithm,
                            uint8_t*& result,
                            size_t& result_size);

  /*! \brief Retrieve a hash for a file already returned by a directory listing
   \param listedFile the file, as listed by CDirectory::GetDirectory() without DIR_FLAG_NO_FILE_INFO
   \return a hash string for this file, or empty if the listing didn't provide the information -
           not every VFS layer does, and the caller should then let the file be stat'ed as usual
   */
  static std::string GetImageHash(const CFileItem& listedFile);

  std::string m_url;
  CTextureDetails m_oldDetails;
  CTextureDetails m_details;

private:
  /*! \brief Whether the copy this image was previously cached to is still present
   Revalidating leaves that copy in place rather than writing it again, so it has to still be
   there - if it has been deleted the image needs caching again.
   */
  bool HasCachedFile() const;

  /*! \brief retrieve a hash for the given image
   Combines the size, ctime and mtime of the image file into a "unique" hash
   \param url location of the image
   \return a hash string for this image
   */
  static std::string GetImageHashFromStat(const std::string& url);

  /*! \brief Format a hash from a file's modification time and size

   \param modificationTime the file's modification time, as a unix timestamp
   \param size the file's size in bytes
   \return the hash, or empty if neither value was usable
   */
  static std::string FormatImageHash(int64_t modificationTime, int64_t size);

  std::string m_knownHash;

  /*! \brief Load an image at a given target size and orientation.

   Doesn't necessarily load the image at the desired size - the loader *may* decide to load it slightly larger
   or smaller than the desired size for speed reasons.

   \param image the URL of the image file.
   \return a pointer to a CTexture object, NULL if failed.
   */
  static std::unique_ptr<CTexture> LoadImage(const IMAGE_FILES::CImageFileURL& imageURL);

  std::string    m_cachePath;
};

/* \brief Job class for storing the use count of textures
 */
class CTextureUseCountJob : public CJob
{
public:
  explicit CTextureUseCountJob(const std::vector<CTextureDetails> &textures);

  const char* GetType() const override { return "usecount"; }
  bool Equals(const CJob* job) const override;
  bool DoWork() override;

private:
  std::vector<CTextureDetails> m_textures;
};
