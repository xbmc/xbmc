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

#include "TextureCache.h"
#include "filesystem/File.h"
#include "threads/SingleLock.h"
#include "utils/Crc32.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include "guilib/Texture.h"
#include "guilib/DDSImage.h"
#include "pictures/Picture.h"
#include "guilib/TextureManager.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CTextureCache::CCacheJob::CCacheJob(const CStdString &url, const CStdString &oldHash)
{
  m_url = url;
  m_oldHash = oldHash;
  m_original = CTextureCache::GetCacheFile(m_url);
}

bool CTextureCache::CCacheJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CCacheJob* cacheJob = dynamic_cast<const CCacheJob*>(job);
    if (cacheJob && cacheJob->m_original == m_original)
      return true;
  }
  return false;
}

bool CTextureCache::CCacheJob::DoWork()
{
  m_hash = CacheImage(m_url, m_original, m_oldHash);
  return !m_hash.IsEmpty();
}

CStdString CTextureCache::CCacheJob::CacheImage(const CStdString &url, const CStdString &original, const CStdString &oldHash)
{
  // unwrap the URL as required
  CStdString image(url);
  bool fullSize = true;
  if (0 == strncmp(url.c_str(), "thumb://", 8))
  {
    fullSize = false;
    image = CURL(url).GetHostName();
    CURL::Decode(image);
  }

  // generate the hash
  CStdString hash = CTextureCache::Get().GetImageHash(image);
  if (hash.IsEmpty() || hash == oldHash)
    return hash;

  if (!oldHash.IsEmpty())
    CLog::Log(LOGDEBUG, "Re-caching image '%s' as '%s' %s size", image.c_str(), original.c_str(), fullSize ? "full" : "thumb");
  else
    CLog::Log(LOGDEBUG, "Caching image '%s' as '%s' %s size", image.c_str(), original.c_str(), fullSize ? "full" : "thumb");

  CStdString originalURL = CTextureCache::GetCachedPath(original);
  if (fullSize && CPicture::CacheFanart(image, originalURL))
    return hash;
  if (!fullSize && CPicture::CacheThumb(image, originalURL))
    return hash;
  return "";
}

CTextureCache::CDDSJob::CDDSJob(const CStdString &original)
{
  m_original = original;
}

bool CTextureCache::CDDSJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CDDSJob* ddsJob = dynamic_cast<const CDDSJob*>(job);
    if (ddsJob && ddsJob->m_original == m_original)
      return true;
  }
  return false;
}

bool CTextureCache::CDDSJob::DoWork()
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

CTextureCache &CTextureCache::Get()
{
  static CTextureCache s_cache;
  return s_cache;
}

CTextureCache::CTextureCache()
{
}

CTextureCache::~CTextureCache()
{
}

void CTextureCache::Initialize()
{
  CSingleLock lock(m_databaseSection);
  if (!m_database.IsOpen())
    m_database.Open();
}

void CTextureCache::Deinitialize()
{
  CancelJobs();
  CSingleLock lock(m_databaseSection);
  m_database.Close();
}

bool CTextureCache::IsCachedImage(const CStdString &url) const
{
  if (url != "-" && !CURL::IsFullPath(url))
    return true;
  if (URIUtils::IsInPath(url, "special://skin/") ||
      URIUtils::IsInPath(url, g_settings.GetThumbnailsFolder()))
    return true;
  return false;
}

CStdString CTextureCache::GetCachedImage(const CStdString &url)
{
  if (IsCachedImage(url))
    return url;

  // lookup the item in the database
  CStdString cacheFile;
  if (GetCachedTexture(url, cacheFile))
    return GetCachedPath(cacheFile);
  return "";
}

CStdString CTextureCache::GetWrappedThumbURL(const CStdString &image)
{
  CStdString url(image);
  CURL::Encode(url);
  return URIUtils::AddFileToFolder("thumb://" + url, URIUtils::GetFileName(image));
}

CStdString CTextureCache::CheckAndCacheImage(const CStdString &url, bool returnDDS)
{
  CStdString path(GetCachedImage(url));
  if (!path.IsEmpty())
  {
    if (returnDDS && !URIUtils::IsInPath(url, "special://skin/")) // TODO: should skin images be .dds'd (currently they're not necessarily writeable)
    { // check for dds version
      CStdString ddsPath = URIUtils::ReplaceExtension(path, ".dds");
      if (CFile::Exists(ddsPath))
        return ddsPath;
      if (g_advancedSettings.LibrarySettings->UseDDSFanArt())
        AddJob(new CDDSJob(path));
    }
    return path;
  }

  // Uncached image - best we can do for now is cache it so that the texture manager
  // can load it.

  // TODO: In the future we need a cache job to callback when the image is loaded
  //       thus automatically updating the images.  We'd also need fallback code inside
  //       the CGUITexture class to display something from this point on.

  // Have this caching stuff be a lifo stack, and bump things up the stack when we should (check whether
  // CJobQueue does this...)  That way we can have a bunch of "cache thumb kthxpls" run from a background
  // thread, and all we do here is cache from the size we've already been given.  If we haven't been
  // given a size, we must assume that the user wants fullsize.  We could, in fact, add the sizing of it
  // into the URL scheme using options... i.e. http://my.thumb/file|width=blah|height=foo
  // that gives us sizing goodness, plus pre-caching goodness where we know shit should be cached
  // all with the fandangled jobmanager....

  // We almost need a better interface on the loading side of things - i.e. we need the textures to
  // request a particular image and to get some (immediate?) recognition as to whether it's cached
  // so that a fallback image can be specified.  Perhaps the texture updating routines (i.e.
  // UpdateInfo and/or SetFileName) might handle this?  The procedure would be to hit the db
  // to see if this image is available or not.  If it isn't, then we simply load the fallback or
  // "loading..." image, but keep testing to see if the image has been cached or not.  Hmm,
  // that's inefficient as well - at the very least a string compare or 12 per frame as it tests
  // a list of caching jobs, or (more inefficiently) a db query every frame.

  // The "best" method is the callback technique - this can be done quite easily though if the texture
  // is the one that makes the request I think?  At least when one texture is involved - we pass in our
  // pointer to the callback list.  In fact, we could generalize this somewhat with the texture
  // manager handling those pointers - after all, it already handles reference counting, so why not
  // count using the callback pointers instead?  We then wouldn't have to call AllocResources() all
  // the time.  When the texture is loaded it's moved from the queued to the allocated list, and at
  // that point we could have an interim list (loaded) that we then run the callbacks on once a frame.
  // There'd be a "LOADING" enum for allocation of the image and the texture could then show a fallback?
  // Main problem with this is CGUITexture doesn't have any concept of a fallback: CGUIImage does instead.
  // The main fallback mechanism we use is LISTITEM_ICON vs LISTITEM_THUMB - with the former we actually
  // use the thumb if it's available, and drop back to the icon if it's not.  In either case, having
  // a "loading" fallback would be useful even if it wasn't the icon.  I guess this could be a property
  // of CGUITexture similar to how background="true" is?  The loading texture would be displayed if
  // and only if there is an image being loaded.  Need to talk to Jezz_X about this - eg if you have
  // a current image and a new one is loading we currently hold on to the current one and render
  // the current one faded out - we'd need to change this so that the fading only happened once it
  // was ready to render.

  CStdString originalFile = GetCacheFile(url);

  CStdString hash = CCacheJob::CacheImage(url, originalFile);
  if (!hash.IsEmpty())
  {
    AddCachedTexture(url, originalFile, hash);
    if (g_advancedSettings.LibrarySettings->UseDDSFanArt())
      AddJob(new CDDSJob(GetCachedPath(originalFile)));
    return GetCachedPath(originalFile);
  }
  return "";
}

void CTextureCache::ClearCachedImage(const CStdString &url, bool deleteSource /*= false */)
{
  // TODO: This can be removed when the texture cache covers everything.
  CStdString path = deleteSource ? url : "";
  CStdString cachedFile;
  if (ClearCachedTexture(url, cachedFile))
    path = GetCachedPath(cachedFile);
  if (CFile::Exists(path))
    CFile::Delete(path);
  path = URIUtils::ReplaceExtension(path, ".dds");
  if (CFile::Exists(path))
    CFile::Delete(path);
}

bool CTextureCache::GetCachedTexture(const CStdString &url, CStdString &cachedURL)
{
  CSingleLock lock(m_databaseSection);
  CStdString imageHash;
  if (m_database.GetCachedTexture(url, cachedURL, imageHash))
  {
    if (!imageHash.IsEmpty()) // check for an updated image
      AddJob(new CCacheJob(url, imageHash));
    return true;
  }
  return false;
}

bool CTextureCache::AddCachedTexture(const CStdString &url, const CStdString &cachedURL, const CStdString &hash)
{
  CSingleLock lock(m_databaseSection);
  return m_database.AddCachedTexture(url, cachedURL, hash);
}

bool CTextureCache::ClearCachedTexture(const CStdString &url, CStdString &cachedURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.ClearCachedTexture(url, cachedURL);
}

CStdString CTextureCache::GetImageHash(const CStdString &url) const
{
  struct __stat64 st;
  if (CFile::Stat(url, &st) == 0)
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

CStdString CTextureCache::GetCacheFile(const CStdString &url)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(url);
  CStdString hex;
  hex.Format("%08x", (unsigned int)crc);
  CStdString hash;
  hash.Format("%c/%s%s", hex[0], hex.c_str(), URIUtils::GetExtension(url).c_str());
  return hash;
}

CStdString CTextureCache::GetCachedPath(const CStdString &file)
{
  return URIUtils::AddFileToFolder(g_settings.GetThumbnailsFolder(), file);
}

void CTextureCache::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (strcmp(job->GetType(), "cacheimage") == 0 && success)
  {
    CCacheJob *cacheJob = (CCacheJob *)job;
    AddCachedTexture(cacheJob->m_url, cacheJob->m_original, cacheJob->m_hash);
    // TODO: call back to the UI indicating that it can update it's image...
    if (g_advancedSettings.LibrarySettings->UseDDSFanArt())
      AddJob(new CDDSJob(GetCachedPath(cacheJob->m_original)));
  }
  return CJobQueue::OnJobComplete(jobID, success, job);
}

CStdString CTextureCache::GetUniqueImage(const CStdString &url, const CStdString &extension)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(url);
  CStdString hex;
  hex.Format("%08x", (unsigned int)crc);
  CStdString hash;
  hash.Format("generated/%c/%s%s", hex[0], hex.c_str(), extension.c_str());
  return GetCachedPath(hash);
}
