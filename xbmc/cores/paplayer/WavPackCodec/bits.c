////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2005 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// bits.c

// This module provides utilities to support the BitStream structure which is
// used to read and write all WavPack audio data streams. It also contains a
// wrapper for the stream I/O functions and a set of functions dealing with
// endian-ness, both for enhancing portability. Finally, a debug wrapper for
// the malloc() system is provided.

#include "wavpack.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#if defined(WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

////////////////////////// Bitstream functions ////////////////////////////////

#if defined(UNPACK) || defined(INFO_ONLY)

// Open the specified BitStream and associate with the specified buffer.

static void bs_read (Bitstream *bs);

void bs_open_read (Bitstream *bs, uchar *buffer_start, uchar *buffer_end)
{
    bs->error = bs->sr = bs->bc = 0;
    bs->ptr = (bs->buf = buffer_start) - 1;
    bs->end = buffer_end;
    bs->wrap = bs_read;
}

// This function is only called from the getbit() and getbits() macros when
// the BitStream has been exhausted and more data is required. Sinve these
// bistreams no longer access files, this function simple sets an error and
// resets the buffer.

static void bs_read (Bitstream *bs)
{
    bs->ptr = bs->buf - 1;
    bs->error = 1;
}

// This function is called to close the bitstream. It returns the number of
// full bytes actually read as bits.

uint32_t bs_close_read (Bitstream *bs)
{
    uint32_t bytes_read;

    if (bs->bc < 8)
	bs->ptr++;

    if ((bs->buf - bs->ptr) & 1)
	bs->ptr++;

    bytes_read = bs->ptr - bs->buf;
    CLEAR (*bs);
    return bytes_read;
}

#endif

#ifdef PACK

// Open the specified BitStream using the specified buffer pointers. It is
// assumed that enough buffer space has been allocated for all data that will
// be written, otherwise an error will be generated.

static void bs_write (Bitstream *bs);

void bs_open_write (Bitstream *bs, uchar *buffer_start, uchar *buffer_end)
{
    bs->error = bs->sr = bs->bc = 0;
    bs->ptr = bs->buf = buffer_start;
    bs->end = buffer_end;
    bs->wrap = bs_write;
}

// This function is only called from the putbit() and putbits() macros when
// the buffer is full, which is now flagged as an error.

static void bs_write (Bitstream *bs)
{
    bs->ptr = bs->buf;
    bs->error = 1;
}

// This function forces a flushing write of the specified BitStream, and
// returns the total number of bytes written into the buffer.

uint32_t bs_close_write (Bitstream *bs)
{
    uint32_t bytes_written;

    if (bs->error)
	return (uint32_t) -1;

    while (bs->bc || ((bs->ptr - bs->buf) & 1)) putbit_1 (bs);
    bytes_written = bs->ptr - bs->buf;
    CLEAR (*bs);
    return bytes_written;
}

#endif

/////////////////////// Endian Correction Routines ////////////////////////////

void little_endian_to_native (void *data, char *format)
{
    uchar *cp = (uchar *) data;
    int32_t temp;

    while (*format) {
	switch (*format) {
	    case 'L':
		temp = cp [0] + ((int32_t) cp [1] << 8) + ((int32_t) cp [2] << 16) + ((int32_t) cp [3] << 24);
		* (int32_t *) cp = temp;
		cp += 4;
		break;

	    case 'S':
		temp = cp [0] + (cp [1] << 8);
		* (short *) cp = (short) temp;
		cp += 2;
		break;

	    default:
		if (isdigit (*format))
		    cp += *format - '0';

		break;
	}

	format++;
    }
}

void native_to_little_endian (void *data, char *format)
{
    uchar *cp = (uchar *) data;
    int32_t temp;

    while (*format) {
	switch (*format) {
	    case 'L':
		temp = * (int32_t *) cp;
		*cp++ = (uchar) temp;
		*cp++ = (uchar) (temp >> 8);
		*cp++ = (uchar) (temp >> 16);
		*cp++ = (uchar) (temp >> 24);
		break;

	    case 'S':
		temp = * (short *) cp;
		*cp++ = (uchar) temp;
		*cp++ = (uchar) (temp >> 8);
		break;

	    default:
		if (isdigit (*format))
		    cp += *format - '0';

		break;
	}

	format++;
    }
}

////////////////////////// Debug Wrapper for Malloc ///////////////////////////

#ifdef DEBUG_ALLOC

void *vptrs [512];

static void *add_ptr (void *ptr)
{
    int i;

    for (i = 0; i < 512; ++i)
	if (!vptrs [i]) {
	    vptrs [i] = ptr;
	    break;
	}

    if (i == 512)
	error_line ("too many mallocs!");

    return ptr;
}

static void *del_ptr (void *ptr)
{
    int i;

    for (i = 0; i < 512; ++i)
	if (vptrs [i] == ptr) {
	    vptrs [i] = NULL;
	    break;
	}

    if (i == 512)
	error_line ("free invalid ptr!");

    return ptr;
}

void *malloc_db (uint32_t size)
{
    if (size)
	return add_ptr (malloc (size));
    else
	return NULL;
}

void free_db (void *ptr)
{
    if (ptr)
	free (del_ptr (ptr));
}

void *realloc_db (void *ptr, uint32_t size)
{
    if (ptr && size)
	return add_ptr (realloc (del_ptr (ptr), size));
    else if (size)
	return malloc_db (size);
    else
	free_db (ptr);

    return NULL;
}

int32_t dump_alloc (void)
{
    int i, j;

    for (j = i = 0; i < 512; ++i)
	if (vptrs [i])
	    j++;

    return j;
}

#endif
