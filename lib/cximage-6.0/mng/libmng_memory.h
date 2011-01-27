/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_memory.h           copyright (c) 2000-2003 G.Juyn   * */
/* * version   : 1.0.0                                                      * */
/* *                                                                        * */
/* * purpose   : Memory management (definition)                             * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of memory management functions                  * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *                                                                        * */
/* *             0.5.3 - 06/12/2000 - G.Juyn                                * */
/* *             - swapped MNG_COPY parameter-names                         * */
/* *             0.5.3 - 06/27/2000 - G.Juyn                                * */
/* *             - changed size parameter to mng_size_t                     * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* ************************************************************************** */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_memory_h_
#define _libmng_memory_h_

/* ************************************************************************** */
/* *                                                                        * */
/* * Generic memory manager macros                                          * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_INTERNAL_MEMMNGMT
#define MNG_ALLOC(H,P,L)  { P = calloc (1, (mng_size_t)(L)); \
                            if (P == 0) { MNG_ERROR (H, MNG_OUTOFMEMORY) } }
#define MNG_ALLOCX(H,P,L) { P = calloc (1, (mng_size_t)(L)); }
#define MNG_FREE(H,P,L)   { if (P) { free (P); P = 0; } }
#define MNG_FREEX(H,P,L)  { if (P) free (P); }
#else
#define MNG_ALLOC(H,P,L)  { P = H->fMemalloc ((mng_size_t)(L)); \
                            if (P == 0) { MNG_ERROR (H, MNG_OUTOFMEMORY) } }
#define MNG_ALLOCX(H,P,L) { P = H->fMemalloc ((mng_size_t)(L)); }
#define MNG_FREE(H,P,L)   { if (P) { H->fMemfree (P, (mng_size_t)(L)); P = 0; } }
#define MNG_FREEX(H,P,L)  { if (P) { H->fMemfree (P, (mng_size_t)(L)); } }
#endif /* mng_internal_memmngmt */

#define MNG_COPY(D,S,L)   { memcpy (D, S, (mng_size_t)(L)); }

/* ************************************************************************** */

#endif /* _libmng_memory_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
