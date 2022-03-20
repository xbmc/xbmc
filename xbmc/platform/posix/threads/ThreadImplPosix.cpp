/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ThreadImplPosix.h"

#include "utils/log.h"

#include <pthread.h>

std::unique_ptr<IThreadImpl> IThreadImpl::CreateThreadImpl(std::thread::native_handle_type handle)
{
  return std::make_unique<CThreadImplPosix>(handle);
}

CThreadImplPosix::CThreadImplPosix(std::thread::native_handle_type handle) : IThreadImpl(handle)
{
}

void CThreadImplPosix::SetThreadInfo(const std::string& name)
{
#if defined(TARGET_DARWIN)
  pthread_setname_np(name.c_str());
#endif
}

bool CThreadImplPosix::SetPriority(const ThreadPriority& priority)
{
  CLog::Log(LOGDEBUG, "[threads] setting priority is not supported on this platform");
  return false;
}
