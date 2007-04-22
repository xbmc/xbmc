/* bits.h
 * (c) 2002 Karel 'Clock' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

/* t2c
 * Type that has exactly 2 chars.
 * If there is none, t2c is not defined
 * The type may be signed or unsigned, it doesn't matter
 */
#if SIZEOF_SHORT == 2
#define t2c short
#elif SIZEOF_UNSIGNED_SHORT == 2
#define t2c unsigned short
#elif SIZEOF_INT == 2
#define t2c int
#elif SIZEOF_UNSIGNED == 2
#define t2c unsigned
#elif SIZEOF_LONG == 2
#define t2c long
#elif SIZEOF_UNSIGNED_LONG == 2
#define t2c unsigned long
#endif /* #if sizeof(short) */

/* t4c
 * Type that has exactly 4 chars.
 * If there is none, t2c is not defined
 * The type may be signed or unsigned, it doesn't matter
 */
#if SIZEOF_SHORT == 4
#define t4c short
#elif SIZEOF_UNSIGNED_SHORT == 4
#define t4c unsigned short
#elif SIZEOF_INT == 4
#define t4c int
#elif SIZEOF_UNSIGNED == 4
#define t4c unsigned
#elif SIZEOF_LONG == 4
#define t4c long
#elif SIZEOF_UNSIGNED_LONG == 4
#define t4c unsigned long
#endif /* #if sizeof(short) */

