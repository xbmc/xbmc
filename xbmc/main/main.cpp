/*
 *      Copyright (C) 2005-2012 Team XBMC
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

/*
stacktrace code from http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112
*/

#include "system.h"
#include "settings/AppParamParser.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "utils/log.h"
#include "xbmc.h"
#ifdef _LINUX
#include <sys/resource.h>
#include <signal.h>

#ifdef TARGET_RASPBERRY_PI
#include "backtrace.h"
#endif

#endif
#if defined(TARGET_DARWIN_OSX)
  #include "Util.h"
  // SDL redefines main as SDL_main 
  #ifdef HAS_SDL
    #include <SDL/SDL.h>
  #endif
#endif
#ifdef HAS_LIRC
#include "input/linux/LIRC.h"
#endif
#include "XbmcContext.h"




int main(int argc, char* argv[])
{
/*#ifdef TARGET_RASPBERRY_PI
    struct sigaction sa;

    sa.sa_sigaction = bt_sighandler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGKILL, &sa, NULL);
#endif
*/

  // set up some xbmc specific relationships
  XBMC::Context context;

  bool renderGUI = true;
  //this can't be set from CAdvancedSettings::Initialize() because it will overwrite
  //the loglevel set with the --debug flag
#ifdef _DEBUG
  g_advancedSettings.m_logLevel     = LOG_LEVEL_DEBUG;
  g_advancedSettings.m_logLevelHint = LOG_LEVEL_DEBUG;
#else
  g_advancedSettings.m_logLevel     = LOG_LEVEL_NORMAL;
  g_advancedSettings.m_logLevelHint = LOG_LEVEL_NORMAL;
#endif
  CLog::SetLogLevel(g_advancedSettings.m_logLevel);

#ifdef _LINUX
#if defined(DEBUG)
  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &rlim) == -1)
    CLog::Log(LOGDEBUG, "Failed to set core size limit (%s)", strerror(errno));
#endif
#endif
  setlocale(LC_NUMERIC, "C");
  g_advancedSettings.Initialize();

#ifndef _WIN32
  CAppParamParser appParamParser;
  appParamParser.Parse((const char **)argv, argc);
#endif
  return XBMC_Run(renderGUI);
}
