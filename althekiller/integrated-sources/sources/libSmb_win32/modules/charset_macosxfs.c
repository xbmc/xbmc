/* 
   Unix SMB/CIFS implementation.
   Samba charset module for Mac OS X/Darwin
   Copyright (C) Benjamin Riefenstahl 2003
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 * modules/charset_macosxfs.c
 *
 * A Samba charset module to use on Mac OS X/Darwin as the filesystem
 * and display encoding.
 *
 * Actually two implementations are provided here.  The default
 * implementation is based on the official CFString API.  The other is
 * based on internal CFString APIs as defined in the OpenDarwin
 * source.
 */

#include "includes.h"

/*
 * Include OS frameworks.  These are only needed in this module.
 */
#include <CoreFoundation/CFString.h>

/*
 * See if autoconf has found us the internal headers in some form.
 */
#if HAVE_COREFOUNDATION_CFSTRINGENCODINGCONVERTER_H
#	include <Corefoundation/CFStringEncodingConverter.h>
#	include <Corefoundation/CFUnicodePrecomposition.h>
#	define USE_INTERNAL_API 1
#elif HAVE_CFSTRINGENCODINGCONVERTER_H
#	include <CFStringEncodingConverter.h>
#	include <CFUnicodePrecomposition.h>
#	define USE_INTERNAL_API 1
#endif

/*
 * Compile time configuration: Do we want debug output?
 */
/* #define DEBUG_STRINGS 1 */

/*
 * A simple, but efficient memory provider for our buffers.
 */
static inline void *resize_buffer (void *buffer, size_t *size, size_t newsize)
{
	if (newsize > *size) {
		*size = newsize + 128;
		buffer = realloc(buffer, *size);
	}
	return buffer;
}

/*
 * While there is a version of OpenDarwin for intel, the usual case is
 * big-endian PPC.  So we need byte swapping to handle the
 * little-endian byte order of the network protocol.  We also need an
 * additional dynamic buffer to do this work for incoming data blocks,
 * because we have to consider the original data as constant.
 *
 * We abstract the differences away by providing a simple facade with
 * these functions/macros:
 *
 *	le_to_native(dst,src,len)
 *	native_to_le(cp,len)
 *	set_ucbuffer_with_le(buffer,bufsize,data,size)
 *	set_ucbuffer_with_le_copy(buffer,bufsize,data,size,reserve)
 */
#ifdef WORDS_BIGENDIAN

static inline void swap_bytes (char * dst, const char * src, size_t len)
{
	const char *srcend = src + len;
	while (src < srcend) {
		dst[0] = src[1];
		dst[1] = src[0];
		dst += 2;
		src += 2;
	}
}
static inline void swap_bytes_inplace (char * cp, size_t len)
{
	char temp;
	char *end = cp + len;
	while (cp  < end) {
		temp = cp[1];
		cp[1] = cp[0];
		cp[0] = temp;
		cp += 2;
	}
}

#define le_to_native(dst,src,len)	swap_bytes(dst,src,len)
#define native_to_le(cp,len)		swap_bytes_inplace(cp,len)
#define set_ucbuffer_with_le(buffer,bufsize,data,size) \
	set_ucbuffer_with_le_copy(buffer,bufsize,data,size,0)

#else	/* ! WORDS_BIGENDIAN */

#define le_to_native(dst,src,len)	memcpy(dst,src,len)
#define native_to_le(cp,len)		/* nothing */
#define	set_ucbuffer_with_le(buffer,bufsize,data,size) \
	(((void)(bufsize)),(UniChar*)(data))

#endif

static inline UniChar *set_ucbuffer_with_le_copy (
	UniChar *buffer, size_t *bufsize,
	const void *data, size_t size, size_t reserve)
{
	buffer = resize_buffer(buffer, bufsize, size+reserve);
	le_to_native((char*)buffer,data,size);
	return buffer;
}


/*
 * A simple hexdump function for debugging error conditions.
 */
#define	debug_out(s)	DEBUG(0,(s))

#ifdef DEBUG_STRINGS

static void hexdump( const char * label, const char * s, size_t len )
{
	size_t restlen = len;
	debug_out("<<<<<<<\n");
	debug_out(label);
	debug_out("\n");
	while (restlen > 0) {
		char line[100];
		size_t i, j;
		char * d = line;
#undef sprintf
		d += sprintf(d, "%04X ", (unsigned)(len-restlen));
		*d++ = ' ';
		for( i = 0; i<restlen && i<8; ++i ) {
			d += sprintf(d, "%02X ", ((unsigned)s[i]) & 0xFF);
		}
		for( j = i; j<8; ++j ) {
			d += sprintf(d, "   ");
		}
		*d++ = ' ';
		for( i = 8; i<restlen && i<16; ++i ) {
			d += sprintf(d, "%02X ", ((unsigned)s[i]) & 0xFF);
		}
		for( j = i; j<16; ++j ) {
			d += sprintf(d, "   ");
		}
		*d++ = ' ';
		for( i = 0; i<restlen && i<16; ++i ) {
			if(s[i] < ' ' || s[i] >= 0x7F || !isprint(s[i]))
				*d++ = '.';
			else
				*d++ = s[i];
		}
		*d++ = '\n';
		*d = 0;
		restlen -= i;
		s += i;
		debug_out(line);
	}
	debug_out(">>>>>>>\n");
}

#else	/* !DEBUG_STRINGS */

#define hexdump(label,s,len) /* nothing */

#endif


#if !USE_INTERNAL_API

/*
 * An implementation based on documented Mac OS X APIs.
 *
 * This does a certain amount of memory management, creating and
 * manipulating CFString objects.  We try to minimize the impact by
 * keeping those objects around and re-using them.  We also use
 * external backing store for the CFStrings where this is possible and
 * benficial.
 *
 * The Unicode normalizations forms available at this level are
 * generic, not specifically for the file system.  So they may not be
 * perfect fits.
 */
static size_t macosxfs_encoding_pull(
	void *cd,				/* Encoder handle */
	char **inbuf, size_t *inbytesleft,	/* Script string */
	char **outbuf, size_t *outbytesleft)	/* UTF-16-LE string */
{
	static const int script_code = kCFStringEncodingUTF8;
	static CFMutableStringRef cfstring = NULL;
	size_t outsize;
	CFRange range;

	(void) cd; /* UNUSED */

	if (0 == *inbytesleft) {
		return 0;
	}

	if (NULL == cfstring) {
		/*
		 * A version with an external backing store as in the
		 * push function should have been more efficient, but
		 * testing shows, that it is actually slower (!).
		 * Maybe kCFAllocatorDefault gets shortcut evaluation
		 * internally, while kCFAllocatorNull doesn't.
		 */
		cfstring = CFStringCreateMutable(kCFAllocatorDefault,0);
	}

	/*
	 * Three methods of appending to a CFString, choose the most
	 * efficient.
	 */
	if (0 == (*inbuf)[*inbytesleft-1]) {
		CFStringAppendCString(cfstring, *inbuf, script_code);
	} else if (*inbytesleft <= 255) {
		Str255 buffer;
		buffer[0] = *inbytesleft;
		memcpy(buffer+1, *inbuf, buffer[0]);
		CFStringAppendPascalString(cfstring, buffer, script_code);
	} else {
		/*
		 * We would like to use a fixed buffer and a loop
		 * here, but than we can't garantee that the input is
		 * well-formed UTF-8, as we are supposed to do.
		 */
		static char *buffer = NULL;
		static size_t buflen = 0;
		buffer = resize_buffer(buffer, &buflen, *inbytesleft+1);
		memcpy(buffer, *inbuf, *inbytesleft);
		buffer[*inbytesleft] = 0;
		CFStringAppendCString(cfstring, *inbuf, script_code);
	}

	/*
	 * Compose characters, using the non-canonical composition
	 * form.
	 */
	CFStringNormalize(cfstring, kCFStringNormalizationFormC);

	outsize = CFStringGetLength(cfstring);
	range = CFRangeMake(0,outsize);

	if (outsize == 0) {
		/*
		 * HACK: smbd/mangle_hash2.c:is_legal_name() expects
		 * errors here.  That function will always pass 2
		 * characters.  smbd/open.c:check_for_pipe() cuts a
		 * patchname to 10 characters blindly.  Suppress the
		 * debug output in those cases.
		 */
		if(2 != *inbytesleft && 10 != *inbytesleft) {
			debug_out("String conversion: "
				  "An unknown error occurred\n");
			hexdump("UTF8->UTF16LE (old) input",
				*inbuf, *inbytesleft);
		}
		errno = EILSEQ; /* Not sure, but this is what we have
				 * actually seen. */
		return -1;
	}
	if (outsize*2 > *outbytesleft) {
		CFStringDelete(cfstring, range);
		debug_out("String conversion: "
			  "Output buffer too small\n");
		hexdump("UTF8->UTF16LE (old) input",
			*inbuf, *inbytesleft);
		errno = E2BIG;
		return -1;
	}

        CFStringGetCharacters(cfstring, range, (UniChar*)*outbuf);
	CFStringDelete(cfstring, range);

	native_to_le(*outbuf, outsize*2);

	/*
	 * Add a converted null byte, if the CFString conversions
	 * prevented that until now.
	 */
	if (0 == (*inbuf)[*inbytesleft-1] && 
	    (0 != (*outbuf)[outsize*2-1] || 0 != (*outbuf)[outsize*2-2])) {

		if ((outsize*2+2) > *outbytesleft) {
			debug_out("String conversion: "
				  "Output buffer too small\n");
			hexdump("UTF8->UTF16LE (old) input",
				*inbuf, *inbytesleft);
			errno = E2BIG;
			return -1;
		}

		(*outbuf)[outsize*2] = (*outbuf)[outsize*2+1] = 0;
		outsize += 2;
	}

	*inbuf += *inbytesleft;
	*inbytesleft = 0;
	*outbuf += outsize*2;
	*outbytesleft -= outsize*2;

	return 0;
}

static size_t macosxfs_encoding_push(
	void *cd,				/* Encoder handle */
	char **inbuf, size_t *inbytesleft,	/* UTF-16-LE string */
	char **outbuf, size_t *outbytesleft)	/* Script string */
{
	static const int script_code = kCFStringEncodingUTF8;
	static CFMutableStringRef cfstring = NULL;
	static UniChar *buffer = NULL;
	static size_t buflen = 0;
	CFIndex outsize, cfsize, charsconverted;

	(void) cd; /* UNUSED */

	if (0 == *inbytesleft) {
		return 0;
	}

	/*
	 * We need a buffer that can hold 4 times the original data,
	 * because that is the theoretical maximum that decomposition
	 * can create currently (in Unicode 4.0).
	 */
	buffer = set_ucbuffer_with_le_copy(
		buffer, &buflen, *inbuf, *inbytesleft, 3 * *inbytesleft);

	if (NULL == cfstring) {
		cfstring = CFStringCreateMutableWithExternalCharactersNoCopy(
			kCFAllocatorDefault,
			buffer, *inbytesleft/2, buflen/2,
			kCFAllocatorNull);
	} else {
		CFStringSetExternalCharactersNoCopy(
			cfstring,
			buffer, *inbytesleft/2, buflen/2);
	}

	/*
	 * Decompose characters, using the non-canonical decomposition
	 * form.
	 *
	 * NB: This isn't exactly what HFS+ wants (see note on
	 * kCFStringEncodingUseHFSPlusCanonical in
	 * CFStringEncodingConverter.h), but AFAIK it's the best that
	 * the official API can do.
	 */
	CFStringNormalize(cfstring, kCFStringNormalizationFormD);

	cfsize = CFStringGetLength(cfstring);
	charsconverted = CFStringGetBytes(
		cfstring, CFRangeMake(0,cfsize),
		script_code, 0, False,
		*outbuf, *outbytesleft, &outsize);

	if (0 == charsconverted) {
		debug_out("String conversion: "
			  "Buffer too small or not convertable\n");
		hexdump("UTF16LE->UTF8 (old) input",
			*inbuf, *inbytesleft);
		errno = EILSEQ; /* Probably more likely. */
		return -1;
	}

	/*
	 * Add a converted null byte, if the CFString conversions
	 * prevented that until now.
	 */
	if (0 == (*inbuf)[*inbytesleft-1] && 0 == (*inbuf)[*inbytesleft-2] &&
	    (0 != (*outbuf)[outsize-1])) {

		if (((size_t)outsize+1) > *outbytesleft) {
			debug_out("String conversion: "
				  "Output buffer too small\n");
			hexdump("UTF16LE->UTF8 (old) input",
				*inbuf, *inbytesleft);
			errno = E2BIG;
			return -1;
		}

		(*outbuf)[outsize] = 0;
		++outsize;
	}

	*inbuf += *inbytesleft;
	*inbytesleft = 0;
	*outbuf += outsize;
	*outbytesleft -= outsize;

	return 0;
}

#else /* USE_INTERNAL_API */

/*
 * An implementation based on internal code as known from the
 * OpenDarwin CVS.
 *
 * This code doesn't need much memory management because it uses
 * functions that operate on the raw memory directly.
 *
 * The push routine here is faster and more compatible with HFS+ than
 * the other implementation above.  The pull routine is only faster
 * for some strings, slightly slower for others.  The pull routine
 * looses because it has to iterate over the data twice, once to
 * decode UTF-8 and than to do the character composition required by
 * Windows.
 */
static size_t macosxfs_encoding_pull(
	void *cd,				/* Encoder handle */
	char **inbuf, size_t *inbytesleft,	/* Script string */
	char **outbuf, size_t *outbytesleft)	/* UTF-16-LE string */
{
	static const int script_code = kCFStringEncodingUTF8;
	UInt32 srcCharsUsed = 0;
	UInt32 dstCharsUsed = 0;
	UInt32 result;
	uint32_t dstDecomposedUsed = 0;
	uint32_t dstPrecomposedUsed = 0;

	(void) cd; /* UNUSED */

	if (0 == *inbytesleft) {
		return 0;
	}

        result = CFStringEncodingBytesToUnicode(
		script_code, kCFStringEncodingComposeCombinings,
		*inbuf, *inbytesleft, &srcCharsUsed,
		(UniChar*)*outbuf, *outbytesleft, &dstCharsUsed);

	switch(result) {
	case kCFStringEncodingConversionSuccess:
		if (*inbytesleft == srcCharsUsed)
			break;
		else
			; /*fall through*/
	case kCFStringEncodingInsufficientOutputBufferLength:
		debug_out("String conversion: "
			  "Output buffer too small\n");
		hexdump("UTF8->UTF16LE (new) input",
			*inbuf, *inbytesleft);
		errno = E2BIG;
		return -1;
	case kCFStringEncodingInvalidInputStream:
		/*
		 * HACK: smbd/mangle_hash2.c:is_legal_name() expects
		 * errors here.  That function will always pass 2
		 * characters.  smbd/open.c:check_for_pipe() cuts a
		 * patchname to 10 characters blindly.  Suppress the
		 * debug output in those cases.
		 */
		if(2 != *inbytesleft && 10 != *inbytesleft) {
			debug_out("String conversion: "
				  "Invalid input sequence\n");
			hexdump("UTF8->UTF16LE (new) input",
				*inbuf, *inbytesleft);
		}
		errno = EILSEQ;
		return -1;
	case kCFStringEncodingConverterUnavailable:
		debug_out("String conversion: "
			  "Unknown encoding\n");
		hexdump("UTF8->UTF16LE (new) input",
			*inbuf, *inbytesleft);
		errno = EINVAL;
		return -1;
	}

	/*
	 * It doesn't look like CFStringEncodingBytesToUnicode() can
	 * produce precomposed characters (flags=ComposeCombinings
	 * doesn't do it), so we need another pass over the data here.
	 * We can do this in-place, as the string can only get
	 * shorter.
	 *
	 * (Actually in theory there should be an internal
	 * decomposition and reordering before the actual composition
	 * step.  But we should be able to rely on that we always get
	 * fully decomposed strings for input, so this can't create
	 * problems in reality.)
	 */
	CFUniCharPrecompose(
		(const UTF16Char *)*outbuf, dstCharsUsed, &dstDecomposedUsed,
		(UTF16Char *)*outbuf, dstCharsUsed, &dstPrecomposedUsed);

	native_to_le(*outbuf, dstPrecomposedUsed*2);

	*inbuf += srcCharsUsed;
	*inbytesleft -= srcCharsUsed;
	*outbuf += dstPrecomposedUsed*2;
	*outbytesleft -= dstPrecomposedUsed*2;

	return 0;
}

static size_t macosxfs_encoding_push(
	void *cd,				/* Encoder handle */
	char **inbuf, size_t *inbytesleft,	/* UTF-16-LE string */
	char **outbuf, size_t *outbytesleft)	/* Script string */
{
	static const int script_code = kCFStringEncodingUTF8;
	static UniChar *buffer = NULL;
	static size_t buflen = 0;
	UInt32 srcCharsUsed=0, dstCharsUsed=0, result;

	(void) cd; /* UNUSED */

	if (0 == *inbytesleft) {
		return 0;
	}

	buffer = set_ucbuffer_with_le(
		buffer, &buflen, *inbuf, *inbytesleft);

	result = CFStringEncodingUnicodeToBytes(
		script_code, kCFStringEncodingUseHFSPlusCanonical,
		buffer, *inbytesleft/2, &srcCharsUsed,
		*outbuf, *outbytesleft, &dstCharsUsed);

	switch(result) {
	case kCFStringEncodingConversionSuccess:
		if (*inbytesleft/2 == srcCharsUsed)
			break;
		else
			; /*fall through*/
	case kCFStringEncodingInsufficientOutputBufferLength:
		debug_out("String conversion: "
			  "Output buffer too small\n");
		hexdump("UTF16LE->UTF8 (new) input",
			*inbuf, *inbytesleft);
		errno = E2BIG;
		return -1;
	case kCFStringEncodingInvalidInputStream:
		/*
		 * HACK: smbd/open.c:check_for_pipe():is_legal_name()
		 * cuts a pathname to 10 characters blindly.  Suppress
		 * the debug output in those cases.
		 */
		if(10 != *inbytesleft) {
			debug_out("String conversion: "
				  "Invalid input sequence\n");
			hexdump("UTF16LE->UTF8 (new) input",
				*inbuf, *inbytesleft);
		}
		errno = EILSEQ;
		return -1;
	case kCFStringEncodingConverterUnavailable:
		debug_out("String conversion: "
			  "Unknown encoding\n");
		hexdump("UTF16LE->UTF8 (new) input",
			*inbuf, *inbytesleft);
		errno = EINVAL;
		return -1;
	}

	*inbuf += srcCharsUsed*2;
	*inbytesleft -= srcCharsUsed*2;
	*outbuf += dstCharsUsed;
	*outbytesleft -= dstCharsUsed;

	return 0;
}

#endif /* USE_INTERNAL_API */

/*
 * For initialization, actually install the encoding as "macosxfs".
 */
static struct charset_functions macosxfs_encoding_functions = {
	"MACOSXFS", macosxfs_encoding_pull, macosxfs_encoding_push
};

NTSTATUS init_module(void)
{
	return smb_register_charset(&macosxfs_encoding_functions);
}

/* eof */
