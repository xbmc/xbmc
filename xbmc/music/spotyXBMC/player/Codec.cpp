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

#include "../session/Session.h"
#include "Codec.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include <stdint.h>
#include "PlayerHandler.h"
#include "../radio/RadioHandler.h"
#include "../SxSettings.h"
#include "../../PlayListPlayer.h"
#include "../../../playlists/PlayList.h"

using namespace std;
using namespace PLAYLIST;

namespace addon_music_spotify {

  Codec::Codec() {
    m_SampleRate = 44100;
    m_Channels = 2;
    m_BitsPerSample = 16;
    //the bitrate is hardcoded, we dont no it before first music delivery and then its to late, the skin has already printed it out
    m_Bitrate = Settings::getInstance()->useHighBitrate() ? 320000 : 160000;
    m_CodecName = "spotify";
	m_DataFormat = AE_FMT_S16NE;
    m_TotalTime = 0;
    m_currentTrack = 0;
    m_isPlayerLoaded = false;
    m_buffer = 0;
  }

  Codec::~Codec() {
    DeInit();
    delete m_buffer;
  }

  bool Codec::Init(const CStdString & strFile, unsigned int filecache) {
    m_bufferSize = 2048 * sizeof(int16_t) * 50;
    m_buffer = new char[m_bufferSize];
    CStdString uri = URIUtils::GetFileName(strFile);
    CStdString extension = uri.Right(uri.GetLength() - uri.Find('.') - 1);
    if (extension.Left(12) == "spotifyradio") {
      //if its a radiotrack the radionumber and tracknumber is secretly encoded at the end of the extension
      CStdString trackStr = extension.Right(
          extension.GetLength() - extension.ReverseFind('#') - 1);
      Logger::printOut(extension);
      CStdString radioNumber = extension.Left(uri.Find('#'));
      Logger::printOut(radioNumber);
      radioNumber = radioNumber.Right(
          radioNumber.GetLength() - radioNumber.Find('#') - 1);
      Logger::printOut("loading codec radio");
      RadioHandler::getInstance()->pushToTrack(atoi(radioNumber),
          atoi(trackStr));
    }
    //we have a non legit extension so remove it manually
    uri = uri.Left(uri.Find('.'));

    Logger::printOut("trying to load track:");
    Logger::printOut(uri);
    sp_link *spLink = sp_link_create_from_string(uri);
    m_currentTrack = sp_link_as_track(spLink);
    sp_track_add_ref(m_currentTrack);
    sp_link_release(spLink);
    m_endOfTrack = false;
    m_bufferPos = 0;
    m_startStream = false;
    m_isPlayerLoaded = false;
    m_TotalTime = sp_track_duration(m_currentTrack);

    //prefetch the next track!

	  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
	  int nextSong = g_playlistPlayer.GetNextSong();

	  if (nextSong >= 0 && nextSong < playlist.size()){
	  	CFileItemPtr song = playlist[nextSong];
	  	if (song != NULL){
	  		CStdString uri = song->GetPath();
	  		if (uri.Left(7).Equals("spotify")){
	  			uri = uri.Left(uri.Find('.'));
	  	    Logger::printOut("prefetching track:");
	  	    Logger::printOut(uri);
	  	    sp_link *spLink = sp_link_create_from_string(uri);
	  	    sp_track* track = sp_link_as_track(spLink);
	  	    sp_session_player_prefetch(getSession(), track);
	  	    sp_link_release(spLink);
	  		}
	  	}
	  }

    return true;
  }

  void Codec::DeInit() {
    unloadPlayer();
    PlayerHandler::getInstance()->removeCodec();
  }

  int64_t Codec::Seek(int64_t iSeekTime) {
    Logger::printOut("trying to seek");
    sp_session_player_seek(getSession(), iSeekTime);
    m_bufferPos = 0;
    return iSeekTime;
  }

  int Codec::ReadPCM(BYTE *pBuffer, int size, int *actualsize) {
    *actualsize = 0;
    if (!m_isPlayerLoaded)
      loadPlayer();

    if (m_startStream) {
      if (m_endOfTrack && m_bufferPos == 0) {
        return READ_EOF;
      } else if (m_bufferPos > 0) {
        int amountToMove = m_bufferPos;
        if (m_bufferPos > size)
          amountToMove = size;
        memcpy(pBuffer, m_buffer, amountToMove);
        memmove(m_buffer, m_buffer + amountToMove, m_bufferSize - amountToMove);
        m_bufferPos -= amountToMove;
        *actualsize = amountToMove;
      }
    }
    return READ_SUCCESS;
  }

  bool Codec::CanInit() {
    return true;
  }

  void Codec::endOfTrack() {
    m_endOfTrack = true;
  }

  bool Codec::loadPlayer() {
    Logger::printOut("load player");
    if (!m_isPlayerLoaded) {
      //do we have a track at all?
      if (m_currentTrack) {
        CStdString name;
        Logger::printOut("load player 2");
        if (sp_track_is_loaded(m_currentTrack)) {
          sp_error error = sp_session_player_load(getSession(), m_currentTrack);
          CStdString message;
          Logger::printOut("load player 3");
          message.Format("%s", sp_error_message(error));
          Logger::printOut(message);
          Logger::printOut("load player 4");
          if (SP_ERROR_OK == error) {
            sp_session_player_play(getSession(), true);
            m_isPlayerLoaded = true;
            Logger::printOut("load player 5");
            return true;
          }
        }
      } else
        return false;
    }
    return true;
  }

  bool Codec::unloadPlayer() {
    //make sure there is no music_delivery while we are removing the codec
    while (!Session::getInstance()->lock()) {
    }
    if (m_isPlayerLoaded) {
      sp_session_player_play(getSession(), false);
      sp_session_player_unload(getSession());
      if (m_currentTrack != NULL) {
        sp_track_release(m_currentTrack);
      }
    }

    m_currentTrack = NULL;
    m_isPlayerLoaded = false;
    m_endOfTrack = true;
    Session::getInstance()->unlock();
    return true;
  }

  int Codec::musicDelivery(int channels, int sample_rate, const void *frames,
      int num_frames) {
    //Logger::printOut("music delivery");
    int amountToMove = num_frames * (int) sizeof(int16_t) * channels;

    if ((m_bufferPos + amountToMove) >= m_bufferSize) {
      amountToMove = m_bufferSize - m_bufferPos;
    }

    memcpy(m_buffer + m_bufferPos, frames, amountToMove);
    m_bufferPos += amountToMove;

    if (!m_startStream && m_bufferPos == m_bufferSize) {
      //now the buffer is full, start playing
      m_startStream = true;
    }

    return amountToMove / ((int) sizeof(int16_t) * channels);
  }

  sp_session* Codec::getSession() {
    return Session::getInstance()->getSpSession();
  }

  CAEChannelInfo Codec::GetChannelInfo()
  {
    static enum AEChannel map[2][3] = {
      {AE_CH_FC, AE_CH_NULL},
      {AE_CH_FL, AE_CH_FR  , AE_CH_NULL}
    };

    if (m_Channels > 2) {
	  Logger::printOut("m_Channels is bigger than 2, please fix code, I can´t return a valid AEChannel map");
//    return CAEUtil::GuessChLayout(m_Channels);
    }

  return CAEChannelInfo(map[m_Channels - 1]);
  }
}

/* namespace addon_music_spotify */
