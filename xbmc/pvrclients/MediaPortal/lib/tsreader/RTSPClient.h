#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if defined TSREADER && defined LIVE555

//#include "os-dependent.h"
#include "TSThread.h"

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"

#include "MemoryBuffer.h"

class CRTSPClient: public TSThread
{
public:
  CRTSPClient();
  virtual ~CRTSPClient(void);
  bool Initialize(CMemoryBuffer* buffer);
  bool OpenStream(char* url);
  bool Play(double fStart,double fDuration);
  void Stop();
  bool IsRunning();
  long Duration();
  bool Run();
  bool Pause();
  bool IsPaused();
  void Continue();
  void FillBuffer(DWORD byteCount);

  char* getSDPDescription();
  bool UpdateDuration();

protected:
  CMemoryBuffer* m_buffer;
  Medium* createClient(UsageEnvironment& env,int verbosityLevel, char const* applicationName) ;
  char* getSDPDescriptionFromURL(Medium* client, char const* url,
               char const* username, char const* password,
               char const* /*proxyServerName*/,
               unsigned short /*proxyServerPortNum*/,
               unsigned short /*clientStartPort*/) ;
  Boolean clientSetupSubsession(Medium* client, MediaSubsession* subsession,Boolean streamUsingTCP) ;
  Boolean clientStartPlayingSession(Medium* client,MediaSession* session) ;
  Boolean clientTearDownSession(Medium* client,MediaSession* session) ;
  void closeMediaSinks();
  void tearDownStreams();
  bool setupStreams();
  void checkForPacketArrival(void* /*clientData*/) ;
  MediaSession* m_session ;

  Boolean allowProxyServers ;
  Boolean controlConnectionUsesTCP ;
  Boolean supportCodecSelection ;
  char const* clientProtocolName ;
  portNumBits tunnelOverHTTPPortNum ;
  unsigned statusCode ;
  char const* singleMedium ;
  unsigned short desiredPortNum   ;
  Boolean createReceivers ;
  int simpleRTPoffsetArg ;
  unsigned socketInputBufferSize ;
  Boolean streamUsingTCP ;
  unsigned fileSinkBufferSize ;
  Boolean oneFilePerFrame ;
  
public:
  UsageEnvironment* m_env;
  Medium* m_ourClient ;
  char* getOptionsResponse(Medium* client, char const* url,char* username, char* password) ;
  void shutdown();
  bool startPlayingStreams();

  //thread
  void StartBufferThread();
  void StopBufferThread();
  virtual void ThreadProc();
  bool m_BufferThreadActive;
  long m_duration;
  double m_fStart;
  double m_fDuration;
  char m_url[2048];
  bool m_bRunning;
  bool m_bPaused;
  char m_outFileName[1000];
};
#endif //TSREADER
