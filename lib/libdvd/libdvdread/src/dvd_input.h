/*
 * Copyright (C) 2001, 2002 Samuel Hocevar <sam@zoy.org>,
 *                          Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
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

#ifndef LIBDVDREAD_DVD_INPUT_H
#define LIBDVDREAD_DVD_INPUT_H

/**
 * Defines and flags.  Make sure they fit the libdvdcss API!
 */
#define DVDINPUT_NOFLAGS         0

#define DVDINPUT_READ_DECRYPT    (1 << 0)

typedef struct dvd_input_s *dvd_input_t;

#if defined( __MINGW32__ )
#   undef  lseek
#   define lseek  _lseeki64
#   undef  off_t
#   define off_t off64_t
#   undef  stat
#   define stat  _stati64
#   define fstat _fstati64
#   define wstat _wstati64
#endif

/**
 * Function pointers that will be filled in by the input implementation.
 * These functions provide the main API.
 */
extern dvd_input_t (*dvdinput_open)  (const char *);
extern int         (*dvdinput_close) (dvd_input_t);
extern int         (*dvdinput_seek)  (dvd_input_t, int);
extern int         (*dvdinput_title) (dvd_input_t, int);
extern int         (*dvdinput_read)  (dvd_input_t, void *, int, int);
extern char *      (*dvdinput_error) (dvd_input_t);

/**
 * Setup function accessed by dvd_reader.c.  Returns 1 if there is CSS support.
 */
int dvdinput_setup(void);

#endif /* LIBDVDREAD_DVD_INPUT_H */
