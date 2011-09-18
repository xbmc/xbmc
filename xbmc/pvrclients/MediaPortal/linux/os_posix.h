#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PVRCLIENT_MEDIAPORTAL_OS_POSIX_H
#define PVRCLIENT_MEDIAPORTAL_OS_POSIX_H

#define _FILE_OFFSET_BITS 64

// Success codes
#define S_OK                             0L
#define S_FALSE                          1L
//
// Error codes
#define ERROR_FILENAME_EXCED_RANGE       206L
#define E_OUTOFMEMORY                    0x8007000EL

#define THREAD_FUNC_PREFIX void *

typedef pthread_mutex_t criticalsection_t;
typedef sem_t wait_event_t;

#define PATH_SEPARATOR_CHAR '/'

#endif
