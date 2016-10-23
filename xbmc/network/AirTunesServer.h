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

#include "DllLibShairplay.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <list>
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "utils/HttpParser.h"
#include "filesystem/PipeFile.h"
#include "interfaces/IAnnouncer.h"
#include "interfaces/IActionListener.h"

class CDACP;
class CVariant;

class CAirTunesServer : public ANNOUNCEMENT::IAnnouncer, public IActionListener, public CThread
{
public:
  // ANNOUNCEMENT::IAnnouncer
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

  void RegisterActionListener(bool doRegister);
  static void EnableActionProcessing(bool enable);
  // IACtionListener
  virtual bool OnAction(const CAction &action);
  
  //CThread
  virtual void Process();

  static bool StartServer(int port, bool nonlocal, bool usePassword, const std::string &password="");
  static void StopServer(bool bWait);
  static bool IsRunning();
  bool IsRAOPRunningInternal();
  static void SetMetadataFromBuffer(const char *buffer, unsigned int size);
  static void SetCoverArtFromBuffer(const char *buffer, unsigned int size);
  static void SetupRemoteControl();
  static void FreeDACPRemote();

private:
  CAirTunesServer(int port, bool nonlocal);
  ~CAirTunesServer();
  bool Initialize(const std::string &password);
  void Deinitialize();
  static void RefreshCoverArt(const char *outputFilename = NULL);
  static void RefreshMetadata();
  static void ResetMetadata();

  int m_port;
  static DllLibShairplay *m_pLibShairplay;//the lib
  raop_t *m_pRaop;
  XFILE::CPipeFile *m_pPipe;
  static CAirTunesServer *ServerInstance;
  static std::string m_macAddress;
  static CCriticalSection m_metadataLock;
  static std::string m_metadata[3];//0 - album, 1 - title, 2 - artist
  static bool m_streamStarted;
  static CCriticalSection m_dacpLock;
  static CDACP *m_pDACP;
  static std::string m_dacp_id;
  static std::string m_active_remote_header;
  static CCriticalSection m_actionQueueLock;
  static std::list<CAction> m_actionQueue;
  static CEvent m_processActions;
  static int m_sampleRate;

  class AudioOutputFunctions
  {
    public:
      static void* audio_init(void *cls, int bits, int channels, int samplerate);
      static void  audio_set_volume(void *cls, void *session, float volume);
	    static void  audio_set_metadata(void *cls, void *session, const void *buffer, int buflen);
	    static void  audio_set_coverart(void *cls, void *session, const void *buffer, int buflen);
      static void  audio_process(void *cls, void *session, const void *buffer, int buflen);
      static void  audio_destroy(void *cls, void *session);
      static void  audio_remote_control_id(void *cls, const char *identifier, const char *active_remote_header);
      static void  audio_set_progress(void *cls, void *session, unsigned int start, unsigned int curr, unsigned int end);
    };
};

#endif
