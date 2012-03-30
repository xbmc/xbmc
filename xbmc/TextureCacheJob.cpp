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
  m_relativeCacheFile = CTextureCache::GetCacheFile(m_url);
}

bool CTextureCacheJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CTextureCacheJob* cacheJob = dynamic_cast<const CTextureCacheJob*>(job);
    if (cacheJob && cacheJob->m_relativeCacheFile == m_relativeCacheFile)
      return true;
  }
  return false;
}

bool CTextureCacheJob::DoWork()
{
  m_hash = CacheImage(m_url, m_relativeCacheFile, m_oldHash);
  return !m_hash.IsEmpty();
}

CStdString CTextureCacheJob::CacheImage(const CStdString &url, const CStdString &cacheFile, const CStdString &oldHash)
{
  // unwrap the URL as required
  CStdString image(url);
  bool fullSize = true;
  bool flipped = false;
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
        fullSize = false;
      }
      else if (option == "flipped")
      {
        flipped = true;
      }
    }
  }

  // generate the hash
  CStdString hash = GetImageHash(image);
  if (hash.IsEmpty() || hash == oldHash)
    return hash;

  // Validate file URL to see if it is an image
  CFileItem file(image, false);
  if (!(file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ() ))
      && !file.GetMimeType().Left(6).Equals("image/")) // ignore non-pictures
    return "";

  CStdString logMessage = oldHash.IsEmpty() ? "Caching" : "Recaching";
  CStdString cacheURL = CTextureCache::GetCachedPath(cacheFile);
  if (flipped)
  {
    CLog::Log(LOGDEBUG, "%s flipped image '%s' as '%s'", logMessage.c_str(), image.c_str(), cacheFile.c_str());
    if (CPicture::ConvertFile(image, cacheURL, 0, 1920, 1080, 90, true))
     return hash;
  }
  else if (fullSize)
  {
    CLog::Log(LOGDEBUG, "%s full image '%s' as '%s'", logMessage.c_str(), image.c_str(), cacheFile.c_str());
    if (CPicture::CacheFanart(image, cacheURL))
      return hash;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s thumb image '%s' as '%s'", logMessage.c_str(), image.c_str(), cacheFile.c_str());
    if (CPicture::CacheThumb(image, cacheURL))
      return hash;
  }
  return "";
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
