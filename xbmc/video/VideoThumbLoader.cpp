/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoThumbLoader.h"
#include "filesystem/StackDirectory.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "filesystem/File.h"
#include "filesystem/DirectoryCache.h"
#include "FileItem.h"
#include "settings/GUISettings.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "TextureCache.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "video/VideoDatabase.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "video/VideoInfoScanner.h"
#include "music/MusicDatabase.h"

using namespace XFILE;
using namespace std;
using namespace VIDEO;

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
  ||  m_item.IsDiscStub()
  ||  m_item.IsPlayList())
    return false;

  if (URIUtils::IsRemote(m_path) && !URIUtils::IsOnLAN(m_path))
    return false;

  bool result=false;
  if (m_thumb)
  {
    CLog::Log(LOGDEBUG,"%s - trying to extract thumb from video file %s", __FUNCTION__, m_path.c_str());
    // construct the thumb cache file
    CTextureDetails details;
    details.file = CTextureCache::GetCacheFile(m_target) + ".jpg";
    result = CDVDFileInfo::ExtractThumb(m_path, details, &m_item.GetVideoInfoTag()->m_streamDetails);
    if(result)
    {
      CTextureCache::Get().AddCachedTexture(m_target, details);
      m_item.SetProperty("HasAutoThumb", true);
      m_item.SetProperty("AutoThumbImage", m_target);
      m_item.SetArt("thumb", CTextureCache::GetCachedPath(details.file));
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
  m_database = new CVideoDatabase();
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
  delete m_database;
}

void CVideoThumbLoader::Initialize()
{
  m_database->Open();
  m_showArt.clear();
}

void CVideoThumbLoader::OnLoaderStart()
{
  Initialize();
}

void CVideoThumbLoader::OnLoaderFinish()
{
  m_database->Close();
  m_showArt.clear();
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

vector<string> CVideoThumbLoader::GetArtTypes(const string &type)
{
  vector<string> ret;
  if (type == "episode")
    ret.push_back("thumb");
  else if (type == "tvshow" || type == "season")
  {
    ret.push_back("banner");
    ret.push_back("poster");
    ret.push_back("fanart");
  }
  else if (type == "movie" || type == "musicvideo" || type == "set")
  {
    ret.push_back("poster");
    ret.push_back("fanart");
  }
  else if (type.empty()) // unknown - just throw everything in
  {
    ret.push_back("poster");
    ret.push_back("banner");
    ret.push_back("thumb");
    ret.push_back("fanart");
  }
  return ret;
}

/**
 * Look for a thumbnail for pItem.  If one does not exist, look for an autogenerated
 * thumbnail.  If that does not exist, attempt to autogenerate one.  Finally, check
 * for the existance of fanart and set properties accordingly.
 * @return: true if pItem has been modified
 */
bool CVideoThumbLoader::LoadItem(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive
  ||  pItem->IsParentFolder())
    return false;

  m_database->Open();

  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->HasStreamDetails() &&
     (pItem->GetVideoInfoTag()->m_type == "movie" || pItem->GetVideoInfoTag()->m_type == "episode" || pItem->GetVideoInfoTag()->m_type == "musicvideo"))
  {
    if (m_database->GetStreamDetails(*pItem->GetVideoInfoTag()))
      pItem->SetInvalid();
  }

  // video db items normally have info in the database
  if (pItem->HasVideoInfoTag() && !pItem->HasArt("thumb"))
  {
    FillLibraryArt(*pItem);

    if (!pItem->GetVideoInfoTag()->m_type.empty()         &&
         pItem->GetVideoInfoTag()->m_type != "movie"      &&
         pItem->GetVideoInfoTag()->m_type != "tvshow"     &&
         pItem->GetVideoInfoTag()->m_type != "episode"    &&
         pItem->GetVideoInfoTag()->m_type != "musicvideo")
    {
      m_database->Close();
      return true; // nothing else to be done
    }
  }

  // if we have no art, look for it all
  map<string, string> artwork = pItem->GetArt();
  if (artwork.empty())
  {
    vector<string> artTypes = GetArtTypes(pItem->HasVideoInfoTag() ? pItem->GetVideoInfoTag()->m_type : "");
    if (find(artTypes.begin(), artTypes.end(), "thumb") == artTypes.end())
      artTypes.push_back("thumb"); // always look for "thumb" art for files
    for (vector<string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
    {
      std::string type = *i;
      std::string art = GetCachedImage(*pItem, type);
      if (art.empty())
      {
        art = GetLocalArt(*pItem, type, type=="fanart");
        if (!art.empty()) // cache it
          SetCachedImage(*pItem, type, art);
      }
      if (!art.empty())
      {
        CTextureCache::Get().BackgroundCacheImage(art);
        artwork.insert(make_pair(type, art));
      }
    }
    pItem->SetArt(artwork);
  }

  // thumbnails are special-cased due to auto-generation
  if (!pItem->HasArt("thumb") && !pItem->m_bIsFolder && pItem->IsVideo())
  {
    // create unique thumb for auto generated thumbs
    CStdString thumbURL = GetEmbeddedThumbURL(*pItem);
    if (CTextureCache::Get().HasCachedImage(thumbURL))
    {
      CTextureCache::Get().BackgroundCacheImage(thumbURL);
      pItem->SetProperty("HasAutoThumb", true);
      pItem->SetProperty("AutoThumbImage", thumbURL);
      pItem->SetArt("thumb", thumbURL);
    }
    else if (g_guiSettings.GetBool("myvideos.extractthumb") &&
             g_guiSettings.GetBool("myvideos.extractflags"))
    {
      CFileItem item(*pItem);
      CStdString path(item.GetPath());
      if (URIUtils::IsInRAR(item.GetPath()))
        SetupRarOptions(item,path);

      CThumbExtractor* extract = new CThumbExtractor(item, path, true, thumbURL);
      AddJob(extract);

      m_database->Close();
      return true;
    }
  }

  // flag extraction
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

  m_database->Close();
  return true;
}

bool CVideoThumbLoader::FillLibraryArt(CFileItem &item)
{
  CVideoInfoTag &tag = *item.GetVideoInfoTag();
  if (tag.m_iDbId > -1 && !tag.m_type.IsEmpty())
  {
    map<string, string> artwork;
    m_database->Open();
    if (m_database->GetArtForItem(tag.m_iDbId, tag.m_type, artwork))
      item.SetArt(artwork);
    else if (tag.m_type == "artist")
    { // we retrieve music video art from the music database (no backward compat)
      CMusicDatabase database;
      database.Open();
      int idArtist = database.GetArtistByName(item.GetLabel());
      if (database.GetArtForItem(idArtist, "artist", artwork))
        item.SetArt(artwork);
    }
    else if (tag.m_type == "album")
    { // we retrieve music video art from the music database (no backward compat)
      CMusicDatabase database;
      database.Open();
      int idAlbum = database.GetAlbumByName(item.GetLabel(), tag.m_artist);
      if (database.GetArtForItem(idAlbum, "album", artwork))
        item.SetArt(artwork);
    }
    // For episodes and seasons, we want to set fanart for that of the show
    if (!item.HasArt("fanart") && tag.m_iIdShow >= 0)
    {
      ArtCache::const_iterator i = m_showArt.find(tag.m_iIdShow);
      if (i != m_showArt.end())
        item.AppendArt(i->second);
      else
      {
        map<string, string> showArt, cacheArt;
        if (m_database->GetArtForItem(tag.m_iIdShow, "tvshow", showArt))
        {
          for (CGUIListItem::ArtMap::iterator i = showArt.begin(); i != showArt.end(); ++i)
          {
            if (i->first == "fanart")
              cacheArt.insert(*i);
            else
              cacheArt.insert(make_pair("tvshow." + i->first, i->second));
          }
          item.AppendArt(cacheArt);
        }
        m_showArt.insert(make_pair(tag.m_iIdShow, cacheArt));
      }
    }
    m_database->Close();
  }
  return !item.GetArt().empty();
}

bool CVideoThumbLoader::FillThumb(CFileItem &item)
{
  if (item.HasArt("thumb"))
    return true;
  CStdString thumb = GetCachedImage(item, "thumb");
  if (thumb.IsEmpty())
  {
    thumb = GetLocalArt(item, "thumb");
    if (!thumb.IsEmpty())
      SetCachedImage(item, "thumb", thumb);
  }
  item.SetArt("thumb", thumb);
  return !thumb.IsEmpty();
}

std::string CVideoThumbLoader::GetLocalArt(const CFileItem &item, const std::string &type, bool checkFolder)
{
  std::string art;
  if (!type.empty())
  {
    art = item.FindLocalArt(type + ".jpg", checkFolder);
    if (art.empty())
      art = item.FindLocalArt(type + ".png", checkFolder);
  }
  if (art.empty() && (type.empty() || type == "thumb"))
  { // backward compatibility
    art = item.FindLocalArt("", false);
    if (art.empty() && (checkFolder || (item.m_bIsFolder && !item.IsFileFolder())))
    { // try movie.tbn
      art = item.FindLocalArt("movie.tbn", true);
      if (art.empty()) // try folder.jpg
        art = item.FindLocalArt("folder.jpg", true);
    }
  }
  return art;
}

CStdString CVideoThumbLoader::GetEmbeddedThumbURL(const CFileItem &item)
{
  CStdString path(item.GetPath());
  if (item.IsVideoDb() && item.HasVideoInfoTag())
    path = item.GetVideoInfoTag()->m_strFileNameAndPath;
  if (URIUtils::IsStack(path))
    path = CStackDirectory::GetFirstStackedFile(path);

  return CTextureCache::GetWrappedImageURL(path, "video");
}

void CVideoThumbLoader::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  {
    CThumbExtractor* loader = (CThumbExtractor*)job;
    loader->m_item.SetPath(loader->m_listpath);
    CVideoInfoTag* info = loader->m_item.GetVideoInfoTag();

    if (loader->m_thumb && info->m_iDbId > 0 && !info->m_type.empty())
    {
      // This runs in a different thread than the CVideoThumbLoader object.
      CVideoDatabase db;
      if (db.Open())
      {
        db.SetArtForItem(info->m_iDbId, info->m_type, "thumb", loader->m_item.GetArt("thumb"));
        db.Close();
      }

    }

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
