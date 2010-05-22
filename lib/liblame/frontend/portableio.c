/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * Warranty Information
 * Even though Apple has reviewed this software, Apple makes no warranty
 * or representation, either express or implied, with respect to this
 * software, its quality, accuracy, merchantability, or fitness for a
 * particular purpose.  As a result, this software is provided "as is,"
 * and you, its user, are assuming the entire risk as to its quality
 * and accuracy.
 *
 * This code may be used and freely distributed as long as it includes
 * this copyright notice and the warranty information.
 *
 *
 * Motorola processors (Macintosh, Sun, Sparc, MIPS, etc)
 * pack bytes from high to low (they are big-endian).
 * Use the HighLow routines to match the native format
 * of these machines.
 *
 * Intel-like machines (PCs, Sequent)
 * pack bytes from low to high (the are little-endian).
 * Use the LowHigh routines to match the native format
 * of these machines.
 *
 * These routines have been tested on the following machines:
 *	Apple Macintosh, MPW 3.1 C compiler
 *	Apple Macintosh, THINK C compiler
 *	Silicon Graphics IRIS, MIPS compiler
 *	Cray X/MP and Y/MP
 *	Digital Equipment VAX
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
 *
 * $Id: portableio.c,v 1.13 2007/10/14 19:54:32 robert Exp $
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include	<stdio.h>
#if defined(__riscos__) && defined(FPA10)
#include	"ymath.h"
#else
#include	<math.h>
#endif
#include	"portableio.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/****************************************************************
 * Big/little-endian independent I/O routines.
 ****************************************************************/

/*
 * It is a hoax to call this code portable-IO:
 * 
 *   - It doesn't work on machines with CHAR_BIT != 8
 *   - it also don't test this error condition
 *   - otherwise it tries to handle CHAR_BIT != 8 by things like 
 *     masking 'putc(i&0xff,fp)'
 *   - It doesn't handle EOF in any way
 *   - it only works with ints with 32 or more bits
 *   - It is a collection of initial buggy code with patching the known errors
 *     instead of CORRECTING them! 
 *     For that see comments on the old Read16BitsHighLow()
 */

#ifdef KLEMM_36

signed int
ReadByte(FILE * fp)
{
    int     result = getc(fp);
    return result == EOF ? 0 : (signed char) (result & 0xFF);
}

unsigned int
ReadByteUnsigned(FILE * fp)
{
    int     result = getc(fp);
    return result == EOF ? 0 : (unsigned char) (result & 0xFF);
}

#else

int
ReadByte(FILE * fp)
{
    int     result;

    result = getc(fp) & 0xff;
    if (result & 0x80)
        result = result - 0x100;
    return result;
}

#endif

#ifdef KLEMM_36

int
Read16BitsLowHigh(FILE * fp)
{
    int     low = ReadByteUnsigned(fp);
    int     high = ReadByte(fp);

    return (high << 8) | low;
}

#else
int
Read16BitsLowHigh(FILE * fp)
{
    int     first, second, result;

    first = 0xff & getc(fp);
    second = 0xff & getc(fp);

    result = (second << 8) + first;
#ifndef	THINK_C42
    if (result & 0x8000)
        result = result - 0x10000;
#endif /* THINK_C */
    return (result);
}
#endif


#ifdef KLEMM_36

int
Read16BitsHighLow(FILE * fp)
{
    int     high = ReadByte(fp);
    int     low = ReadByteUnsigned(fp);

    return (high << 8) | low;
}

#else
int
Read16BitsHighLow(FILE * fp)
{
    int     first, second, result;

    /* Reads the High bits, the value is -128...127 
     * (which gave after upscaling the -32768...+32512
     * Why this value is not converted to signed char?
     */
    first = 0xff & getc(fp);
    /* Reads the Lows bits, the value is 0...255 
     * This is correct. This value gives an additional offset
     * for the High bits
     */
    second = 0xff & getc(fp);

    /* This is right */
    result = (first << 8) + second;

    /* Now we are starting to correct the nasty bug of the first instruction
     * The value of the high bits is wrong. Always. So we must correct this
     * value. This seems to be not necessary for THINK_C42. This is either
     * a 16 bit compiler with 16 bit ints (where this bug is hidden and 0x10000
     * is not in the scope of an int) or it is not a C compiler, but only a
     * C like compiler. In the first case the '#ifndef THINK_C42' is wrong
     * because it's not a property of the THINK_C42 compiler, but of all compilers
     * with sizeof(int)*CHAR_BIT < 18.
     * Another nasty thing is that the rest of the code doesn't work for 16 bit ints,
     * so this patch don't solve the 16 bit problem.
     */
#ifndef	THINK_C42
    if (result & 0x8000)
        result = result - 0x10000;
#endif /* THINK_C */
    return (result);
}
#endif

void
Write8Bits(FILE * fp, int i)
{
    putc(i & 0xff, fp);
}


void
Write16BitsLowHigh(FILE * fp, int i)
{
    putc(i & 0xff, fp);
    putc((i >> 8) & 0xff, fp);
}


void
Write16BitsHighLow(FILE * fp, int i)
{
    putc((i >> 8) & 0xff, fp);
    putc(i & 0xff, fp);
}

#ifdef KLEMM_36

int
Read24BitsHighLow(FILE * fp)
{
    int     high = ReadByte(fp);
    int     med = ReadByteUnsigned(fp);
    int     low = ReadByteUnsigned(fp);

    return (high << 16) | (med << 8) | low;
}

#else
int
Read24BitsHighLow(FILE * fp)
{
    int     first, second, third;
    int     result;

    first = 0xff & getc(fp);
    second = 0xff & getc(fp);
    third = 0xff & getc(fp);

    result = (first << 16) + (second << 8) + third;
    if (result & 0x800000)
        result = result - 0x1000000;
    return (result);
}
#endif

#define	Read32BitsLowHigh(f)	Read32Bits(f)

#ifdef KLEMM_36

int
Read32Bits(FILE * fp)
{
    int     low = ReadByteUnsigned(fp);
    int     medl = ReadByteUnsigned(fp);
    int     medh = ReadByteUnsigned(fp);
    int     high = ReadByte(fp);

    return (high << 24) | (medh << 16) | (medl << 8) | low;
}

#else

int
Read32Bits(FILE * fp)
{
    int     first, second, result;

    first = 0xffff & Read16BitsLowHigh(fp);
    second = 0xffff & Read16BitsLowHigh(fp);

    result = (second << 16) + first;
#ifdef	CRAY
    if (result & 0x80000000)
        result = result - 0x100000000;
#endif /* CRAY */
    return (result);
}
#endif


#ifdef KLEMM_36

int
Read32BitsHighLow(FILE * fp)
{
    int     high = ReadByte(fp);
    int     medh = ReadByteUnsigned(fp);
    int     medl = ReadByteUnsigned(fp);
    int     low = ReadByteUnsigned(fp);

    return (high << 24) | (medh << 16) | (medl << 8) | low;
}

#else

int
Read32BitsHighLow(FILE * fp)
{
    int     first, second, result;

    first = 0xffff & Read16BitsHighLow(fp);
    second = 0xffff & Read16BitsHighLow(fp);

    result = (first << 16) + second;
#ifdef	CRAY
    if (result & 0x80000000)
        result = result - 0x100000000;
#endif
    return (result);
}

#endif

void
Write32Bits(FILE * fp, int i)
{
    Write16BitsLowHigh(fp, (int) (i & 0xffffL));
    Write16BitsLowHigh(fp, (int) ((i >> 16) & 0xffffL));
}


void
Write32BitsLowHigh(FILE * fp, int i)
{
    Write16BitsLowHigh(fp, (int) (i & 0xffffL));
    Write16BitsLowHigh(fp, (int) ((i >> 16) & 0xffffL));
}


void
Write32BitsHighLow(FILE * fp, int i)
{
    Write16BitsHighLow(fp, (int) ((i >> 16) & 0xffffL));
    Write16BitsHighLow(fp, (int) (i & 0xffffL));
}

#ifdef KLEMM_36
void
ReadBytes(FILE * fp, char *p, int n)
{
    memset(p, 0, n);
    fread(p, 1, n, fp);
}
#else
void
ReadBytes(FILE * fp, char *p, int n)
{
    /* What about fread? */

    while (!feof(fp) & (n-- > 0))
        *p++ = getc(fp);
}
#endif

void
ReadBytesSwapped(FILE * fp, char *p, int n)
{
    register char *q = p;

    /* What about fread? */

    while (!feof(fp) & (n-- > 0))
        *q++ = getc(fp);

    /* If not all bytes could be read, the resorting is different
     * from the normal resorting. Is this intention or another bug?
     */
    for (q--; p < q; p++, q--) {
        n = *p;
        *p = *q;
        *q = n;
    }
}

#ifdef KLEMM_36
void
WriteBytes(FILE * fp, char *p, int n)
{
    /* return n == */
    fwrite(p, 1, n, fp);
}
#else
void
WriteBytes(FILE * fp, char *p, int n)
{
    /* No error condition checking */
    while (n-- > 0)
        putc(*p++, fp);
}
#endif
#ifdef KLEMM_36
void
WriteBytesSwapped(FILE * fp, char *p, int n)
{
    p += n;
    while (n-- > 0)
        putc(*--p, fp);
}
#else
void
WriteBytesSwapped(FILE * fp, char *p, int n)
{
    p += n - 1;
    while (n-- > 0)
        putc(*p--, fp);
}
#endif



/****************************************************************
 * The following two routines make up for deficiencies in many
 * compilers to convert properly between unsigned integers and
 * floating-point.  Some compilers which have this bug are the
 * THINK_C compiler for the Macintosh and the C compiler for the
 * Silicon Graphics MIPS-based Iris.
 ****************************************************************/

#ifdef applec           /* The Apple C compiler works */
# define FloatToUnsigned(f)	((unsigned long)(f))
# define UnsignedToFloat(u)	((double)(u))
#else /* applec */
# define FloatToUnsigned(f)	((unsigned long)(((long)((f) - 2147483648.0)) + 2147483647L + 1))
# define UnsignedToFloat(u)	(((double)((long)((u) - 2147483647L - 1))) + 2147483648.0)
#endif /* applec */
/****************************************************************
 * Extended precision IEEE floating-point conversion routines
 ****************************************************************/

static double
ConvertFromIeeeExtended(char *bytes)
{
    double  f;
    long    expon;
    unsigned long hiMant, loMant;

#ifdef	TEST
    printf("ConvertFromIEEEExtended(%lx,%lx,%lx,%lx,%lx,%lx,%lx,%lx,%lx,%lx\r",
           (long) bytes[0], (long) bytes[1], (long) bytes[2], (long) bytes[3],
           (long) bytes[4], (long) bytes[5], (long) bytes[6],
           (long) bytes[7], (long) bytes[8], (long) bytes[9]);
#endif

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant = ((unsigned long) (bytes[2] & 0xFF) << 24)
        | ((unsigned long) (bytes[3] & 0xFF) << 16)
        | ((unsigned long) (bytes[4] & 0xFF) << 8)
        | ((unsigned long) (bytes[5] & 0xFF));
    loMant = ((unsigned long) (bytes[6] & 0xFF) << 24)
        | ((unsigned long) (bytes[7] & 0xFF) << 16)
        | ((unsigned long) (bytes[8] & 0xFF) << 8)
        | ((unsigned long) (bytes[9] & 0xFF));

    /* This case should also be called if the number is below the smallest
     * positive double variable */
    if (expon == 0 && hiMant == 0 && loMant == 0) {
        f = 0;
    }
    else {
        /* This case should also be called if the number is too large to fit into 
         * a double variable */

        if (expon == 0x7FFF) { /* Infinity or NaN */
            f = HUGE_VAL;
        }
        else {
            expon -= 16383;
            f = ldexp(UnsignedToFloat(hiMant), (int) (expon -= 31));
            f += ldexp(UnsignedToFloat(loMant), (int) (expon -= 32));
        }
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}





double
ReadIeeeExtendedHighLow(FILE * fp)
{
    char    bytes[10];

    ReadBytes(fp, bytes, 10);
    return ConvertFromIeeeExtended(bytes);
}
