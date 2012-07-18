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

#ifndef CODEC_H_
#define CODEC_H_
#include "../../../cores/paplayer/CachingCodec.h"
#include <libspotify/api.h>

namespace addon_music_spotify {

  class Codec: public CachingCodec {
  public:
    Codec();
    virtual ~Codec();
    virtual bool Init(const CStdString &strFile, unsigned int filecache);
    virtual void DeInit();
    virtual bool CanSeek() {
      return true;
    }
    virtual int64_t Seek(int64_t iSeekTime);
    virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
    virtual bool CanInit();
	virtual CAEChannelInfo GetChannelInfo();

    int musicDelivery(int channels, int sample_rate, const void *frames, int num_frames);
    void endOfTrack();

    friend class PlayerHandler;

  private:

    bool loadPlayer();
    bool unloadPlayer();
    sp_session * getSession();
    sp_track *m_currentTrack;
    bool m_startStream;
    bool m_isPlayerLoaded;
    bool m_endOfTrack;
    int m_bufferSize;
    char *m_buffer;
    int m_bufferPos;
  };

} /* namespace addon_music_spotify */
#endif /* CODEC_H_ */
