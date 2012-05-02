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

#include "TextureCacheJob.h"
#include "TextureCache.h"
#include "guilib/Texture.h"
#include "guilib/DDSImage.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "pictures/Picture.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "FileItem.h"

CTextureCacheJob::CTextureCacheJob(const CStdString &url, const CStdString &oldHash)
{
  m_url = url;
  m_oldHash = oldHash;
  m_cachePath = CTextureCache::GetCacheFile(m_url);
  m_texture = NULL;
}

CTextureCacheJob::CTextureCacheJob(const CStdString &url, const CBaseTexture *texture)
{
  m_url = url;
  if (texture)
    m_texture = new CTexture(*(CTexture *)texture);
  else
    m_texture = NULL;
  m_cachePath = CTextureCache::GetCacheFile(m_url);
}

CTextureCacheJob::~CTextureCacheJob()
{
  delete m_texture;
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

  return CacheTexture(&m_texture);
}

bool CTextureCacheJob::CacheTexture(CBaseTexture **out_texture)
{
  // unwrap the URL as required
  bool flipped;
  unsigned int width, height;
  CStdString image = DecodeImageURL(m_url, width, height, flipped);

  m_details.updateable = UpdateableURL(image);

  // generate the hash
  m_details.hash = GetImageHash(image);
  if (m_details.hash.empty())
    return false;
  else if (m_details.hash == m_oldHash)
    return true;

  CBaseTexture *texture = (old_texture && *old_texture) ? *old_texture : NULL;
  if (!texture)
    texture = LoadImage(image, width, height, flipped);
  if (texture)
  {
    if (texture->HasAlpha())
      m_details.file = m_cachePath + ".png";
    else
      m_details.file = m_cachePath + ".jpg";

    if (width > 0 && height > 0)
      CLog::Log(LOGDEBUG, "%s image '%s' at %dx%d with orientation %d as '%s'", m_oldHash.IsEmpty() ? "Caching" : "Recaching", image.c_str(),
                width, height, texture->GetOrientation(), m_details.file.c_str());
    else
      CLog::Log(LOGDEBUG, "%s image '%s' fullsize with orientation %d as '%s'", m_oldHash.IsEmpty() ? "Caching" : "Recaching", image.c_str(),
                texture->GetOrientation(), m_details.file.c_str());

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

CStdString CTextureCacheJob::DecodeImageURL(const CStdString &url, unsigned int &width, unsigned int &height, bool &flipped)
{
  // unwrap the URL as required
  CStdString image(url);
  flipped = false;
  width = g_advancedSettings.m_fanartHeight * 16/9;
  height = g_advancedSettings.m_fanartHeight;
  if (url.compare(0, 8, "image://") == 0)
  {
    // format is image://[type@]<url_encoded_path>?options
    CURL thumbURL(url);

    if (!thumbURL.GetUserName().IsEmpty())
      return ""; // we don't re-cache special images (eg picturefolder/video embedded thumbs)

    image = thumbURL.GetHostName();
    CURL::Decode(image);

    CStdString optionString = thumbURL.GetOptions().Mid(1);
    optionString.TrimRight('/'); // in case XBMC adds a slash

    std::vector<CStdString> options;
    StringUtils::SplitString(optionString, "&", options);
    for (std::vector<CStdString>::iterator i = options.begin(); i != options.end(); i++)
    {
      CStdString option, value;
      int pos = i->Find('=');
      if (pos != -1)
      {
        option = i->Left(pos);
        value  = i->Mid(pos + 1);
      }
      else
      {
        option = *i;
        value = "";
      }
      if (option == "size" && value == "thumb")
      {
        width = height = g_advancedSettings.m_thumbSize;
      }
      else if (option == "flipped")
      {
        flipped = true;
      }
    }
  }
  return image;
}

CBaseTexture *CTextureCacheJob::LoadImage(const CStdString &image, unsigned int width, unsigned int height, bool flipped)
{
  // Validate file URL to see if it is an image
  CFileItem file(image, false);
  if (!(file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ() ))
      && !file.GetMimeType().Left(6).Equals("image/")) // ignore non-pictures
    return NULL;

  CTexture *texture = new CTexture();
  if (!texture->LoadFromFile(image, width, height, g_guiSettings.GetBool("pictures.useexifrotation")))
  {
    delete texture;
    return NULL;
  }
  // EXIF bits are interpreted as: <flipXY><flipY*flipX><flipX>
  // where to undo the operation we apply them in reverse order <flipX>*<flipY*flipX>*<flipXY>
  // When flipped = true we have an additional <flipX> on the left, which is equivalent to toggling the last bit
  if (flipped)
    texture->SetOrientation(texture->GetOrientation() ^ 1);

  return texture;
}

bool CTextureCacheJob::UpdateableURL(const CStdString &url) const
{
  // we don't constantly check online images
  if (url.compare(0, 7, "http://") == 0 ||
      url.compare(0, 8, "https://") == 0)
    return false;
  return true;
}

CStdString CTextureCacheJob::GetImageHash(const CStdString &url)
{
  struct __stat64 st;
  if (XFILE::CFile::Stat(url, &st) == 0)
  {
    int64_t time = st.st_mtime;
    if (!time)
      time = st.st_ctime;
    if (time || st.st_size)
    {
      CStdString hash;
      hash.Format("d%"PRId64"s%"PRId64, time, st.st_size);
      return hash;
    }
  }
  CLog::Log(LOGDEBUG, "%s - unable to stat url %s", __FUNCTION__, url.c_str());
  return "";
}

CTextureDDSJob::CTextureDDSJob(const CStdString &original)
{
  m_original = original;
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
  CTexture texture;
  if (URIUtils::GetExtension(m_original).Equals(".dds"))
    return false;
  if (texture.LoadFromFile(m_original))
  { // convert to DDS
    CDDSImage dds;
    CLog::Log(LOGDEBUG, "Creating DDS version of: %s", m_original.c_str());
    return dds.Create(URIUtils::ReplaceExtension(m_original, ".dds"), texture.GetWidth(), texture.GetHeight(), texture.GetPitch(), texture.GetPixels(), 40);
  }
  return false;
}
