/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <signal.h>
#include <sys/resource.h>

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
#include "PlatformPosix.h"
#include "utils/log.h"

#ifdef HAS_LIRC
#include "platform/linux/input/LIRC.h"
#endif

#include <locale.h>

namespace
{

extern "C"
{

void XBMC_POSIX_HandleSignal(int sig)
{
  // Setting an atomic flag is one of the only useful things that is permitted by POSIX
  // in signal handlers
  CPlatformPosix::RequestQuit();
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
