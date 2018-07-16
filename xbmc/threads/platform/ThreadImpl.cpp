/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if (defined TARGET_POSIX)
#include "threads/platform/pthreads/ThreadImpl.cpp"
#if defined(TARGET_DARWIN_IOS)
#include "threads/platform/darwin/ThreadSchedImpl.cpp"
#else
#include "threads/platform/linux/ThreadSchedImpl.cpp"
#endif
#elif (defined TARGET_WINDOWS)
#include "threads/platform/win/ThreadImpl.cpp"
#endif

