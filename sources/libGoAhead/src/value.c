/*
 * value.c -- Generic type (holds all types)
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * $Id: value.c,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	This module provides a generic type that can hold all possible types.
 *	It is designed to provide maximum effeciency.
 */

/********************************* Includes ***********************************/

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	"basic/basicInternal.h"
#endif

/*********************************** Locals ***********************************/
#ifndef UEMF
static value_t value_null;					/* All zeros */

/***************************** Forward Declarations ***************************/

static void	coerce_types(value_t* v1, value_t* v2);
static int	value_to_integer(value_t* vp);
#endif /*!UEMF*/
/*********************************** Code *************************************/
/*
 *	Initialize a integer value.
 */

value_t valueInteger(long value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = integer;
	v.value.integer = value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a string value.
 */

value_t valueString(char_t* value, int flags)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = vtype_string;
	if (flags & VALUE_ALLOCATE) {
		v.allocated = 1;
		v.value.string = gstrdup(B_L, value);
	} else {
		v.allocated = 0;
		v.value.string = value;
	}
	return v;
}

/******************************************************************************/
/*
 *	Free any storage allocated for a value.
 */

void valueFree(value_t* v)
{
	if (v->valid && v->allocated && v->type == vtype_string &&
			v->value.string != NULL) {
		bfree(B_L, v->value.string);
	}
#ifndef UEMF
	if (v->valid && v->type == symbol && v->value.symbol.data != NULL &&
			v->value.symbol.freeCb !=NULL) {
		v->value.symbol.freeCb(v->value.symbol.data);
	}
#endif
	v->type = undefined;
	v->valid = 0;
	v->allocated = 0;
}

#ifndef UEMF

/******************************************************************************/
/*
 *	Initialize an invalid value.
 */

value_t valueInvalid()
{
	value_t	v;
	v.valid = 0;
	v.type = undefined;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a flag value.
 */

value_t valueBool(int value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.type = flag;
	v.valid = 1;
	v.value.flag = (char) value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a byteint value.
 */

value_t valueByteint(char value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = byteint;
	v.value.byteint = value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a shortint value.
 */

value_t valueShortint(short value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = shortint;
	v.value.shortint = value;
	return v;
}

#ifdef FLOATING_POINT_SUPPORT
/******************************************************************************/
/*
 *	Initialize a floating value.
 */

value_t valueFloating(double value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = floating;
	v.value.floating = value;
	return v;
}
#endif /* FLOATING_POINT_SUPPORT */

/******************************************************************************/
/*
 *	Initialize a big value.
 */

value_t valueBig(long high_word, long low_word)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = big;
	v.value.big[BLOW] = low_word;
	v.value.big[BHIGH] = high_word;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a hex value.
 */

value_t valueHex(int value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = hex;
	v.value.integer = value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a octal value.
 */

value_t valueOctal(int value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = octal;
	v.value.integer = value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a percent value.
 */

value_t valuePercent(int value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = percent;
	v.value.percent = (char) value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize an byte array. Note: no allocation, just store the ptr
 */

value_t valueBytes(char* value, int flags)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = bytes;
	if (flags & VALUE_ALLOCATE) {
		v.allocated = 1;
		v.value.bytes = bstrdupA(B_L, value);
	} else {
		v.allocated = 0;
		v.value.bytes = value;
	}
	return v;
}

/******************************************************************************/
/*
 *	Initialize a symbol value.
 *	Value parameter can hold a pointer to any type of value
 *	Free parameter can be NULL, or a function pointer to a function that will
 *		free the value
 */

value_t valueSymbol(void *value, freeCallback freeCb)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = symbol;
	v.value.symbol.data = value;
	v.value.symbol.freeCb = freeCb;
	return v;
}

/******************************************************************************/
/*
 *	Initialize an error message value.
 */

value_t valueErrmsg(char_t* value)
{
	value_t	v;

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;
	v.type = errmsg;
	v.value.errmsg = value;
	return v;
}

/******************************************************************************/
/*
 *	Copy a value. If the type is 'string' then allocate another string.
 *	Note: we allow the copy of a null value.
 */

value_t valueCopy(value_t v2)
{
	value_t		v1;

	v1 = v2;
	if (v2.valid && v2.type == string && v2.value.string != NULL) {
		v1.value.string = gstrdup(B_L, v2.value.string);
		v1.allocated = 1;
	}
	return v1;
}


/******************************************************************************/
/*
 *	Add a value.
 */

value_t valueAdd(value_t v1, value_t v2)
{
	value_t	v;

	a_assert(v1.valid);
	a_assert(v2.valid);

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;

	if (v1.type != v2.type)
		coerce_types(&v1, &v2);

	switch (v1.type) {
	default:
	case string:
	case bytes:
		a_assert(0);
		break;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		v1.value.floating += v2.value.floating;
		return v1;
#endif

	case flag:
		v1.value.bool |= v2.value.flag;
		return v1;

	case byteint:
	case percent:
		v1.value.byteint += v2.value.byteint;
		return v1;

	case shortint:
		v1.value.shortint += v2.value.shortint;
		return v1;

	case hex:
	case integer:
	case octal:
		v1.value.integer += v2.value.integer;
		return v1;

	case big:
		v.type = big;
		badd(v.value.big, v1.value.big, v2.value.big);
		return v;
	}

	return v1;
}


/******************************************************************************/
/*
 *	Subtract a value.
 */

value_t valueSub(value_t v1, value_t v2)
{
	value_t	v;

	a_assert(v1.valid);
	a_assert(v2.valid);

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;

	if (v1.type != v2.type)
		coerce_types(&v1, &v2);
	switch (v1.type) {
	default:
		a_assert(0);
		break;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		v1.value.floating -= v2.value.floating;
		return v1;
#endif

	case flag:
		v1.value.flag &= v2.value.flag;
		return v1;

	case byteint:
	case percent:
		v1.value.byteint -= v2.value.byteint;
		return v1;

	case shortint:
		v1.value.shortint -= v2.value.shortint;
		return v1;

	case hex:
	case integer:
	case octal:
		v1.value.integer -= v2.value.integer;
		return v1;

	case big:
		v.type = big;
		bsub(v.value.big, v1.value.big, v2.value.big);
		return v;
	}

	return v1;
}


/******************************************************************************/
/*
 *	Multiply a value.
 */

value_t valueMul(value_t v1, value_t v2)
{
	value_t	v;

	a_assert(v1.valid);
	a_assert(v2.valid);

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;

	if (v1.type != v2.type)
		coerce_types(&v1, &v2);
	switch (v1.type) {
	default:
		a_assert(0);
		break;

	case flag:
		a_assert(v1.type != flag);
		break;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		v1.value.floating *= v2.value.floating;
		return v1;
#endif

	case byteint:
	case percent:
		v1.value.byteint *= v2.value.byteint;
		return v1;

	case shortint:
		v1.value.shortint *= v2.value.shortint;
		return v1;

	case hex:
	case integer:
	case octal:
		v1.value.integer *= v2.value.integer;
		return v1;

	case big:
		v.type = big;
		bmul(v.value.big, v1.value.big, v2.value.big);
		return v;
	}

	return v1;
}


/******************************************************************************/
/*
 *	Divide a value.
 */

value_t valueDiv(value_t v1, value_t v2)
{
	value_t	v;

	a_assert(v1.valid);
	a_assert(v2.valid);

	memset(&v, 0x0, sizeof(v));
	v.valid = 1;

	if (v1.type != v2.type)
		coerce_types(&v1, &v2);
	switch (v1.type) {
	default:
		a_assert(0);
		break;

	case flag:
		a_assert(v1.type != flag);
		break;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		v1.value.floating /= v2.value.floating;
		return v1;
#endif

	case byteint:
	case percent:
		v1.value.byteint /= v2.value.byteint;
		return v1;

	case shortint:
		v1.value.shortint /= v2.value.shortint;
		return v1;

	case hex:
	case integer:
	case octal:
		v1.value.integer /= v2.value.integer;
		return v1;

	case big:
		v.type = big;
		bdiv(v.value.big, v1.value.big, v2.value.big);
		return v;
	}

	return v1;
}


/******************************************************************************/
/*
 *	Compare a value.
 */

int valueCmp(value_t v1, value_t v2)
{
	a_assert(v1.valid);
	a_assert(v2.valid);

	if (v1.type != v2.type)
		coerce_types(&v1, &v2);
	if (v1.type != v2.type) {
/*
 *		Make v2 == v1
 */
		a_assert(v1.type == v2.type);
		v2 = v1;
		return 0;
	}
	switch (v1.type) {
	case string:
		if (v1.value.string == NULL && v2.value.string == NULL) {
			return 0;
		} else if (v1.value.string == NULL) {
			return -1;
		} else if (v2.value.string == NULL) {
			return 1;
		} else {
			return gstrcmp(v1.value.string, v2.value.string);
		}
		/* Nobody here */

	case flag:
		if (v1.value.flag < v2.value.flag)
			return -1;
		else if (v1.value.flag == v2.value.flag)
			return 0;
		else return 1;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		if (v1.value.floating < v2.value.floating)
			return -1;
		else if (v1.value.floating == v2.value.floating)
			return 0;
		else return 1;
#endif

	case byteint:
	case percent:
		if (v1.value.byteint < v2.value.byteint)
			return -1;
		else if (v1.value.byteint == v2.value.byteint)
			return 0;
		else return 1;

	case shortint:
		if (v1.value.shortint < v2.value.shortint)
			return -1;
		else if (v1.value.shortint == v2.value.shortint)
			return 0;
		else return 1;

	case hex:
	case integer:
	case octal:
		if (v1.value.integer < v2.value.integer)
			return -1;
		else if (v1.value.integer == v2.value.integer)
			return 0;
		else return 1;

	case big:
		return bcompare(v1.value.big, v2.value.big);

	default:
		a_assert(0);
		return 0;
	}
}


/******************************************************************************/
/*
 *	If type mismatch, then coerce types to big.
 *	Note: Known bug, casting of negative bigs to floats doesn't work.
 */

static void coerce_types(register value_t* v1, register value_t* v2)
{
#ifdef FLOATING_POINT_SUPPORT
	if (v1->type == floating) {
		v2->type = floating;
		v2->value.floating = (double) v2->value.integer;
		if (v2->type == big)
			v2->value.floating = (double) v2->value.big[BLOW] +
				(double) v2->value.big[BHIGH] * (double) MAXINT;

	} else if (v2->type == floating) {
		v1->type = floating;
		v1->value.floating = (double) v1->value.integer;
		if (v1->type == big)
			v1->value.floating = (double) v1->value.big[BLOW] +
				(double) v1->value.big[BHIGH] * (double) MAXINT;

	} else if (v1->type == big) {
#else
	if (v1->type == big) {
#endif /* FLOATING_POINT_SUPPORT */
		v2->value.big[BLOW] = value_to_integer(v2);
		if (valueNegative(v2))
			v2->value.big[BHIGH] = -1;
		else
			v2->value.big[BHIGH] = 0;
		v2->type = big;

	} else if (v2->type == big) {
		if (valueNegative(v1))
			v1->value.big[BHIGH] = -1;
		else
			v1->value.big[BHIGH] = 0;
		v1->value.big[BLOW] = value_to_integer(v1);
		v1->type = big;


	} else if (v1->type == integer) {
		v2->value.integer = value_to_integer(v2);
		v2->type = integer;

	} else if (v2->type == integer) {
		v1->value.integer = value_to_integer(v1);
		v1->type = integer;

	} else if (v1->type != integer) {
		v2->type = v1->type;

	} else if (v2->type != integer) {
		v1->type = v2->type;

	}
	a_assert(v1->type == v2->type);
}


/******************************************************************************/
/*
 *	Return true if the value is numeric and negative. Otherwise return 0.
 */

int valueNegative(value_t* vp)
{
	switch (vp->type) {
	default:
	case string:
	case bytes:
		return 0;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		if (vp->value.floating < 0)
			return 1;
		return 0;
#endif

	case flag:
		if ((signed char)vp->value.flag < 0)
			return 1;
		return 0;

	case byteint:
	case percent:
		if ((signed char)vp->value.byteint < 0)
			return 1;
		return 0;

	case shortint:
		if (vp->value.shortint < 0)
			return 1;
		return 0;

	case hex:
	case integer:
	case octal:
		if (vp->value.integer < 0)
			return 1;
		return 0;

	case big:
		if (vp->value.big[BHIGH] < 0)
			return 1;
		return 0;
	}
}

/******************************************************************************/
/*
 *	Return true if the value is numeric and zero. Otherwise return 0.
 */

int valueZero(value_t* vp)
{
	switch (vp->type) {
	default:
	case string:
	case bytes:
		return 0;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		if (vp->value.floating == 0)
			return 1;
		return 0;
#endif

	case flag:
		if (vp->value.flag == 0)
			return 1;
		return 0;

	case byteint:
	case percent:
		if (vp->value.byteint == 0)
			return 1;
		return 0;

	case shortint:
		if (vp->value.shortint == 0)
			return 1;
		return 0;

	case hex:
	case integer:
	case octal:
		if (vp->value.integer == 0)
			return 1;
		return 0;

	case big:
		if (vp->value.big[BHIGH] == 0 && vp->value.big[BLOW] == 0)
			return 1;
		return 0;
	}
}


/******************************************************************************/
/*
 *	Cast a value to an integer. Cannot be called for floating, non-numerics
 *	or bigs.
 */

static int value_to_integer(value_t* vp)
{
	switch (vp->type) {
	default:
	case string:
	case bytes:
	case big:
#ifdef FLOATING_POINT_SUPPORT
	case floating:
		a_assert(0);
		return -1;
#endif

	case flag:
		return (int) vp->value.flag;

	case byteint:
	case percent:
		return (int) vp->value.byteint;

	case shortint:
		return (int) vp->value.shortint;

	case hex:
	case integer:
	case octal:
		return (int) vp->value.integer;
	}
}


/******************************************************************************/
/*
 *	Convert a value to a text based representation of its value
 */

void valueSprintf(char_t** out, int size, char_t* fmt, value_t vp)
{
	char_t		*src, *dst, *tmp, *dst_start;

	a_assert(out);

	*out = NULL;

	if (! vp.valid) {
		*out = bstrdup(B_L, T("Invalid"));
		return;
	}

	switch (vp.type) {
	case flag:
		if (fmt == NULL || *fmt == '\0') {
			*out = bstrdup(B_L, (vp.value.flag) ? T("true") : T("false"));
		} else {
			fmtAlloc(out, size, fmt, (vp.value.flag) ? T("true") : T("false"));
		}
		break;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		if (fmt == NULL || *fmt == '\0') {
			fmtAlloc(out, size, T("%f"), vp.value.floating);
		} else {
			fmtAlloc(out, size, fmt, vp.value.floating);
		}
		break;
#endif

	case hex:
		if (fmt == NULL || *fmt == '\0') {
			fmtAlloc(out, size, T("0x%lx"), vp.value.hex);
		} else {
			fmtAlloc(out, size, fmt, vp.value.hex);
		}
		break;

	case big:
		if (*out == NULL) {
			*out = btoa(vp.value.big, NULL, 0);
		} else {
			btoa(vp.value.big, *out, size);
		}
		break;

	case integer:
		if (fmt == NULL || *fmt == '\0') {
			fmtAlloc(out, size, T("%ld"), vp.value.integer);
		} else {
			fmtAlloc(out, size, fmt, vp.value.integer);
		}
		break;

	case octal:
		if (fmt == NULL || *fmt == '\0') {
			fmtAlloc(out, size, T("0%lo"), vp.value.octal);
		} else {
			fmtAlloc(out, size, fmt, vp.value.octal);
		}
		break;

	case percent:
		if (fmt == NULL || *fmt == '\0') {
			fmtAlloc(out, size, T("%d%%"), vp.value.percent);
		} else {
			fmtAlloc(out, size, fmt, vp.value.percent);
		}
		break;

	case byteint:
		if (fmt == NULL || *fmt == '\0') {
			fmtAlloc(out, size, T("%d"), (int) vp.value.byteint);
		} else {
			fmtAlloc(out, size, fmt, (int) vp.value.byteint);
		}
		break;

	case shortint:
		if (fmt == NULL || *fmt == '\0') {
			fmtAlloc(out, size, T("%d"), (int) vp.value.shortint);
		} else {
			fmtAlloc(out, size, fmt, (int) vp.value.shortint);
		}
		break;

	case string:
	case errmsg:
		src = vp.value.string;

		if (src == NULL) {
			*out = bstrdup(B_L, T("NULL"));
		} else if (fmt && *fmt) {
			fmtAlloc(out, size, fmt, src);

		} else {

			*out = balloc(B_L, size);
			dst_start = dst = *out;
			for (; *src != '\0'; src++) {
				if (dst >= &dst_start[VALUE_MAX_STRING - 5])
					break;
				switch (*src) {
				case '\a':	*dst++ = '\\';	*dst++ = 'a';		break;
				case '\b':	*dst++ = '\\';	*dst++ = 'b';		break;
				case '\f':	*dst++ = '\\';	*dst++ = 'f';		break;
				case '\n':	*dst++ = '\\';	*dst++ = 'n';		break;
				case '\r':	*dst++ = '\\';	*dst++ = 'r';		break;
				case '\t':	*dst++ = '\\';	*dst++ = 't';		break;
				case '\v':	*dst++ = '\\';	*dst++ = 'v';		break;
				case '\\':	*dst++ = '\\';	*dst++ = '\\';		break;
				case '"':	*dst++ = '\\';	*dst++ = '\"';		break;
				default:
					if (gisprint(*src)) {
						*dst++ = *src;
					} else {
						fmtAlloc(&tmp, size, T("\\x%02x"),
							(unsigned int) *src);
						gstrcpy(dst, tmp);
						bfreeSafe(B_L, tmp);
						dst += 4;
					}
					break;
				}
			}
			*dst++ = '\0';
		}
		break;

#ifdef UNUSED
	case bytes:
		asrc = vp.value.bytes;

		if (asrc == NULL) {
			*out = bstrdup(B_L, T("NULL"));

		} else if (fmt && *fmt) {
			fmtAlloc(out, size, fmt, asrc);

		} else {

			dst_start = dst;
			for (; *asrc != '\0'; asrc++) {
				if (dst >= &dst_start[VALUE_MAX_STRING - 5])
					break;
				switch (*asrc) {
				case '\a':	*dst++ = '\\';	*dst++ = 'a';		break;
				case '\b':	*dst++ = '\\';	*dst++ = 'b';		break;
				case '\f':	*dst++ = '\\';	*dst++ = 'f';		break;
				case '\n':	*dst++ = '\\';	*dst++ = 'n';		break;
				case '\r':	*dst++ = '\\';	*dst++ = 'r';		break;
				case '\t':	*dst++ = '\\';	*dst++ = 't';		break;
				case '\v':	*dst++ = '\\';	*dst++ = 'v';		break;
				case '\\':	*dst++ = '\\';	*dst++ = '\\';		break;
				case '"':	*dst++ = '\\';	*dst++ = '\"';		break;
				default:
					if (gisprint(*asrc)) {
						*dst++ = *asrc;
					} else {
						fmtAlloc(dst, size,
							T("\\x%02x"), (unsigned int) *asrc);
						dst += 4;
					}
					break;
				}
			}
			*dst++ = '\0';
		}
		break;
#endif

	default:
		a_assert(0);
	}
}

/******************************************************************************/
/*
 *	Print a value to the named file descriptor
 */

void valueFprintf(FILE* fp, char_t* fmt, value_t vp)
{
	char_t	*buf;

	buf = NULL;
	valueSprintf(&buf, VALUE_MAX_STRING, fmt, vp);
	gfputs(buf, fp);
	bfreeSafe(B_L, buf);
	fflush(fp);
}

/******************************************************************************/
/*
 *	Ascii to value conversion
 */

value_t valueAtov(char_t* s, int pref_type)
{
	vtype_t	type;
	value_t			v;
	long			tmp[2], tmp2[2], base[2];
	int				i, len, num;

	a_assert(0 <= pref_type && pref_type < 99);		/* Sanity check */
	a_assert(s);

	v = value_null;
	if (s == NULL) {
		return value_null;
	}

	base[BLOW] = 10;
	base[BHIGH] = 0;
	len = gstrlen(s);

/*
 *	Determine the value type
 */
	type = undefined;
	if (pref_type <= 0) {
		if (gisdigit(*s)) {
			base[BHIGH] = 0;
			if (s[len - 1] == '%') {
				type = percent;
				len --;
				base[BLOW] = 10;
			} else if (*s == '0') {
				if (s[1] == 'x') {
					type = hex;
					s += 2;
					len -= 2;
					base[BLOW] = 16;
				} else if (s[1] == '\0') {
					type = integer;
					base[BLOW] = 10;
				} else {
					type = octal;
					s++;
					len--;
					base[BLOW] = 8;
				}
			} else {
				type = integer;
				base[BLOW] = 10;
			}

		} else {
			if (gstrcmp(s, T("true")) == 0 || gstrcmp(s, T("false")) == 0) {
				type = flag;
			} else if (*s == '\'' && s[len - 1] == '\'') {
				type = string;
				s++;
				len -= 2;
			} else if (*s == '\"' && s[len - 1] == '\"') {
				type = string;
				s++;
				len -= 2;
			} else {
				type = string;
			}
		}
		v.type = type;

	} else
		v.type = pref_type;
	v.valid = 1;

/*
 *	Do the conversion. Always use big arithmetic
 */
	switch (v.type) {
	case hex:
		if (!isdigit(s[0])) {
			if (gtolower(s[0]) >= 'a' || gtolower(s[0]) <= 'f') {
				v.value.big[BLOW] = 10 + gtolower(s[0]) - 'a';
			} else {
				v.value.big[BLOW] = 0;
			}
		} else {
			v.value.big[BLOW] = s[0] - '0';
		}
		v.value.big[BHIGH] = 0;
		for (i = 1; i < len; i++) {
			if (!isdigit(s[i])) {
				if (gtolower(s[i]) < 'a' || gtolower(s[i]) > 'f') {
					break;
				}
				num = 10 + gtolower(s[i]) - 'a';
			} else {
				num = s[i] - '0';
			}
			bmul(tmp, v.value.big, base);
			binit(tmp2, 0, num);
			badd(v.value.big, tmp, tmp2);
		}
		v.value.hex = v.value.big[BLOW];
		break;

	case shortint:
	case byteint:
	case integer:
	case percent:
	case octal:
	case big:
		v.value.big[BHIGH] = 0;
		if (gisdigit(s[0]))
			v.value.big[BLOW] = s[0] - '0';
		else
			v.value.big[BLOW] = 0;
		for (i = 1; i < len && gisdigit(s[i]); i++) {
			bmul(tmp, v.value.big, base);
			binit(tmp2, 0, s[i] - '0');
			badd(v.value.big, tmp, tmp2);
		}
		switch (v.type) {
		case shortint:
			v.value.shortint = (short) v.value.big[BLOW];
			break;
		case byteint:
			v.value.byteint = (char) v.value.big[BLOW];
			break;
		case integer:
			v.value.integer = (int) v.value.big[BLOW];
			break;
		case percent:
			v.value.percent = (char) v.value.big[BLOW];
			break;
		case octal:
			v.value.octal = (int) v.value.big[BLOW];
			break;
		default:
			break;
		}
		break;

#ifdef FLOATING_POINT_SUPPORT
	case floating:
		gsscanf(s, T("%f"), &v.value.floating);
		break;
#endif

	case flag:
		if (*s == 't')
			v.value.flag = 1;
		else v.value.flag = 0;
		break;

	case string:
/*
 *		Note this always ballocs a string
 */
		v = valueString(s, VALUE_ALLOCATE);
		break;

	case bytes:
		v = valueBytes((char*) s, VALUE_ALLOCATE);
		break;

#ifdef UNUSED
	case literal:
		v = value_literal(bstrdup(B_L, s));
		v.value.literal[len] = '\0';
		break;
#endif

	case undefined:
	case symbol:
	default:
		v.valid = 0;
		a_assert(0);
	}
	return v;
}

#endif /* !UEMF */
/******************************************************************************/

