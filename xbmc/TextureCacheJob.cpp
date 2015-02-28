/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "TextureCacheJob.h"
#include "TextureCache.h"
#include "guilib/Texture.h"
#include "guilib/DDSImage.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "pictures/Picture.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "FileItem.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"
#if defined(HAS_OMXPLAYER)
#include "cores/omxplayer/OMXImage.h"
#endif

CTextureCacheJob::CTextureCacheJob(const std::string &url, const std::string &oldHash):
  m_url(url),
  m_oldHash(oldHash),
  m_cachePath(CTextureCache::GetCacheFile(m_url))
{
}

CTextureCacheJob::~CTextureCacheJob()
{
}

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
  if (ShouldCancel(1, 0)) // HACK: second check is because we cancel the job in the first callback, but we don't detect it
    return false;         //       until the second

  // check whether we need cache the job anyway
  bool needsRecaching = false;
  std::string path(CTextureCache::Get().CheckCachedImage(m_url, false, needsRecaching));
  if (!path.empty() && !needsRecaching)
    return false;
  return CacheTexture();
}

bool CTextureCacheJob::CacheTexture(CBaseTexture **out_texture)
{
  // unwrap the URL as required
  std::string additional_info;
  unsigned int width, height;
  std::string image = DecodeImageURL(m_url, width, height, additional_info);

  m_details.updateable = additional_info != "music" && UpdateableURL(image);

  // generate the hash
  m_details.hash = GetImageHash(image);
  if (m_details.hash.empty())
    return false;
  else if (m_details.hash == m_oldHash)
    return true;

#if defined(HAS_OMXPLAYER)
  if (COMXImage::CreateThumb(image, width, height, additional_info, CTextureCache::GetCachedPath(m_cachePath + ".jpg")))
  {
    m_details.width = width;
    m_details.height = height;
    m_details.file = m_cachePath + ".jpg";
    if (out_texture)
      *out_texture = LoadImage(CTextureCache::GetCachedPath(m_details.file), width, height, "" /* already flipped */);
    CLog::Log(LOGDEBUG, "Fast %s image '%s' to '%s': %p", m_oldHash.empty() ? "Caching" : "Recaching", image.c_str(), m_details.file.c_str(), out_texture);
    return true;
  }
#endif
  CBaseTexture *texture = LoadImage(image, width, height, additional_info, true);
  if (texture)
  {
    if (texture->HasAlpha())
      m_details.file = m_cachePath + ".png";
    else
      m_details.file = m_cachePath + ".jpg";

    CLog::Log(LOGDEBUG, "%s image '%s' to '%s':", m_oldHash.empty() ? "Caching" : "Recaching", image.c_str(), m_details.file.c_str());

    if (CPicture::CacheTexture(texture, width, height, CTextureCache::GetCachedPath(m_details.file)))
    {
      m_details.width = width;
      m_details.height = height;
      if (out_texture) // caller wants the texture
        *out_texture = texture;
      else
        delete texture;
      return true;
    }
  }
  delete texture;
  return false;
}

bool CTextureCacheJob::ResizeTexture(const std::string &url, uint8_t* &result, size_t &result_size)
{
  result = NULL;
  result_size = 0;

  if (url.empty())
    return false;

  // unwrap the URL as required
  std::string additional_info;
  unsigned int width, height;
  std::string image = DecodeImageURL(url, width, height, additional_info);
  if (image.empty())
    return false;

  CBaseTexture *texture = LoadImage(image, width, height, additional_info, true);
  if (texture == NULL)
    return false;

  bool success = CPicture::ResizeTexture(image, texture, width, height, result, result_size);
  delete texture;

  return success;
}

std::string CTextureCacheJob::DecodeImageURL(const std::string &url, unsigned int &width, unsigned int &height, std::string &additional_info)
{
  // unwrap the URL as required
  std::string image(url);
  additional_info.clear();
  width = height = 0;
  if (StringUtils::StartsWith(url, "image://"))
  {
    // format is image://[type@]<url_encoded_path>?options
    CURL thumbURL(url);

    if (!CTextureCache::CanCacheImageURL(thumbURL))
      return "";
    if (thumbURL.GetUserName() == "music")
      additional_info = "music";

    image = thumbURL.GetHostName();

    if (thumbURL.HasOption("flipped"))
      additional_info = "flipped";

    if (thumbURL.GetOption("size") == "thumb")
      width = height = g_advancedSettings.GetThumbSize();
    else
    {
      if (thumbURL.HasOption("width") && StringUtils::IsInteger(thumbURL.GetOption("width")))
        width = strtol(thumbURL.GetOption("width").c_str(), NULL, 0);
      if (thumbURL.HasOption("height") && StringUtils::IsInteger(thumbURL.GetOption("height")))
        height = strtol(thumbURL.GetOption("height").c_str(), NULL, 0);
    }
  }
  return image;
}

CBaseTexture *CTextureCacheJob::LoadImage(const std::string &image, unsigned int width, unsigned int height, const std::string &additional_info, bool requirePixels)
{
  if (additional_info == "music")
  { // special case for embedded music images
    MUSIC_INFO::EmbeddedArt art;
    if (CMusicThumbLoader::GetEmbeddedThumb(image, art))
      return CBaseTexture::LoadFromFileInMemory(&art.data[0], art.size, art.mime, width, height);
  }

  // Validate file URL to see if it is an image
  CFileItem file(image, false);
  file.FillInMimeType();
  if (!(file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ() ))
      && !StringUtils::StartsWithNoCase(file.GetMimeType(), "image/") && !StringUtils::EqualsNoCase(file.GetMimeType(), "application/octet-stream")) // ignore non-pictures
    return NULL;

  CBaseTexture *texture = CBaseTexture::LoadFromFile(image, width, height, CSettings::Get().GetBool("pictures.useexifrotation"), requirePixels, file.GetMimeType());
  if (!texture)
    return NULL;

  // EXIF bits are interpreted as: <flipXY><flipY*flipX><flipX>
  // where to undo the operation we apply them in reverse order <flipX>*<flipY*flipX>*<flipXY>
  // When flipped we have an additional <flipX> on the left, which is equivalent to toggling the last bit
  if (additional_info == "flipped")
    texture->SetOrientation(texture->GetOrientation() ^ 1);

  return texture;
}

bool CTextureCacheJob::UpdateableURL(const std::string &url) const
{
  // we don't constantly check online images
  if (StringUtils::StartsWith(url, "http://") ||
      StringUtils::StartsWith(url, "https://"))
    return false;
  return true;
}

std::string CTextureCacheJob::GetImageHash(const std::string &url)
{
  struct __stat64 st;
  if (XFILE::CFile::Stat(url, &st) == 0)
  {
    int64_t time = st.st_mtime;
    if (!time)
      time = st.st_ctime;
    if (time || st.st_size)
      return StringUtils::Format("d%" PRId64"s%" PRId64, time, st.st_size);;

    // the image exists but we couldn't determine the mtime/ctime and/or size
    // so set an obviously bad hash
    return "BADHASH";
  }
  CLog::Log(LOGDEBUG, "%s - unable to stat url %s", __FUNCTION__, url.c_str());
  return "";
}

CTextureDDSJob::CTextureDDSJob(const std::string &original):
  m_original(original)
{
}

bool CTextureDDSJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CTextureDDSJob* ddsJob = dynamic_cast<const CTextureDDSJob*>(job);
    if (ddsJob && ddsJob->m_original == m_original)
      return true;
  }
  return false;
}

bool CTextureDDSJob::DoWork()
{
  if (URIUtils::HasExtension(m_original, ".dds"))
    return false;
  CBaseTexture *texture = CBaseTexture::LoadFromFile(m_original);
  if (texture)
  { // convert to DDS
    CDDSImage dds;
    CLog::Log(LOGDEBUG, "Creating DDS version of: %s", m_original.c_str());
    bool ret = dds.Create(URIUtils::ReplaceExtension(m_original, ".dds"), texture->GetWidth(), texture->GetHeight(), texture->GetPitch(), texture->GetPixels(), 40);
    delete texture;
    return ret;
  }
  return false;
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
