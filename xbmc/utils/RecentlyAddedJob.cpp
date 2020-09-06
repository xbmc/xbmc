/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RecentlyAddedJob.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "music/MusicDatabase.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"

#if defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/TVOSTopShelf.h"
#endif

#define NUM_ITEMS 10

CRecentlyAddedJob::CRecentlyAddedJob(int flag)
{
  m_flag = flag;
}

bool CRecentlyAddedJob::UpdateVideo()
{
  auto home = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_HOME);

  if ( home == nullptr )
    return false;

  CLog::Log(LOGDEBUG, "CRecentlyAddedJob::UpdateVideos() - Running RecentlyAdded home screen update");

  int            i = 0;
  CFileItemList  items;
  CVideoDatabase videodatabase;
  CVideoThumbLoader loader;
  loader.OnLoaderStart();

  videodatabase.Open();

  if (videodatabase.GetRecentlyAddedMoviesNav("videodb://recentlyaddedmovies/", items, NUM_ITEMS))
  {
    for (; i < items.Size(); ++i)
    {
      auto item = items.Get(i);
      std::string   value = StringUtils::Format("%i", i + 1);
      std::string   strRating = StringUtils::Format("%.1f", item->GetVideoInfoTag()->GetRating().rating);

      home->SetProperty("LatestMovie." + value + ".Title"       , item->GetLabel());
      home->SetProperty("LatestMovie." + value + ".Rating"      , strRating);
      home->SetProperty("LatestMovie." + value + ".Year"        , item->GetVideoInfoTag()->GetYear());
      home->SetProperty("LatestMovie." + value + ".Plot"        , item->GetVideoInfoTag()->m_strPlot);
      home->SetProperty("LatestMovie." + value + ".RunningTime" , item->GetVideoInfoTag()->GetDuration() / 60);
      home->SetProperty("LatestMovie." + value + ".Path"        , item->GetVideoInfoTag()->m_strFileNameAndPath);
      home->SetProperty("LatestMovie." + value + ".Trailer"     , item->GetVideoInfoTag()->m_strTrailer);

      if (!item->HasArt("thumb"))
        loader.LoadItem(item.get());

      home->SetProperty("LatestMovie." + value + ".Thumb"       , item->GetArt("thumb"));
      home->SetProperty("LatestMovie." + value + ".Fanart"      , item->GetArt("fanart"));
      home->SetProperty("LatestMovie." + value + ".Poster"      , item->GetArt("poster"));
    }
  }
  for (; i < NUM_ITEMS; ++i)
  {
    std::string value = StringUtils::Format("%i", i + 1);
    home->SetProperty("LatestMovie." + value + ".Title"       , "");
    home->SetProperty("LatestMovie." + value + ".Thumb"       , "");
    home->SetProperty("LatestMovie." + value + ".Rating"      , "");
    home->SetProperty("LatestMovie." + value + ".Year"        , "");
    home->SetProperty("LatestMovie." + value + ".Plot"        , "");
    home->SetProperty("LatestMovie." + value + ".RunningTime" , "");
    home->SetProperty("LatestMovie." + value + ".Path"        , "");
    home->SetProperty("LatestMovie." + value + ".Trailer"     , "");
    home->SetProperty("LatestMovie." + value + ".Fanart"      , "");
    home->SetProperty("LatestMovie." + value + ".Poster"      , "");
  }

  i = 0;
  CFileItemList  TVShowItems;

  if (videodatabase.GetRecentlyAddedEpisodesNav("videodb://recentlyaddedepisodes/", TVShowItems, NUM_ITEMS))
  {
    for (; i < TVShowItems.Size(); ++i)
    {
      auto item          = TVShowItems.Get(i);
      int          EpisodeSeason = item->GetVideoInfoTag()->m_iSeason;
      int          EpisodeNumber = item->GetVideoInfoTag()->m_iEpisode;
      std::string   EpisodeNo = StringUtils::Format("s%02de%02d", EpisodeSeason, EpisodeNumber);
      std::string   value = StringUtils::Format("%i", i + 1);
      std::string   strRating = StringUtils::Format("%.1f", item->GetVideoInfoTag()->GetRating().rating);

      home->SetProperty("LatestEpisode." + value + ".ShowTitle"     , item->GetVideoInfoTag()->m_strShowTitle);
      home->SetProperty("LatestEpisode." + value + ".EpisodeTitle"  , item->GetVideoInfoTag()->m_strTitle);
      home->SetProperty("LatestEpisode." + value + ".Rating"        , strRating);
      home->SetProperty("LatestEpisode." + value + ".Plot"          , item->GetVideoInfoTag()->m_strPlot);
      home->SetProperty("LatestEpisode." + value + ".EpisodeNo"     , EpisodeNo);
      home->SetProperty("LatestEpisode." + value + ".EpisodeSeason" , EpisodeSeason);
      home->SetProperty("LatestEpisode." + value + ".EpisodeNumber" , EpisodeNumber);
      home->SetProperty("LatestEpisode." + value + ".Path"          , item->GetVideoInfoTag()->m_strFileNameAndPath);

      if (!item->HasArt("thumb"))
        loader.LoadItem(item.get());

      std::string seasonThumb;
      if (item->GetVideoInfoTag()->m_iIdSeason > 0)
        seasonThumb = videodatabase.GetArtForItem(item->GetVideoInfoTag()->m_iIdSeason, MediaTypeSeason, "thumb");

      home->SetProperty("LatestEpisode." + value + ".Thumb"         , item->GetArt("thumb"));
      home->SetProperty("LatestEpisode." + value + ".ShowThumb"     , item->GetArt("tvshow.thumb"));
      home->SetProperty("LatestEpisode." + value + ".SeasonThumb"   , seasonThumb);
      home->SetProperty("LatestEpisode." + value + ".Fanart"        , item->GetArt("fanart"));
    }
  }
  for (; i < NUM_ITEMS; ++i)
  {
    std::string value = StringUtils::Format("%i", i + 1);
    home->SetProperty("LatestEpisode." + value + ".ShowTitle"     , "");
    home->SetProperty("LatestEpisode." + value + ".EpisodeTitle"  , "");
    home->SetProperty("LatestEpisode." + value + ".Rating"        , "");
    home->SetProperty("LatestEpisode." + value + ".Plot"          , "");
    home->SetProperty("LatestEpisode." + value + ".EpisodeNo"     , "");
    home->SetProperty("LatestEpisode." + value + ".EpisodeSeason" , "");
    home->SetProperty("LatestEpisode." + value + ".EpisodeNumber" , "");
    home->SetProperty("LatestEpisode." + value + ".Path"          , "");
    home->SetProperty("LatestEpisode." + value + ".Thumb"         , "");
    home->SetProperty("LatestEpisode." + value + ".ShowThumb"     , "");
    home->SetProperty("LatestEpisode." + value + ".SeasonThumb"   , "");
    home->SetProperty("LatestEpisode." + value + ".Fanart"        , "");
  }

#if defined(TARGET_DARWIN_TVOS)
  // Add recently added Movies and TvShows items on tvOS Kodi TopShelf
  CTVOSTopShelf::GetInstance().SetTopShelfItems(items, TVOSTopShelfItemsCategory::MOVIES);
  CTVOSTopShelf::GetInstance().SetTopShelfItems(TVShowItems, TVOSTopShelfItemsCategory::TV_SHOWS);
#endif

  i = 0;
  CFileItemList MusicVideoItems;

  if (videodatabase.GetRecentlyAddedMusicVideosNav("videodb://recentlyaddedmusicvideos/", MusicVideoItems, NUM_ITEMS))
  {
    for (; i < MusicVideoItems.Size(); ++i)
    {
      auto item = MusicVideoItems.Get(i);
      std::string   value = StringUtils::Format("%i", i + 1);

      home->SetProperty("LatestMusicVideo." + value + ".Title"       , item->GetLabel());
      home->SetProperty("LatestMusicVideo." + value + ".Year"        , item->GetVideoInfoTag()->GetYear());
      home->SetProperty("LatestMusicVideo." + value + ".Plot"        , item->GetVideoInfoTag()->m_strPlot);
      home->SetProperty("LatestMusicVideo." + value + ".RunningTime" , item->GetVideoInfoTag()->GetDuration() / 60);
      home->SetProperty("LatestMusicVideo." + value + ".Path"        , item->GetVideoInfoTag()->m_strFileNameAndPath);
      home->SetProperty("LatestMusicVideo." + value + ".Artist"      , StringUtils::Join(item->GetVideoInfoTag()->m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator));

      if (!item->HasArt("thumb"))
        loader.LoadItem(item.get());

      home->SetProperty("LatestMusicVideo." + value + ".Thumb"       , item->GetArt("thumb"));
      home->SetProperty("LatestMusicVideo." + value + ".Fanart"      , item->GetArt("fanart"));
    }
  }
  for (; i < NUM_ITEMS; ++i)
  {
    std::string value = StringUtils::Format("%i", i + 1);
    home->SetProperty("LatestMusicVideo." + value + ".Title"       , "");
    home->SetProperty("LatestMusicVideo." + value + ".Thumb"       , "");
    home->SetProperty("LatestMusicVideo." + value + ".Year"        , "");
    home->SetProperty("LatestMusicVideo." + value + ".Plot"        , "");
    home->SetProperty("LatestMusicVideo." + value + ".RunningTime" , "");
    home->SetProperty("LatestMusicVideo." + value + ".Path"        , "");
    home->SetProperty("LatestMusicVideo." + value + ".Artist"      , "");
    home->SetProperty("LatestMusicVideo." + value + ".Fanart"      , "");
  }

  videodatabase.Close();
  return true;
}

bool CRecentlyAddedJob::UpdateMusic()
{
  auto home = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_HOME);

  if ( home == nullptr )
    return false;

  CLog::Log(LOGDEBUG, "CRecentlyAddedJob::UpdateMusic() - Running RecentlyAdded home screen update");

  int            i = 0;
  CFileItemList  musicItems;
  CMusicDatabase musicdatabase;
  CMusicThumbLoader loader;
  loader.OnLoaderStart();

  musicdatabase.Open();

  if (musicdatabase.GetRecentlyAddedAlbumSongs("musicdb://songs/", musicItems, NUM_ITEMS))
  {
    long idAlbum = -1;
    std::string strAlbumThumb;
    std::string strAlbumFanart;
    for (; i < musicItems.Size(); ++i)
    {
      auto item = musicItems.Get(i);
      std::string   value = StringUtils::Format("%d", i + 1);

      std::string   strRating;
      std::string   strAlbum  = item->GetMusicInfoTag()->GetAlbum();
      std::string   strArtist = item->GetMusicInfoTag()->GetArtistString();

      if (idAlbum != item->GetMusicInfoTag()->GetAlbumId())
      {
        strAlbumThumb.clear();
        strAlbumFanart.clear();
        idAlbum = item->GetMusicInfoTag()->GetAlbumId();

        if (loader.LoadItem(item.get()))
        {
          strAlbumThumb = item->GetArt("thumb");
          strAlbumFanart = item->GetArt("fanart");
        }
      }

      strRating = StringUtils::Format("%c", item->GetMusicInfoTag()->GetUserrating());

      home->SetProperty("LatestSong." + value + ".Title"   , item->GetMusicInfoTag()->GetTitle());
      home->SetProperty("LatestSong." + value + ".Year"    , item->GetMusicInfoTag()->GetYear());
      home->SetProperty("LatestSong." + value + ".Artist"  , strArtist);
      home->SetProperty("LatestSong." + value + ".Album"   , strAlbum);
      home->SetProperty("LatestSong." + value + ".Rating"  , strRating);
      home->SetProperty("LatestSong." + value + ".Path"    , item->GetMusicInfoTag()->GetURL());
      home->SetProperty("LatestSong." + value + ".Thumb"   , strAlbumThumb);
      home->SetProperty("LatestSong." + value + ".Fanart"  , strAlbumFanart);
    }
  }
  for (; i < NUM_ITEMS; ++i)
  {
    std::string value = StringUtils::Format("%i", i + 1);
    home->SetProperty("LatestSong." + value + ".Title"   , "");
    home->SetProperty("LatestSong." + value + ".Year"    , "");
    home->SetProperty("LatestSong." + value + ".Artist"  , "");
    home->SetProperty("LatestSong." + value + ".Album"   , "");
    home->SetProperty("LatestSong." + value + ".Rating"  , "");
    home->SetProperty("LatestSong." + value + ".Path"    , "");
    home->SetProperty("LatestSong." + value + ".Thumb"   , "");
    home->SetProperty("LatestSong." + value + ".Fanart"  , "");
  }

  i = 0;
  VECALBUMS albums;

  if (musicdatabase.GetRecentlyAddedAlbums(albums, NUM_ITEMS))
  {
    size_t j = 0;
    for (; j < albums.size(); ++j)
    {
      auto& album=albums[j];
      std::string value = StringUtils::Format("%lu", j + 1);
      std::string strThumb;
      std::string strFanart;
      bool artfound = false;
      std::vector<ArtForThumbLoader> art;
      // Get album thumb and fanart for first album artist
      artfound = musicdatabase.GetArtForItem(-1, album.idAlbum, -1, true, art);
      if (artfound)
      {
        for (auto artitem : art)
        {
          if (artitem.mediaType == MediaTypeAlbum && artitem.artType == "thumb")
            strThumb = artitem.url;
          else if (artitem.mediaType == MediaTypeArtist && artitem.artType == "fanart")
            strFanart = artitem.url;
        }
      }

      std::string strDBpath = StringUtils::Format("musicdb://albums/%li/", album.idAlbum);

      home->SetProperty("LatestAlbum." + value + ".Title"   , album.strAlbum);
      home->SetProperty("LatestAlbum." + value + ".Year"    , album.strReleaseDate);
      home->SetProperty("LatestAlbum." + value + ".Artist"  , album.GetAlbumArtistString());
      home->SetProperty("LatestAlbum." + value + ".Rating"  , album.fRating);
      home->SetProperty("LatestAlbum." + value + ".Path"    , strDBpath);
      home->SetProperty("LatestAlbum." + value + ".Thumb"   , strThumb);
      home->SetProperty("LatestAlbum." + value + ".Fanart"  , strFanart);
    }
    i = j;
  }
  for (; i < NUM_ITEMS; ++i)
  {
    std::string value = StringUtils::Format("%i", i + 1);
    home->SetProperty("LatestAlbum." + value + ".Title"   , "");
    home->SetProperty("LatestAlbum." + value + ".Year"    , "");
    home->SetProperty("LatestAlbum." + value + ".Artist"  , "");
    home->SetProperty("LatestAlbum." + value + ".Rating"  , "");
    home->SetProperty("LatestAlbum." + value + ".Path"    , "");
    home->SetProperty("LatestAlbum." + value + ".Thumb"   , "");
    home->SetProperty("LatestAlbum." + value + ".Fanart"  , "");
  }

  musicdatabase.Close();
  return true;
}

bool CRecentlyAddedJob::UpdateTotal()
{
  auto home = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_HOME);

  if ( home == nullptr )
    return false;

  CLog::Log(LOGDEBUG, "CRecentlyAddedJob::UpdateTotal() - Running RecentlyAdded home screen update");

  CVideoDatabase videodatabase;
  CMusicDatabase musicdatabase;

  musicdatabase.Open();

  CMusicDbUrl musicUrl;
  musicUrl.FromString("musicdb://artists/");
  musicUrl.AddOption("albumartistsonly", !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS));

  CFileItemList items;
  CDatabase::Filter filter;
  musicdatabase.GetArtistsByWhere(musicUrl.ToString(), filter, items, SortDescription(), true);
  int MusArtistTotals = 0;
  if (items.Size() == 1 && items.Get(0)->HasProperty("total"))
    MusArtistTotals = items.Get(0)->GetProperty("total").asInteger();

  int MusSongTotals   = atoi(musicdatabase.GetSingleValue("songview"       , "count(1)").c_str());
  int MusAlbumTotals  = atoi(musicdatabase.GetSingleValue("songview"       , "count(distinct strAlbum)").c_str());
  musicdatabase.Close();

  videodatabase.Open();
  int tvShowCount     = atoi(videodatabase.GetSingleValue("tvshow_view"     , "count(1)").c_str());
  int movieTotals     = atoi(videodatabase.GetSingleValue("movie_view"      , "count(1)").c_str());
  int movieWatched    = atoi(videodatabase.GetSingleValue("movie_view"      , "count(playCount)").c_str());
  int MusVidTotals    = atoi(videodatabase.GetSingleValue("musicvideo_view" , "count(1)").c_str());
  int MusVidWatched   = atoi(videodatabase.GetSingleValue("musicvideo_view" , "count(playCount)").c_str());
  int EpWatched       = atoi(videodatabase.GetSingleValue("tvshow_view"     , "sum(watchedcount)").c_str());
  int EpCount         = atoi(videodatabase.GetSingleValue("tvshow_view"     , "sum(totalcount)").c_str());
  int TvShowsWatched  = atoi(videodatabase.GetSingleValue("tvshow_view"     , "sum(watchedcount = totalcount)").c_str());
  videodatabase.Close();

  home->SetProperty("TVShows.Count"         , tvShowCount);
  home->SetProperty("TVShows.Watched"       , TvShowsWatched);
  home->SetProperty("TVShows.UnWatched"     , tvShowCount - TvShowsWatched);
  home->SetProperty("Episodes.Count"        , EpCount);
  home->SetProperty("Episodes.Watched"      , EpWatched);
  home->SetProperty("Episodes.UnWatched"    , EpCount-EpWatched);
  home->SetProperty("Movies.Count"          , movieTotals);
  home->SetProperty("Movies.Watched"        , movieWatched);
  home->SetProperty("Movies.UnWatched"      , movieTotals - movieWatched);
  home->SetProperty("MusicVideos.Count"     , MusVidTotals);
  home->SetProperty("MusicVideos.Watched"   , MusVidWatched);
  home->SetProperty("MusicVideos.UnWatched" , MusVidTotals - MusVidWatched);
  home->SetProperty("Music.SongsCount"      , MusSongTotals);
  home->SetProperty("Music.AlbumsCount"     , MusAlbumTotals);
  home->SetProperty("Music.ArtistsCount"    , MusArtistTotals);

  return true;
}


bool CRecentlyAddedJob::DoWork()
{
  bool ret = true;
  if (m_flag & Audio)
    ret &= UpdateMusic();

  if (m_flag & Video)
    ret &= UpdateVideo();

  if (m_flag & Totals)
    ret &= UpdateTotal();

  return ret;
}
