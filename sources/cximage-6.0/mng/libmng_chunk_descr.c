/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_chunk_descr.c      copyright (c) 2005-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Chunk descriptor functions (implementation)                * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the chunk- anf field-descriptor          * */
/* *             routines                                                   * */
/* *                                                                        * */
/* * changes   : 1.0.9 - 12/06/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_CHUNKREADER               * */
/* *             1.0.9 - 12/11/2004 - G.Juyn                                * */
/* *             - made all constants 'static'                              * */
/* *             1.0.9 - 12/20/2004 - G.Juyn                                * */
/* *             - cleaned up macro-invocations (thanks to D. Airlie)       * */
/* *             1.0.9 - 01/17/2005 - G.Juyn                                * */
/* *             - fixed problem with global PLTE/tRNS                      * */
/* *                                                                        * */
/* *             1.0.10 - 01/17/2005 - G.R-P.                               * */
/* *             - added typecast to appease the compiler                   * */
/* *             1.0.10 - 04/08/2007 - G.Juyn                               * */
/* *             - added support for mPNG proposal                          * */
/* *             1.0.10 - 04/12/2007 - G.Juyn                               * */
/* *             - added support for ANG proposal                           * */
/* *                                                                        * */
/* ************************************************************************** */

#include <stddef.h>                    /* needed for offsetof() */

#include "libmng.h"
#include "libmng_data.h"
#include "libmng_error.h"
#include "libmng_trace.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#include "libmng_memory.h"
#include "libmng_objects.h"
#include "libmng_chunks.h"
#include "libmng_chunk_descr.h"
#include "libmng_object_prc.h"
#include "libmng_chunk_prc.h"
#include "libmng_chunk_io.h"
#include "libmng_display.h"

#ifdef MNG_INCLUDE_ANG_PROPOSAL
#include "libmng_pixels.h"
#include "libmng_filter.h"
#endif

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */

#ifdef MNG_OPTIMIZE_CHUNKREADER
#if defined(MNG_INCLUDE_READ_PROCS) || defined(MNG_INCLUDE_WRITE_PROCS)

/* ************************************************************************** */
/* ************************************************************************** */
/* PNG chunks */

MNG_LOCAL mng_field_descriptor mng_fields_ihdr [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_NOHIGHBIT,
     1, 0, 4, 4,
     offsetof(mng_ihdr, iWidth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_NOHIGHBIT,
     1, 0, 4, 4,
     offsetof(mng_ihdr, iHeight), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 16, 1, 1,
     offsetof(mng_ihdr, iBitdepth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 6, 1, 1,
     offsetof(mng_ihdr, iColortype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_ihdr, iCompression), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_ihdr, iFilter), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_ihdr, iInterlace), MNG_NULL, MNG_NULL}
  };

/* ************************************************************************** */

MNG_LOCAL mng_field_descriptor mng_fields_plte [] =
  {
    {mng_debunk_plte,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };

/* ************************************************************************** */

MNG_LOCAL mng_field_descriptor mng_fields_idat [] =
  {
    {MNG_NULL,
     MNG_NULL,
     0, 0, 0, 0,
     offsetof(mng_idat, pData), MNG_NULL, offsetof(mng_idat, iDatasize)}
  };

/* ************************************************************************** */

MNG_LOCAL mng_field_descriptor mng_fields_trns [] =
  {
    {mng_debunk_trns,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_gAMA
MNG_LOCAL mng_field_descriptor mng_fields_gama [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_gama, iGamma), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_cHRM
MNG_LOCAL mng_field_descriptor mng_fields_chrm [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_chrm, iWhitepointx), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_chrm, iWhitepointy), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_chrm, iRedx), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_chrm, iRedy), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_chrm, iGreeny), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_chrm, iGreeny), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_chrm, iBluex), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_chrm, iBluey), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sRGB
MNG_LOCAL mng_field_descriptor mng_fields_srgb [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 4, 1, 1,
     offsetof(mng_srgb, iRenderingintent), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
MNG_LOCAL mng_field_descriptor mng_fields_iccp [] =
  {
    {MNG_NULL,
     MNG_FIELD_TERMINATOR,
     0, 0, 1, 79,
     offsetof(mng_iccp, zName), MNG_NULL, offsetof(mng_iccp, iNamesize)},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_iccp, iCompression), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_DEFLATED,
     0, 0, 0, 0,
     offsetof(mng_iccp, pProfile), MNG_NULL, offsetof(mng_iccp, iProfilesize)}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tEXt
MNG_LOCAL mng_field_descriptor mng_fields_text [] =
  {
    {MNG_NULL,
     MNG_FIELD_TERMINATOR,
     0, 0, 1, 79,
     offsetof(mng_text, zKeyword), MNG_NULL, offsetof(mng_text, iKeywordsize)},
    {MNG_NULL,
     MNG_NULL,
     0, 0, 0, 0,
     offsetof(mng_text, zText), MNG_NULL, offsetof(mng_text, iTextsize)}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_zTXt
MNG_LOCAL mng_field_descriptor mng_fields_ztxt [] =
  {
    {MNG_NULL,
     MNG_FIELD_TERMINATOR,
     0, 0, 1, 79,
     offsetof(mng_ztxt, zKeyword), MNG_NULL, offsetof(mng_ztxt, iKeywordsize)},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_ztxt, iCompression), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_DEFLATED,
     0, 0, 0, 0,
     offsetof(mng_ztxt, zText), MNG_NULL, offsetof(mng_ztxt, iTextsize)}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iTXt
MNG_LOCAL mng_field_descriptor mng_fields_itxt [] =
  {
    {MNG_NULL,
     MNG_FIELD_TERMINATOR,
     0, 0, 1, 79,
     offsetof(mng_itxt, zKeyword), MNG_NULL, offsetof(mng_itxt, iKeywordsize)},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_itxt, iCompressionflag), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_itxt, iCompressionmethod), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_TERMINATOR,
     0, 0, 0, 0,
     offsetof(mng_itxt, zLanguage), MNG_NULL, offsetof(mng_itxt, iLanguagesize)},
    {MNG_NULL,
     MNG_FIELD_TERMINATOR,
     0, 0, 0, 0,
     offsetof(mng_itxt, zTranslation), MNG_NULL, offsetof(mng_itxt, iTranslationsize)},
    {mng_deflate_itxt,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_bKGD
MNG_LOCAL mng_field_descriptor mng_fields_bkgd [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_PUTIMGTYPE,
     0, 0, 0, 0,
     offsetof(mng_bkgd, iType), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPE3,
     0, 0xFF, 1, 1,
     offsetof(mng_bkgd, iIndex), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPE0 | MNG_FIELD_IFIMGTYPE4,
     0, 0xFFFF, 2, 2,
     offsetof(mng_bkgd, iGray), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPE2 | MNG_FIELD_IFIMGTYPE6,
     0, 0xFFFF, 2, 2,
     offsetof(mng_bkgd, iRed), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPE2 | MNG_FIELD_IFIMGTYPE6,
     0, 0xFFFF, 2, 2,
     offsetof(mng_bkgd, iGreen), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPE2 | MNG_FIELD_IFIMGTYPE6,
     0, 0xFFFF, 2, 2,
     offsetof(mng_bkgd, iBlue), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYs
MNG_LOCAL mng_field_descriptor mng_fields_phys [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_phys, iSizex), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_phys, iSizey), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_phys, iUnit), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sBIT
MNG_LOCAL mng_field_descriptor mng_fields_sbit [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_PUTIMGTYPE,
     0, 0, 0, 0,
     offsetof(mng_sbit, iType), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPES,
     0, 0xFF, 1, 1,
     offsetof(mng_sbit, aBits[0]), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPE2 | MNG_FIELD_IFIMGTYPE3 | MNG_FIELD_IFIMGTYPE4 | MNG_FIELD_IFIMGTYPE6,
     0, 0xFF, 1, 1,
     offsetof(mng_sbit, aBits[1]), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPE2 | MNG_FIELD_IFIMGTYPE3 | MNG_FIELD_IFIMGTYPE6,
     0, 0xFF, 1, 1,
     offsetof(mng_sbit, aBits[2]), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_IFIMGTYPE6,
     0, 0xFF, 1, 1,
     offsetof(mng_sbit, aBits[3]), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
MNG_LOCAL mng_field_descriptor mng_fields_splt [] =
  {
    {MNG_NULL,
     MNG_NULL,
     0, 0, 1, 79,
     offsetof(mng_splt, zName), MNG_NULL, offsetof(mng_splt, iNamesize)},
    {MNG_NULL,
     MNG_FIELD_INT,
     8, 16, 1, 1,
     offsetof(mng_splt, iSampledepth), MNG_NULL, MNG_NULL},
    {mng_splt_entries,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_hIST
MNG_LOCAL mng_field_descriptor mng_fields_hist [] =
  {
    {mng_hist_entries,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tIME
MNG_LOCAL mng_field_descriptor mng_fields_time [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_time, iYear), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 12, 1, 1,
     offsetof(mng_time, iMonth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 31, 1, 1,
     offsetof(mng_time, iDay), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 24, 1, 1,
     offsetof(mng_time, iHour), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 60, 1, 1,
     offsetof(mng_time, iMinute), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 60, 1, 1,
     offsetof(mng_time, iSecond), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */
/* ************************************************************************** */
/* JNG chunks */

#ifdef MNG_INCLUDE_JNG
MNG_LOCAL mng_field_descriptor mng_fields_jhdr [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_NOHIGHBIT,
     1, 0, 4, 4,
     offsetof(mng_jhdr, iWidth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_NOHIGHBIT,
     1, 0, 4, 4,
     offsetof(mng_jhdr, iHeight), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     8, 16, 1, 1,
     offsetof(mng_jhdr, iColortype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     8, 20, 1, 1,
     offsetof(mng_jhdr, iImagesampledepth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     8, 8, 1, 1,
     offsetof(mng_jhdr, iImagecompression), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 8, 1, 1,
     offsetof(mng_jhdr, iImageinterlace), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 16, 1, 1,
     offsetof(mng_jhdr, iAlphasampledepth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 8, 1, 1,
     offsetof(mng_jhdr, iAlphacompression), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_jhdr, iAlphafilter), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_jhdr, iAlphainterlace), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
#define mng_fields_jdaa mng_fields_idat
#define mng_fields_jdat mng_fields_idat
#endif

/* ************************************************************************** */
/* ************************************************************************** */
/* MNG chunks */

MNG_LOCAL mng_field_descriptor mng_fields_mhdr [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_mhdr, iWidth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_mhdr, iHeight), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_mhdr, iTicks), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_mhdr, iLayercount), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_mhdr, iFramecount), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_mhdr, iPlaytime), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_mhdr, iSimplicity), MNG_NULL, MNG_NULL}
  };

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
MNG_LOCAL mng_field_descriptor mng_fields_loop [] =
  {
    {mng_debunk_loop,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
MNG_LOCAL mng_field_descriptor mng_fields_endl [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFF, 1, 1,
     offsetof(mng_endl, iLevel), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DEFI
MNG_LOCAL mng_field_descriptor mng_fields_defi [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_defi, iObjectid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 0xFF, 1, 1,
     offsetof(mng_defi, iDonotshow), offsetof(mng_defi, bHasdonotshow), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 0xFF, 1, 1,
     offsetof(mng_defi, iConcrete), offsetof(mng_defi, bHasconcrete), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0, 4, 4,
     offsetof(mng_defi, iXlocation), offsetof(mng_defi, bHasloca), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0, 4, 4,
     offsetof(mng_defi, iYlocation), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP2,
     0, 0, 4, 4,
     offsetof(mng_defi, iLeftcb), offsetof(mng_defi, bHasclip), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP2,
     0, 0, 4, 4,
     offsetof(mng_defi, iRightcb), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP2,
     0, 0, 4, 4,
     offsetof(mng_defi, iTopcb), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP2,
     0, 0, 4, 4,
     offsetof(mng_defi, iBottomcb), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BASI
MNG_LOCAL mng_field_descriptor mng_fields_basi [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_basi, iWidth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_basi, iHeight), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 16, 1, 1,
     offsetof(mng_basi, iBitdepth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 6, 1, 1,
     offsetof(mng_basi, iColortype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_basi, iCompression), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_basi, iFilter), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_basi, iInterlace), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0xFFFF, 2, 2,
     offsetof(mng_basi, iRed), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0xFFFF, 2, 2,
     offsetof(mng_basi, iGreen), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0xFFFF, 2, 2,
     offsetof(mng_basi, iBlue), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 0xFFFF, 2, 2,
     offsetof(mng_basi, iAlpha), offsetof(mng_basi, bHasalpha), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 1, 1, 1,
     offsetof(mng_basi, iViewable), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLON
MNG_LOCAL mng_field_descriptor mng_fields_clon [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_clon, iSourceid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_clon, iCloneid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 2, 1, 1,
     offsetof(mng_clon, iClonetype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 1, 1, 1,
     offsetof(mng_clon, iDonotshow), offsetof(mng_clon, bHasdonotshow), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 1, 1, 1,
     offsetof(mng_clon, iConcrete), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 2, 1, 1,
     offsetof(mng_clon, iLocationtype), offsetof(mng_clon, bHasloca), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0, 4, 4,
     offsetof(mng_clon, iLocationx), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0, 4, 4,
     offsetof(mng_clon, iLocationy), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
MNG_LOCAL mng_field_descriptor mng_fields_past [] =
  {
    {mng_debunk_past,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
MNG_LOCAL mng_field_descriptor mng_fields_disc [] =
  {
    {mng_disc_entries,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BACK
MNG_LOCAL mng_field_descriptor mng_fields_back [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_back, iRed), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_back, iGreen), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_back, iBlue), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 3, 1, 1,
     offsetof(mng_back, iMandatory), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 0xFFFF, 2, 2,
     offsetof(mng_back, iImageid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 1, 1, 1,
     offsetof(mng_back, iTile), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
MNG_LOCAL mng_field_descriptor mng_fields_fram [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 4, 1, 1,
     offsetof(mng_fram, iMode), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_TERMINATOR | MNG_FIELD_OPTIONAL,
     0, 0, 1, 79,
     offsetof(mng_fram, zName), MNG_NULL, offsetof(mng_fram, iNamesize)},
    {mng_fram_remainder,
     MNG_FIELD_OPTIONAL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MOVE
MNG_LOCAL mng_field_descriptor mng_fields_move [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_move, iFirstid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_move, iLastid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_move, iMovetype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_move, iMovex), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_move, iMovey), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLIP
MNG_LOCAL mng_field_descriptor mng_fields_clip [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_clip, iFirstid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_clip, iLastid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_clip, iCliptype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_clip, iClipl), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_clip, iClipr), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_clip, iClipt), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_clip, iClipb), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SHOW
MNG_LOCAL mng_field_descriptor mng_fields_show [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 0xFFFF, 2, 2,
     offsetof(mng_show, iFirstid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     1, 0xFFFF, 2, 2,
     offsetof(mng_show, iLastid), offsetof(mng_show, bHaslastid), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL,
     0, 7, 1, 1,
     offsetof(mng_show, iMode), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_TERM
MNG_LOCAL mng_field_descriptor mng_fields_term [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 3, 1, 1,
     offsetof(mng_term, iTermaction), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 2, 1, 1,
     offsetof(mng_term, iIteraction), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0, 4, 4,
     offsetof(mng_term, iDelay), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0, 4, 4,
     offsetof(mng_term, iItermax), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
MNG_LOCAL mng_field_descriptor mng_fields_save [] =
  {
    {mng_save_entries,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
MNG_LOCAL mng_field_descriptor mng_fields_seek [] =
  {
    {MNG_NULL,
     MNG_NULL,
     0, 0, 1, 79,
     offsetof(mng_seek, zName), MNG_NULL, offsetof(mng_seek, iNamesize)}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_eXPI
MNG_LOCAL mng_field_descriptor mng_fields_expi [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_expi, iSnapshotid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_NULL,
     0, 0, 1, 79,
     offsetof(mng_expi, zName), MNG_NULL, offsetof(mng_expi, iNamesize)}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_fPRI
MNG_LOCAL mng_field_descriptor mng_fields_fpri [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_fpri, iDeltatype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFF, 1, 1,
     offsetof(mng_fpri, iPriority), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
MNG_LOCAL mng_field_descriptor mng_fields_need [] =
  {
    {MNG_NULL,
     MNG_NULL,
     0, 0, 1, 0,
     offsetof(mng_need, zKeywords), MNG_NULL, offsetof(mng_need, iKeywordssize)}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYg
#define mng_fields_phyg mng_fields_phys
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_field_descriptor mng_fields_dhdr [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_dhdr, iObjectid), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 2, 1, 1,
     offsetof(mng_dhdr, iImagetype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 7, 1, 1,
     offsetof(mng_dhdr, iDeltatype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0, 4, 4,
     offsetof(mng_dhdr, iBlockwidth), offsetof(mng_dhdr, bHasblocksize), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP1,
     0, 0, 4, 4,
     offsetof(mng_dhdr, iBlockheight), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP2,
     0, 0, 4, 4,
     offsetof(mng_dhdr, iBlockx), offsetof(mng_dhdr, bHasblockloc), MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT | MNG_FIELD_OPTIONAL | MNG_FIELD_GROUP2,
     0, 0, 4, 4,
     offsetof(mng_dhdr, iBlocky), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_field_descriptor mng_fields_prom [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 14, 1, 1,
     offsetof(mng_prom, iColortype), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 16, 1, 1,
     offsetof(mng_prom, iSampledepth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_prom, iFilltype), MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_field_descriptor mng_fields_pplt [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 5, 1, 1,
     offsetof(mng_pplt, iDeltatype), MNG_NULL, MNG_NULL},
    {mng_pplt_entries,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_field_descriptor mng_fields_drop [] =
  {
    {mng_drop_entries,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
MNG_LOCAL mng_field_descriptor mng_fields_dbyk [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_dbyk, iChunkname), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_dbyk, iPolarity), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_NULL,
     0, 0, 1, 0,
     offsetof(mng_dbyk, zKeywords), MNG_NULL, offsetof(mng_dbyk, iKeywordssize)}
  };
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
MNG_LOCAL mng_field_descriptor mng_fields_ordr [] =
  {
    {mng_drop_entries,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
MNG_LOCAL mng_field_descriptor mng_fields_magn [] =
  {
    {mng_debunk_magn,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
MNG_LOCAL mng_field_descriptor mng_fields_mpng [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 0, 4, 4,
     offsetof(mng_mpng, iFramewidth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 0, 4, 4,
     offsetof(mng_mpng, iFrameheight), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0xFFFF, 2, 2,
     offsetof(mng_mpng, iNumplays), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 0xFFFF, 2, 2,
     offsetof(mng_mpng, iTickspersec), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 1, 1,
     offsetof(mng_mpng, iCompressionmethod), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_DEFLATED,
     0, 0, 1, 0,
     offsetof(mng_mpng, pFrames), MNG_NULL, offsetof(mng_mpng, iFramessize)}
  };
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
MNG_LOCAL mng_field_descriptor mng_fields_ahdr [] =
  {
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 0, 4, 4,
     offsetof(mng_ahdr, iNumframes), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_ahdr, iTickspersec), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 0, 4, 4,
     offsetof(mng_ahdr, iNumplays), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 0, 4, 4,
     offsetof(mng_ahdr, iTilewidth), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     1, 0, 4, 4,
     offsetof(mng_ahdr, iTileheight), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_ahdr, iInterlace), MNG_NULL, MNG_NULL},
    {MNG_NULL,
     MNG_FIELD_INT,
     0, 1, 1, 1,
     offsetof(mng_ahdr, iStillused), MNG_NULL, MNG_NULL}
  };

MNG_LOCAL mng_field_descriptor mng_fields_adat [] =
  {
    {mng_adat_tiles,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_evNT
MNG_LOCAL mng_field_descriptor mng_fields_evnt [] =
  {
    {mng_evnt_entries,
     MNG_NULL,
     0, 0, 0, 0,
     MNG_NULL, MNG_NULL, MNG_NULL}
  };
#endif

/* ************************************************************************** */

MNG_LOCAL mng_field_descriptor mng_fields_unknown [] =
  {
    {MNG_NULL,
     MNG_NULL,
     0, 0, 1, 0,
     offsetof(mng_unknown_chunk, pData), MNG_NULL, offsetof(mng_unknown_chunk, iDatasize)}
  };

/* ************************************************************************** */
/* ************************************************************************** */
/* PNG chunks */

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_ihdr =
    {mng_it_png, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_ihdr,
     mng_fields_ihdr, (sizeof(mng_fields_ihdr) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL,
     MNG_NULL,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOJHDR | MNG_DESCR_NOBASI | MNG_DESCR_NOIDAT | MNG_DESCR_NOPLTE};

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_plte =
    {mng_it_png, mng_create_none, 0, offsetof(mng_plte, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_plte,
     mng_fields_plte, (sizeof(mng_fields_plte) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_idat =
    {mng_it_png, mng_create_none, 0, offsetof(mng_idat, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_idat,
     mng_fields_idat, (sizeof(mng_fields_idat) / sizeof(mng_field_descriptor)),
     MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOJSEP};

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_iend =
    {mng_it_png, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_iend,
     MNG_NULL, 0,
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_GenHDR,
     MNG_NULL};

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_trns =
    {mng_it_png, mng_create_none, 0, offsetof(mng_trns, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_trns,
     mng_fields_trns, (sizeof(mng_fields_trns) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};

#ifndef MNG_SKIPCHUNK_gAMA
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_gama =
    {mng_it_png, mng_create_none, 0, offsetof(mng_gama, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_gama,
     mng_fields_gama, (sizeof(mng_fields_gama) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOPLTE | MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_cHRM
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_chrm =
    {mng_it_png, mng_create_none, 0, offsetof(mng_chrm, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_chrm,
     mng_fields_chrm, (sizeof(mng_fields_chrm) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOPLTE | MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_sRGB
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_srgb =
    {mng_it_png, mng_create_none, 0, offsetof(mng_srgb, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_srgb,
     mng_fields_srgb, (sizeof(mng_fields_srgb) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOPLTE | MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_iCCP
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_iccp =
    {mng_it_png, mng_create_none, 0, offsetof(mng_iccp, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_iccp,
     mng_fields_iccp, (sizeof(mng_fields_iccp) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOPLTE | MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_tEXt
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_text =
    {mng_it_png, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_text,
     mng_fields_text, (sizeof(mng_fields_text) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL,
     MNG_DESCR_GenHDR,
     MNG_NULL};
#endif

#ifndef MNG_SKIPCHUNK_zTXt
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_ztxt =
    {mng_it_png, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_ztxt,
     mng_fields_ztxt, (sizeof(mng_fields_ztxt) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL,
     MNG_DESCR_GenHDR,
     MNG_NULL};
#endif

#ifndef MNG_SKIPCHUNK_iTXt
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_itxt =
    {mng_it_png, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_itxt,
     mng_fields_itxt, (sizeof(mng_fields_itxt) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL,
     MNG_DESCR_GenHDR,
     MNG_NULL};
#endif

#ifndef MNG_SKIPCHUNK_bKGD
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_bkgd =
    {mng_it_png, mng_create_none, 0, offsetof(mng_bkgd, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_bkgd,
     mng_fields_bkgd, (sizeof(mng_fields_bkgd) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_pHYs
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_phys =
    {mng_it_png, mng_create_none, 0, offsetof(mng_phys, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_phys,
     mng_fields_phys, (sizeof(mng_fields_phys) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_sBIT
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_sbit =
    {mng_it_png, mng_create_none, 0, offsetof(mng_sbit, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_sbit,
     mng_fields_sbit, (sizeof(mng_fields_sbit) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_sPLT
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_splt =
    {mng_it_png, mng_create_none, 0, offsetof(mng_splt, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_splt,
     mng_fields_splt, (sizeof(mng_fields_splt) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL | MNG_DESCR_EMPTYEMBED | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_GenHDR,
     MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_hIST
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_hist =
    {mng_it_png, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_hist,
     mng_fields_hist, (sizeof(mng_fields_hist) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_GenHDR | MNG_DESCR_PLTE,
     MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT | MNG_DESCR_NOJDAA};
#endif

#ifndef MNG_SKIPCHUNK_tIME
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_time =
    {mng_it_png, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_time,
     mng_fields_time, (sizeof(mng_fields_time) / sizeof(mng_field_descriptor)),
     MNG_DESCR_GLOBAL,
     MNG_DESCR_GenHDR,
     MNG_NULL};
#endif

/* ************************************************************************** */
/* JNG chunks */

#ifdef MNG_INCLUDE_JNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_jhdr =
    {mng_it_jng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_jhdr,
     mng_fields_jhdr, (sizeof(mng_fields_jhdr) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_NULL,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifdef MNG_INCLUDE_JNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_jdaa =
    {mng_it_jng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_jdaa,
     mng_fields_jdaa, (sizeof(mng_fields_jdaa) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_JngHDR,
     MNG_DESCR_NOJSEP};
#endif

#ifdef MNG_INCLUDE_JNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_jdat =
    {mng_it_jng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_jdat,
     mng_fields_jdat, (sizeof(mng_fields_jdat) / sizeof(mng_field_descriptor)),
     MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_JngHDR,
     MNG_NULL};
#endif

#ifdef MNG_INCLUDE_JNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_jsep =
    {mng_it_jng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_jsep,
     MNG_NULL, 0,
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_JngHDR,
     MNG_DESCR_NOJSEP};
#endif

/* ************************************************************************** */
/* MNG chunks */

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_mhdr =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_mhdr,
     mng_fields_mhdr, (sizeof(mng_fields_mhdr) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_NULL,
     MNG_DESCR_NOMHDR | MNG_DESCR_NOIHDR | MNG_DESCR_NOJHDR};

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_mend =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_mend,
     MNG_NULL, 0,
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_MHDR,
     MNG_NULL};

#ifndef MNG_SKIPCHUNK_LOOP
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_loop =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_loop,
     mng_fields_loop, (sizeof(mng_fields_loop) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_endl =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_endl,
     mng_fields_endl, (sizeof(mng_fields_endl) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_DEFI
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_defi =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_defi,
     mng_fields_defi, (sizeof(mng_fields_defi) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_BASI
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_basi =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_basi,
     mng_fields_basi, (sizeof(mng_fields_basi) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_CLON
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_clon =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_clon,
     mng_fields_clon, (sizeof(mng_fields_clon) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_PAST
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_past =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_past,
     mng_fields_past, (sizeof(mng_fields_past) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_DISC
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_disc =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_disc,
     mng_fields_disc, (sizeof(mng_fields_disc) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_BACK
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_back =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_back,
     mng_fields_back, (sizeof(mng_fields_back) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_FRAM
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_fram =
    {mng_it_mng, mng_create_none, 0, offsetof(mng_fram, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_fram,
     mng_fields_fram, (sizeof(mng_fields_fram) / sizeof(mng_field_descriptor)),
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_MOVE
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_move =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_move,
     mng_fields_move, (sizeof(mng_fields_move) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_CLIP
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_clip =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_clip,
     mng_fields_clip, (sizeof(mng_fields_clip) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_SHOW
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_show =
    {mng_it_mng, mng_create_none, 0, offsetof(mng_show, bEmpty),
     MNG_NULL, MNG_NULL, mng_special_show,
     mng_fields_show, (sizeof(mng_fields_show) / sizeof(mng_field_descriptor)),
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_TERM
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_term =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_term,
     mng_fields_term, (sizeof(mng_fields_term) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR | MNG_DESCR_NOTERM | MNG_DESCR_NOLOOP};
#endif

#ifndef MNG_SKIPCHUNK_SAVE
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_save =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_save,
     mng_fields_save, (sizeof(mng_fields_save) / sizeof(mng_field_descriptor)),
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOSAVE | MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_SEEK
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_seek =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_seek,
     mng_fields_seek, (sizeof(mng_fields_seek) / sizeof(mng_field_descriptor)),
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYGLOBAL,
     MNG_DESCR_MHDR | MNG_DESCR_SAVE,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_eXPI
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_expi =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_expi,
     mng_fields_expi, (sizeof(mng_fields_expi) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_fPRI
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_fpri =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_fpri,
     mng_fields_fpri, (sizeof(mng_fields_fpri) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_nEED
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_need =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_need,
     mng_fields_need, (sizeof(mng_fields_need) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_pHYg
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_phyg =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_phyg,
     mng_fields_phyg, (sizeof(mng_fields_phyg) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_dhdr =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_dhdr,
     mng_fields_dhdr, (sizeof(mng_fields_dhdr) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_prom =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_prom,
     mng_fields_prom, (sizeof(mng_fields_prom) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR | MNG_DESCR_DHDR,
     MNG_NULL};
#endif

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_ipng =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_ipng,
     MNG_NULL, 0,
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_MHDR | MNG_DESCR_DHDR,
     MNG_NULL};
#endif

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_pplt =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_pplt,
     mng_fields_pplt, (sizeof(mng_fields_pplt) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR | MNG_DESCR_DHDR,
     MNG_NULL};
#endif

#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_ijng =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_ijng,
     MNG_NULL, 0,
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_MHDR | MNG_DESCR_DHDR,
     MNG_NULL};
#endif
#endif

#ifndef MNG_NO_DELTA_PNG
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_drop =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_drop,
     mng_fields_drop, (sizeof(mng_fields_drop) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR | MNG_DESCR_DHDR,
     MNG_NULL};
#endif

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_dbyk =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_dbyk,
     mng_fields_dbyk, (sizeof(mng_fields_dbyk) / sizeof(mng_field_descriptor)),
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYEMBED,
     MNG_DESCR_MHDR | MNG_DESCR_DHDR,
     MNG_NULL};
#endif
#endif

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_ordr =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_ordr,
     mng_fields_ordr, (sizeof(mng_fields_ordr) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR | MNG_DESCR_DHDR,
     MNG_NULL};
#endif
#endif

#ifndef MNG_SKIPCHUNK_MAGN
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_magn =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_magn,
     mng_fields_magn, (sizeof(mng_fields_magn) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOIHDR | MNG_DESCR_NOBASI | MNG_DESCR_NODHDR | MNG_DESCR_NOJHDR};
#endif

#ifndef MNG_SKIPCHUNK_evNT
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_evnt =
    {mng_it_mng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_evnt,
     mng_fields_evnt, (sizeof(mng_fields_evnt) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_MHDR,
     MNG_DESCR_NOSAVE};
#endif

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_mpng =
    {mng_it_mpng, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_mpng,
     mng_fields_mpng, (sizeof(mng_fields_mpng) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_NULL,
     MNG_DESCR_NOMHDR | MNG_DESCR_NOIDAT | MNG_DESCR_NOJDAT};
#endif

#ifdef MNG_INCLUDE_ANG_PROPOSAL
MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_ahdr =
    {mng_it_ang, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_ahdr,
     mng_fields_ahdr, (sizeof(mng_fields_ahdr) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_IHDR,
     MNG_DESCR_NOMHDR | MNG_DESCR_NOJHDR | MNG_DESCR_NOIDAT};

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_adat =
    {mng_it_ang, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_adat,
     mng_fields_adat, (sizeof(mng_fields_adat) / sizeof(mng_field_descriptor)),
     MNG_NULL,
     MNG_DESCR_IHDR,
     MNG_DESCR_NOMHDR | MNG_DESCR_NOJHDR};
#endif

/* ************************************************************************** */
/* ************************************************************************** */
/* the good ol' unknown babe */

MNG_LOCAL mng_chunk_descriptor mng_chunk_descr_unknown =
    {mng_it_png, mng_create_none, 0, 0,
     MNG_NULL, MNG_NULL, mng_special_unknown,
     mng_fields_unknown, (sizeof(mng_fields_unknown) / sizeof(mng_field_descriptor)),
     MNG_DESCR_EMPTY | MNG_DESCR_EMPTYEMBED,
     MNG_NULL,
     MNG_NULL};

/* ************************************************************************** */
/* ************************************************************************** */

MNG_LOCAL mng_chunk_header mng_chunk_unknown =
    {MNG_UINT_HUH, mng_init_general, mng_free_unknown,
     mng_read_general, mng_write_unknown, mng_assign_unknown,
     0, 0, sizeof(mng_unknown_chunk), &mng_chunk_descr_unknown};

/* ************************************************************************** */

  /* the table-idea & binary search code was adapted from
     libpng 1.1.0 (pngread.c) */
  /* NOTE1: the table must remain sorted by chunkname, otherwise the binary
     search will break !!! (ps. watch upper-/lower-case chunknames !!) */
  /* NOTE2: the layout must remain equal to the header part of all the
     chunk-structures (yes, that means even the pNext and pPrev fields;
     it's wasting a bit of space, but hey, the code is a lot easier) */

MNG_LOCAL mng_chunk_header mng_chunk_table [] =
  {
#ifndef MNG_SKIPCHUNK_BACK
    {MNG_UINT_BACK, mng_init_general, mng_free_general, mng_read_general, mng_write_back, mng_assign_general, 0, 0, sizeof(mng_back), &mng_chunk_descr_back},
#endif
#ifndef MNG_SKIPCHUNK_BASI
    {MNG_UINT_BASI, mng_init_general, mng_free_general, mng_read_general, mng_write_basi, mng_assign_general, 0, 0, sizeof(mng_basi), &mng_chunk_descr_basi},
#endif
#ifndef MNG_SKIPCHUNK_CLIP
    {MNG_UINT_CLIP, mng_init_general, mng_free_general, mng_read_general, mng_write_clip, mng_assign_general, 0, 0, sizeof(mng_clip), &mng_chunk_descr_clip},
#endif
#ifndef MNG_SKIPCHUNK_CLON
    {MNG_UINT_CLON, mng_init_general, mng_free_general, mng_read_general, mng_write_clon, mng_assign_general, 0, 0, sizeof(mng_clon), &mng_chunk_descr_clon},
#endif
#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
    {MNG_UINT_DBYK, mng_init_general, mng_free_dbyk,    mng_read_general, mng_write_dbyk, mng_assign_dbyk,    0, 0, sizeof(mng_dbyk), &mng_chunk_descr_dbyk},
#endif
#endif
#ifndef MNG_SKIPCHUNK_DEFI
    {MNG_UINT_DEFI, mng_init_general, mng_free_general, mng_read_general, mng_write_defi, mng_assign_general, 0, 0, sizeof(mng_defi), &mng_chunk_descr_defi},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_UINT_DHDR, mng_init_general, mng_free_general, mng_read_general, mng_write_dhdr, mng_assign_general, 0, 0, sizeof(mng_dhdr), &mng_chunk_descr_dhdr},
#endif
#ifndef MNG_SKIPCHUNK_DISC
    {MNG_UINT_DISC, mng_init_general, mng_free_disc,    mng_read_general, mng_write_disc, mng_assign_disc,    0, 0, sizeof(mng_disc), &mng_chunk_descr_disc},
#endif
#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DROP
    {MNG_UINT_DROP, mng_init_general, mng_free_drop,    mng_read_general, mng_write_drop, mng_assign_drop,    0, 0, sizeof(mng_drop), &mng_chunk_descr_drop},
#endif
#endif
#ifndef MNG_SKIPCHUNK_LOOP
    {MNG_UINT_ENDL, mng_init_general, mng_free_general, mng_read_general, mng_write_endl, mng_assign_general, 0, 0, sizeof(mng_endl), &mng_chunk_descr_endl},
#endif
#ifndef MNG_SKIPCHUNK_FRAM
    {MNG_UINT_FRAM, mng_init_general, mng_free_fram,    mng_read_general, mng_write_fram, mng_assign_fram,    0, 0, sizeof(mng_fram), &mng_chunk_descr_fram},
#endif
    {MNG_UINT_IDAT, mng_init_general, mng_free_idat,    mng_read_general, mng_write_idat, mng_assign_idat,    0, 0, sizeof(mng_idat), &mng_chunk_descr_idat},  /* 12-th element! */
    {MNG_UINT_IEND, mng_init_general, mng_free_general, mng_read_general, mng_write_iend, mng_assign_general, 0, 0, sizeof(mng_iend), &mng_chunk_descr_iend},
    {MNG_UINT_IHDR, mng_init_general, mng_free_general, mng_read_general, mng_write_ihdr, mng_assign_general, 0, 0, sizeof(mng_ihdr), &mng_chunk_descr_ihdr},
#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
    {MNG_UINT_IJNG, mng_init_general, mng_free_general, mng_read_general, mng_write_ijng, mng_assign_general, 0, 0, sizeof(mng_ijng), &mng_chunk_descr_ijng},
#endif
    {MNG_UINT_IPNG, mng_init_general, mng_free_general, mng_read_general, mng_write_ipng, mng_assign_general, 0, 0, sizeof(mng_ipng), &mng_chunk_descr_ipng},
#endif
#ifdef MNG_INCLUDE_JNG
    {MNG_UINT_JDAA, mng_init_general, mng_free_jdaa,    mng_read_general, mng_write_jdaa, mng_assign_jdaa,    0, 0, sizeof(mng_jdaa), &mng_chunk_descr_jdaa},
    {MNG_UINT_JDAT, mng_init_general, mng_free_jdat,    mng_read_general, mng_write_jdat, mng_assign_jdat,    0, 0, sizeof(mng_jdat), &mng_chunk_descr_jdat},
    {MNG_UINT_JHDR, mng_init_general, mng_free_general, mng_read_general, mng_write_jhdr, mng_assign_general, 0, 0, sizeof(mng_jhdr), &mng_chunk_descr_jhdr},
    {MNG_UINT_JSEP, mng_init_general, mng_free_general, mng_read_general, mng_write_jsep, mng_assign_general, 0, 0, sizeof(mng_jsep), &mng_chunk_descr_jsep},
    {MNG_UINT_JdAA, mng_init_general, mng_free_jdaa,    mng_read_general, mng_write_jdaa, mng_assign_jdaa,    0, 0, sizeof(mng_jdaa), &mng_chunk_descr_jdaa},
#endif
#ifndef MNG_SKIPCHUNK_LOOP
    {MNG_UINT_LOOP, mng_init_general, mng_free_loop,    mng_read_general, mng_write_loop, mng_assign_loop,    0, 0, sizeof(mng_loop), &mng_chunk_descr_loop},
#endif
#ifndef MNG_SKIPCHUNK_MAGN
    {MNG_UINT_MAGN, mng_init_general, mng_free_general, mng_read_general, mng_write_magn, mng_assign_general, 0, 0, sizeof(mng_magn), &mng_chunk_descr_magn},
#endif
    {MNG_UINT_MEND, mng_init_general, mng_free_general, mng_read_general, mng_write_mend, mng_assign_general, 0, 0, sizeof(mng_mend), &mng_chunk_descr_mend},
    {MNG_UINT_MHDR, mng_init_general, mng_free_general, mng_read_general, mng_write_mhdr, mng_assign_general, 0, 0, sizeof(mng_mhdr), &mng_chunk_descr_mhdr},
#ifndef MNG_SKIPCHUNK_MOVE
    {MNG_UINT_MOVE, mng_init_general, mng_free_general, mng_read_general, mng_write_move, mng_assign_general, 0, 0, sizeof(mng_move), &mng_chunk_descr_move},
#endif
#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
    {MNG_UINT_ORDR, mng_init_general, mng_free_ordr,    mng_read_general, mng_write_ordr, mng_assign_ordr,    0, 0, sizeof(mng_ordr), &mng_chunk_descr_ordr},
#endif
#endif
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_UINT_PAST, mng_init_general, mng_free_past,    mng_read_general, mng_write_past, mng_assign_past,    0, 0, sizeof(mng_past), &mng_chunk_descr_past},
#endif
    {MNG_UINT_PLTE, mng_init_general, mng_free_general, mng_read_general, mng_write_plte, mng_assign_general, 0, 0, sizeof(mng_plte), &mng_chunk_descr_plte},
#ifndef MNG_NO_DELTA_PNG
    {MNG_UINT_PPLT, mng_init_general, mng_free_general, mng_read_general, mng_write_pplt, mng_assign_general, 0, 0, sizeof(mng_pplt), &mng_chunk_descr_pplt},
    {MNG_UINT_PROM, mng_init_general, mng_free_general, mng_read_general, mng_write_prom, mng_assign_general, 0, 0, sizeof(mng_prom), &mng_chunk_descr_prom},
#endif
#ifndef MNG_SKIPCHUNK_SAVE
    {MNG_UINT_SAVE, mng_init_general, mng_free_save,    mng_read_general, mng_write_save, mng_assign_save,    0, 0, sizeof(mng_save), &mng_chunk_descr_save},
#endif
#ifndef MNG_SKIPCHUNK_SEEK
    {MNG_UINT_SEEK, mng_init_general, mng_free_seek,    mng_read_general, mng_write_seek, mng_assign_seek,    0, 0, sizeof(mng_seek), &mng_chunk_descr_seek},
#endif
#ifndef MNG_SKIPCHUNK_SHOW
    {MNG_UINT_SHOW, mng_init_general, mng_free_general, mng_read_general, mng_write_show, mng_assign_general, 0, 0, sizeof(mng_show), &mng_chunk_descr_show},
#endif
#ifndef MNG_SKIPCHUNK_TERM
    {MNG_UINT_TERM, mng_init_general, mng_free_general, mng_read_general, mng_write_term, mng_assign_general, 0, 0, sizeof(mng_term), &mng_chunk_descr_term},
#endif
#ifdef MNG_INCLUDE_ANG_PROPOSAL
    {MNG_UINT_adAT, mng_init_general, mng_free_adat,    mng_read_general, mng_write_adat, mng_assign_adat,    0, 0, sizeof(mng_adat), &mng_chunk_descr_adat},
    {MNG_UINT_ahDR, mng_init_general, mng_free_general, mng_read_general, mng_write_ahdr, mng_assign_ahdr,    0, 0, sizeof(mng_ahdr), &mng_chunk_descr_ahdr},
#endif
#ifndef MNG_SKIPCHUNK_bKGD
    {MNG_UINT_bKGD, mng_init_general, mng_free_general, mng_read_general, mng_write_bkgd, mng_assign_general, 0, 0, sizeof(mng_bkgd), &mng_chunk_descr_bkgd},
#endif
#ifndef MNG_SKIPCHUNK_cHRM
    {MNG_UINT_cHRM, mng_init_general, mng_free_general, mng_read_general, mng_write_chrm, mng_assign_general, 0, 0, sizeof(mng_chrm), &mng_chunk_descr_chrm},
#endif
#ifndef MNG_SKIPCHUNK_eXPI
    {MNG_UINT_eXPI, mng_init_general, mng_free_expi,    mng_read_general, mng_write_expi, mng_assign_expi,    0, 0, sizeof(mng_expi), &mng_chunk_descr_expi},
#endif
#ifndef MNG_SKIPCHUNK_evNT
    {MNG_UINT_evNT, mng_init_general, mng_free_evnt,    mng_read_general, mng_write_evnt, mng_assign_evnt,    0, 0, sizeof(mng_evnt), &mng_chunk_descr_evnt},
#endif
#ifndef MNG_SKIPCHUNK_fPRI
    {MNG_UINT_fPRI, mng_init_general, mng_free_general, mng_read_general, mng_write_fpri, mng_assign_general, 0, 0, sizeof(mng_fpri), &mng_chunk_descr_fpri},
#endif
#ifndef MNG_SKIPCHUNK_gAMA
    {MNG_UINT_gAMA, mng_init_general, mng_free_general, mng_read_general, mng_write_gama, mng_assign_general, 0, 0, sizeof(mng_gama), &mng_chunk_descr_gama},
#endif
#ifndef MNG_SKIPCHUNK_hIST
    {MNG_UINT_hIST, mng_init_general, mng_free_general, mng_read_general, mng_write_hist, mng_assign_general, 0, 0, sizeof(mng_hist), &mng_chunk_descr_hist},
#endif
#ifndef MNG_SKIPCHUNK_iCCP
    {MNG_UINT_iCCP, mng_init_general, mng_free_iccp,    mng_read_general, mng_write_iccp, mng_assign_iccp,    0, 0, sizeof(mng_iccp), &mng_chunk_descr_iccp},
#endif
#ifndef MNG_SKIPCHUNK_iTXt
    {MNG_UINT_iTXt, mng_init_general, mng_free_itxt,    mng_read_general, mng_write_itxt, mng_assign_itxt,    0, 0, sizeof(mng_itxt), &mng_chunk_descr_itxt},
#endif
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_UINT_mpNG, mng_init_general, mng_free_mpng,    mng_read_general, mng_write_mpng, mng_assign_mpng,    0, 0, sizeof(mng_mpng), &mng_chunk_descr_mpng},
#endif
#ifndef MNG_SKIPCHUNK_nEED
    {MNG_UINT_nEED, mng_init_general, mng_free_need,    mng_read_general, mng_write_need, mng_assign_need,    0, 0, sizeof(mng_need), &mng_chunk_descr_need},
#endif
/* TODO:     {MNG_UINT_oFFs, 0, 0, 0, 0, 0, 0},  */
/* TODO:     {MNG_UINT_pCAL, 0, 0, 0, 0, 0, 0},  */
#ifndef MNG_SKIPCHUNK_pHYg
    {MNG_UINT_pHYg, mng_init_general, mng_free_general, mng_read_general, mng_write_phyg, mng_assign_general, 0, 0, sizeof(mng_phyg), &mng_chunk_descr_phyg},
#endif
#ifndef MNG_SKIPCHUNK_pHYs
    {MNG_UINT_pHYs, mng_init_general, mng_free_general, mng_read_general, mng_write_phys, mng_assign_general, 0, 0, sizeof(mng_phys), &mng_chunk_descr_phys},
#endif
#ifndef MNG_SKIPCHUNK_sBIT
    {MNG_UINT_sBIT, mng_init_general, mng_free_general, mng_read_general, mng_write_sbit, mng_assign_general, 0, 0, sizeof(mng_sbit), &mng_chunk_descr_sbit},
#endif
/* TODO:     {MNG_UINT_sCAL, 0, 0, 0, 0, 0, 0},  */
#ifndef MNG_SKIPCHUNK_sPLT
    {MNG_UINT_sPLT, mng_init_general, mng_free_splt,    mng_read_general, mng_write_splt, mng_assign_splt,    0, 0, sizeof(mng_splt), &mng_chunk_descr_splt},
#endif
    {MNG_UINT_sRGB, mng_init_general, mng_free_general, mng_read_general, mng_write_srgb, mng_assign_general, 0, 0, sizeof(mng_srgb), &mng_chunk_descr_srgb},
#ifndef MNG_SKIPCHUNK_tEXt
    {MNG_UINT_tEXt, mng_init_general, mng_free_text,    mng_read_general, mng_write_text, mng_assign_text,    0, 0, sizeof(mng_text), &mng_chunk_descr_text},
#endif
#ifndef MNG_SKIPCHUNK_tIME
    {MNG_UINT_tIME, mng_init_general, mng_free_general, mng_read_general, mng_write_time, mng_assign_general, 0, 0, sizeof(mng_time), &mng_chunk_descr_time},
#endif
    {MNG_UINT_tRNS, mng_init_general, mng_free_general, mng_read_general, mng_write_trns, mng_assign_general, 0, 0, sizeof(mng_trns), &mng_chunk_descr_trns},
#ifndef MNG_SKIPCHUNK_zTXt
    {MNG_UINT_zTXt, mng_init_general, mng_free_ztxt,    mng_read_general, mng_write_ztxt, mng_assign_ztxt,    0, 0, sizeof(mng_ztxt), &mng_chunk_descr_ztxt},
#endif
  };

/* ************************************************************************** */
/* ************************************************************************** */

void mng_get_chunkheader (mng_chunkid       iChunkname,
                          mng_chunk_headerp pResult)
{
                                       /* binary search variables */
  mng_int32         iTop, iLower, iUpper, iMiddle;
  mng_chunk_headerp pEntry;            /* pointer to found entry */
                                       /* determine max index of table */
  iTop = (sizeof (mng_chunk_table) / sizeof (mng_chunk_table [0])) - 1;

  /* binary search; with 54 chunks, worst-case is 7 comparisons */
  iLower  = 0;
#ifndef MNG_NO_DELTA_PNG
  iMiddle = 11;                        /* start with the IDAT entry */
#else
  iMiddle = 8;
#endif
  iUpper  = iTop;
  pEntry  = 0;                         /* no goods yet! */

  do                                   /* the binary search itself */
    {
      if (mng_chunk_table [iMiddle].iChunkname < iChunkname)
        iLower = iMiddle + 1;
      else if (mng_chunk_table [iMiddle].iChunkname > iChunkname)
        iUpper = iMiddle - 1;
      else
      {
        pEntry = &mng_chunk_table [iMiddle];
        break;
      }
      iMiddle = (iLower + iUpper) >> 1;
    }
  while (iLower <= iUpper);

  if (!pEntry)                         /* unknown chunk ? */
    pEntry = &mng_chunk_unknown;       /* make it so! */

  MNG_COPY (pResult, pEntry, sizeof(mng_chunk_header));

  return;
}

/* ************************************************************************** */
/* ************************************************************************** */
/* PNG chunks */

MNG_C_SPECIALFUNC (mng_special_ihdr)
{
  pData->bHasIHDR      = MNG_TRUE;     /* indicate IHDR is present */
                                       /* and store interesting fields */
  if ((!pData->bHasDHDR) || (pData->iDeltatype == MNG_DELTATYPE_NOCHANGE))
  {
    pData->iDatawidth  = ((mng_ihdrp)pChunk)->iWidth;
    pData->iDataheight = ((mng_ihdrp)pChunk)->iHeight;
  }

  pData->iBitdepth     = ((mng_ihdrp)pChunk)->iBitdepth;
  pData->iColortype    = ((mng_ihdrp)pChunk)->iColortype;
  pData->iCompression  = ((mng_ihdrp)pChunk)->iCompression;
  pData->iFilter       = ((mng_ihdrp)pChunk)->iFilter;
  pData->iInterlace    = ((mng_ihdrp)pChunk)->iInterlace;

#if defined(MNG_NO_1_2_4BIT_SUPPORT) || defined(MNG_NO_16BIT_SUPPORT)
  pData->iPNGmult = 1;
  pData->iPNGdepth = pData->iBitdepth;
#endif

#ifdef MNG_NO_1_2_4BIT_SUPPORT
  if (pData->iBitdepth < 8)
      pData->iBitdepth = 8;
#endif

#ifdef MNG_NO_16BIT_SUPPORT
  if (pData->iBitdepth > 8)
    {
      pData->iBitdepth = 8;
      pData->iPNGmult = 2;
    }
#endif

  if ((pData->iBitdepth !=  8)      /* parameter validity checks */
#ifndef MNG_NO_1_2_4BIT_SUPPORT
      && (pData->iBitdepth !=  1) &&
      (pData->iBitdepth !=  2) &&
      (pData->iBitdepth !=  4)
#endif
#ifndef MNG_NO_16BIT_SUPPORT
      && (pData->iBitdepth != 16)   
#endif
      )
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

  if ((pData->iColortype != MNG_COLORTYPE_GRAY   ) &&
      (pData->iColortype != MNG_COLORTYPE_RGB    ) &&
      (pData->iColortype != MNG_COLORTYPE_INDEXED) &&
      (pData->iColortype != MNG_COLORTYPE_GRAYA  ) &&
      (pData->iColortype != MNG_COLORTYPE_RGBA   )    )
    MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);

  if ((pData->iColortype == MNG_COLORTYPE_INDEXED) && (pData->iBitdepth > 8))
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

  if (((pData->iColortype == MNG_COLORTYPE_RGB    ) ||
       (pData->iColortype == MNG_COLORTYPE_GRAYA  ) ||
       (pData->iColortype == MNG_COLORTYPE_RGBA   )    ) &&
      (pData->iBitdepth < 8                            )    )
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

  if (pData->iCompression != MNG_COMPRESSION_DEFLATE)
    MNG_ERROR (pData, MNG_INVALIDCOMPRESS);

#if defined(FILTER192) || defined(FILTER193)
  if ((pData->iFilter != MNG_FILTER_ADAPTIVE ) &&
#if defined(FILTER192) && defined(FILTER193)
      (pData->iFilter != MNG_FILTER_DIFFERING) &&
      (pData->iFilter != MNG_FILTER_NOFILTER )    )
#else
#ifdef FILTER192
      (pData->iFilter != MNG_FILTER_DIFFERING)    )
#else
      (pData->iFilter != MNG_FILTER_NOFILTER )    )
#endif
#endif
    MNG_ERROR (pData, MNG_INVALIDFILTER);
#else
  if (pData->iFilter)
    MNG_ERROR (pData, MNG_INVALIDFILTER);
#endif

  if ((pData->iInterlace != MNG_INTERLACE_NONE ) &&
      (pData->iInterlace != MNG_INTERLACE_ADAM7)    )
    MNG_ERROR (pData, MNG_INVALIDINTERLACE);

#ifdef MNG_SUPPORT_DISPLAY 
#ifndef MNG_NO_DELTA_PNG
  if (pData->bHasDHDR)                 /* check the colortype for delta-images ! */
  {
    mng_imagedatap pBuf = ((mng_imagep)pData->pObjzero)->pImgbuf;

    if (pData->iColortype != pBuf->iColortype)
    {
      if ( ( (pData->iColortype != MNG_COLORTYPE_INDEXED) ||
             (pBuf->iColortype  == MNG_COLORTYPE_GRAY   )    ) &&
           ( (pData->iColortype != MNG_COLORTYPE_GRAY   ) ||
             (pBuf->iColortype  == MNG_COLORTYPE_INDEXED)    )    )
        MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);
    }
  }
#endif
#endif

  if (!pData->bHasheader)              /* first chunk ? */
  {
    pData->bHasheader = MNG_TRUE;      /* we've got a header */
    pData->eImagetype = mng_it_png;    /* then this must be a PNG */
    pData->iWidth     = pData->iDatawidth;
    pData->iHeight    = pData->iDataheight;
                                       /* predict alpha-depth ! */
    if ((pData->iColortype == MNG_COLORTYPE_GRAYA  ) ||
        (pData->iColortype == MNG_COLORTYPE_RGBA   )    )
      pData->iAlphadepth = pData->iBitdepth;
    else
    if (pData->iColortype == MNG_COLORTYPE_INDEXED)
      pData->iAlphadepth = 8;          /* worst case scenario */
    else
      pData->iAlphadepth = 1;  /* Possible tRNS cheap binary transparency */
                                       /* fits on maximum canvas ? */
    if ((pData->iWidth > pData->iMaxwidth) || (pData->iHeight > pData->iMaxheight))
      MNG_WARNING (pData, MNG_IMAGETOOLARGE);

#if !defined(MNG_INCLUDE_MPNG_PROPOSAL) || !defined(MNG_SUPPORT_DISPLAY)
    if (pData->fProcessheader)         /* inform the app ? */
      if (!pData->fProcessheader (((mng_handle)pData), pData->iWidth, pData->iHeight))
        MNG_ERROR (pData, MNG_APPMISCERROR);
#endif
  }

  if (!pData->bHasDHDR)
    pData->iImagelevel++;              /* one level deeper */

#ifdef MNG_SUPPORT_DISPLAY
  return mng_process_display_ihdr (pData);
#else
  return MNG_NOERROR;                 
#endif /* MNG_SUPPORT_DISPLAY */
}

/* ************************************************************************** */

MNG_F_SPECIALFUNC (mng_debunk_plte)
{
  mng_pltep  pPLTE    = (mng_pltep)pChunk;
  mng_uint32 iRawlen  = *piRawlen;
  mng_uint8p pRawdata = *ppRawdata;
                                       /* length must be multiple of 3 */
  if (((iRawlen % 3) != 0) || (iRawlen > 768))
    MNG_ERROR (pData, MNG_INVALIDLENGTH);
                                       /* this is the exact length */
  pPLTE->iEntrycount = iRawlen / 3;

  MNG_COPY (pPLTE->aEntries, pRawdata, iRawlen);

  *piRawlen = 0;

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_C_SPECIALFUNC (mng_special_plte)
{                                      /* multiple PLTE only inside BASI */
  if ((pData->bHasPLTE) && (!pData->bHasBASI))
    MNG_ERROR (pData, MNG_MULTIPLEERROR);

  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
  {                                    /* only allowed for indexed-color or
                                          rgb(a)-color! */
    if ((pData->iColortype != MNG_COLORTYPE_RGB    ) &&
        (pData->iColortype != MNG_COLORTYPE_INDEXED) &&
        (pData->iColortype != MNG_COLORTYPE_RGBA   )   )
      MNG_ERROR (pData, MNG_CHUNKNOTALLOWED);
                                       /* empty only allowed if global present */
    if ((((mng_pltep)pChunk)->bEmpty) && (!pData->bHasglobalPLTE))
      MNG_ERROR (pData, MNG_CANNOTBEEMPTY);
  }
  else
  {
    if (((mng_pltep)pChunk)->bEmpty)   /* cannot be empty as global! */
      MNG_ERROR (pData, MNG_CANNOTBEEMPTY);
  }

  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
    pData->bHasPLTE = MNG_TRUE;        /* got it! */
  else
    pData->bHasglobalPLTE = MNG_TRUE;

  pData->iPLTEcount = ((mng_pltep)pChunk)->iEntrycount;

#ifdef MNG_SUPPORT_DISPLAY
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
  {
    mng_imagep     pImage;
    mng_imagedatap pBuf;

#ifndef MNG_NO_DELTA_PNG
    if (pData->bHasDHDR)               /* processing delta-image ? */
    {                                  /* store in object 0 !!! */
      pImage           = (mng_imagep)pData->pObjzero;
      pBuf             = pImage->pImgbuf;
      pBuf->bHasPLTE   = MNG_TRUE;     /* it's definitely got a PLTE now */
      pBuf->iPLTEcount = ((mng_pltep)pChunk)->iEntrycount;
      MNG_COPY (pBuf->aPLTEentries, ((mng_pltep)pChunk)->aEntries,
                sizeof (pBuf->aPLTEentries));
    }
    else
#endif
    {                                  /* get the current object */
      pImage = (mng_imagep)pData->pCurrentobj;
      if (!pImage)                     /* no object then dump it in obj 0 */
        pImage = (mng_imagep)pData->pObjzero;

      pBuf = pImage->pImgbuf;          /* address the object buffer */
      pBuf->bHasPLTE = MNG_TRUE;       /* and tell it it's got a PLTE now */

      if (((mng_pltep)pChunk)->bEmpty) /* if empty, inherit from global */
      {
        pBuf->iPLTEcount = pData->iGlobalPLTEcount;
        MNG_COPY (pBuf->aPLTEentries, pData->aGlobalPLTEentries,
                  sizeof (pBuf->aPLTEentries));

        if (pData->bHasglobalTRNS)     /* also copy global tRNS ? */
        {
          mng_uint32 iRawlen2  = pData->iGlobalTRNSrawlen;
          mng_uint8p pRawdata2 = (mng_uint8p)(pData->aGlobalTRNSrawdata);
                                       /* indicate tRNS available */
          pBuf->bHasTRNS = MNG_TRUE;
                                       /* global length oke ? */
          if ((iRawlen2 == 0) || (iRawlen2 > pBuf->iPLTEcount))
            MNG_ERROR (pData, MNG_GLOBALLENGTHERR);
                                       /* copy it */
          pBuf->iTRNScount = iRawlen2;
          MNG_COPY (pBuf->aTRNSentries, pRawdata2, iRawlen2);
        }
      }
      else
      {                                /* store fields for future reference */
        pBuf->iPLTEcount = ((mng_pltep)pChunk)->iEntrycount;
        MNG_COPY (pBuf->aPLTEentries, ((mng_pltep)pChunk)->aEntries,
                  sizeof (pBuf->aPLTEentries));
      }
    }
  }
  else                                 /* store as global */
  {
    pData->iGlobalPLTEcount = ((mng_pltep)pChunk)->iEntrycount;
    MNG_COPY (pData->aGlobalPLTEentries, ((mng_pltep)pChunk)->aEntries,
              sizeof (pData->aGlobalPLTEentries));
                                       /* create an animation object */
    return mng_create_ani_plte (pData);
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

MNG_C_SPECIALFUNC (mng_special_idat)
{
#ifdef MNG_INCLUDE_JNG
  if ((pData->bHasJHDR) &&
      (pData->iJHDRalphacompression != MNG_COMPRESSION_DEFLATE))
    MNG_ERROR (pData, MNG_SEQUENCEERROR);
#endif
                                       /* not allowed for deltatype NO_CHANGE */
#ifndef MNG_NO_DELTA_PNG
  if ((pData->bHasDHDR) && ((pData->iDeltatype == MNG_DELTATYPE_NOCHANGE)))
    MNG_ERROR (pData, MNG_CHUNKNOTALLOWED);
#endif
                                       /* can only be empty in BASI-block! */
  if ((((mng_idatp)pChunk)->bEmpty) && (!pData->bHasBASI))
    MNG_ERROR (pData, MNG_INVALIDLENGTH);
                                       /* indexed-color requires PLTE */
  if ((pData->bHasIHDR) && (pData->iColortype == 3) && (!pData->bHasPLTE))
    MNG_ERROR (pData, MNG_PLTEMISSING);

  pData->bHasIDAT = MNG_TRUE;          /* got some IDAT now, don't we */

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

MNG_C_SPECIALFUNC (mng_special_iend)
{                                      /* IHDR-block requires IDAT */
  if ((pData->bHasIHDR) && (!pData->bHasIDAT))
    MNG_ERROR (pData, MNG_IDATMISSING);

  pData->iImagelevel--;                /* one level up */

#ifdef MNG_SUPPORT_DISPLAY
  {                                    /* create an animation object */
    mng_retcode iRetcode = mng_create_ani_image (pData);
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
                                       /* display processing */
    iRetcode = mng_process_display_iend (pData);
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }
#endif /* MNG_SUPPORT_DISPLAY */

#ifdef MNG_SUPPORT_DISPLAY
  if (!pData->bTimerset)               /* reset only if not broken !!! */
  {
#endif
                                       /* IEND signals the end for most ... */
    pData->bHasIHDR         = MNG_FALSE;
    pData->bHasBASI         = MNG_FALSE;
    pData->bHasDHDR         = MNG_FALSE;
#ifdef MNG_INCLUDE_JNG
    pData->bHasJHDR         = MNG_FALSE;
    pData->bHasJSEP         = MNG_FALSE;
    pData->bHasJDAA         = MNG_FALSE;
    pData->bHasJDAT         = MNG_FALSE;
#endif
    pData->bHasPLTE         = MNG_FALSE;
    pData->bHasTRNS         = MNG_FALSE;
    pData->bHasGAMA         = MNG_FALSE;
    pData->bHasCHRM         = MNG_FALSE;
    pData->bHasSRGB         = MNG_FALSE;
    pData->bHasICCP         = MNG_FALSE;
    pData->bHasBKGD         = MNG_FALSE;
    pData->bHasIDAT         = MNG_FALSE;
#ifdef MNG_SUPPORT_DISPLAY
  }
#endif

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

MNG_F_SPECIALFUNC (mng_debunk_trns)
{
  mng_trnsp  pTRNS    = (mng_trnsp)pChunk;
  mng_uint32 iRawlen  = *piRawlen;
  mng_uint8p pRawdata = *ppRawdata;

  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
  {                                  /* not global! */
    pTRNS->bGlobal = MNG_FALSE;
    pTRNS->iType   = pData->iColortype;

    if (iRawlen != 0)
    {
      switch (pData->iColortype)     /* store fields */
      {
        case 0: {                    /* gray */
                  if (iRawlen != 2)
                    MNG_ERROR (pData, MNG_INVALIDLENGTH);
                  pTRNS->iGray  = mng_get_uint16 (pRawdata);
                  break;
                }
        case 2: {                    /* rgb */
                  if (iRawlen != 6)
                    MNG_ERROR (pData, MNG_INVALIDLENGTH);
                  pTRNS->iRed   = mng_get_uint16 (pRawdata);
                  pTRNS->iGreen = mng_get_uint16 (pRawdata+2);
                  pTRNS->iBlue  = mng_get_uint16 (pRawdata+4);
                  break;
                }
        case 3: {                    /* indexed */
                  if (iRawlen > 256)
                    MNG_ERROR (pData, MNG_INVALIDLENGTH);
                  pTRNS->iCount = iRawlen;
                  MNG_COPY (pTRNS->aEntries, pRawdata, iRawlen);
                  break;
                }
      }
    }
  }
  else                               /* it's global! */
  {
    if (iRawlen == 0)
      MNG_ERROR (pData, MNG_INVALIDLENGTH);
    pTRNS->bGlobal = MNG_TRUE;
    pTRNS->iType   = 0;
    pTRNS->iRawlen = iRawlen;
    MNG_COPY (pTRNS->aRawdata, pRawdata, iRawlen);

    pData->iGlobalTRNSrawlen = iRawlen;
    MNG_COPY (pData->aGlobalTRNSrawdata, pRawdata, iRawlen);
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_C_SPECIALFUNC (mng_special_trns)
{                                      /* multiple tRNS only inside BASI */
  if ((pData->bHasTRNS) && (!pData->bHasBASI))
    MNG_ERROR (pData, MNG_MULTIPLEERROR);

  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
  {                                    /* not allowed with full alpha-channel */
    if ((pData->iColortype == 4) || (pData->iColortype == 6))
      MNG_ERROR (pData, MNG_CHUNKNOTALLOWED);

    if (!((mng_trnsp)pChunk)->bEmpty)  /* filled ? */
    {                                
#ifdef MNG_SUPPORT_DISPLAY
      if (pData->iColortype == 3)
      {
        mng_imagep     pImage = (mng_imagep)pData->pCurrentobj;
        mng_imagedatap pBuf;

        if (!pImage)                   /* no object then check obj 0 */
          pImage = (mng_imagep)pData->pObjzero;

        pBuf = pImage->pImgbuf;        /* address object buffer */

        if (((mng_trnsp)pChunk)->iCount > pBuf->iPLTEcount)
          MNG_ERROR (pData, MNG_INVALIDLENGTH);
      }
#endif
    }
    else                               /* if empty there must be global stuff! */
    {
      if (!pData->bHasglobalTRNS)
        MNG_ERROR (pData, MNG_CANNOTBEEMPTY);
    }
  }

  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
    pData->bHasTRNS = MNG_TRUE;        /* indicate tRNS available */
  else
    pData->bHasglobalTRNS = MNG_TRUE;

#ifdef MNG_SUPPORT_DISPLAY
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
  {
    mng_imagep     pImage;
    mng_imagedatap pBuf;
    mng_uint8p     pRawdata2;
    mng_uint32     iRawlen2;

#ifndef MNG_NO_DELTA_PNG
    if (pData->bHasDHDR)               /* processing delta-image ? */
    {                                  /* store in object 0 !!! */
#if defined(MNG_NO_1_2_4BIT_SUPPORT)
      mng_uint8 multiplier[]={0,255,85,0,17,0,0,0,1,0,0,0,0,0,0,0,1};
#endif
      pImage = (mng_imagep)pData->pObjzero;
      pBuf   = pImage->pImgbuf;        /* address object buffer */
      pBuf->bHasTRNS   = MNG_TRUE;     /* tell it it's got a tRNS now */
      pBuf->iTRNSgray  = 0;
      pBuf->iTRNSred   = 0;
      pBuf->iTRNSgreen = 0;
      pBuf->iTRNSblue  = 0;
      pBuf->iTRNScount = 0;

      switch (pData->iColortype)       /* store fields for future reference */
      {
        case 0: {                      /* gray */
                  pBuf->iTRNSgray  = ((mng_trnsp)pChunk)->iGray;
#if defined(MNG_NO_1_2_4BIT_SUPPORT)
                  pBuf->iTRNSgray *= multiplier[pData->iPNGdepth];
#endif
#if defined(MNG_NO_16BIT_SUPPORT)
                  if (pData->iPNGmult == 2)
                     pBuf->iTRNSgray >>= 8;
#endif
                  break;
                }
        case 2: {                      /* rgb */
                  pBuf->iTRNSred   = ((mng_trnsp)pChunk)->iRed;
                  pBuf->iTRNSgreen = ((mng_trnsp)pChunk)->iGreen;
                  pBuf->iTRNSblue  = ((mng_trnsp)pChunk)->iBlue;
#if defined(MNG_NO_16BIT_SUPPORT)
                  if (pData->iPNGmult == 2)
                  {
                     pBuf->iTRNSred   >>= 8;
                     pBuf->iTRNSgreen >>= 8;
                     pBuf->iTRNSblue  >>= 8;
                  }
#endif
                  break;
                }
        case 3: {                      /* indexed */
                  pBuf->iTRNScount = ((mng_trnsp)pChunk)->iCount;
                  MNG_COPY (pBuf->aTRNSentries,
                            ((mng_trnsp)pChunk)->aEntries,
                            ((mng_trnsp)pChunk)->iCount);
                  break;
                }
      }
    }
    else
#endif
    {                                  /* address current object */
      pImage = (mng_imagep)pData->pCurrentobj;

      if (!pImage)                     /* no object then dump it in obj 0 */
        pImage = (mng_imagep)pData->pObjzero;

      pBuf = pImage->pImgbuf;          /* address object buffer */
      pBuf->bHasTRNS   = MNG_TRUE;     /* and tell it it's got a tRNS now */
      pBuf->iTRNSgray  = 0;
      pBuf->iTRNSred   = 0;
      pBuf->iTRNSgreen = 0;
      pBuf->iTRNSblue  = 0;
      pBuf->iTRNScount = 0;

      if (((mng_trnsp)pChunk)->bEmpty) /* if empty, inherit from global */
      {
        iRawlen2  = pData->iGlobalTRNSrawlen;
        pRawdata2 = (mng_ptr)(pData->aGlobalTRNSrawdata);
                                       /* global length oke ? */
        if ((pData->iColortype == 0) && (iRawlen2 != 2))
          MNG_ERROR (pData, MNG_GLOBALLENGTHERR);

        if ((pData->iColortype == 2) && (iRawlen2 != 6))
          MNG_ERROR (pData, MNG_GLOBALLENGTHERR);

        if ((pData->iColortype == 3) && ((iRawlen2 == 0) || (iRawlen2 > pBuf->iPLTEcount)))
          MNG_ERROR (pData, MNG_GLOBALLENGTHERR);

        switch (pData->iColortype)     /* store fields for future reference */
        {
          case 0: {                    /* gray */
                    pBuf->iTRNSgray = mng_get_uint16 (pRawdata2);
#if defined(MNG_NO_16BIT_SUPPORT)
                    if (pData->iPNGmult == 2)
                       pBuf->iTRNSgray >>= 8;
#endif
                    break;
                  }
          case 2: {                    /* rgb */
                    pBuf->iTRNSred   = mng_get_uint16 (pRawdata2);
                    pBuf->iTRNSgreen = mng_get_uint16 (pRawdata2+2);
                    pBuf->iTRNSblue  = mng_get_uint16 (pRawdata2+4);
#if defined(MNG_NO_16BIT_SUPPORT)
                    if (pData->iPNGmult == 2)
                    {
                       pBuf->iTRNSred   >>= 8;
                       pBuf->iTRNSgreen >>= 8;
                       pBuf->iTRNSblue  >>= 8;
                    }
#endif
                    break;
                  }
          case 3: {                    /* indexed */
                    pBuf->iTRNScount = iRawlen2;
                    MNG_COPY (pBuf->aTRNSentries, pRawdata2, iRawlen2);
                    break;
                  }
        }
      }
      else
      {
        switch (pData->iColortype)     /* store fields for future reference */
        {
          case 0: {                    /* gray */
                    pBuf->iTRNSgray = ((mng_trnsp)pChunk)->iGray;
#if defined(MNG_NO_16BIT_SUPPORT)
                    if (pData->iPNGmult == 2)
                       pBuf->iTRNSgray >>= 8;
#endif
                    break;
                  }
          case 2: {                    /* rgb */
                    pBuf->iTRNSred   = ((mng_trnsp)pChunk)->iRed;
                    pBuf->iTRNSgreen = ((mng_trnsp)pChunk)->iGreen;
                    pBuf->iTRNSblue  = ((mng_trnsp)pChunk)->iBlue;
#if defined(MNG_NO_16BIT_SUPPORT)
                    if (pData->iPNGmult == 2)
                    {
                       pBuf->iTRNSred   >>= 8;
                       pBuf->iTRNSgreen >>= 8;
                       pBuf->iTRNSblue  >>= 8;
                    }
#endif
                    break;
                  }
          case 3: {                    /* indexed */
                    pBuf->iTRNScount = ((mng_trnsp)pChunk)->iCount;
                    MNG_COPY (pBuf->aTRNSentries,
                              ((mng_trnsp)pChunk)->aEntries,
                              ((mng_trnsp)pChunk)->iCount);
                    break;
                  }
        }
      }
    }
  }
  else
  {                                    /* create an animation object */
    return mng_create_ani_trns (pData);
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

MNG_C_SPECIALFUNC (mng_special_gama)
{
#ifdef MNG_INCLUDE_JNG
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
    pData->bHasGAMA = MNG_TRUE;        /* indicate we've got it */
  else
    pData->bHasglobalGAMA = (mng_bool)!((mng_gamap)pChunk)->bEmpty;

#ifdef MNG_SUPPORT_DISPLAY
#ifdef MNG_INCLUDE_JNG
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
  {
    mng_imagep pImage;

#ifndef MNG_NO_DELTA_PNG
    if (pData->bHasDHDR)               /* update delta image ? */
      pImage = (mng_imagep)pData->pObjzero;
    else
#endif
    {
      pImage = (mng_imagep)pData->pCurrentobj;
      if (!pImage)                     /* no object then dump it in obj 0 */
        pImage = (mng_imagep)pData->pObjzero;
    }
                                       /* store for color-processing routines */
    pImage->pImgbuf->iGamma   = ((mng_gamap)pChunk)->iGamma;
    pImage->pImgbuf->bHasGAMA = MNG_TRUE;
  }
  else
  {                                    /* store as global */
    if (!((mng_gamap)pChunk)->bEmpty)
      pData->iGlobalGamma = ((mng_gamap)pChunk)->iGamma;
                                       /* create an animation object */
    return mng_create_ani_gama (pData, pChunk);
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_cHRM
MNG_C_SPECIALFUNC (mng_special_chrm)
{
#ifdef MNG_INCLUDE_JNG
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
    pData->bHasCHRM = MNG_TRUE;        /* indicate we've got it */
  else
    pData->bHasglobalCHRM = (mng_bool)!((mng_chrmp)pChunk)->bEmpty;

#ifdef MNG_SUPPORT_DISPLAY
  {
#ifdef MNG_INCLUDE_JNG
    if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
    if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
    {
      mng_imagep     pImage;
      mng_imagedatap pBuf;

#ifndef MNG_NO_DELTA_PNG
      if (pData->bHasDHDR)             /* update delta image ? */
        pImage = (mng_imagep)pData->pObjzero;
      else
#endif
      {
        pImage = (mng_imagep)pData->pCurrentobj;
        if (!pImage)                   /* no object then dump it in obj 0 */
          pImage = (mng_imagep)pData->pObjzero;
      }

      pBuf = pImage->pImgbuf;          /* address object buffer */
      pBuf->bHasCHRM = MNG_TRUE;       /* and tell it it's got a CHRM now */
                                       /* store for color-processing routines */
      pBuf->iWhitepointx   = ((mng_chrmp)pChunk)->iWhitepointx;
      pBuf->iWhitepointy   = ((mng_chrmp)pChunk)->iWhitepointy;
      pBuf->iPrimaryredx   = ((mng_chrmp)pChunk)->iRedx;
      pBuf->iPrimaryredy   = ((mng_chrmp)pChunk)->iRedy;
      pBuf->iPrimarygreenx = ((mng_chrmp)pChunk)->iGreenx;
      pBuf->iPrimarygreeny = ((mng_chrmp)pChunk)->iGreeny;
      pBuf->iPrimarybluex  = ((mng_chrmp)pChunk)->iBluex;
      pBuf->iPrimarybluey  = ((mng_chrmp)pChunk)->iBluey;
    }
    else
    {                                  /* store as global */
      if (!((mng_chrmp)pChunk)->bEmpty)
      {
        pData->iGlobalWhitepointx   = ((mng_chrmp)pChunk)->iWhitepointx;
        pData->iGlobalWhitepointy   = ((mng_chrmp)pChunk)->iWhitepointy;
        pData->iGlobalPrimaryredx   = ((mng_chrmp)pChunk)->iRedx;
        pData->iGlobalPrimaryredy   = ((mng_chrmp)pChunk)->iRedy;
        pData->iGlobalPrimarygreenx = ((mng_chrmp)pChunk)->iGreenx;
        pData->iGlobalPrimarygreeny = ((mng_chrmp)pChunk)->iGreeny;
        pData->iGlobalPrimarybluex  = ((mng_chrmp)pChunk)->iBluex;
        pData->iGlobalPrimarybluey  = ((mng_chrmp)pChunk)->iBluey;
      }
                                       /* create an animation object */
      return mng_create_ani_chrm (pData, pChunk);
    }
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

MNG_C_SPECIALFUNC (mng_special_srgb)
{
#ifdef MNG_INCLUDE_JNG
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
    pData->bHasSRGB = MNG_TRUE;        /* indicate we've got it */
  else
    pData->bHasglobalSRGB = (mng_bool)!((mng_srgbp)pChunk)->bEmpty;

#ifdef MNG_SUPPORT_DISPLAY
#ifdef MNG_INCLUDE_JNG
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
  {
    mng_imagep pImage;

#ifndef MNG_NO_DELTA_PNG
    if (pData->bHasDHDR)               /* update delta image ? */
      pImage = (mng_imagep)pData->pObjzero;
    else
#endif
    {
      pImage = (mng_imagep)pData->pCurrentobj;
      if (!pImage)                     /* no object then dump it in obj 0 */
        pImage = (mng_imagep)pData->pObjzero;
    }
                                       /* store for color-processing routines */
    pImage->pImgbuf->iRenderingintent = ((mng_srgbp)pChunk)->iRenderingintent;
    pImage->pImgbuf->bHasSRGB         = MNG_TRUE;
  }
  else
  {                                    /* store as global */
    if (!((mng_srgbp)pChunk)->bEmpty)
      pData->iGlobalRendintent = ((mng_srgbp)pChunk)->iRenderingintent;
                                       /* create an animation object */
    return mng_create_ani_srgb (pData, pChunk);
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
MNG_C_SPECIALFUNC (mng_special_iccp)
{
  mng_retcode       iRetcode;
  mng_chunk_headerp pDummy;

#ifdef MNG_CHECK_BAD_ICCP              /* Check for bad iCCP chunk */
  if (!strncmp (((mng_iccpp)pChunk)->zName, "Photoshop ICC profile", 21))
  {
    if (((mng_iccpp)pChunk)->iProfilesize == 2615) /* is it the sRGB profile ? */
    {
      mng_chunk_header chunk_srgb;
      mng_get_chunkheader (MNG_UINT_sRGB, &chunk_srgb);
                                       /* pretend it's an sRGB chunk then ! */
      iRetcode = mng_read_general (pData, &chunk_srgb, 1, (mng_ptr)"0", &pDummy);
      if (iRetcode)                    /* on error bail out */
        return iRetcode;

      pDummy->fCleanup (pData, pDummy);  
    }
  }
  else
  {
#endif /* MNG_CHECK_BAD_ICCP */

#ifdef MNG_INCLUDE_JNG
    if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
    if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
      pData->bHasICCP = MNG_TRUE;      /* indicate we've got it */
    else
      pData->bHasglobalICCP = (mng_bool)!((mng_iccpp)pChunk)->bEmpty;

#ifdef MNG_SUPPORT_DISPLAY
#ifdef MNG_INCLUDE_JNG
    if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
    if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
    {
      mng_imagep pImage;

#ifndef MNG_NO_DELTA_PNG
      if (pData->bHasDHDR)             /* update delta image ? */
      {                                /* store in object 0 ! */
        pImage = (mng_imagep)pData->pObjzero;

        if (pImage->pImgbuf->pProfile) /* profile existed ? */
          MNG_FREEX (pData, pImage->pImgbuf->pProfile, pImage->pImgbuf->iProfilesize);
                                       /* allocate a buffer & copy it */
        MNG_ALLOC (pData, pImage->pImgbuf->pProfile, ((mng_iccpp)pChunk)->iProfilesize);
        MNG_COPY  (pImage->pImgbuf->pProfile, ((mng_iccpp)pChunk)->pProfile, ((mng_iccpp)pChunk)->iProfilesize);
                                       /* store its length as well */
        pImage->pImgbuf->iProfilesize = ((mng_iccpp)pChunk)->iProfilesize;
        pImage->pImgbuf->bHasICCP     = MNG_TRUE;
      }
      else
#endif
      {
        pImage = (mng_imagep)pData->pCurrentobj;

        if (!pImage)                   /* no object then dump it in obj 0 */
          pImage = (mng_imagep)pData->pObjzero;

        if (pImage->pImgbuf->pProfile) /* profile existed ? */
          MNG_FREEX (pData, pImage->pImgbuf->pProfile, pImage->pImgbuf->iProfilesize);
                                       /* allocate a buffer & copy it */
        MNG_ALLOC (pData, pImage->pImgbuf->pProfile, ((mng_iccpp)pChunk)->iProfilesize);
        MNG_COPY  (pImage->pImgbuf->pProfile, ((mng_iccpp)pChunk)->pProfile, ((mng_iccpp)pChunk)->iProfilesize);
                                       /* store its length as well */
        pImage->pImgbuf->iProfilesize = ((mng_iccpp)pChunk)->iProfilesize;
        pImage->pImgbuf->bHasICCP     = MNG_TRUE;
      }
    }
    else
    {                                  /* store as global */
      if (pData->pGlobalProfile)     /* did we have a global profile ? */
        MNG_FREEX (pData, pData->pGlobalProfile, pData->iGlobalProfilesize);

      if (((mng_iccpp)pChunk)->bEmpty) /* empty chunk ? */
      {
        pData->iGlobalProfilesize = 0; /* reset to null */
        pData->pGlobalProfile     = MNG_NULL;
      }
      else
      {                                /* allocate a global buffer & copy it */
        MNG_ALLOC (pData, pData->pGlobalProfile, ((mng_iccpp)pChunk)->iProfilesize);
        MNG_COPY  (pData->pGlobalProfile, ((mng_iccpp)pChunk)->pProfile, ((mng_iccpp)pChunk)->iProfilesize);
                                       /* store its length as well */
        pData->iGlobalProfilesize = ((mng_iccpp)pChunk)->iProfilesize;
      }
                                       /* create an animation object */
      return mng_create_ani_iccp (pData, pChunk);
    }
#endif /* MNG_SUPPORT_DISPLAY */

#ifdef MNG_CHECK_BAD_ICCP
  }
#endif

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tEXt
MNG_C_SPECIALFUNC (mng_special_text)
{
  if (pData->fProcesstext)             /* inform the application ? */
  {
    mng_bool bOke = pData->fProcesstext ((mng_handle)pData, MNG_TYPE_TEXT,
                                         ((mng_textp)pChunk)->zKeyword,
                                         ((mng_textp)pChunk)->zText, 0, 0);
    if (!bOke)
      MNG_ERROR (pData, MNG_APPMISCERROR);
  }

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_zTXt
MNG_C_SPECIALFUNC (mng_special_ztxt)
{
  if (pData->fProcesstext)             /* inform the application ? */
  {
    mng_bool bOke = pData->fProcesstext ((mng_handle)pData, MNG_TYPE_ZTXT,
                                         ((mng_ztxtp)pChunk)->zKeyword,
                                         ((mng_ztxtp)pChunk)->zText, 0, 0);
    if (!bOke)
      MNG_ERROR (pData, MNG_APPMISCERROR);
  }

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iTXt
MNG_F_SPECIALFUNC (mng_deflate_itxt)
{
  mng_itxtp  pITXT    = (mng_itxtp)pChunk;
  mng_uint32 iBufsize = 0;
  mng_uint8p pBuf     = 0;
  mng_uint32 iTextlen = 0;

  if (pITXT->iCompressionflag)         /* decompress the text ? */
  {
    mng_retcode iRetcode = mng_inflate_buffer (pData, *ppRawdata, *piRawlen,
                                               &pBuf, &iBufsize, &iTextlen);

    if (iRetcode)                      /* on error bail out */
    {                                  /* don't forget to drop the temp buffer */
      MNG_FREEX (pData, pBuf, iBufsize);
      return iRetcode;
    }

    MNG_ALLOC (pData, pITXT->zText, iTextlen+1);
    MNG_COPY (pITXT->zText, pBuf, iTextlen);

    pITXT->iTextsize = iTextlen;

    MNG_FREEX (pData, pBuf, iBufsize);

  } else {

    MNG_ALLOC (pData, pITXT->zText, (*piRawlen)+1);
    MNG_COPY (pITXT->zText, *ppRawdata, *piRawlen);

    pITXT->iTextsize = *piRawlen;
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iTXt
MNG_C_SPECIALFUNC (mng_special_itxt)
{
  if (pData->fProcesstext)             /* inform the application ? */
  {
    mng_bool bOke = pData->fProcesstext ((mng_handle)pData, MNG_TYPE_ITXT,
                                         ((mng_itxtp)pChunk)->zKeyword,
                                         ((mng_itxtp)pChunk)->zText,
                                         ((mng_itxtp)pChunk)->zLanguage,
                                         ((mng_itxtp)pChunk)->zTranslation);
    if (!bOke)
      MNG_ERROR (pData, MNG_APPMISCERROR);
  }

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_bKGD
MNG_C_SPECIALFUNC (mng_special_bkgd)
{
#ifdef MNG_SUPPORT_DISPLAY
  mng_imagep     pImage = (mng_imagep)pData->pCurrentobj;
  mng_imagedatap pBuf;
#endif

#ifdef MNG_INCLUDE_JNG
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR) || (pData->bHasJHDR))
#else
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
#endif
    pData->bHasBKGD = MNG_TRUE;        /* indicate bKGD available */
  else
    pData->bHasglobalBKGD = (mng_bool)!(((mng_bkgdp)pChunk)->bEmpty);

#ifdef MNG_SUPPORT_DISPLAY
  if (!pImage)                         /* if no object dump it in obj 0 */
    pImage = (mng_imagep)pData->pObjzero;
  pBuf = pImage->pImgbuf;              /* address object buffer */

#ifdef MNG_INCLUDE_JNG
  if (pData->bHasJHDR)
  {
    pBuf->bHasBKGD = MNG_TRUE;         /* tell the object it's got bKGD now */

    switch (pData->iJHDRcolortype)     /* store fields for future reference */
    {
      case  8 : ;                      /* gray */
      case 12 : {                      /* graya */
                  pBuf->iBKGDgray  = ((mng_bkgdp)pChunk)->iGray;
                  break;
                }
      case 10 : ;                      /* rgb */
      case 14 : {                      /* rgba */
                  pBuf->iBKGDred   = ((mng_bkgdp)pChunk)->iRed;
                  pBuf->iBKGDgreen = ((mng_bkgdp)pChunk)->iGreen;
                  pBuf->iBKGDblue  = ((mng_bkgdp)pChunk)->iBlue;
                  break;
                }
    }
  }
  else
#endif /* MNG_INCLUDE_JNG */
  if ((pData->bHasIHDR) || (pData->bHasBASI) || (pData->bHasDHDR))
  {
    pBuf->bHasBKGD = MNG_TRUE;         /* tell the object it's got bKGD now */

    switch (pData->iColortype)         /* store fields for future reference */
    {
      case 0 : ;                        /* gray */
      case 4 : {                        /* graya */
                 pBuf->iBKGDgray  = ((mng_bkgdp)pChunk)->iGray;
                 break;
               }
      case 2 : ;                        /* rgb */
      case 6 : {                        /* rgba */
                 pBuf->iBKGDred   = ((mng_bkgdp)pChunk)->iRed;
                 pBuf->iBKGDgreen = ((mng_bkgdp)pChunk)->iGreen;
                 pBuf->iBKGDblue  = ((mng_bkgdp)pChunk)->iBlue;
                 break;
               }
      case 3 : {                        /* indexed */
                 pBuf->iBKGDindex = ((mng_bkgdp)pChunk)->iIndex;
                 break;
               }
    }
  }
  else                                 /* store as global */
  {
    if (!(((mng_bkgdp)pChunk)->bEmpty))
    {
      pData->iGlobalBKGDred   = ((mng_bkgdp)pChunk)->iRed;
      pData->iGlobalBKGDgreen = ((mng_bkgdp)pChunk)->iGreen;
      pData->iGlobalBKGDblue  = ((mng_bkgdp)pChunk)->iBlue;
    }
                                       /* create an animation object */
    return mng_create_ani_bkgd (pData);
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYs
MNG_C_SPECIALFUNC (mng_special_phys)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sBIT
MNG_C_SPECIALFUNC (mng_special_sbit)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
MNG_F_SPECIALFUNC (mng_splt_entries)
{
  mng_spltp  pSPLT    = (mng_spltp)pChunk;
  mng_uint32 iRawlen  = *piRawlen;
  mng_uint8p pRawdata = *ppRawdata;

  if ((pSPLT->iSampledepth != MNG_BITDEPTH_8 ) &&
      (pSPLT->iSampledepth != MNG_BITDEPTH_16)   )
    MNG_ERROR (pData, MNG_INVSAMPLEDEPTH);
                                       /* check remaining length */
  if ( ((pSPLT->iSampledepth == MNG_BITDEPTH_8 ) && (iRawlen %  6 != 0)) ||
       ((pSPLT->iSampledepth == MNG_BITDEPTH_16) && (iRawlen % 10 != 0))    )
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  if (pSPLT->iSampledepth == MNG_BITDEPTH_8)
    pSPLT->iEntrycount = iRawlen / 6;
  else
    pSPLT->iEntrycount = iRawlen / 10;

  if (iRawlen)
  {
    MNG_ALLOC (pData, pSPLT->pEntries, iRawlen);
    MNG_COPY (pSPLT->pEntries, pRawdata, iRawlen);
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
MNG_C_SPECIALFUNC (mng_special_splt)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_hIST
MNG_F_SPECIALFUNC (mng_hist_entries)
{
  mng_histp  pHIST    = (mng_histp)pChunk;
  mng_uint32 iRawlen  = *piRawlen;
  mng_uint8p pRawdata = *ppRawdata;
  mng_uint32 iX;

  if ( ((iRawlen & 0x01) != 0) || ((iRawlen >> 1) != pData->iPLTEcount) )
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  pHIST->iEntrycount = iRawlen >> 1;

  for (iX = 0; iX < pHIST->iEntrycount; iX++)
  {
    pHIST->aEntries[iX] = mng_get_uint16 (pRawdata);
    pRawdata += 2;
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_hIST
MNG_C_SPECIALFUNC (mng_special_hist)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tIME
MNG_C_SPECIALFUNC (mng_special_time)
{
/*  if (pData->fProcesstime) */            /* inform the application ? */
/*  {

    pData->fProcesstime ((mng_handle)pData, );
  } */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */
/* JNG chunks */

#ifdef MNG_INCLUDE_JNG
MNG_C_SPECIALFUNC (mng_special_jhdr)
{
  if ((pData->eSigtype == mng_it_jng) && (pData->iChunkseq > 1))
    MNG_ERROR (pData, MNG_SEQUENCEERROR);
                                       /* inside a JHDR-IEND block now */
  pData->bHasJHDR              = MNG_TRUE;
                                       /* and store interesting fields */
  pData->iDatawidth            = ((mng_jhdrp)pChunk)->iWidth;
  pData->iDataheight           = ((mng_jhdrp)pChunk)->iHeight;
  pData->iJHDRcolortype        = ((mng_jhdrp)pChunk)->iColortype;
  pData->iJHDRimgbitdepth      = ((mng_jhdrp)pChunk)->iImagesampledepth;
  pData->iJHDRimgcompression   = ((mng_jhdrp)pChunk)->iImagecompression;
  pData->iJHDRimginterlace     = ((mng_jhdrp)pChunk)->iImageinterlace;
  pData->iJHDRalphabitdepth    = ((mng_jhdrp)pChunk)->iAlphasampledepth;
  pData->iJHDRalphacompression = ((mng_jhdrp)pChunk)->iAlphacompression;
  pData->iJHDRalphafilter      = ((mng_jhdrp)pChunk)->iAlphafilter;
  pData->iJHDRalphainterlace   = ((mng_jhdrp)pChunk)->iAlphainterlace;

#if defined(MNG_NO_1_2_4BIT_SUPPORT) || defined(MNG_NO_16BIT_SUPPORT)
  pData->iPNGmult = 1;
  pData->iPNGdepth = pData->iJHDRalphabitdepth;
#endif

#ifdef MNG_NO_1_2_4BIT_SUPPORT
  if (pData->iJHDRalphabitdepth < 8)
    pData->iJHDRalphabitdepth = 8;
#endif

#ifdef MNG_NO_16BIT_SUPPORT
  if (pData->iJHDRalphabitdepth > 8)
  {
    pData->iPNGmult = 2;
    pData->iJHDRalphabitdepth = 8;
  }
#endif
                                       /* parameter validity checks */
  if ((pData->iJHDRcolortype != MNG_COLORTYPE_JPEGGRAY  ) &&
      (pData->iJHDRcolortype != MNG_COLORTYPE_JPEGCOLOR ) &&
      (pData->iJHDRcolortype != MNG_COLORTYPE_JPEGGRAYA ) &&
      (pData->iJHDRcolortype != MNG_COLORTYPE_JPEGCOLORA)    )
    MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);

  if ((pData->iJHDRimgbitdepth != MNG_BITDEPTH_JPEG8     ) &&
      (pData->iJHDRimgbitdepth != MNG_BITDEPTH_JPEG12    ) &&
      (pData->iJHDRimgbitdepth != MNG_BITDEPTH_JPEG8AND12)    )
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

  if ((pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAYA ) ||
      (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLORA)    )
  {
    if ((pData->iJHDRalphabitdepth != MNG_BITDEPTH_8 )
#ifndef MNG_NO_1_2_4BIT_SUPPORT
        && (pData->iJHDRalphabitdepth != MNG_BITDEPTH_1 ) &&
        (pData->iJHDRalphabitdepth != MNG_BITDEPTH_2 ) &&
        (pData->iJHDRalphabitdepth != MNG_BITDEPTH_4 )
#endif
#ifndef MNG_NO_16BIT_SUPPORT
        && (pData->iJHDRalphabitdepth != MNG_BITDEPTH_16)
#endif
        )
      MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

    if ((pData->iJHDRalphacompression != MNG_COMPRESSION_DEFLATE     ) &&
        (pData->iJHDRalphacompression != MNG_COMPRESSION_BASELINEJPEG)    )
      MNG_ERROR (pData, MNG_INVALIDCOMPRESS);

    if ((pData->iJHDRalphacompression == MNG_COMPRESSION_BASELINEJPEG) &&
        (pData->iJHDRalphabitdepth    !=  MNG_BITDEPTH_8             )    )
      MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

#if defined(FILTER192) || defined(FILTER193)
    if ((pData->iJHDRalphafilter != MNG_FILTER_ADAPTIVE ) &&
#if defined(FILTER192) && defined(FILTER193)
        (pData->iJHDRalphafilter != MNG_FILTER_DIFFERING) &&
        (pData->iJHDRalphafilter != MNG_FILTER_NOFILTER )    )
#else
#ifdef FILTER192
        (pData->iJHDRalphafilter != MNG_FILTER_DIFFERING)    )
#else
        (pData->iJHDRalphafilter != MNG_FILTER_NOFILTER )    )
#endif
#endif
      MNG_ERROR (pData, MNG_INVALIDFILTER);
#else
    if (pData->iJHDRalphafilter)
      MNG_ERROR (pData, MNG_INVALIDFILTER);
#endif

  }
  else
  {
    if (pData->iJHDRalphabitdepth)
      MNG_ERROR (pData, MNG_INVALIDBITDEPTH);
    if (pData->iJHDRalphacompression)
      MNG_ERROR (pData, MNG_INVALIDCOMPRESS);
    if (pData->iJHDRalphafilter)
      MNG_ERROR (pData, MNG_INVALIDFILTER);
    if (pData->iJHDRalphainterlace)
      MNG_ERROR (pData, MNG_INVALIDINTERLACE);
  }

  if (!pData->bHasheader)              /* first chunk ? */
  {
    pData->bHasheader = MNG_TRUE;      /* we've got a header */
    pData->eImagetype = mng_it_jng;    /* then this must be a JNG */
    pData->iWidth     = ((mng_jhdrp)pChunk)->iWidth;
    pData->iHeight    = ((mng_jhdrp)pChunk)->iHeight;
                                       /* predict alpha-depth ! */
    if ((pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAYA ) ||
        (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLORA)    )
      pData->iAlphadepth = pData->iJHDRalphabitdepth;
    else
      pData->iAlphadepth = 0;
                                       /* fits on maximum canvas ? */
    if ((pData->iWidth > pData->iMaxwidth) || (pData->iHeight > pData->iMaxheight))
      MNG_WARNING (pData, MNG_IMAGETOOLARGE);

    if (pData->fProcessheader)         /* inform the app ? */
      if (!pData->fProcessheader (((mng_handle)pData), pData->iWidth, pData->iHeight))
        MNG_ERROR (pData, MNG_APPMISCERROR);

  }

  pData->iColortype = 0;               /* fake grayscale for other routines */
  pData->iImagelevel++;                /* one level deeper */

#ifdef MNG_SUPPORT_DISPLAY
  {
    mng_retcode iRetcode = mng_process_display_jhdr (pData);
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }
#endif /* MNG_SUPPORT_DISPLAY */

#ifdef MNG_NO_16BIT_SUPPORT
  if (((mng_jhdrp)pChunk)->iAlphasampledepth > 8)
    ((mng_jhdrp)pChunk)->iAlphasampledepth = 8;
#endif

  return MNG_NOERROR;                  /* done */
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
MNG_C_SPECIALFUNC (mng_special_jdaa)
{
  if (pData->iJHDRalphacompression != MNG_COMPRESSION_BASELINEJPEG)
    MNG_ERROR (pData, MNG_SEQUENCEERROR);

  pData->bHasJDAA = MNG_TRUE;          /* got some JDAA now, don't we */
  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
MNG_C_SPECIALFUNC (mng_special_jdat)
{
  pData->bHasJDAT = MNG_TRUE;          /* got some JDAT now, don't we */
  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
MNG_C_SPECIALFUNC (mng_special_jsep)
{
  pData->bHasJSEP = MNG_TRUE;          /* indicate we've had the 8-/12-bit separator */
  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */
/* ************************************************************************** */
/* MNG chunks */

MNG_C_SPECIALFUNC (mng_special_mhdr)
{
  if (pData->bHasheader)               /* can only be the first chunk! */
    MNG_ERROR (pData, MNG_SEQUENCEERROR);

  pData->bHasMHDR     = MNG_TRUE;      /* oh boy, a real MNG */
  pData->bHasheader   = MNG_TRUE;      /* we've got a header */
  pData->eImagetype   = mng_it_mng;    /* fill header fields */
  pData->iWidth       = ((mng_mhdrp)pChunk)->iWidth;
  pData->iHeight      = ((mng_mhdrp)pChunk)->iHeight;
  pData->iTicks       = ((mng_mhdrp)pChunk)->iTicks;
  pData->iLayercount  = ((mng_mhdrp)pChunk)->iLayercount;
  pData->iFramecount  = ((mng_mhdrp)pChunk)->iFramecount;
  pData->iPlaytime    = ((mng_mhdrp)pChunk)->iPlaytime;
  pData->iSimplicity  = ((mng_mhdrp)pChunk)->iSimplicity;
#ifndef MNG_NO_OLD_VERSIONS
  pData->bPreDraft48  = MNG_FALSE;
#endif
                                       /* predict alpha-depth */
  if ((pData->iSimplicity & 0x00000001) == 0)
#ifndef MNG_NO_16BIT_SUPPORT
    pData->iAlphadepth = 16;           /* no indicators = assume the worst */
#else
    pData->iAlphadepth = 8;            /* anything else = assume the worst */
#endif
  else
  if ((pData->iSimplicity & 0x00000008) == 0)
    pData->iAlphadepth = 0;            /* no transparency at all */
  else
  if ((pData->iSimplicity & 0x00000140) == 0x00000040)
    pData->iAlphadepth = 1;            /* no semi-transparency guaranteed */
  else
#ifndef MNG_NO_16BIT_SUPPORT
    pData->iAlphadepth = 16;           /* anything else = assume the worst */
#else
    pData->iAlphadepth = 8;            /* anything else = assume the worst */
#endif

#ifdef MNG_INCLUDE_JNG                 /* can we handle the complexity ? */
  if (pData->iSimplicity & 0x0000FC00)
#else
  if (pData->iSimplicity & 0x0000FC10)
#endif
    MNG_ERROR (pData, MNG_MNGTOOCOMPLEX);
                                       /* fits on maximum canvas ? */
  if ((pData->iWidth > pData->iMaxwidth) || (pData->iHeight > pData->iMaxheight))
    MNG_WARNING (pData, MNG_IMAGETOOLARGE);

  if (pData->fProcessheader)           /* inform the app ? */
    if (!pData->fProcessheader (((mng_handle)pData), pData->iWidth, pData->iHeight))
      MNG_ERROR (pData, MNG_APPMISCERROR);

  pData->iImagelevel++;                /* one level deeper */

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

MNG_C_SPECIALFUNC (mng_special_mend)
{
#ifdef MNG_SUPPORT_DISPLAY
  {                                    /* do something */
    mng_retcode iRetcode = mng_process_display_mend (pData);

    if (iRetcode)                      /* on error bail out */
      return iRetcode;

    if (!pData->iTotalframes)          /* save totals */
      pData->iTotalframes   = pData->iFrameseq;
    if (!pData->iTotallayers)
      pData->iTotallayers   = pData->iLayerseq;
    if (!pData->iTotalplaytime)
      pData->iTotalplaytime = pData->iFrametime;
  }
#endif /* MNG_SUPPORT_DISPLAY */

  pData->bHasMHDR = MNG_FALSE;         /* end of the line, bro! */

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
MNG_F_SPECIALFUNC (mng_debunk_loop)
{
  mng_loopp  pLOOP    = (mng_loopp)pChunk;
  mng_uint32 iRawlen  = *piRawlen;
  mng_uint8p pRawdata = *ppRawdata;

  if (iRawlen >= 5)                    /* length checks */
  {
    if (iRawlen >= 6)
    {
      if ((iRawlen - 6) % 4 != 0)
        MNG_ERROR (pData, MNG_INVALIDLENGTH);
    }
  }
  else
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  if (iRawlen >= 5)                  /* store the fields */
  {
    pLOOP->iLevel  = *pRawdata;

#ifndef MNG_NO_OLD_VERSIONS
    if (pData->bPreDraft48)
    {
      pLOOP->iTermination = *(pRawdata+1);
      pLOOP->iRepeat = mng_get_uint32 (pRawdata+2);
    }
    else
#endif
    {
      pLOOP->iRepeat = mng_get_uint32 (pRawdata+1);
    }

    if (iRawlen >= 6)
    {
#ifndef MNG_NO_OLD_VERSIONS
      if (!pData->bPreDraft48)
#endif
        pLOOP->iTermination = *(pRawdata+5);

      if (iRawlen >= 10)
      {
        pLOOP->iItermin = mng_get_uint32 (pRawdata+6);

#ifndef MNG_NO_LOOP_SIGNALS_SUPPORTED
        if (iRawlen >= 14)
        {
          pLOOP->iItermax = mng_get_uint32 (pRawdata+10);
          pLOOP->iCount   = (iRawlen - 14) / 4;

          if (pLOOP->iCount)
          {
            MNG_ALLOC (pData, pLOOP->pSignals, pLOOP->iCount << 2);

#ifndef MNG_BIGENDIAN_SUPPORTED
            {
              mng_uint32  iX;
              mng_uint8p  pIn  = pRawdata + 14;
              mng_uint32p pOut = (mng_uint32p)pLOOP->pSignals;

              for (iX = 0; iX < pLOOP->iCount; iX++)
              {
                *pOut++ = mng_get_uint32 (pIn);
                pIn += 4;
              }
            }
#else
            MNG_COPY (pLOOP->pSignals, pRawdata + 14, pLOOP->iCount << 2);
#endif /* !MNG_BIGENDIAN_SUPPORTED */
          }
        }
#endif
      }
    }
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
MNG_C_SPECIALFUNC (mng_special_loop)
{
  if (!pData->bCacheplayback)          /* must store playback info to work!! */
    MNG_ERROR (pData, MNG_LOOPWITHCACHEOFF);

#ifdef MNG_SUPPORT_DISPLAY
  {
    mng_retcode iRetcode;

    pData->bHasLOOP = MNG_TRUE;        /* indicate we're inside a loop */
                                       /* create the LOOP ani-object */
    iRetcode = mng_create_ani_loop (pData, pChunk);
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
                                       /* skip till matching ENDL if iteration=0 */
    if ((!pData->bSkipping) && (((mng_loopp)pChunk)->iRepeat == 0))
      pData->bSkipping = MNG_TRUE;
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
MNG_C_SPECIALFUNC (mng_special_endl)
{
#ifdef MNG_SUPPORT_DISPLAY
  if (pData->bHasLOOP)                 /* are we really processing a loop ? */
  {
    mng_uint8 iLevel = ((mng_endlp)pChunk)->iLevel;
                                       /* create an ENDL animation object */
    return mng_create_ani_endl (pData, iLevel);
  }
  else
    MNG_ERROR (pData, MNG_NOMATCHINGLOOP);
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DEFI
MNG_C_SPECIALFUNC (mng_special_defi)
{
#ifdef MNG_SUPPORT_DISPLAY
  mng_retcode iRetcode;

  pData->iDEFIobjectid     = ((mng_defip)pChunk)->iObjectid;
  pData->bDEFIhasdonotshow = ((mng_defip)pChunk)->bHasdonotshow;
  pData->iDEFIdonotshow    = ((mng_defip)pChunk)->iDonotshow;
  pData->bDEFIhasconcrete  = ((mng_defip)pChunk)->bHasconcrete;
  pData->iDEFIconcrete     = ((mng_defip)pChunk)->iConcrete;
  pData->bDEFIhasloca      = ((mng_defip)pChunk)->bHasloca;
  pData->iDEFIlocax        = ((mng_defip)pChunk)->iXlocation;
  pData->iDEFIlocay        = ((mng_defip)pChunk)->iYlocation;
  pData->bDEFIhasclip      = ((mng_defip)pChunk)->bHasclip;
  pData->iDEFIclipl        = ((mng_defip)pChunk)->iLeftcb;
  pData->iDEFIclipr        = ((mng_defip)pChunk)->iRightcb;
  pData->iDEFIclipt        = ((mng_defip)pChunk)->iTopcb;
  pData->iDEFIclipb        = ((mng_defip)pChunk)->iBottomcb;
                                       /* create an animation object */
  iRetcode = mng_create_ani_defi (pData);
  if (!iRetcode)                       /* do display processing */
    iRetcode = mng_process_display_defi (pData);
  return iRetcode;
#else
  return MNG_NOERROR;                  /* done */
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BASI
MNG_C_SPECIALFUNC (mng_special_basi)
{
  pData->bHasBASI     = MNG_TRUE;      /* inside a BASI-IEND block now */
                                       /* store interesting fields */
  pData->iDatawidth   = ((mng_basip)pChunk)->iWidth;
  pData->iDataheight  = ((mng_basip)pChunk)->iHeight;
  pData->iBitdepth    = ((mng_basip)pChunk)->iBitdepth;   
  pData->iColortype   = ((mng_basip)pChunk)->iColortype;
  pData->iCompression = ((mng_basip)pChunk)->iCompression;
  pData->iFilter      = ((mng_basip)pChunk)->iFilter;
  pData->iInterlace   = ((mng_basip)pChunk)->iInterlace;

#if defined(MNG_NO_1_2_4BIT_SUPPORT) || defined(MNG_NO_16BIT_SUPPORT)
  pData->iPNGmult = 1;
  pData->iPNGdepth = pData->iBitdepth;
#endif

#ifdef MNG_NO_1_2_4BIT_SUPPORT
  if (pData->iBitdepth < 8)
    pData->iBitdepth = 8;
#endif
#ifdef MNG_NO_16BIT_SUPPORT
  if (pData->iBitdepth > 8)
    {
      pData->iBitdepth = 8;
      pData->iPNGmult = 2;
    }
#endif

  if ((pData->iBitdepth !=  8)      /* parameter validity checks */
#ifndef MNG_NO_1_2_4BIT_SUPPORT
      && (pData->iBitdepth !=  1) &&
      (pData->iBitdepth !=  2) &&
      (pData->iBitdepth !=  4)
#endif
#ifndef MNG_NO_16BIT_SUPPORT
      && (pData->iBitdepth != 16)
#endif
      )
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

  if ((pData->iColortype != MNG_COLORTYPE_GRAY   ) &&
      (pData->iColortype != MNG_COLORTYPE_RGB    ) &&
      (pData->iColortype != MNG_COLORTYPE_INDEXED) &&
      (pData->iColortype != MNG_COLORTYPE_GRAYA  ) &&
      (pData->iColortype != MNG_COLORTYPE_RGBA   )    )
    MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);

  if ((pData->iColortype == MNG_COLORTYPE_INDEXED) && (pData->iBitdepth > 8))
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

  if (((pData->iColortype == MNG_COLORTYPE_RGB    ) ||
       (pData->iColortype == MNG_COLORTYPE_GRAYA  ) ||
       (pData->iColortype == MNG_COLORTYPE_RGBA   )    ) &&
      (pData->iBitdepth < 8                            )    )
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

#if defined(FILTER192) || defined(FILTER193)
  if ((pData->iFilter != MNG_FILTER_ADAPTIVE ) &&
#if defined(FILTER192) && defined(FILTER193)
      (pData->iFilter != MNG_FILTER_DIFFERING) &&
      (pData->iFilter != MNG_FILTER_NOFILTER )    )
#else
#ifdef FILTER192
      (pData->iFilter != MNG_FILTER_DIFFERING)    )
#else
      (pData->iFilter != MNG_FILTER_NOFILTER )    )
#endif
#endif
    MNG_ERROR (pData, MNG_INVALIDFILTER);
#else
  if (pData->iFilter)
    MNG_ERROR (pData, MNG_INVALIDFILTER);
#endif

  pData->iImagelevel++;                /* one level deeper */

#ifdef MNG_SUPPORT_DISPLAY
  {                                    /* create an animation object */
    mng_retcode iRetcode = mng_create_ani_basi (pData, pChunk);
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }
#endif /* MNG_SUPPORT_DISPLAY */

#ifdef MNG_NO_16BIT_SUPPORT
  if (((mng_basip)pChunk)->iBitdepth > 8)
    ((mng_basip)pChunk)->iBitdepth = 8;
#endif

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLON
MNG_C_SPECIALFUNC (mng_special_clon)
{
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ani_clon (pData, pChunk);
#else
  return MNG_NOERROR;                  /* done */
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
MNG_F_SPECIALFUNC (mng_debunk_past)
{
  mng_pastp        pPAST    = (mng_pastp)pChunk;
  mng_uint32       iRawlen  = *piRawlen;
  mng_uint8p       pRawdata = *ppRawdata;
  mng_uint32       iSize;
  mng_uint32       iX;
  mng_past_sourcep pSource;
                                       /* check the length */
  if ((iRawlen < 41) || (((iRawlen - 11) % 30) != 0))
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  pPAST->iDestid     = mng_get_uint16 (pRawdata);
  pPAST->iTargettype = *(pRawdata+2);
  pPAST->iTargetx    = mng_get_int32  (pRawdata+3);
  pPAST->iTargety    = mng_get_int32  (pRawdata+7);
  pPAST->iCount      = ((iRawlen - 11) / 30); /* how many entries again? */
  iSize              = pPAST->iCount * sizeof (mng_past_source);

  pRawdata += 11;
                                       /* get a buffer for all the source blocks */
  MNG_ALLOC (pData, pPAST->pSources, iSize);

  pSource = (mng_past_sourcep)(pPAST->pSources);

  for (iX = pPAST->iCount; iX > 0; iX--)
  {                                    /* now copy the source blocks */
    pSource->iSourceid     = mng_get_uint16 (pRawdata);
    pSource->iComposition  = *(pRawdata+2);
    pSource->iOrientation  = *(pRawdata+3);
    pSource->iOffsettype   = *(pRawdata+4);
    pSource->iOffsetx      = mng_get_int32 (pRawdata+5);
    pSource->iOffsety      = mng_get_int32 (pRawdata+9);
    pSource->iBoundarytype = *(pRawdata+13);
    pSource->iBoundaryl    = mng_get_int32 (pRawdata+14);
    pSource->iBoundaryr    = mng_get_int32 (pRawdata+18);
    pSource->iBoundaryt    = mng_get_int32 (pRawdata+22);
    pSource->iBoundaryb    = mng_get_int32 (pRawdata+26);

    pSource++;
    pRawdata += 30;
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
MNG_C_SPECIALFUNC (mng_special_past)
{
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ani_past (pData, pChunk);
#else
  return MNG_NOERROR;
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
MNG_F_SPECIALFUNC (mng_disc_entries)
{
  mng_discp   pDISC    = (mng_discp)pChunk;
  mng_uint32  iRawlen  = *piRawlen;
  mng_uint8p  pRawdata = *ppRawdata;

  if ((iRawlen % 2) != 0)              /* check the length */
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  pDISC->iCount = (iRawlen / sizeof (mng_uint16));

  if (pDISC->iCount)
  {
    MNG_ALLOC (pData, pDISC->pObjectids, iRawlen);

#ifndef MNG_BIGENDIAN_SUPPORTED
    {
      mng_uint32  iX;
      mng_uint8p  pIn  = pRawdata;
      mng_uint16p pOut = pDISC->pObjectids;

      for (iX = pDISC->iCount; iX > 0; iX--)
      {
        *pOut++ = mng_get_uint16 (pIn);
        pIn += 2;
      }
    }
#else
    MNG_COPY (pDISC->pObjectids, pRawdata, iRawlen);
#endif /* !MNG_BIGENDIAN_SUPPORTED */
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
MNG_C_SPECIALFUNC (mng_special_disc)
{
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ani_disc (pData, pChunk);
#else
  return MNG_NOERROR;                  
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BACK
MNG_C_SPECIALFUNC (mng_special_back)
{
#ifdef MNG_SUPPORT_DISPLAY
                                       /* retrieve the fields */
  pData->bHasBACK       = MNG_TRUE;
  pData->iBACKred       = ((mng_backp)pChunk)->iRed;
  pData->iBACKgreen     = ((mng_backp)pChunk)->iGreen;
  pData->iBACKblue      = ((mng_backp)pChunk)->iBlue;
  pData->iBACKmandatory = ((mng_backp)pChunk)->iMandatory;
  pData->iBACKimageid   = ((mng_backp)pChunk)->iImageid;
  pData->iBACKtile      = ((mng_backp)pChunk)->iTile;

  return mng_create_ani_back (pData);
#else
  return MNG_NOERROR;
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
MNG_F_SPECIALFUNC (mng_fram_remainder)
{
  mng_framp  pFRAM     = (mng_framp)pChunk;
  mng_uint32 iRawlen   = *piRawlen;
  mng_uint8p pRawdata  = *ppRawdata;
  mng_uint32 iRequired = 0;

  if (iRawlen < 4)                     /* must have at least 4 bytes */
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  iRequired = 4;                       /* calculate and check required remaining length */

  pFRAM->iChangedelay    = *pRawdata;
  pFRAM->iChangetimeout  = *(pRawdata+1);
  pFRAM->iChangeclipping = *(pRawdata+2);
  pFRAM->iChangesyncid   = *(pRawdata+3);

  if (pFRAM->iChangedelay   ) { iRequired +=  4; }
  if (pFRAM->iChangetimeout ) { iRequired +=  4; }
  if (pFRAM->iChangeclipping) { iRequired += 17; }

  if (pFRAM->iChangesyncid)
  {
    if ((iRawlen - iRequired) % 4 != 0)
      MNG_ERROR (pData, MNG_INVALIDLENGTH);
  }
  else
  {
    if (iRawlen != iRequired)
      MNG_ERROR (pData, MNG_INVALIDLENGTH);
  }

  pRawdata += 4;

  if (pFRAM->iChangedelay)              /* delay changed ? */
  {
    pFRAM->iDelay = mng_get_uint32 (pRawdata);
    pRawdata += 4;
  }

  if (pFRAM->iChangetimeout)            /* timeout changed ? */
  {
    pFRAM->iTimeout = mng_get_uint32 (pRawdata);
    pRawdata += 4;
  }

  if (pFRAM->iChangeclipping)           /* clipping changed ? */
  {
    pFRAM->iBoundarytype = *pRawdata;
    pFRAM->iBoundaryl    = mng_get_int32 (pRawdata+1);
    pFRAM->iBoundaryr    = mng_get_int32 (pRawdata+5);
    pFRAM->iBoundaryt    = mng_get_int32 (pRawdata+9);
    pFRAM->iBoundaryb    = mng_get_int32 (pRawdata+13);
    pRawdata += 17;
  }

  if (pFRAM->iChangesyncid)
  {
    pFRAM->iCount    = (iRawlen - iRequired) / 4;

    if (pFRAM->iCount)
    {
      MNG_ALLOC (pData, pFRAM->pSyncids, pFRAM->iCount * 4);

#ifndef MNG_BIGENDIAN_SUPPORTED
      {
        mng_uint32 iX;
        mng_uint32p pOut = pFRAM->pSyncids;

        for (iX = pFRAM->iCount; iX > 0; iX--)
        {
          *pOut++ = mng_get_uint32 (pRawdata);
          pRawdata += 4;
        }
      }
#else
      MNG_COPY (pFRAM->pSyncids, pRawdata, pFRAM->iCount * 4);
#endif /* !MNG_BIGENDIAN_SUPPORTED */
    }
  }

#ifndef MNG_NO_OLD_VERSIONS
  if (pData->bPreDraft48)              /* old style input-stream ? */
  {
    switch (pFRAM->iMode)              /* fix the framing mode then */
    {
      case  0: { break; }
      case  1: { pFRAM->iMode = 3; break; }
      case  2: { pFRAM->iMode = 4; break; }
      case  3: { pFRAM->iMode = 1; break; }
      case  4: { pFRAM->iMode = 1; break; }
      case  5: { pFRAM->iMode = 2; break; }
      default: { pFRAM->iMode = 1; break; }
    }
  }
#endif

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
MNG_C_SPECIALFUNC (mng_special_fram)
{
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ani_fram (pData, pChunk);
#else
  return MNG_NOERROR;
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MOVE
MNG_C_SPECIALFUNC (mng_special_move)
{
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ani_move (pData, pChunk);
#else
  return MNG_NOERROR;
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLIP
MNG_C_SPECIALFUNC (mng_special_clip)
{
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ani_clip (pData, pChunk);
#else
  return MNG_NOERROR;                  
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SHOW
MNG_C_SPECIALFUNC (mng_special_show)
{
#ifdef MNG_SUPPORT_DISPLAY
  mng_retcode iRetcode;

  if (!((mng_showp)pChunk)->bEmpty)    /* any data ? */
  {
    if (!((mng_showp)pChunk)->bHaslastid)
      ((mng_showp)pChunk)->iLastid = ((mng_showp)pChunk)->iFirstid;

    pData->iSHOWfromid = ((mng_showp)pChunk)->iFirstid;
    pData->iSHOWtoid   = ((mng_showp)pChunk)->iLastid;
    pData->iSHOWmode   = ((mng_showp)pChunk)->iMode;
  }
  else                                 /* use defaults then */
  {
    pData->iSHOWfromid = 1;
    pData->iSHOWtoid   = 65535;
    pData->iSHOWmode   = 2;
  }
                                       /* create a SHOW animation object */
  iRetcode = mng_create_ani_show (pData);
  if (!iRetcode)                       /* go and do it! */
    iRetcode = mng_process_display_show (pData);

#endif /* MNG_SUPPORT_DISPLAY */

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_TERM
MNG_C_SPECIALFUNC (mng_special_term)
{
                                       /* should be behind MHDR or SAVE !! */
  if ((!pData->bHasSAVE) && (pData->iChunkseq > 2))
  {
    pData->bMisplacedTERM = MNG_TRUE;  /* indicate we found a misplaced TERM */
                                       /* and send a warning signal!!! */
    MNG_WARNING (pData, MNG_SEQUENCEERROR);
  }

  pData->bHasTERM = MNG_TRUE;

  if (pData->fProcessterm)             /* inform the app ? */
    if (!pData->fProcessterm (((mng_handle)pData),
                              ((mng_termp)pChunk)->iTermaction,
                              ((mng_termp)pChunk)->iIteraction,
                              ((mng_termp)pChunk)->iDelay,
                              ((mng_termp)pChunk)->iItermax))
      MNG_ERROR (pData, MNG_APPMISCERROR);

#ifdef MNG_SUPPORT_DISPLAY
  {                                    /* create the TERM ani-object */
    mng_retcode iRetcode = mng_create_ani_term (pData, pChunk);
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
                                       /* save for future reference */
    pData->pTermaniobj = pData->pLastaniobj;
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
MNG_F_SPECIALFUNC (mng_save_entries)
{
  mng_savep       pSAVE     = (mng_savep)pChunk;
  mng_uint32      iRawlen   = *piRawlen;
  mng_uint8p      pRawdata  = *ppRawdata;
  mng_save_entryp pEntry    = MNG_NULL;
  mng_uint32      iCount    = 0;
  mng_uint8       iOtype    = *pRawdata;
  mng_uint8       iEtype;
  mng_uint8p      pTemp;
  mng_uint8p      pNull;
  mng_uint32      iLen;
  mng_uint32      iOffset[2];
  mng_uint32      iStarttime[2];
  mng_uint32      iFramenr;
  mng_uint32      iLayernr;
  mng_uint32      iX;
  mng_uint32      iNamesize;

  if ((iOtype != 4) && (iOtype != 8))
    MNG_ERROR (pData, MNG_INVOFFSETSIZE);

  pSAVE->iOffsettype = iOtype;

  for (iX = 0; iX < 2; iX++)       /* do this twice to get the count first ! */
  {
    pTemp = pRawdata + 1;
    iLen  = iRawlen  - 1;

    if (iX)                        /* second run ? */
    {
      MNG_ALLOC (pData, pEntry, (iCount * sizeof (mng_save_entry)));

      pSAVE->iCount   = iCount;
      pSAVE->pEntries = pEntry;
    }

    while (iLen)                   /* anything left ? */
    {
      iEtype = *pTemp;             /* entrytype */

      if ((iEtype != 0) && (iEtype != 1) && (iEtype != 2) && (iEtype != 3))
        MNG_ERROR (pData, MNG_INVENTRYTYPE);

      pTemp++;

      if (iEtype > 1)
      {
        iOffset    [0] = 0;
        iOffset    [1] = 0;
        iStarttime [0] = 0;
        iStarttime [1] = 0;
        iLayernr       = 0;
        iFramenr       = 0;
      }
      else
      {
        if (iOtype == 4)
        {
          iOffset [0] = 0;
          iOffset [1] = mng_get_uint32 (pTemp);

          pTemp += 4;
        }
        else
        {
          iOffset [0] = mng_get_uint32 (pTemp);
          iOffset [1] = mng_get_uint32 (pTemp+4);

          pTemp += 8;
        }

        if (iEtype > 0)
        {
          iStarttime [0] = 0;
          iStarttime [1] = 0;
          iLayernr       = 0;
          iFramenr       = 0;
        }
        else
        {
          if (iOtype == 4)
          {
            iStarttime [0] = 0;
            iStarttime [1] = mng_get_uint32 (pTemp+0);
            iLayernr       = mng_get_uint32 (pTemp+4);
            iFramenr       = mng_get_uint32 (pTemp+8);

            pTemp += 12;
          }
          else
          {
            iStarttime [0] = mng_get_uint32 (pTemp+0);
            iStarttime [1] = mng_get_uint32 (pTemp+4);
            iLayernr       = mng_get_uint32 (pTemp+8);
            iFramenr       = mng_get_uint32 (pTemp+12);

            pTemp += 16;
          }
        }
      }

      pNull = pTemp;               /* get the name length */
      while (*pNull)
        pNull++;

      if ((pNull - pRawdata) > (mng_int32)iRawlen)
      {
        iNamesize = iLen;          /* no null found; so end of SAVE */
        iLen      = 0;
      }
      else
      {
        iNamesize = pNull - pTemp; /* should be another entry */
        iLen     -= iNamesize;

        if (!iLen)                 /* must not end with a null ! */
          MNG_ERROR (pData, MNG_ENDWITHNULL);
      }

      if (!pEntry)
      {
        iCount++;
      }
      else
      {
        pEntry->iEntrytype     = iEtype;
        pEntry->iOffset    [0] = iOffset    [0];
        pEntry->iOffset    [1] = iOffset    [1];
        pEntry->iStarttime [0] = iStarttime [0];
        pEntry->iStarttime [1] = iStarttime [1];
        pEntry->iLayernr       = iLayernr;
        pEntry->iFramenr       = iFramenr;
        pEntry->iNamesize      = iNamesize;

        if (iNamesize)
        {
          MNG_ALLOC (pData, pEntry->zName, iNamesize+1);
          MNG_COPY (pEntry->zName, pTemp, iNamesize);
        }

        pEntry++;
      }

      pTemp += iNamesize;
    }
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
MNG_C_SPECIALFUNC (mng_special_save)
{
  pData->bHasSAVE = MNG_TRUE;

  if (pData->fProcesssave)             /* inform the application ? */
  {
    mng_bool bOke = pData->fProcesssave ((mng_handle)pData);
    if (!bOke)
      MNG_ERROR (pData, MNG_APPMISCERROR);
  }

#ifdef MNG_SUPPORT_DISPLAY
  {
    mng_retcode iRetcode;

    /* TODO: something with the parameters */

                                       /* create a SAVE animation object */
    iRetcode = mng_create_ani_save (pData);
    if (!iRetcode)                     /* process it */
      iRetcode = mng_process_display_save (pData);
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
MNG_C_SPECIALFUNC (mng_special_seek)
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_DISPLAY
                                       /* create a SEEK animation object */
  iRetcode = mng_create_ani_seek (pData, pChunk);
  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#endif /* MNG_SUPPORT_DISPLAY */

  if (pData->fProcessseek)             /* inform the app ? */
    if (!pData->fProcessseek ((mng_handle)pData, ((mng_seekp)pChunk)->zName))
      MNG_ERROR (pData, MNG_APPMISCERROR);

#ifdef MNG_SUPPORT_DISPLAY
  return mng_process_display_seek (pData);
#else
  return MNG_NOERROR;                  
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_eXPI
MNG_C_SPECIALFUNC (mng_special_expi)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_fPRI
MNG_C_SPECIALFUNC (mng_special_fpri)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
MNG_LOCAL mng_bool CheckKeyword (mng_datap  pData,
                                 mng_uint8p pKeyword)
{
  mng_chunkid handled_chunks [] =
  {
    MNG_UINT_BACK,                     /* keep it sorted !!!! */
    MNG_UINT_BASI,
    MNG_UINT_CLIP,
    MNG_UINT_CLON,
#ifndef MNG_NO_DELTA_PNG
/* TODO:    MNG_UINT_DBYK,  */
#endif
    MNG_UINT_DEFI,
#ifndef MNG_NO_DELTA_PNG
    MNG_UINT_DHDR,
#endif
    MNG_UINT_DISC,
#ifndef MNG_NO_DELTA_PNG
/* TODO:    MNG_UINT_DROP,  */
#endif
    MNG_UINT_ENDL,
    MNG_UINT_FRAM,
    MNG_UINT_IDAT,
    MNG_UINT_IEND,
    MNG_UINT_IHDR,
#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
    MNG_UINT_IJNG,
#endif    
    MNG_UINT_IPNG,
#endif
#ifdef MNG_INCLUDE_JNG
    MNG_UINT_JDAA,
    MNG_UINT_JDAT,
    MNG_UINT_JHDR,
/* TODO:    MNG_UINT_JSEP,  */
    MNG_UINT_JdAA,
#endif
    MNG_UINT_LOOP,
    MNG_UINT_MAGN,
    MNG_UINT_MEND,
    MNG_UINT_MHDR,
    MNG_UINT_MOVE,
/* TODO:    MNG_UINT_ORDR,  */
    MNG_UINT_PAST,
    MNG_UINT_PLTE,
#ifndef MNG_NO_DELTA_PNG
    MNG_UINT_PPLT,
    MNG_UINT_PROM,
#endif
    MNG_UINT_SAVE,
    MNG_UINT_SEEK,
    MNG_UINT_SHOW,
    MNG_UINT_TERM,
    MNG_UINT_bKGD,
    MNG_UINT_cHRM,
/* TODO:    MNG_UINT_eXPI,  */
    MNG_UINT_evNT,
/* TODO:    MNG_UINT_fPRI,  */
    MNG_UINT_gAMA,
/* TODO:    MNG_UINT_hIST,  */
    MNG_UINT_iCCP,
    MNG_UINT_iTXt,
    MNG_UINT_nEED,
/* TODO:    MNG_UINT_oFFs,  */
/* TODO:    MNG_UINT_pCAL,  */
/* TODO:    MNG_UINT_pHYg,  */
/* TODO:    MNG_UINT_pHYs,  */
/* TODO:    MNG_UINT_sBIT,  */
/* TODO:    MNG_UINT_sCAL,  */
/* TODO:    MNG_UINT_sPLT,  */
    MNG_UINT_sRGB,
    MNG_UINT_tEXt,
    MNG_UINT_tIME,
    MNG_UINT_tRNS,
    MNG_UINT_zTXt,
  };

  mng_bool bOke = MNG_FALSE;

  if (pData->fProcessneed)             /* does the app handle it ? */
    bOke = pData->fProcessneed ((mng_handle)pData, (mng_pchar)pKeyword);

  if (!bOke)
  {                                    /* find the keyword length */
    mng_uint8p pNull = pKeyword;
    while (*pNull)
      pNull++;

    if ((pNull - pKeyword) == 4)       /* test a chunk ? */
    {                                  /* get the chunk-id */
      mng_chunkid iChunkid = (*pKeyword     << 24) + (*(pKeyword+1) << 16) +
                             (*(pKeyword+2) <<  8) + (*(pKeyword+3)      );
                                       /* binary search variables */
      mng_int32   iTop, iLower, iUpper, iMiddle;
                                       /* determine max index of table */
      iTop = (sizeof (handled_chunks) / sizeof (handled_chunks [0])) - 1;

      /* binary search; with 52 chunks, worst-case is 7 comparisons */
      iLower  = 0;
      iMiddle = iTop >> 1;
      iUpper  = iTop;

      do                                   /* the binary search itself */
        {
          if (handled_chunks [iMiddle] < iChunkid)
            iLower = iMiddle + 1;
          else if (handled_chunks [iMiddle] > iChunkid)
            iUpper = iMiddle - 1;
          else
          {
            bOke = MNG_TRUE;
            break;
          }

          iMiddle = (iLower + iUpper) >> 1;
        }
      while (iLower <= iUpper);
    }
                                       /* test draft ? */
    if ((!bOke) && ((pNull - pKeyword) == 8) &&
        (*pKeyword     == 'd') && (*(pKeyword+1) == 'r') &&
        (*(pKeyword+2) == 'a') && (*(pKeyword+3) == 'f') &&
        (*(pKeyword+4) == 't') && (*(pKeyword+5) == ' '))
    {
      mng_uint32 iDraft;

      iDraft = (*(pKeyword+6) - '0') * 10 + (*(pKeyword+7) - '0');
      bOke   = (mng_bool)(iDraft <= MNG_MNG_DRAFT);
    }
                                       /* test MNG 1.0/1.1 ? */
    if ((!bOke) && ((pNull - pKeyword) == 7) &&
        (*pKeyword     == 'M') && (*(pKeyword+1) == 'N') &&
        (*(pKeyword+2) == 'G') && (*(pKeyword+3) == '-') &&
        (*(pKeyword+4) == '1') && (*(pKeyword+5) == '.') &&
        ((*(pKeyword+6) == '0') || (*(pKeyword+6) == '1')))
      bOke   = MNG_TRUE;
                                       /* test CACHEOFF ? */
    if ((!bOke) && ((pNull - pKeyword) == 8) &&
        (*pKeyword     == 'C') && (*(pKeyword+1) == 'A') &&
        (*(pKeyword+2) == 'C') && (*(pKeyword+3) == 'H') &&
        (*(pKeyword+4) == 'E') && (*(pKeyword+5) == 'O') &&
        (*(pKeyword+6) == 'F') && (*(pKeyword+7) == 'F'))
    {
      if (!pData->pFirstaniobj)        /* only if caching hasn't started yet ! */
      {
        bOke                  = MNG_TRUE;
        pData->bCacheplayback = MNG_FALSE;
        pData->bStorechunks   = MNG_FALSE;
      }
    }
  }

  return bOke;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
MNG_C_SPECIALFUNC (mng_special_need)
{
                                       /* let's check it */
  mng_bool   bOke = MNG_TRUE;
  mng_uint8p pNull, pTemp, pMax;

  pTemp = (mng_uint8p)((mng_needp)pChunk)->zKeywords;
  pMax  = (mng_uint8p)(pTemp + ((mng_needp)pChunk)->iKeywordssize);
  pNull = pTemp;
  while (*pNull)
    pNull++;

  while ((bOke) && (pNull < pMax))
  {
    bOke  = CheckKeyword (pData, pTemp);
    pTemp = pNull + 1;
    pNull = pTemp;
    while (*pNull)
      pNull++;
  }

  if (bOke)
    bOke = CheckKeyword (pData, pTemp);

  if (!bOke)
    MNG_ERROR (pData, MNG_UNSUPPORTEDNEED);

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYg
MNG_C_SPECIALFUNC (mng_special_phyg)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_C_SPECIALFUNC (mng_special_dhdr)
{
  if ((((mng_dhdrp)pChunk)->iDeltatype == MNG_DELTATYPE_REPLACE) && (((mng_dhdrp)pChunk)->bHasblockloc))
    MNG_ERROR (pData, MNG_INVALIDLENGTH);
  if ((((mng_dhdrp)pChunk)->iDeltatype == MNG_DELTATYPE_NOCHANGE) && (((mng_dhdrp)pChunk)->bHasblocksize))
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  pData->bHasDHDR   = MNG_TRUE;        /* inside a DHDR-IEND block now */
  pData->iDeltatype = ((mng_dhdrp)pChunk)->iDeltatype;

  pData->iImagelevel++;                /* one level deeper */

#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ani_dhdr (pData, pChunk);
#else
  return MNG_NOERROR;                  
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_C_SPECIALFUNC (mng_special_prom)
{
  if ((((mng_promp)pChunk)->iColortype != MNG_COLORTYPE_GRAY   ) &&
      (((mng_promp)pChunk)->iColortype != MNG_COLORTYPE_RGB    ) &&
      (((mng_promp)pChunk)->iColortype != MNG_COLORTYPE_INDEXED) &&
      (((mng_promp)pChunk)->iColortype != MNG_COLORTYPE_GRAYA  ) &&
      (((mng_promp)pChunk)->iColortype != MNG_COLORTYPE_RGBA   )    )
    MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);

#ifdef MNG_NO_16BIT_SUPPORT
  if (((mng_promp)pChunk)->iSampledepth == MNG_BITDEPTH_16 )
      ((mng_promp)pChunk)->iSampledepth = MNG_BITDEPTH_8;
#endif

  if ((((mng_promp)pChunk)->iSampledepth != MNG_BITDEPTH_1 ) &&
      (((mng_promp)pChunk)->iSampledepth != MNG_BITDEPTH_2 ) &&
      (((mng_promp)pChunk)->iSampledepth != MNG_BITDEPTH_4 ) &&
      (((mng_promp)pChunk)->iSampledepth != MNG_BITDEPTH_8 )
#ifndef MNG_NO_16BIT_SUPPORT
      && (((mng_promp)pChunk)->iSampledepth != MNG_BITDEPTH_16)
#endif
    )
    MNG_ERROR (pData, MNG_INVSAMPLEDEPTH);

#ifdef MNG_SUPPORT_DISPLAY
  {
    mng_retcode iRetcode = mng_create_ani_prom (pData, pChunk);
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_C_SPECIALFUNC (mng_special_ipng)
{
#ifdef MNG_SUPPORT_DISPLAY
  mng_retcode iRetcode = mng_create_ani_ipng (pData);
  if (!iRetcode)                       /* process it */
    iRetcode = mng_process_display_ipng (pData);
  if (iRetcode)                        /* on error bail out */
    return iRetcode;
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_F_SPECIALFUNC (mng_pplt_entries)
{
  mng_ppltp     pPPLT      = (mng_ppltp)pChunk;
  mng_uint32    iRawlen    = *piRawlen;
  mng_uint8p    pRawdata   = *ppRawdata;
  mng_uint8     iDeltatype = pPPLT->iDeltatype;
  mng_uint32    iMax       = 0;
  mng_int32     iX, iY, iM;
  mng_rgbpaltab aIndexentries;
  mng_uint8arr  aAlphaentries;
  mng_uint8arr  aUsedentries;
                                       /* must be indexed color ! */
  if (pData->iColortype != MNG_COLORTYPE_INDEXED)
    MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);

  for (iY = 255; iY >= 0; iY--)        /* reset arrays */
  {
    aIndexentries [iY].iRed   = 0;
    aIndexentries [iY].iGreen = 0;
    aIndexentries [iY].iBlue  = 0;
    aAlphaentries [iY]        = 255;
    aUsedentries  [iY]        = 0;
  }

  while (iRawlen)                      /* as long as there are entries left ... */
  {
    mng_uint32 iDiff;

    if (iRawlen < 2)
      MNG_ERROR (pData, MNG_INVALIDLENGTH);

    iX = (mng_int32)(*pRawdata);      /* get start and end index */
    iM = (mng_int32)(*(pRawdata+1));

    if (iM < iX)
      MNG_ERROR (pData, MNG_INVALIDINDEX);

    if (iM >= (mng_int32) iMax)       /* determine highest used index */
      iMax = iM + 1;

    pRawdata += 2;
    iRawlen  -= 2;
    iDiff = (iM - iX + 1);
    if ((iDeltatype == MNG_DELTATYPE_REPLACERGB  ) ||
        (iDeltatype == MNG_DELTATYPE_DELTARGB    )    )
      iDiff = iDiff * 3;
    else
    if ((iDeltatype == MNG_DELTATYPE_REPLACERGBA) ||
        (iDeltatype == MNG_DELTATYPE_DELTARGBA  )    )
      iDiff = iDiff * 4;

    if (iRawlen < iDiff)
      MNG_ERROR (pData, MNG_INVALIDLENGTH);

    if ((iDeltatype == MNG_DELTATYPE_REPLACERGB  ) ||
        (iDeltatype == MNG_DELTATYPE_DELTARGB    )    )
    {
      for (iY = iX; iY <= iM; iY++)
      {
        aIndexentries [iY].iRed   = *pRawdata;
        aIndexentries [iY].iGreen = *(pRawdata+1);
        aIndexentries [iY].iBlue  = *(pRawdata+2);
        aUsedentries  [iY]        = 1;

        pRawdata += 3;
        iRawlen  -= 3;
      }
    }
    else
    if ((iDeltatype == MNG_DELTATYPE_REPLACEALPHA) ||
        (iDeltatype == MNG_DELTATYPE_DELTAALPHA  )    )
    {
      for (iY = iX; iY <= iM; iY++)
      {
        aAlphaentries [iY]        = *pRawdata;
        aUsedentries  [iY]        = 1;

        pRawdata++;
        iRawlen--;
      }
    }
    else
    {
      for (iY = iX; iY <= iM; iY++)
      {
        aIndexentries [iY].iRed   = *pRawdata;
        aIndexentries [iY].iGreen = *(pRawdata+1);
        aIndexentries [iY].iBlue  = *(pRawdata+2);
        aAlphaentries [iY]        = *(pRawdata+3);
        aUsedentries  [iY]        = 1;

        pRawdata += 4;
        iRawlen  -= 4;
      }
    }
  }

  switch (pData->iBitdepth)            /* check maximum allowed entries for bitdepth */
  {
    case MNG_BITDEPTH_1 : {
                            if (iMax > 2)
                              MNG_ERROR (pData, MNG_INVALIDINDEX);
                            break;
                          }
    case MNG_BITDEPTH_2 : {
                            if (iMax > 4)
                              MNG_ERROR (pData, MNG_INVALIDINDEX);
                            break;
                          }
    case MNG_BITDEPTH_4 : {
                            if (iMax > 16)
                              MNG_ERROR (pData, MNG_INVALIDINDEX);
                            break;
                          }
  }

  pPPLT->iCount = iMax;

  for (iY = 255; iY >= 0; iY--)        
  {
    pPPLT->aEntries [iY].iRed   = aIndexentries [iY].iRed;
    pPPLT->aEntries [iY].iGreen = aIndexentries [iY].iGreen;
    pPPLT->aEntries [iY].iBlue  = aIndexentries [iY].iBlue;
    pPPLT->aEntries [iY].iAlpha = aAlphaentries [iY];
    pPPLT->aEntries [iY].bUsed  = (mng_bool)(aUsedentries [iY]);
  }

  {                                    /* create animation object */
    mng_retcode iRetcode = mng_create_ani_pplt (pData, iDeltatype, iMax,
                                                aIndexentries, aAlphaentries,
                                                aUsedentries);
    if (iRetcode)
      return iRetcode;
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_C_SPECIALFUNC (mng_special_pplt)
{
  return MNG_NOERROR;                 
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
MNG_C_SPECIALFUNC (mng_special_ijng)
{
#ifdef MNG_SUPPORT_DISPLAY
  mng_retcode iRetcode = mng_create_ani_ijng (pData);
  if (!iRetcode)                       /* process it */
    iRetcode = mng_process_display_ijng (pData);
  return iRetcode;
#else
  return MNG_NOERROR;                  /* done */
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_F_SPECIALFUNC (mng_drop_entries)
{
  mng_dropp   pDROP    = (mng_dropp)pChunk;
  mng_uint32  iRawlen  = *piRawlen;
  mng_uint8p  pRawdata = *ppRawdata;
  mng_uint32  iX;
  mng_uint32p pEntry;
                                       /* check length */
  if ((iRawlen < 4) || ((iRawlen % 4) != 0))
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  MNG_ALLOC (pData, pEntry, iRawlen);
  pDROP->iCount      = iRawlen / 4;
  pDROP->pChunknames = (mng_ptr)pEntry;

  for (iX = pDROP->iCount; iX > 0; iX--)
  {
    *pEntry++ = mng_get_uint32 (pRawdata);
    pRawdata += 4;
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
MNG_C_SPECIALFUNC (mng_special_drop)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
MNG_C_SPECIALFUNC (mng_special_dbyk)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
MNG_F_SPECIALFUNC (mng_ordr_entries)
{
  mng_ordrp       pORDR    = (mng_ordrp)pChunk;
  mng_uint32      iRawlen  = *piRawlen;
  mng_uint8p      pRawdata = *ppRawdata;
  mng_uint32      iX;
  mng_ordr_entryp pEntry;
                                       /* check length */
  if ((iRawlen < 5) || ((iRawlen % 5) != 0))
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  MNG_ALLOC (pData, pEntry, iRawlen);
  pORDR->iCount   = iRawlen / 5;
  pORDR->pEntries = (mng_ptr)pEntry;

  for (iX = pORDR->iCount; iX > 0; iX--)
  {
    pEntry->iChunkname = mng_get_uint32 (pRawdata);
    pEntry->iOrdertype = *(pRawdata+4);
    pEntry++;
    pRawdata += 5;
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
MNG_C_SPECIALFUNC (mng_special_ordr)
{
#ifdef MNG_SUPPORT_DISPLAY
  {


    /* TODO: something !!! */


  }
#endif /* MNG_SUPPORT_DISPLAY */

  return MNG_NOERROR;                  /* done */
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
MNG_F_SPECIALFUNC (mng_debunk_magn)
{
  mng_magnp  pMAGN    = (mng_magnp)pChunk;
  mng_uint32 iRawlen  = *piRawlen;
  mng_uint8p pRawdata = *ppRawdata;
  mng_bool   bFaulty;
                                       /* check length */
  if (iRawlen > 20)
    MNG_ERROR (pData, MNG_INVALIDLENGTH);

  /* following is an ugly hack to allow faulty layout caused by previous
     versions of libmng and MNGeye, which wrote MAGN with a 16-bit
     MethodX/MethodY (as opposed to the proper 8-bit as defined in the spec!) */

  if ((iRawlen ==  6) || (iRawlen ==  8) || (iRawlen == 10) || (iRawlen == 12) ||
      (iRawlen == 14) || (iRawlen == 16) || (iRawlen == 20))
    bFaulty = MNG_TRUE;                /* these lengths are all wrong */
  else                                 /* length 18 can be right or wrong !!! */
  if ((iRawlen ==  18) && (mng_get_uint16 (pRawdata+4) <= 5) &&
      (mng_get_uint16 (pRawdata+6)  < 256) &&
      (mng_get_uint16 (pRawdata+8)  < 256) &&
      (mng_get_uint16 (pRawdata+10) < 256) &&
      (mng_get_uint16 (pRawdata+12) < 256) &&
      (mng_get_uint16 (pRawdata+14) < 256) &&
      (mng_get_uint16 (pRawdata+16) < 256))
    bFaulty = MNG_TRUE;                /* this is very likely the wrong layout */
  else
    bFaulty = MNG_FALSE;               /* all other cases are handled as right */

  if (bFaulty)                         /* wrong layout ? */
  {
    if (iRawlen > 0)                   /* get the fields */
      pMAGN->iFirstid = mng_get_uint16 (pRawdata);
    else
      pMAGN->iFirstid = 0;

    if (iRawlen > 2)
      pMAGN->iLastid  = mng_get_uint16 (pRawdata+2);
    else
      pMAGN->iLastid  = pMAGN->iFirstid;

    if (iRawlen > 4)
      pMAGN->iMethodX = (mng_uint8)(mng_get_uint16 (pRawdata+4));
    else
      pMAGN->iMethodX = 0;

    if (iRawlen > 6)
      pMAGN->iMX      = mng_get_uint16 (pRawdata+6);
    else
      pMAGN->iMX      = 1;

    if (iRawlen > 8)
      pMAGN->iMY      = mng_get_uint16 (pRawdata+8);
    else
      pMAGN->iMY      = pMAGN->iMX;

    if (iRawlen > 10)
      pMAGN->iML      = mng_get_uint16 (pRawdata+10);
    else
      pMAGN->iML      = pMAGN->iMX;

    if (iRawlen > 12)
      pMAGN->iMR      = mng_get_uint16 (pRawdata+12);
    else
      pMAGN->iMR      = pMAGN->iMX;

    if (iRawlen > 14)
      pMAGN->iMT      = mng_get_uint16 (pRawdata+14);
    else
      pMAGN->iMT      = pMAGN->iMY;

    if (iRawlen > 16)
      pMAGN->iMB      = mng_get_uint16 (pRawdata+16);
    else
      pMAGN->iMB      = pMAGN->iMY;

    if (iRawlen > 18)
      pMAGN->iMethodY = (mng_uint8)(mng_get_uint16 (pRawdata+18));
    else
      pMAGN->iMethodY = pMAGN->iMethodX;
  }
  else                                 /* proper layout !!!! */
  {
    if (iRawlen > 0)                   /* get the fields */
      pMAGN->iFirstid = mng_get_uint16 (pRawdata);
    else
      pMAGN->iFirstid = 0;

    if (iRawlen > 2)
      pMAGN->iLastid  = mng_get_uint16 (pRawdata+2);
    else
      pMAGN->iLastid  = pMAGN->iFirstid;

    if (iRawlen > 4)
      pMAGN->iMethodX = *(pRawdata+4);
    else
      pMAGN->iMethodX = 0;

    if (iRawlen > 5)
      pMAGN->iMX      = mng_get_uint16 (pRawdata+5);
    else
      pMAGN->iMX      = 1;

    if (iRawlen > 7)
      pMAGN->iMY      = mng_get_uint16 (pRawdata+7);
    else
      pMAGN->iMY      = pMAGN->iMX;

    if (iRawlen > 9)
      pMAGN->iML      = mng_get_uint16 (pRawdata+9);
    else
      pMAGN->iML      = pMAGN->iMX;

    if (iRawlen > 11)
      pMAGN->iMR      = mng_get_uint16 (pRawdata+11);
    else
      pMAGN->iMR      = pMAGN->iMX;

    if (iRawlen > 13)
      pMAGN->iMT      = mng_get_uint16 (pRawdata+13);
    else
      pMAGN->iMT      = pMAGN->iMY;

    if (iRawlen > 15)
      pMAGN->iMB      = mng_get_uint16 (pRawdata+15);
    else
      pMAGN->iMB      = pMAGN->iMY;

    if (iRawlen > 17)
      pMAGN->iMethodY = *(pRawdata+17);
    else
      pMAGN->iMethodY = pMAGN->iMethodX;
  }
                                       /* check field validity */
  if ((pMAGN->iMethodX > 5) || (pMAGN->iMethodY > 5))
    MNG_ERROR (pData, MNG_INVALIDMETHOD);

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
MNG_C_SPECIALFUNC (mng_special_magn)
{
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ani_magn (pData, pChunk);
#else
  return MNG_NOERROR;                  
#endif /* MNG_SUPPORT_DISPLAY */
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_evNT
MNG_F_SPECIALFUNC (mng_evnt_entries)
{
  mng_evntp       pEVNT = (mng_evntp)pChunk;
  mng_uint32      iRawlen;
  mng_uint8p      pRawdata;
#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_SUPPORT_DYNAMICMNG)
  mng_retcode     iRetcode;
#endif
  mng_uint8p      pNull;
  mng_uint8       iEventtype;
  mng_uint8       iMasktype;
  mng_int32       iLeft;
  mng_int32       iRight;
  mng_int32       iTop;
  mng_int32       iBottom;
  mng_uint16      iObjectid;
  mng_uint8       iIndex;
  mng_uint32      iNamesize;
  mng_uint32      iCount = 0;
  mng_evnt_entryp pEntry = MNG_NULL;
  mng_uint32      iX;

  for (iX = 0; iX < 2; iX++)
  {
    iRawlen  = *piRawlen;
    pRawdata = *ppRawdata;

    if (iX)                            /* second run ? */
    {
      MNG_ALLOC (pData, pEntry, (iCount * sizeof (mng_evnt_entry)));
      pEVNT->iCount   = iCount;
      pEVNT->pEntries = pEntry;
    }

    while (iRawlen)                    /* anything left ? */
    {
      if (iRawlen < 2)                 /* must have at least 2 bytes ! */
        MNG_ERROR (pData, MNG_INVALIDLENGTH);

      iEventtype = *pRawdata;          /* eventtype */
      if (iEventtype > 5)
        MNG_ERROR (pData, MNG_INVALIDEVENT);

      pRawdata++;

      iMasktype  = *pRawdata;          /* masktype */
      if (iMasktype > 5)
        MNG_ERROR (pData, MNG_INVALIDMASK);

      pRawdata++;
      iRawlen -= 2;

      iLeft     = 0;
      iRight    = 0;
      iTop      = 0;
      iBottom   = 0;
      iObjectid = 0;
      iIndex    = 0;

      switch (iMasktype)
      {
        case 1 :
          {
            if (iRawlen > 16)
            {
              iLeft     = mng_get_int32 (pRawdata);
              iRight    = mng_get_int32 (pRawdata+4);
              iTop      = mng_get_int32 (pRawdata+8);
              iBottom   = mng_get_int32 (pRawdata+12);
              pRawdata += 16;
              iRawlen -= 16;
            }
            else
              MNG_ERROR (pData, MNG_INVALIDLENGTH);
            break;
          }
        case 2 :
          {
            if (iRawlen > 2)
            {
              iObjectid = mng_get_uint16 (pRawdata);
              pRawdata += 2;
              iRawlen -= 2;
            }
            else
              MNG_ERROR (pData, MNG_INVALIDLENGTH);
            break;
          }
        case 3 :
          {
            if (iRawlen > 3)
            {
              iObjectid = mng_get_uint16 (pRawdata);
              iIndex    = *(pRawdata+2);
              pRawdata += 3;
              iRawlen -= 3;
            }
            else
              MNG_ERROR (pData, MNG_INVALIDLENGTH);
            break;
          }
        case 4 :
          {
            if (iRawlen > 18)
            {
              iLeft     = mng_get_int32 (pRawdata);
              iRight    = mng_get_int32 (pRawdata+4);
              iTop      = mng_get_int32 (pRawdata+8);
              iBottom   = mng_get_int32 (pRawdata+12);
              iObjectid = mng_get_uint16 (pRawdata+16);
              pRawdata += 18;
              iRawlen -= 18;
            }
            else
              MNG_ERROR (pData, MNG_INVALIDLENGTH);
            break;
          }
        case 5 :
          {
            if (iRawlen > 19)
            {
              iLeft     = mng_get_int32 (pRawdata);
              iRight    = mng_get_int32 (pRawdata+4);
              iTop      = mng_get_int32 (pRawdata+8);
              iBottom   = mng_get_int32 (pRawdata+12);
              iObjectid = mng_get_uint16 (pRawdata+16);
              iIndex    = *(pRawdata+18);
              pRawdata += 19;
              iRawlen -= 19;
            }
            else
              MNG_ERROR (pData, MNG_INVALIDLENGTH);
            break;
          }
      }

      pNull = pRawdata;                /* get the name length */
      while (*pNull)
        pNull++;

      if ((pNull - pRawdata) > (mng_int32)iRawlen)
      {
        iNamesize = iRawlen;           /* no null found; so end of evNT */
        iRawlen   = 0;
      }
      else
      {
        iNamesize = pNull - pRawdata;  /* should be another entry */
        iRawlen   = iRawlen - iNamesize - 1;

        if (!iRawlen)                  /* must not end with a null ! */
          MNG_ERROR (pData, MNG_ENDWITHNULL);
      }

      if (!iX)
      {
        iCount++;
      }
      else
      {
        pEntry->iEventtype       = iEventtype;
        pEntry->iMasktype        = iMasktype;
        pEntry->iLeft            = iLeft;
        pEntry->iRight           = iRight;
        pEntry->iTop             = iTop;
        pEntry->iBottom          = iBottom;
        pEntry->iObjectid        = iObjectid;
        pEntry->iIndex           = iIndex;
        pEntry->iSegmentnamesize = iNamesize;

        if (iNamesize)
        {
          MNG_ALLOC (pData, pEntry->zSegmentname, iNamesize+1);
          MNG_COPY (pEntry->zSegmentname, pRawdata, iNamesize);
        }

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_SUPPORT_DYNAMICMNG)
        iRetcode = mng_create_event (pData, (mng_ptr)pEntry);
        if (iRetcode)                    /* on error bail out */
          return iRetcode;
#endif

        pEntry++;
      }

      pRawdata = pRawdata + iNamesize + 1;
    }
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_evNT
MNG_C_SPECIALFUNC (mng_special_evnt)
{
  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
MNG_C_SPECIALFUNC (mng_special_mpng)
{
  if ((pData->eImagetype != mng_it_png) && (pData->eImagetype != mng_it_jng))
    MNG_ERROR (pData, MNG_CHUNKNOTALLOWED);
    
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_mpng_obj (pData, pChunk);
#else
  return MNG_NOERROR;
#endif
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
MNG_C_SPECIALFUNC (mng_special_ahdr)
{
#ifdef MNG_SUPPORT_DISPLAY
  return mng_create_ang_obj (pData, pChunk);
#else
  return MNG_NOERROR;
#endif
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
MNG_F_SPECIALFUNC (mng_adat_tiles)
{
  if ((pData->eImagetype != mng_it_ang) || (!pData->pANG))
    MNG_ERROR (pData, MNG_CHUNKNOTALLOWED);

  {
    mng_adatp      pADAT = (mng_adatp)pChunk;
    mng_ang_objp   pANG  = (mng_ang_objp)pData->pANG;
    mng_uint32     iRawlen  = *piRawlen;
    mng_uint8p     pRawdata = *ppRawdata;
    mng_retcode    iRetcode;
    mng_uint8p     pBuf;
    mng_uint32     iBufsize;
    mng_uint32     iRealsize;
    mng_uint8p     pTemp;
    mng_uint8p     pTemp2;
    mng_int32      iX;
    mng_int32      iSize;

#ifdef MNG_SUPPORT_DISPLAY
    mng_imagep     pImage;
    mng_int32      iTemplen;
    mng_uint8p     pSwap;

    mng_processobject pProcess;

    mng_uint32     iSavedatawidth;
    mng_uint32     iSavedataheight;

    mng_fptr       fSaveinitrowproc;
    mng_fptr       fSavestorerow;
    mng_fptr       fSaveprocessrow;
    mng_fptr       fSavedifferrow;
    mng_imagep     fSavestoreobj;
    mng_imagedatap fSavestorebuf;

#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
    png_imgtype    eSavepngimgtype;
#endif

    mng_uint8      iSaveinterlace;
    mng_int8       iSavepass;
    mng_int32      iSaverow;
    mng_int32      iSaverowinc;
    mng_int32      iSavecol;
    mng_int32      iSavecolinc;
    mng_int32      iSaverowsamples;
    mng_int32      iSavesamplemul;
    mng_int32      iSavesampleofs;
    mng_int32      iSavesamplediv;
    mng_int32      iSaverowsize;
    mng_int32      iSaverowmax;
    mng_int32      iSavefilterofs;
    mng_int32      iSavepixelofs;
    mng_uint32     iSavelevel0;
    mng_uint32     iSavelevel1;
    mng_uint32     iSavelevel2;
    mng_uint32     iSavelevel3;
    mng_uint8p     pSaveworkrow;
    mng_uint8p     pSaveprevrow;
    mng_uint8p     pSaverGBArow;
    mng_bool       bSaveisRGBA16;
    mng_bool       bSaveisOpaque;
    mng_int32      iSavefilterbpp;

    mng_int32      iSavedestl;
    mng_int32      iSavedestt;
    mng_int32      iSavedestr;
    mng_int32      iSavedestb;
    mng_int32      iSavesourcel;
    mng_int32      iSavesourcet;
    mng_int32      iSavesourcer;
    mng_int32      iSavesourceb;
#endif /* MNG_SUPPORT_DISPLAY */

    iRetcode = mng_inflate_buffer (pData, pRawdata, iRawlen,
                                   &pBuf, &iBufsize, &iRealsize);
    if (iRetcode)                      /* on error bail out */
    {                                  /* don't forget to drop the temp buffer */
      MNG_FREEX (pData, pBuf, iBufsize);
      return iRetcode;
    }
                                       /* get buffer for tile info in ADAT chunk */
    pADAT->iTilessize = pANG->iNumframes * sizeof(mng_adat_tile);
    MNG_ALLOCX (pData, pADAT->pTiles, pADAT->iTilessize);
    if (!pADAT->pTiles)
    {
      pADAT->iTilessize = 0;
      MNG_FREEX (pData, pBuf, iBufsize);
      MNG_ERROR (pData, MNG_OUTOFMEMORY);
    }

    pTemp  = pBuf;
    pTemp2 = (mng_uint8p)pADAT->pTiles;

    if (!pANG->iStillused)
      iSize = 12;
    else
      iSize = 13;

    for (iX = 0; iX < pANG->iNumframes; iX++)
    {
      MNG_COPY (pTemp2, pTemp, iSize);
      pTemp  += iSize;
      pTemp2 += sizeof(mng_adat_tile);
    }

#ifdef MNG_SUPPORT_DISPLAY
                                       /* get buffer for tile info in ANG object */
    pANG->iTilessize = pADAT->iTilessize;
    MNG_ALLOCX (pData, pANG->pTiles, pANG->iTilessize);
    if (!pANG->pTiles)
    {
      pANG->iTilessize = 0;
      MNG_FREEX (pData, pBuf, iBufsize);
      MNG_ERROR (pData, MNG_OUTOFMEMORY);
    }
                                       /* copy it from the ADAT object */
    MNG_COPY (pANG->pTiles, pADAT->pTiles, pANG->iTilessize);

                                       /* save IDAT work-parms */
    fSaveinitrowproc    = pData->fInitrowproc;
    fSavestorerow       = pData->fDisplayrow;
    fSaveprocessrow     = pData->fProcessrow;
    fSavedifferrow      = pData->fDifferrow;
    fSavestoreobj       = pData->pStoreobj;
    fSavestorebuf       = pData->pStorebuf;

#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
    eSavepngimgtype     = pData->ePng_imgtype;
#endif

    iSavedatawidth      = pData->iDatawidth;
    iSavedataheight     = pData->iDataheight;
    iSaveinterlace      = pData->iInterlace;
    iSavepass           = pData->iPass;
    iSaverow            = pData->iRow;
    iSaverowinc         = pData->iRowinc;
    iSavecol            = pData->iCol;
    iSavecolinc         = pData->iColinc;
    iSaverowsamples     = pData->iRowsamples;
    iSavesamplemul      = pData->iSamplemul;
    iSavesampleofs      = pData->iSampleofs;
    iSavesamplediv      = pData->iSamplediv;
    iSaverowsize        = pData->iRowsize;
    iSaverowmax         = pData->iRowmax;
    iSavefilterofs      = pData->iFilterofs;
    iSavepixelofs       = pData->iPixelofs;
    iSavelevel0         = pData->iLevel0;
    iSavelevel1         = pData->iLevel1;
    iSavelevel2         = pData->iLevel2;
    iSavelevel3         = pData->iLevel3;
    pSaveworkrow        = pData->pWorkrow;
    pSaveprevrow        = pData->pPrevrow;
    pSaverGBArow        = pData->pRGBArow;
    bSaveisRGBA16       = pData->bIsRGBA16;
    bSaveisOpaque       = pData->bIsOpaque;
    iSavefilterbpp      = pData->iFilterbpp;
    iSavedestl          = pData->iDestl;
    iSavedestt          = pData->iDestt;
    iSavedestr          = pData->iDestr;
    iSavedestb          = pData->iDestb;
    iSavesourcel        = pData->iSourcel;
    iSavesourcet        = pData->iSourcet;
    iSavesourcer        = pData->iSourcer;
    iSavesourceb        = pData->iSourceb;

    pData->iDatawidth   = pANG->iTilewidth;
    pData->iDataheight  = pANG->iTileheight;

    pData->iDestl       = 0;
    pData->iDestt       = 0;
    pData->iDestr       = pANG->iTilewidth;
    pData->iDestb       = pANG->iTileheight;
    pData->iSourcel     = 0;
    pData->iSourcet     = 0;
    pData->iSourcer     = pANG->iTilewidth;
    pData->iSourceb     = pANG->iTileheight;

    pData->fInitrowproc = MNG_NULL;
    pData->fStorerow    = MNG_NULL;
    pData->fProcessrow  = MNG_NULL;
    pData->fDifferrow   = MNG_NULL;

    /* clone image object to store the pixel-data from object 0 */
    iRetcode = mng_clone_imageobject (pData, 1, MNG_FALSE, MNG_FALSE, MNG_FALSE,
                                      MNG_FALSE, 0, 0, 0, pData->pObjzero, &pImage);
    if (iRetcode)                      /* on error, drop temp buffer and bail */
    {
      MNG_FREEX (pData, pBuf, iBufsize);
      return iRetcode;
    }

    /* make sure we got the right dimensions and interlacing */
    iRetcode = mng_reset_object_details (pData, pImage, pANG->iTilewidth, pANG->iTileheight,
                                         pImage->pImgbuf->iBitdepth, pImage->pImgbuf->iColortype,
                                         pImage->pImgbuf->iCompression, pImage->pImgbuf->iFilter,
                                         pANG->iInterlace, MNG_FALSE);
    if (iRetcode)                      /* on error, drop temp buffer and bail */
    {
      MNG_FREEX (pData, pBuf, iBufsize);
      return iRetcode;
    }

    pData->pStoreobj    = pImage;

#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
    pData->fInitrowproc = (mng_fptr)mng_init_rowproc;
    pData->ePng_imgtype = mng_png_imgtype(pData->iColortype,pData->iBitdepth);
#else
    switch (pData->iColortype)         /* determine row initialization routine */
    {
      case 0 : {                       /* gray */
                 switch (pData->iBitdepth)
                 {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
                   case  1 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_g1_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_g1_i;

                               break;
                             }
                   case  2 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_g2_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_g2_i;

                               break;
                             }
                   case  4 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_g4_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_g4_i;
                               break;
                             }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
                   case  8 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_g8_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_g8_i;

                               break;
                             }
#ifndef MNG_NO_16BIT_SUPPORT
                   case 16 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_g16_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_g16_i;

                               break;
                             }
#endif
                 }

                 break;
               }
      case 2 : {                       /* rgb */
                 switch (pData->iBitdepth)
                 {
                   case  8 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_rgb8_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_rgb8_i;
                               break;
                             }
#ifndef MNG_NO_16BIT_SUPPORT
                   case 16 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_rgb16_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_rgb16_i;

                               break;
                             }
#endif
                 }

                 break;
               }
      case 3 : {                       /* indexed */
                 switch (pData->iBitdepth)
                 {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
                   case  1 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_idx1_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_idx1_i;

                               break;
                             }
                   case  2 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_idx2_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_idx2_i;

                               break;
                             }
                   case  4 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_idx4_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_idx4_i;

                               break;
                             }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
                   case  8 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_idx8_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_idx8_i;

                               break;
                             }
                 }

                 break;
               }
      case 4 : {                       /* gray+alpha */
                 switch (pData->iBitdepth)
                 {
                   case  8 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_ga8_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_ga8_i;

                               break;
                             }
#ifndef MNG_NO_16BIT_SUPPORT
                   case 16 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_ga16_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_ga16_i;
                               break;
                             }
#endif
                 }

                 break;
               }
      case 6 : {                       /* rgb+alpha */
                 switch (pData->iBitdepth)
                 {
                   case  8 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_rgba8_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_rgba8_i;

                               break;
                             }
#ifndef MNG_NO_16BIT_SUPPORT
                   case 16 : {
                               if (!pData->iInterlace)
                                 pData->fInitrowproc = (mng_fptr)mng_init_rgba16_ni;
                               else
                                 pData->fInitrowproc = (mng_fptr)mng_init_rgba16_i;

                               break;
                             }
#endif
                 }

                 break;
               }
    }
#endif /* MNG_OPTIMIZE_FOOTPRINT_INIT */

    pData->iFilterofs = 0;             /* determine filter characteristics */
    pData->iLevel0    = 0;             /* default levels */
    pData->iLevel1    = 0;    
    pData->iLevel2    = 0;
    pData->iLevel3    = 0;

#ifdef FILTER192                       /* leveling & differing ? */
    if (pData->iFilter == MNG_FILTER_DIFFERING)
    {
      switch (pData->iColortype)
      {
        case 0 : {
                   if (pData->iBitdepth <= 8)
                     pData->iFilterofs = 1;
                   else
                     pData->iFilterofs = 2;

                   break;
                 }
        case 2 : {
                   if (pData->iBitdepth <= 8)
                     pData->iFilterofs = 3;
                   else
                     pData->iFilterofs = 6;

                   break;
                 }
        case 3 : {
                   pData->iFilterofs = 1;
                   break;
                 }
        case 4 : {
                   if (pData->iBitdepth <= 8)
                     pData->iFilterofs = 2;
                   else
                     pData->iFilterofs = 4;

                   break;
                 }
        case 6 : {
                   if (pData->iBitdepth <= 8)
                     pData->iFilterofs = 4;
                   else
                     pData->iFilterofs = 8;

                   break;
                 }
      }
    }
#endif

#ifdef FILTER193                       /* no adaptive filtering ? */
    if (pData->iFilter == MNG_FILTER_NOFILTER)
      pData->iPixelofs = pData->iFilterofs;
    else
#endif
      pData->iPixelofs = pData->iFilterofs + 1;

    if (pData->fInitrowproc)           /* need to initialize row processing? */
    {
      iRetcode = ((mng_initrowproc)pData->fInitrowproc) (pData);
      if (iRetcode)
      {
         MNG_FREEX (pData, pBuf, iBufsize);
         return iRetcode;
      }
    }
                                       /* calculate remainder of buffer */
    pTemp    = pBuf + (mng_int32)(pANG->iNumframes * iSize);
    iTemplen = iRealsize - (mng_int32)(pANG->iNumframes * iSize);

    do
    {
      if (iTemplen > pData->iRowmax)   /* get a pixel-row from the temp buffer */
      {
        MNG_COPY (pData->pWorkrow, pTemp, pData->iRowmax);
      }
      else
      {
        MNG_COPY (pData->pWorkrow, pTemp, iTemplen);
      }

      {                                /* image not completed yet ? */
        if (pData->iRow < (mng_int32)pData->iDataheight)
        {
#ifdef MNG_NO_1_2_4BIT_SUPPORT
          if (pData->iPNGdepth == 1)
          {
            /* Inflate Workrow to 8-bit */
            mng_int32  iX;
            mng_uint8p pSrc = pData->pWorkrow+1;
            mng_uint8p pDest = pSrc + pData->iRowsize - (pData->iRowsize+7)/8;

            for (iX = ((pData->iRowsize+7)/8) ; iX > 0 ; iX--)
              *pDest++ = *pSrc++;

            pDest = pData->pWorkrow+1;
            pSrc = pDest + pData->iRowsize - (pData->iRowsize+7)/8;
            for (iX = pData->iRowsize; ;)
            {
              *pDest++ = (((*pSrc)>>7)&1);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)>>6)&1);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)>>5)&1);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)>>4)&1);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)>>3)&1);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)>>2)&1);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)>>1)&1);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)   )&1);
              if (iX-- <= 0)
                break;
              pSrc++;
            }
          }
          else if (pData->iPNGdepth == 2)
          {
            /* Inflate Workrow to 8-bit */
            mng_int32  iX;
            mng_uint8p pSrc = pData->pWorkrow+1;
            mng_uint8p pDest = pSrc + pData->iRowsize - (2*pData->iRowsize+7)/8;

            for (iX = ((2*pData->iRowsize+7)/8) ; iX > 0 ; iX--)
               *pDest++ = *pSrc++;

            pDest = pData->pWorkrow+1;
            pSrc = pDest + pData->iRowsize - (2*pData->iRowsize+7)/8;
            for (iX = pData->iRowsize; ;)
            {
              *pDest++ = (((*pSrc)>>6)&3);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)>>4)&3);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)>>2)&3);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)   )&3);
              if (iX-- <= 0)
                break;
              pSrc++;
            }
          }
          else if (pData->iPNGdepth == 4)
          {
            /* Inflate Workrow to 8-bit */
            mng_int32  iX;
            mng_uint8p pSrc = pData->pWorkrow+1;
            mng_uint8p pDest = pSrc + pData->iRowsize - (4*pData->iRowsize+7)/8;

            for (iX = ((4*pData->iRowsize+7)/8) ; iX > 0 ; iX--)
               *pDest++ = *pSrc++;

            pDest = pData->pWorkrow+1;
            pSrc = pDest + pData->iRowsize - (4*pData->iRowsize+7)/8;
            for (iX = pData->iRowsize; ;)
            {
              *pDest++ = (((*pSrc)>>4)&0x0f);
              if (iX-- <= 0)
                break;
              *pDest++ = (((*pSrc)   )&0x0f);
              if (iX-- <= 0)
                break;
              pSrc++;
            }
          }
          if (pData->iPNGdepth < 8 && pData->iColortype == 0)
          {
            /* Expand samples to 8-bit by LBR */
            mng_int32  iX;
            mng_uint8p pSrc = pData->pWorkrow+1;
            mng_uint8 multiplier[]={0,255,85,0,17,0,0,0,1};

            for (iX = pData->iRowsize; iX > 0; iX--)
                *pSrc++ *= multiplier[pData->iPNGdepth];
          }
#endif
#ifdef MNG_NO_16BIT_SUPPORT
          if (pData->iPNGdepth > 8)
          {
            /* Reduce Workrow to 8-bit */
            mng_int32  iX;
            mng_uint8p pSrc = pData->pWorkrow+1;
            mng_uint8p pDest = pSrc;

            for (iX = pData->iRowsize; iX > 0; iX--)
            {
              *pDest = *pSrc;
              pDest++;
              pSrc+=2;
            }
          }
#endif

#ifdef FILTER192                       /* has leveling info ? */
          if (pData->iFilterofs == MNG_FILTER_DIFFERING)
            iRetcode = init_rowdiffering (pData);
          else
#endif
            iRetcode = MNG_NOERROR;
                                       /* filter the row if necessary */
          if ((!iRetcode) && (pData->iFilterofs < pData->iPixelofs  ) &&
                             (*(pData->pWorkrow + pData->iFilterofs))    )
            iRetcode = mng_filter_a_row (pData);

                                       /* additional leveling/differing ? */
          if ((!iRetcode) && (pData->fDifferrow))
          {
            iRetcode = ((mng_differrow)pData->fDifferrow) (pData);

            pSwap           = pData->pWorkrow;
            pData->pWorkrow = pData->pPrevrow;
            pData->pPrevrow = pSwap;   /* make sure we're processing the right data */
          }

          if (!iRetcode)
          {
            {                          /* process this row */
              if ((!iRetcode) && (pData->fProcessrow))
                iRetcode = ((mng_processrow)pData->fProcessrow) (pData);
                                       /* store in object ? */
              if ((!iRetcode) && (pData->fStorerow))
                iRetcode = ((mng_storerow)pData->fStorerow)     (pData);
            }
          }

          if (iRetcode)                   /* on error bail out */
          {
            MNG_FREEX (pData, pBuf, iBufsize);
            MNG_ERROR (pData, iRetcode);
          }

          if (!pData->fDifferrow)      /* swap row-pointers */
          {
            pSwap           = pData->pWorkrow;
            pData->pWorkrow = pData->pPrevrow;
            pData->pPrevrow = pSwap;   /* so prev points to the processed row! */
          }
                                       /* adjust variables for next row */
          iRetcode = mng_next_row (pData);

          if (iRetcode)                   /* on error bail out */
          {
            MNG_FREEX (pData, pBuf, iBufsize);
            MNG_ERROR (pData, iRetcode);
          }
        }
      }

      pTemp    += pData->iRowmax;
      iTemplen -= pData->iRowmax;
    }                                  /* until some error or EOI
                                          or all pixels received */
    while ( (iTemplen > 0)  &&
            ( (pData->iRow < (mng_int32)pData->iDataheight) ||
              ( (pData->iPass >= 0) && (pData->iPass < 7) )    )    );

    mng_cleanup_rowproc (pData);       /* cleanup row processing buffers !! */

                                       /* restore saved work-parms */
    pData->iDatawidth   = iSavedatawidth;
    pData->iDataheight  = iSavedataheight;

    pData->fInitrowproc = fSaveinitrowproc;
    pData->fDisplayrow  = fSavestorerow;
    pData->fProcessrow  = fSaveprocessrow;
    pData->fDifferrow   = fSavedifferrow;
    pData->pStoreobj    = fSavestoreobj;
    pData->pStorebuf    = fSavestorebuf;

#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
    pData->ePng_imgtype = eSavepngimgtype;
#endif

    pData->iInterlace   = iSaveinterlace;
    pData->iPass        = iSavepass;
    pData->iRow         = iSaverow;
    pData->iRowinc      = iSaverowinc;
    pData->iCol         = iSavecol;
    pData->iColinc      = iSavecolinc;
    pData->iRowsamples  = iSaverowsamples;
    pData->iSamplemul   = iSavesamplemul;
    pData->iSampleofs   = iSavesampleofs;
    pData->iSamplediv   = iSavesamplediv;
    pData->iRowsize     = iSaverowsize;
    pData->iRowmax      = iSaverowmax;
    pData->iFilterofs   = iSavefilterofs;
    pData->iPixelofs    = iSavepixelofs;
    pData->iLevel0      = iSavelevel0;
    pData->iLevel1      = iSavelevel1;
    pData->iLevel2      = iSavelevel2;
    pData->iLevel3      = iSavelevel3;
    pData->pWorkrow     = pSaveworkrow;
    pData->pPrevrow     = pSaveprevrow;
    pData->pRGBArow     = pSaverGBArow;
    pData->bIsRGBA16    = bSaveisRGBA16;
    pData->bIsOpaque    = bSaveisOpaque;
    pData->iFilterbpp   = iSavefilterbpp;
    pData->iDestl       = iSavedestl;
    pData->iDestt       = iSavedestt;
    pData->iDestr       = iSavedestr;
    pData->iDestb       = iSavedestb;
    pData->iSourcel     = iSavesourcel;
    pData->iSourcet     = iSavesourcet;
    pData->iSourcer     = iSavesourcer;
    pData->iSourceb     = iSavesourceb;

                                       /* create the animation directives ! */
    pProcess = (mng_processobject)pANG->sHeader.fProcess;
    iRetcode = pProcess (pData, (mng_objectp)pData->pANG);
    if (iRetcode)
      return iRetcode;

#endif /* MNG_SUPPORT_DISPLAY */

    MNG_FREE (pData, pBuf, iBufsize);  /* always free the temp buffer ! */
  }

  *piRawlen = 0;

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
MNG_C_SPECIALFUNC (mng_special_adat)
{
  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

MNG_C_SPECIALFUNC (mng_special_unknown)
{
                                       /* critical chunk ? */
  if ((((mng_uint32)pData->iChunkname & 0x20000000) == 0)
#ifdef MNG_SKIPCHUNK_SAVE
    && (pData->iChunkname != MNG_UINT_SAVE)
#endif
#ifdef MNG_SKIPCHUNK_SEEK
    && (pData->iChunkname != MNG_UINT_SEEK)
#endif
#ifdef MNG_SKIPCHUNK_DBYK
    && (pData->iChunkname != MNG_UINT_DBYK)
#endif
#ifdef MNG_SKIPCHUNK_ORDR
    && (pData->iChunkname != MNG_UINT_ORDR)
#endif
      )
    MNG_ERROR (pData, MNG_UNKNOWNCRITICAL);

  if (pData->fProcessunknown)          /* let the app handle it ? */
  {
    mng_bool bOke = pData->fProcessunknown ((mng_handle)pData, pData->iChunkname,
                                            ((mng_unknown_chunkp)pChunk)->iDatasize,
                                            ((mng_unknown_chunkp)pChunk)->pData);
    if (!bOke)
      MNG_ERROR (pData, MNG_APPMISCERROR);
  }

  return MNG_NOERROR;                  /* done */
}

/* ************************************************************************** */

#endif /* MNG_INCLUDE_READ_PROCS || MNG_INCLUDE_WRITE_PROCS */
#endif /* MNG_OPTIMIZE_CHUNKREADER */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */





