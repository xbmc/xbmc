/*
*      Copyright (C) 2005-2013 Team XBMC
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
#define WL_EGL_PLATFORM

#include "system.h"

/* HAVE_WAYLAND is implicit in HAVE_WAYLAND_XBMC_PROTO */
#if defined(HAVE_WAYLAND_XBMC_PROTO)

#include <limits>
#include <sstream>
#include <stdexcept>
#include <queue>

#include <tr1/tuple>

#include <iostream>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>

#include <gtest/gtest.h>

#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-version.h>
#include "xbmc_wayland_test_client_protocol.h"

#include "windowing/egl/wayland/Callback.h"
#include "windowing/egl/wayland/Compositor.h"
#include "windowing/egl/wayland/Display.h"
#include "windowing/egl/wayland/OpenGLSurface.h"
#include "windowing/egl/wayland/Registry.h"
#include "windowing/egl/wayland/Surface.h"
#include "windowing/egl/wayland/Shell.h"
#include "windowing/egl/wayland/ShellSurface.h"
#include "windowing/egl/wayland/XBMCConnection.h"
#include "windowing/egl/wayland/XBMCSurface.h"
#include "windowing/egl/EGLNativeTypeWayland.h"
#include "windowing/wayland/EventLoop.h"
#include "windowing/wayland/EventQueueStrategy.h"
#include "windowing/wayland/TimeoutManager.h"
#include "windowing/wayland/CursorManager.h"
#include "windowing/wayland/InputFactory.h"
#include "windowing/wayland/Wayland11EventQueueStrategy.h"
#include "windowing/wayland/Wayland12EventQueueStrategy.h"
#include "windowing/WinEvents.h"

#include "windowing/DllWaylandClient.h"
#include "windowing/DllWaylandEgl.h"
#include "windowing/DllXKBCommon.h"

#include "utils/StringUtils.h"
#include "test/TestUtils.h"
#include "input/linux/XKBCommonKeymap.h"
#include "input/XBMC_keysym.h"

#define WAYLAND_VERSION ((WAYLAND_VERSION_MAJOR << 16) | (WAYLAND_VERSION_MINOR << 8) | (WAYLAND_VERSION_MICRO))
#define WAYLAND_VERSION_CHECK(major, minor, micro) ((major << 16) | (minor << 8) | (micro))

using ::testing::Values;
using ::testing::WithParamInterface;

namespace
{
static const int DefaultProcessWaitTimeout = 3000; // 3000ms
}

namespace xbmc
{
namespace test
{
namespace wayland
{
class XBMCWayland :
  boost::noncopyable
{
public:

  XBMCWayland(struct xbmc_wayland *xbmcWayland);
  ~XBMCWayland();

  struct wl_surface * MostRecentSurface();

  void AddMode(int width,
               int height,
               uint32_t refresh,
               enum wl_output_mode mode);
  void MovePointerTo(struct wl_surface *surface,
                     wl_fixed_t x,
                     wl_fixed_t y);
  void SendButtonTo(struct wl_surface *surface,
                    uint32_t button,
                    uint32_t state);
  void SendAxisTo(struct wl_surface *,
                  uint32_t axis,
                  wl_fixed_t value);
  void SendKeyToKeyboard(struct wl_surface *surface,
                         uint32_t key,
                         enum wl_keyboard_key_state state);
  void SendModifiersToKeyboard(struct wl_surface *surface,
                               uint32_t depressed,
                               uint32_t latched,
                               uint32_t locked,
                               uint32_t group);
  void GiveSurfaceKeyboardFocus(struct wl_surface *surface);
  void PingSurface (struct wl_surface *surface,
                    uint32_t serial);

private:

  struct xbmc_wayland *m_xbmcWayland;
};
}

class Process :
  boost::noncopyable
{
public:

  Process(const CStdString &base,
          const CStdString &socket);
  ~Process();

  void WaitForSignal(int signal, int timeout);
  void WaitForStatus(int status, int timeout);

  void Interrupt();
  void Terminate();
  void Kill();
  
  pid_t Pid();

private:
  
  void SendSignal(int signal);
  
  void Child(const char *program,
             char * const *options);
  void ForkError();
  void Parent();
  
  pid_t m_pid;
};
}
}

namespace xt = xbmc::test;
namespace xw = xbmc::wayland;
namespace xtw = xbmc::test::wayland;
namespace xwe = xbmc::wayland::events;

xtw::XBMCWayland::XBMCWayland(struct xbmc_wayland *xbmcWayland) :
  m_xbmcWayland(xbmcWayland)
{
}

xtw::XBMCWayland::~XBMCWayland()
{
  xbmc_wayland_destroy(m_xbmcWayland);
}

void
xtw::XBMCWayland::AddMode(int width,
                          int height,
                          uint32_t refresh,
                          enum wl_output_mode flags)
{
  xbmc_wayland_add_mode(m_xbmcWayland,
                        width,
                        height,
                        refresh,
                        static_cast<uint32_t>(flags));
}

void
xtw::XBMCWayland::MovePointerTo(struct wl_surface *surface,
                                wl_fixed_t x,
                                wl_fixed_t y)
{
  xbmc_wayland_move_pointer_to_on_surface(m_xbmcWayland,
                                          surface,
                                          x,
                                          y);
}

void
xtw::XBMCWayland::SendButtonTo(struct wl_surface *surface,
                               uint32_t button,
                               uint32_t state)
{
  xbmc_wayland_send_button_to_surface(m_xbmcWayland,
                                      surface,
                                      button,
                                      state);
}

void
xtw::XBMCWayland::SendAxisTo(struct wl_surface *surface,
                             uint32_t axis,
                             wl_fixed_t value)
{
  xbmc_wayland_send_axis_to_surface(m_xbmcWayland,
                                    surface,
                                    axis,
                                    value);
}

void
xtw::XBMCWayland::SendKeyToKeyboard(struct wl_surface *surface,
                                    uint32_t key,
                                    enum wl_keyboard_key_state state)
{
  xbmc_wayland_send_key_to_keyboard(m_xbmcWayland,
                                    surface,
                                    key,
                                    state);
}

void
xtw::XBMCWayland::SendModifiersToKeyboard(struct wl_surface *surface,
                                          uint32_t depressed,
                                          uint32_t latched,
                                          uint32_t locked,
                                          uint32_t group)
{
  xbmc_wayland_send_modifiers_to_keyboard(m_xbmcWayland,
                                          surface,
                                          depressed,
                                          latched,
                                          locked,
                                          group);
}

void
xtw::XBMCWayland::GiveSurfaceKeyboardFocus(struct wl_surface *surface)
{
  xbmc_wayland_give_surface_keyboard_focus(m_xbmcWayland,
                                           surface);
}

void
xtw::XBMCWayland::PingSurface(struct wl_surface *surface,
                              uint32_t serial)
{
  xbmc_wayland_ping_surface(m_xbmcWayland, surface, serial);
}

namespace
{
class TempFileWrapper :
  boost::noncopyable
{
public:

  TempFileWrapper(const CStdString &suffix);
  ~TempFileWrapper();
  
  void FetchDirectory(CStdString &directory);
  void FetchFilename(CStdString &name);
private:

  XFILE::CFile *m_file;
};

TempFileWrapper::TempFileWrapper(const CStdString &suffix) :
  m_file(CXBMCTestUtils::Instance().CreateTempFile(suffix))
{
}

TempFileWrapper::~TempFileWrapper()
{
  CXBMCTestUtils::Instance().DeleteTempFile(m_file);
}

void TempFileWrapper::FetchDirectory(CStdString &directory)
{
  directory = CXBMCTestUtils::Instance().TempFileDirectory(m_file);
  /* Strip trailing "/" */
  directory.resize(directory.size() - 1);
}

void TempFileWrapper::FetchFilename(CStdString &name)
{
  CStdString path(CXBMCTestUtils::Instance().TempFilePath(m_file));
  CStdString directory(CXBMCTestUtils::Instance().TempFileDirectory(m_file));
  
  name = path.substr(directory.size());
}

class SavedTempSocket :
  boost::noncopyable
{
public:

  SavedTempSocket();

  const CStdString & FetchFilename();
  const CStdString & FetchDirectory();

private:

  CStdString m_filename;
  CStdString m_directory;
};

SavedTempSocket::SavedTempSocket()
{
  TempFileWrapper wrapper("");
  wrapper.FetchDirectory(m_directory);
  wrapper.FetchFilename(m_filename);
}

const CStdString &
SavedTempSocket::FetchFilename()
{
  return m_filename;
}

const CStdString &
SavedTempSocket::FetchDirectory()
{
  return m_directory;
}

class TmpEnv :
  boost::noncopyable
{
public:

  TmpEnv(const char *env, const char *val);
  ~TmpEnv();

private:

  const char *m_env;
  const char *m_previous;
};

TmpEnv::TmpEnv(const char *env,
               const char *val) :
  m_env(env),
  m_previous(getenv(env))
{
  setenv(env, val, 1);
}

TmpEnv::~TmpEnv()
{
  if (m_previous)
    setenv(m_env, m_previous, 1);
  else
    unsetenv(m_env);
}

std::string
FindBinaryFromPATH(const std::string &binary)
{
  const char *pathEnvironmentCArray = getenv("PATH");
  if (!pathEnvironmentCArray)
    throw std::runtime_error("PATH is not set");

  std::string pathEnvironment(pathEnvironmentCArray);
  
  typedef boost::char_separator<char> CharSeparator;
  typedef boost::tokenizer<CharSeparator> CharTokenizer;
  
  CharTokenizer paths(pathEnvironment,
                      CharSeparator(":"));

  for (CharTokenizer::iterator it = paths.begin();
       it != paths.end();
       ++it)
  {
    std::stringstream possibleBinaryLocationStream;
    possibleBinaryLocationStream << *it
                                 << "/"
                                 << binary;
    std::string possibleBinaryLocation(possibleBinaryLocationStream.str());
    int ok = access(possibleBinaryLocation.c_str(), X_OK);

    if (ok == -1)
    {
      switch (errno)
      {
        case EACCES:
        case ENOENT:
          continue;
        default:
          throw std::runtime_error(strerror(errno));
      }
    }

    return possibleBinaryLocation.c_str();
  }
  
  std::stringstream ss;
  ss << "Unable to find "
     << binary
     << " in PATH as it does not exist or is not executable";
  
  throw std::runtime_error(ss.str());
}
}

xt::Process::Process(const CStdString &xbmcTestBase,
                     const CStdString &tempFileName) :
  m_pid(0)
{
  std::stringstream socketOptionStream;
  socketOptionStream << "--socket=";
  socketOptionStream << tempFileName.c_str();
  
  std::string socketOption(socketOptionStream.str());

  std::stringstream modulesOptionStream;
  modulesOptionStream << "--modules=";
  modulesOptionStream << xbmcTestBase.c_str();
  modulesOptionStream << "xbmc/windowing/tests/wayland/xbmc-wayland-test-extension.so";
  
  std::string modulesOption(modulesOptionStream.str());
  
  std::string program(FindBinaryFromPATH("weston"));
  const char *options[] =
  {
    program.c_str(),
    "--backend=headless-backend.so",
    modulesOption.c_str(),
    socketOption.c_str(),
    NULL
  };
  
  m_pid = fork();
  
  switch (m_pid)
  {
    case 0:
      Child(program.c_str(),
            const_cast <char * const *>(options));
    case -1:
      ForkError();
    default:
      Parent();
  }
}

pid_t
xt::Process::Pid()
{
  return m_pid;
}

void
xt::Process::Child(const char *program,
                   char * const *options)
{
  signal(SIGUSR2, SIG_IGN);
  
  /* Unblock SIGUSR2 */
  sigset_t signalMask;
  sigemptyset(&signalMask);
  sigaddset(&signalMask, SIGUSR2);
  if (sigprocmask(SIG_UNBLOCK, &signalMask, NULL))
  {
    std::stringstream ss;
    ss << "sigprocmask: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }
  
  if (!getenv("XBMC_WESTON_GTEST_CHILD_STDOUT"))
  {
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }
  
  if (execvpe(program, options, environ) == -1)
  {
    std::stringstream ss;
    ss << "execvpe: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }
}

void
xt::Process::Parent()
{
}

void
xt::Process::ForkError()
{
  std::stringstream ss;
  ss << "fork: "
     << strerror(errno);
  throw std::runtime_error(ss.str());
}

void
xt::Process::WaitForSignal(int signal, int timeout)
{
  sigset_t signalMask;
  
  if (timeout >= 0)
  {
    static const uint32_t MsecToNsec = 1000000;
    static const uint32_t SecToMsec = 1000;
    int seconds = timeout / SecToMsec;
    
    /* Remove seconds from timeout */
    timeout -= seconds * SecToMsec;
    struct timespec ts = { seconds, timeout * MsecToNsec };
    
    sigemptyset(&signalMask);
    sigaddset(&signalMask, signal);
    int received = 0;
    
    do
    {
      errno = 0;
      received = sigtimedwait(&signalMask,
                              NULL,
                              &ts);
      if (received == -1)
      {
        /* Just retry if we got signalled */
        if (errno != EINTR)
        {
          std::stringstream ss;
          ss << "sigtimedwait: "
             << strerror(errno);

          throw std::runtime_error(ss.str());
        }
      }
    } while (errno != 0);
    
    return;
  }
  else
  {
    sigemptyset(&signalMask);
    sigaddset(&signalMask, signal);
    errno = 0;
    int received = sigwaitinfo(&signalMask, NULL);
    
    if (received != signal)
    {
      std::stringstream ss;
      ss << "sigwaitinfo: "
         << strerror(errno);
    }
  }
}

namespace
{
void
WestonMisbehaviourMessage(std::stringstream &ss)
{
  ss << std::endl;
  ss << "It is possible that Weston is just shutting down uncleanly "
     << " - you should check the stacktrace and run with "
     << " ALLOW_WESTON_MISBEHAVIOUR set to suppress this";
}

bool
NoMisbehaviour()
{
  return !getenv("ALLOW_WESTON_MISBEHAVIOUR");
}

class StatusWaitTimeoutError :
  public std::exception
{
public:

  StatusWaitTimeoutError(int expected, int timeout);
  ~StatusWaitTimeoutError() throw() {}

private:

  const char * what() const throw();

  int m_expected;
  int m_timeout;
  
  mutable std::string m_what;
};

class TerminatedBySignalError :
  public std::exception
{
public:

  TerminatedBySignalError(int expected, int signal);
  ~TerminatedBySignalError() throw() {}

private:

  const char * what() const throw();

  int m_expected;
  int m_signal;
  
  mutable std::string m_what;
};

class AbnormalExitStatusError :
  public std::exception
{
public:

  AbnormalExitStatusError(int expected, int status);
  ~AbnormalExitStatusError() throw() {}

private:

  const char * what() const throw();

  int m_expected;
  int m_status;
  
  mutable std::string m_what;
};

StatusWaitTimeoutError::StatusWaitTimeoutError(int expected,
                                               int timeout) :
  m_expected(expected),
  m_timeout(timeout)
{
}

const char *
StatusWaitTimeoutError::what() const throw()
{
  std::stringstream ss;
  ss << "Expected exit status "
     << m_expected
     << " within "
     << m_timeout
     << " ms";
  m_what = ss.str();
  return m_what.c_str();
}

TerminatedBySignalError::TerminatedBySignalError(int expected,
                                                 int signal) :
  m_expected(expected),
  m_signal(signal)
{
}

const char *
TerminatedBySignalError::what() const throw()
{
  std::stringstream ss;
  ss << "Expected exit status "
     << m_expected
     << " but was instead terminated by signal "
     << m_signal
     << " - "
     << strsignal(m_signal);
  WestonMisbehaviourMessage(ss);
  m_what = ss.str();
  return m_what.c_str();
}

AbnormalExitStatusError::AbnormalExitStatusError(int expected,
                                                 int status) :
  m_expected(expected),
  m_status(status)
{
}

const char *
AbnormalExitStatusError::what() const throw()
{
  std::stringstream ss;
  ss << "Expected exit status "
     << m_expected
     << " but instead exited with "
     << m_status
     << " - "
     << strsignal(m_status);
  WestonMisbehaviourMessage(ss);
  m_what = ss.str();
  return m_what.c_str();
}
}

void
xt::Process::WaitForStatus(int code, int timeout)
{
  struct timespec startTime;
  struct timespec currentTime;
  clock_gettime(CLOCK_MONOTONIC, &startTime);
  clock_gettime(CLOCK_MONOTONIC, &currentTime);
  
  const uint32_t SecToMsec = 1000;
  const uint32_t MsecToNsec = 1000000;
  
  int32_t startTimestamp = startTime.tv_sec * SecToMsec +
                           startTime.tv_nsec / MsecToNsec;
  int32_t currentTimestamp = currentTime.tv_sec * SecToMsec +
                             currentTime.tv_nsec / MsecToNsec;
  
  int options = WUNTRACED;
  
  std::stringstream statusMessage;
  
  if (timeout >= 0)
    options |= WNOHANG;
  
  do
  {
    clock_gettime(CLOCK_MONOTONIC, &currentTime);
    
    currentTimestamp = currentTime.tv_sec * SecToMsec +
                       currentTime.tv_nsec / MsecToNsec;
    
    int returnedStatus;
    pid_t pid = waitpid(m_pid, &returnedStatus, options);
    
    if (pid == m_pid)
    {
      /* At least one child has exited */
      if (WIFEXITED(returnedStatus))
      {
        int returnedExitCode = WEXITSTATUS(returnedStatus);
        if (returnedExitCode == code)
          return;
        
        /* Abnormal exit status */
        throw AbnormalExitStatusError(code, returnedExitCode);
      }
      else if (WIFSIGNALED(returnedStatus))
      {
        int returnedSignal = WTERMSIG(returnedStatus);

        /* Signaled and died */
        throw TerminatedBySignalError(code, returnedSignal);
      }
    }
    else if (pid == -1)
    {
      std::stringstream ss;
      ss << "waitpid failed: "
         << strerror(errno);
      throw std::runtime_error(ss.str());
    }
    else if (!pid)
    {
      struct timespec ts;
      ts.tv_sec = 0;
      
      /* Don't sleep the whole time, we might have just missed
       * the signal */
      ts.tv_nsec = timeout * MsecToNsec / 10;
      
      nanosleep(&ts, NULL);
    }
  }
  while (timeout == -1 ||
         (timeout > currentTimestamp - startTimestamp));

  /* If we didn't get out early, it means we timed out */
  throw StatusWaitTimeoutError(code, timeout);
}

void
xt::Process::SendSignal(int signal)
{
  if (kill(m_pid, signal) == -1)
  {
    /* Already dead ... lets see if it exited normally */
    if (errno == ESRCH)
    {
      try
      {
        WaitForStatus(0, 0);
      }
      catch (std::runtime_error &err)
      {
        std::stringstream ss;
        ss << err.what()
           << " - process was already dead"
           << std::endl;
        throw std::runtime_error(ss.str());
      }
    }
    else
    {
      std::stringstream ss;
      ss << "failed to send signal "
         << signal
         << " to process "
         << m_pid
         << ": " << strerror(errno);
      throw std::runtime_error(ss.str());
    }
  }
}

void
xt::Process::Interrupt()
{
  SendSignal(SIGINT);
}

void
xt::Process::Terminate()
{
  SendSignal(SIGTERM);
}

void
xt::Process::Kill()
{
  SendSignal(SIGKILL);
}

xt::Process::~Process()
{
  typedef void (Process::*SignalAction)(void);
  SignalAction deathActions[] =
  {
    &Process::Interrupt,
    &Process::Terminate,
    &Process::Kill
  };

  static const size_t deathActionsSize = sizeof(deathActions) /
                                         sizeof(deathActions[0]);

  size_t i = 0;
  
  std::stringstream processStatusMessages;

  for (; i < deathActionsSize; ++i)
  {
    try
    {
      SignalAction func(deathActions[i]);
      ((*this).*(func))();
      WaitForStatus(0, DefaultProcessWaitTimeout);
      break;
    }
    catch (const TerminatedBySignalError &err)
    {
      if (NoMisbehaviour())
        throw;
      else
        break;
    }
    catch (const AbnormalExitStatusError &err)
    {
      if (NoMisbehaviour())
        throw;
      else
        break;
    }
    catch (const StatusWaitTimeoutError &err)
    {
      processStatusMessages << "[TIMEOUT] "
                            << static_cast<const std::exception &>(err).what()
                            << std::endl;
    }
  }
  
  if (i == deathActionsSize)
  {
    std::stringstream ss;
    ss << "Failed to terminate "
       << m_pid
       << std::endl;
    ss << processStatusMessages;
    throw std::runtime_error(ss.str());
  }
}

namespace
{
template <typename Iterator>
class SignalGuard :
  boost::noncopyable
{
public:

  SignalGuard(const Iterator &begin,
              const Iterator &end);
  ~SignalGuard();
private:

  sigset_t mask;
};

template <typename Iterator>
SignalGuard<Iterator>::SignalGuard(const Iterator &begin,
                                   const Iterator &end)
{
  sigemptyset(&mask);
  for (Iterator it = begin;
       it != end;
       ++it)
    sigaddset(&mask, *it);

  if (sigprocmask(SIG_BLOCK, &mask, NULL))
  {
    std::stringstream ss;
    ss << "sigprogmask: "
       << strerror(errno);
    throw std::runtime_error(ss.str());
  }
}

template <typename Iterator>
SignalGuard<Iterator>::~SignalGuard()
{
  if (sigprocmask(SIG_UNBLOCK, &mask, NULL))
    CLog::Log(LOGERROR, "Failed to unblock signals");
}

typedef boost::array<int, 4> SigArray;
SigArray BlockedSignals =
{
  {
    SIGUSR2,
    SIGCHLD
  }
};
}

class WestonTest :
  public ::testing::Test
{
public:

  WestonTest();
  ~WestonTest();
  pid_t Pid();
  
  virtual void SetUp();

protected:

  CEGLNativeTypeWayland m_nativeType;
  xw::Display *m_display;
  struct wl_surface *m_mostRecentSurface;
  
  CStdString m_xbmcTestBase;
  SavedTempSocket m_tempSocketName;
  TmpEnv m_xdgRuntimeDir;
  
private:

  void DisplayAvailable(xw::Display &display);
  void SurfaceCreated(xw::Surface &surface);

  SignalGuard<SigArray::iterator> m_signalGuard;
  xt::Process m_process;
};

WestonTest::WestonTest() :
  m_display(NULL),
  m_mostRecentSurface(NULL),
  m_xbmcTestBase(CXBMCTestUtils::Instance().ReferenceFilePath("")),
  /* We want wayland (client and server) to look in our
   * temp file directory for the socket */
  m_xdgRuntimeDir("XDG_RUNTIME_DIR", m_tempSocketName.FetchDirectory().c_str()),
  /* Block emission of SIGUSR2 so that we can wait on it */
  m_signalGuard(BlockedSignals.begin(), BlockedSignals.end()),
  m_process(m_xbmcTestBase,
            m_tempSocketName.FetchFilename())
{
  xw::WaylandDisplayListener &displayListener(xw::WaylandDisplayListener::GetInstance());
  displayListener.SetHandler(boost::bind(&WestonTest::DisplayAvailable,
                                         this, _1));

  xw::WaylandSurfaceListener &surfaceListener(xw::WaylandSurfaceListener::GetInstance());
  surfaceListener.SetHandler(boost::bind(&WestonTest::SurfaceCreated,
                                        this, _1));
}

WestonTest::~WestonTest()
{
  xw::WaylandDisplayListener &displayListener(xw::WaylandDisplayListener::GetInstance());
  displayListener.SetHandler(xw::WaylandDisplayListener::Handler());
  
  xw::WaylandSurfaceListener &surfaceListener(xw::WaylandSurfaceListener::GetInstance());
  surfaceListener.SetHandler(xw::WaylandSurfaceListener::Handler());
}

pid_t
WestonTest::Pid()
{
  return m_process.Pid();
}

void
WestonTest::SetUp()
{
  m_process.WaitForSignal(SIGUSR2, DefaultProcessWaitTimeout);
}

void
WestonTest::DisplayAvailable(xw::Display &display)
{
  m_display = &display;
}

void
WestonTest::SurfaceCreated(xw::Surface &surface)
{
  m_mostRecentSurface = surface.GetWlSurface();
}

TEST_F(WestonTest, TestCheckCompatibilityWithEnvSet)
{
  TmpEnv env("WAYLAND_DISPLAY", m_tempSocketName.FetchFilename().c_str());
  EXPECT_TRUE(m_nativeType.CheckCompatibility());
}

TEST_F(WestonTest, TestCheckCompatibilityWithEnvNotSet)
{
  EXPECT_FALSE(m_nativeType.CheckCompatibility());
}

class CompatibleWestonTest :
  public WestonTest
{
public:

  CompatibleWestonTest();
  virtual void SetUp();
  
private:

  TmpEnv m_waylandDisplayEnv;
};

CompatibleWestonTest::CompatibleWestonTest() :
  m_waylandDisplayEnv("WAYLAND_DISPLAY",
                      m_tempSocketName.FetchFilename().c_str())
{
}

void
CompatibleWestonTest::SetUp()
{
  WestonTest::SetUp();
  ASSERT_TRUE(m_nativeType.CheckCompatibility());
}

TEST_F(CompatibleWestonTest, TestConnection)
{
  EXPECT_TRUE(m_nativeType.CreateNativeDisplay());
}

class ConnectedWestonTest :
  public CompatibleWestonTest
{
public:

  ConnectedWestonTest();
  ~ConnectedWestonTest();
  virtual void SetUp();

protected:

  boost::scoped_ptr<xtw::XBMCWayland> m_xbmcWayland;

private:

  void Global(struct wl_registry *, uint32_t, const char *, uint32_t);
};

ConnectedWestonTest::ConnectedWestonTest()
{
  xw::ExtraWaylandGlobals &extra(xw::ExtraWaylandGlobals::GetInstance());
  extra.SetHandler(boost::bind(&ConnectedWestonTest::Global,
                               this, _1, _2, _3, _4));
}

ConnectedWestonTest::~ConnectedWestonTest()
{
  xw::ExtraWaylandGlobals &extra(xw::ExtraWaylandGlobals::GetInstance());
  extra.SetHandler(xw::ExtraWaylandGlobals::GlobalHandler());
}

void
ConnectedWestonTest::SetUp()
{
  CompatibleWestonTest::SetUp();
  ASSERT_TRUE(m_nativeType.CreateNativeDisplay());
}

void
ConnectedWestonTest::Global(struct wl_registry *registry,
                            uint32_t name,
                            const char *interface,
                            uint32_t version)
{
  if (std::string(interface) == "xbmc_wayland")
    m_xbmcWayland.reset(new xtw::XBMCWayland(static_cast<xbmc_wayland *>(wl_registry_bind(registry,
                                                                                          name,
                                                                                          &xbmc_wayland_interface,
                                                                                          version))));
}

TEST_F(ConnectedWestonTest, TestGotXBMCWayland)
{
  EXPECT_TRUE(m_xbmcWayland.get() != NULL);
}

TEST_F(ConnectedWestonTest, CreateNativeWindowSuccess)
{
  EXPECT_TRUE(m_nativeType.CreateNativeWindow());
}

TEST_F(ConnectedWestonTest, ProbeResolutionsSuccess)
{
  std::vector<RESOLUTION_INFO> info;
  EXPECT_TRUE(m_nativeType.ProbeResolutions(info));
}

TEST_F(ConnectedWestonTest, PreferredResolutionSuccess)
{
  RESOLUTION_INFO info;
  EXPECT_TRUE(m_nativeType.GetPreferredResolution(&info));
}

TEST_F(ConnectedWestonTest, CurrentNativeSuccess)
{
  RESOLUTION_INFO info;
  EXPECT_TRUE(m_nativeType.GetNativeResolution(&info));
}

TEST_F(ConnectedWestonTest, GetMostRecentSurface)
{
  m_nativeType.CreateNativeWindow();
  EXPECT_TRUE(m_mostRecentSurface != NULL);
}

class XBMCWaylandAssistedWestonTest :
  public ConnectedWestonTest
{
protected:

  virtual void SetUp();
};

void
XBMCWaylandAssistedWestonTest::SetUp()
{
  ConnectedWestonTest::SetUp();
  ASSERT_TRUE(m_xbmcWayland.get());
}

TEST_F(XBMCWaylandAssistedWestonTest, AdditionalResolutions)
{
  m_xbmcWayland->AddMode(2, 2, 2, static_cast<enum wl_output_mode>(0));
  std::vector<RESOLUTION_INFO> resolutions;
  m_nativeType.ProbeResolutions(resolutions);
  EXPECT_TRUE(resolutions.size() == 2);
}

TEST_F(XBMCWaylandAssistedWestonTest, PreferredResolutionChange)
{
  m_xbmcWayland->AddMode(2, 2, 2, static_cast<enum wl_output_mode>(WL_OUTPUT_MODE_PREFERRED));
  RESOLUTION_INFO res;
  m_nativeType.GetPreferredResolution(&res);
  EXPECT_EQ(res.iWidth, 2);
  EXPECT_EQ(res.iHeight, 2);
}

TEST_F(XBMCWaylandAssistedWestonTest, CurrentResolutionChange)
{
  m_xbmcWayland->AddMode(2, 2, 2, static_cast<enum wl_output_mode>(WL_OUTPUT_MODE_CURRENT));
  RESOLUTION_INFO res;
  m_nativeType.GetNativeResolution(&res);
  EXPECT_EQ(res.iWidth, 2);
  EXPECT_EQ(res.iHeight, 2);
}

namespace
{
class StubEventListener :
  public xbmc::IEventListener
{
public:

  StubEventListener();

  /* Returns front of event queue, otherwise throws */
  XBMC_Event FetchLastEvent();
  bool Focused();

private:

  void OnFocused();
  void OnUnfocused();
  void OnEvent(XBMC_Event &);

  bool m_focused;
  std::queue<XBMC_Event> m_events;
};

StubEventListener::StubEventListener() :
  m_focused(false)
{
}

XBMC_Event
StubEventListener::FetchLastEvent()
{
  if (m_events.empty())
    throw std::logic_error("No events left to get!");
  
  XBMC_Event ev = m_events.front();
  m_events.pop();
  return ev;
}

bool
StubEventListener::Focused()
{
  return m_focused;
}

void
StubEventListener::OnFocused()
{
  m_focused = true;
}

void
StubEventListener::OnUnfocused()
{
  m_focused = false;
}

void
StubEventListener::OnEvent(XBMC_Event &ev)
{
  m_events.push(ev);
}

class StubCursorManager :
  public xbmc::ICursorManager
{
public:

  void SetCursor(uint32_t serial,
                 struct wl_surface *surface,
                 double surfaceX,
                 double surfaceY)
  {
  }
};

class SingleThreadedEventQueue :
  public xwe::IEventQueueStrategy
{
public:

  SingleThreadedEventQueue(IDllWaylandClient &clientLibrary,
                           struct wl_display *display);

private:

  void PushAction(const Action &action);
  void DispatchEventsFromMain();
  
  IDllWaylandClient &m_clientLibrary;
  struct wl_display *m_display;
};

SingleThreadedEventQueue::SingleThreadedEventQueue(IDllWaylandClient &clientLibrary,
                                                   struct wl_display *display) :
  m_clientLibrary(clientLibrary),
  m_display(display)
{
}

void SingleThreadedEventQueue::PushAction(const Action &action)
{
  action();
}

void SingleThreadedEventQueue::DispatchEventsFromMain()
{
  m_clientLibrary.wl_display_dispatch_pending(m_display);
  m_clientLibrary.wl_display_flush(m_display);
  m_clientLibrary.wl_display_dispatch(m_display);
}
}

class InputEventsWestonTest :
  public WestonTest,
  public xw::IWaylandRegistration
{
public:

  InputEventsWestonTest();
  virtual void SetUp();

protected:

  DllWaylandClient clientLibrary;
  DllWaylandEGL eglLibrary;
  DllXKBCommon xkbCommonLibrary;
  
  StubCursorManager cursors;
  StubEventListener listener;

  boost::shared_ptr<struct xkb_context> xkbContext;
  boost::scoped_ptr<CXKBKeymap> keymap;

  boost::scoped_ptr<xw::Display> display;
  boost::scoped_ptr<xwe::IEventQueueStrategy> queue;
  boost::scoped_ptr<xw::Registry> registry;

  boost::scoped_ptr<xwe::Loop> loop;
  boost::scoped_ptr<xbmc::InputFactory> input;

  boost::scoped_ptr<xw::Compositor> compositor;
  boost::scoped_ptr<xw::Shell> shell;
  boost::scoped_ptr<xtw::XBMCWayland> xbmcWayland;

  boost::scoped_ptr<xw::Surface> surface;
  boost::scoped_ptr<xw::ShellSurface> shellSurface;
  boost::scoped_ptr<xw::OpenGLSurface> openGLSurface;

  virtual xwe::IEventQueueStrategy * CreateEventQueue() = 0;

  void WaitForSynchronize();
  
  static const unsigned int SurfaceWidth = 512;
  static const unsigned int SurfaceHeight = 512;

private:

  virtual bool OnGlobalInterfaceAvailable(uint32_t name,
                                          const char *interface,
                                          uint32_t version);

  bool synchronized;
  void Synchronize();
  boost::scoped_ptr<xw::Callback> syncCallback;
  
  TmpEnv m_waylandDisplayEnv;
};

InputEventsWestonTest::InputEventsWestonTest() :
  m_waylandDisplayEnv("WAYLAND_DISPLAY",
                      m_tempSocketName.FetchFilename().c_str())
{
}

void InputEventsWestonTest::SetUp()
{
  WestonTest::SetUp();
  
  clientLibrary.Load();
  eglLibrary.Load();
  xkbCommonLibrary.Load();
  
  xkbContext.reset(CXKBKeymap::CreateXKBContext(xkbCommonLibrary),
                   boost::bind(&IDllXKBCommon::xkb_context_unref,
                               &xkbCommonLibrary, _1));
  keymap.reset(new CXKBKeymap(
                 xkbCommonLibrary, 
                 CXKBKeymap::CreateXKBKeymapFromNames(xkbCommonLibrary,
                                                      xkbContext.get(),
                                                      "evdev",
                                                      "pc105",
                                                      "us",
                                                      "",
                                                      "")));
  
  display.reset(new xw::Display(clientLibrary));
  queue.reset(CreateEventQueue());
  registry.reset(new xw::Registry(clientLibrary,
                                  display->GetWlDisplay(),
                                  *this));
  loop.reset(new xwe::Loop(listener, *queue));

  /* Wait for the seat, shell, compositor to appear */
  WaitForSynchronize();
  
  ASSERT_TRUE(input.get() != NULL);
  ASSERT_TRUE(compositor.get() != NULL);
  ASSERT_TRUE(shell.get() != NULL);
  ASSERT_TRUE(xbmcWayland.get() != NULL);
  
  /* Wait for input devices to appear etc */
  WaitForSynchronize();
  
  surface.reset(new xw::Surface(clientLibrary,
                                compositor->CreateSurface()));
  shellSurface.reset(new xw::ShellSurface(clientLibrary,
                                          shell->CreateShellSurface(
                                            surface->GetWlSurface())));
  openGLSurface.reset(new xw::OpenGLSurface(eglLibrary,
                                            surface->GetWlSurface(),
                                            SurfaceWidth,
                                            SurfaceHeight));

  wl_shell_surface_set_toplevel(shellSurface->GetWlShellSurface());
  surface->Commit();
}

bool InputEventsWestonTest::OnGlobalInterfaceAvailable(uint32_t name,
                                                       const char *interface,
                                                       uint32_t version)
{
  if (strcmp(interface, "wl_seat") == 0)
  {
    /* We must use the one provided by dlopen, as the address
     * may be different */
    struct wl_interface **seatInterface =
      clientLibrary.Get_wl_seat_interface();
    struct wl_seat *seat =
      registry->Bind<struct wl_seat *>(name,
                                       seatInterface,
                                       1);
    input.reset(new xbmc::InputFactory(clientLibrary,
                                       xkbCommonLibrary,
                                       seat,
                                       listener,
                                       *loop));
    return true;
  }
  else if (strcmp(interface, "wl_compositor") == 0)
  {
    struct wl_interface **compositorInterface =
      clientLibrary.Get_wl_compositor_interface();
    struct wl_compositor *wlcompositor =
      registry->Bind<struct wl_compositor *>(name,
                                             compositorInterface,
                                             1);
    compositor.reset(new xw::Compositor(clientLibrary, wlcompositor));
    return true;
  }
  else if (strcmp(interface, "wl_shell") == 0)
  {
    struct wl_interface **shellInterface =
      clientLibrary.Get_wl_shell_interface();
    struct wl_shell *wlshell =
      registry->Bind<struct wl_shell *>(name,
                                        shellInterface,
                                        1);
    shell.reset(new xw::Shell(clientLibrary, wlshell));
    return true;
  }
  else if (strcmp(interface, "xbmc_wayland") == 0)
  {
    struct wl_interface **xbmcWaylandInterface =
      (struct wl_interface **) &xbmc_wayland_interface;
    struct xbmc_wayland *wlxbmc_wayland =
      registry->Bind<struct xbmc_wayland *>(name,
                                            xbmcWaylandInterface,
                                            version);
    xbmcWayland.reset(new xtw::XBMCWayland(wlxbmc_wayland));
    return true;
  }
  
  return false;
}

void InputEventsWestonTest::WaitForSynchronize()
{
  synchronized = false;
  syncCallback.reset(new xw::Callback(clientLibrary,
                                      display->Sync(),
                                      boost::bind(&InputEventsWestonTest::Synchronize,
                                                  this)));
  
  while (!synchronized)
    loop->Dispatch();
}

void InputEventsWestonTest::Synchronize()
{
  synchronized = true;
}

template <typename EventQueue>
class InputEventQueueWestonTest :
  public InputEventsWestonTest
{
private:

  virtual xwe::IEventQueueStrategy * CreateEventQueue()
  {
    return new EventQueue(clientLibrary, display->GetWlDisplay());
  }
};
TYPED_TEST_CASE_P(InputEventQueueWestonTest);

TYPED_TEST_P(InputEventQueueWestonTest, Construction)
{
}

TYPED_TEST_P(InputEventQueueWestonTest, MotionEvent)
{
  typedef InputEventsWestonTest Base;
  int x = Base::SurfaceWidth / 2;
  int y = Base::SurfaceHeight / 2;
  Base::xbmcWayland->MovePointerTo(Base::surface->GetWlSurface(),
                                   wl_fixed_from_int(x),
                                   wl_fixed_from_int(y));
  Base::WaitForSynchronize();
  XBMC_Event event(Base::listener.FetchLastEvent());

  EXPECT_EQ(XBMC_MOUSEMOTION, event.type);
  EXPECT_EQ(x, event.motion.xrel);
  EXPECT_EQ(y, event.motion.yrel);
}

TYPED_TEST_P(InputEventQueueWestonTest, ButtonEvent)
{
  typedef InputEventsWestonTest Base;
  int x = Base::SurfaceWidth / 2;
  int y = Base::SurfaceHeight / 2;
  const unsigned int WaylandLeftButton = 272;
  
  Base::xbmcWayland->MovePointerTo(Base::surface->GetWlSurface(),
                                   wl_fixed_from_int(x),
                                   wl_fixed_from_int(y));
  Base::xbmcWayland->SendButtonTo(Base::surface->GetWlSurface(),
                                  WaylandLeftButton,
                                  WL_POINTER_BUTTON_STATE_PRESSED);
  Base::WaitForSynchronize();
  
  /* Throw away motion event */
  Base::listener.FetchLastEvent();

  XBMC_Event event(Base::listener.FetchLastEvent());

  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(1, event.button.button);
  EXPECT_EQ(x, event.button.x);
  EXPECT_EQ(y, event.button.y);
}

TYPED_TEST_P(InputEventQueueWestonTest, AxisEvent)
{
  typedef InputEventsWestonTest Base;
  int x = Base::SurfaceWidth / 2;
  int y = Base::SurfaceHeight / 2;
  
  Base::xbmcWayland->MovePointerTo(Base::surface->GetWlSurface(),
                                   wl_fixed_from_int(x),
                                   wl_fixed_from_int(y));
  Base::xbmcWayland->SendAxisTo(Base::surface->GetWlSurface(),
                                WL_POINTER_AXIS_VERTICAL_SCROLL,
                                wl_fixed_from_int(10));
  Base::WaitForSynchronize();
  
  /* Throw away motion event */
  Base::listener.FetchLastEvent();

  /* Should get button up and down */
  XBMC_Event event(Base::listener.FetchLastEvent());

  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(5, event.button.button);
  EXPECT_EQ(x, event.button.x);
  EXPECT_EQ(y, event.button.y);
  
  event = Base::listener.FetchLastEvent();

  EXPECT_EQ(XBMC_MOUSEBUTTONUP, event.type);
  EXPECT_EQ(5, event.button.button);
  EXPECT_EQ(x, event.button.x);
  EXPECT_EQ(y, event.button.y);
}

namespace
{
/* Brute-force lookup functions to compensate for the fact that
 * Keymap interface only supports conversion from scancodes
 * to keysyms and not vice-versa (as such is not implemented in
 * xkbcommon)
 */
uint32_t LookupKeycodeForKeysym(ILinuxKeymap &keymap,
                                XBMCKey sym)
{
  uint32_t code = 0;
  
  while (code < XKB_KEYCODE_MAX)
  {
    /* Supress exceptions from unsupported keycodes */
    try
    {
      if (keymap.XBMCKeysymForKeycode(code) == sym)
        return code;
    }
    catch (std::runtime_error &err)
    {
    }
    
    ++code;
  }
  
  throw std::logic_error("Keysym has no corresponding keycode");
}

uint32_t LookupModifierIndexForModifier(ILinuxKeymap &keymap,
                                        XBMCMod modifier)
{
  uint32_t maxIndex = std::numeric_limits<uint32_t>::max();
  uint32_t index = 0;
  
  while (index < maxIndex)
  {
    keymap.UpdateMask(1 << index, 0, 0, 0);
    XBMCMod mask = static_cast<XBMCMod>(keymap.ActiveXBMCModifiers());
    keymap.UpdateMask(0, 0, 0, 0);
    if (mask & modifier)
      return index;
    
    ++index;
  }
  
  throw std::logic_error("Modifier has no corresponding keymod index");
}
}

TYPED_TEST_P(InputEventQueueWestonTest, KeyEvent)
{
  typedef InputEventsWestonTest Base;
  
  const unsigned int oKeycode = LookupKeycodeForKeysym(*Base::keymap,
                                                       XBMCK_o);

  Base::xbmcWayland->GiveSurfaceKeyboardFocus(Base::surface->GetWlSurface());
  Base::xbmcWayland->SendKeyToKeyboard(Base::surface->GetWlSurface(),
                                       oKeycode,
                                       WL_KEYBOARD_KEY_STATE_PRESSED);
  Base::WaitForSynchronize();
  
  XBMC_Event event(Base::listener.FetchLastEvent());
  EXPECT_EQ(XBMC_KEYDOWN, event.type);
  EXPECT_EQ(oKeycode, event.key.keysym.scancode);
  EXPECT_EQ(XBMCK_o, event.key.keysym.sym);
  EXPECT_EQ(XBMCK_o, event.key.keysym.unicode);
}

TYPED_TEST_P(InputEventQueueWestonTest, Modifiers)
{
  typedef InputEventsWestonTest Base;
  
  const unsigned int oKeycode = LookupKeycodeForKeysym(*Base::keymap,
                                                       XBMCK_o);
  const unsigned int leftShiftIndex =
    LookupModifierIndexForModifier(*Base::keymap, XBMCKMOD_LSHIFT);

  Base::xbmcWayland->GiveSurfaceKeyboardFocus(Base::surface->GetWlSurface());
  Base::xbmcWayland->SendModifiersToKeyboard(Base::surface->GetWlSurface(),
                                             1 << leftShiftIndex,
                                             0,
                                             0,
                                             0);
  Base::xbmcWayland->SendKeyToKeyboard(Base::surface->GetWlSurface(),
                                       oKeycode,
                                       WL_KEYBOARD_KEY_STATE_PRESSED);
  Base::WaitForSynchronize();
  
  XBMC_Event event(Base::listener.FetchLastEvent());
  EXPECT_EQ(XBMC_KEYDOWN, event.type);
  EXPECT_EQ(oKeycode, event.key.keysym.scancode);
  EXPECT_TRUE((XBMCKMOD_LSHIFT & event.key.keysym.mod) != 0);
}

REGISTER_TYPED_TEST_CASE_P(InputEventQueueWestonTest,
                           Construction,
                           MotionEvent,
                           ButtonEvent,
                           AxisEvent,
                           KeyEvent,
                           Modifiers);

typedef ::testing::Types<SingleThreadedEventQueue,
#if (WAYLAND_VERSION >= WAYLAND_VERSION_CHECK(1, 1, 90))
                         xw::version_11::EventQueueStrategy,
                         xw::version_12::EventQueueStrategy> EventQueueTypes;
#else
                         xw::version_11::EventQueueStrategy> EventQueueTypes;
#endif

INSTANTIATE_TYPED_TEST_CASE_P(EventQueues,
                              InputEventQueueWestonTest,
                              EventQueueTypes);

class WaylandPointerProcessor :
  public ::testing::Test
{
public:

  WaylandPointerProcessor();

protected:

  StubCursorManager cursorManager;
  StubEventListener listener;
  xbmc::PointerProcessor processor;
};

WaylandPointerProcessor::WaylandPointerProcessor() :
  processor(listener, cursorManager)
{
}

class WaylandPointerProcessorButtons :
  public WaylandPointerProcessor,
  public ::testing::WithParamInterface<std::tr1::tuple<uint32_t, uint32_t> >
{
protected:

  uint32_t WaylandButton();
  uint32_t XBMCButton();
};

uint32_t WaylandPointerProcessorButtons::WaylandButton()
{
  return std::tr1::get<0>(GetParam());
}

uint32_t WaylandPointerProcessorButtons::XBMCButton()
{
  return std::tr1::get<1>(GetParam());
}

TEST_P(WaylandPointerProcessorButtons, ButtonPress)
{
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Button(0, 0, WaylandButton(), WL_POINTER_BUTTON_STATE_PRESSED);
  
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(XBMCButton(), event.button.button);
}

TEST_P(WaylandPointerProcessorButtons, ButtonRelease)
{
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Button(0, 0, WaylandButton(), WL_POINTER_BUTTON_STATE_RELEASED);
  
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEBUTTONUP, event.type);
  EXPECT_EQ(XBMCButton(), event.button.button);
}

INSTANTIATE_TEST_CASE_P(ThreeButtonMouse,
                        WaylandPointerProcessorButtons,
                        Values(std::tr1::tuple<uint32_t, uint32_t>(272, 1),
                               std::tr1::tuple<uint32_t, uint32_t>(274, 2),
                               std::tr1::tuple<uint32_t, uint32_t>(273, 3)));

class WaylandPointerProcessorAxisButtons :
  public WaylandPointerProcessor,
  public ::testing::WithParamInterface<std::tr1::tuple<float, uint32_t> >
{
protected:

  float Magnitude();
  uint32_t XBMCButton();
};

float WaylandPointerProcessorAxisButtons::Magnitude()
{
  return std::tr1::get<0>(GetParam());
}

uint32_t WaylandPointerProcessorAxisButtons::XBMCButton()
{
  return std::tr1::get<1>(GetParam());
}

TEST_P(WaylandPointerProcessorAxisButtons, Axis)
{
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Axis(0, WL_POINTER_AXIS_VERTICAL_SCROLL, Magnitude());
  
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(XBMCButton(), event.button.button);
  
  event = listener.FetchLastEvent();
  EXPECT_EQ(XBMC_MOUSEBUTTONUP, event.type);
  EXPECT_EQ(XBMCButton(), event.button.button);
}

INSTANTIATE_TEST_CASE_P(VerticalScrollWheel,
                        WaylandPointerProcessorAxisButtons,
                        Values(std::tr1::tuple<float, uint32_t>(-1.0, 4),
                               std::tr1::tuple<float, uint32_t>(1.0, 5)));

TEST_F(WaylandPointerProcessor, Motion)
{
  const float x = 5.0;
  const float y = 5.0;
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Motion(0, x, y);
  
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEMOTION, event.type);
  EXPECT_EQ(::round(x), event.motion.xrel);
  EXPECT_EQ(::round(y), event.motion.yrel);  
}

TEST_F(WaylandPointerProcessor, MotionThenButton)
{
  const float x = 5.0;
  const float y = 5.0;
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Motion(0, x, y);
  receiver.Button(0, 0, 272, WL_POINTER_BUTTON_STATE_PRESSED);
  
  listener.FetchLastEvent();
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(::round(x), event.button.y);
  EXPECT_EQ(::round(y), event.button.x);  
}
#endif
