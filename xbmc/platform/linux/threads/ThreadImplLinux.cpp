/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ThreadImplLinux.h"

#include <unistd.h>

#if !defined(TARGET_ANDROID) && (defined(__GLIBC__) || defined(__UCLIBC__))
#if defined(__UCLIBC__) || !__GLIBC_PREREQ(2, 30)
#include <sys/syscall.h>
#endif
#endif

namespace
{

#if !defined(TARGET_ANDROID) && (defined(__GLIBC__) || defined(__UCLIBC__))
#if defined(__UCLIBC__) || !__GLIBC_PREREQ(2, 30)
static pid_t gettid()
{
  return syscall(__NR_gettid);
}
#endif
#endif

} // namespace

std::unique_ptr<IThreadImpl> IThreadImpl::CreateThreadImpl(std::thread::native_handle_type handle)
{
  return std::make_unique<CThreadImplLinux>(handle);
}

CThreadImplLinux::CThreadImplLinux(std::thread::native_handle_type handle)
  : IThreadImpl(handle), m_threadID(gettid())
{
}

void CThreadImplLinux::SetThreadInfo(const std::string& name)
{
#if defined(__GLIBC__)
  pthread_setname_np(m_handle, name.c_str());
#endif
}
