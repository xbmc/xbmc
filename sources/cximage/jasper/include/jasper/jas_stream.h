/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2003 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer Software License
 * 
 * IMAGE POWER JPEG-2000 PUBLIC LICENSE
 * ************************************
 * 
 * GRANT:
 * 
 * Permission is hereby granted, free of charge, to any person (the "User")
 * obtaining a copy of this software and associated documentation, to deal
 * in the JasPer Software without restriction, including without limitation
 * the right to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the JasPer Software (in source and binary forms),
 * and to permit persons to whom the JasPer Software is furnished to do so,
 * provided further that the License Conditions below are met.
 * 
 * License Conditions
 * ******************
 * 
 * A.  Redistributions of source code must retain the above copyright notice,
 * and this list of conditions, and the following disclaimer.
 * 
 * B.  Redistributions in binary form must reproduce the above copyright
 * notice, and this list of conditions, and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * 
 * C.  Neither the name of Image Power, Inc. nor any other contributor
 * (including, but not limited to, the University of British Columbia and
 * Michael David Adams) may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * D.  User agrees that it shall not commence any action against Image Power,
 * Inc., the University of British Columbia, Michael David Adams, or any
 * other contributors (collectively "Licensors") for infringement of any
 * intellectual property rights ("IPR") held by the User in respect of any
 * technology that User owns or has a right to license or sublicense and
 * which is an element required in order to claim compliance with ISO/IEC
 * 15444-1 (i.e., JPEG-2000 Part 1).  "IPR" means all intellectual property
 * rights worldwide arising under statutory or common law, and whether
 * or not perfected, including, without limitation, all (i) patents and
 * patent applications owned or licensable by User; (ii) rights associated
 * with works of authorship including copyrights, copyright applications,
 * copyright registrations, mask work rights, mask work applications,
 * mask work registrations; (iii) rights relating to the protection of
 * trade secrets and confidential information; (iv) any right analogous
 * to those set forth in subsections (i), (ii), or (iii) and any other
 * proprietary rights relating to intangible property (other than trademark,
 * trade dress, or service mark rights); and (v) divisions, continuations,
 * renewals, reissues and extensions of the foregoing (as and to the extent
 * applicable) now existing, hereafter filed, issued or acquired.
 * 
 * E.  If User commences an infringement action against any Licensor(s) then
 * such Licensor(s) shall have the right to terminate User's license and
 * all sublicenses that have been granted hereunder by User to other parties.
 * 
 * F.  This software is for use only in hardware or software products that
 * are compliant with ISO/IEC 15444-1 (i.e., JPEG-2000 Part 1).  No license
 * or right to this Software is granted for products that do not comply
 * with ISO/IEC 15444-1.  The JPEG-2000 Part 1 standard can be purchased
 * from the ISO.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE.
 * NO USE OF THE JASPER SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE JASPER SOFTWARE IS PROVIDED BY THE LICENSORS AND
 * CONTRIBUTORS UNDER THIS LICENSE ON AN ``AS-IS'' BASIS, WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
 * WARRANTIES THAT THE JASPER SOFTWARE IS FREE OF DEFECTS, IS MERCHANTABLE,
 * IS FIT FOR A PARTICULAR PURPOSE OR IS NON-INFRINGING.  THOSE INTENDING
 * TO USE THE JASPER SOFTWARE OR MODIFICATIONS THEREOF FOR USE IN HARDWARE
 * OR SOFTWARE PRODUCTS ARE ADVISED THAT THEIR USE MAY INFRINGE EXISTING
 * PATENTS, COPYRIGHTS, TRADEMARKS, OR OTHER INTELLECTUAL PROPERTY RIGHTS.
 * THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE JASPER SOFTWARE
 * IS WITH THE USER.  SHOULD ANY PART OF THE JASPER SOFTWARE PROVE DEFECTIVE
 * IN ANY RESPECT, THE USER (AND NOT THE INITIAL DEVELOPERS, THE UNIVERSITY
 * OF BRITISH COLUMBIA, IMAGE POWER, INC., MICHAEL DAVID ADAMS, OR ANY
 * OTHER CONTRIBUTOR) SHALL ASSUME THE COST OF ANY NECESSARY SERVICING,
 * REPAIR OR CORRECTION.  UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY,
 * WHETHER TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE, SHALL THE
 * INITIAL DEVELOPER, THE UNIVERSITY OF BRITISH COLUMBIA, IMAGE POWER, INC.,
 * MICHAEL DAVID ADAMS, ANY OTHER CONTRIBUTOR, OR ANY DISTRIBUTOR OF THE
 * JASPER SOFTWARE, OR ANY SUPPLIER OF ANY OF SUCH PARTIES, BE LIABLE TO
 * THE USER OR ANY OTHER PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES OF ANY CHARACTER INCLUDING, WITHOUT LIMITATION,
 * DAMAGES FOR LOSS OF GOODWILL, WORK STOPPAGE, COMPUTER FAILURE OR
 * MALFUNCTION, OR ANY AND ALL OTHER COMMERCIAL DAMAGES OR LOSSES, EVEN IF
 * SUCH PARTY HAD BEEN INFORMED, OR OUGHT TO HAVE KNOWN, OF THE POSSIBILITY
 * OF SUCH DAMAGES.  THE JASPER SOFTWARE AND UNDERLYING TECHNOLOGY ARE NOT
 * FAULT-TOLERANT AND ARE NOT DESIGNED, MANUFACTURED OR INTENDED FOR USE OR
 * RESALE AS ON-LINE CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING
 * FAIL-SAFE PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT
 * LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 * JASPER SOFTWARE OR UNDERLYING TECHNOLOGY OR PRODUCT COULD LEAD DIRECTLY
 * TO DEATH, PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE
 * ("HIGH RISK ACTIVITIES").  LICENSOR SPECIFICALLY DISCLAIMS ANY EXPRESS
 * OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.  USER WILL NOT
 * KNOWINGLY USE, DISTRIBUTE OR RESELL THE JASPER SOFTWARE OR UNDERLYING
 * TECHNOLOGY OR PRODUCTS FOR HIGH RISK ACTIVITIES AND WILL ENSURE THAT ITS
 * CUSTOMERS AND END-USERS OF ITS PRODUCTS ARE PROVIDED WITH A COPY OF THE
 * NOTICE SPECIFIED IN THIS SECTION.
 * 
 * __END_OF_JASPER_LICENSE__
 */

/*
 * I/O Stream Class
 *
 * $Id$
 */

#ifndef JAS_STREAM_H
#define JAS_STREAM_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <jasper/jas_config.h>

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <jasper/jas_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
* Constants.
\******************************************************************************/

/* On most UNIX systems, we probably need to define O_BINARY ourselves. */
#ifndef O_BINARY
#define O_BINARY	0
#endif

/*
 * Stream open flags.
 */

/* The stream was opened for reading. */
#define JAS_STREAM_READ	0x0001
/* The stream was opened for writing. */
#define JAS_STREAM_WRITE	0x0002
/* The stream was opened for appending. */
#define JAS_STREAM_APPEND	0x0004
/* The stream was opened in binary mode. */
#define JAS_STREAM_BINARY	0x0008
/* The stream should be created/truncated. */
#define JAS_STREAM_CREATE	0x0010


/*
 * Stream buffering flags.
 */

/* The stream is unbuffered. */
#define JAS_STREAM_UNBUF	0x0000
/* The stream is line buffered. */
#define JAS_STREAM_LINEBUF	0x0001
/* The stream is fully buffered. */
#define JAS_STREAM_FULLBUF	0x0002
/* The buffering mode mask. */
#define	JAS_STREAM_BUFMODEMASK	0x000f

/* The memory associated with the buffer needs to be deallocated when the
  stream is destroyed. */
#define JAS_STREAM_FREEBUF	0x0008
/* The buffer is currently being used for reading. */
#define JAS_STREAM_RDBUF	0x0010
/* The buffer is currently being used for writing. */
#define JAS_STREAM_WRBUF	0x0020

/*
 * Stream error flags.
 */

/* The end-of-file has been encountered (on reading). */
#define JAS_STREAM_EOF	0x0001
/* An I/O error has been encountered on the stream. */
#define JAS_STREAM_ERR	0x0002
/* The read/write limit has been exceeded. */
#define	JAS_STREAM_RWLIMIT	0x0004
/* The error mask. */
#define JAS_STREAM_ERRMASK \
	(JAS_STREAM_EOF | JAS_STREAM_ERR | JAS_STREAM_RWLIMIT)

/*
 * Other miscellaneous constants.
 */

/* The default buffer size (for fully-buffered operation). */
#define JAS_STREAM_BUFSIZE	8192
/* The default permission mask for file creation. */
#define JAS_STREAM_PERMS	0666

/* The maximum number of characters that can always be put back on a stream. */
#define	JAS_STREAM_MAXPUTBACK	16

/******************************************************************************\
* Types.
\******************************************************************************/

/*
 * Generic file object.
 */

typedef void jas_stream_obj_t;

/*
 * Generic file object operations.
 */

typedef struct {

	/* Read characters from a file object. */
	int (*read_)(jas_stream_obj_t *obj, char *buf, int cnt);

	/* Write characters to a file object. */
	int (*write_)(jas_stream_obj_t *obj, char *buf, int cnt);

	/* Set the position for a file object. */
	long (*seek_)(jas_stream_obj_t *obj, long offset, int origin);

	/* Close a file object. */
	int (*close_)(jas_stream_obj_t *obj);

} jas_stream_ops_t;

/*
 * Stream object.
 */

typedef struct {

	/* The mode in which the stream was opened. */
	int openmode_;

	/* The buffering mode. */
	int bufmode_;

	/* The stream status. */
	int flags_;

	/* The start of the buffer area to use for reading/writing. */
	uchar *bufbase_;

	/* The start of the buffer area excluding the extra initial space for
	  character putback. */
	uchar *bufstart_;

	/* The buffer size. */
	int bufsize_;

	/* The current position in the buffer. */
	uchar *ptr_;

	/* The number of characters that must be read/written before
	the buffer needs to be filled/flushed. */
	int cnt_;

	/* A trivial buffer to be used for unbuffered operation. */
	uchar tinybuf_[JAS_STREAM_MAXPUTBACK + 1];

	/* The operations for the underlying stream file object. */
	jas_stream_ops_t *ops_;

	/* The underlying stream file object. */
	jas_stream_obj_t *obj_;

	/* The number of characters read/written. */
	long rwcnt_;

	/* The maximum number of characters that may be read/written. */
	long rwlimit_;

} jas_stream_t;

/*
 * Regular file object.
 */

/*
 * File descriptor file object.
 */
typedef struct {
	int fd;
	int flags;
	char pathname[L_tmpnam + 1];
} jas_stream_fileobj_t;

#define	JAS_STREAM_FILEOBJ_DELONCLOSE	0x01
#define JAS_STREAM_FILEOBJ_NOCLOSE	0x02

/*
 * Memory file object.
 */

typedef struct {

	/* The data associated with this file. */
	uchar *buf_;

	/* The allocated size of the buffer for holding file data. */
	int bufsize_;

	/* The length of the file. */
	int_fast32_t len_;

	/* The seek position. */
	int_fast32_t pos_;

	/* Is the buffer growable? */
	int growable_;

	/* Was the buffer allocated internally? */
	int myalloc_;

} jas_stream_memobj_t;

/******************************************************************************\
* Macros/functions for opening and closing streams.
\******************************************************************************/

/* Open a file as a stream. */
jas_stream_t *jas_stream_fopen(const char *filename, const char *mode);

/* Open a memory buffer as a stream. */
jas_stream_t *jas_stream_memopen(char *buf, int bufsize);

/* Open a file descriptor as a stream. */
jas_stream_t *jas_stream_fdopen(int fd, const char *mode);

/* Open a stdio stream as a stream. */
jas_stream_t *jas_stream_freopen(const char *path, const char *mode, FILE *fp);

/* Open a temporary file as a stream. */
jas_stream_t *jas_stream_tmpfile(void);

/* Close a stream. */
int jas_stream_close(jas_stream_t *stream);

/******************************************************************************\
* Macros/functions for getting/setting the stream state.
\******************************************************************************/

/* Get the EOF indicator for a stream. */
#define jas_stream_eof(stream) \
	(((stream)->flags_ & JAS_STREAM_EOF) != 0)

/* Get the error indicator for a stream. */
#define jas_stream_error(stream) \
	(((stream)->flags_ & JAS_STREAM_ERR) != 0)

/* Clear the error indicator for a stream. */
#define jas_stream_clearerr(stream) \
	((stream)->flags_ &= ~(JAS_STREAM_ERR | JAS_STREAM_EOF))

/* Get the read/write limit for a stream. */
#define	jas_stream_getrwlimit(stream) \
	(((const jas_stream_t *)(stream))->rwlimit_)

/* Set the read/write limit for a stream. */
int jas_stream_setrwlimit(jas_stream_t *stream, long rwlimit);

/* Get the read/write count for a stream. */
#define	jas_stream_getrwcount(stream) \
	(((const jas_stream_t *)(stream))->rwcnt_)

/* Set the read/write count for a stream. */
long jas_stream_setrwcount(jas_stream_t *stream, long rwcnt);

/******************************************************************************\
* Macros/functions for I/O.
\******************************************************************************/

/* Read a character from a stream. */
#if defined(DEBUG)
#define	jas_stream_getc(stream)	jas_stream_getc_func(stream)
#else
#define jas_stream_getc(stream)	jas_stream_getc_macro(stream)
#endif

/* Write a character to a stream. */
#if defined(DEBUG)
#define jas_stream_putc(stream, c)	jas_stream_putc_func(stream, c)
#else
#define jas_stream_putc(stream, c)	jas_stream_putc_macro(stream, c)
#endif

/* Read characters from a stream into a buffer. */
int jas_stream_read(jas_stream_t *stream, void *buf, int cnt);

/* Write characters from a buffer to a stream. */
int jas_stream_write(jas_stream_t *stream, const void *buf, int cnt);

/* Write formatted output to a stream. */
int jas_stream_printf(jas_stream_t *stream, const char *fmt, ...);

/* Write a string to a stream. */
int jas_stream_puts(jas_stream_t *stream, const char *s);

/* Read a line of input from a stream. */
char *jas_stream_gets(jas_stream_t *stream, char *buf, int bufsize);

/* Look at the next character to be read from a stream without actually
  removing it from the stream. */
#define	jas_stream_peekc(stream) \
	(((stream)->cnt_ <= 0) ? jas_stream_fillbuf(stream, 0) : \
	  ((int)(*(stream)->ptr_)))

/* Put a character back on a stream. */
int jas_stream_ungetc(jas_stream_t *stream, int c);

/******************************************************************************\
* Macros/functions for getting/setting the stream position.
\******************************************************************************/

/* Is it possible to seek on this stream? */
int jas_stream_isseekable(jas_stream_t *stream);

/* Set the current position within the stream. */
long jas_stream_seek(jas_stream_t *stream, long offset, int origin);

/* Get the current position within the stream. */
long jas_stream_tell(jas_stream_t *stream);

/* Seek to the beginning of a stream. */
int jas_stream_rewind(jas_stream_t *stream);

/******************************************************************************\
* Macros/functions for flushing.
\******************************************************************************/

/* Flush any pending output to a stream. */
int jas_stream_flush(jas_stream_t *stream);

/******************************************************************************\
* Miscellaneous macros/functions.
\******************************************************************************/

/* Copy data from one stream to another. */
int jas_stream_copy(jas_stream_t *dst, jas_stream_t *src, int n);

/* Display stream contents (for debugging purposes). */
int jas_stream_display(jas_stream_t *stream, FILE *fp, int n);

/* Consume (i.e., discard) characters from stream. */
int jas_stream_gobble(jas_stream_t *stream, int n);

/* Write a character multiple times to a stream. */
int jas_stream_pad(jas_stream_t *stream, int n, int c);

/* Get the size of the file associated with the specified stream.
  The specified stream must be seekable. */
long jas_stream_length(jas_stream_t *stream);

/******************************************************************************\
* Internal functions.
\******************************************************************************/

/* The following functions are for internal use only!  If you call them
directly, you will die a horrible, miserable, and painful death! */

/* Read a character from a stream. */
#define jas_stream_getc_macro(stream) \
	((!((stream)->flags_ & (JAS_STREAM_ERR | JAS_STREAM_EOF | \
	  JAS_STREAM_RWLIMIT))) ? \
	  (((stream)->rwlimit_ >= 0 && (stream)->rwcnt_ >= (stream)->rwlimit_) ? \
	  (stream->flags_ |= JAS_STREAM_RWLIMIT, EOF) : \
	  jas_stream_getc2(stream)) : EOF)
#define jas_stream_getc2(stream) \
	((--(stream)->cnt_ < 0) ? jas_stream_fillbuf(stream, 1) : \
	  (++(stream)->rwcnt_, (int)(*(stream)->ptr_++)))

/* Write a character to a stream. */
#define jas_stream_putc_macro(stream, c) \
	((!((stream)->flags_ & (JAS_STREAM_ERR | JAS_STREAM_EOF | \
	  JAS_STREAM_RWLIMIT))) ? \
	  (((stream)->rwlimit_ >= 0 && (stream)->rwcnt_ >= (stream)->rwlimit_) ? \
	  (stream->flags_ |= JAS_STREAM_RWLIMIT, EOF) : \
	  jas_stream_putc2(stream, c)) : EOF)
#define jas_stream_putc2(stream, c) \
	(((stream)->bufmode_ |= JAS_STREAM_WRBUF, --(stream)->cnt_ < 0) ? \
	  jas_stream_flushbuf((stream), (uchar)(c)) : \
	  (++(stream)->rwcnt_, (int)(*(stream)->ptr_++ = (c))))

/* These prototypes need to be here for the sake of the stream_getc and
stream_putc macros. */
int jas_stream_fillbuf(jas_stream_t *stream, int getflag);
int jas_stream_flushbuf(jas_stream_t *stream, int c);
int jas_stream_getc_func(jas_stream_t *stream);
int jas_stream_putc_func(jas_stream_t *stream, int c);

#ifdef __cplusplus
}
#endif

#endif
