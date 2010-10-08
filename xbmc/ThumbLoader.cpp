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

#include "FileSystem/StackDirectory.h"
#include "ThumbLoader.h"
#include "Util.h"
#include "URL.h"
#include "Picture.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "TextureManager.h"
#include "TextureCache.h"
#include "VideoInfoTag.h"
#include "VideoDatabase.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "Shortcut.h"

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
    return db.GetTextureForPath(item.m_strPath);
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

  m_path = item.m_strPath;

  if (item.IsVideoDb() && item.HasVideoInfoTag())
    m_path = item.GetVideoInfoTag()->m_strFileNameAndPath;

  if (CUtil::IsStack(m_path))
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
  if (CUtil::IsLiveTV(m_path)
  ||  CUtil::IsUPnP(m_path)
  ||  CUtil::IsDAAP(m_path)
  ||  m_item.IsDVD()
  ||  m_item.IsDVDImage()
  ||  m_item.IsDVDFile(false, true)
  ||  m_item.IsInternetStream()
  ||  m_item.IsPlayList())
    return false;

  if (CUtil::IsRemote(m_path) && !CUtil::IsOnLAN(m_path))
    return false;

  bool result=false;
  if (m_thumb)
  {
    CLog::Log(LOGDEBUG,"%s - trying to extract thumb from video file %s", __FUNCTION__, m_path.c_str());
    result = CDVDFileInfo::ExtractThumb(m_path, m_target, &m_item.GetVideoInfoTag()->m_streamDetails);
    if(result)
    {
      m_item.SetProperty("HasAutoThumb", "1");
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

/**
* Reads watched status from the database and sets the watched overlay accordingly
*/
void CVideoThumbLoader::SetWatchedOverlay(CFileItem *item)
{
  // do this only for video files and exclude everything else.
  if (item->IsVideo() && !item->IsVideoDb() && !item->IsInternetStream()
      && !item->IsFileFolder() && !item->IsPlugin())
  {
    CVideoDatabase dbs;
    if (dbs.Open())
    {
      int playCount = dbs.GetPlayCount(*item);
      if (playCount >= 0)
        item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, playCount > 0);

      dbs.Close();
    }
  }
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

  SetWatchedOverlay(pItem);

  CFileItem item(*pItem);
  CStdString cachedThumb(item.GetCachedVideoThumb());

  if (!pItem->HasThumbnail())
  {
    item.SetUserVideoThumb();
    if (CFile::Exists(cachedThumb))
      pItem->SetThumbnailImage(cachedThumb);
    else
    {
      CStdString strPath, strFileName;
      CUtil::Split(cachedThumb, strPath, strFileName);

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
          pItem->SetProperty("HasAutoThumb", "1");
          pItem->SetProperty("AutoThumbImage", cachedThumb);
          pItem->SetThumbnailImage(cachedThumb);
        }
      }
      else if (!item.m_bIsFolder && item.IsVideo() && g_guiSettings.GetBool("myvideos.extractthumb") &&
               g_guiSettings.GetBool("myvideos.extractflags"))
      {
        CThumbExtractor* extract = new CThumbExtractor(item, pItem->m_strPath, true, cachedThumb);
        AddJob(extract);
      }
    }
  }
  else if (!pItem->GetThumbnailImage().Left(10).Equals("special://"))
    LoadRemoteThumb(pItem);

  if (!pItem->HasProperty("fanart_image"))
  {
    if (pItem->CacheLocalFanart())
      pItem->SetProperty("fanart_image",pItem->GetCachedFanart());
  }

  if (!pItem->m_bIsFolder &&
       pItem->HasVideoInfoTag() &&
       g_guiSettings.GetBool("myvideos.extractflags") &&
       (!pItem->GetVideoInfoTag()->HasStreamDetails() ||
         pItem->GetVideoInfoTag()->m_streamDetails.GetVideoDuration() <= 0))
  {
    CThumbExtractor* extract = new CThumbExtractor(*pItem,pItem->m_strPath,false);
    AddJob(extract);
  }
  return true;
}

void CVideoThumbLoader::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  {
    CThumbExtractor* loader = (CThumbExtractor*)job;
    loader->m_item.m_strPath = loader->m_listpath;
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
      db.SetTextureForPath(item.m_strPath, thumb);
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
    if ( shortcut.Create( item.m_strPath ) )
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

