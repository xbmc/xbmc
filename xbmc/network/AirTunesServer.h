#pragma once
/*
 * Many concepts and protocol specification in this code are taken from
 * the Boxee project. http://www.boxee.tv
 *
 *      Copyright (C) 2011-2013 Team XBMC
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
#include "system.h"

#ifdef HAS_AIRTUNES

#if defined(HAVE_LIBSHAIRPLAY)
#include "DllLibShairplay.h"
#else
#include "DllLibShairport.h"
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "utils/HttpParser.h"
#include "utils/StdString.h"
#include "filesystem/PipeFile.h"
#include "interfaces/IAnnouncer.h"

class DllLibShairport;


class CAirTunesServer : public CThread, public ANNOUNCEMENT::IAnnouncer
{
public:
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

  static bool StartServer(int port, bool nonlocal, bool usePassword, const CStdString &password="");
  static void StopServer(bool bWait);
  static bool IsRunning();
  static void SetMetadataFromBuffer(const char *buffer, unsigned int size);
  static void SetCoverArtFromBuffer(const char *buffer, unsigned int size);

protected:
  void Process();

private:
  CAirTunesServer(int port, bool nonlocal);
  ~CAirTunesServer();
  bool Initialize(const CStdString &password);
  void Deinitialize();
  static void RefreshCoverArt();
  static void RefreshMetadata();

  int m_port;
#if defined(HAVE_LIBSHAIRPLAY)
  static DllLibShairplay *m_pLibShairplay;//the lib
  raop_t *m_pRaop;
  XFILE::CPipeFile *m_pPipe;
#else
  static DllLibShairport *m_pLibShairport;//the lib
#endif
  static CAirTunesServer *ServerInstance;
  static CStdString m_macAddress;
  static CCriticalSection m_metadataLock;
  static std::string m_metadata[3];//0 - album, 1 - title, 2 - artist
  static bool m_streamStarted;

  class AudioOutputFunctions
  {
    public:
#if defined(HAVE_LIBSHAIRPLAY)
      static void* audio_init(void *cls, int bits, int channels, int samplerate);
      static void  audio_set_volume(void *cls, void *session, float volume);
	    static void  audio_set_metadata(void *cls, void *session, const void *buffer, int buflen);
	    static void  audio_set_coverart(void *cls, void *session, const void *buffer, int buflen);
      static void  audio_process(void *cls, void *session, const void *buffer, int buflen);
      static void  audio_flush(void *cls, void *session);
      static void  audio_destroy(void *cls, void *session);
#else
      static void ao_initialize(void);
      static void ao_set_volume(float volume);
      static int ao_play(ao_device *device, char *output_samples, uint32_t num_bytes);
      static int ao_default_driver_id(void);
      static ao_device* ao_open_live( int driver_id, ao_sample_format *format,
                                      ao_option *option);
      static int ao_close(ao_device *device);
      /* -- Device Setup/Playback/Teardown -- */
      static int ao_append_option(ao_option **options, const char *key, const char *value);
      static void ao_free_options(ao_option *options);
      static char* ao_get_option(ao_option *options, const char* key);
      static void ao_set_metadata(const char *buffer, unsigned int size);
      static void ao_set_metadata_coverart(const char *buffer, unsigned int size);      
#endif
    };
};

#endif
