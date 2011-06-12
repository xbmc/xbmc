/*
    spotyxbmc - A project to integrate Spotify into XBMC
    Copyright (C) 2010  David Erenger

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    For contact with the author:
    david.erenger@gmail.com
*/

#include "spotinterface.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "music/MusicDatabase.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "GUIInfoManager.h"
#include "Application.h"
#include "guilib/LocalizeStrings.h"
#include <cstdlib>
#include <stdint.h>
#include "music/tags/MusicInfoTag.h"
#include "filesystem/FileMusicDatabase.h"
#include "music/MusicDatabase.h"
#include "music/dialogs/GUIDialogMusicScan.h"
#include "dialogs/GUIDialogYesNo.h"
#include "pictures/Picture.h"
#include "settings/Settings.h"
#include "guilib/Texture.h"
#include "filesystem/Directory.h"
#include "dialogs/GUIDialogBusy.h"
#include "cores/paplayer/spotifyCodec.h"

using namespace std;
using namespace XFILE;

SpotifyInterface *g_spotifyInterface;



//spotify session callbacks
bool SpotifyInterface::processEvents()
{
  //did the user cancel the connection process?
  if (m_isShowingReconnect && m_reconectingDialog->IsCanceled())
  {
    hideReconectingDialog();
    disconnect();
  }
  //do we need to process the spotify api?
  int now = CTimeUtils::GetTimeMS();
  if (now >= m_nextEvent)
  {
    sp_session_process_events(g_spotifyInterface->m_session, &m_nextEvent);
    m_nextEvent += now;
  }
  return true;
}

void SpotifyInterface::cb_connectionError(sp_session *session, sp_error error)
{
  CStdString message;
  message.Format("%s",sp_error_message(error));
  CLog::Log( LOGERROR, "Spotifylog: connection to Spotify failed: %s\n", message.c_str());
  g_spotifyInterface->hideReconectingDialog();
  g_spotifyInterface->showConnectionErrorDialog(error);
}

void SpotifyInterface::cb_loggedIn(sp_session *session, sp_error error)
{
  if (SP_ERROR_OK != error) {
    CLog::Log( LOGERROR, "Spotifylog: failed to log in to Spotify: %s\n", sp_error_message(error));
    sp_session_release(g_spotifyInterface->m_session);
    g_spotifyInterface->m_session = 0;
    g_spotifyInterface->showConnectionErrorDialog(error);
    return;
  }
  sp_user *me = sp_session_user(session);
  const char *my_name = (sp_user_is_loaded(me) ?
                         sp_user_display_name(me) :
                         sp_user_canonical_name(me));
  CLog::Log( LOGDEBUG, "Spotifylog: Logged in to Spotify as user %s\n", my_name);
  g_spotifyInterface->hideReconectingDialog();
}

void SpotifyInterface::cb_loggedOut(sp_session *session)
{
    CLog::Log( LOGNOTICE, "Spotifylog: disconnected to Spotify!");
  g_spotifyInterface->m_isWaitingForLogout = false; 
}

void SpotifyInterface::cb_notifyMainThread(sp_session *session)
{
  //spotify needs to advance itself, set it to do so the next tick
  g_spotifyInterface->m_nextEvent = CTimeUtils::GetTimeMS();
}

void SpotifyInterface::cb_logMessage(sp_session *session, const char *data)
{
  CLog::Log( LOGNOTICE, "Spotifylog: %s\n", data);
}

//thumb delivery callback
void SpotifyInterface::cb_imageLoaded(sp_image *image, void *userdata)
{
  if (image)
  {
    try{
      CFileItem *item = (CFileItem*)userdata;
      CStdString fileName;
      fileName.Format("%s", item->GetExtraInfo());    

      //if there is a wierd name, something is wrong, or do we allready have the image, return
      if (XFILE::CFile::Exists(fileName)) 
      {
        item->SetThumbnailImage(fileName);
	
        return;
      }
      if (fileName.Left(10) != "special://")
      {
        return; //without a new thumb!
      }

      CFile file;
      if (file.OpenForWrite(fileName,true))
      {
        const void *buf;
        size_t len, written;

        buf = sp_image_data(image, &len);
        written = file.Write(buf, len);
        if (written != len)
        {
          CLog::Log( LOGERROR, "Spotifylog: error creating thumb %s", fileName.c_str());
          file.Close();
          file.Delete(fileName);
        }
        else
        {
          item->SetThumbnailImage(fileName);
          file.Close();
        }
      }
    }catch(...)
    {
      CLog::Log( LOGERROR, "Spotifylog: error creating thumb");
    }
  }
  g_spotifyInterface->m_noWaitingThumbs--;  
}

//load the tracks from a playlist
bool SpotifyInterface::getPlaylistTracks(CFileItemList &items, int playlist)
{
  CLog::Log( LOGDEBUG, "Spotifylog: loading playlist: %i", playlist);
  sp_playlistcontainer *pc = sp_session_playlistcontainer (m_session);
  sp_playlist *pl = sp_playlistcontainer_playlist(pc, playlist);
  if (pl)
  {
    for (int index=0; index < sp_playlist_num_tracks(pl); index++)
    {
      CFileItemPtr pItem;
      pItem = spTrackToItem(sp_playlist_track(pl,index), PLAYLIST_TRACK, true);
//      pItem->SetContentType("audio/spotify");
      items.Add(pItem);
    }
  }
  return true;
}

//browse and search callbacks
void SpotifyInterface::cb_albumBrowseComplete(sp_albumbrowse *result, void *userdata)
{
  SpotifyInterface *spInt = g_spotifyInterface;
  if (result && SP_ERROR_OK == sp_albumbrowse_error(result) && sp_albumbrowse_num_tracks > 0)
  {
    spInt->m_progressDialog->SetPercentage(50);
    spInt->m_progressDialog->Progress();

    //the first track, load it with thumbnail
    CFileItemPtr pItem;
    pItem = spInt->spTrackToItem(sp_albumbrowse_track(result, 0), ALBUMBROWSE_TRACK, true);
//    pItem->SetContentType("audio/spotify");
    spInt->m_browseAlbumVector.Add(pItem);

    CStdString oldThumb = pItem->GetExtraInfo();
    CStdString newThumb;
    newThumb.Format("%s%s", spInt->m_currentPlayingDir, URIUtils::GetFileName(oldThumb));
    CPicture::CacheThumb(oldThumb ,newThumb);
    pItem->SetThumbnailImage(newThumb);

    //add the "add to library" item
    CMusicDatabase *musicdatabase = new CMusicDatabase;
    musicdatabase->Open();
    MUSIC_INFO::CMusicInfoTag *tag = pItem->GetMusicInfoTag();
    int albumId = musicdatabase->GetAlbumByName(tag->GetAlbum());
    if (albumId == -1)
    {
      CAlbum album;
      album.iYear = tag->GetYear();
      album.strArtist = tag->GetAlbumArtist();
      album.strAlbum = tag->GetAlbum();
      CFileItemPtr pItem3(new CFileItem("musicdb://spotify/command/addalbum/",album));
      pItem3->m_strPath = "musicdb://spotify/command/addalbum/";
      pItem3->m_strTitle = "Add album to library";
      pItem3->SetLabel("Add album to library");
      pItem3->SetThumbnailImage(pItem->GetThumbnailImage());
      spInt->m_browseAlbumVector.Add(pItem3);
    }

    musicdatabase->Close();
    delete musicdatabase;

    //the rest of the tracks
    for (int index=1; index < sp_albumbrowse_num_tracks(result); index++)
    {
      CFileItemPtr pItem2;
      pItem2 = spInt->spTrackToItem(sp_albumbrowse_track(result, index), ALBUMBROWSE_TRACK, true);
//      pItem2->SetContentType("audio/spotify");
      pItem2->SetThumbnailImage(newThumb);
      spInt->m_browseAlbumVector.Add(pItem2);
    }

    spInt->waitForThumbs();

    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    CStdString dir;
    dir.Format("%s",spInt->m_albumBrowseStr);
    message.SetStringParam(dir);
    g_windowManager.SendThreadMessage(message);
  }
  else
    CLog::Log( LOGERROR, "Spotifylog: browse failed!");
  spInt->hideProgressDialog();
}

void SpotifyInterface::cb_topListAritstsComplete(sp_toplistbrowse *result, void *userdata)
{
  SpotifyInterface *spInt = g_spotifyInterface;
  if (result && SP_ERROR_OK == sp_toplistbrowse_error(result))
  {
    spInt->m_progressDialog->SetPercentage(50);
    spInt->m_progressDialog->Progress();
    for (int index=0; index < sp_toplistbrowse_num_artists(result); index++)
    {
      CFileItemPtr pItem;
      pItem = spInt->spArtistToItem(sp_toplistbrowse_artist(result, index));
//      pItem->SetContentType("spotify toplist artist");
      spInt->m_browseToplistArtistsVector.Add(pItem);
    }

    //if the result is empty, add a note
    if (sp_toplistbrowse_num_artists(result) < 1)
    {
      CMediaSource share;
      share.strPath.Format("musicdb://spotify/artists/toplist/");
      share.strName.Format("No artists found");
      CFileItemPtr pItem(new CFileItem(share));
      //pItem->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
      spInt->m_browseToplistArtistsVector.Add(pItem);
    }

    spInt->m_progressDialog->SetPercentage(99);
    spInt->m_progressDialog->Progress();

    spInt->waitForThumbs();

    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    CStdString dir;
    dir.Format("musicdb://spotify/artists/toplist/");
    message.SetStringParam(dir);
    g_windowManager.SendThreadMessage(message);
  }
  else
    CLog::Log( LOGERROR, "Spotifylog: toplistartist failed!");
  spInt->hideProgressDialog();
}

void SpotifyInterface::cb_topListAlbumsComplete(sp_toplistbrowse *result, void *userdata)
{
  SpotifyInterface *spInt = g_spotifyInterface;
  if (result && SP_ERROR_OK == sp_toplistbrowse_error(result))
  {
    spInt->m_progressDialog->SetPercentage(50);
    spInt->m_progressDialog->Progress();

    CMusicDatabase *musicdatabase = new CMusicDatabase;
    musicdatabase->Open();
    int updateProgressWhen = sp_toplistbrowse_num_albums(result) / 10;
    int progress = 50;
    int progressCounter = 0;

    for (int index = 0; index < sp_toplistbrowse_num_albums(result); index++)
    {
      sp_album *spAlbum = sp_toplistbrowse_album(result, index);
      if ( sp_album_is_available(spAlbum))
      {
        sp_artist *spArtist = sp_album_artist(spAlbum);

        //is the album in our database?
        int albumId = musicdatabase->GetAlbumByName(sp_album_name(spAlbum), sp_artist_name(spArtist));
        if (albumId != -1)
        {
          CStdString path;
          CAlbum album;
          path.Format("musicdb://3/%ld/",albumId);
          musicdatabase->GetAlbumInfo(albumId, album, NULL);
          CStdString thumb ="";
          CFileItemPtr pItem(new CFileItem(path, album));
          musicdatabase->GetAlbumThumb(albumId, thumb);
          if (thumb != "NONE")
            pItem->SetThumbnailImage(thumb);
          else
            pItem->SetThumbnailImage("DefaultMusicAlbums.png");
          spInt->m_browseArtistAlbumVector.Add(pItem);
        }else
        {
          CFileItemPtr pItem;
          pItem = spInt->spAlbumToItem(spAlbum, TOPLIST_ALBUM);
//          pItem->SetContentType("audio/spotify");
          spInt->m_browseToplistAlbumVector.Add(pItem);
        }

        //set the progressbar
        if (progressCounter++ >= updateProgressWhen)
        {
          progressCounter = 0;
          progress +=5;
          spInt->m_progressDialog->SetPercentage(progress);
          spInt->m_progressDialog->Progress();
        }
      }
    }
    delete musicdatabase;

    //if the result is empty, add a note
    if (sp_toplistbrowse_num_albums(result) < 1)
    {
      CMediaSource share;
      share.strPath.Format("musicdb://spotify/albums/toplist/");
      share.strName.Format("No albums found");
      CFileItemPtr pItem(new CFileItem(share));
      //pItem->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
      spInt->m_browseToplistAlbumVector.Add(pItem);
    }

    spInt->m_progressDialog->SetPercentage(99);
    spInt->m_progressDialog->Progress();

    spInt->waitForThumbs();

    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    CStdString dir;
    dir.Format("musicdb://spotify/albums/toplist/");
    message.SetStringParam(dir);
    g_windowManager.SendThreadMessage(message);
  }
  else
    CLog::Log( LOGERROR, "Spotifylog: toplist album failed!");
  spInt->hideProgressDialog();
}

void SpotifyInterface::cb_topListTracksComplete(sp_toplistbrowse *result, void *userdata)
{
  SpotifyInterface *spInt = g_spotifyInterface;
  if (result && SP_ERROR_OK == sp_toplistbrowse_error(result))
  {
    spInt->m_progressDialog->SetPercentage(50);
    spInt->m_progressDialog->Progress();

    for (int index=0; index < sp_toplistbrowse_num_tracks(result); index++)
    {
      sp_track *spTrack = sp_toplistbrowse_track(result,index);
      if ( sp_track_is_available(g_spotifyInterface->m_session, spTrack))
      {
        CFileItemPtr pItem;
        pItem = spInt->spTrackToItem(spTrack,TOPLIST_TRACK,true);
//        pItem->SetContentType("audio/spotify");
        spInt->m_browseToplistTracksVector.Add(pItem);
      }
    }

    //if the result is empty, add a note
    if (sp_toplistbrowse_num_tracks(result) < 1)
    {
      CMediaSource share;
      share.strPath.Format("musicdb://spotify/tracks/toplist/");
      share.strName.Format("No albums found");
      CFileItemPtr pItem(new CFileItem(share));
      //pItem->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
      spInt->m_browseToplistTracksVector.Add(pItem);
    }

    spInt->m_progressDialog->SetPercentage(99);
    spInt->m_progressDialog->Progress();
    
    spInt->waitForThumbs();

    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    CStdString dir;
    dir.Format("musicdb://spotify/tracks/toplist/");
    message.SetStringParam(dir);
    g_windowManager.SendThreadMessage(message);
  }
  else
    CLog::Log( LOGERROR, "Spotifylog: toplist track failed!");
  spInt->hideProgressDialog();
}

void SpotifyInterface::cb_artistBrowseComplete(sp_artistbrowse *result, void *userdata)
{
  SpotifyInterface *spInt = g_spotifyInterface;
  if (result && SP_ERROR_OK == sp_artistbrowse_error(result))
  {
    spInt->m_progressDialog->SetPercentage(50);
    spInt->m_progressDialog->Progress();
    CLog::Log( LOGDEBUG, "Spotifylog: artistbrowse results are done!");
    //sp_album *tempalbum = 0;

    CMusicDatabase *musicdatabase = new CMusicDatabase;
    musicdatabase->Open();
    int updateProgressWhen = sp_artistbrowse_num_albums(result) / 10;
    int progress = 50;
    int progressCounter = 0;

    //if you are using spotifylib (not openspotifylib) 0.0.3, use the iterate over the tracks instead
    //  for (int index=0; index < sp_artistbrowse_num_tracks(result); index++)
    //{
    //sp_track *spTrack = sp_artistbrowse_track(result, index);
    //sp_album *spAlbum = sp_track_album(spTrack);
    //we want to populate the list with albums, but spotify returns tracks
    //if ( tempalbum != spAlbum && sp_album_is_available(spAlbum))
    for (int index = 0; index < sp_artistbrowse_num_albums(result); index++)
    {
      sp_album *spAlbum = sp_artistbrowse_album(result, index);
      if ( sp_album_is_available(spAlbum))
      {
        sp_artist *spArtist = sp_album_artist(spAlbum);

        //is the album in our database?
        int albumId = musicdatabase->GetAlbumByName(sp_album_name(spAlbum), sp_artist_name(spArtist));
        if (albumId != -1)
        {
          CStdString path;
          CAlbum album;
          path.Format("musicdb://3/%ld/",albumId);
          musicdatabase->GetAlbumInfo(albumId, album, NULL);
          CStdString thumb ="";
          CFileItemPtr pItem(new CFileItem(path, album));
          musicdatabase->GetAlbumThumb(albumId, thumb);
          if (thumb != "NONE")
            pItem->SetThumbnailImage(thumb);
          else
            pItem->SetThumbnailImage("DefaultMusicAlbums.png");
          spInt->m_browseArtistAlbumVector.Add(pItem);
        }else
        {
          CFileItemPtr pItem;
          pItem = spInt->spAlbumToItem(spAlbum, ARTISTBROWSE_ALBUM);
//          pItem->SetContentType("spotify artistbrowse album");
          spInt->m_browseArtistAlbumVector.Add(pItem);
        }

        //set the progressbar
        if (progressCounter++ >= updateProgressWhen)
        {
          progressCounter = 0;
          progress +=5;
          spInt->m_progressDialog->SetPercentage(progress);
          spInt->m_progressDialog->Progress();
        }
      }
    }
    delete musicdatabase;
    spInt->m_progressDialog->SetPercentage(99);
    spInt->m_progressDialog->Progress();

    //get the similar artists
    for (int index=0; index < sp_artistbrowse_num_similar_artists(result); index++)
    {
      CFileItemPtr pItem;
      pItem = spInt->spArtistToItem(sp_artistbrowse_similar_artist(result, index));
//      pItem->SetContentType("spotify artistbrowse similar artist");
      spInt->m_browseArtistSimilarArtistsVector.Add(pItem);
    }

    //menu
    CStdString thumb;
    thumb.Format("DefaultMusicArtists.png");
    sp_artist *spArtist = sp_artistbrowse_artist(result);
    //CStdString artistUri;
    //char spotify_artist_uri[256];
    //sp_link_as_string(sp_link_create_from_artist(sp_artistbrowse_artist(result)),spotify_artist_uri,256);
    //artistUri.Format("%s", spotify_artist_uri);

    CMediaSource share;
    CURL url(spInt->m_artistBrowseStr);
    CStdString uri = url.GetFileNameWithoutPath();

    //albums
    if (!spInt->m_browseArtistAlbumVector.IsEmpty())
    {
      share.strPath.Format("musicdb://spotify/albums/artistbrowse/%s/",uri.c_str());
      share.strName.Format("%s, %i albums",sp_artist_name(spArtist), spInt->m_browseArtistAlbumVector.Size());
    }else
    {
      share.strPath.Format("musicdb://spotify/menu/artisbrowse/%s/",spInt->m_artistBrowseStr);
      share.strName.Format("%s, No albums found",sp_artist_name(spArtist));
    }
    CFileItemPtr pItem3(new CFileItem(share));
    pItem3->SetThumbnailImage(thumb);
    spInt->m_browseArtistMenuVector.Add(pItem3);

    //similar artists
    if (!spInt->m_browseArtistSimilarArtistsVector.IsEmpty())
    {
      share.strPath.Format("musicdb://spotify/artists/artistbrowse/%s/",uri.c_str());
      share.strName.Format("%s, %i similar artists",sp_artist_name(spArtist), spInt->m_browseArtistSimilarArtistsVector.Size());
    }else
    {
      share.strPath.Format("musicdb://spotify/menu/artisbrowse/%s/",uri.c_str());
      share.strName.Format("No similar artists found",sp_artist_name(spArtist));
    }
    CFileItemPtr pItem4(new CFileItem(share));
    pItem4->SetThumbnailImage(thumb);
    spInt->m_browseArtistMenuVector.Add(pItem4);
    //get some portrait images

    if (sp_artistbrowse_num_portraits(result) > 0)
    {
      //    spInt->requestThumb((unsigned char*)sp_artistbrowse_portrait(result,0),artistUri,pItem3, ARTISTBROWSE_ARTIST);
      //    spInt->requestThumb((unsigned char*)sp_artistbrowse_portrait(result,0),artistUri,pItem4, ARTISTBROWSE_ARTIST);
    }

    spInt->waitForThumbs();

    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    CStdString dir;
    dir.Format("%s",spInt->m_artistBrowseStr);
    message.SetStringParam(dir);
    g_windowManager.SendThreadMessage(message);
  }
  else
    CLog::Log( LOGERROR, "Spotifylog: artistbrowse failed!");
  spInt->hideProgressDialog();
}

void SpotifyInterface::cb_searchComplete(sp_search *search, void *userdata)
{
  SpotifyInterface *spInt = g_spotifyInterface;
  if (search && SP_ERROR_OK == sp_search_error(search))
  {
    CLog::Log( LOGNOTICE, "Spotifylog: search results are done!");

    //did you misspell?
    CStdString newSearch;
    newSearch.Format("%s", sp_search_did_you_mean(search));
    if (newSearch !="")
    {
      spInt->hideProgressDialog();
      CStdString message;
      message.Format("Did you mean %s?",newSearch.c_str());
      CGUIDialogYesNo* m_yesNoDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      m_yesNoDialog->SetHeading("Spotify");
      m_yesNoDialog->SetLine(0, "" );
      m_yesNoDialog->SetLine(1, message );
      m_yesNoDialog->SetLine(2, "" );
      m_yesNoDialog->DoModal();
      if (m_yesNoDialog->IsConfirmed())
      {
        spInt->search(newSearch);
        return;
      }
      else
      {
        CStdString message;
        message.Format("Searching for %s", spInt->m_searchStr);
        spInt->showProgressDialog(message);
        spInt->m_progressDialog->SetPercentage(50);
        spInt->m_progressDialog->Progress();
      }
    }

    spInt->m_progressDialog->SetPercentage(50);
    spInt->m_progressDialog->Progress();

    //get the artists
    for (int index=0; index < sp_search_num_artists(search); index++)
    {
      CFileItemPtr pItem;
      pItem = spInt->spArtistToItem(sp_search_artist(search, index));
//      pItem->SetContentType("spotify search artist");
      spInt->m_searchArtistVector.Add(pItem);
    }
    spInt->m_progressDialog->SetPercentage(60);
    spInt->m_progressDialog->Progress();

    //albums
    CMusicDatabase *musicdatabase = new CMusicDatabase;
    musicdatabase->Open();
    for (int index=0; index < sp_search_num_albums(search); index++)
    {
      sp_album *spAlbum = sp_search_album(search,index);
      if ( sp_album_is_available(spAlbum))
      {
        sp_artist *spArtist = sp_album_artist(spAlbum);
        //is the album in our database?
        int albumId = musicdatabase->GetAlbumByName(sp_album_name(spAlbum), sp_artist_name(spArtist));
        if (albumId != -1)
        {
          CStdString path;
          CAlbum album;
          path.Format("musicdb://3/%ld/",albumId);
          musicdatabase->GetAlbumInfo(albumId, album, NULL);
          CStdString thumb ="";
          CFileItemPtr pItem(new CFileItem(path, album));
          musicdatabase->GetAlbumThumb(albumId, thumb);
          if (thumb != "NONE")
            pItem->SetThumbnailImage(thumb);
          else
            pItem->SetThumbnailImage("DefaultMusicAlbums.png");
          spInt->m_searchAlbumVector.Add(pItem);
        }else
        {
          CFileItemPtr pItem;
          pItem = spInt->spAlbumToItem(spAlbum, SEARCH_ALBUM);
//          pItem->SetContentType("spotify search album");
         
          spInt->m_searchAlbumVector.Add(pItem);
        }
      }
    }
    delete musicdatabase;

    spInt->m_progressDialog->SetPercentage(80);
    spInt->m_progressDialog->Progress();

    //tracks
    for (int index=0; index < sp_search_num_tracks(search); index++)
    {
      sp_track *spTrack = sp_search_track(search,index);
      if ( sp_track_is_available(g_spotifyInterface->m_session, spTrack))
      {
        CFileItemPtr pItem;
        pItem = spInt->spTrackToItem(spTrack,SEARCH_TRACK,true);
//        pItem->SetContentType("audio/spotify");
        spInt->m_searchTrackVector.Add(pItem);
      }
    }

    spInt->m_progressDialog->SetPercentage(99);
    spInt->m_progressDialog->Progress();
    spInt->m_isSearching = false;

    spInt->waitForThumbs();

    CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
    message.SetStringParam("musicdb://spotify/menu/search/");
    g_windowManager.SendThreadMessage(message);

  }else
    CLog::Log( LOGERROR, "Spotifylog: search failed!");
  spInt->hideProgressDialog();
}

SpotifyInterface::SpotifyInterface()
{
  m_session = 0;
  m_nextEvent = CTimeUtils::GetTimeMS();
  m_showDisclaimer = true;
  m_isShowingReconnect = false;
  m_searchStr = "";
  m_search = 0;
  m_artistBrowse = 0;
  m_albumBrowse = 0;
  m_toplistArtistsBrowse = 0;
  m_toplistAlbumsBrowse = 0;
  m_toplistTracksBrowse = 0;
  m_isSearching = false;
  m_noWaitingThumbs = 0;
  m_isWaitingForLogout = false;

  m_callbacks.connection_error = &cb_connectionError;
  m_callbacks.logged_out = &cb_loggedOut;
  m_callbacks.message_to_user = 0;
  m_callbacks.logged_in = &cb_loggedIn;
  m_callbacks.notify_main_thread = &cb_notifyMainThread;
  m_callbacks.music_delivery = &SpotifyCodec::cb_musicDelivery;
  m_callbacks.metadata_updated = 0;
  m_callbacks.play_token_lost = 0;
  m_callbacks.log_message = &cb_logMessage;
  m_callbacks.end_of_track = &SpotifyCodec::cb_endOfTrack;


  m_thumbDir.Format("special://temp/spotify/thumbs/");
  m_playlistsThumbDir.Format("special://temp/spotify/playlistthumbs/");
  m_toplistsThumbDir.Format("special://temp/spotify/toplistthumbs/");
  m_currentPlayingDir.Format("special://temp/spotify/currentplayingthumbs/");

  //create a temp dir for the thumbnails
  CDirectory::Create("special://temp/spotify/");
  clean();
}

SpotifyInterface::~SpotifyInterface()
{
  clean();
  disconnect();

}

bool SpotifyInterface::connect(bool forceNewUser)
{
  //clean();
  //do we need to create a new session
  if (!m_session)
  {
    CLog::Log( LOGDEBUG, "Spotifylog: creating session \n");
    // Setup for waking up the main thread in notify_main_thread()
    //m_main_thread = pthread_self();
    // Always do this. It allows libspotify to check for
    // header/library inconsistencies.
    m_config.api_version = SPOTIFY_API_VERSION;
    // The path of the directory to store the cache. This must be specified.
    // Please read the documentation on preferred values.
    m_config.cache_location = g_advancedSettings.m_spotifyCacheFolder;
    // The path of the directory to store the settings. This must be specified.
    // Please read the documentation on preferred values.
    m_config.settings_location = g_advancedSettings.m_spotifyCacheFolder;
    // The key of the application. They are generated by Spotify,
    // and are specific to each application using libspotify.

    extern const char g_appkey[];
    /// The size of the application key.
    extern const size_t g_appkey_size;

    m_config.application_key = g_appkey;
    m_config.application_key_size = g_appkey_size;
    // This identifies the application using some
    // free-text string [1, 255] characters.
    m_config.user_agent = "spotify-for-XBMC";
    // Register the callbacks.
    m_config.callbacks = &m_callbacks;

    m_error = sp_session_create(&m_config, &m_session);
    //set high bitrate
    if (g_advancedSettings.m_spotifyUseHighBitrate)
      sp_session_preferred_bitrate(m_session, SP_BITRATE_320k);
    if (SP_ERROR_OK != m_error) {
      CLog::Log( LOGERROR, "Spotifylog: failed to create session: error: %s", sp_error_message(m_error));
      m_session = NULL;
      return false;
    }
  }

  if (forceNewUser)
  {
    disconnect();
    g_advancedSettings.m_spotifyUsername = "";
    g_advancedSettings.m_spotifyPassword = "";
  }
  //are we logged in?
  if (sp_session_connectionstate(m_session) != SP_CONNECTION_STATE_LOGGED_IN || forceNewUser)
  {
    CLog::Log( LOGNOTICE, "Spotifylog: logging in \n");
    CStdString username = getUsername();
    CStdString password = getPassword();
    sp_session_login(m_session, username.c_str(), password.c_str() );
    if (sp_session_user(m_session) == 0 ) {
      CLog::Log( LOGERROR, "Spotifylog: failed to login %s\n", sp_error_message(m_error));
      return false;
    }
  }
  return true;
}

bool SpotifyInterface::disconnect()
{
  CLog::Log( LOGNOTICE, "Spotifylog: disconnecting to Spotify!");
  sp_session_logout( m_session);
  //session_terminated();
  if (SP_ERROR_OK != m_error) {
    CLog::Log( LOGERROR, "Spotifylog: failed to logout %s\n", sp_error_message(m_error));
    return false;
  }
  m_isWaitingForLogout = true;
  int i = 100;
  while(m_isWaitingForLogout && i > 0){
    clock_t goal = 100 + clock();
    while (goal > clock());
    processEvents();
    i--;
  }
  return true;
}

bool SpotifyInterface::reconnect(bool forceNewUser)
{
  if(m_session)
  {
    if (forceNewUser)
      sp_session_logout(m_session);
    if (forceNewUser || sp_session_connectionstate(m_session) != SP_CONNECTION_STATE_LOGGED_IN )
    {
      showReconectingDialog();
      connect(forceNewUser);
      m_nextEvent = CTimeUtils::GetTimeMS();
      processEvents();
      return false;
    }
    return true;
  }
  return false;
}

void SpotifyInterface::clean()
{
  clean(true,true,true,true,true,true,true,true,true);
}

void SpotifyInterface::clean(bool search, bool artistbrowse, bool albumbrowse, bool playlists, bool toplists, bool searchthumbs, bool playliststhumbs, bool toplistthumbs, bool currentplayingthumbs)
{
  CLog::Log(LOGNOTICE, "Spotifylog: clean");
  if (search)
  {
    //stop the thumb downloading and release the images
    while (!m_searchWaitingThumbs.empty())
    {
      imageItemPair pair = m_searchWaitingThumbs.back();
      CFileItemPtr pItem = pair.second;
      //sp_image_remove_load_callback(pair.first,&cb_imageLoaded, pItem.get());
      if (pair.first)
        sp_image_release(pair.first);
      m_searchWaitingThumbs.pop_back();
    }

    if (m_search)
      sp_search_release(m_search);
    m_search = 0;

    //clear the result vectors
    m_searchArtistVector.Clear();
    m_searchAlbumVector.Clear();
    m_searchTrackVector.Clear();
  }

  if (artistbrowse)
  {
    //stop the thumb downloading and release the images
    while (!m_artistWaitingThumbs.empty())
    {
      imageItemPair pair = m_artistWaitingThumbs.back();
      CFileItemPtr pItem = pair.second;
      //sp_image_remove_load_callback(pair.first,&cb_imageLoaded, pItem.get());
      sp_image_release(pair.first);
      m_artistWaitingThumbs.pop_back();
    }

    if (m_artistBrowse)
      sp_artistbrowse_release(m_artistBrowse);
    m_artistBrowse = 0;
    m_artistBrowseStr = "";
    m_browseArtistMenuVector.Clear();
    m_browseArtistAlbumVector.Clear();
    m_browseArtistSimilarArtistsVector.Clear();
  }

  if (albumbrowse)
  {
    if (m_albumBrowse)
      sp_albumbrowse_release(m_albumBrowse);
    m_albumBrowse = 0;

    m_albumBrowseStr = "";
    m_browseAlbumVector.Clear();
  }

  if (playlists)
  {
    //stop the thumb downloading and release the images
    while (!m_playlistWaitingThumbs.empty())
    {
      imageItemPair pair = m_playlistWaitingThumbs.back();
      CFileItemPtr pItem = pair.second;
      //sp_image_remove_load_callback(pair.first,&cb_imageLoaded, pItem.get());
      sp_image_release(pair.first);
      m_playlistWaitingThumbs.pop_back();
    }

    m_playlistItems.Clear();
  }

  if (toplists)
  {
    //stop the thumb downloading and release the images
    while (!m_toplistWaitingThumbs.empty())
    {
      imageItemPair pair = m_toplistWaitingThumbs.back();
      CFileItemPtr pItem = pair.second;
      //sp_image_remove_load_callback(pair.first,&cb_imageLoaded, pItem.get());
      sp_image_release(pair.first);
      m_toplistWaitingThumbs.pop_back();
    }

    if (m_toplistArtistsBrowse)
      sp_toplistbrowse_release(m_toplistArtistsBrowse);
    m_toplistArtistsBrowse = 0;

    if (m_toplistAlbumsBrowse)
      sp_toplistbrowse_release(m_toplistAlbumsBrowse);
    m_toplistAlbumsBrowse = 0;

    if (m_toplistTracksBrowse)
      sp_toplistbrowse_release(m_toplistTracksBrowse);
    m_toplistTracksBrowse = 0;
    m_browseToplistArtistsVector.Clear();
    m_browseToplistAlbumVector.Clear();
    m_browseToplistTracksVector.Clear();
  }

  if (searchthumbs)
  {
    CDirectory::Remove(m_thumbDir);
    CDirectory::Create(m_thumbDir);
  }

  if (playliststhumbs)
  {
    CDirectory::Remove(m_playlistsThumbDir);
    CDirectory::Create(m_playlistsThumbDir);
  }

  if (toplistthumbs)
  {
    CDirectory::Remove(m_toplistsThumbDir);
    CDirectory::Create(m_toplistsThumbDir);
  }

  if (currentplayingthumbs)
  {
    CDirectory::Remove(m_currentPlayingDir);
    CDirectory::Create(m_currentPlayingDir);
  }
}



bool SpotifyInterface::getDirectory(const CStdString &strPath, CFileItemList &items)
{
  CLog::Log(LOGNOTICE, "Spotifylog: getDirectory: %s", strPath.c_str());
  if (strPath.Left(28) == "musicdb://spotify/menu/main/")
  {
    getMainMenuItems(items);
    return true;
  }

  if (strPath.Left(32) == "musicdb://spotify/menu/settings/")
  {
    getSettingsMenuItems(items);
    return true;
  }

  if (strPath.Left(30) == "musicdb://spotify/menu/search/")
  {
    if (!reconnect() || m_isSearching)
    {
      return true;
    }
    else if (!hasSearchResults())
    {
      return search();
    }
    else
    {
      getSearchMenuItems(items);
      return true;
    }
  }

  if (strPath.Left(32) == "musicdb://spotify/menu/toplists/")
  {
    getBrowseToplistMenu(items);
    return true;
  }

  if (strPath.Left(33) == "musicdb://spotify/menu/playlists/")
  {
    if (!reconnect())
    {
      return true;
    }
    getPlaylistItems(items);
    return true;
  }

  if (strPath.Left(36) == "musicdb://spotify/menu/artistbrowse/")
  {
    if (!reconnect())
    {
      return true;
    }
    return getBrowseArtistMenu(strPath.c_str(), items);
  }

  if (strPath.Left(36) == "musicdb://spotify/command/newsearch/")
  {
    if (!reconnect())
    {
      return false;
    }
    search();
    return false;
  }

  if (strPath.Left(36) == "musicdb://spotify/command/reconnect/")
  {
    reconnect(true);
    return false;
  }

  if (strPath.Left(34) == "musicdb://spotify/command/connect/")
  {
    reconnect();
    return false;
  }

  if (strPath.Left(37) == "musicdb://spotify/command/disconnect/")
  {
    disconnect();
    return false;
  }

  if (strPath.Left(33) == "musicdb://spotify/artists/search/")
  {
    if (!reconnect())
    {
      return true;
    }
    items.Append(m_searchArtistVector);
    return true;
  }

  if (strPath.Left(35) == "musicdb://spotify/command/addalbum/")
  {
    addAlbumToLibrary();
    clean(false,false,true,false,false,false,false,false,false);
    return false;
  }

  if (strPath.Left(39) == "musicdb://spotify/artists/artistbrowse/")
  {
    if (!reconnect())
    {
      return true;
    }
    return getBrowseArtistArtists(strPath.c_str(), items);
  }

  if (strPath.Left(34) == "musicdb://spotify/artists/toplist/")
  {
    return getBrowseToplistArtists(items);
  }

  if (strPath.Left(32) == "musicdb://spotify/albums/search/")
  {
    if (!reconnect())
    {
      return true;
    }
    items.Append(m_searchAlbumVector);
    return true;
  }

  if (strPath.Left(38) == "musicdb://spotify/albums/artistbrowse/")
  {
    if (!reconnect())
    {
      return true;
    }
    return getBrowseArtistAlbums(strPath.c_str(), items);
  }

  if (strPath.Left(33) == "musicdb://spotify/albums/toplist/")
  {
    if (!reconnect())
    {
      return true;
    }
    return getBrowseToplistAlbums(items);
  }

  if (strPath.Left(32) == "musicdb://spotify/tracks/search/")
  {
    if (!reconnect())
    {
      return true;
    }
    items.Append(m_searchTrackVector);
    return true;
  }

  if (strPath.Left(33) == "musicdb://spotify/tracks/toplist/")
  {
    if (!reconnect())
    {
      return true;
    }
    return getBrowseToplistTracks(items);
  }

  if (strPath.Left(34) == "musicdb://spotify/tracks/playlist/")
  {
    if (!reconnect())
    {
      return true;
    }
    CStdString playListNr = strPath;
    playListNr.Delete(0, 34);
    URIUtils::RemoveSlashAtEnd(playListNr);
    getPlaylistTracks(items,atoi(playListNr.c_str()));
    if (items.IsEmpty())
      return false;
    return true;
  }
  if (strPath.Left(37) == "musicdb://spotify/tracks/albumbrowse/")
  {
    if (!reconnect())
    {
      return true;
    }
    return getBrowseAlbumTracks(strPath,items);
  }
  return false;

}

XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE SpotifyInterface::getChildType(const CStdString &strPath)
{
  if (strPath.Left(25) == "musicdb://spotify/tracks/")
  {
    return XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_SONG;
  }

  if (strPath.Left(26) == "musicdb://spotify/artists/")
  {
    return XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_ARTIST;
  }

  if (strPath.Left(25) == "musicdb://spotify/albums/")
  {
    return XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM;
  }

  return XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_NONE;
}

void SpotifyInterface::getMainMenuItems(CFileItemList &items)
{
  //search
  CMediaSource share;
  share.strPath.Format("musicdb://spotify/menu/search/");
  share.strName.Format("Search");
  CFileItemPtr pItem(new CFileItem(share));
  //pItem->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem);

  //playlists
  share.strPath.Format("musicdb://spotify/menu/playlists/");
  share.strName.Format("Playlists");
  CFileItemPtr pItem2(new CFileItem(share));
  //pItem2->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem2);

  //toplists
  share.strPath.Format("musicdb://spotify/menu/toplists/");
  share.strName.Format("Top lists");
  CFileItemPtr pItem3(new CFileItem(share));
  //pItem3->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem3);

  //settings
  share.strPath.Format("musicdb://spotify/menu/settings/");
  share.strName.Format("Settings");
  CFileItemPtr pItem4(new CFileItem(share));
  //pItem4->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem4);
}

void SpotifyInterface::getBrowseToplistMenu(CFileItemList &items)
{
  //Artists
  CMediaSource share;
  share.strPath.Format("musicdb://spotify/artists/toplist/");
  share.strName.Format("Top artists");
  CFileItemPtr pItem(new CFileItem(share));
  //pItem->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem);

  //Albums
  share.strPath.Format("musicdb://spotify/albums/toplist/");
  share.strName.Format("Top albums");
  CFileItemPtr pItem2(new CFileItem(share));
  //pItem2->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem2);

  //tracks
  share.strPath.Format("musicdb://spotify/tracks/toplist/");
  share.strName.Format("Top tracks");
  CFileItemPtr pItem3(new CFileItem(share));
  //pItem3->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem3);
}

void SpotifyInterface::getSettingsMenuItems(CFileItemList &items)
{
  CMediaSource share;
  bool connected = false;
  connected = m_session;
  if (connected)
    connected = sp_session_connectionstate(m_session) == SP_CONNECTION_STATE_LOGGED_IN;

  if(connected)
  {
    //disconnect
    share.strPath.Format("musicdb://spotify/command/disconnect/");
    share.strName.Format("Logged in as %s, logout", sp_user_display_name(sp_session_user(m_session)));
  }else
  {
    //connect
    share.strPath.Format("musicdb://spotify/command/connect/");
    share.strName.Format("Connect as %s", g_advancedSettings.m_spotifyUsername);
  }

  CFileItemPtr pItem4(new CFileItem(share));
  //pItem4->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem4);

  //reconnect
  share.strPath.Format("musicdb://spotify/command/reconnect/");
  share.strName.Format("Connect as a different user");
  CFileItemPtr pItem5(new CFileItem(share));
  //pItem5->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem5);

  //disclaimer
  //share.strPath.Format("spotify://disclaimer");
  //share.strName.Format("Show Disclaimer");
  //CFileItemPtr pItem6(new CFileItem(share));
  //pItem6->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  //items.Add(pItem6);
}

void SpotifyInterface::getSearchMenuItems(CFileItemList &items)
{
  CMediaSource share;
  CStdString thumb;
  CStdString query = sp_search_query(m_search);
  //artists
  if (!m_searchArtistVector.IsEmpty())
  {
    share.strPath.Format("musicdb://spotify/artists/search/");
    share.strName.Format("%s, %i artists",query.c_str(), m_searchArtistVector.Size());
  }else
  {
    share.strPath.Format("musicdb://spotify/menu/search/");
    share.strName.Format("%s, no artists found", query.c_str());
  }
  CFileItemPtr pItem3(new CFileItem(share));
  pItem3->SetThumbnailImage("DefaultMusicArtists.png");
  items.Add(pItem3);

  //albums
  if (!m_searchAlbumVector.IsEmpty())
  {
    share.strPath.Format("musicdb://spotify/albums/search/");
    share.strName.Format("%s, %i albums",query.c_str(), m_searchAlbumVector.Size());
  }else
  {
    share.strPath.Format("musicdb://spotify/menu/search/");
    share.strName.Format("%s, no albums found", query.c_str());
  }
  CFileItemPtr pItem4(new CFileItem(share));

  //what thumb should we have?
  pItem4->SetThumbnailImage("DefaultMusicAlbums.png");
  /*if (!m_searchAlbumVector.IsEmpty())
  {
    //request a thumbnail image
    CURL url = m_searchAlbumVector[0]->GetAsUrl();
    CStdString Uri = url.GetFileNameWithoutPath();
    CUtil::RemoveExtension(Uri);
    sp_album *spAlbum = sp_link_as_album(sp_link_create_from_string(Uri));
    url = m_searchAlbumVector[0]->GetAsUrl();
    CLog::Log( LOGDEBUG, "Spotifylog: searchmenu thumb:%s", Uri.c_str());
    requestThumb((unsigned char*)sp_album_cover(spAlbum),Uri, pItem4, SEARCH_ALBUM);
  }*/
  items.Add(pItem4);
  //tracks
  if (!m_searchTrackVector.IsEmpty())
  {
    share.strPath.Format("musicdb://spotify/tracks/search/");
    share.strName.Format("%s, %i tracks",query.c_str(), m_searchTrackVector.Size());
  }else
  {
    share.strPath.Format("musicdb://spotify/menu/search/");
    share.strName.Format("%s, no tracks found", query.c_str());
  }
  CFileItemPtr pItem5(new CFileItem(share));

  //what thumb should we have?
  pItem5->SetThumbnailImage("DefaultMusicSongs.png");
  /*if (!m_searchTrackVector.IsEmpty())
  {
    //request a thumbnail image
    CURL url = m_searchTrackVector[0]->GetAsUrl();
    CStdString Uri = url.GetFileNameWithoutPath();
    CUtil::RemoveExtension(Uri);

    sp_track *spTrack = sp_link_as_track(sp_link_create_from_string(Uri));
    if (spTrack)
    {
    sp_album *spAlbum = sp_track_album(spTrack);
    if (spAlbum)
      requestThumb((unsigned char*)sp_album_cover(spAlbum),Uri, pItem5, SEARCH_ALBUM);
    }
  }*/
  items.Add(pItem5);
  //new search
  share.strPath.Format("musicdb://spotify/command/newsearch/");
  share.strName.Format("New search");
  CFileItemPtr pItem6(new CFileItem(share));
  //pItem6->SetThumbnailImage(CUtil::GetDefaultFolderThumb("special://xbmc/media/spotify_core_logo.png"));
  items.Add(pItem6);
}

bool SpotifyInterface::getBrowseArtistMenu(CStdString strPath, CFileItemList &items)
{
  if (reconnect())
  {
    //do we have this artist loaded allready?
    CURL url(strPath);
    CStdString newUri = url.GetFileNameWithoutPath();

    
    CURL oldUrl(m_artistBrowseStr);
    CStdString uri = oldUrl.GetFileNameWithoutPath();
    
    CLog::Log(LOGNOTICE, "Spotifylog: get artist menu new: %s old: %s", newUri.c_str(), uri.c_str());

    if (newUri == uri)
    {
      items.Append(m_browseArtistMenuVector);
      return true;
    }
    else
    {
      return browseArtist(strPath);
    }
  }
  return false;
}

void SpotifyInterface::getPlaylistItems(CFileItemList &items)
{
  //get the playlists
  CMediaSource share;
  sp_playlistcontainer *pc = sp_session_playlistcontainer(m_session);
  for (int i=0; i < sp_playlistcontainer_num_playlists(pc); i++)
  {
    sp_playlist* pl = sp_playlistcontainer_playlist(pc, i);
    if (sp_playlist_is_loaded(pl))
    {
      share.strPath.Format("musicdb://spotify/tracks/playlist/%ld/", i);
      share.strName.Format("%s",sp_playlist_name(pl));
    }else
    {
      share.strPath.Format("musicdb://spotify/menu/playlists/");
      share.strName.Format("Loading playlist...");
    }
    CFileItemPtr pItem(new CFileItem(share));
    CStdString thumb = "DefaultPlaylist.png";
    //ask for a thumbnail
    pItem->SetThumbnailImage("DefaultMusicPlaylists.png");
    if (sp_playlist_is_loaded(pl) && sp_playlist_num_tracks(pl) > 0)
    {
      sp_track *spTrack = sp_playlist_track(pl,0);
      sp_album *spAlbum = sp_track_album(spTrack);
      sp_link *spLink  = sp_link_create_from_album(spAlbum);
      CStdString Uri = "";
      char spotify_uri[256];
      sp_link_as_string (spLink,spotify_uri,256);
      sp_link_release(spLink);
      Uri.Format("%s", spotify_uri);
      CLog::Log( LOGDEBUG, "Spotifylog: searchmenu thumb:%s", Uri.c_str());
      requestThumb((unsigned char*)sp_album_cover(spAlbum),Uri, pItem, PLAYLIST_TRACK);
    }
    items.Add(pItem);
  }
}

bool SpotifyInterface::search()
{
  CStdString searchString = "";
  while (searchString.IsEmpty())
    CGUIDialogKeyboard::ShowAndGetInput(searchString,"Spotify search",false);
  return search(searchString);
}

bool SpotifyInterface::search(CStdString searchstring)
{
  m_searchStr = searchstring;
  CLog::Log(LOGDEBUG, "Spotifylog: search");
  clean(true,true,true,false,false,true,false,false,false);
  m_search = sp_search_create(m_session, searchstring, 0, g_advancedSettings.m_spotifyMaxSearchTracks, 0, g_advancedSettings.m_spotifyMaxSearchAlbums, 0, g_advancedSettings.m_spotifyMaxSearchArtists, &cb_searchComplete, NULL);
  CStdString message;
  message.Format("Searching for %s", searchstring.c_str());
  showProgressDialog(message);
  m_isSearching = true;
  return true;
}

bool SpotifyInterface::getBrowseArtistAlbums(CStdString strPath, CFileItemList &items)
{
  if (reconnect())
  {
    //do we have this artist loaded allready?
    CURL url(strPath);
    CStdString newUri = url.GetFileNameWithoutPath();

    CURL oldUrl(m_artistBrowseStr);
    CStdString uri = oldUrl.GetFileNameWithoutPath();

    if (newUri == uri)
    {
      items.Append(m_browseArtistAlbumVector);
      return true;
    }
    else
    {
      return browseArtist(strPath);
    }
  }
  return false;
}

bool SpotifyInterface::getBrowseArtistArtists(CStdString strPath, CFileItemList &items)
{
  if (reconnect())
  {
    //do we have this artist loaded allready?
    CURL url(strPath);
    CStdString newUri = url.GetFileNameWithoutPath();

    CURL oldUrl(m_artistBrowseStr);
    CStdString uri = oldUrl.GetFileNameWithoutPath();

    if (newUri == uri)
    {
      items.Append(m_browseArtistSimilarArtistsVector);
      return true;
    }
    else
    {
      return browseArtist(strPath);
    }
  }
  return false;
}

bool SpotifyInterface::browseArtist(CStdString strPath)
{
  if (reconnect())
  {
    CURL url(strPath);
    CStdString uri = url.GetFileNameWithoutPath();
    CLog::Log( LOGNOTICE, "Spotifylog: browsing artist %s", uri.c_str());
    sp_link *spLink = sp_link_create_from_string(uri.c_str());
    sp_artist * spArtist = sp_link_as_artist(spLink);
    if (spArtist)
    {
      clean(false,true,false,false,false,false,false,false,false);
      CStdString message;
      message.Format("Browsing albums from %s", sp_artist_name(spArtist));
      showProgressDialog(message);
      CLog::Log( LOGDEBUG, "Spotifylog: browsing artist %s", sp_artist_name(spArtist));
      m_artistBrowse = sp_artistbrowse_create(m_session, spArtist, &cb_artistBrowseComplete, 0);
      m_artistBrowseStr = strPath;
      sp_link_release(spLink);
      return true;
    }
    sp_link_release(spLink);
  }
  return false;
}

bool SpotifyInterface::getBrowseAlbumTracks(CStdString strPath, CFileItemList &items)
{
  if (reconnect())
  {
    //do we have this album loaded allready?
    CURL url(strPath);
    CStdString newUri = url.GetFileNameWithoutPath();

    CURL oldUrl(m_albumBrowseStr);
    CStdString uri = oldUrl.GetFileNameWithoutPath();

    if (newUri == uri)
    {
      items.Append(m_browseAlbumVector);
      return true;
    }
    else
    {
      sp_link *spLink = sp_link_create_from_string(newUri.c_str());
      sp_album *spAlbum = sp_link_as_album (spLink);
      if (spAlbum)
      {
        clean(false,false,true,false,false,false,false,false,false);
        CStdString message;
        message.Format("Browsing tracks from %s", sp_album_name(spAlbum));
        showProgressDialog(message);
        CLog::Log( LOGDEBUG, "Spotifylog: browsing album");
        m_albumBrowse = sp_albumbrowse_create(m_session, spAlbum, &cb_albumBrowseComplete, spAlbum);
        m_albumBrowseStr = strPath;
        sp_link_release(spLink);
        return true;
      }
    }
  }
  return false;
}

//toplists
bool SpotifyInterface::getBrowseToplistArtists(CFileItemList &items)
{
  if (reconnect())
  {
    if (!m_browseToplistArtistsVector.IsEmpty())
    {
      items.Append(m_browseToplistArtistsVector);
      return true;
    }
    else
    {
      CStdString message;
      message.Format("Browsing top artists");
      showProgressDialog(message);
      m_toplistArtistsBrowse = 
        sp_toplistbrowse_create(m_session, 
                                SP_TOPLIST_TYPE_ARTISTS,
                                SP_TOPLIST_REGION_EVERYWHERE,
                                getUsername(),
                                &cb_topListAritstsComplete,
                                0);
      return true;
    }
  }
  return false;
}

bool SpotifyInterface::getBrowseToplistAlbums(CFileItemList &items)
{
  if (reconnect())
  {
    if (!m_browseToplistAlbumVector.IsEmpty())
    {
      items.Append(m_browseToplistAlbumVector);
      return true;
    }
    else
    {
      CStdString message;
      message.Format("Browsing top albums");
      showProgressDialog(message);
      m_toplistAlbumsBrowse = 
        sp_toplistbrowse_create(m_session,
                                SP_TOPLIST_TYPE_ALBUMS,
                                SP_TOPLIST_REGION_EVERYWHERE,
                                getUsername(),
                                &cb_topListAlbumsComplete,
                                0);
      return true;
    }
  }
  return false;
}

bool SpotifyInterface::getBrowseToplistTracks(CFileItemList &items)
{
  if (reconnect())
  {
    if (!m_browseToplistTracksVector.IsEmpty())
    {
      items.Append(m_browseToplistTracksVector);
      return true;
    }
    else
    {
      CStdString message;
      message.Format("Browsing top tracks");
      showProgressDialog(message);
      m_toplistTracksBrowse =
      sp_toplistbrowse_create(m_session,
                              SP_TOPLIST_TYPE_TRACKS,
                              SP_TOPLIST_REGION_EVERYWHERE,
                              getUsername(),
                              &cb_topListTracksComplete,
                              0);
      return true;
    }
  }
  return false;
}

//converting functions
CFileItemPtr SpotifyInterface::spArtistToItem(sp_artist *spArtist)
{
  //path with artist Uri
  CStdString path;
  char spotify_uri[256];
  sp_link_as_string(sp_link_create_from_artist(spArtist),spotify_uri,256);
  CStdString Uri = "";
  Uri.Format("%s", spotify_uri);

  path.Format("musicdb://spotify/menu/artistbrowse/%s", Uri);

  //why the hell is it a CAlbum instead of CArtist?
  //we dont want it to load albums from the database and you cant provide an artist item eith a custom path
  CAlbum album;
  album.strArtist = sp_artist_name(spArtist);

  CFileItemPtr pItem(new CFileItem(path, album));
  return pItem;
}

CFileItemPtr SpotifyInterface::spAlbumToItem(sp_album *spAlbum, SPOTIFY_TYPE type)
{
  sp_artist *albumArtist = sp_album_artist(spAlbum);
  //path with album Uri
  CStdString path;
  CStdString Uri = "";
  char spotify_uri[256];
  sp_link_as_string(sp_link_create_from_album(spAlbum),spotify_uri,256);
  Uri.Format("%s", spotify_uri);

  path.Format("musicdb://spotify/tracks/albumbrowse/%s", Uri.c_str());

  CAlbum album;
  album.strArtist = sp_artist_name(albumArtist);
  album.strAlbum = sp_album_name(spAlbum);
  album.iYear = sp_album_year(spAlbum);

  CFileItemPtr pItem(new CFileItem(path, album));
  pItem->SetThumbnailImage("DefaultMusicAlbums.png");
  requestThumb((unsigned char*)sp_album_cover(spAlbum),Uri,pItem, type);
  return pItem;
}

CFileItemPtr SpotifyInterface::spTrackToItem(sp_track *spTrack, SPOTIFY_TYPE type, bool loadthumb)
{
  sp_album *spAlbum = sp_track_album(spTrack);
  sp_artist *spArtist = sp_track_artist(spTrack, 0);
  sp_artist *albumArtist = sp_album_artist(spAlbum);
  CStdString path;

  CStdString Uri;
  char spotify_uri[256];
  sp_link_as_string(sp_link_create_from_track(spTrack, 0),spotify_uri,256);
  Uri.Format("%s", spotify_uri);
  path.Format("%s.spotify", Uri);

   CSong song;
   song.strTitle = sp_track_name(spTrack);
   if (!sp_track_is_available(m_session, spTrack))
 {
   song.strTitle.Format("NOT PLAYABLE, %s", sp_track_name(spTrack));
   path.Format("unplayable%s.unplayable", Uri);
 }
  song.strFileName = path.c_str();

  song.iDuration = 0.001 * sp_track_duration(spTrack);
  song.iTrack = sp_track_index(spTrack);

  song.strAlbum = sp_album_name(spAlbum);
  song.strAlbumArtist = sp_artist_name(albumArtist);
  song.strArtist = sp_artist_name(spArtist);

  CFileItemPtr pItem(new CFileItem(song));
  if (loadthumb)
  {
    CStdString albumUri;
    char spotify_album_uri[256];
    sp_link_as_string(sp_link_create_from_album(spAlbum),spotify_album_uri,256);
    albumUri.Format("%s", spotify_album_uri);
    requestThumb((unsigned char*)sp_album_cover(spAlbum),albumUri,pItem, type);
  }

  int popularity = sp_track_popularity(spTrack);
  char rating = '0';
  if (popularity > 10) rating = '1';
  if (popularity > 20) rating = '2';
  if (popularity > 40) rating = '3';
  if (popularity > 60) rating = '4';
  if (popularity > 80) rating = '5';
  pItem->GetMusicInfoTag()->SetRating(rating);

  return pItem;
}

bool SpotifyInterface::requestThumb(unsigned char *imageId, CStdString Uri, CFileItemPtr pItem, SPOTIFY_TYPE type)
{
  //do we have it the cache?
  CStdString thumb;
  Uri.Delete(0,14);

  switch(type){
  case PLAYLIST_TRACK:
    thumb.Format("%s%s.tbn", m_playlistsThumbDir, Uri);
    break;
  case TOPLIST_ALBUM:
  case TOPLIST_TRACK:
    thumb.Format("%s%s.tbn", m_toplistsThumbDir, Uri);
    break;
  default:
    thumb.Format("%s%s.tbn", m_thumbDir, Uri);
    break;
  }

  pItem->SetExtraInfo(thumb);
  if (XFILE::CFile::Exists(thumb))
  {
    //the file exists, then we dont need to download it again
    pItem->SetThumbnailImage(thumb);
    return true;
  }else if (imageId)
  {
    //request for it
    sp_image *spImage = sp_image_create(m_session, (byte*)imageId);
    if (spImage)
    {
      //ok there is one, so download it!
      m_noWaitingThumbs++;
      sp_image_add_load_callback(spImage, &cb_imageLoaded, pItem.get());
	
      //we need to remember what we ask for so we can unload their callbacks if we need to
      imageItemPair pair(spImage, pItem);
      switch(type){
      case PLAYLIST_TRACK:
        m_playlistWaitingThumbs.push_back(pair);
        break;
      case TOPLIST_ALBUM:
      case TOPLIST_TRACK:
        m_toplistWaitingThumbs.push_back(pair);
        break;
      case ARTISTBROWSE_ALBUM:
        m_artistWaitingThumbs.push_back(pair);
        break;
      default:
        m_searchWaitingThumbs.push_back(pair);
        break;
      }
      return true;
    }
  }
  return false;
}

bool SpotifyInterface::addAlbumToLibrary()
{
  CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
  dialog->SetHeading("Spotify");
  if (reconnect())
  {
    CMusicDatabase db;
    CStdString albumname = "";
    CStdString artistname = "";
    CStdString cachedThumb ="";
    if (!m_browseAlbumVector.IsEmpty() && db.Open())
    {
      CSong *song;
      db.BeginTransaction();
      for (int i=0; i < m_browseAlbumVector.GetFileCount(); i++)
      {
        CFileItemPtr item;
        item = m_browseAlbumVector.Get(i);
        MUSIC_INFO::CMusicInfoTag *tag = item->GetMusicInfoTag();
        song = new CSong(*tag);
        albumname = song->strAlbum;
        artistname = song->strAlbumArtist;
        //the AddSong function seems to crash if you dont provide it with a path before the filename
        song->strFileName = "/home/" + song->strFileName;
        cachedThumb = item->GetThumbnailImage();
        CLog::Log( LOGDEBUG, "Spotifylog: adding track to library: %s", song->strFileName.c_str());
        db.AddSong(*song, false);
        delete song;
      }

      //did it create an album?
      int albumId = db.GetAlbumByName(albumname, artistname);
      CAlbum album;
      VECSONGS songs;
      db.GetAlbumInfo(albumId,album,&songs);
      album.strType.Format("spotifyalbum");
      db.SetAlbumInfo(albumId,album,songs,true);
      if (albumId != -1 && !cachedThumb.IsEmpty())
      {
        //yes it did, so add the thumb to the database
        CStdString thumb = CUtil::GetCachedAlbumThumb(albumname,artistname);
        CPicture::CacheThumb(cachedThumb,thumb);
        db.SaveAlbumThumb(albumId, thumb);
      }
      db.CommitTransaction();

      //download info for the artist
     /* CGUIDialogMusicScan* musicScan = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
      if (!musicScan->IsScanning())
      {
        CStdString path;
        int artistId = db.GetArtistByName(artistname);
        if (artistId != -1 )
        {
          path.Format("musicdb://2/%ld/",artistId);
          db.Close();
          musicScan->StartArtistScan(path);
        }
      }*/
      db.Close();
      dialog->SetLine(0 ,"");
      dialog->SetLine(1 ,"Added album to library");
      dialog->SetLine(2 ,"");
      dialog->DoModal();
          
      CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
      message.SetStringParam("musicdb://");
      g_windowManager.SendThreadMessage(message);

      return true;
    }
  }
  dialog->SetLine(0 ,"");
  dialog->SetLine(1 ,"Failed to add album to library");
  dialog->SetLine(2 ,"");
  dialog->DoModal();
  return false;
}

CStdString SpotifyInterface::getUsername()
{
  if (g_advancedSettings.m_spotifyUsername.IsEmpty())
  {
    CGUIDialogKeyboard::ShowAndGetInput(g_advancedSettings.m_spotifyUsername, "Spotify username",false,false);
  }
  return g_advancedSettings.m_spotifyUsername;
}

CStdString SpotifyInterface::getPassword()
{
  if (g_advancedSettings.m_spotifyPassword.IsEmpty())
  {
    CStdString message;
    message.Format("Spotify password for user %s", g_advancedSettings.m_spotifyUsername);
    CGUIDialogKeyboard::ShowAndGetInput(g_advancedSettings.m_spotifyPassword, message,false,true);
  }
  return g_advancedSettings.m_spotifyPassword;
}

void SpotifyInterface::showReconectingDialog()
{
  m_reconectingDialog = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  m_reconectingDialog->SetHeading("Spotify");
  m_reconectingDialog->SetLine(0, "");
  m_reconectingDialog->SetLine(1 ,"Not connected to Spotify.");
  m_reconectingDialog->SetLine(2 ,"Reconnecting...");
  m_reconectingDialog->StartModal();
  m_reconectingDialog->SetPercentage(50);
  m_reconectingDialog->Progress();
  m_isShowingReconnect = true;
}

void SpotifyInterface::hideReconectingDialog()
{
  if (m_isShowingReconnect)
  {
    m_reconectingDialog->Close(true);
    m_isShowingReconnect = false;
  }
}

void SpotifyInterface::showProgressDialog(CStdString message)
{
  m_progressDialog = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  m_progressDialog->SetHeading("Spotify");
  m_progressDialog->SetLine(0, "");
  m_progressDialog->SetLine(1, message.c_str());
  m_progressDialog->SetLine(2, "");
  m_progressDialog->SetCanCancel(false);
  m_progressDialog->SetPercentage(0);
  m_progressDialog->Progress();
  m_progressDialog->StartModal();
}

void SpotifyInterface::hideProgressDialog()
{
  m_progressDialog->Close(true);
}

void SpotifyInterface::showDisclaimer()
{
  CGUIDialogOK* discDialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
  discDialog->SetHeading("This product uses SPOTIFY(R) CORE");
  discDialog->SetLine(0, "but is not endorsed, certified or otherwise" );
  discDialog->SetLine(1, "in any way by Spotify. Spotify is the registered" );
  discDialog->SetLine(2, "trade mark of the Spotify Group." );
  discDialog->DoModal();
}

void SpotifyInterface::waitForThumbs(){
int i = 100;
  while(m_noWaitingThumbs > 0 && i > 0){
    clock_t goal = 100 + clock();
    while (goal > clock());
    CLog::Log( LOGNOTICE, "Spotifylog: waiting for thumbs: %i",m_noWaitingThumbs );
    processEvents();
    i--;
  }
}

void SpotifyInterface::showConnectionErrorDialog(sp_error error)
{
 /* CStdString errorMessage;
  errorMessage.Format("%s", sp_error_message(error));
  CGUIDialogYesNo* m_yesNoDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  m_yesNoDialog->SetHeading("Spotify");
  m_yesNoDialog->SetLine(0, "Disconnected:" );
  m_yesNoDialog->SetLine(1, errorMessage );
  m_yesNoDialog->SetLine(2, "Connect?" );
  m_yesNoDialog->DoModal();
  if (m_yesNoDialog->IsConfirmed())
  {
    //if its a problem with username and password, reset them*/
    if (SP_ERROR_BAD_USERNAME_OR_PASSWORD == error)
    {
      connect(true);
    }else
    {
      connect(false);
    }
  //}
}
