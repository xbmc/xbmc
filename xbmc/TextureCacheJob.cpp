/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureCacheJob.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/audiodecoder.h"
#include "commons/ilog.h"
#include "filesystem/File.h"
#include "guilib/Texture.h"
#include "imagefiles/ImageFileURL.h"
#include "imagefiles/SpecialImageLoaderFactory.h"
#include "pictures/Picture.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <utility>

#include "PlatformDefs.h"

CTextureCacheJob::CTextureCacheJob(const std::string &url, const std::string &oldHash):
  m_url(url),
  m_oldHash(oldHash),
  m_cachePath(CTextureCache::GetCacheFile(m_url))
{
}

CTextureCacheJob::~CTextureCacheJob() = default;

bool CTextureCacheJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CTextureCacheJob* cacheJob = dynamic_cast<const CTextureCacheJob*>(job);
    if (cacheJob && cacheJob->m_cachePath == m_cachePath)
      return true;
  }
  return false;
}

bool CTextureCacheJob::DoWork()
{
  if (ShouldCancel(0, 0))
    return false;

  // check whether we need cache the job anyway
  bool needsRecaching = false;
  std::string path(CServiceBroker::GetTextureCache()->CheckCachedImage(m_url, needsRecaching));
  if (!path.empty() && !needsRecaching)
    return false;
  if (CServiceBroker::GetTextureCache()->StartCacheImage(m_url))
    return CacheTexture();

  return false;
}

namespace
{
// Most PVR images use "type" to signify 'ownership' of basic images for easy
// cache cleaning, rather than special generated images
bool IsPVROwnedImage(const std::string& specialType)
{
  return specialType == "pvrchannel_radio" || specialType == "pvrchannel_tv" ||
         specialType == "pvrprovider" || specialType == "pvrrecording" ||
         StringUtils::StartsWith(specialType, "epgtag_");
}

// special generated images and images served via HTTP should not be regularly checked for changes
bool ShouldCheckForChanges(const std::string& specialType, const std::string& url)
{
  const bool isSpecialImage = !specialType.empty() && !IsPVROwnedImage(specialType);
  if (isSpecialImage)
    return false;

  const bool isHTTP =
      StringUtils::StartsWith(url, "http://") || StringUtils::StartsWith(url, "https://");
  return !isHTTP;
}
} // namespace

bool CTextureCacheJob::CacheTexture(std::unique_ptr<CTexture>* out_texture)
{
  IMAGE_FILES::CImageFileURL imageURL{m_url};

  const auto& image = imageURL.GetTargetFile();
  m_details.updateable = ShouldCheckForChanges(imageURL.GetSpecialType(), image);

  if (m_details.updateable)
  {
    // generate the hash
    m_details.hash = GetImageHash(image);
    if (m_details.hash.empty())
      return false;

    if (m_details.hash == m_oldHash)
    {
      m_details.hashRevalidated = true;
      return true;
    }
  }

  std::unique_ptr<CTexture> texture = LoadImage(imageURL);
  if (texture)
  {
    if (texture->HasAlpha())
      m_details.file = m_cachePath + ".png";
    else
      m_details.file = m_cachePath + ".jpg";

    CLog::Log(LOGDEBUG, "{} image '{}' to '{}':", m_oldHash.empty() ? "Caching" : "Recaching",
              CURL::GetRedacted(image), m_details.file);

    unsigned int cached_width = 0;
    unsigned int cached_height = 0;
    if (CPicture::CacheTexture(texture.get(), cached_width, cached_height,
                               CTextureCache::GetCachedPath(m_details.file)))
    {
      m_details.width = cached_width;
      m_details.height = cached_height;
      if (out_texture) // caller wants the texture
        *out_texture = std::move(texture);
      return true;
    }
  }
  return false;
}

bool CTextureCacheJob::ResizeTexture(const std::string& url,
                                     unsigned int height,
                                     unsigned int width,
                                     CPictureScalingAlgorithm::Algorithm scalingAlgorithm,
                                     uint8_t*& result,
                                     size_t& result_size)
{
  result = NULL;
  result_size = 0;

  const IMAGE_FILES::CImageFileURL imageURL{url};
  const auto& image = imageURL.GetTargetFile();
  if (image.empty())
    return false;

  std::unique_ptr<CTexture> texture = LoadImage(imageURL);
  if (texture == NULL)
    return false;

  bool success = CPicture::ResizeTexture(image, texture.get(), width, height, result, result_size,
                                         scalingAlgorithm);

  return success;
}

std::unique_ptr<CTexture> CTextureCacheJob::LoadImage(const IMAGE_FILES::CImageFileURL& imageURL)
{
  if (imageURL.IsSpecialImage())
  {
    IMAGE_FILES::CSpecialImageLoaderFactory specialImageLoader{};
    auto texture = specialImageLoader.Load(imageURL);
    if (texture)
      return texture;
  }

  // Validate file URL to see if it is an image
  CFileItem file(imageURL.GetTargetFile(), false);
  file.FillInMimeType();
  if (!(file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ())) &&
      !StringUtils::StartsWithNoCase(file.GetMimeType(), "image/") &&
      !StringUtils::EqualsNoCase(file.GetMimeType(),
                                 "application/octet-stream")) // ignore non-pictures
  {
    return {};
  }

  auto texture = CTexture::LoadFromFile(imageURL.GetTargetFile(), 0, 0, CAspectRatio::CENTER,
                                        file.GetMimeType());
  if (!texture)
    return {};

  // EXIF bits are interpreted as: <flipXY><flipY*flipX><flipX>
  // where to undo the operation we apply them in reverse order <flipX>*<flipY*flipX>*<flipXY>
  // When flipped we have an additional <flipX> on the left, which is equivalent to toggling the last bit
  if (imageURL.flipped)
    texture->SetOrientation(texture->GetOrientation() ^ 1);

  return texture;
}

std::string CTextureCacheJob::GetImageHash(const std::string &url)
{
  // silently ignore - we cannot stat these
  // in the case of upnp thumbs are/should be provided when filling the directory list, there's no reason to stat all object ids
  if (URIUtils::IsProtocol(url, "addons") || URIUtils::IsProtocol(url, "plugin") ||
      URIUtils::IsProtocol(url, "upnp"))
    return "";

  struct __stat64 st;
  if (XFILE::CFile::Stat(url, &st) == 0)
  {
    int64_t time = st.st_mtime;
    if (!time)
      time = st.st_ctime;
    if (time || st.st_size)
      return StringUtils::Format("d{}s{}", time, st.st_size);

    // the image exists but we couldn't determine the mtime/ctime and/or size
    // so set an obviously bad hash
    return "BADHASH";
  }

  CLog::Log(LOGDEBUG, "{} - unable to stat url {}", __FUNCTION__, CURL::GetRedacted(url));
  return "";
}

CTextureUseCountJob::CTextureUseCountJob(const std::vector<CTextureDetails> &textures) : m_textures(textures)
{
}

bool CTextureUseCountJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CTextureUseCountJob* useJob = dynamic_cast<const CTextureUseCountJob*>(job);
    if (useJob && useJob->m_textures == m_textures)
      return true;
  }
  return false;
}

bool CTextureUseCountJob::DoWork()
{
  CTextureDatabase db;
  if (db.Open())
  {
    db.BeginTransaction();
    for (std::vector<CTextureDetails>::const_iterator i = m_textures.begin(); i != m_textures.end(); ++i)
      db.IncrementUseCount(*i);
    db.CommitTransaction();
  }
  return true;
}
