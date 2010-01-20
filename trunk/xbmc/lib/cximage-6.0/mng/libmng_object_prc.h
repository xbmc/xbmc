/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_object_prc.h       copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Object processing routines (definition)                    * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of the internal object processing routines      * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *                                                                        * */
/* *             0.5.2 - 05/24/2000 - G.Juyn                                * */
/* *             - added support for global color-chunks in animation       * */
/* *             - added support for global PLTE,tRNS,bKGD in animation     * */
/* *             - added SAVE & SEEK animation objects                      * */
/* *             0.5.2 - 05/29/2000 - G.Juyn                                * */
/* *             - changed ani_object create routines not to return the     * */
/* *               created object (wasn't necessary)                        * */
/* *             - added compression/filter/interlace fields to             * */
/* *               object-buffer for delta-image processing                 * */
/* *                                                                        * */
/* *             0.5.3 - 06/22/2000 - G.Juyn                                * */
/* *             - added support for PPLT chunk                             * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 10/17/2000 - G.Juyn                                * */
/* *             - added routine to discard "invalid" objects               * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             1.0.5 - 09/13/2002 - G.Juyn                                * */
/* *             - fixed read/write of MAGN chunk                           * */
/* *             1.0.5 - 09/15/2002 - G.Juyn                                * */
/* *             - added event handling for dynamic MNG                     * */
/* *             1.0.5 - 09/20/2002 - G.Juyn                                * */
/* *             - added support for PAST                                   * */
/* *             1.0.5 - 09/23/2002 - G.Juyn                                * */
/* *             - added in-memory color-correction of abstract images      * */
/* *             1.0.5 - 10/07/2002 - G.Juyn                                * */
/* *             - fixed DISC support                                       * */
/* *                                                                        * */
/* *             1.0.6 - 07/07/2003 - G.R-P                                 * */
/* *             - added conditionals around Delta-PNG code                 * */
/* *             - added SKIPCHUNK feature                                  * */
/* *             1.0.6 - 07/29/2003 - G.R-P                                 * */
/* *             - added conditionals around PAST chunk support             * */
/* *                                                                        * */
/* *             1.0.7 - 03/24/2004 - G.R-P                                 * */
/* *             - added more SKIPCHUNK conditionals                        * */
/* *                                                                        * */
/* *             1.0.9 - 12/05/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_OBJCLEANUP                * */
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

#ifndef _libmng_object_prc_h_
#define _libmng_object_prc_h_

/* ************************************************************************** */

#ifdef MNG_INCLUDE_DISPLAY_PROCS

/* ************************************************************************** */

mng_retcode mng_drop_invalid_objects   (mng_datap      pData);

/* ************************************************************************** */

mng_retcode mng_create_imagedataobject (mng_datap      pData,
                                        mng_bool       bConcrete,
                                        mng_bool       bViewable,
                                        mng_uint32     iWidth,
                                        mng_uint32     iHeight,
                                        mng_uint8      iBitdepth,
                                        mng_uint8      iColortype,
                                        mng_uint8      iCompression,
                                        mng_uint8      iFilter,
                                        mng_uint8      iInterlace,
                                        mng_imagedatap *ppObject);

mng_retcode mng_free_imagedataobject   (mng_datap      pData,
                                        mng_imagedatap pImagedata);

mng_retcode mng_clone_imagedataobject  (mng_datap      pData,
                                        mng_bool       bConcrete,
                                        mng_imagedatap pSource,
                                        mng_imagedatap *ppClone);

/* ************************************************************************** */

mng_retcode mng_create_imageobject   (mng_datap  pData,
                                      mng_uint16 iId,
                                      mng_bool   bConcrete,
                                      mng_bool   bVisible,
                                      mng_bool   bViewable,
                                      mng_uint32 iWidth,
                                      mng_uint32 iHeight,
                                      mng_uint8  iBitdepth,
                                      mng_uint8  iColortype,
                                      mng_uint8  iCompression,
                                      mng_uint8  iFilter,
                                      mng_uint8  iInterlace,
                                      mng_int32  iPosx,
                                      mng_int32  iPosy,
                                      mng_bool   bClipped,
                                      mng_int32  iClipl,
                                      mng_int32  iClipr,
                                      mng_int32  iClipt,
                                      mng_int32  iClipb,
                                      mng_imagep *ppObject);

mng_retcode mng_free_imageobject     (mng_datap  pData,
                                      mng_imagep pImage);

mng_imagep  mng_find_imageobject     (mng_datap  pData,
                                      mng_uint16 iId);

mng_retcode mng_clone_imageobject    (mng_datap  pData,
                                      mng_uint16 iId,
                                      mng_bool   bPartial,
                                      mng_bool   bVisible,
                                      mng_bool   bAbstract,
                                      mng_bool   bHasloca,
                                      mng_uint8  iLocationtype,
                                      mng_int32  iLocationx,
                                      mng_int32  iLocationy,
                                      mng_imagep pSource,
                                      mng_imagep *ppClone);

mng_retcode mng_renum_imageobject    (mng_datap  pData,
                                      mng_imagep pSource,
                                      mng_uint16 iId,
                                      mng_bool   bVisible,
                                      mng_bool   bAbstract,
                                      mng_bool   bHasloca,
                                      mng_uint8  iLocationtype,
                                      mng_int32  iLocationx,
                                      mng_int32  iLocationy);

mng_retcode mng_reset_object_details (mng_datap  pData,
                                      mng_imagep pImage,
                                      mng_uint32 iWidth,
                                      mng_uint32 iHeight,
                                      mng_uint8  iBitdepth,
                                      mng_uint8  iColortype,
                                      mng_uint8  iCompression,
                                      mng_uint8  iFilter,
                                      mng_uint8  iInterlace,
                                      mng_bool   bResetall);

mng_retcode mng_promote_imageobject  (mng_datap  pData,
                                      mng_imagep pImage,
                                      mng_uint8  iBitdepth,
                                      mng_uint8  iColortype,
                                      mng_uint8  iFilltype);

mng_retcode mng_magnify_imageobject  (mng_datap  pData,
                                      mng_imagep pImage);

mng_retcode mng_colorcorrect_object  (mng_datap  pData,
                                      mng_imagep pImage);

/* ************************************************************************** */

mng_retcode mng_create_ani_image  (mng_datap      pData);

#ifndef MNG_OPTIMIZE_CHUNKREADER

mng_retcode mng_create_ani_plte   (mng_datap      pData,
                                   mng_uint32     iEntrycount,
                                   mng_palette8ep paEntries);

mng_retcode mng_create_ani_trns   (mng_datap      pData,
                                   mng_uint32     iRawlen,
                                   mng_uint8p     pRawdata);

mng_retcode mng_create_ani_gama   (mng_datap      pData,
                                   mng_bool       bEmpty,
                                   mng_uint32     iGamma);

mng_retcode mng_create_ani_chrm   (mng_datap      pData,
                                   mng_bool       bEmpty,
                                   mng_uint32     iWhitepointx,
                                   mng_uint32     iWhitepointy,
                                   mng_uint32     iRedx,
                                   mng_uint32     iRedy,
                                   mng_uint32     iGreenx,
                                   mng_uint32     iGreeny,
                                   mng_uint32     iBluex,
                                   mng_uint32     iBluey);

mng_retcode mng_create_ani_srgb   (mng_datap      pData,
                                   mng_bool       bEmpty,
                                   mng_uint8      iRenderinginent);

mng_retcode mng_create_ani_iccp   (mng_datap      pData,
                                   mng_bool       bEmpty,
                                   mng_uint32     iProfilesize,
                                   mng_ptr        pProfile);

mng_retcode mng_create_ani_bkgd   (mng_datap      pData,
                                   mng_uint16     iRed,
                                   mng_uint16     iGreen,
                                   mng_uint16     iBlue);

mng_retcode mng_create_ani_loop   (mng_datap      pData,
                                   mng_uint8      iLevel,
                                   mng_uint32     iRepeatcount,
                                   mng_uint8      iTermcond,
                                   mng_uint32     iItermin,
                                   mng_uint32     iItermax,
                                   mng_uint32     iCount,
                                   mng_uint32p    pSignals);

mng_retcode mng_create_ani_endl   (mng_datap      pData,
                                   mng_uint8      iLevel);

mng_retcode mng_create_ani_defi   (mng_datap      pData);

mng_retcode mng_create_ani_basi   (mng_datap      pData,
                                   mng_uint16     iRed,
                                   mng_uint16     iGreen,
                                   mng_uint16     iBlue,
                                   mng_bool       bHasalpha,
                                   mng_uint16     iAlpha,
                                   mng_uint8      iViewable);

mng_retcode mng_create_ani_clon   (mng_datap      pData,
                                   mng_uint16     iSourceid,
                                   mng_uint16     iCloneid,
                                   mng_uint8      iClonetype,
                                   mng_bool       bHasdonotshow,
                                   mng_uint8      iDonotshow,
                                   mng_uint8      iConcrete,
                                   mng_bool       bHasloca,
                                   mng_uint8      iLocatype,
                                   mng_int32      iLocax,
                                   mng_int32      iLocay);

mng_retcode mng_create_ani_back   (mng_datap      pData,
                                   mng_uint16     iRed,
                                   mng_uint16     iGreen,
                                   mng_uint16     iBlue,
                                   mng_uint8      iMandatory,
                                   mng_uint16     iImageid,
                                   mng_uint8      iTile);

mng_retcode mng_create_ani_fram   (mng_datap      pData,
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

mng_retcode mng_create_ani_move   (mng_datap      pData,
                                   mng_uint16     iFirstid,
                                   mng_uint16     iLastid,
                                   mng_uint8      iType,
                                   mng_int32      iLocax,
                                   mng_int32      iLocay);

mng_retcode mng_create_ani_clip   (mng_datap      pData,
                                   mng_uint16     iFirstid,
                                   mng_uint16     iLastid,
                                   mng_uint8      iType,
                                   mng_int32      iClipl,
                                   mng_int32      iClipr,
                                   mng_int32      iClipt,
                                   mng_int32      iClipb);

mng_retcode mng_create_ani_show   (mng_datap      pData,
                                   mng_uint16     iFirstid,
                                   mng_uint16     iLastid,
                                   mng_uint8      iMode);

mng_retcode mng_create_ani_term   (mng_datap      pData,
                                   mng_uint8      iTermaction,
                                   mng_uint8      iIteraction,
                                   mng_uint32     iDelay,
                                   mng_uint32     iItermax);

#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_create_ani_save   (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode mng_create_ani_seek   (mng_datap      pData,
                                   mng_uint32     iSegmentnamesize,
                                   mng_pchar      zSegmentname);
#endif
#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_create_ani_dhdr   (mng_datap      pData,
                                   mng_uint16     iObjectid,
                                   mng_uint8      iImagetype,
                                   mng_uint8      iDeltatype,
                                   mng_uint32     iBlockwidth,
                                   mng_uint32     iBlockheight,
                                   mng_uint32     iBlockx,
                                   mng_uint32     iBlocky);

mng_retcode mng_create_ani_prom   (mng_datap      pData,
                                   mng_uint8      iBitdepth,
                                   mng_uint8      iColortype,
                                   mng_uint8      iFilltype);

mng_retcode mng_create_ani_ipng   (mng_datap      pData);
mng_retcode mng_create_ani_ijng   (mng_datap      pData);

mng_retcode mng_create_ani_pplt   (mng_datap      pData,
                                   mng_uint8      iType,
                                   mng_uint32     iCount,
                                   mng_palette8ep paIndexentries,
                                   mng_uint8p     paAlphaentries,
                                   mng_uint8p     paUsedentries);
#endif

#ifndef MNG_SKIPCHUNK_MAGN
mng_retcode mng_create_ani_magn   (mng_datap      pData,
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
#endif

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_create_ani_past   (mng_datap      pData,
                                   mng_uint16     iTargetid,
                                   mng_uint8      iTargettype,
                                   mng_int32      iTargetx,
                                   mng_int32      iTargety,
                                   mng_uint32     iCount,
                                   mng_ptr        pSources);
#endif

#ifndef MNG_SKIPCHUNK_DISC
mng_retcode mng_create_ani_disc   (mng_datap      pData,
                                   mng_uint32     iCount,
                                   mng_uint16p    pIds);
#endif

#else /* MNG_OPTIMIZE_CHUNKREADER */

mng_retcode mng_create_ani_plte   (mng_datap      pData);
mng_retcode mng_create_ani_trns   (mng_datap      pData);
mng_retcode mng_create_ani_gama   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_chrm   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_srgb   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_iccp   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_bkgd   (mng_datap      pData);
mng_retcode mng_create_ani_loop   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_endl   (mng_datap      pData,
                                   mng_uint8      iLevel);
mng_retcode mng_create_ani_defi   (mng_datap      pData);
mng_retcode mng_create_ani_basi   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_clon   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_back   (mng_datap      pData);
mng_retcode mng_create_ani_fram   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_move   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_clip   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_show   (mng_datap      pData);
mng_retcode mng_create_ani_term   (mng_datap      pData,
                                   mng_chunkp     pChunk);
#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_create_ani_save   (mng_datap      pData);
#endif
#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode mng_create_ani_seek   (mng_datap      pData,
                                   mng_chunkp     pChunk);
#endif
#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_create_ani_dhdr   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_prom   (mng_datap      pData,
                                   mng_chunkp     pChunk);
mng_retcode mng_create_ani_ipng   (mng_datap      pData);
mng_retcode mng_create_ani_ijng   (mng_datap      pData);

mng_retcode mng_create_ani_pplt   (mng_datap      pData,
                                   mng_uint8      iType,
                                   mng_uint32     iCount,
                                   mng_palette8ep paIndexentries,
                                   mng_uint8p     paAlphaentries,
                                   mng_uint8p     paUsedentries);
#endif

#ifndef MNG_SKIPCHUNK_MAGN
mng_retcode mng_create_ani_magn   (mng_datap      pData,
                                   mng_chunkp     pChunk);
#endif
#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_create_ani_past   (mng_datap      pData,
                                   mng_chunkp     pChunk);
#endif
#ifndef MNG_SKIPCHUNK_DISC
mng_retcode mng_create_ani_disc   (mng_datap      pData,
                                   mng_chunkp     pChunk);
#endif

#endif /* MNG_OPTIMIZE_CHUNKREADER */

/* ************************************************************************** */

mng_retcode mng_free_ani_image    (mng_datap    pData,
                                   mng_objectp  pObject);

#ifndef MNG_OPTIMIZE_OBJCLEANUP

mng_retcode mng_free_ani_plte     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_trns     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_gama     (mng_datap    pData,
                                   mng_objectp  pObject);
#ifndef MNG_SKIPCHUNK_cHRM
mng_retcode mng_free_ani_chrm     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_sRGB
mng_retcode mng_free_ani_srgb     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_bKGD
mng_retcode mng_free_ani_bkgd     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_LOOP
mng_retcode mng_free_ani_endl     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
mng_retcode mng_free_ani_defi     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_basi     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_clon     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_back     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_fram     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_move     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_clip     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_show     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_term     (mng_datap    pData,
                                   mng_objectp  pObject);
#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_free_ani_save     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_free_ani_dhdr     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_prom     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_ipng     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_ijng     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_free_ani_pplt     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_MAGN
mng_retcode mng_free_ani_magn     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif

#endif /* MNG_OPTIMIZE_OBJCLEANUP */


#ifndef MNG_SKIPCHUNK_iCCP
mng_retcode mng_free_ani_iccp     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_LOOP
mng_retcode mng_free_ani_loop     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_free_ani_seek     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_free_ani_past     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
mng_retcode mng_free_ani_disc     (mng_datap    pData,
                                   mng_objectp  pObject);

/* ************************************************************************** */

mng_retcode mng_process_ani_image (mng_datap    pData,
                                   mng_objectp  pObject);

mng_retcode mng_process_ani_plte  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_trns  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_gama  (mng_datap    pData,
                                   mng_objectp  pObject);
#ifndef MNG_SKIPCHUNK_cHRM
mng_retcode mng_process_ani_chrm  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_sRGB
mng_retcode mng_process_ani_srgb  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_iCCP
mng_retcode mng_process_ani_iccp  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_bKGD
mng_retcode mng_process_ani_bkgd  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_LOOP
mng_retcode mng_process_ani_loop  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_endl  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
mng_retcode mng_process_ani_defi  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_basi  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_clon  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_back  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_fram  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_move  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_clip  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_show  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_term  (mng_datap    pData,
                                   mng_objectp  pObject);
#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_process_ani_save  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode mng_process_ani_seek  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_process_ani_dhdr  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_prom  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_ipng  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_ijng  (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ani_pplt  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
mng_retcode mng_process_ani_magn  (mng_datap    pData,
                                   mng_objectp  pObject);
#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_process_ani_past  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif
mng_retcode mng_process_ani_disc  (mng_datap    pData,
                                   mng_objectp  pObject);

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DYNAMICMNG
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_event      (mng_datap    pData,
                                   mng_uint8    iEventtype,
                                   mng_uint8    iMasktype,
                                   mng_int32    iLeft,
                                   mng_int32    iRight,
                                   mng_int32    iTop,
                                   mng_int32    iBottom,
                                   mng_uint16   iObjectid,
                                   mng_uint8    iIndex,
                                   mng_uint32   iSegmentnamesize,
                                   mng_pchar    zSegmentname);
#else
mng_retcode mng_create_event      (mng_datap    pData,
                                   mng_ptr      pEntry);
#endif
mng_retcode mng_free_event        (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_event     (mng_datap    pData,
                                   mng_objectp  pObject);
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_mpng_obj   (mng_datap    pData,
                                   mng_uint32   iFramewidth,
                                   mng_uint32   iFrameheight,
                                   mng_uint16   iNumplays,
                                   mng_uint16   iTickspersec,
                                   mng_uint32   iFramessize,
                                   mng_ptr      pFrames);
#else
mng_retcode mng_create_mpng_obj   (mng_datap    pData,
                                   mng_ptr      pEntry);
#endif
mng_retcode mng_free_mpng_obj     (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_mpng_obj  (mng_datap    pData,
                                   mng_objectp  pObject);
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ang_obj    (mng_datap    pData,
                                   mng_uint32   iNumframes,
                                   mng_uint32   iTickspersec,
                                   mng_uint32   iNumplays,
                                   mng_uint32   iTilewidth,
                                   mng_uint32   iTileheight,
                                   mng_uint8    iInterlace,
                                   mng_uint8    iStillused);
#else
mng_retcode mng_create_ang_obj    (mng_datap    pData,
                                   mng_ptr      pEntry);
#endif
mng_retcode mng_free_ang_obj      (mng_datap    pData,
                                   mng_objectp  pObject);
mng_retcode mng_process_ang_obj   (mng_datap    pData,
                                   mng_objectp  pObject);
#endif

/* ************************************************************************** */

#endif /* MNG_INCLUDE_DISPLAY_PROCS */

/* ************************************************************************** */

#endif /* _libmng_object_prc_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
