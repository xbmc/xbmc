/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_error.h            copyright (c) 2000-2002 G.Juyn   * */
/* * version   : 1.0.5                                                      * */
/* *                                                                        * */
/* * purpose   : Error functions (definition)                               * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of the generic error-codes and functions        * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/06/2000 - G.Juyn                                * */
/* *             - added some errorcodes                                    * */
/* *             0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - added some errorcodes                                    * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/11/2000 - G.Juyn                                * */
/* *             - added application errorcodes (used with callbacks)       * */
/* *             - moved chunk-access errorcodes to severity 5              * */
/* *                                                                        * */
/* *             0.5.2 - 05/20/2000 - G.Juyn                                * */
/* *             - added JNG errorcodes                                     * */
/* *             0.5.2 - 05/23/2000 - G.Juyn                                * */
/* *             - added error tell-tale definition                         * */
/* *             0.5.2 - 05/30/2000 - G.Juyn                                * */
/* *             - added errorcodes for delta-image processing              * */
/* *             0.5.2 - 06/06/2000 - G.Juyn                                * */
/* *             - added errorcode for delayed buffer-processing            * */
/* *             - moved errorcodes to "libmng.h"                           * */
/* *                                                                        * */
/* *             0.9.1 - 07/15/2000 - G.Juyn                                * */
/* *             - added macro + routine to set returncode without          * */
/* *               calling error callback                                   * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             1.0.5 - 08/20/2002 - G.Juyn                                * */
/* *             - added option for soft-handling of errors                 * */
/* *                                                                        * */
/* ************************************************************************** */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_error_h_
#define _libmng_error_h_

/* ************************************************************************** */
/* *                                                                        * */
/* * Default error routines                                                 * */
/* *                                                                        * */
/* ************************************************************************** */

mng_bool mng_store_error   (mng_datap   pData,
                            mng_retcode iError,
                            mng_retcode iExtra1,
                            mng_retcode iExtra2);

mng_bool mng_process_error (mng_datap   pData,
                            mng_retcode iError,
                            mng_retcode iExtra1,
                            mng_retcode iExtra2);

/* ************************************************************************** */
/* *                                                                        * */
/* * Error handling macros                                                  * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_SOFTERRORS
#define MNG_ERROR(D,C)      { if (!mng_process_error (D, C, 0, 0)) return C; }
#define MNG_ERRORZ(D,Z)     { if (!mng_process_error (D, MNG_ZLIBERROR, Z, 0)) return MNG_ZLIBERROR; }
#define MNG_ERRORJ(D,J)     { if (!mng_process_error (D, MNG_JPEGERROR, J, 0)) return MNG_JPEGERROR; }
#define MNG_ERRORL(D,L)     { if (!mng_process_error (D, MNG_LCMSERROR, L, 0)) return MNG_LCMSERROR; }
#else
#define MNG_ERROR(D,C)      { mng_process_error (D, C, 0, 0); return C; }
#define MNG_ERRORZ(D,Z)     { mng_process_error (D, MNG_ZLIBERROR, Z, 0); return MNG_ZLIBERROR; }
#define MNG_ERRORJ(D,J)     { mng_process_error (D, MNG_JPEGERROR, J, 0); return MNG_JPEGERROR; }
#define MNG_ERRORL(D,L)     { mng_process_error (D, MNG_LCMSERROR, L, 0); return MNG_LCMSERROR; }
#endif

#define MNG_RETURN(D,C)     { mng_store_error (D, C, 0, 0); return C; }

#define MNG_WARNING(D,C)    { if (!mng_process_error (D, C, 0, 0)) return C; }

#define MNG_VALIDHANDLE(H)  { if ((H == 0) || (((mng_datap)H)->iMagic != MNG_MAGIC)) \
                                return MNG_INVALIDHANDLE; }
#define MNG_VALIDHANDLEX(H) { if ((H == 0) || (((mng_datap)H)->iMagic != MNG_MAGIC)) \
                                return 0; }
#define MNG_VALIDCB(D,C)    { if (!((mng_datap)D)->C) \
                                MNG_ERROR (((mng_datap)D), MNG_NOCALLBACK) }

/* ************************************************************************** */
/* *                                                                        * */
/* * Error string-table entry                                               * */
/* *                                                                        * */
/* ************************************************************************** */

typedef struct {
                 mng_retcode iError;
                 mng_pchar   zErrortext;
               } mng_error_entry;
typedef mng_error_entry const * mng_error_entryp;

/* ************************************************************************** */

#endif /* _libmng_error_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
