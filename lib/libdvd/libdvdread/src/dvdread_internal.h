/*
 * This file is part of libdvdread.
 *
 * libdvdread is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdread is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdread; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBDVDREAD_DVDREAD_INTERNAL_H
#define LIBDVDREAD_DVDREAD_INTERNAL_H

#include <stdint.h>
#include <sys/types.h>

#ifdef _WIN32
#include <unistd.h>
#endif /* _WIN32 */

#include "dvdread/dvd_reader.h"

#define CHECK_VALUE(arg) \
 if(!(arg)) { \
   fprintf(stderr, "\n*** libdvdread: CHECK_VALUE failed in %s:%i ***" \
                   "\n*** for %s ***\n\n", \
                   __FILE__, __LINE__, # arg ); \
 }

int UDFReadBlocksRaw(dvd_reader_t *device, uint32_t lb_number,
                     size_t block_count, unsigned char *data, int encrypted);

#endif /* LIBDVDREAD_DVDREAD_INTERNAL_H */
