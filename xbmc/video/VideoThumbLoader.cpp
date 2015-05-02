/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <cstdlib>

#include "VideoThumbLoader.h"
#include "filesystem/StackDirectory.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "filesystem/DirectoryCache.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "settings/VideoSettings.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/StereoscopicsManager.h"
#include "rendering/RenderSystem.h"
#include "TextureCache.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "video/VideoDatabase.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

using namespace XFILE;
using namespace std;
using namespace VIDEO;

CThumbExtractor::CThumbExtractor(const CFileItem& item,
                                 const std::string& listpath,
                                 bool thumb,
                                 const std::string& target,
                                 int64_t pos,
                                 bool fillStreamDetails)
{
  m_listpath = listpath;
  m_target = target;
  m_thumb = thumb;
  m_item = item;
  m_pos = pos;
  m_fillStreamDetails = fillStreamDetails;

  if (item.IsVideoDb() && item.HasVideoInfoTag())
    m_item.SetPath(item.GetVideoInfoTag()->m_strFileNameAndPath);

  if (m_item.IsStack())
    m_item.SetPath(CStackDirectory::GetFirstStackedFile(m_item.GetPath()));
}

CThumbExtractor::~CThumbExtractor()
{
}

bool CThumbExtractor::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CThumbExtractor* jobExtract = dynamic_cast<const CThumbExtractor*>(job);
    if (jobExtract && jobExtract->m_listpath == m_listpath
                   && jobExtract->m_target == m_target)
      return true;
  }
  return false;
}

bool CThumbExtractor::DoWork()
{
  if (m_item.IsLiveTV()
  ||  URIUtils::IsUPnP(m_item.GetPath())
  ||  m_item.IsDVD()
  ||  m_item.IsDiscImage()
  ||  m_item.IsDVDFile(false, true)
  ||  m_item.IsInternetStream()
  ||  m_item.IsDiscStub()
  ||  m_item.IsPlayList())
    return false;

  // For HTTP/FTP we only allow extraction when on a LAN
  if (URIUtils::IsRemote(m_item.GetPath()) &&
     !URIUtils::IsOnLAN(m_item.GetPath())  &&
     (URIUtils::IsFTP(m_item.GetPath())    ||
      URIUtils::IsHTTP(m_item.GetPath())))
    return false;

  bool result=false;
  if (m_thumb)
  {
    CLog::Log(LOGDEBUG,"%s - trying to extract thumb from video file %s", __FUNCTION__, CURL::GetRedacted(m_item.GetPath()).c_str());
    // construct the thumb cache file
    CTextureDetails details;
    details.file = CTextureCache::GetCacheFile(m_target) + ".jpg";
    result = CDVDFileInfo::ExtractThumb(m_item.GetPath(), details, m_fillStreamDetails ? &m_item.GetVideoInfoTag()->m_streamDetails : NULL, (int) m_pos);
    if(result)
    {
      CTextureCache::Get().AddCachedTexture(m_target, details);
      m_item.SetProperty("HasAutoThumb", true);
      m_item.SetProperty("AutoThumbImage", m_target);
      m_item.SetArt("thumb", m_target);

      CVideoInfoTag* info = m_item.GetVideoInfoTag();
      if (info->m_iDbId > 0 && !info->m_type.empty())
      {
        CVideoDatabase db;
        if (db.Open())
        {
          db.SetArtForItem(info->m_iDbId, info->m_type, "thumb", m_item.GetArt("thumb"));
          db.Close();
        }
      }
    }
  }
  else if (!m_item.HasVideoInfoTag() || !m_item.GetVideoInfoTag()->HasStreamDetails())
  {
    // No tag or no details set, so extract them
    CLog::Log(LOGDEBUG,"%s - trying to extract filestream details from video file %s", __FUNCTION__, CURL::GetRedacted(m_item.GetPath()).c_str());
    result = CDVDFileInfo::GetFileStreamDetails(&m_item);
  }

  if (result)
  {
    CVideoInfoTag* info = m_item.GetVideoInfoTag();
    CVideoDatabase db;
    if (db.Open())
    {
      if (URIUtils::IsStack(m_listpath))
      {
        // Don't know the total time of the stack, so set duration to zero to avoid confusion
        info->m_streamDetails.SetVideoDuration(0, 0);

        // Restore original stack path
        m_item.SetPath(m_listpath);
      }

      if (info->m_iFileId < 0)
        db.SetStreamDetailsForFile(info->m_streamDetails, !info->m_strFileNameAndPath.empty() ? info->m_strFileNameAndPath : static_cast<const std::string&>(m_item.GetPath()));
      else
        db.SetStreamDetailsForFileId(info->m_streamDetails, info->m_iFileId);

      // overwrite the runtime value if the one from streamdetails is available
      if (info->m_iDbId > 0 && info->m_duration != static_cast<int>(info->GetDuration()))
      {
        info->m_duration = info->GetDuration();

        // store the updated information in the database
        db.SetDetailsForItem(info->m_iDbId, info->m_type, *info, m_item.GetArt());
      }

      db.Close();
    }
    return true;
  }

  return false;
}

CVideoThumbLoader::CVideoThumbLoader() :
  CThumbLoader(), CJobQueue(true, 1, CJob::PRIORITY_LOW_PAUSABLE)
{
  m_videoDatabase = new CVideoDatabase();
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
  delete m_videoDatabase;
}

void CVideoThumbLoader::OnLoaderStart()
{
  m_videoDatabase->Open();
  m_showArt.clear();
  CThumbLoader::OnLoaderStart();
}

void CVideoThumbLoader::OnLoaderFinish()
{
  m_videoDatabase->Close();
  m_showArt.clear();
  CThumbLoader::OnLoaderFinish();
}

static void SetupRarOptions(CFileItem& item, const std::string& path)
{
  std::string path2(path);
  if (item.IsVideoDb() && item.HasVideoInfoTag())
    path2 = item.GetVideoInfoTag()->m_strFileNameAndPath;
  CURL url(path2);
  std::string opts = url.GetOptions();
  if (opts.find("flags") != std::string::npos)
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
  if (type == MediaTypeEpisode)
    ret.push_back("thumb");
  else if (type == MediaTypeTvShow || type == MediaTypeSeason)
  {
    ret.push_back("banner");
    ret.push_back("poster");
    ret.push_back("fanart");
  }
  else if (type == MediaTypeMovie || type == MediaTypeMusicVideo || type == MediaTypeVideoCollection)
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
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CVideoThumbLoader::LoadItemCached(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive
  ||  pItem->IsParentFolder())
    return false;

  m_videoDatabase->Open();

  if (!pItem->HasVideoInfoTag() || !pItem->GetVideoInfoTag()->HasStreamDetails()) // no stream details
  {
    if ((pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iFileId >= 0) // file (or maybe folder) is in the database
    || (!pItem->m_bIsFolder && pItem->IsVideo())) // Some other video file for which we haven't yet got any database details
    {
      if (m_videoDatabase->GetStreamDetails(*pItem))
        pItem->SetInvalid();
    }
  }

  // video db items normally have info in the database
  if (pItem->HasVideoInfoTag() && !pItem->HasArt("thumb"))
  {
    FillLibraryArt(*pItem);

    if (!pItem->GetVideoInfoTag()->m_type.empty()                &&
         pItem->GetVideoInfoTag()->m_type != MediaTypeMovie      &&
         pItem->GetVideoInfoTag()->m_type != MediaTypeTvShow     &&
         pItem->GetVideoInfoTag()->m_type != MediaTypeEpisode    &&
         pItem->GetVideoInfoTag()->m_type != MediaTypeMusicVideo)
    {
      m_videoDatabase->Close();
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
      if (!art.empty())
        artwork.insert(make_pair(type, art));
    }
    SetArt(*pItem, artwork);
  }

  m_videoDatabase->Close();

  return true;
}

bool CVideoThumbLoader::LoadItemLookup(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive || pItem->IsParentFolder() || pItem->GetPath() == "add")
    return false;

  if (pItem->HasVideoInfoTag()                                &&
     !pItem->GetVideoInfoTag()->m_type.empty()                &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeMovie      &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeTvShow     &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeEpisode    &&
      pItem->GetVideoInfoTag()->m_type != MediaTypeMusicVideo)
    return false; // Nothing to do here

  DetectAndAddMissingItemData(*pItem);

  m_videoDatabase->Open();

  map<string, string> artwork = pItem->GetArt();
  vector<string> artTypes = GetArtTypes(pItem->HasVideoInfoTag() ? pItem->GetVideoInfoTag()->m_type : "");
  if (find(artTypes.begin(), artTypes.end(), "thumb") == artTypes.end())
    artTypes.push_back("thumb"); // always look for "thumb" art for files
  for (vector<string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
  {
    std::string type = *i;
    if (!pItem->HasArt(type))
    {
      std::string art = GetLocalArt(*pItem, type, type=="fanart");
      if (!art.empty()) // cache it
      {
        SetCachedImage(*pItem, type, art);
        CTextureCache::Get().BackgroundCacheImage(art);
        artwork.insert(make_pair(type, art));
      }
    }
  }
  SetArt(*pItem, artwork);

  // We can only extract flags/thumbs for file-like items
  if (!pItem->m_bIsFolder && pItem->IsVideo())
  {
    // An auto-generated thumb may have been cached on a different device - check we have it here
    std::string url = pItem->GetArt("thumb");
    if (StringUtils::StartsWith(url, "image://video@") && !CTextureCache::Get().HasCachedImage(url))
      pItem->SetArt("thumb", "");

    if (!pItem->HasArt("thumb"))
    {
      // create unique thumb for auto generated thumbs
      std::string thumbURL = GetEmbeddedThumbURL(*pItem);
      if (CTextureCache::Get().HasCachedImage(thumbURL))
      {
        CTextureCache::Get().BackgroundCacheImage(thumbURL);
        pItem->SetProperty("HasAutoThumb", true);
        pItem->SetProperty("AutoThumbImage", thumbURL);
        pItem->SetArt("thumb", thumbURL);

        if (pItem->HasVideoInfoTag())
        {
          // Item has cached autogen image but no art entry. Save it to db.
          CVideoInfoTag* info = pItem->GetVideoInfoTag();
          if (info->m_iDbId > 0 && !info->m_type.empty())
            m_videoDatabase->SetArtForItem(info->m_iDbId, info->m_type, "thumb", thumbURL);
        }
      }
      else if (CSettings::Get().GetBool("myvideos.extractthumb") &&
               CSettings::Get().GetBool("myvideos.extractflags"))
      {
        CFileItem item(*pItem);
        std::string path(item.GetPath());
        if (URIUtils::IsInRAR(item.GetPath()))
          SetupRarOptions(item,path);

        CThumbExtractor* extract = new CThumbExtractor(item, path, true, thumbURL);
        AddJob(extract);

        m_videoDatabase->Close();
        return true;
      }
    }

    // flag extraction
    if (CSettings::Get().GetBool("myvideos.extractflags") &&
       (!pItem->HasVideoInfoTag()                     ||
        !pItem->GetVideoInfoTag()->HasStreamDetails() ) )
    {
      CFileItem item(*pItem);
      std::string path(item.GetPath());
      if (URIUtils::IsInRAR(item.GetPath()))
        SetupRarOptions(item,path);
      CThumbExtractor* extract = new CThumbExtractor(item,path,false);
      AddJob(extract);
    }
  }

  m_videoDatabase->Close();
  return true;
}

void CVideoThumbLoader::SetArt(CFileItem &item, const map<string, string> &artwork)
{
  item.SetArt(artwork);
  if (artwork.find("thumb") == artwork.end())
  { // set fallback for "thumb"
    if (artwork.find("poster") != artwork.end())
      item.SetArtFallback("thumb", "poster");
    else if (artwork.find("banner") != artwork.end())
      item.SetArtFallback("thumb", "banner");
  }
}

bool CVideoThumbLoader::FillLibraryArt(CFileItem &item)
{
  CVideoInfoTag &tag = *item.GetVideoInfoTag();
  if (tag.m_iDbId > -1 && !tag.m_type.empty())
  {
    map<string, string> artwork;
    m_videoDatabase->Open();
    if (m_videoDatabase->GetArtForItem(tag.m_iDbId, tag.m_type, artwork))
      SetArt(item, artwork);
    else if (tag.m_type == MediaTypeArtist)
    { // we retrieve music video art from the music database (no backward compat)
      CMusicDatabase database;
      database.Open();
      int idArtist = database.GetArtistByName(item.GetLabel());
      if (database.GetArtForItem(idArtist, MediaTypeArtist, artwork))
        item.SetArt(artwork);
    }
    else if (tag.m_type == MediaTypeAlbum)
    { // we retrieve music video art from the music database (no backward compat)
      CMusicDatabase database;
      database.Open();
      int idAlbum = database.GetAlbumByName(item.GetLabel(), tag.m_artist);
      if (database.GetArtForItem(idAlbum, MediaTypeAlbum, artwork))
        item.SetArt(artwork);
    }
    // For episodes and seasons, we want to set fanart for that of the show
    if (!item.HasArt("fanart") && tag.m_iIdShow >= 0)
    {
      ArtCache::const_iterator i = m_showArt.find(tag.m_iIdShow);
      if (i == m_showArt.end())
      {
        map<string, string> showArt;
        m_videoDatabase->GetArtForItem(tag.m_iIdShow, MediaTypeTvShow, showArt);
        i = m_showArt.insert(make_pair(tag.m_iIdShow, showArt)).first;
      }
      if (i != m_showArt.end())
      {
        item.AppendArt(i->second, "tvshow");
        item.SetArtFallback("fanart", "tvshow.fanart");
        item.SetArtFallback("tvshow.thumb", "tvshow.poster");
      }
    }
    m_videoDatabase->Close();
  }
  return !item.GetArt().empty();
}

bool CVideoThumbLoader::FillThumb(CFileItem &item)
{
  if (item.HasArt("thumb"))
    return true;
  std::string thumb = GetCachedImage(item, "thumb");
  if (thumb.empty())
  {
    thumb = GetLocalArt(item, "thumb");
    if (!thumb.empty())
      SetCachedImage(item, "thumb", thumb);
  }
  if (!thumb.empty())
    item.SetArt("thumb", thumb);
  return !thumb.empty();
}

std::string CVideoThumbLoader::GetLocalArt(const CFileItem &item, const std::string &type, bool checkFolder)
{
  if (item.SkipLocalArt())
    return "";

  /* Cache directory for (sub) folders on streamed filesystems. We need to do this
     else entering (new) directories from the app thread becomes much slower. This
     is caused by the fact that Curl Stat/Exist() is really slow and that the 
     thumbloader thread accesses the streamed filesystem at the same time as the
     App thread and the latter has to wait for it.
   */
  if (item.m_bIsFolder && (item.IsInternetStream(true) || g_advancedSettings.m_networkBufferMode == 1))
  {
    CFileItemList items; // Dummy list
    CDirectory::GetDirectory(item.GetPath(), items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  }

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
    if (art.empty() && (checkFolder || (item.m_bIsFolder && !item.IsFileFolder()) || item.IsOpticalMediaFile()))
    { // try movie.tbn
      art = item.FindLocalArt("movie.tbn", true);
      if (art.empty()) // try folder.jpg
        art = item.FindLocalArt("folder.jpg", true);
    }
  }
  return art;
}

std::string CVideoThumbLoader::GetEmbeddedThumbURL(const CFileItem &item)
{
  std::string path(item.GetPath());
  if (item.IsVideoDb() && item.HasVideoInfoTag())
    path = item.GetVideoInfoTag()->m_strFileNameAndPath;
  if (URIUtils::IsStack(path))
    path = CStackDirectory::GetFirstStackedFile(path);

  return CTextureUtils::GetWrappedImageURL(path, "video");
}

void CVideoThumbLoader::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  {
    CThumbExtractor* loader = (CThumbExtractor*)job;
    loader->m_item.SetPath(loader->m_listpath);

    if (m_pObserver)
      m_pObserver->OnItemLoaded(&loader->m_item);
    CFileItemPtr pItem(new CFileItem(loader->m_item));
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, pItem);
    g_windowManager.SendThreadMessage(msg);
  }
  CJobQueue::OnJobComplete(jobID, success, job);
}

void CVideoThumbLoader::DetectAndAddMissingItemData(CFileItem &item)
{
  if (item.m_bIsFolder) return;

  std::string stereoMode;
  // detect stereomode for videos
  if (item.HasVideoInfoTag())
    stereoMode = item.GetVideoInfoTag()->m_streamDetails.GetStereoMode();
  if (stereoMode.empty())
  {
    std::string path = item.GetPath();
    if (item.IsVideoDb() && item.HasVideoInfoTag())
      path = item.GetVideoInfoTag()->GetPath();

    // check for custom stereomode setting in video settings
    CVideoSettings itemVideoSettings;
    m_videoDatabase->Open();
    if (m_videoDatabase->GetVideoSettings(item, itemVideoSettings) && itemVideoSettings.m_StereoMode != RENDER_STEREO_MODE_OFF)
      stereoMode = CStereoscopicsManager::Get().ConvertGuiStereoModeToString( (RENDER_STEREO_MODE) itemVideoSettings.m_StereoMode );
    m_videoDatabase->Close();

    // still empty, try grabbing from filename
    // TODO: in case of too many false positives due to using the full path, extract the filename only using string utils
    if (stereoMode.empty())
      stereoMode = CStereoscopicsManager::Get().DetectStereoModeByString( path );
  }
  if (!stereoMode.empty())
    item.SetProperty("stereomode", CStereoscopicsManager::Get().NormalizeStereoMode(stereoMode));
}
