/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_filter.h           copyright (c) 2000-2002 G.Juyn   * */
/* * version   : 1.0.5                                                      * */
/* *                                                                        * */
/* * purpose   : Filtering routines (definition)                            * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of the filtering routines                       * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 09/07/2000 - G.Juyn                                * */
/* *             - added support for new filter_types                       * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *                                                                        * */
/* ************************************************************************** */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_filter_h_
#define _libmng_filter_h_

/* ************************************************************************** */

mng_retcode mng_filter_a_row         (mng_datap pData);

/* ************************************************************************** */

#ifdef FILTER192
mng_retcode mng_init_rowdiffering    (mng_datap pData);

mng_retcode mng_differ_g1            (mng_datap pData);
mng_retcode mng_differ_g2            (mng_datap pData);
mng_retcode mng_differ_g4            (mng_datap pData);
mng_retcode mng_differ_g8            (mng_datap pData);
mng_retcode mng_differ_g16           (mng_datap pData);
mng_retcode mng_differ_rgb8          (mng_datap pData);
mng_retcode mng_differ_rgb16         (mng_datap pData);
mng_retcode mng_differ_idx1          (mng_datap pData);
mng_retcode mng_differ_idx2          (mng_datap pData);
mng_retcode mng_differ_idx4          (mng_datap pData);
mng_retcode mng_differ_idx8          (mng_datap pData);
mng_retcode mng_differ_ga8           (mng_datap pData);
mng_retcode mng_differ_ga16          (mng_datap pData);
mng_retcode mng_differ_rgba8         (mng_datap pData);
mng_retcode mng_differ_rgba16        (mng_datap pData);
#endif

/* ************************************************************************** */

#endif /* _libmng_filter_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
