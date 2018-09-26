/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <sys/resource.h>
#include <signal.h>

#include <cstring>

#if defined(TARGET_DARWIN_OSX) || defined(TARGET_FREEBSD)
  #include "Util.h"
  // SDL redefines main as SDL_main
  #ifdef HAS_SDL
    #include <SDL/SDL.h>
  #endif
#endif

#include "AppParamParser.h"
#include "FileItem.h"
#include "messaging/ApplicationMessenger.h"
#include "PlayListPlayer.h"
#include "platform/MessagePrinter.h"
#include "platform/xbmc.h"
#include "utils/log.h"

#ifdef HAS_LIRC
#include "platform/linux/input/LIRC.h"
#endif

#include <locale.h>

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
#if defined(_DEBUG)
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

  CAppParamParser appParamParser;
  appParamParser.Parse(argv, argc);

  return XBMC_Run(true, appParamParser);
}
