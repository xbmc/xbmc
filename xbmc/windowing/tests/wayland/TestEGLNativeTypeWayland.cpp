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

#include <sstream>
#include <stdexcept>

#include <iostream>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

#include <gtest/gtest.h>

#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include "xbmc_wayland_test_client_protocol.h"

#include "windowing/egl/wayland/Callback.h"
#include "windowing/egl/wayland/Display.h"
#include "windowing/egl/wayland/Registry.h"
#include "windowing/egl/wayland/Surface.h"
#include "windowing/egl/EGLNativeTypeWayland.h"
#include "windowing/WinEvents.h"

#include "windowing/DllWaylandClient.h"

#include "utils/StringUtils.h"
#include "test/TestUtils.h"

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
  pid_t Pid();
  
  virtual void SetUp();

protected:

  CEGLNativeTypeWayland m_nativeType;
  xw::Display *m_display;
  boost::scoped_ptr<xtw::XBMCWayland> m_xbmcWayland;
  struct wl_surface *m_mostRecentSurface;
  
  CStdString m_xbmcTestBase;
  SavedTempSocket m_tempSocketName;
  TmpEnv m_xdgRuntimeDir;
  
private:

  void Global(struct wl_registry *, uint32_t, const char *, uint32_t);
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
WestonTest::Global(struct wl_registry *registry,
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

#endif
