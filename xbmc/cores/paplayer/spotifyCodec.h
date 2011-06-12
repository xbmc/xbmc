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

#pragma once

#include "spotinterface.h"
#include "CachingCodec.h"
#include "settings/AdvancedSettings.h"

class SpotifyCodec : public CachingCodec
{
public:
  SpotifyCodec();
  virtual ~SpotifyCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual bool CanSeek(){ return true; }
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

  static bool spotifyPlayerIsFree(){ return playerIsFree; }
  static SpotifyCodec *m_currentPlayer;
  static bool playerIsFree;
  static int SP_CALLCONV cb_musicDelivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
  static void SP_CALLCONV cb_endOfTrack(sp_session *sess);

private:

  bool spotifyPlayerLoad(CStdString trackURI, __int64 &totalTime);
  bool spotifyPlayerUnload();
  sp_session * getSession(){ return g_spotifyInterface->getSession(); }
  bool reconnect(){ return g_spotifyInterface->reconnect(); }
  bool loadPlayer();
  bool unloadPlayer();

  sp_track *m_currentTrack;
  int m_sampleRate;
  int m_channels;
  int m_bitsPerSample;
  int m_bitrate;
  int64_t m_totalTime;
  bool m_hasPlayer;
  bool m_startStream;
  bool m_isPlayerLoaded;
  bool m_endOfTrack;
  int m_bufferSize;
  char *m_buffer;
  int m_bufferPos;
};
