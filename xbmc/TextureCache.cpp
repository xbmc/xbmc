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
#include "TextureCacheJob.h"
#include "filesystem/File.h"
#include "threads/SingleLock.h"
#include "utils/Crc32.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

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
  CStdString cachedHash;
  return GetCachedImage(url, cachedHash);
}

CStdString CTextureCache::GetCachedImage(const CStdString &url, CStdString &cachedHash)
{
  cachedHash.clear();
  if (url.compare(0, 8, "image://") == 0)
  {
    CStdString image = CURL(url).GetHostName();
    CURL::Decode(image);
    if (IsCachedImage(image))
      return image; // no point generating thumbs of already cached images
  }
  if (IsCachedImage(url))
    return url;

  // lookup the item in the database
  CStdString cacheFile;
  if (GetCachedTexture(url, cacheFile, cachedHash))
    return GetCachedPath(cacheFile);
  return "";
}

CStdString CTextureCache::GetWrappedImageURL(const CStdString &image, const CStdString &type)
{
  CStdString url(image);
  CURL::Encode(url);
  return "image://" + type + "@" + url;
}

CStdString CTextureCache::GetWrappedThumbURL(const CStdString &image)
{
  CStdString url(image);
  CURL::Encode(url);
  return "image://" + url + "/transform?size=thumb";
}

CStdString CTextureCache::CheckCachedImage(const CStdString &url, bool returnDDS, bool &needsRecaching)
{
  CStdString cachedHash;
  CStdString path(GetCachedImage(url, cachedHash));
  needsRecaching = !cachedHash.IsEmpty();
  if (!path.IsEmpty())
  {
    if (!needsRecaching && returnDDS && !URIUtils::IsInPath(url, "special://skin/")) // TODO: should skin images be .dds'd (currently they're not necessarily writeable)
    { // check for dds version
      CStdString ddsPath = URIUtils::ReplaceExtension(path, ".dds");
      if (CFile::Exists(ddsPath))
        return ddsPath;
      if (g_advancedSettings.m_useDDSFanart)
        AddJob(new CTextureDDSJob(path));
    }
    return path;
  }
  return "";
}

CStdString CTextureCache::CheckAndCacheImage(const CStdString &url, bool returnDDS)
{
  bool needsRecaching = false;
  CStdString path(CheckCachedImage(url, returnDDS, needsRecaching));
  if (!path.IsEmpty())
  {
    return path;
  }
  return CacheImageFile(url);
}

void CTextureCache::BackgroundCacheImage(const CStdString &url)
{
  CStdString cacheHash;
  CStdString path(GetCachedImage(url, cacheHash));
  if (!path.IsEmpty() && cacheHash.IsEmpty())
    return; // image is already cached and doesn't need to be checked further

  // needs (re)caching
  AddJob(new CTextureCacheJob(url, cacheHash));
}

void CTextureCache::BackgroundCacheTexture(const CStdString &url, const CBaseTexture *texture, unsigned int max_width, unsigned int max_height)
{
  AddJob(new CTextureCacheJob(url, texture, max_width, max_height));
}

CStdString CTextureCache::CacheImageFile(const CStdString &url)
{
  // Cache image so that the texture manager can load it.
  CTextureCacheJob job(url, "");
  if (job.DoWork() && !job.m_hash.IsEmpty())
  {
    AddCachedTexture(url, job.m_cacheFile, job.m_hash, job.m_updateable);
    if (g_advancedSettings.m_useDDSFanart && !job.m_cacheFile.IsEmpty())
      AddJob(new CTextureDDSJob(GetCachedPath(job.m_cacheFile)));
    return GetCachedPath(job.m_cacheFile);
  }
  return "";

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

bool CTextureCache::GetCachedTexture(const CStdString &url, CStdString &cachedURL, CStdString &cachedHash)
{
  CSingleLock lock(m_databaseSection);
  return m_database.GetCachedTexture(url, cachedURL, cachedHash);
}

bool CTextureCache::AddCachedTexture(const CStdString &url, const CStdString &cachedURL, const CStdString &hash, bool updateable)
{
  CSingleLock lock(m_databaseSection);
  return m_database.AddCachedTexture(url, cachedURL, hash, updateable);
}

bool CTextureCache::ClearCachedTexture(const CStdString &url, CStdString &cachedURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.ClearCachedTexture(url, cachedURL);
}

CStdString CTextureCache::GetCacheFile(const CStdString &url)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(url);
  CStdString hex;
  hex.Format("%08x", (unsigned int)crc);
  CStdString hash;
  hash.Format("%c/%s", hex[0], hex.c_str());
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
    CTextureCacheJob *cacheJob = (CTextureCacheJob *)job;
    AddCachedTexture(cacheJob->m_url, cacheJob->m_cacheFile, cacheJob->m_hash, cacheJob->m_updateable);
    // TODO: call back to the UI indicating that it can update it's image...
    if (g_advancedSettings.m_useDDSFanart && !cacheJob->m_cacheFile.IsEmpty())
      AddJob(new CTextureDDSJob(GetCachedPath(cacheJob->m_cacheFile)));
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

bool CTextureCache::Export(const CStdString &image, const CStdString &destination)
{
  CStdString cachedImage(GetCachedImage(image));
  if (!cachedImage.IsEmpty())
  {
    if (CFile::Cache(cachedImage, destination))
      return true;
    CLog::Log(LOGERROR, "%s failed exporting '%s' to '%s'", __FUNCTION__, cachedImage.c_str(), destination.c_str());
  }
  return false;
}
