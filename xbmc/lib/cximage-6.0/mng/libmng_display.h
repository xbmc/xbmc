/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_display.h          copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Display management (definition)                            * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of the display managament routines              * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *                                                                        * */
/* *             0.5.2 - 05/20/2000 - G.Juyn                                * */
/* *             - added JNG support stuff                                  * */
/* *                                                                        * */
/* *             0.5.3 - 06/16/2000 - G.Juyn                                * */
/* *             - changed progressive-display processing                   * */
/* *             0.5.3 - 06/22/2000 - G.Juyn                                * */
/* *             - added support for delta-image processing                 * */
/* *             - added support for PPLT chunk processing                  * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *             0.9.3 - 08/07/2000 - G.Juyn                                * */
/* *             - B111300 - fixup for improved portability                 * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 10/16/2000 - G.Juyn                                * */
/* *             - added JDAA chunk                                         * */
/* *                                                                        * */
/* *             0.9.4 - 11/24/2000 - G.Juyn                                * */
/* *             - moved restore of object 0 to libmng_display              * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             1.0.5 - 09/13/2002 - G.Juyn                                * */
/* *             - fixed read/write of MAGN chunk                           * */
/* *             1.0.5 - 09/20/2002 - G.Juyn                                * */
/* *             - added support for PAST                                   * */
/* *             1.0.5 - 10/07/2002 - G.Juyn                                * */
/* *             - added proposed change in handling of TERM- & if-delay    * */
/* *             1.0.5 - 10/20/2002 - G.Juyn                                * */
/* *             - fixed display of visible target of PAST operation        * */
/* *                                                                        * */
/* *             1.0.7 - 03/24/2004 - G.R-P.                                * */
/* *             - added some SKIPCHUNK conditionals                        * */
/* *                                                                        * */
/* *             1.0.9 - 12/11/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_DISPLAYCALLS              * */
/* *                                                                        * */
/* *             1.0.10 - 04/08/2007 - G.Juyn                               * */
/* *             - added support for mPNG proposal                          * */
/* *             1.0.10 - 04/12/2007 - G.Juyn                               * */
/* *             - added support for ANG proposal                           * */
/* *                                                                        * */
/* ************************************************************************** */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_display_h_
#define _libmng_display_h_

/* ************************************************************************** */

#ifdef MNG_INCLUDE_DISPLAY_PROCS

/* ************************************************************************** */

mng_retcode mng_display_progressive_refresh (mng_datap  pData,
                                             mng_uint32 iInterval);

/* ************************************************************************** */

mng_retcode mng_reset_objzero         (mng_datap      pData);

mng_retcode mng_display_image         (mng_datap      pData,
                                       mng_imagep     pImage,
                                       mng_bool       bLayeradvanced);

mng_retcode mng_execute_delta_image   (mng_datap      pData,
                                       mng_imagep     pTarget,
                                       mng_imagep     pDelta);

/* ************************************************************************** */

mng_retcode mng_process_display       (mng_datap      pData);

/* ************************************************************************** */

#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
png_imgtype mng_png_imgtype           (mng_uint8      colortype,
                                       mng_uint8      bitdepth);
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_DISPLAYCALLS

mng_retcode mng_process_display_ihdr  (mng_datap      pData);

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
mng_retcode mng_process_display_mpng  (mng_datap      pData);
#endif

#ifdef MNG_INCLUDE_ANG_PROPOSAL
mng_retcode mng_process_display_ang   (mng_datap      pData);
#endif

mng_retcode mng_process_display_idat  (mng_datap      pData,
                                       mng_uint32     iRawlen,
                                       mng_uint8p     pRawdata);

mng_retcode mng_process_display_iend  (mng_datap      pData);
mng_retcode mng_process_display_mend  (mng_datap      pData);
mng_retcode mng_process_display_mend2 (mng_datap      pData);
mng_retcode mng_process_display_defi  (mng_datap      pData);

#ifndef MNG_SKIPCHUNK_BASI
mng_retcode mng_process_display_basi  (mng_datap      pData,
                                       mng_uint16     iRed,
                                       mng_uint16     iGreen,
                                       mng_uint16     iBlue,
                                       mng_bool       bHasalpha,
                                       mng_uint16     iAlpha,
                                       mng_uint8      iViewable);
#endif

#ifndef MNG_SKIPCHUNK_CLON
mng_retcode mng_process_display_clon  (mng_datap      pData,
                                       mng_uint16     iSourceid,
                                       mng_uint16     iCloneid,
                                       mng_uint8      iClonetype,
                                       mng_bool       bHasdonotshow,
                                       mng_uint8      iDonotshow,
                                       mng_uint8      iConcrete,
                                       mng_bool       bHasloca,
                                       mng_uint8      iLocationtype,
                                       mng_int32      iLocationx,
                                       mng_int32      iLocationy);
mng_retcode mng_process_display_clon2 (mng_datap      pData);
#endif

#ifndef MNG_SKIPCHUNK_DISC
mng_retcode mng_process_display_disc  (mng_datap      pData,
                                       mng_uint32     iCount,
                                       mng_uint16p    pIds);
#endif

#ifndef MNG_SKIPCHUNK_FRAM
mng_retcode mng_process_display_fram  (mng_datap      pData,
                                       mng_uint8      iFramemode,
                                       mng_uint8      iChangedelay,
                                       mng_uint32     iDelay,
                                       mng_uint8      iChangetimeout,
                                       mng_uint32     iTimeout,
                                       mng_uint8      iChangeclipping,
                                       mng_uint8      iCliptype,
                                       mng_int32      iClipl,
                                       mng_int32      iClipr,
                                       mng_int32      iClipt,
                                       mng_int32      iClipb);
mng_retcode mng_process_display_fram2 (mng_datap      pData);
#endif

#ifndef MNG_SKIPCHUNK_MOVE
mng_retcode mng_process_display_move  (mng_datap      pData,
                                       mng_uint16     iFromid,
                                       mng_uint16     iToid,
                                       mng_uint8      iMovetype,
                                       mng_int32      iMovex,
                                       mng_int32      iMovey);
#endif

#ifndef MNG_SKIPCHUNK_CLIP
mng_retcode mng_process_display_clip  (mng_datap      pData,
                                       mng_uint16     iFromid,
                                       mng_uint16     iToid,
                                       mng_uint8      iCliptype,
                                       mng_int32      iClipl,
                                       mng_int32      iClipr,
                                       mng_int32      iClipt,
                                       mng_int32      iClipb);
#endif

#ifndef MNG_SKIPCHUNK_SHOW
mng_retcode mng_process_display_show  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_process_display_save  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode mng_process_display_seek  (mng_datap      pData);
#endif
#ifdef MNG_INCLUDE_JNG
mng_retcode mng_process_display_jhdr  (mng_datap      pData);

mng_retcode mng_process_display_jdaa  (mng_datap      pData,
                                       mng_uint32     iRawlen,
                                       mng_uint8p     pRawdata);

mng_retcode mng_process_display_jdat  (mng_datap      pData,
                                       mng_uint32     iRawlen,
                                       mng_uint8p     pRawdata);

#endif
#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_process_display_dhdr  (mng_datap      pData,
                                       mng_uint16     iObjectid,
                                       mng_uint8      iImagetype,
                                       mng_uint8      iDeltatype,
                                       mng_uint32     iBlockwidth,
                                       mng_uint32     iBlockheight,
                                       mng_uint32     iBlockx,
                                       mng_uint32     iBlocky);

mng_retcode mng_process_display_prom  (mng_datap      pData,
                                       mng_uint8      iBitdepth,
                                       mng_uint8      iColortype,
                                       mng_uint8      iFilltype);

mng_retcode mng_process_display_ipng  (mng_datap      pData);
#ifdef MNG_INCLUDE_JNG
mng_retcode mng_process_display_ijng  (mng_datap      pData);
#endif

mng_retcode mng_process_display_pplt  (mng_datap      pData,
                                       mng_uint8      iType,
                                       mng_uint32     iCount,
                                       mng_palette8ep paIndexentries,
                                       mng_uint8p     paAlphaentries,
                                       mng_uint8p     paUsedentries);
#endif

#ifndef MNG_SKIPCHUNK_MAGN
mng_retcode mng_process_display_magn  (mng_datap      pData,
                                       mng_uint16     iFirstid,
                                       mng_uint16     iLastid,
                                       mng_uint8      iMethodX,
                                       mng_uint16     iMX,
                                       mng_uint16     iMY,
                                       mng_uint16     iML,
                                       mng_uint16     iMR,
                                       mng_uint16     iMT,
                                       mng_uint16     iMB,
                                       mng_uint8      iMethodY);
mng_retcode mng_process_display_magn2 (mng_datap      pData);
#endif

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_process_display_past  (mng_datap      pData,
                                       mng_uint16     iTargetid,
                                       mng_uint8      iTargettype,
                                       mng_int32      iTargetx,
                                       mng_int32      iTargety,
                                       mng_uint32     iCount,
                                       mng_ptr        pSources);
mng_retcode mng_process_display_past2 (mng_datap      pData);
#endif

#else /* MNG_OPTIMIZE_DISPLAYCALLS */

mng_retcode mng_process_display_ihdr  (mng_datap      pData);
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
mng_retcode mng_process_display_mpng  (mng_datap      pData);
#endif
mng_retcode mng_process_display_idat  (mng_datap      pData);
mng_retcode mng_process_display_iend  (mng_datap      pData);
mng_retcode mng_process_display_mend  (mng_datap      pData);
mng_retcode mng_process_display_mend2 (mng_datap      pData);
mng_retcode mng_process_display_defi  (mng_datap      pData);
#ifndef MNG_SKIPCHUNK_BASI
mng_retcode mng_process_display_basi  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_CLON
mng_retcode mng_process_display_clon  (mng_datap      pData);
mng_retcode mng_process_display_clon2 (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_DISC
mng_retcode mng_process_display_disc  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_FRAM
mng_retcode mng_process_display_fram  (mng_datap      pData);
mng_retcode mng_process_display_fram2 (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_MOVE
mng_retcode mng_process_display_move  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_CLIP
mng_retcode mng_process_display_clip  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_SHOW
mng_retcode mng_process_display_show  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_process_display_save  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode mng_process_display_seek  (mng_datap      pData);
#endif
#ifdef MNG_INCLUDE_JNG
mng_retcode mng_process_display_jhdr  (mng_datap      pData);
mng_retcode mng_process_display_jdaa  (mng_datap      pData);
mng_retcode mng_process_display_jdat  (mng_datap      pData);
#endif
#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_process_display_dhdr  (mng_datap      pData);
mng_retcode mng_process_display_prom  (mng_datap      pData);
mng_retcode mng_process_display_ipng  (mng_datap      pData);
#ifdef MNG_INCLUDE_JNG
mng_retcode mng_process_display_ijng  (mng_datap      pData);
#endif
mng_retcode mng_process_display_pplt  (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_MAGN
mng_retcode mng_process_display_magn  (mng_datap      pData);
mng_retcode mng_process_display_magn2 (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_process_display_past  (mng_datap      pData);
mng_retcode mng_process_display_past2 (mng_datap      pData);
#endif

#endif /* MNG_OPTIMIZE_DISPLAYCALLS */

/* ************************************************************************** */

#endif /* MNG_INCLUDE_DISPLAY_PROCS */

/* ************************************************************************** */

#endif /* _libmng_display_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
