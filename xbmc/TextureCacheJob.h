/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "pictures/PictureScalingAlgorithm.h"
#include "utils/Job.h"

class CBaseTexture;

/*!
 \ingroup textures
 \brief Simple class for passing texture detail around
 */
class CTextureDetails
{
public:
  CTextureDetails()
  {
    id = -1;
    width = height = 0;
    updateable = false;
  };
  bool operator==(const CTextureDetails &right) const
  {
    return (id    == right.id    &&
            file  == right.file  &&
            width == right.width );
  };
  int          id;
  std::string  file;
  std::string  hash;
  unsigned int width;
  unsigned int height;
  bool         updateable;
};

/*!
 \ingroup textures
 \brief Job class for caching textures

 Handles loading and caching of textures.
 */
class CTextureCacheJob : public CJob
{
public:
  CTextureCacheJob(const std::string &url, const std::string &oldHash = "");
  ~CTextureCacheJob() override;

  const char* GetType() const override { return kJobTypeCacheImage; };
  bool operator==(const CJob *job) const override;
  bool DoWork() override;

  /*! \brief retrieve a hash for the given image
   Combines the size, ctime and mtime of the image file into a "unique" hash
   \param url location of the image
   \return a hash string for this image
   */
  bool CacheTexture(CBaseTexture **texture = NULL);

  static bool ResizeTexture(const std::string &url, uint8_t* &result, size_t &result_size);

  std::string m_url;
  std::string m_oldHash;
  CTextureDetails m_details;
private:
  /*! \brief retrieve a hash for the given image
   Combines the size, ctime and mtime of the image file into a "unique" hash
   \param url location of the image
   \return a hash string for this image
   */
  static std::string GetImageHash(const std::string &url);

  /*! \brief Check whether a given URL represents an image that can be updated
   We currently don't check http:// and https:// URLs for updates, under the assumption that
   a image URL is much more likely to be static and the actual image at the URL is unlikely
   to change, so no point checking all the time.
   \param url the url to check
   \return true if the image given by the URL should be checked for updates, false otherwise
   */
  bool UpdateableURL(const std::string &url) const;

  /*! \brief Decode an image URL to the underlying image, width, height and orientation
   \param url wrapped URL of the image
   \param width width derived from URL
   \param height height derived from URL
   \param scalingAlgorithm scaling algorithm derived from URL
   \param additional_info additional information, such as "flipped" to flip horizontally
   \return URL of the underlying image file.
   */
  static std::string DecodeImageURL(const std::string &url, unsigned int &width, unsigned int &height, CPictureScalingAlgorithm::Algorithm& scalingAlgorithm, std::string &additional_info);

  /*! \brief Load an image at a given target size and orientation.

   Doesn't necessarily load the image at the desired size - the loader *may* decide to load it slightly larger
   or smaller than the desired size for speed reasons.

   \param image the URL of the image file.
   \param width the desired maximum width.
   \param height the desired maximum height.
   \param additional_info extra info for loading, such as whether to flip horizontally.
   \return a pointer to a CBaseTexture object, NULL if failed.
   */
  static CBaseTexture *LoadImage(const std::string &image, unsigned int width, unsigned int height, const std::string &additional_info, bool requirePixels = false);

  std::string    m_cachePath;
};

/* \brief Job class for storing the use count of textures
 */
class CTextureUseCountJob : public CJob
{
public:
  explicit CTextureUseCountJob(const std::vector<CTextureDetails> &textures);

  const char* GetType() const override { return "usecount"; };
  bool operator==(const CJob *job) const override;
  bool DoWork() override;

private:
  std::vector<CTextureDetails> m_textures;
};
