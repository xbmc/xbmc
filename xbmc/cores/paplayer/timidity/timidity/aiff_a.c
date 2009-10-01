/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    aiff_a.c - Functions to output AIFF audio file (*.aiff).
				Written by Masanao Izumo <mo@goice.co.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#ifdef __W32__
#include <io.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <fcntl.h>
#include <math.h>
#include <ctype.h>

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 bytes);
static int acntl(int request, void *arg);
static int write_u32(uint32 value);
static int write_u16(uint16 value);
static int chunk_start(const char *id, uint32 chunk_len);
static int write_str(const char *s);
static void ConvertToIeeeExtended(double num, char *bytes);

/* export the playback mode */
#define dpm aiff_play_mode

PlayMode dpm = {
    44100,
#ifdef LITTLE_ENDIAN
    PE_SIGNED|PE_16BIT|PE_BYTESWAP,
#else
    PE_SIGNED|PE_16BIT,
#endif
    PF_PCM_STREAM,
    -1,
    {0,0,0,0,0},
    "AIFF file", 'a',
    NULL,
    open_output,
    close_output,
    output_data,
    acntl
};

#define UPDATE_HEADER_STEP (128*1024) /* 128KB */
static uint32 bytes_output, next_bytes;
static int already_warning_lseek;
static int comm_chunk_offset, comm_chunk_size;

/*************************************************************************/

static int write_u32(uint32 value)
{
    int n;
    value = BE_LONG(value);
    if((n = write(dpm.fd, (char *)&value, 4)) == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: write: %s",
		  dpm.name, strerror(errno));
	close_output();
	return -1;
    }
    return n;
}

static int write_u16(uint16 value)
{
    int n;
    value = BE_SHORT(value);
    if((n = write(dpm.fd, (char *)&value, 2)) == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: write: %s",
		  dpm.name, strerror(errno));
	close_output();
	return -1;
    }
    return n;
}

static int write_str(const char *s)
{
    int n;
    if((n = write(dpm.fd, s, strlen(s))) == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: write: %s",
		  dpm.name, strerror(errno));
	close_output();
	return -1;
    }
    return n;
}

static int write_ieee_80bitfloat(double num)
{
    char bytes[10];
    int n;
    ConvertToIeeeExtended(num, bytes);

    if((n = write(dpm.fd, bytes, 10)) == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: write: %s",
		  dpm.name, strerror(errno));
	close_output();
	return -1;
    }
    return n;
}

static int chunk_start(const char *id, uint32 chunk_len)
{
    int i, j;

    if((i = write_str(id)) == -1)
	return -1;
    if((j = write_u32(chunk_len)) == -1)
	return -1;
    return i + j;
}

static int update_header(void)
{
    off_t save_point;
    uint32 f;

    if(already_warning_lseek)
	return 0;

    save_point = lseek(dpm.fd, 0, SEEK_CUR);
    if(save_point == -1 || lseek(dpm.fd, 4, SEEK_SET) == -1)
    {
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "Warning: %s: %s: Can't make valid header",
		  dpm.name, strerror(errno));
	already_warning_lseek = 1;
	return 0;
    }

    /* file size - 8 */
    if(write_u32((uint32)(comm_chunk_offset - 8	/* FORM,FVER chunk */
			  + 8 + comm_chunk_size	/* COMM chunk */
			  + 16 + bytes_output	/* SSND chunk */
			  )) == -1) return -1;
    /* COMM chunk */
    /* number of frames */
    lseek(dpm.fd, comm_chunk_offset + 10, SEEK_SET);
    f = bytes_output;
    if(!(dpm.encoding & PE_MONO))
	f /= 2;
    if(dpm.encoding & PE_24BIT)
	f /= 3;
    else if(dpm.encoding & PE_16BIT)
	f /= 2;
    if(write_u32(f) == -1) return -1;

    /* SSND chunk */
    lseek(dpm.fd, comm_chunk_offset + 8 + comm_chunk_size + 4, SEEK_SET);
    if(write_u32((uint32)(8 + bytes_output)) == -1) return -1;

    lseek(dpm.fd, save_point, SEEK_SET);

    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
	      "%s: Update AIFF header", dpm.name, bytes_output);
    return 0;
}

#define AIFC_VERSION_1	(uint32)0xA2805140

static int aiff_output_open(const char *fname)
{
  int t, compressed;
  const char *compressionName = NULL;
  uint8 padByte, compressionNameLength;

  if(strcmp(fname, "-") == 0) {
    dpm.fd = 1; /* data to stdout */
  } else {
    /* Open the audio file */
    dpm.fd = open(fname, FILE_OUTPUT_MODE);
    if(dpm.fd < 0) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		fname, strerror(errno));
      return -1;
    }
  }

  /* FORM chunk */
  if(write_str("FORM") == -1) return -1;

  /* file size - 8 (dmy) */
  if(write_u32((uint32)0xffffffff) == -1) return -1;

  /* form type */
  compressed = dpm.encoding & (PE_ULAW|PE_ALAW);
  if(write_str(compressed ? "AIFC" : "AIFF") == -1) return -1;

  /* FVER chunk */
  if(compressed) {
    if(chunk_start("FVER", 4) == -1) return -1;
    if(write_u32(AIFC_VERSION_1) == -1) return -1;
    compressionName = (dpm.encoding & PE_ULAW) ? "\xB5Law 2:1" : "ALaw 2:1";
  }

  /* COMM chunk */
  if(!compressed) {
    comm_chunk_offset = 12;
    comm_chunk_size = 18;
  } else {
    compressionNameLength = strlen(compressionName);
    comm_chunk_offset = 12 + 12;
    comm_chunk_size = 18 + 4 + (1 + compressionNameLength);
  }
  if(chunk_start("COMM", comm_chunk_size) == -1) return -1;

  /* number of channels */
  if(dpm.encoding & PE_MONO) {
    if(write_u16((uint16)1) == -1) return -1;
  } else {
    if(write_u16((uint16)2) == -1) return -1;
  }

  /* number of frames (dmy) */
  if(write_u32((uint32)0xffffffff) == -1) return -1;

  /* bits per sample (decompressed size) */
  if(dpm.encoding & PE_24BIT)
    t = 24;
  else if(dpm.encoding & (PE_16BIT | PE_ULAW | PE_ALAW))
    t = 16;
  else
    t = 8;
  if(write_u16((uint16)t) == -1) return -1;

  /* sample rate */
  if(write_ieee_80bitfloat((double)dpm.rate) == -1) return -1;
  
  /* compression type */
  if(compressed) {
    if(write_str((dpm.encoding & PE_ULAW) ? "ulaw" : "alaw") == -1) return -1;
    if(write(dpm.fd, &compressionNameLength, 1) == -1) return -1;
    if(write_str(compressionName) == -1) return -1;
  }
  if(comm_chunk_size & 1) {
    padByte = 0;
    if(write(dpm.fd, &padByte, 1) == -1) return -1;
    comm_chunk_size++;
  }

  /* SSND chunk */
  if(chunk_start("SSND", (int32)0xffffffff) == -1) return -1;

  /* offset */
  if(write_u32((uint32)0) == -1) return -1;

  /* block size */
  if(write_u32((uint32)0) == -1) return -1;

  return 0;
}

static int auto_aiff_output_open(const char *input_filename)
{
  char *output_filename = (char *)safe_malloc(strlen(input_filename) + 5);
  char *ext, *p;

  strcpy(output_filename, input_filename);
  if((ext = strrchr(output_filename, '.')) == NULL)
    ext = output_filename + strlen(output_filename);
  else {
    /* strip ".gz" */
    if(strcasecmp(ext, ".gz") == 0) {
      *ext = '\0';
      if((ext = strrchr(output_filename, '.')) == NULL)
	ext = output_filename + strlen(output_filename);
    }
  }

  /* replace '.' and '#' before ext */
  for(p = output_filename; p < ext; p++)
    if(*p == '.' || *p == '#')
      *p = '_';

  if(*ext && isupper(*(ext + 1)))
    strcpy(ext, ".AIFF");
  else
    strcpy(ext, ".aiff");
  if(aiff_output_open(output_filename) == -1) {
    free(output_filename);
    return -1;
  }

  if(dpm.name != NULL)
    free(dpm.name);
  dpm.name = output_filename;
  ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Output %s", dpm.name);
  return 0;
}

static int open_output(void)
{
    int include_enc, exclude_enc;

    include_enc = PE_SIGNED;
    exclude_enc = PE_ULAW | PE_ALAW;
    if(dpm.encoding & (PE_16BIT | PE_24BIT))
    {
#ifdef LITTLE_ENDIAN
	include_enc |= PE_BYTESWAP;
#else
	exclude_enc |= PE_BYTESWAP;
#endif /* LITTLE_ENDIAN */
    }
    else if(dpm.encoding & (PE_ULAW|PE_ALAW))
	include_enc = exclude_enc = 0;

    dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);

    if(dpm.name == NULL) {
      dpm.flag |= PF_AUTO_SPLIT_FILE;
      dpm.name = NULL;
    } else {
      dpm.flag &= ~PF_AUTO_SPLIT_FILE;
      if(aiff_output_open(dpm.name) == -1)
	return -1;
    }

    bytes_output = 0;
    already_warning_lseek = 0;
    next_bytes = bytes_output + UPDATE_HEADER_STEP;

    return 0;
}

static int output_data(char *buf, int32 bytes)
{
    int n;

    if(dpm.fd == -1) return -1;

    while(((n = write(dpm.fd, buf, bytes)) == -1) && errno == EINTR)
	;
    if(n == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, strerror(errno));
	return -1;
    }

    bytes_output += bytes;

#if UPDATE_HEADER_STEP > 0
    if(bytes_output >= next_bytes)
    {
	if(update_header() == -1) return -1;
	next_bytes = bytes_output + UPDATE_HEADER_STEP;
    }
#endif /* UPDATE_HEADER_STEP */
    return n;
}

static void close_output(void)
{
    if(dpm.fd != 1 && /* We don't close stdout */
       dpm.fd != -1)
    {
	update_header();
	close(dpm.fd);
	dpm.fd = -1;
    }
}

static int acntl(int request, void *arg)
{
  switch(request) {
  case PM_REQ_PLAY_START:
    if(dpm.flag & PF_AUTO_SPLIT_FILE)
      return auto_aiff_output_open(current_file_info->filename);
    break;
  case PM_REQ_PLAY_END:
    if(dpm.flag & PF_AUTO_SPLIT_FILE) {
      close_output();
      return 0;
    }
    break;
  case PM_REQ_DISCARD:
    return 0;
  }
  return -1;
}


/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 */

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/

# define FloatToUnsigned(f)      ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L) + 1)

static void ConvertToIeeeExtended(double num, char *bytes)
{
    int    sign;
    int expon;
    double fMant, fsMant;
    unsigned long hiMant, loMant;

    if (num < 0) {
        sign = 0x8000;
        num *= -1;
    } else {
        sign = 0;
    }

    if (num == 0) {
        expon = 0; hiMant = 0; loMant = 0;
    }
    else {
        fMant = frexp(num, &expon);
        if ((expon > 16384) || !(fMant < 1)) {    /* Infinity or NaN */
            expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
        }
        else {    /* Finite */
            expon += 16382;
            if (expon < 0) {    /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);
            fsMant = floor(fMant);
            hiMant = FloatToUnsigned(fsMant);
            fMant = ldexp(fMant - fsMant, 32);
            fsMant = floor(fMant);
            loMant = FloatToUnsigned(fsMant);
        }
    }

    bytes[0] = expon >> 8;
    bytes[1] = expon;
    bytes[2] = (char)(hiMant >> 24);
    bytes[3] = (char)(hiMant >> 16);
    bytes[4] = (char)(hiMant >> 8);
    bytes[5] = (char)hiMant;
    bytes[6] = (char)(loMant >> 24);
    bytes[7] = (char)(loMant >> 16);
    bytes[8] = (char)(loMant >> 8);
    bytes[9] = (char)loMant;
}
