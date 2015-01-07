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
#include <sstream>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/tokenizer.hpp>

#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "Util.h"

#include "WestonProcess.h"

namespace xt = xbmc::test;

namespace
{
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

xt::Process::Process(const westring &xbmcTestBase,
                     const westring &tempFileName) :
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
  ::signal(SIGUSR2, SIG_IGN);
  
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
    ::close(STDOUT_FILENO);
    ::close(STDERR_FILENO);
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



