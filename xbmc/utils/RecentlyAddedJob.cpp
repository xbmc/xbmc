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

#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "FileItem.h"
#include "RecentlyAddedJob.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "music/MusicThumbLoader.h"
#include "video/VideoThumbLoader.h"

#define NUM_ITEMS 10

CRecentlyAddedJob::CRecentlyAddedJob(int flag)
{
  m_flag = flag;
} 

bool CRecentlyAddedJob::UpdateVideo()
{
  CGUIWindow* home = g_windowManager.GetWindow(WINDOW_HOME);

  if ( home == NULL )
    return false;

  CLog::Log(LOGDEBUG, "CRecentlyAddedJob::UpdateVideos() - Running RecentlyAdded home screen update");
  
  int i;
  CVideoDatabase videodatabase;
  CVideoThumbLoader loader;
  loader.OnLoaderStart();
  
  videodatabase.Open();

  i = 0;
  CFileItemList          MovieItems;
  CVideoDatabase::Filter MovieFilter;
  MovieFilter.limit = videodatabase.PrepareSQL("%u", NUM_ITEMS);
  MovieFilter.order = "dateAdded desc, idMovie desc";
  if (!CSettings::Get().GetBool("videolibrary.showseeninhome"))
    MovieFilter.where = "playCount is NULL";

  if (videodatabase.GetMoviesByWhere("videodb://recentlyaddedmovies/", MovieFilter, MovieItems))
  {
    for (; i < MovieItems.Size(); ++i)
    {
      CFileItemPtr item      = MovieItems.Get(i);
      std::string   value     = StringUtils::Format("%i", i + 1);
      std::string   strRating = StringUtils::Format("%.1f", item->GetVideoInfoTag()->m_fRating);;

      home->SetProperty("LatestMovie." + value + ".Title"       , item->GetLabel());
      home->SetProperty("LatestMovie." + value + ".Rating"      , strRating);
      home->SetProperty("LatestMovie." + value + ".Year"        , item->GetVideoInfoTag()->m_iYear);
      home->SetProperty("LatestMovie." + value + ".Plot"        , item->GetVideoInfoTag()->m_strPlot);
      home->SetProperty("LatestMovie." + value + ".RunningTime" , item->GetVideoInfoTag()->GetDuration() / 60);
      home->SetProperty("LatestMovie." + value + ".Path"        , item->GetVideoInfoTag()->m_strFileNameAndPath);
      home->SetProperty("LatestMovie." + value + ".Trailer"     , item->GetVideoInfoTag()->m_strTrailer);

      if (!item->HasArt("thumb"))
        loader.LoadItem(item.get());

      home->SetProperty("LatestMovie." + value + ".Thumb"       , item->GetArt("thumb"));
      home->SetProperty("LatestMovie." + value + ".Fanart"      , item->GetArt("fanart"));
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
  }

  i = 0;
  if (CSettings::Get().GetInt("videolibrary.tvshowsinhome") == SETTINGS_VIDEODB_TVSHOW_HOME_EPISODES)
  {
    home->SetProperty("LatestEpisode.Title", g_localizeStrings.Get(20387));
    CFileItemList          EpisodesItems;
    CVideoDatabase::Filter EpisodesFilter;
    EpisodesFilter.limit = videodatabase.PrepareSQL("%u", NUM_ITEMS);
    EpisodesFilter.order = "dateAdded desc, idEpisode desc";
    if (!CSettings::Get().GetBool("videolibrary.showseeninhome"))
      EpisodesFilter.where = "playCount is NULL";

    if (videodatabase.GetEpisodesByWhere("videodb://recentlyaddedepisodes/", EpisodesFilter, EpisodesItems, NUM_ITEMS))
    {
      for (; i < EpisodesItems.Size(); ++i)
      {
        CFileItemPtr item          = EpisodesItems.Get(i);
        int          EpisodeSeason = item->GetVideoInfoTag()->m_iSeason;
        int          EpisodeNumber = item->GetVideoInfoTag()->m_iEpisode;
        std::string  EpisodeNo     = StringUtils::Format("s%02de%02d", EpisodeSeason, EpisodeNumber);
        std::string  value         = StringUtils::Format("%i", i + 1);
        std::string  strRating     = StringUtils::Format("%.1f", item->GetVideoInfoTag()->m_fRating);

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
  }
  else
  {
    CFileItemList          TVShowItems;
    CVideoDatabase::Filter TVShowFilter;
    TVShowFilter.limit = videodatabase.PrepareSQL("%u", NUM_ITEMS);
    switch (CSettings::Get().GetInt("videolibrary.tvshowsinhome"))
    {
      case SETTINGS_VIDEODB_TVSHOW_HOME_RECENTLY_ADDED:
      {
        home->SetProperty("LatestEpisode.Title", g_localizeStrings.Get(38017));
        TVShowFilter.order = "dateAdded desc, idShow desc";
        if (CSettings::Get().GetBool("videolibrary.showseeninhome"))
          TVShowFilter.where = "totalcount != 0";
        else
          TVShowFilter.where = "totalcount != 0 AND watchedcount != totalcount";
        break;
      }
      case SETTINGS_VIDEODB_TVSHOW_HOME_INPROGRESS:
      {
        home->SetProperty("LatestEpisode.Title", g_localizeStrings.Get(626));
        TVShowFilter.order = "dateAdded desc, idShow desc";
        TVShowFilter.where = "watchedcount != 0 AND watchedcount != totalcount";
        break;
      }
      case SETTINGS_VIDEODB_TVSHOW_HOME_INPROGRESS_RECENTLY_ADDED:
      {
        home->SetProperty("LatestEpisode.Title", g_localizeStrings.Get(38018));
        TVShowFilter.order = "(watchedCount != 0 AND watchedCount != totalCount) desc, dateAdded desc, idShow desc";
        if (CSettings::Get().GetBool("videolibrary.showseeninhome"))
          TVShowFilter.where = "totalcount != 0";
        else
          TVShowFilter.where = "totalcount != 0 AND watchedcount != totalcount";
        break;
      }
    }
    if (videodatabase.GetTvShowsByWhere("videodb://tvshows/", TVShowFilter, TVShowItems))
    {
      int failed = 0;
      for (; i < TVShowItems.Size(); ++i)
      {
        CFileItemPtr item  = TVShowItems.Get(i);
        int          dbID  = item->GetVideoInfoTag()->m_iDbId;
        CFileItemPtr nextEpisode;
        if (!videodatabase.GetNextEpisodeFromTvShow(dbID, nextEpisode))
        {
          CLog::Log(LOGWARNING, "CRecentlyAddedJob::UpdateVideos() - Failed to load episode for tvshow %u", dbID);
          failed++;
          continue;
        }
        std::string  value = StringUtils::Format("%i", i + 1 - failed);

        // tvshow values
        std::string  strRating = StringUtils::Format("%.1f", item->GetVideoInfoTag()->m_fRating);
        home->SetProperty("LatestEpisode." + value + ".ShowTitle"     , item->GetVideoInfoTag()->m_strShowTitle);
        home->SetProperty("LatestEpisode." + value + ".Rating"        , strRating);
        home->SetProperty("LatestEpisode." + value + ".Plot"          , item->GetVideoInfoTag()->m_strPlot);

        if (!item->HasArt("thumb"))
          loader.LoadItem(item.get());
        home->SetProperty("LatestEpisode." + value + ".Thumb"         , item->GetArt("thumb"));
        home->SetProperty("LatestEpisode." + value + ".ShowThumb"     , item->GetArt("thumb"));
        home->SetProperty("LatestEpisode." + value + ".Fanart"        , item->GetArt("fanart"));

        // episode value
        int          EpisodeSeason = nextEpisode->GetVideoInfoTag()->m_iSeason;
        int          EpisodeNumber = nextEpisode->GetVideoInfoTag()->m_iEpisode;
        std::string  EpisodeNo     = StringUtils::Format("s%02de%02d", EpisodeSeason, EpisodeNumber);
        std::string  seasonThumb;
        if (nextEpisode->GetVideoInfoTag()->m_iIdSeason > 0)
          seasonThumb = videodatabase.GetArtForItem(nextEpisode->GetVideoInfoTag()->m_iIdSeason, MediaTypeSeason, "thumb");
        home->SetProperty("LatestEpisode." + value + ".EpisodeTitle"  , nextEpisode->GetVideoInfoTag()->m_strTitle);
        home->SetProperty("LatestEpisode." + value + ".EpisodeNo"     , EpisodeNo);
        home->SetProperty("LatestEpisode." + value + ".EpisodeSeason" , EpisodeSeason);
        home->SetProperty("LatestEpisode." + value + ".EpisodeNumber" , EpisodeNumber);
        home->SetProperty("LatestEpisode." + value + ".Path"          , nextEpisode->GetVideoInfoTag()->m_strFileNameAndPath);
        home->SetProperty("LatestEpisode." + value + ".SeasonThumb"   , seasonThumb);
      }
      i = i - failed;
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

  i = 0;
  CFileItemList          MusicVideoItems;
  CVideoDatabase::Filter MusicVideoFilter;
  MusicVideoFilter.limit = videodatabase.PrepareSQL("%u", NUM_ITEMS);
  MusicVideoFilter.order = "dateAdded desc, idMVideo desc";
  if (!CSettings::Get().GetBool("videolibrary.showseeninhome"))
	  MusicVideoFilter.where = "playCount is NULL";

  if (videodatabase.GetMusicVideosByWhere("videodb://recentlyaddedmusicvideos/", MusicVideoFilter, MusicVideoItems))
  {
    for (; i < MusicVideoItems.Size(); ++i)
    {
      CFileItemPtr item = MusicVideoItems.Get(i);
      std::string   value = StringUtils::Format("%i", i + 1);

      home->SetProperty("LatestMusicVideo." + value + ".Title"       , item->GetLabel());
      home->SetProperty("LatestMusicVideo." + value + ".Year"        , item->GetVideoInfoTag()->m_iYear);
      home->SetProperty("LatestMusicVideo." + value + ".Plot"        , item->GetVideoInfoTag()->m_strPlot);
      home->SetProperty("LatestMusicVideo." + value + ".RunningTime" , item->GetVideoInfoTag()->GetDuration() / 60);
      home->SetProperty("LatestMusicVideo." + value + ".Path"        , item->GetVideoInfoTag()->m_strFileNameAndPath);
      home->SetProperty("LatestMusicVideo." + value + ".Artist"      , StringUtils::Join(item->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator));

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
  CGUIWindow* home = g_windowManager.GetWindow(WINDOW_HOME);
  
  if ( home == NULL )
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
      CFileItemPtr item = musicItems.Get(i);
      std::string   value = StringUtils::Format("%i", i + 1);
      
      std::string   strRating;
      std::string   strAlbum  = item->GetMusicInfoTag()->GetAlbum();
      std::string   strArtist = StringUtils::Join(item->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator);

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

      strRating = StringUtils::Format("%c", item->GetMusicInfoTag()->GetRating());
      
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
    for (; i < (int)albums.size(); ++i)
    {
      CAlbum&    album=albums[i];
      std::string value = StringUtils::Format("%i", i + 1);
      std::string strThumb = musicdatabase.GetArtForItem(album.idAlbum, MediaTypeAlbum, "thumb");
      std::string strFanart = musicdatabase.GetArtistArtForItem(album.idAlbum, MediaTypeAlbum, "fanart");
      std::string strDBpath = StringUtils::Format("musicdb://albums/%li/", album.idAlbum);
      
      home->SetProperty("LatestAlbum." + value + ".Title"   , album.strAlbum);
      home->SetProperty("LatestAlbum." + value + ".Year"    , album.iYear);
      home->SetProperty("LatestAlbum." + value + ".Artist"  , album.artist);
      home->SetProperty("LatestAlbum." + value + ".Rating"  , album.iRating);
      home->SetProperty("LatestAlbum." + value + ".Path"    , strDBpath);
      home->SetProperty("LatestAlbum." + value + ".Thumb"   , strThumb);
      home->SetProperty("LatestAlbum." + value + ".Fanart"  , strFanart);
    }
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
  CGUIWindow* home = g_windowManager.GetWindow(WINDOW_HOME);
  
  if ( home == NULL )
    return false;
  
  CLog::Log(LOGDEBUG, "CRecentlyAddedJob::UpdateTotal() - Running RecentlyAdded home screen update");
  
  CVideoDatabase videodatabase;  
  CMusicDatabase musicdatabase;
  
  musicdatabase.Open();
  int MusSongTotals   = atoi(musicdatabase.GetSingleValue("songview"       , "count(1)").c_str());
  int MusAlbumTotals  = atoi(musicdatabase.GetSingleValue("songview"       , "count(distinct strAlbum)").c_str());
  int MusArtistTotals = atoi(musicdatabase.GetSingleValue("songview"       , "count(distinct strArtists)").c_str());
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
