/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_read.h             copyright (c) 2000-2004 G.Juyn   * */
/* * version   : 1.0.8                                                      * */
/* *                                                                        * */
/* * purpose   : Read management (definition)                               * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of the read management routines                 * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 10/18/2000 - G.Juyn                                * */
/* *             - added closestream() processing for mng_cleanup()         * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *                                                                        * */
/* *             1.0.8 - 04/12/2004 - G.Juyn                                * */
/* *             - added data-push mechanisms for specialized decoders      * */
/* *                                                                        * */
/* ************************************************************************** */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_read_h_
#define _libmng_read_h_

/* ************************************************************************** */

mng_retcode mng_process_eof       (mng_datap pData);

mng_retcode mng_release_pushdata  (mng_datap pData);

mng_retcode mng_release_pushchunk (mng_datap pData);

mng_retcode mng_read_graphic      (mng_datap pData);

/* ************************************************************************** */

#endif /* _libmng_read_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
