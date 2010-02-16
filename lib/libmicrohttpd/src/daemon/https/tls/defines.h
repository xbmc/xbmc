/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007 Free Software Foundation
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

#ifndef DEFINES_H
# define DEFINES_H

#ifdef HAVE_CONFIG_H
# include "MHD_config.h"
#endif

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

#ifdef NO_SSIZE_T
# define HAVE_SSIZE_T
typedef int ssize_t;
#endif

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#ifndef MINGW
#include <sys/socket.h>
#endif
#include <time.h>

/* TODO check if these should go into config.h */
#define SIZEOF_UNSIGNED_INT 4
#define SIZEOF_UNSIGNED_LONG 8
#define SIZEOF_UNSIGNED_LONG_INT SIZEOF_UNSIGNED_LONG

/* some systems had problems with long long int, thus,
 * it is not used.
 */
typedef struct
{
  unsigned char i[8];
} uint64;

#endif /* defines_h */
