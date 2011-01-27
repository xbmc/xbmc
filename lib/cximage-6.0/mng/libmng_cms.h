/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_cms.h              copyright (c) 2000-2003 G.Juyn   * */
/* * version   : 1.0.6                                                      * */
/* *                                                                        * */
/* * purpose   : color management routines (definition)                     * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of color management routines                    * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/11/2000 - G.Juyn                                * */
/* *             - added creatememprofile                                   * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             1.0.1 - 04/25/2001 - G.Juyn                                * */
/* *             - moved mng_clear_cms to libmng_cms                        * */
/* *             1.0.1 - 05/02/2001 - G.Juyn                                * */
/* *             - added "default" sRGB generation (Thanks Marti!)          * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             1.0.5 - 09/19/2002 - G.Juyn                                * */
/* *             - optimized color-correction routines                      * */
/* *                                                                        * */
/* *             1.0.6 - 04/11/2003 - G.Juyn                                * */
/* *             - B719420 - fixed several MNG_APP_CMS problems             * */
/* *                                                                        * */
/* ************************************************************************** */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_cms_h_
#define _libmng_cms_h_

/* ************************************************************************** */

#ifdef MNG_INCLUDE_LCMS
void        mnglcms_initlibrary       (void);
mng_cmsprof mnglcms_createfileprofile (mng_pchar    zFilename);
mng_cmsprof mnglcms_creatememprofile  (mng_uint32   iProfilesize,
                                       mng_ptr      pProfile );
mng_cmsprof mnglcms_createsrgbprofile (void);
void        mnglcms_freeprofile       (mng_cmsprof  hProf    );
void        mnglcms_freetransform     (mng_cmstrans hTrans   );

mng_retcode mng_clear_cms             (mng_datap    pData    );
#endif

/* ************************************************************************** */

#ifdef MNG_FULL_CMS
mng_retcode mng_init_full_cms          (mng_datap pData,
                                        mng_bool  bGlobal,
                                        mng_bool  bObject,
                                        mng_bool  bRetrobj);
mng_retcode mng_correct_full_cms       (mng_datap pData);
#endif

#if defined(MNG_FULL_CMS) || defined(MNG_GAMMA_ONLY) || defined(MNG_APP_CMS)
mng_retcode mng_init_gamma_only        (mng_datap pData,
                                        mng_bool  bGlobal,
                                        mng_bool  bObject,
                                        mng_bool  bRetrobj);
mng_retcode mng_correct_gamma_only     (mng_datap pData);
#endif

#ifdef MNG_APP_CMS
mng_retcode mng_init_app_cms           (mng_datap pData,
                                        mng_bool  bGlobal,
                                        mng_bool  bObject,
                                        mng_bool  bRetrobj);
mng_retcode mng_correct_app_cms        (mng_datap pData);
#endif

/* ************************************************************************** */

#endif /* _libmng_cms_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
