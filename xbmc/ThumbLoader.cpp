/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "filesystem/StackDirectory.h"
#include "ThumbLoader.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "pictures/Picture.h"
#include "filesystem/File.h"
#include "filesystem/DirectoryCache.h"
#include "FileItem.h"
#include "settings/GUISettings.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/TextureManager.h"
#include "TextureCache.h"
#include "utils/log.h"
#include "programs/Shortcut.h"
#include "video/VideoInfoTag.h"
#include "video/VideoDatabase.h"
#include "cores/dvdplayer/DVDFileInfo.h"

using namespace XFILE;
using namespace std;

CThumbLoader::CThumbLoader(int nThreads) :
  CBackgroundInfoLoader(nThreads)
{
}

CThumbLoader::~CThumbLoader()
{
}

bool CThumbLoader::LoadRemoteThumb(CFileItem *pItem)
{
  // look for remote thumbs
  CStdString thumb(pItem->GetThumbnailImage());
  if (!g_TextureManager.CanLoad(thumb))
  {
    CStdString cachedThumb(pItem->GetCachedVideoThumb());
    if (CFile::Exists(cachedThumb))
      pItem->SetThumbnailImage(cachedThumb);
    else
    {
      if (CPicture::CreateThumbnail(thumb, cachedThumb))
        pItem->SetThumbnailImage(cachedThumb);
      else
        pItem->SetThumbnailImage("");
    }
  }
  return pItem->HasThumbnail();
}

CStdString CThumbLoader::GetCachedThumb(const CFileItem &item)
{
  CTextureDatabase db;
  if (db.Open())
    return db.GetTextureForPath(item.GetPath());
  return "";
}

bool CThumbLoader::CheckAndCacheThumb(CFileItem &item)
{
  if (item.HasThumbnail() && !g_TextureManager.CanLoad(item.GetThumbnailImage()))
  {
    CStdString thumb = CTextureCache::Get().CheckAndCacheImage(item.GetThumbnailImage());
    item.SetThumbnailImage(thumb);
    return !thumb.IsEmpty();
  }
  return false;
}

CThumbExtractor::CThumbExtractor(const CFileItem& item, const CStdString& listpath, bool thumb, const CStdString& target)
{
  m_listpath = listpath;
  m_target = target;
  m_thumb = thumb;
  m_item = item;

  m_path = item.GetPath();

  if (item.IsVideoDb() && item.HasVideoInfoTag())
    m_path = item.GetVideoInfoTag()->m_strFileNameAndPath;

  if (URIUtils::IsStack(m_path))
    m_path = CStackDirectory::GetFirstStackedFile(m_path);
}

CThumbExtractor::~CThumbExtractor()
{
}

bool CThumbExtractor::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CThumbExtractor* jobExtract = dynamic_cast<const CThumbExtractor*>(job);
    if (jobExtract && jobExtract->m_listpath == m_listpath)
      return true;
  }
  return false;
}

bool CThumbExtractor::DoWork()
{
  if (URIUtils::IsLiveTV(m_path)
  ||  URIUtils::IsUPnP(m_path)
  ||  URIUtils::IsDAAP(m_path)
  ||  m_item.IsDVD()
  ||  m_item.IsDVDImage()
  ||  m_item.IsDVDFile(false, true)
  ||  m_item.IsInternetStream()
  ||  m_item.IsPlayList())
    return false;

  if (URIUtils::IsRemote(m_path) && !URIUtils::IsOnLAN(m_path))
    return false;

  bool result=false;
  if (m_thumb)
  {
    CLog::Log(LOGDEBUG,"%s - trying to extract thumb from video file %s", __FUNCTION__, m_path.c_str());
    result = CDVDFileInfo::ExtractThumb(m_path, m_target, &m_item.GetVideoInfoTag()->m_streamDetails);
    if(result)
    {
      m_item.SetProperty("HasAutoThumb", true);
      m_item.SetProperty("AutoThumbImage", m_target);
      m_item.SetThumbnailImage(m_target);
    }
  }
  else if (m_item.HasVideoInfoTag() && !m_item.GetVideoInfoTag()->HasStreamDetails())
  {
    CLog::Log(LOGDEBUG,"%s - trying to extract filestream details from video file %s", __FUNCTION__, m_path.c_str());
    result = CDVDFileInfo::GetFileStreamDetails(&m_item);
  }

  return result;
}

CVideoThumbLoader::CVideoThumbLoader() :
  CThumbLoader(1), CJobQueue(true), m_pStreamDetailsObs(NULL)
{
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
}

void CVideoThumbLoader::OnLoaderStart()
{
}

void CVideoThumbLoader::OnLoaderFinish()
{
}

static void SetupRarOptions(CFileItem& item, const CStdString& path)
{
  CStdString path2(path);
  if (item.IsVideoDb() && item.HasVideoInfoTag())
    path2 = item.GetVideoInfoTag()->m_strFileNameAndPath;
  CURL url(path2);
  CStdString opts = url.GetOptions();
  if (opts.Find("flags") > -1)
    return;
  if (opts.size())
    opts += "&flags=8";
  else
    opts = "?flags=8";
  url.SetOptions(opts);
  if (item.IsVideoDb() && item.HasVideoInfoTag())
    item.GetVideoInfoTag()->m_strFileNameAndPath = url.Get();
  else
    item.SetPath(url.Get());
  g_directoryCache.ClearDirectory(url.GetWithoutFilename());
}

/**
 * Look for a thumbnail for pItem.  If one does not exist, look for an autogenerated
 * thumbnail.  If that does not exist, attempt to autogenerate one.  Finally, check
 * for the existance of fanart and set properties accordinly.
 * @return: true if pItem has been modified
 */
bool CVideoThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive
  ||  pItem->IsParentFolder())
    return false;

  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds == 0)
  {
    CVideoDatabase db;
    db.Open();
    if (db.GetResumePoint(*pItem->GetVideoInfoTag()))
      pItem->SetInvalid();
    db.Close();
  }

  CStdString cachedThumb(pItem->GetCachedVideoThumb());

  if (!pItem->HasProperty("fanart_image"))
  {
    if (pItem->CacheLocalFanart())
      pItem->SetProperty("fanart_image",pItem->GetCachedFanart());
  }

  if (!pItem->HasThumbnail())
  {
    pItem->SetUserVideoThumb();
    if (CFile::Exists(cachedThumb))
      pItem->SetThumbnailImage(cachedThumb);
    else
    {
      CStdString strPath, strFileName;
      URIUtils::Split(cachedThumb, strPath, strFileName);

      // create unique thumb for auto generated thumbs
      cachedThumb = strPath + "auto-" + strFileName;
      if (CFile::Exists(cachedThumb))
      {
        // this is abit of a hack to avoid loading zero sized images
        // which we know will fail. They will just display empty image
        // we should really have some way for the texture loader to
        // do fallbacks to default images for a failed image instead
        struct __stat64 st;
        if(CFile::Stat(cachedThumb, &st) == 0 && st.st_size > 0)
        {
          pItem->SetProperty("HasAutoThumb", true);
          pItem->SetProperty("AutoThumbImage", cachedThumb);
          pItem->SetThumbnailImage(cachedThumb);
        }
      }
      else if (!pItem->m_bIsFolder && pItem->IsVideo() && g_guiSettings.GetBool("myvideos.extractthumb") &&
               g_guiSettings.GetBool("myvideos.extractflags"))
      {
        CFileItem item(*pItem);
        CStdString path(item.GetPath());
        if (URIUtils::IsInRAR(item.GetPath()))
          SetupRarOptions(item,path);

        CThumbExtractor* extract = new CThumbExtractor(item, path, true, cachedThumb);
        AddJob(extract);
        return true;
      }
    }
  }
  else if (!pItem->GetThumbnailImage().Left(10).Equals("special://"))
    LoadRemoteThumb(pItem);

  if (!pItem->m_bIsFolder &&
       pItem->HasVideoInfoTag() &&
       g_guiSettings.GetBool("myvideos.extractflags") &&
       (!pItem->GetVideoInfoTag()->HasStreamDetails() ||
         pItem->GetVideoInfoTag()->m_streamDetails.GetVideoDuration() <= 0))
  {
    CFileItem item(*pItem);
    CStdString path(item.GetPath());
    if (URIUtils::IsInRAR(item.GetPath()))
      SetupRarOptions(item,path);
    CThumbExtractor* extract = new CThumbExtractor(item,path,false);
    AddJob(extract);
  }

  return true;
}

void CVideoThumbLoader::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  {
    CThumbExtractor* loader = (CThumbExtractor*)job;
    loader->m_item.SetPath(loader->m_listpath);
    CVideoInfoTag* info = loader->m_item.GetVideoInfoTag();
    if (m_pStreamDetailsObs)
      m_pStreamDetailsObs->OnStreamDetails(info->m_streamDetails, info->m_strFileNameAndPath, info->m_iFileId);
    if (m_pObserver)
      m_pObserver->OnItemLoaded(&loader->m_item);
    CFileItemPtr pItem(new CFileItem(loader->m_item));
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, pItem);
    g_windowManager.SendThreadMessage(msg);
  }
  CJobQueue::OnJobComplete(jobID, success, job);
}

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  if (pItem->IsParentFolder()) return true;
  return FillThumb(*pItem);
}

bool CProgramThumbLoader::FillThumb(CFileItem &item)
{
  // no need to do anything if we already have a thumb set
  if (CheckAndCacheThumb(item) || item.HasThumbnail())
    return true;

  // see whether we have a cached image for this item
  CStdString thumb = GetCachedThumb(item);
  if (!thumb.IsEmpty())
  {
    item.SetThumbnailImage(CTextureCache::Get().CheckAndCacheImage(thumb));
    return true;
  }
  thumb = GetLocalThumb(item);
  if (!thumb.IsEmpty())
  {
    CTextureDatabase db;
    if (db.Open())
      db.SetTextureForPath(item.GetPath(), thumb);
    thumb = CTextureCache::Get().CheckAndCacheImage(thumb);
  }
  item.SetThumbnailImage(thumb);
  return true;
}

CStdString CProgramThumbLoader::GetLocalThumb(const CFileItem &item)
{
  // look for the thumb
  if (item.IsShortCut())
  {
    CShortcut shortcut;
    if ( shortcut.Create( item.GetPath() ) )
    {
      // use the shortcut's thumb
      if (!shortcut.m_strThumb.IsEmpty())
        return shortcut.m_strThumb;
      else
      {
        CFileItem cut(shortcut.m_strPath,false);
        if (FillThumb(cut))
          return cut.GetThumbnailImage();
      }
    }
  }
  else if (item.m_bIsFolder)
  {
    CStdString folderThumb = item.GetFolderThumb();
    if (XFILE::CFile::Exists(folderThumb))
      return folderThumb;
  }
  else
  {
    CStdString fileThumb(item.GetTBNFile());
    if (CFile::Exists(fileThumb))
      return fileThumb;
  }
  return "";
}

CMusicThumbLoader::CMusicThumbLoader()
{
}

CMusicThumbLoader::~CMusicThumbLoader()
{
}

bool CMusicThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (!pItem->HasThumbnail())
    pItem->SetUserMusicThumb();
  else
    LoadRemoteThumb(pItem);
  return true;
}

