/*
 *	Lame time routines include file
 *
 *	Copyright (c) 2000 Mark Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LAME_LAMETIME_H
#define LAME_LAMETIME_H

#include <sys/types.h>
#include "lame.h"

extern double GetCPUTime(void);
extern double GetRealTime(void);

extern int lame_set_stream_binary_mode(FILE * const fp);
extern off_t lame_get_file_size(const char *const filename);

#endif /* LAME_LAMETIME_H */
