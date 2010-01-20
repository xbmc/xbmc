/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_jpeg.h             copyright (c) 2000-2002 G.Juyn   * */
/* * version   : 1.0.0                                                      * */
/* *                                                                        * */
/* * purpose   : JPEG library interface (definition)                        * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of the JPEG library interface                   * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 10/16/2000 - G.Juyn                                * */
/* *             - added support for JDAA                                   * */
/* *                                                                        * */
/* ************************************************************************** */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_jpeg_h_
#define _libmng_jpeg_h_

/* ************************************************************************** */

mng_retcode mngjpeg_initialize      (mng_datap  pData);
mng_retcode mngjpeg_cleanup         (mng_datap  pData);

mng_retcode mngjpeg_decompressinit  (mng_datap  pData);
mng_retcode mngjpeg_decompressdata  (mng_datap  pData,
                                     mng_uint32 iRawsize,
                                     mng_uint8p pRawdata);
mng_retcode mngjpeg_decompressfree  (mng_datap  pData);

mng_retcode mngjpeg_decompressinit2 (mng_datap  pData);
mng_retcode mngjpeg_decompressdata2 (mng_datap  pData,
                                     mng_uint32 iRawsize,
                                     mng_uint8p pRawdata);
mng_retcode mngjpeg_decompressfree2 (mng_datap  pData);

/* ************************************************************************** */

#endif /* _libmng_jpeg_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
