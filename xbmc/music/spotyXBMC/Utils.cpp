/*
 spotyxbmc2 - A project to integrate Spotify into XBMC
 Copyright (C) 2011  David Erenger

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

#include "Utils.h"
#include "SxSettings.h"
#include "settings/AdvancedSettings.h"
#include "Logger.h"
#include "../tags/MusicInfoTag.h"
#include "../Album.h"
#include "../Artist.h"
#include "../../MediaSource.h"
#include "Util.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "utils/StringUtils.h"

namespace addon_music_spotify {

  Utils::Utils() {
  }

  Utils::~Utils() {
  }

  void Utils::cleanTags(string & text) {
    bool done = false;
    while (!done) {
      // Look for start of tag:
      size_t leftPos = text.find('<');
      if (leftPos != string::npos) {
        // See if tag close is in this line:
        size_t rightPos = text.find('>');
        if (rightPos == string::npos) {
          done = true;
          text.erase(leftPos);
        } else
          text.erase(leftPos, rightPos - leftPos + 1);
      } else
        done = true;
    }
  }

  const CFileItemPtr Utils::SxAlbumToItem(SxAlbum *album, string prefix,
      int discNumber) {
    //wait for it to finish loading
    while (!album->isLoaded()) {
    }

    CAlbum outAlbum;
    outAlbum.artist =  StringUtils::Split(album->getAlbumArtistName(), g_advancedSettings.m_musicItemSeparator);
    CStdString title;
    if (discNumber != 0)
      title.Format("%s%s %s %i", prefix, album->getAlbumName(), "disc",
          discNumber);
    else
      title.Format("%s%s", prefix, album->getAlbumName());
    outAlbum.strAlbum = title;
    outAlbum.iYear = album->getAlbumYear();
    outAlbum.strReview = album->getReview();
    outAlbum.iRating = album->getAlbumRating();
    CStdString path;
    path.Format("musicdb://3/%s#%i", album->getUri(), discNumber);
    const CFileItemPtr pItem(new CFileItem(path, outAlbum));
    if (album->hasThumb())
      pItem->SetArt("thumb",album->getThumb()->getPath());
    pItem->SetProperty("fanart_image", *album->getFanart());
    return pItem;
  }

  const CFileItemPtr Utils::SxTrackToItem(SxTrack* track, string prefix,
      int trackNumber) {
    //wait for it to finish loading
    while (!track->isLoaded()) {
    }
    CSong outSong;
    CStdString path;
    path.Format("%s.spotify", track->getUri());
    outSong.strFileName = path;
    CStdString title;
    title.Format("%s%s", prefix, track->getName());
    outSong.strTitle = title;
    outSong.iTrack = trackNumber == -1 ? track->getTrackNumber() : trackNumber;
    outSong.iDuration = track->getDuration();
    outSong.rating = track->getRating();
    char ratingChar[3];
    CStdString ratingStr; //= itoa(1 + (track->getRating() / 2), ratingChar, 10);
    ratingStr.Format("%i", 1 + (track->getRating() / 2), ratingChar, 10);
    //delete ratingChar;
    outSong.rating = ratingStr[0];
    outSong.artist =  StringUtils::Split(track->getArtistName(), g_advancedSettings.m_musicItemSeparator);
    outSong.iYear = track->getYear();
    outSong.strAlbum = track->getAlbumName();
    outSong.albumArtist = StringUtils::Split(track->getAlbumArtistName(), g_advancedSettings.m_musicItemSeparator);
    const CFileItemPtr pItem(new CFileItem(outSong));
    if (track->hasThumb())
      pItem->SetArt("thumb",track->getThumb()->getPath());
    pItem->SetProperty("fanart_image", *track->getFanart());
    return pItem;
  }

  const CFileItemPtr Utils::SxArtistToItem(SxArtist* artist, string prefix) {
    //wait for it to finish loading
    while (!artist->isLoaded()) {
    }

    CStdString path;
    path.Format("musicdb://2/%s/", artist->getUri());

    CFileItemPtr pItem(new CFileItem(path, true));
    CStdString label;
    label.Format("%s%s", prefix, artist->getArtistName());
    pItem->SetLabel(label);
    label.Format("A %s", artist->getArtistName());

    pItem->GetMusicInfoTag()->SetTitle(label);
    pItem->GetMusicInfoTag()->SetArtist(artist->getArtistName());

    if (artist->hasThumb())
      pItem->SetArt("thumb",artist->getThumb()->getPath());

    pItem->SetIconImage("DefaultArtist.png");
    pItem->SetProperty("fanart_image", *artist->getFanart());
    pItem->SetProperty("artist_description", artist->getBio());

    return pItem;
  }

  void Utils::createDir(CStdString path) {
    XFILE::CDirectory::Create(path);

  }

  void Utils::removeDir(CStdString path) {
    XFILE::CDirectory::Remove(path);
  }

  void Utils::removeFile(CStdString path) {
    XFILE::CFile::Delete(path);
  }

  void Utils::updatePlaylists() {
     CStdString path;
     path.Format("special://musicplaylists/");
     updatePath(path);
   }

   void Utils::updateMenu() {
     Logger::printOut("updating main music menu");
     CStdString path;
     path.Format("");
     updatePath(path);
   }

   void Utils::updatePlaylist(int index) {
     //TODO FIX!
     Logger::printOut("updating playlist view");
     CStdString path;
     path.Format("musicdb://3/spotify:playlist:%i/", index);
     updatePath(path);
   }

   void Utils::updateAllArtists() {
     Logger::printOut("updating all artists");
     CStdString path;
     path.Format("musicdb://2/");
     updatePath(path);
   }

   void Utils::updateAllAlbums() {
     Logger::printOut("updating all albums");
     CStdString path;
     path.Format("musicdb://3/");
     updatePath(path);
   }

   void Utils::updateAllTracks() {
     Logger::printOut("updating all tracks");
     CStdString path;
     path.Format("musicdb://4/");
     updatePath(path);
   }

   void Utils::updateRadio(int radio) {
     Logger::printOut("updating radio results");
     CStdString path;
     path.Format("musicdb://3/spotify:radio:%i/", radio);
     updatePath(path);
   }

   void Utils::updateToplistMenu() {
     Logger::printOut("updating toplistmenu");
     CStdString path;
     path.Format("musicdb://5/");
     updatePath(path);
   }

   void Utils::updateSearchResults(string query) {
     //we need to replace the whitespaces with %20

     int pos = 0;
     while (pos != string::npos) {
       pos = query.find(' ');
       if (pos != string::npos) {
         query.replace(pos, 1, "%20");
       }
     }

     Logger::printOut("updating search results");
     CStdString path;
     path.Format("musicsearch://%s/", query);
     updatePath(path);
   }

   void Utils::updatePath(CStdString& path) {
     CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
     message.SetStringParam(path);
     g_windowManager.SendThreadMessage(message);
   }

} /* namespace addon_music_spotify */
