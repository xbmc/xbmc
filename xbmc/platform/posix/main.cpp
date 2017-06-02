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

#include <sys/resource.h>
#include <signal.h>

#include <cstring>

#if defined(TARGET_DARWIN_OSX)
  #include "Util.h"
  // SDL redefines main as SDL_main 
  #ifdef HAS_SDL
    #include <SDL/SDL.h>
  #endif
#include <locale.h>
#endif

#include "AppParamParser.h"
#include "FileItem.h"
#include "messaging/ApplicationMessenger.h"
#include "PlayListPlayer.h"
#include "platform/MessagePrinter.h"
#include "platform/xbmc.h"
#include "platform/XbmcContext.h"
#include "settings/AdvancedSettings.h"
#include "system.h"
#include "utils/log.h"

#ifdef HAS_LIRC
#include "input/linux/LIRC.h"
#endif


namespace
{

class CPOSIXSignalHandleThread : public CThread
{
public:
  CPOSIXSignalHandleThread()
  : CThread("POSIX signal handler")
  {}
protected:
  void Process() override
  {
    CMessagePrinter::DisplayMessage("Exiting application");
    KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
  }
};

extern "C"
{

void XBMC_POSIX_HandleSignal(int sig)
{
  // Spawn handling thread: the current thread that this signal was catched on
  // might have been interrupted in a call to PostMsg() while holding a lock
  // there, which would lead to a deadlock if PostMsg() was called directly here
  // as PostMsg() is not supposed to be reentrant
  auto thread = new CPOSIXSignalHandleThread;
  thread->Create(true);
}

}

}


int main(int argc, char* argv[])
{
  // set up some xbmc specific relationships
  XBMC::Context context;

  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &rlim) == -1)
    CLog::Log(LOGDEBUG, "Failed to set core size limit (%s)", strerror(errno));
#endif
  
  // Set up global SIGINT/SIGTERM handler
  struct sigaction signalHandler;
  std::memset(&signalHandler, 0, sizeof(signalHandler));
  signalHandler.sa_handler = &XBMC_POSIX_HandleSignal;
  signalHandler.sa_flags = SA_RESTART;
  sigaction(SIGINT, &signalHandler, nullptr);
  sigaction(SIGTERM, &signalHandler, nullptr);

  setlocale(LC_NUMERIC, "C");
  g_advancedSettings.Initialize();

  CAppParamParser appParamParser;
  appParamParser.Parse(argv, argc);
  
  return XBMC_Run(true, appParamParser.m_playlist);
}
