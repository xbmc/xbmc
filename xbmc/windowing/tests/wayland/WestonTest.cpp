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

#include <boost/array.hpp>

#include <signal.h>

#include "test/TestUtils.h"
#include "utils/log.h"

#include "TmpEnv.h"
#include "WestonProcess.h"
#include "WestonTest.h"

namespace xt = xbmc::test;

namespace
{
class TempFileWrapper :
  boost::noncopyable
{
public:

  TempFileWrapper(const westring &suffix);
  ~TempFileWrapper();
  
  void FetchDirectory(westring &directory);
  void FetchFilename(westring &name);
private:

  XFILE::CFile *m_file;
};

TempFileWrapper::TempFileWrapper(const westring &suffix) :
  m_file(CXBMCTestUtils::Instance().CreateTempFile(suffix))
{
}

TempFileWrapper::~TempFileWrapper()
{
  CXBMCTestUtils::Instance().DeleteTempFile(m_file);
}

void TempFileWrapper::FetchDirectory(westring &directory)
{
  directory = CXBMCTestUtils::Instance().TempFileDirectory(m_file);
  /* Strip trailing "/" */
  directory.resize(directory.size() - 1);
}

void TempFileWrapper::FetchFilename(westring &name)
{
  westring path(CXBMCTestUtils::Instance().TempFilePath(m_file));
  westring directory(CXBMCTestUtils::Instance().TempFileDirectory(m_file));
  
  name = path.substr(directory.size());
}

class SavedTempSocket :
  boost::noncopyable
{
public:

  SavedTempSocket();

  const westring & FetchFilename();
  const westring & FetchDirectory();

private:

  westring m_filename;
  westring m_directory;
};

SavedTempSocket::SavedTempSocket()
{
  TempFileWrapper wrapper("");
  wrapper.FetchDirectory(m_directory);
  wrapper.FetchFilename(m_filename);
}

const westring &
SavedTempSocket::FetchFilename()
{
  return m_filename;
}

const westring &
SavedTempSocket::FetchDirectory()
{
  return m_directory;
}

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

class WestonTest::Private
{
public:
  
  Private();
  ~Private();
  
  westring m_xbmcTestBase;
  SavedTempSocket m_tempSocketName;
  TmpEnv m_xdgRuntimeDir;

  SignalGuard<SigArray::iterator> m_signalGuard;

  xt::Process m_process;
};

WestonTest::WestonTest() :
  priv(new Private())
{
}

/* We need a defined destructor, otherwise we will
 * generate the destructors for all of the owned objects
 * multiple times where the definitions for those
 * destructors is unavailable */
WestonTest::~WestonTest()
{
}

WestonTest::Private::Private() :
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

WestonTest::Private::~Private()
{
}

pid_t
WestonTest::Pid()
{
  return priv->m_process.Pid();
}

const westring &
WestonTest::TempSocketName()
{
  return priv->m_tempSocketName.FetchFilename();
}

void
WestonTest::SetUp()
{
  priv->m_process.WaitForSignal(SIGUSR2, xt::Process::DefaultProcessWaitTimeout);
}
