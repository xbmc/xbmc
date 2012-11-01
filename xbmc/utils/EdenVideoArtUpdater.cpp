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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "EdenVideoArtUpdater.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoScanner.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/Crc32.h"
#include "utils/URIUtils.h"
#include "utils/ScraperUrl.h"
#include "utils/StringUtils.h"
#include "TextureCache.h"
#include "TextureCacheJob.h"
#include "pictures/Picture.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "guilib/Texture.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/File.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "interfaces/AnnouncementManager.h"

using namespace std;
using namespace VIDEO;
using namespace XFILE;

CEdenVideoArtUpdater::CEdenVideoArtUpdater() : CThread("EdenVideoArtUpdater")
{
  m_textureDB.Open();
}

CEdenVideoArtUpdater::~CEdenVideoArtUpdater()
{
  m_textureDB.Close();
}

void CEdenVideoArtUpdater::Start()
{
  CEdenVideoArtUpdater *updater = new CEdenVideoArtUpdater();
  updater->Create(true); // autodelete
}

void CEdenVideoArtUpdater::Process()
{
  // grab all movies...
  CVideoDatabase db;
  if (!db.Open())
    return;

  CFileItemList items;

  CGUIDialogExtendedProgressBar* dialog =
    (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);

  CGUIDialogProgressBarHandle *handle = dialog->GetHandle(g_localizeStrings.Get(314));
  handle->SetTitle(g_localizeStrings.Get(12349));

  // movies
  db.GetMoviesByWhere("videodb://1/2/", CDatabase::Filter(), items, false);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    handle->SetProgress(i, items.Size());
    handle->SetText(StringUtils::Format(g_localizeStrings.Get(12350).c_str(), item->GetLabel().c_str()));
    string cachedThumb = GetCachedVideoThumb(*item);
    string cachedFanart = GetCachedFanart(*item);

    item->SetPath(item->GetVideoInfoTag()->m_strFileNameAndPath);
    item->GetVideoInfoTag()->m_fanart.Unpack();
    item->GetVideoInfoTag()->m_strPictureURL.Parse();

    map<string, string> artwork;
    if (!db.GetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork))
    {
      CStdString art = CVideoInfoScanner::GetImage(item.get(), true, item->GetVideoInfoTag()->m_basePath != item->GetPath(), "thumb");
      std::string type;
      if (!art.empty() && CacheTexture(art, cachedThumb, type))
        artwork.insert(make_pair(type, art));

      art = CVideoInfoScanner::GetFanart(item.get(), true);
      if (!art.empty() && CacheTexture(art, cachedFanart))
        artwork.insert(make_pair("fanart", art));

      if (artwork.empty())
        artwork.insert(make_pair("thumb", ""));
      db.SetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork);
    }
  }
  items.Clear();

  // music videos
  db.GetMusicVideosNav("videodb://3/2/", items, false);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    handle->SetProgress(i, items.Size());
    handle->SetText(StringUtils::Format(g_localizeStrings.Get(12350).c_str(), item->GetLabel().c_str()));
    string cachedThumb = GetCachedVideoThumb(*item);
    string cachedFanart = GetCachedFanart(*item);

    item->SetPath(item->GetVideoInfoTag()->m_strFileNameAndPath);
    item->GetVideoInfoTag()->m_fanart.Unpack();
    item->GetVideoInfoTag()->m_strPictureURL.Parse();

    map<string, string> artwork;
    if (!db.GetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork))
    {
      CStdString art = CVideoInfoScanner::GetImage(item.get(), true, item->GetVideoInfoTag()->m_basePath != item->GetPath(), "thumb");
      std::string type;
      if (!art.empty() && CacheTexture(art, cachedThumb, type))
        artwork.insert(make_pair(type, art));

      art = CVideoInfoScanner::GetFanart(item.get(), true);
      if (!art.empty() && CacheTexture(art, cachedFanart))
        artwork.insert(make_pair("fanart", art));

      if (artwork.empty())
        artwork.insert(make_pair("thumb", ""));
      db.SetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork);
    }
  }
  items.Clear();

  // tvshows
  // count the number of episodes
  db.GetTvShowsNav("videodb://3/2/", items);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    handle->SetText(StringUtils::Format(g_localizeStrings.Get(12350).c_str(), item->GetLabel().c_str()));
    string cachedThumb = GetCachedVideoThumb(*item);
    string cachedFanart = GetCachedFanart(*item);

    item->SetPath(item->GetVideoInfoTag()->m_strPath);
    item->GetVideoInfoTag()->m_fanart.Unpack();
    item->GetVideoInfoTag()->m_strPictureURL.Parse();

    map<string, string> artwork;
    if (!db.GetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork))
    {
      CStdString art = CVideoInfoScanner::GetImage(item.get(), true, false, "thumb");
      std::string type;
      if (!art.empty() && CacheTexture(art, cachedThumb, type))
        artwork.insert(make_pair(type, art));

      art = CVideoInfoScanner::GetFanart(item.get(), true);
      if (!art.empty() && CacheTexture(art, cachedFanart))
        artwork.insert(make_pair("fanart", art));

      if (artwork.empty())
        artwork.insert(make_pair("thumb", ""));
      db.SetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork);
    }

    // now season art...
    map<int, map<string, string> > seasons;
    vector<string> artTypes; artTypes.push_back("thumb");
    CVideoInfoScanner::GetSeasonThumbs(*item->GetVideoInfoTag(), seasons, artTypes, true);
    for (map<int, map<string, string> >::const_iterator j = seasons.begin(); j != seasons.end(); ++j)
    {
      if (j->second.empty())
        continue;
      int idSeason = db.AddSeason(item->GetVideoInfoTag()->m_iDbId, j->first);
      map<string, string> seasonArt;
      if (idSeason > -1 && !db.GetArtForItem(idSeason, "season", seasonArt))
      {
        std::string cachedSeason = GetCachedSeasonThumb(j->first, item->GetVideoInfoTag()->m_strPath);
        std::string type;
        if (CacheTexture(j->second.begin()->second, cachedSeason, type))
          db.SetArtForItem(idSeason, "season", type, j->second.begin()->second);
      }
    }

    // now episodes...
    CFileItemList items2;
    db.GetEpisodesByWhere("videodb://2/2/-1/-1/", db.PrepareSQL("episodeview.idShow=%d", item->GetVideoInfoTag()->m_iDbId), items2);
    for (int j = 0; j < items2.Size(); j++)
    {
      handle->SetProgress(j, items2.Size());
      CFileItemPtr episode = items2[j];
      string cachedThumb = GetCachedVideoThumb(*episode);
      episode->SetPath(episode->GetVideoInfoTag()->m_strFileNameAndPath);
      episode->GetVideoInfoTag()->m_strPictureURL.Parse();

      map<string, string> artwork;
      if (!db.GetArtForItem(episode->GetVideoInfoTag()->m_iDbId, episode->GetVideoInfoTag()->m_type, artwork))
      {
        CStdString art = CVideoInfoScanner::GetImage(episode.get(), true, episode->GetVideoInfoTag()->m_basePath != episode->GetPath(), "thumb");
        if (!art.empty() && CacheTexture(art, cachedThumb))
          artwork.insert(make_pair("thumb", art));
        else
          artwork.insert(make_pair("thumb", ""));
        db.SetArtForItem(episode->GetVideoInfoTag()->m_iDbId, episode->GetVideoInfoTag()->m_type, artwork);
      }
    }
  }
  items.Clear();

  // now sets
  db.GetSetsNav("videodb://1/7/", items, VIDEODB_CONTENT_MOVIES);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    handle->SetProgress(i, items.Size());
    handle->SetText(StringUtils::Format(g_localizeStrings.Get(12350).c_str(), item->GetLabel().c_str()));
    map<string, string> artwork;
    if (!db.GetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork))
    { // grab the first movie from this set
      CFileItemList items2;
      db.GetMoviesNav("videodb://1/2/", items2, -1, -1, -1, -1, -1, -1, item->GetVideoInfoTag()->m_iDbId);
      if (items2.Size() > 1)
      {
        if (db.GetArtForItem(items2[0]->GetVideoInfoTag()->m_iDbId, items2[0]->GetVideoInfoTag()->m_type, artwork))
          db.SetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork);
      }
    }
  }
  items.Clear();

  // now actors
  if (g_guiSettings.GetBool("videolibrary.actorthumbs"))
  {
    db.GetActorsNav("videodb://1/2/", items, VIDEODB_CONTENT_MOVIES);
    db.GetActorsNav("videodb://2/2/", items, VIDEODB_CONTENT_TVSHOWS);
    db.GetActorsNav("videodb://2/2/", items, VIDEODB_CONTENT_EPISODES);
    db.GetActorsNav("videodb://3/2/", items, VIDEODB_CONTENT_MUSICVIDEOS);
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      handle->SetProgress(i, items.Size());
      handle->SetText(StringUtils::Format(g_localizeStrings.Get(12350).c_str(), item->GetLabel().c_str()));
      map<string, string> artwork;
      if (!db.GetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork))
      {
        item->GetVideoInfoTag()->m_strPictureURL.Parse();
        string cachedThumb = GetCachedActorThumb(*item);

        string art = CScraperUrl::GetThumbURL(item->GetVideoInfoTag()->m_strPictureURL.GetFirstThumb());
        if (!art.empty() && CacheTexture(art, cachedThumb))
          artwork.insert(make_pair("thumb", art));
        else
          artwork.insert(make_pair("thumb", ""));
        db.SetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artwork);
      }
    }
  }
  handle->MarkFinished();

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnScanFinished");

  items.Clear();
}

bool CEdenVideoArtUpdater::CacheTexture(const std::string &originalUrl, const std::string &cachedFile)
{
  std::string type;
  return CacheTexture(originalUrl, cachedFile, type);
}

bool CEdenVideoArtUpdater::CacheTexture(const std::string &originalUrl, const std::string &cachedFile, std::string &type)
{
  if (!CFile::Exists(cachedFile))
    return false;

  CTextureDetails details;
  details.updateable = false;
  details.hash = "NOHASH";
  type = "thumb"; // unknown art type

  CBaseTexture *texture = CTextureCacheJob::LoadImage(cachedFile, 0, 0, "");
  if (texture)
  {
    if (texture->HasAlpha())
      details.file = CTextureCache::GetCacheFile(originalUrl) + ".png";
    else
      details.file = CTextureCache::GetCacheFile(originalUrl) + ".jpg";

    CLog::Log(LOGDEBUG, "Caching image '%s' ('%s') to '%s':", originalUrl.c_str(), cachedFile.c_str(), details.file.c_str());

    uint32_t width = 0, height = 0;
    if (CPicture::CacheTexture(texture, width, height, CTextureCache::GetCachedPath(details.file)))
    {
      details.width = width;
      details.height = height;
      type = CVideoInfoScanner::GetArtTypeFromSize(details.width, details.height);
      delete texture;
      m_textureDB.AddCachedTexture(originalUrl, details);
      return true;
    }
  }
  CLog::Log(LOGDEBUG, "Can't cache image '%s' ('%s')", originalUrl.c_str(), cachedFile.c_str());
  return false;
}

CStdString CEdenVideoArtUpdater::GetCachedActorThumb(const CFileItem &item)
{
  return GetThumb("actor" + item.GetLabel(), g_settings.GetVideoThumbFolder(), true);
}

CStdString CEdenVideoArtUpdater::GetCachedSeasonThumb(int season, const CStdString &path)
{
  CStdString label;
  if (season == 0)
    label = g_localizeStrings.Get(20381);
  else
    label.Format(g_localizeStrings.Get(20358), season);
  return GetThumb("season" + path + label, g_settings.GetVideoThumbFolder(), true);
}

CStdString CEdenVideoArtUpdater::GetCachedEpisodeThumb(const CFileItem &item)
{
  // get the locally cached thumb
  CStdString strCRC;
  strCRC.Format("%sepisode%i", item.GetVideoInfoTag()->m_strFileNameAndPath.c_str(), item.GetVideoInfoTag()->m_iEpisode);
  return GetThumb(strCRC, g_settings.GetVideoThumbFolder(), true);
}

CStdString CEdenVideoArtUpdater::GetCachedVideoThumb(const CFileItem &item)
{
  if (item.m_bIsFolder && !item.GetVideoInfoTag()->m_strPath.IsEmpty())
    return GetThumb(item.GetVideoInfoTag()->m_strPath, g_settings.GetVideoThumbFolder(), true);
  else if (!item.GetVideoInfoTag()->m_strFileNameAndPath.IsEmpty())
    return GetThumb(item.GetVideoInfoTag()->m_strFileNameAndPath, g_settings.GetVideoThumbFolder(), true);
  return GetThumb(item.GetPath(), g_settings.GetVideoThumbFolder(), true);
}

CStdString CEdenVideoArtUpdater::GetCachedFanart(const CFileItem &item)
{
  if (!item.GetVideoInfoTag()->m_artist.empty())
    return GetThumb(StringUtils::Join(item.GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator), URIUtils::AddFileToFolder(g_settings.GetThumbnailsFolder(), "Music/Fanart/"), false);
  CStdString path = item.GetVideoInfoTag()->GetPath();
  if (path.empty())
    return "";
  return GetThumb(path, URIUtils::AddFileToFolder(g_settings.GetVideoThumbFolder(), "Fanart/"), false);
}

CStdString CEdenVideoArtUpdater::GetThumb(const CStdString &path, const CStdString &path2, bool split)
{
  // get the locally cached thumb
  Crc32 crc;
  crc.ComputeFromLowerCase(path);

  CStdString thumb;
  if (split)
  {
    CStdString hex;
    hex.Format("%08x", (__int32)crc);
    thumb.Format("%c\\%08x.tbn", hex[0], (unsigned __int32)crc);
  }
  else
    thumb.Format("%08x.tbn", (unsigned __int32)crc);

  return URIUtils::AddFileToFolder(path2, thumb);
}
