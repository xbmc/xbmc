/*
 * misc.c -- Miscellaneous routines.
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: misc.c,v 1.6 2003/09/29 19:50:24 bporter Exp $
 */

/********************************* Includes ***********************************/

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	"basic/basicInternal.h"
#endif

/*
 * 16 Sep 03 -- added option to use memcpy() instead of strncpy() in the
 * ascToUni and uniToAsc functions. 
 */
#define kUseMemcopy


/********************************* Defines ************************************/
/*
 *	Sprintf buffer structure. Make the increment 64 so that
 *	a balloc can use a 64 byte block.
 */

#define STR_REALLOC		0x1				/* Reallocate the buffer as required */
#define STR_INC			64				/* Growth increment */

typedef struct {
	char_t	*s;							/* Pointer to buffer */
	int		size;						/* Current buffer size */
	int		max;						/* Maximum buffer size */
	int		count;						/* Buffer count */
	int		flags;						/* Allocation flags */
} strbuf_t;

/*
 *	Sprintf formatting flags
 */
enum flag {
	flag_none = 0,
	flag_minus = 1,
	flag_plus = 2,
	flag_space = 4,
	flag_hash = 8,
	flag_zero = 16,
	flag_short = 32,
	flag_long = 64
};

/************************** Forward Declarations ******************************/

int 	dsnprintf(char_t **s, int size, char_t *fmt, va_list arg,
				int msize);
int	webs_strnlen(char_t *s, unsigned int n);
void	put_char(strbuf_t *buf, char_t c);
void	put_string(strbuf_t *buf, char_t *s, int len,
				int width, int prec, enum flag f);
void	put_ulong(strbuf_t *buf, unsigned long int value, int base,
				int upper, char_t *prefix, int width, int prec, enum flag f);

/************************************ Code ************************************/
/*
 *	"basename" returns a pointer to the last component of a pathname
 *  LINUX, LynxOS and Mac OS X have their own basename function
 */

#if (!defined (LINUX) && !defined (LYNX) && !defined (MACOSX))
char_t *basename(char_t *name)
{
	char_t	*cp;

#if (defined (NW) || defined (WIN))
	if (((cp = gstrrchr(name, '\\')) == NULL) &&
			((cp = gstrrchr(name, '/')) == NULL)) {
		return name;
#else
	if ((cp = gstrrchr(name, '/')) == NULL) {
		return name;
#endif
	} else if (*(cp + 1) == '\0' && cp == name) {
		return name;
	} else if (*(cp + 1) == '\0' && cp != name) {
		return T("");
	} else {
		return ++cp;
	}
}
#endif /* ! LINUX & ! LYNX & ! MACOSX */

/******************************************************************************/
/*
 *	Returns a pointer to the directory component of a pathname. bufsize is
 *	the size of the buffer in BYTES!
 */

char_t *dirname(char_t *buf, char_t *name, int bufsize)
{
	char_t *cp;
	int		len;

	a_assert(name);
	a_assert(buf);
	a_assert(bufsize > 0);

#if (defined (WIN) || defined (NW))
	if ((cp = gstrrchr(name, '/')) == NULL && 
		(cp = gstrrchr(name, '\\')) == NULL)
#else
	if ((cp = gstrrchr(name, '/')) == NULL)
#endif
	{
		gstrcpy(buf, T("."));
		return buf;
	}

	if ((*(cp + 1) == '\0' && cp == name)) {
		gstrncpy(buf, T("."), TSZ(bufsize));
		gstrcpy(buf, T("."));
		return buf;
	}

	len = cp - name;

	if (len < bufsize) {
		gstrncpy(buf, name, len);
		buf[len] = '\0';
	} else {
		gstrncpy(buf, name, TSZ(bufsize));
		buf[bufsize - 1] = '\0';
	}

	return buf;
}


/******************************************************************************/
/*
 *	sprintf and vsprintf are bad, ok. You can easily clobber memory. Use
 *	fmtAlloc and fmtValloc instead! These functions do _not_ support floating
 *	point, like %e, %f, %g...
 */

int fmtAlloc(char_t **s, int n, char_t *fmt, ...)
{
	va_list	ap;
	int		result;

	a_assert(s);
	a_assert(fmt);

	*s = NULL;
	va_start(ap, fmt);
	result = dsnprintf(s, n, fmt, ap, 0);
	va_end(ap);
	return result;
}

/******************************************************************************/
/*
 *	Support a static buffer version for small buffers only!
 */

int fmtStatic(char_t *s, int n, char_t *fmt, ...)
{
	va_list	ap;
	int		result;

	a_assert(s);
	a_assert(fmt);
	a_assert(n <= 256);

	if (n <= 0) {
		return -1;
	}
	va_start(ap, fmt);
	result = dsnprintf(&s, n, fmt, ap, 0);
	va_end(ap);
	return result;
}

/******************************************************************************/
/*
 *	This function appends the formatted string to the supplied string,
 *	reallocing if required.
 */

int fmtRealloc(char_t **s, int n, int msize, char_t *fmt, ...)
{
	va_list	ap;
	int		result;

	a_assert(s);
	a_assert(fmt);

	if (msize == -1) {
		*s = NULL;
	}
	va_start(ap, fmt);
	result = dsnprintf(s, n, fmt, ap, msize);
	va_end(ap);
	return result;
}

/******************************************************************************/
/*
 *	A vsprintf replacement.
 */

int fmtValloc(char_t **s, int n, char_t *fmt, va_list arg)
{
	a_assert(s);
	a_assert(fmt);

	*s = NULL;
	return dsnprintf(s, n, fmt, arg, 0);
}

/******************************************************************************/
/*
 *	Dynamic sprintf implementation. Supports dynamic buffer allocation.
 *	This function can be called multiple times to grow an existing allocated
 *	buffer. In this case, msize is set to the size of the previously allocated
 *	buffer. The buffer will be realloced, as required. If msize is set, we
 *	return the size of the allocated buffer for use with the next call. For
 *	the first call, msize can be set to -1.
 */

int dsnprintf(char_t **s, int size, char_t *fmt, va_list arg, int msize)
{
	strbuf_t	buf;
	char_t		c;

	a_assert(s);
	a_assert(fmt);

	memset(&buf, 0, sizeof(buf));
	buf.s = *s;

	if (*s == NULL || msize != 0) {
		buf.max = size;
		buf.flags |= STR_REALLOC;
		if (msize != 0) {
			buf.size = max(msize, 0);
		}
		if (*s != NULL && msize != 0) {
			buf.count = gstrlen(*s);
		}
	} else {
		buf.size = size;
	}

	while ((c = *fmt++) != '\0') {
		if (c != '%' || (c = *fmt++) == '%') {
			put_char(&buf, c);
		} else {
			enum flag f = flag_none;
			int width = 0;
			int prec = -1;
			for ( ; c != '\0'; c = *fmt++) {
				if (c == '-') { 
					f |= flag_minus; 
				} else if (c == '+') { 
					f |= flag_plus; 
				} else if (c == ' ') { 
					f |= flag_space; 
				} else if (c == '#') { 
					f |= flag_hash; 
				} else if (c == '0') { 
					f |= flag_zero; 
				} else {
					break;
				}
			}
			if (c == '*') {
				width = va_arg(arg, int);
				if (width < 0) {
					f |= flag_minus;
					width = -width;
				}
				c = *fmt++;
			} else {
				for ( ; gisdigit((int)c); c = *fmt++) {
					width = width * 10 + (c - '0');
				}
			}
			if (c == '.') {
				f &= ~flag_zero;
				c = *fmt++;
				if (c == '*') {
					prec = va_arg(arg, int);
					c = *fmt++;
				} else {
					for (prec = 0; gisdigit((int)c); c = *fmt++) {
						prec = prec * 10 + (c - '0');
					}
				}
			}
			if (c == 'h' || c == 'l') {
				f |= (c == 'h' ? flag_short : flag_long);
				c = *fmt++;
			}
			if (c == 'd' || c == 'i') {
				long int value;
				if (f & flag_short) {
					value = (short int) va_arg(arg, int);
				} else if (f & flag_long) {
					value = va_arg(arg, long int);
				} else {
					value = va_arg(arg, int);
				}
				if (value >= 0) {
					if (f & flag_plus) {
						put_ulong(&buf, value, 10, 0, T("+"), width, prec, f);
					} else if (f & flag_space) {
						put_ulong(&buf, value, 10, 0, T(" "), width, prec, f);
					} else {
						put_ulong(&buf, value, 10, 0, NULL, width, prec, f);
					}
				} else {
					put_ulong(&buf, -value, 10, 0, T("-"), width, prec, f);
				}
			} else if (c == 'o' || c == 'u' || c == 'x' || c == 'X') {
				unsigned long int value;
				if (f & flag_short) {
					value = (unsigned short int) va_arg(arg, unsigned int);
				} else if (f & flag_long) {
					value = va_arg(arg, unsigned long int);
				} else {
					value = va_arg(arg, unsigned int);
				}
				if (c == 'o') {
					if (f & flag_hash && value != 0) {
						put_ulong(&buf, value, 8, 0, T("0"), width, prec, f);
					} else {
						put_ulong(&buf, value, 8, 0, NULL, width, prec, f);
					}
				} else if (c == 'u') {
					put_ulong(&buf, value, 10, 0, NULL, width, prec, f);
				} else {
					if (f & flag_hash && value != 0) {
						if (c == 'x') {
							put_ulong(&buf, value, 16, 0, T("0x"), width, 
								prec, f);
						} else {
							put_ulong(&buf, value, 16, 1, T("0X"), width, 
								prec, f);
						}
					} else {
                  /* 04 Apr 02 BgP -- changed so that %X correctly outputs
                   * uppercase hex digits when requested.
						put_ulong(&buf, value, 16, 0, NULL, width, prec, f);
                   */
						put_ulong(&buf, value, 16, ('X' == c) , NULL, width, prec, f);
					}
				}

			} else if (c == 'c') {
				char_t value = va_arg(arg, int);
				put_char(&buf, value);

			} else if (c == 's' || c == 'S') {
				char_t *value = va_arg(arg, char_t *);
				if (value == NULL) {
					put_string(&buf, T("(null)"), -1, width, prec, f);
				} else if (f & flag_hash) {
					put_string(&buf,
						value + 1, (char_t) *value, width, prec, f);
				} else {
					put_string(&buf, value, -1, width, prec, f);
				}
			} else if (c == 'p') {
				void *value = va_arg(arg, void *);
				put_ulong(&buf,
					(unsigned long int) value, 16, 0, T("0x"), width, prec, f);
			} else if (c == 'n') {
				if (f & flag_short) {
					short int *value = va_arg(arg, short int *);
					*value = buf.count;
				} else if (f & flag_long) {
					long int *value = va_arg(arg, long int *);
					*value = buf.count;
				} else {
					int *value = va_arg(arg, int *);
					*value = buf.count;
				}
			} else {
				put_char(&buf, c);
			}
		}
	}
	if (buf.s == NULL) {
		put_char(&buf, '\0');
	}

/*
 *	If the user requested a dynamic buffer (*s == NULL), ensure it is returned.
 */
	if (*s == NULL || msize != 0) {
		*s = buf.s;
	}

	if (*s != NULL && size > 0) {
		if (buf.count < size) {
			(*s)[buf.count] = '\0';
		} else {
			(*s)[buf.size - 1] = '\0';
		}
	}

	if (msize != 0) {
		return buf.size;
	}
	return buf.count;
}

/******************************************************************************/
/*
 *	Return the length of a string limited by a given length
 */

int webs_strnlen(char_t *s, unsigned int n)
{
	unsigned int 	len;

	len = gstrlen(s);
	return min(len, n);
}

/******************************************************************************/
/*
 *	Add a character to a string buffer
 */

void put_char(strbuf_t *buf, char_t c)
{
	if (buf->count >= (buf->size - 1)) {
		if (! (buf->flags & STR_REALLOC)) {
			return;
		}
		buf->size += STR_INC;
		if (buf->size > buf->max && buf->size > STR_INC) {
/*
 *			Caller should increase the size of the calling buffer
 */
			buf->size -= STR_INC;
			return;
		}
		if (buf->s == NULL) {
			buf->s = balloc(B_L, buf->size * sizeof(char_t));
		} else {
			buf->s = brealloc(B_L, buf->s, buf->size * sizeof(char_t));
		}
	}
	buf->s[buf->count] = c;
	if (c != '\0') {
		++buf->count;
	}
}

/******************************************************************************/
/*
 *	Add a string to a string buffer
 */

void put_string(strbuf_t *buf, char_t *s, int len, int width,
		int prec, enum flag f)
{
	int		i;

	if (len < 0) { 
		len = webs_strnlen(s, prec >= 0 ? prec : ULONG_MAX); 
	} else if (prec >= 0 && prec < len) { 
		len = prec; 
	}
	if (width > len && !(f & flag_minus)) {
		for (i = len; i < width; ++i) { 
			put_char(buf, ' '); 
		}
	}
	for (i = 0; i < len; ++i) { 
		put_char(buf, s[i]); 
	}
	if (width > len && f & flag_minus) {
		for (i = len; i < width; ++i) { 
			put_char(buf, ' '); 
		}
	}
}

/******************************************************************************/
/*
 *	Add a long to a string buffer
 */

void put_ulong(strbuf_t *buf, unsigned long int value, int base,
		int upper, char_t *prefix, int width, int prec, enum flag f)
{
	unsigned long	x, x2;
	int				len, zeros, i;

	for (len = 1, x = 1; x < ULONG_MAX / base; ++len, x = x2) {
		x2 = x * base;
		if (x2 > value) { 
			break; 
		}
	}
	zeros = (prec > len) ? prec - len : 0;
	width -= zeros + len;
	if (prefix != NULL) { 
		width -= webs_strnlen(prefix, ULONG_MAX); 
	}
	if (!(f & flag_minus)) {
		if (f & flag_zero) {
			for (i = 0; i < width; ++i) { 
				put_char(buf, '0'); 
			}
		} else {
			for (i = 0; i < width; ++i) { 
				put_char(buf, ' '); 
			}
		}
	}
	if (prefix != NULL) { 
		put_string(buf, prefix, -1, 0, -1, flag_none); 
	}
	for (i = 0; i < zeros; ++i) { 
		put_char(buf, '0'); 
	}
	for ( ; x > 0; x /= base) {
		int digit = (value / x) % base;
		put_char(buf, (char) ((digit < 10 ? '0' : (upper ? 'A' : 'a') - 10) +
			digit));
	}
	if (f & flag_minus) {
		for (i = 0; i < width; ++i) { 
			put_char(buf, ' '); 
		}
	}
}

/******************************************************************************/
/*
 *	Convert an ansi string to a unicode string. On an error, we return the
 * 	original ansi string which is better than returning NULL. nBytes is the
 *	size of the destination buffer (ubuf) in _bytes_.
 */

char_t *ascToUni(char_t *ubuf, char *str, int nBytes)
{
#ifdef UNICODE
	if (MultiByteToWideChar(CP_ACP, 0, str, nBytes / sizeof(char_t), ubuf,
			nBytes / sizeof(char_t)) < 0) {
		return (char_t*) str;
	}
#else

#ifdef kUseMemcopy
   memcpy(ubuf, str, nBytes);
#else
	strncpy(ubuf, str, nBytes);
#endif /*kUseMemcopy*/
#endif
	return ubuf;
}

/******************************************************************************/
/*
 *	Convert a unicode string to an ansi string. On an error, return the
 *	original unicode string which is better than returning NULL.
 *	N.B. nBytes is the number of _bytes_ in the destination buffer, buf.
 */

char *uniToAsc(char *buf, char_t *ustr, int nBytes)
{
#ifdef UNICODE
   if (WideCharToMultiByte(CP_ACP, 0, ustr, nBytes, buf, nBytes, 
    NULL, NULL) < 0) 
   {
      return (char*) ustr;
   }
#else
#ifdef kUseMemcopy
   memcpy(buf, ustr, nBytes);
#else
   strncpy(buf, ustr, nBytes);
#endif /* kUseMemcopy */
#endif
   return (char*) buf;
}

/******************************************************************************/
/*
 *	allocate (balloc) a buffer and do ascii to unicode conversion into it.
 *	cp points to the ascii buffer.  alen is the length of the buffer to be
 *	converted not including a terminating NULL.  Return a pointer to the
 *	unicode buffer which must be bfree'd later.  Return NULL on failure to
 *	get buffer.  The buffer returned is NULL terminated.
 */

char_t *ballocAscToUni(char *cp, int alen)
{
	char_t *unip;
	int ulen;

	ulen = (alen + 1) * sizeof(char_t);
	if ((unip = balloc(B_L, ulen)) == NULL) {
		return NULL;
	}
	ascToUni(unip, cp, ulen);
	unip[alen] = 0;
	return unip;
}

/******************************************************************************/
/*
 *	allocate (balloc) a buffer and do unicode to ascii conversion into it.
 *	unip points to the unicoded string. ulen is the number of characters
 *	in the unicode string not including a teminating null.  Return a pointer
 *	to the ascii buffer which must be bfree'd later.  Return NULL on failure
 *	to get buffer.  The buffer returned is NULL terminated.
 */

char *ballocUniToAsc(char_t *unip, int ulen)
{
	char * cp;

	if ((cp = balloc(B_L, ulen+1)) == NULL) {
		return NULL;
	}
	uniToAsc(cp, unip, ulen);
	cp[ulen] = '\0';
	return cp;
}

/******************************************************************************/
/*
 *	convert a hex string to an integer. The end of the string or a non-hex
 *	character will indicate the end of the hex specification.
 */

unsigned int hextoi(char_t *hexstring)
{
	register char_t			*h;
	register unsigned int	c, v;

	v = 0;
	h = hexstring;
	if (*h == '0' && (*(h+1) == 'x' || *(h+1) == 'X')) {
		h += 2;
	}
	while ((c = (unsigned int)*h++) != 0) {
		if (c >= '0' && c <= '9') {
			c -= '0';
		} else if (c >= 'a' && c <= 'f') {
			c = (c - 'a') + 10;
		} else if (c >=  'A' && c <= 'F') {
			c = (c - 'A') + 10;
		} else {
			break;
		}
		v = (v * 0x10) + c;
	}
	return v;
}

/******************************************************************************/
/*
 *	convert a string to an integer. If the string starts with "0x" or "0X"
 *	a hexidecimal conversion is done.
 */

unsigned int gstrtoi(char_t *s)
{
	if (*s == '0' && (*(s+1) == 'x' || *(s+1) == 'X')) {
		s += 2;
		return hextoi(s);
	}
	return gatoi(s);
}

/******************************************************************************/


