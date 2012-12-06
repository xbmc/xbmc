#pragma once
/*
 * Many concepts and protocol specification in this code are taken from
 * the Boxee project. http://www.boxee.tv
 *
 *      Copyright (C) 2011-2012 Team XBMC
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
#include "system.h"

#ifdef HAS_AIRTUNES

#if defined(TARGET_WINDOWS)
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

class DllLibShairport;


class CAirTunesServer : public CThread
{
public:
  static bool StartServer(int port, bool nonlocal, bool usePassword, const CStdString &password="");
  static void StopServer(bool bWait);
  static void SetMetadataFromBuffer(const char *buffer, unsigned int size);
  static void SetCoverArtFromBuffer(const char *buffer, unsigned int size);

protected:
  void Process();

private:
  CAirTunesServer(int port, bool nonlocal);
  ~CAirTunesServer();
  bool Initialize(const CStdString &password);
  void Deinitialize();

  int m_port;
#if defined(TARGET_WINDOWS)
  static DllLibShairplay *m_pLibShairplay;//the lib
  raop_t *m_pRaop;
  XFILE::CPipeFile *m_pPipe;
#else
  static DllLibShairport *m_pLibShairport;//the lib
#endif
  static CAirTunesServer *ServerInstance;
  static CStdString m_macAddress;

  class AudioOutputFunctions
  {
    public:
#if defined(TARGET_WINDOWS)
      static void* audio_init(void *cls, int bits, int channels, int samplerate);
      static void  audio_set_volume(void *cls, void *session, float volume);
	    static void  audio_set_metadata(void *cls, void *session, const void *buffer, int buflen);
	    static void  audio_set_coverart(void *cls, void *session, const void *buffer, int buflen);
      static void  audio_process(void *cls, void *session, const void *buffer, int buflen);
      static void  audio_flush(void *cls, void *session);
      static void  audio_destroy(void *cls, void *session);
#else
      static void ao_initialize(void);
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
