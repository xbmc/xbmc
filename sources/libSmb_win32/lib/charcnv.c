/* 
   Unix SMB/CIFS implementation.
   Character set conversion Extensions
   Copyright (C) Igor Vergeichik <iverg@mail.ru> 2001
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Simo Sorce 2001
   Copyright (C) Martin Pool 2003
   
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
#include "includes.h"

/* We can parameterize this if someone complains.... JRA. */

char lp_failed_convert_char(void)
{
	return '_';
}

/**
 * @file
 *
 * @brief Character-set conversion routines built on our iconv.
 * 
 * @note Samba's internal character set (at least in the 3.0 series)
 * is always the same as the one for the Unix filesystem.  It is
 * <b>not</b> necessarily UTF-8 and may be different on machines that
 * need i18n filenames to be compatible with Unix software.  It does
 * have to be a superset of ASCII.  All multibyte sequences must start
 * with a byte with the high bit set.
 *
 * @sa lib/iconv.c
 */


static smb_iconv_t conv_handles[NUM_CHARSETS][NUM_CHARSETS];
static BOOL conv_silent; /* Should we do a debug if the conversion fails ? */

/**
 * Return the name of a charset to give to iconv().
 **/
static const char *charset_name(charset_t ch)
{
	const char *ret = NULL;

	if (ch == CH_UCS2) ret = "UTF-16LE";
	else if (ch == CH_UNIX) ret = lp_unix_charset();
	else if (ch == CH_DOS) ret = lp_dos_charset();
	else if (ch == CH_DISPLAY) ret = lp_display_charset();
	else if (ch == CH_UTF8) ret = "UTF8";

#if defined(HAVE_NL_LANGINFO) && defined(CODESET)
	if (ret && !strcmp(ret, "LOCALE")) {
		const char *ln = NULL;

#ifdef HAVE_SETLOCALE
		setlocale(LC_ALL, "");
#endif
		ln = nl_langinfo(CODESET);
		if (ln) {
			/* Check whether the charset name is supported
			   by iconv */
			smb_iconv_t handle = smb_iconv_open(ln,"UCS-2LE");
			if (handle == (smb_iconv_t) -1) {
				DEBUG(5,("Locale charset '%s' unsupported, using ASCII instead\n", ln));
				ln = NULL;
			} else {
				DEBUG(5,("Substituting charset '%s' for LOCALE\n", ln));
				smb_iconv_close(handle);
			}
		}
		ret = ln;
	}
#endif

	if (!ret || !*ret) ret = "ASCII";
	return ret;
}

void lazy_initialize_conv(void)
{
	static int initialized = False;

	if (!initialized) {
		initialized = True;
		load_case_tables();
		init_iconv();
	}
}

/**
 * Destroy global objects allocated by init_iconv()
 **/
void gfree_charcnv(void)
{
	int c1, c2;

	for (c1=0;c1<NUM_CHARSETS;c1++) {
		for (c2=0;c2<NUM_CHARSETS;c2++) {
			if ( conv_handles[c1][c2] ) {
				smb_iconv_close( conv_handles[c1][c2] );
				conv_handles[c1][c2] = 0;
			}
		}
	}
}

/**
 * Initialize iconv conversion descriptors.
 *
 * This is called the first time it is needed, and also called again
 * every time the configuration is reloaded, because the charset or
 * codepage might have changed.
 **/
void init_iconv(void)
{
	int c1, c2;
	BOOL did_reload = False;

	/* so that charset_name() works we need to get the UNIX<->UCS2 going
	   first */
	if (!conv_handles[CH_UNIX][CH_UCS2])
		conv_handles[CH_UNIX][CH_UCS2] = smb_iconv_open(charset_name(CH_UCS2), "ASCII");

	if (!conv_handles[CH_UCS2][CH_UNIX])
		conv_handles[CH_UCS2][CH_UNIX] = smb_iconv_open("ASCII", charset_name(CH_UCS2));

	for (c1=0;c1<NUM_CHARSETS;c1++) {
		for (c2=0;c2<NUM_CHARSETS;c2++) {
			const char *n1 = charset_name((charset_t)c1);
			const char *n2 = charset_name((charset_t)c2);
			if (conv_handles[c1][c2] &&
			    strcmp(n1, conv_handles[c1][c2]->from_name) == 0 &&
			    strcmp(n2, conv_handles[c1][c2]->to_name) == 0)
				continue;

			did_reload = True;

			if (conv_handles[c1][c2])
				smb_iconv_close(conv_handles[c1][c2]);

			conv_handles[c1][c2] = smb_iconv_open(n2,n1);
			if (conv_handles[c1][c2] == (smb_iconv_t)-1) {
				DEBUG(0,("init_iconv: Conversion from %s to %s not supported\n",
					 charset_name((charset_t)c1), charset_name((charset_t)c2)));
				if (c1 != CH_UCS2) {
					n1 = "ASCII";
				}
				if (c2 != CH_UCS2) {
					n2 = "ASCII";
				}
				DEBUG(0,("init_iconv: Attempting to replace with conversion from %s to %s\n",
					n1, n2 ));
				conv_handles[c1][c2] = smb_iconv_open(n2,n1);
				if (!conv_handles[c1][c2]) {
					DEBUG(0,("init_iconv: Conversion from %s to %s failed", n1, n2));
					smb_panic("init_iconv: conv_handle initialization failed.");
				}
			}
		}
	}

	if (did_reload) {
		/* XXX: Does this really get called every time the dos
		 * codepage changes? */
		/* XXX: Is the did_reload test too strict? */
		conv_silent = True;
		init_doschar_table();
		init_valid_table();
		conv_silent = False;
	}
}

/**
 * Convert string from one encoding to another, making error checking etc
 * Slow path version - uses (slow) iconv.
 *
 * @param src pointer to source string (multibyte or singlebyte)
 * @param srclen length of the source string in bytes
 * @param dest pointer to destination string (multibyte or singlebyte)
 * @param destlen maximal length allowed for string
 * @param allow_bad_conv determines if a "best effort" conversion is acceptable (never returns errors)
 * @returns the number of bytes occupied in the destination
 *
 * Ensure the srclen contains the terminating zero.
 *
 **/

static size_t convert_string_internal(charset_t from, charset_t to,
		      void const *src, size_t srclen, 
		      void *dest, size_t destlen, BOOL allow_bad_conv)
{
	size_t i_len, o_len;
	size_t retval;
	const char* inbuf = (const char*)src;
	char* outbuf = (char*)dest;
	smb_iconv_t descriptor;

	lazy_initialize_conv();

	descriptor = conv_handles[from][to];

	if (srclen == (size_t)-1) {
		if (from == CH_UCS2) {
			srclen = (strlen_w((const smb_ucs2_t *)src)+1) * 2;
		} else {
			srclen = strlen((const char *)src)+1;
		}
	}


	if (descriptor == (smb_iconv_t)-1 || descriptor == (smb_iconv_t)0) {
		if (!conv_silent)
			DEBUG(0,("convert_string_internal: Conversion not supported.\n"));
		return (size_t)-1;
	}

	i_len=srclen;
	o_len=destlen;

 again:

	retval = smb_iconv(descriptor, &inbuf, &i_len, &outbuf, &o_len);
	if(retval==(size_t)-1) {
	    	const char *reason="unknown error";
		switch(errno) {
			case EINVAL:
				reason="Incomplete multibyte sequence";
				if (!conv_silent)
					DEBUG(3,("convert_string_internal: Conversion error: %s(%s)\n",reason,inbuf));
				if (allow_bad_conv)
					goto use_as_is;
				break;
			case E2BIG:
				reason="No more room"; 
				if (!conv_silent) {
					if (from == CH_UNIX) {
						DEBUG(3,("E2BIG: convert_string(%s,%s): srclen=%u destlen=%u - '%s'\n",
							charset_name(from), charset_name(to),
							(unsigned int)srclen, (unsigned int)destlen, (const char *)src));
					} else {
						DEBUG(3,("E2BIG: convert_string(%s,%s): srclen=%u destlen=%u\n",
							charset_name(from), charset_name(to),
							(unsigned int)srclen, (unsigned int)destlen));
					}
				}
				break;
			case EILSEQ:
				reason="Illegal multibyte sequence";
				if (!conv_silent)
					DEBUG(3,("convert_string_internal: Conversion error: %s(%s)\n",reason,inbuf));
				if (allow_bad_conv)
					goto use_as_is;
				break;
			default:
				if (!conv_silent)
					DEBUG(0,("convert_string_internal: Conversion error: %s(%s)\n",reason,inbuf));
				break;
		}
		/* smb_panic(reason); */
	}
	return destlen-o_len;

 use_as_is:

	/* 
	 * Conversion not supported. This is actually an error, but there are so
	 * many misconfigured iconv systems and smb.conf's out there we can't just
	 * fail. Do a very bad conversion instead.... JRA.
	 */

	{
		if (o_len == 0 || i_len == 0)
			return destlen - o_len;

		if (from == CH_UCS2 && to != CH_UCS2) {
			/* Can't convert from ucs2 to multibyte. Replace with the default fail char. */
			if (i_len < 2)
				return destlen - o_len;
			if (i_len >= 2) {
				*outbuf = lp_failed_convert_char();

				outbuf++;
				o_len--;

				inbuf += 2;
				i_len -= 2;
			}

			if (o_len == 0 || i_len == 0)
				return destlen - o_len;

			/* Keep trying with the next char... */
			goto again;

		} else if (from != CH_UCS2 && to == CH_UCS2) {
			/* Can't convert to ucs2 - just widen by adding the default fail char then zero. */
			if (o_len < 2)
				return destlen - o_len;

			outbuf[0] = lp_failed_convert_char();
			outbuf[1] = '\0';

			inbuf++;
			i_len--;

			outbuf += 2;
			o_len -= 2;

			if (o_len == 0 || i_len == 0)
				return destlen - o_len;

			/* Keep trying with the next char... */
			goto again;

		} else if (from != CH_UCS2 && to != CH_UCS2) {
			/* Failed multibyte to multibyte. Just copy the default fail char and
				try again. */
			outbuf[0] = lp_failed_convert_char();

			inbuf++;
			i_len--;

			outbuf++;
			o_len--;

			if (o_len == 0 || i_len == 0)
				return destlen - o_len;

			/* Keep trying with the next char... */
			goto again;

		} else {
			/* Keep compiler happy.... */
			return destlen - o_len;
		}
	}
}

/**
 * Convert string from one encoding to another, making error checking etc
 * Fast path version - handles ASCII first.
 *
 * @param src pointer to source string (multibyte or singlebyte)
 * @param srclen length of the source string in bytes, or -1 for nul terminated.
 * @param dest pointer to destination string (multibyte or singlebyte)
 * @param destlen maximal length allowed for string - *NEVER* -1.
 * @param allow_bad_conv determines if a "best effort" conversion is acceptable (never returns errors)
 * @returns the number of bytes occupied in the destination
 *
 * Ensure the srclen contains the terminating zero.
 *
 * This function has been hand-tuned to provide a fast path.
 * Don't change unless you really know what you are doing. JRA.
 **/

size_t convert_string(charset_t from, charset_t to,
		      void const *src, size_t srclen, 
		      void *dest, size_t destlen, BOOL allow_bad_conv)
{
	/*
	 * NB. We deliberately don't do a strlen here if srclen == -1.
	 * This is very expensive over millions of calls and is taken
	 * care of in the slow path in convert_string_internal. JRA.
	 */

#ifdef DEVELOPER
	SMB_ASSERT(destlen != (size_t)-1);
#endif

	if (srclen == 0)
		return 0;

	if (from != CH_UCS2 && to != CH_UCS2) {
		const unsigned char *p = (const unsigned char *)src;
		unsigned char *q = (unsigned char *)dest;
		size_t slen = srclen;
		size_t dlen = destlen;
		unsigned char lastp = '\0';
		size_t retval = 0;

		/* If all characters are ascii, fast path here. */
		while (slen && dlen) {
			if ((lastp = *p) <= 0x7f) {
				*q++ = *p++;
				if (slen != (size_t)-1) {
					slen--;
				}
				dlen--;
				retval++;
				if (!lastp)
					break;
			} else {
#ifdef BROKEN_UNICODE_COMPOSE_CHARACTERS
				goto general_case;
#else
				return retval + convert_string_internal(from, to, p, slen, q, dlen, allow_bad_conv);
#endif
			}
		}
		if (!dlen) {
			/* Even if we fast path we should note if we ran out of room. */
			if (((slen != (size_t)-1) && slen) ||
					((slen == (size_t)-1) && lastp)) {
				errno = E2BIG;
			}
		}
		return retval;
	} else if (from == CH_UCS2 && to != CH_UCS2) {
		const unsigned char *p = (const unsigned char *)src;
		unsigned char *q = (unsigned char *)dest;
		size_t retval = 0;
		size_t slen = srclen;
		size_t dlen = destlen;
		unsigned char lastp = '\0';

		/* If all characters are ascii, fast path here. */
		while (((slen == (size_t)-1) || (slen >= 2)) && dlen) {
			if (((lastp = *p) <= 0x7f) && (p[1] == 0)) {
				*q++ = *p;
				if (slen != (size_t)-1) {
					slen -= 2;
				}
				p += 2;
				dlen--;
				retval++;
				if (!lastp)
					break;
			} else {
#ifdef BROKEN_UNICODE_COMPOSE_CHARACTERS
				goto general_case;
#else
				return retval + convert_string_internal(from, to, p, slen, q, dlen, allow_bad_conv);
#endif
			}
		}
		if (!dlen) {
			/* Even if we fast path we should note if we ran out of room. */
			if (((slen != (size_t)-1) && slen) ||
					((slen == (size_t)-1) && lastp)) {
				errno = E2BIG;
			}
		}
		return retval;
	} else if (from != CH_UCS2 && to == CH_UCS2) {
		const unsigned char *p = (const unsigned char *)src;
		unsigned char *q = (unsigned char *)dest;
		size_t retval = 0;
		size_t slen = srclen;
		size_t dlen = destlen;
		unsigned char lastp = '\0';

		/* If all characters are ascii, fast path here. */
		while (slen && (dlen >= 2)) {
			if ((lastp = *p) <= 0x7F) {
				*q++ = *p++;
				*q++ = '\0';
				if (slen != (size_t)-1) {
					slen--;
				}
				dlen -= 2;
				retval += 2;
				if (!lastp)
					break;
			} else {
#ifdef BROKEN_UNICODE_COMPOSE_CHARACTERS
				goto general_case;
#else
				return retval + convert_string_internal(from, to, p, slen, q, dlen, allow_bad_conv);
#endif
			}
		}
		if (!dlen) {
			/* Even if we fast path we should note if we ran out of room. */
			if (((slen != (size_t)-1) && slen) ||
					((slen == (size_t)-1) && lastp)) {
				errno = E2BIG;
			}
		}
		return retval;
	}

#ifdef BROKEN_UNICODE_COMPOSE_CHARACTERS
  general_case:
#endif
	return convert_string_internal(from, to, src, srclen, dest, destlen, allow_bad_conv);
}

/**
 * Convert between character sets, allocating a new buffer for the result.
 *
 * @param ctx TALLOC_CTX to use to allocate with. If NULL use malloc.
 * @param srclen length of source buffer.
 * @param dest always set at least to NULL
 * @note -1 is not accepted for srclen.
 *
 * @returns Size in bytes of the converted string; or -1 in case of error.
 *
 * Ensure the srclen contains the terminating zero.
 * 
 * I hate the goto's in this function. It's embarressing.....
 * There has to be a cleaner way to do this. JRA.
 **/

size_t convert_string_allocate(TALLOC_CTX *ctx, charset_t from, charset_t to,
			       void const *src, size_t srclen, void **dest, BOOL allow_bad_conv)
{
	size_t i_len, o_len, destlen = MAX(srclen, 512);
	size_t retval;
	const char *inbuf = (const char *)src;
	char *outbuf = NULL, *ob = NULL;
	smb_iconv_t descriptor;

	*dest = NULL;

	if (src == NULL || srclen == (size_t)-1)
		return (size_t)-1;
	if (srclen == 0)
		return 0;

	lazy_initialize_conv();

	descriptor = conv_handles[from][to];

	if (descriptor == (smb_iconv_t)-1 || descriptor == (smb_iconv_t)0) {
		if (!conv_silent)
			DEBUG(0,("convert_string_allocate: Conversion not supported.\n"));
		return (size_t)-1;
	}

  convert:

	if ((destlen*2) < destlen) {
		/* wrapped ! abort. */
		if (!conv_silent)
			DEBUG(0, ("convert_string_allocate: destlen wrapped !\n"));
		if (!ctx)
			SAFE_FREE(outbuf);
		return (size_t)-1;
	} else {
		destlen = destlen * 2;
	}

	if (ctx) {
		ob = (char *)TALLOC_REALLOC(ctx, ob, destlen);
	} else {
		ob = (char *)SMB_REALLOC(ob, destlen);
	}

	if (!ob) {
		DEBUG(0, ("convert_string_allocate: realloc failed!\n"));
		return (size_t)-1;
	}
	outbuf = ob;
	i_len = srclen;
	o_len = destlen;

 again:

	retval = smb_iconv(descriptor,
			   &inbuf, &i_len,
			   &outbuf, &o_len);
	if(retval == (size_t)-1) 		{
	    	const char *reason="unknown error";
		switch(errno) {
			case EINVAL:
				reason="Incomplete multibyte sequence";
				if (!conv_silent)
					DEBUG(3,("convert_string_allocate: Conversion error: %s(%s)\n",reason,inbuf));
				if (allow_bad_conv)
					goto use_as_is;
				break;
			case E2BIG:
				goto convert;		
			case EILSEQ:
				reason="Illegal multibyte sequence";
				if (!conv_silent)
					DEBUG(3,("convert_string_allocate: Conversion error: %s(%s)\n",reason,inbuf));
				if (allow_bad_conv)
					goto use_as_is;
				break;
		}
		if (!conv_silent)
			DEBUG(0,("Conversion error: %s(%s)\n",reason,inbuf));
		/* smb_panic(reason); */
		return (size_t)-1;
	}

  out:

	destlen = destlen - o_len;
	if (ctx) {
		ob = (char *)TALLOC_REALLOC(ctx,ob,destlen);
	} else {
		ob = (char *)SMB_REALLOC(ob,destlen);
	}

	if (destlen && !ob) {
		DEBUG(0, ("convert_string_allocate: out of memory!\n"));
		return (size_t)-1;
	}

	*dest = ob;
	return destlen;

 use_as_is:

	/* 
	 * Conversion not supported. This is actually an error, but there are so
	 * many misconfigured iconv systems and smb.conf's out there we can't just
	 * fail. Do a very bad conversion instead.... JRA.
	 */

	{
		if (o_len == 0 || i_len == 0)
			goto out;

		if (from == CH_UCS2 && to != CH_UCS2) {
			/* Can't convert from ucs2 to multibyte. Just use the default fail char. */
			if (i_len < 2)
				goto out;

			if (i_len >= 2) {
				*outbuf = lp_failed_convert_char();

				outbuf++;
				o_len--;

				inbuf += 2;
				i_len -= 2;
			}

			if (o_len == 0 || i_len == 0)
				goto out;

			/* Keep trying with the next char... */
			goto again;

		} else if (from != CH_UCS2 && to == CH_UCS2) {
			/* Can't convert to ucs2 - just widen by adding the default fail char then zero. */
			if (o_len < 2)
				goto out;

			outbuf[0] = lp_failed_convert_char();
			outbuf[1] = '\0';

			inbuf++;
			i_len--;

			outbuf += 2;
			o_len -= 2;

			if (o_len == 0 || i_len == 0)
				goto out;

			/* Keep trying with the next char... */
			goto again;

		} else if (from != CH_UCS2 && to != CH_UCS2) {
			/* Failed multibyte to multibyte. Just copy the default fail char and
				try again. */
			outbuf[0] = lp_failed_convert_char();

			inbuf++;
			i_len--;

			outbuf++;
			o_len--;

			if (o_len == 0 || i_len == 0)
				goto out;

			/* Keep trying with the next char... */
			goto again;

		} else {
			/* Keep compiler happy.... */
			goto out;
		}
	}
}

/**
 * Convert between character sets, allocating a new buffer using talloc for the result.
 *
 * @param srclen length of source buffer.
 * @param dest always set at least to NULL 
 * @note -1 is not accepted for srclen.
 *
 * @returns Size in bytes of the converted string; or -1 in case of error.
 **/
static size_t convert_string_talloc(TALLOC_CTX *ctx, charset_t from, charset_t to,
		      		void const *src, size_t srclen, void **dest, BOOL allow_bad_conv)
{
	size_t dest_len;

	*dest = NULL;
	dest_len=convert_string_allocate(ctx, from, to, src, srclen, dest, allow_bad_conv);
	if (dest_len == (size_t)-1)
		return (size_t)-1;
	if (*dest == NULL)
		return (size_t)-1;
	return dest_len;
}

size_t unix_strupper(const char *src, size_t srclen, char *dest, size_t destlen)
{
	size_t size;
	smb_ucs2_t *buffer;
	
	size = push_ucs2_allocate(&buffer, src);
	if (size == (size_t)-1) {
		smb_panic("failed to create UCS2 buffer");
	}
	if (!strupper_w(buffer) && (dest == src)) {
		free(buffer);
		return srclen;
	}
	
	size = convert_string(CH_UCS2, CH_UNIX, buffer, size, dest, destlen, True);
	free(buffer);
	return size;
}

/**
 strdup() a unix string to upper case.
 Max size is pstring.
**/

char *strdup_upper(const char *s)
{
	pstring out_buffer;
	const unsigned char *p = (const unsigned char *)s;
	unsigned char *q = (unsigned char *)out_buffer;

	/* this is quite a common operation, so we want it to be
	   fast. We optimise for the ascii case, knowing that all our
	   supported multi-byte character sets are ascii-compatible
	   (ie. they match for the first 128 chars) */

	while (1) {
		if (*p & 0x80)
			break;
		*q++ = toupper_ascii(*p);
		if (!*p)
			break;
		p++;
		if (p - ( const unsigned char *)s >= sizeof(pstring))
			break;
	}

	if (*p) {
		/* MB case. */
		size_t size;
		wpstring buffer;
		size = convert_string(CH_UNIX, CH_UCS2, s, -1, buffer, sizeof(buffer), True);
		if (size == (size_t)-1) {
			return NULL;
		}

		strupper_w(buffer);
	
		size = convert_string(CH_UCS2, CH_UNIX, buffer, -1, out_buffer, sizeof(out_buffer), True);
		if (size == (size_t)-1) {
			return NULL;
		}
	}

	return SMB_STRDUP(out_buffer);
}

size_t unix_strlower(const char *src, size_t srclen, char *dest, size_t destlen)
{
	size_t size;
	smb_ucs2_t *buffer = NULL;
	
	size = convert_string_allocate(NULL, CH_UNIX, CH_UCS2, src, srclen,
				       (void **)(void *)&buffer, True);
	if (size == (size_t)-1 || !buffer) {
		smb_panic("failed to create UCS2 buffer");
	}
	if (!strlower_w(buffer) && (dest == src)) {
		SAFE_FREE(buffer);
		return srclen;
	}
	size = convert_string(CH_UCS2, CH_UNIX, buffer, size, dest, destlen, True);
	SAFE_FREE(buffer);
	return size;
}

/**
 strdup() a unix string to lower case.
**/

char *strdup_lower(const char *s)
{
	size_t size;
	smb_ucs2_t *buffer = NULL;
	char *out_buffer;
	
	size = push_ucs2_allocate(&buffer, s);
	if (size == -1 || !buffer) {
		return NULL;
	}

	strlower_w(buffer);
	
	size = pull_ucs2_allocate(&out_buffer, buffer);
	SAFE_FREE(buffer);

	if (size == (size_t)-1) {
		return NULL;
	}
	
	return out_buffer;
}

static size_t ucs2_align(const void *base_ptr, const void *p, int flags)
{
	if (flags & (STR_NOALIGN|STR_ASCII))
		return 0;
	return PTR_DIFF(p, base_ptr) & 1;
}


/**
 * Copy a string from a char* unix src to a dos codepage string destination.
 *
 * @return the number of bytes occupied by the string in the destination.
 *
 * @param flags can include
 * <dl>
 * <dt>STR_TERMINATE</dt> <dd>means include the null termination</dd>
 * <dt>STR_UPPER</dt> <dd>means uppercase in the destination</dd>
 * </dl>
 *
 * @param dest_len the maximum length in bytes allowed in the
 * destination.  If @p dest_len is -1 then no maximum is used.
 **/
size_t push_ascii(void *dest, const char *src, size_t dest_len, int flags)
{
	size_t src_len = strlen(src);
	pstring tmpbuf;

	/* treat a pstring as "unlimited" length */
	if (dest_len == (size_t)-1)
		dest_len = sizeof(pstring);

	if (flags & STR_UPPER) {
		pstrcpy(tmpbuf, src);
		strupper_m(tmpbuf);
		src = tmpbuf;
	}

	if (flags & (STR_TERMINATE | STR_TERMINATE_ASCII))
		src_len++;

	return convert_string(CH_UNIX, CH_DOS, src, src_len, dest, dest_len, True);
}

size_t push_ascii_fstring(void *dest, const char *src)
{
	return push_ascii(dest, src, sizeof(fstring), STR_TERMINATE);
}

size_t push_ascii_pstring(void *dest, const char *src)
{
	return push_ascii(dest, src, sizeof(pstring), STR_TERMINATE);
}

/********************************************************************
 Push an nstring - ensure null terminated. Written by
 moriyama@miraclelinux.com (MORIYAMA Masayuki).
********************************************************************/

size_t push_ascii_nstring(void *dest, const char *src)
{
	size_t i, buffer_len, dest_len;
	smb_ucs2_t *buffer;

	conv_silent = True;
	buffer_len = push_ucs2_allocate(&buffer, src);
	if (buffer_len == (size_t)-1) {
		smb_panic("failed to create UCS2 buffer");
	}

	/* We're using buffer_len below to count ucs2 characters, not bytes. */
	buffer_len /= sizeof(smb_ucs2_t);

	dest_len = 0;
	for (i = 0; buffer[i] != 0 && (i < buffer_len); i++) {
		unsigned char mb[10];
		/* Convert one smb_ucs2_t character at a time. */
		size_t mb_len = convert_string(CH_UCS2, CH_DOS, buffer+i, sizeof(smb_ucs2_t), mb, sizeof(mb), False);
		if ((mb_len != (size_t)-1) && (dest_len + mb_len <= MAX_NETBIOSNAME_LEN - 1)) {
			memcpy((char *)dest + dest_len, mb, mb_len);
			dest_len += mb_len;
		} else {
			errno = E2BIG;
			break;
		}
	}
	((char *)dest)[dest_len] = '\0';

	SAFE_FREE(buffer);
	conv_silent = False;
	return dest_len;
}

/**
 * Copy a string from a dos codepage source to a unix char* destination.
 *
 * The resulting string in "dest" is always null terminated.
 *
 * @param flags can have:
 * <dl>
 * <dt>STR_TERMINATE</dt>
 * <dd>STR_TERMINATE means the string in @p src
 * is null terminated, and src_len is ignored.</dd>
 * </dl>
 *
 * @param src_len is the length of the source area in bytes.
 * @returns the number of bytes occupied by the string in @p src.
 **/
size_t pull_ascii(char *dest, const void *src, size_t dest_len, size_t src_len, int flags)
{
	size_t ret;

	if (dest_len == (size_t)-1)
		dest_len = sizeof(pstring);

	if (flags & STR_TERMINATE) {
		if (src_len == (size_t)-1) {
			src_len = strlen(src) + 1;
		} else {
			size_t len = strnlen(src, src_len);
			if (len < src_len)
				len++;
			src_len = len;
		}
	}

	ret = convert_string(CH_DOS, CH_UNIX, src, src_len, dest, dest_len, True);
	if (ret == (size_t)-1) {
		dest_len = 0;
	}

	if (dest_len)
		dest[MIN(ret, dest_len-1)] = 0;
	else 
		dest[0] = 0;

	return src_len;
}

size_t pull_ascii_pstring(char *dest, const void *src)
{
	return pull_ascii(dest, src, sizeof(pstring), -1, STR_TERMINATE);
}

size_t pull_ascii_fstring(char *dest, const void *src)
{
	return pull_ascii(dest, src, sizeof(fstring), -1, STR_TERMINATE);
}

/* When pulling an nstring it can expand into a larger size (dos cp -> utf8). Cope with this. */

size_t pull_ascii_nstring(char *dest, size_t dest_len, const void *src)
{
	return pull_ascii(dest, src, dest_len, sizeof(nstring)-1, STR_TERMINATE);
}

/**
 * Copy a string from a char* src to a unicode destination.
 *
 * @returns the number of bytes occupied by the string in the destination.
 *
 * @param flags can have:
 *
 * <dl>
 * <dt>STR_TERMINATE <dd>means include the null termination.
 * <dt>STR_UPPER     <dd>means uppercase in the destination.
 * <dt>STR_NOALIGN   <dd>means don't do alignment.
 * </dl>
 *
 * @param dest_len is the maximum length allowed in the
 * destination. If dest_len is -1 then no maxiumum is used.
 **/

size_t push_ucs2(const void *base_ptr, void *dest, const char *src, size_t dest_len, int flags)
{
	size_t len=0;
	size_t src_len;
	size_t ret;

	/* treat a pstring as "unlimited" length */
	if (dest_len == (size_t)-1)
		dest_len = sizeof(pstring);

	if (flags & STR_TERMINATE)
		src_len = (size_t)-1;
	else
		src_len = strlen(src);

	if (ucs2_align(base_ptr, dest, flags)) {
		*(char *)dest = 0;
		dest = (void *)((char *)dest + 1);
		if (dest_len)
			dest_len--;
		len++;
	}

	/* ucs2 is always a multiple of 2 bytes */
	dest_len &= ~1;

	ret =  convert_string(CH_UNIX, CH_UCS2, src, src_len, dest, dest_len, True);
	if (ret == (size_t)-1) {
		return 0;
	}

	len += ret;

	if (flags & STR_UPPER) {
		smb_ucs2_t *dest_ucs2 = dest;
		size_t i;
		for (i = 0; i < (dest_len / 2) && dest_ucs2[i]; i++) {
			smb_ucs2_t v = toupper_w(dest_ucs2[i]);
			if (v != dest_ucs2[i]) {
				dest_ucs2[i] = v;
			}
		}
	}

	return len;
}


/**
 * Copy a string from a unix char* src to a UCS2 destination,
 * allocating a buffer using talloc().
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 *         or -1 in case of error.
 **/
size_t push_ucs2_talloc(TALLOC_CTX *ctx, smb_ucs2_t **dest, const char *src)
{
	size_t src_len = strlen(src)+1;

	*dest = NULL;
	return convert_string_talloc(ctx, CH_UNIX, CH_UCS2, src, src_len, (void **)dest, True);
}


/**
 * Copy a string from a unix char* src to a UCS2 destination, allocating a buffer
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 *         or -1 in case of error.
 **/

size_t push_ucs2_allocate(smb_ucs2_t **dest, const char *src)
{
	size_t src_len = strlen(src)+1;

	*dest = NULL;
	return convert_string_allocate(NULL, CH_UNIX, CH_UCS2, src, src_len, (void **)dest, True);
}

/**
 Copy a string from a char* src to a UTF-8 destination.
 Return the number of bytes occupied by the string in the destination
 Flags can have:
  STR_TERMINATE means include the null termination
  STR_UPPER     means uppercase in the destination
 dest_len is the maximum length allowed in the destination. If dest_len
 is -1 then no maxiumum is used.
**/

static size_t push_utf8(void *dest, const char *src, size_t dest_len, int flags)
{
	size_t src_len = strlen(src);
	pstring tmpbuf;

	/* treat a pstring as "unlimited" length */
	if (dest_len == (size_t)-1)
		dest_len = sizeof(pstring);

	if (flags & STR_UPPER) {
		pstrcpy(tmpbuf, src);
		strupper_m(tmpbuf);
		src = tmpbuf;
	}

	if (flags & STR_TERMINATE)
		src_len++;

	return convert_string(CH_UNIX, CH_UTF8, src, src_len, dest, dest_len, True);
}

size_t push_utf8_fstring(void *dest, const char *src)
{
	return push_utf8(dest, src, sizeof(fstring), STR_TERMINATE);
}

/**
 * Copy a string from a unix char* src to a UTF-8 destination, allocating a buffer using talloc
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 **/

size_t push_utf8_talloc(TALLOC_CTX *ctx, char **dest, const char *src)
{
	size_t src_len = strlen(src)+1;

	*dest = NULL;
	return convert_string_talloc(ctx, CH_UNIX, CH_UTF8, src, src_len, (void**)dest, True);
}

/**
 * Copy a string from a unix char* src to a UTF-8 destination, allocating a buffer
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 **/

size_t push_utf8_allocate(char **dest, const char *src)
{
	size_t src_len = strlen(src)+1;

	*dest = NULL;
	return convert_string_allocate(NULL, CH_UNIX, CH_UTF8, src, src_len, (void **)dest, True);	
}

/**
 Copy a string from a ucs2 source to a unix char* destination.
 Flags can have:
  STR_TERMINATE means the string in src is null terminated.
  STR_NOALIGN   means don't try to align.
 if STR_TERMINATE is set then src_len is ignored if it is -1.
 src_len is the length of the source area in bytes
 Return the number of bytes occupied by the string in src.
 The resulting string in "dest" is always null terminated.
**/

size_t pull_ucs2(const void *base_ptr, char *dest, const void *src, size_t dest_len, size_t src_len, int flags)
{
	size_t ret;

	if (dest_len == (size_t)-1)
		dest_len = sizeof(pstring);

	if (ucs2_align(base_ptr, src, flags)) {
		src = (const void *)((const char *)src + 1);
		if (src_len != (size_t)-1)
			src_len--;
	}

	if (flags & STR_TERMINATE) {
		/* src_len -1 is the default for null terminated strings. */
		if (src_len != (size_t)-1) {
			size_t len = strnlen_w(src, src_len/2);
			if (len < src_len/2)
				len++;
			src_len = len*2;
		}
	}

	/* ucs2 is always a multiple of 2 bytes */
	if (src_len != (size_t)-1)
		src_len &= ~1;
	
	ret = convert_string(CH_UCS2, CH_UNIX, src, src_len, dest, dest_len, True);
	if (ret == (size_t)-1) {
		return 0;
	}

	if (src_len == (size_t)-1)
		src_len = ret*2;
		
	if (dest_len)
		dest[MIN(ret, dest_len-1)] = 0;
	else 
		dest[0] = 0;

	return src_len;
}

size_t pull_ucs2_pstring(char *dest, const void *src)
{
	return pull_ucs2(NULL, dest, src, sizeof(pstring), -1, STR_TERMINATE);
}

size_t pull_ucs2_fstring(char *dest, const void *src)
{
	return pull_ucs2(NULL, dest, src, sizeof(fstring), -1, STR_TERMINATE);
}

/**
 * Copy a string from a UCS2 src to a unix char * destination, allocating a buffer using talloc
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 **/

size_t pull_ucs2_talloc(TALLOC_CTX *ctx, char **dest, const smb_ucs2_t *src)
{
	size_t src_len = (strlen_w(src)+1) * sizeof(smb_ucs2_t);
	*dest = NULL;
	return convert_string_talloc(ctx, CH_UCS2, CH_UNIX, src, src_len, (void **)dest, True);
}

/**
 * Copy a string from a UCS2 src to a unix char * destination, allocating a buffer
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 **/

size_t pull_ucs2_allocate(char **dest, const smb_ucs2_t *src)
{
	size_t src_len = (strlen_w(src)+1) * sizeof(smb_ucs2_t);
	*dest = NULL;
	return convert_string_allocate(NULL, CH_UCS2, CH_UNIX, src, src_len, (void **)dest, True);
}

/**
 * Copy a string from a UTF-8 src to a unix char * destination, allocating a buffer using talloc
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 **/

size_t pull_utf8_talloc(TALLOC_CTX *ctx, char **dest, const char *src)
{
	size_t src_len = strlen(src)+1;
	*dest = NULL;
	return convert_string_talloc(ctx, CH_UTF8, CH_UNIX, src, src_len, (void **)dest, True);
}

/**
 * Copy a string from a UTF-8 src to a unix char * destination, allocating a buffer
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 **/

size_t pull_utf8_allocate(char **dest, const char *src)
{
	size_t src_len = strlen(src)+1;
	*dest = NULL;
	return convert_string_allocate(NULL, CH_UTF8, CH_UNIX, src, src_len, (void **)dest, True);
}
 
/**
 * Copy a string from a DOS src to a unix char * destination, allocating a buffer using talloc
 *
 * @param dest always set at least to NULL 
 *
 * @returns The number of bytes occupied by the string in the destination
 **/

size_t pull_ascii_talloc(TALLOC_CTX *ctx, char **dest, const char *src)
{
	size_t src_len = strlen(src)+1;
	*dest = NULL;
	return convert_string_talloc(ctx, CH_DOS, CH_UNIX, src, src_len, (void **)dest, True);
}

/**
 Copy a string from a char* src to a unicode or ascii
 dos codepage destination choosing unicode or ascii based on the 
 flags in the SMB buffer starting at base_ptr.
 Return the number of bytes occupied by the string in the destination.
 flags can have:
  STR_TERMINATE means include the null termination.
  STR_UPPER     means uppercase in the destination.
  STR_ASCII     use ascii even with unicode packet.
  STR_NOALIGN   means don't do alignment.
 dest_len is the maximum length allowed in the destination. If dest_len
 is -1 then no maxiumum is used.
**/

size_t push_string_fn(const char *function, unsigned int line, const void *base_ptr, void *dest, const char *src, size_t dest_len, int flags)
{
#ifdef DEVELOPER
	/* We really need to zero fill here, not clobber
	 * region, as we want to ensure that valgrind thinks
	 * all of the outgoing buffer has been written to
	 * so a send() or write() won't trap an error.
	 * JRA.
	 */
#if 0
	if (dest_len != (size_t)-1)
		clobber_region(function, line, dest, dest_len);
#else
	if (dest_len != (size_t)-1)
		memset(dest, '\0', dest_len);
#endif
#endif

	if (!(flags & STR_ASCII) && \
	    ((flags & STR_UNICODE || \
	      (SVAL(base_ptr, smb_flg2) & FLAGS2_UNICODE_STRINGS)))) {
		return push_ucs2(base_ptr, dest, src, dest_len, flags);
	}
	return push_ascii(dest, src, dest_len, flags);
}


/**
 Copy a string from a unicode or ascii source (depending on
 the packet flags) to a char* destination.
 Flags can have:
  STR_TERMINATE means the string in src is null terminated.
  STR_UNICODE   means to force as unicode.
  STR_ASCII     use ascii even with unicode packet.
  STR_NOALIGN   means don't do alignment.
 if STR_TERMINATE is set then src_len is ignored is it is -1
 src_len is the length of the source area in bytes.
 Return the number of bytes occupied by the string in src.
 The resulting string in "dest" is always null terminated.
**/

size_t pull_string_fn(const char *function, unsigned int line, const void *base_ptr, char *dest, const void *src, size_t dest_len, size_t src_len, int flags)
{
#ifdef DEVELOPER
	if (dest_len != (size_t)-1)
		clobber_region(function, line, dest, dest_len);
#endif

	if (!(flags & STR_ASCII) && \
	    ((flags & STR_UNICODE || \
	      (SVAL(base_ptr, smb_flg2) & FLAGS2_UNICODE_STRINGS)))) {
		return pull_ucs2(base_ptr, dest, src, dest_len, src_len, flags);
	}
	return pull_ascii(dest, src, dest_len, src_len, flags);
}

size_t align_string(const void *base_ptr, const char *p, int flags)
{
	if (!(flags & STR_ASCII) && \
	    ((flags & STR_UNICODE || \
	      (SVAL(base_ptr, smb_flg2) & FLAGS2_UNICODE_STRINGS)))) {
		return ucs2_align(base_ptr, p, flags);
	}
	return 0;
}

/****************************************************************
 Calculate the size (in bytes) of the next multibyte character in
 our internal character set. Note that p must be pointing to a
 valid mb char, not within one.
****************************************************************/

size_t next_mb_char_size(const char *s)
{
	size_t i;

	if (!(*s & 0x80))
		return 1; /* ascii. */

	conv_silent = True;
	for ( i = 1; i <=4; i++ ) {
		smb_ucs2_t uc;
		if (convert_string(CH_UNIX, CH_UCS2, s, i, &uc, 2, False) == 2) {
#if 0 /* JRATEST */
			DEBUG(10,("next_mb_char_size: size %u at string %s\n",
				(unsigned int)i, s));
#endif
			conv_silent = False;
			return i;
		}
	}
	/* We're hosed - we don't know how big this is... */
	DEBUG(10,("next_mb_char_size: unknown size at string %s\n", s));
	conv_silent = False;
	return 1;
}
