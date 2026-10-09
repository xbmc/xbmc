/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformPosix.h"
#include "application/AppEnvironment.h"
#include "application/AppParamParser.h"
#include "platform/xbmc.h"

#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
#include "platform/linux/AppParamParserLinux.h"
#endif

#ifdef TARGET_WEBOS
#include "platform/linux/AppParamParserWebOS.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>

#include <setjmp.h>
#include <sys/resource.h>
#include <ucontext.h>

thread_local sigjmp_buf g_starfishJmpBuf;
thread_local bool g_inStarfishGuard = false;

namespace
{
extern "C" void XBMC_POSIX_HandleSignal(int sig)
{
  // Setting an atomic flag is one of the only useful things that is permitted by POSIX
  // in signal handlers
  CPlatformPosix::RequestQuit();
}

extern "C" void XBMC_CrashSignalHandler(int sig, siginfo_t* info, void* secret)
{
  const char* sigName = "UNKNOWN";
  if (sig == SIGSEGV) sigName = "SIGSEGV";
  else if (sig == SIGBUS) sigName = "SIGBUS";
  else if (sig == SIGABRT) sigName = "SIGABRT";
  else if (sig == SIGFPE) sigName = "SIGFPE";

  ucontext_t* uc = static_cast<ucontext_t*>(secret);
  uintptr_t pc = 0, lr = 0, sp = 0;
#if defined(__arm__)
  if (uc)
  {
    pc = uc->uc_mcontext.arm_pc;
    lr = uc->uc_mcontext.arm_lr;
    sp = uc->uc_mcontext.arm_sp;
  }
#endif

  char buf[512];
  int len = snprintf(buf, sizeof(buf),
           "\n\n*** KODI CRASH SIGNAL: %s (%d) at fault_addr=%p (PC=%p, LR=%p, SP=%p) ***\n\n",
           sigName, sig, info ? info->si_addr : nullptr,
           reinterpret_cast<void*>(pc), reinterpret_cast<void*>(lr), reinterpret_cast<void*>(sp));
  write(STDERR_FILENO, buf, len);

  int fd = open("/media/developer/apps/usr/palm/applications/org.xbmc.kodi/.kodi/temp/kodi.log",
                O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd >= 0)
  {
    write(fd, buf, len);

    int mapsFd = open("/proc/self/maps", O_RDONLY);
    if (mapsFd >= 0)
    {
      const char hdr[] = "--- /proc/self/maps matching crash addresses ---\n";
      write(fd, hdr, sizeof(hdr) - 1);
      char lineBuf[512];
      char chunk[1024];
      int linePos = 0;
      ssize_t n;
      while ((n = read(mapsFd, chunk, sizeof(chunk))) > 0)
      {
        for (ssize_t i = 0; i < n; ++i)
        {
          char c = chunk[i];
          if (linePos < static_cast<int>(sizeof(lineBuf)) - 1)
            lineBuf[linePos++] = c;
          if (c == '\n' || linePos >= static_cast<int>(sizeof(lineBuf)) - 1)
          {
            lineBuf[linePos] = '\0';
            unsigned long start = 0, end = 0;
            if (sscanf(lineBuf, "%lx-%lx", &start, &end) == 2)
            {
              uintptr_t fault = reinterpret_cast<uintptr_t>(info ? info->si_addr : nullptr);
              if ((pc >= start && pc < end) || (lr >= start && lr < end) || (fault >= start && fault < end))
              {
                write(fd, lineBuf, linePos);
              }
            }
            linePos = 0;
          }
        }
      }
      close(mapsFd);
    }
  }

  if (g_inStarfishGuard)
  {
    const char msg[] = "\n*** RECOVERING FROM CRASH IN STARFISH HWDEC: Safely returning to software fallback ***\n\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    if (fd >= 0)
    {
      write(fd, msg, sizeof(msg) - 1);
      close(fd);
    }
    siglongjmp(g_starfishJmpBuf, 1);
  }

  if (fd >= 0)
    close(fd);

  // Reset to default action and re-raise so kernel terminates / dumps core
  struct sigaction sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_DFL;
  sigaction(sig, &sa, nullptr);
  raise(sig);
}
} // namespace


int main(int argc, char* argv[])
{
#if defined(_DEBUG)
  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &rlim) == -1)
    fprintf(stderr, "Failed to set core size limit (%s).\n", strerror(errno));
#endif

  // Set up global SIGINT/SIGTERM handler
  struct sigaction signalHandler;
  std::memset(&signalHandler, 0, sizeof(signalHandler));
  signalHandler.sa_handler = &XBMC_POSIX_HandleSignal;
  signalHandler.sa_flags = SA_RESTART;
  sigaction(SIGINT, &signalHandler, nullptr);
  sigaction(SIGTERM, &signalHandler, nullptr);

  // Set up alternate signal stack for stack overflow handling
  static char altstackMem[SIGSTKSZ * 4];
  stack_t ss;
  ss.ss_sp = altstackMem;
  ss.ss_size = sizeof(altstackMem);
  ss.ss_flags = 0;
  sigaltstack(&ss, nullptr);

  // Set up crash signal handlers
  struct sigaction crashHandler;
  std::memset(&crashHandler, 0, sizeof(crashHandler));
  crashHandler.sa_sigaction = &XBMC_CrashSignalHandler;
  crashHandler.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigaction(SIGSEGV, &crashHandler, nullptr);
  sigaction(SIGBUS, &crashHandler, nullptr);
  sigaction(SIGABRT, &crashHandler, nullptr);
  sigaction(SIGFPE, &crashHandler, nullptr);

  std::atexit([]() {
    const char msg[] = "\n*** KODI PROCESS EXITED (atexit) ***\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    int fd = open("/media/developer/apps/usr/palm/applications/org.xbmc.kodi/.kodi/temp/kodi.log",
                  O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0)
    {
      write(fd, msg, sizeof(msg) - 1);
      close(fd);
    }
  });

  setlocale(LC_NUMERIC, "C");

#ifdef TARGET_WEBOS
  CAppParamParserWebOS appParamParser;
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
  CAppParamParserLinux appParamParser;
#else
  CAppParamParser appParamParser;
#endif
  appParamParser.Parse(argv, argc);

  CAppEnvironment::SetUp(appParamParser.GetAppParams());
  int status = XBMC_Run(true);
  CAppEnvironment::TearDown();

  return status;
}
