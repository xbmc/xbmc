/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SharedMemory.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#if defined(HAVE_LINUX_MEMFD)
#include <linux/memfd.h>
#include <sys/syscall.h>
#endif

#include <cerrno>
#include <cstdlib>
#include <system_error>

#include "utils/log.h"

using namespace KODI::UTILS::POSIX;

CSharedMemory::CSharedMemory(std::size_t size)
: m_size{size}, m_fd{Open()}, m_mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0)
{
}

CFileHandle CSharedMemory::Open()
{
  CFileHandle fd;
  try
  {
    fd = OpenMemfd();
  }
  catch (std::system_error const& error)
  {
    if (error.code() == std::errc::function_not_supported)
    {
      CLog::Log(LOGDEBUG, "Kernel does not support memfd, falling back to plain shm");
      fd = OpenShm();
    }
    else
    {
      throw;
    }
  }

  if (ftruncate(fd, m_size) < 0)
  {
    throw std::system_error(errno, std::generic_category(), "ftruncate");
  }

  return fd;
}

CFileHandle CSharedMemory::OpenMemfd()
{
#if defined(SYS_memfd_create) && defined(HAVE_LINUX_MEMFD)
  // This is specific to Linux >= 3.17, but preferred over shm_create if available
  // because it is race-free
  int fd = syscall(SYS_memfd_create, "kodi", MFD_CLOEXEC);
  if (fd < 0)
  {
    throw std::system_error(errno, std::generic_category(), "memfd_create");
  }

  return CFileHandle(fd);
#else
  throw std::system_error(std::make_error_code(std::errc::function_not_supported), "memfd_create");
#endif
}

CFileHandle CSharedMemory::OpenShm()
{
  char* xdgRuntimeDir = std::getenv("XDG_RUNTIME_DIR");
  if (!xdgRuntimeDir)
  {
    throw std::runtime_error("XDG_RUNTIME_DIR environment variable must be set");
  }

  std::string tmpFilename(xdgRuntimeDir);
  tmpFilename.append("/kodi-shared-XXXXXX");

  int rawFd;
#if defined(HAVE_MKOSTEMP)
  // Opening the file with O_CLOEXEC is preferred since it avoids races where
  // other threads might exec() before setting the CLOEXEC flag
  rawFd = mkostemp(&tmpFilename[0], O_CLOEXEC);
#else
  rawFd = mkstemp(&tmpFilename[0]);
#endif

  if (rawFd < 0)
  {
    throw std::system_error(errno, std::generic_category(), "mkstemp");
  }
  CFileHandle fd(rawFd);

  int flags = fcntl(fd, F_GETFD);
  if (flags < 0)
  {
    throw std::system_error(errno, std::generic_category(), "fcntl F_GETFD");
  }
  // Set FD_CLOEXEC if unset
  if (!(flags & FD_CLOEXEC) && fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
  {
    throw std::system_error(errno, std::generic_category(), "fcntl F_SETFD FD_CLOEXEC");
  }

  unlink(tmpFilename.c_str());

  return fd;
}