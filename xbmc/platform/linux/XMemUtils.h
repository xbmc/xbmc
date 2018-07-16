/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/linux/PlatformDefs.h"

// aligned memory allocation and free. memory returned will be aligned to "alignTo" bytes.
// this is a linux (actually platform free) implementation of the win32 CRT methods _aligned_malloc and _aligned_free.
void *_aligned_malloc(size_t s, size_t alignTo);
void _aligned_free(void *p) ;

void GlobalMemoryStatusEx(LPMEMORYSTATUSEX lpBuffer);

