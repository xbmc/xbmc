/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_chunk_prc.c        copyright (c) 2000-2005 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Chunk initialization & cleanup (implementation)            * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the chunk initialization & cleanup       * */
/* *             routines                                                   * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *                                                                        * */
/* *             0.9.1 - 07/19/2000 - G.Juyn                                * */
/* *             - fixed creation-code                                      * */
/* *                                                                        * */
/* *             0.9.2 - 07/31/2000 - G.Juyn                                * */
/* *             - put add_chunk() inside MNG_INCLUDE_WRITE_PROCS wrapper   * */
/* *             0.9.2 - 08/01/2000 - G.Juyn                                * */
/* *             - wrapper for add_chunk() changed                          * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 10/16/2000 - G.Juyn                                * */
/* *             - added support for JDAA                                   * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             - added HLAPI function to copy chunks                      * */
/* *             1.0.5 - 09/14/2002 - G.Juyn                                * */
/* *             - added event handling for dynamic MNG                     * */
/* *             1.0.5 - 10/04/2002 - G.Juyn                                * */
/* *             - fixed chunk-storage for evNT chunk                       * */
/* *             1.0.5 - 10/17/2002 - G.Juyn                                * */
/* *             - fixed issue in freeing evNT chunk                        * */
/* *                                                                        * */
/* *             1.0.6 - 07/07/2003 - G.R-P                                 * */
/* *             - added MNG_SKIPCHUNK_cHNK footprint optimizations         * */
/* *             - added MNG_NO_DELTA_PNG reduction feature                 * */
/* *             1.0.6 - 07/14/2003 - G.R-P                                 * */
/* *             - added MNG_NO_LOOP_SIGNALS_SUPPORTED conditional          * */
/* *             1.0.6 - 07/29/2003 - G.R-P                                 * */
/* *             - added conditionals around PAST chunk support             * */
/* *             1.0.6 - 08/17/2003 - G.R-P                                 * */
/* *             - added conditionals around non-VLC chunk support          * */
/* *                                                                        * */
/* *             1.0.7 - 03/24/2004 - G.R-P                                 * */
/* *             - fixed SKIPCHUNK_eXPI -> fPRI typo                        * */
/* *                                                                        * */
/* *             1.0.9 - 09/25/2004 - G.Juyn                                * */
/* *             - replaced MNG_TWEAK_LARGE_FILES with permanent solution   * */
/* *             1.0.9 - 12/05/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_CHUNKINITFREE             * */
/* *             1.0.9 - 12/06/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_CHUNKASSIGN               * */
/* *             1.0.9 - 12/20/2004 - G.Juyn                                * */
/* *             - cleaned up macro-invocations (thanks to D. Airlie)       * */
/* *                                                                        * */
/* *             1.0.10 - 07/30/2005 - G.Juyn                               * */
/* *             - fixed problem with CLON object during readdisplay()      * */
/* *             1.0.10 - 04/08/2007 - G.Juyn                               * */
/* *             - added support for mPNG proposal                          * */
/* *             1.0.10 - 04/12/2007 - G.Juyn                               * */
/* *             - added support for ANG proposal                           * */
/* *                                                                        * */
/* ************************************************************************** */

#include "libmng.h"
#include "libmng_data.h"
#include "libmng_error.h"
#include "libmng_trace.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#include "libmng_memory.h"
#include "libmng_chunks.h"
#include "libmng_chunk_prc.h"

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */
/* *                                                                        * */
/* * General chunk routines                                                 * */
/* *                                                                        * */
/* ************************************************************************** */

void mng_add_chunk (mng_datap  pData,
                    mng_chunkp pChunk)
{
  if (!pData->pFirstchunk)             /* list is still empty ? */
  {
    pData->pFirstchunk      = pChunk;  /* then this becomes the first */
    
#ifdef MNG_SUPPORT_WRITE
    if (!pData->iFirstchunkadded)
    {
      pData->iFirstchunkadded = ((mng_chunk_headerp)pChunk)->iChunkname;
#endif

      if (((mng_chunk_headerp)pChunk)->iChunkname == MNG_UINT_IHDR)
        pData->eImagetype     = mng_it_png;
      else
#ifdef MNG_INCLUDE_JNG
      if (((mng_chunk_headerp)pChunk)->iChunkname == MNG_UINT_JHDR)
        pData->eImagetype     = mng_it_jng;
      else
#endif
        pData->eImagetype     = mng_it_mng;

      pData->eSigtype         = pData->eImagetype;
#ifdef MNG_SUPPORT_WRITE
    }
#endif
  }
  else
  {                                    /* else we make appropriate links */
    ((mng_chunk_headerp)pChunk)->pPrev = pData->pLastchunk;
    ((mng_chunk_headerp)pData->pLastchunk)->pNext = pChunk;
  }

  pData->pLastchunk = pChunk;          /* and it's always the last */

  return;
}

/* ************************************************************************** */
/* *                                                                        * */
/* * Chunk specific initialization routines                                 * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_OPTIMIZE_CHUNKINITFREE
INIT_CHUNK_HDR (mng_init_general)
{
  MNG_ALLOC (pData, *ppChunk, ((mng_chunk_headerp)pHeader)->iChunksize);
  MNG_COPY (*ppChunk, pHeader, sizeof (mng_chunk_header));
  return MNG_NOERROR;
}

#else /* MNG_OPTIMIZE_CHUNKINITFREE */

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_ihdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IHDR, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_ihdr));
  ((mng_ihdrp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_plte)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PLTE, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_plte));
  ((mng_pltep)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PLTE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_idat)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IDAT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_idat));
  ((mng_idatp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_iend)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IEND, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_iend));
  ((mng_iendp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_trns)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_TRNS, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_trns));
  ((mng_trnsp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_TRNS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_gAMA
INIT_CHUNK_HDR (mng_init_gama)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_GAMA, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_gama));
  ((mng_gamap)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_GAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_cHRM
INIT_CHUNK_HDR (mng_init_chrm)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_CHRM, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_chrm));
  ((mng_chrmp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_CHRM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sRGB
INIT_CHUNK_HDR (mng_init_srgb)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SRGB, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_srgb));
  ((mng_srgbp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
INIT_CHUNK_HDR (mng_init_iccp)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ICCP, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_iccp));
  ((mng_iccpp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ICCP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tEXt
INIT_CHUNK_HDR (mng_init_text)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_TEXT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_text));
  ((mng_textp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_TEXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_zTXt
INIT_CHUNK_HDR (mng_init_ztxt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ZTXT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_ztxt));
  ((mng_ztxtp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ZTXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iTXt
INIT_CHUNK_HDR (mng_init_itxt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ITXT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_itxt));
  ((mng_itxtp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ITXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_bKGD
INIT_CHUNK_HDR (mng_init_bkgd)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_BKGD, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_bkgd));
  ((mng_bkgdp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_BKGD, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYs
INIT_CHUNK_HDR (mng_init_phys)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PHYS, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_phys));
  ((mng_physp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PHYS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sBIT
INIT_CHUNK_HDR (mng_init_sbit)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SBIT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_sbit));
  ((mng_sbitp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SBIT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
INIT_CHUNK_HDR (mng_init_splt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SPLT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_splt));
  ((mng_spltp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_hIST
INIT_CHUNK_HDR (mng_init_hist)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_HIST, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_hist));
  ((mng_histp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_HIST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tIME
INIT_CHUNK_HDR (mng_init_time)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_TIME, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_time));
  ((mng_timep)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_TIME, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_mhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_MHDR, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_mhdr));
  ((mng_mhdrp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_MHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_mend)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_MEND, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_mend));
  ((mng_mendp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_MEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
INIT_CHUNK_HDR (mng_init_loop)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_LOOP, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_loop));
  ((mng_loopp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_LOOP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_endl)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ENDL, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_endl));
  ((mng_endlp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ENDL, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DEFI
INIT_CHUNK_HDR (mng_init_defi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DEFI, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_defi));
  ((mng_defip)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BASI
INIT_CHUNK_HDR (mng_init_basi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_BASI, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_basi));
  ((mng_basip)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_BASI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLON
INIT_CHUNK_HDR (mng_init_clon)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_CLON, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_clon));
  ((mng_clonp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_CLON, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
INIT_CHUNK_HDR (mng_init_past)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PAST, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_past));
  ((mng_pastp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PAST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
INIT_CHUNK_HDR (mng_init_disc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DISC, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_disc));
  ((mng_discp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DISC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BACK
INIT_CHUNK_HDR (mng_init_back)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_BACK, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_back));
  ((mng_backp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_BACK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
INIT_CHUNK_HDR (mng_init_fram)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_FRAM, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_fram));
  ((mng_framp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_FRAM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MOVE
INIT_CHUNK_HDR (mng_init_move)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_MOVE, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_move));
  ((mng_movep)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_MOVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLIP
INIT_CHUNK_HDR (mng_init_clip)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_CLIP, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_clip));
  ((mng_clipp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_CLIP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SHOW
INIT_CHUNK_HDR (mng_init_show)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SHOW, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_show));
  ((mng_showp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SHOW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_TERM
INIT_CHUNK_HDR (mng_init_term)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_TERM, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_term));
  ((mng_termp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_TERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
INIT_CHUNK_HDR (mng_init_save)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SAVE, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_save));
  ((mng_savep)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;

}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
INIT_CHUNK_HDR (mng_init_seek)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SEEK, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_seek));
  ((mng_seekp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_SEEK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_eXPI
INIT_CHUNK_HDR (mng_init_expi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_EXPI, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_expi));
  ((mng_expip)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_EXPI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_fPRI
INIT_CHUNK_HDR (mng_init_fpri)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_FPRI, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_fpri));
  ((mng_fprip)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_FPRI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
INIT_CHUNK_HDR (mng_init_need)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_NEED, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_need));
  ((mng_needp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_NEED, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYg
INIT_CHUNK_HDR (mng_init_phyg)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PHYG, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_phyg));
  ((mng_phygp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PHYG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
INIT_CHUNK_HDR (mng_init_jhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_JHDR, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_jhdr));
  ((mng_jhdrp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_JHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
INIT_CHUNK_HDR (mng_init_jdaa)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_JDAA, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_jdaa));
  ((mng_jdaap)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_JDAA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
INIT_CHUNK_HDR (mng_init_jdat)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_JDAT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_jdat));
  ((mng_jdatp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_JDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
INIT_CHUNK_HDR (mng_init_jsep)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_JSEP, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_jsep));
  ((mng_jsepp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_JSEP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
INIT_CHUNK_HDR (mng_init_dhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DHDR, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_dhdr));
  ((mng_dhdrp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
INIT_CHUNK_HDR (mng_init_prom)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PROM, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_prom));
  ((mng_promp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PROM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
INIT_CHUNK_HDR (mng_init_ipng)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IPNG, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_ipng));
  ((mng_ipngp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
INIT_CHUNK_HDR (mng_init_pplt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PPLT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_pplt));
  ((mng_ppltp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_PPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
INIT_CHUNK_HDR (mng_init_ijng)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IJNG, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_ijng));
  ((mng_ijngp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_IJNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
INIT_CHUNK_HDR (mng_init_drop)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DROP, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_drop));
  ((mng_dropp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DROP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif


/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
INIT_CHUNK_HDR (mng_init_dbyk)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DBYK, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_dbyk));
  ((mng_dbykp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_DBYK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
INIT_CHUNK_HDR (mng_init_ordr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ORDR, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_ordr));
  ((mng_ordrp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ORDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
INIT_CHUNK_HDR (mng_init_magn)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_MAGN, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_magn));
  ((mng_magnp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_MAGN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_evNT
INIT_CHUNK_HDR (mng_init_evnt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_EVNT, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_evnt));
  ((mng_evntp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_EVNT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

INIT_CHUNK_HDR (mng_init_unknown)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_UNKNOWN, MNG_LC_START);
#endif

  MNG_ALLOC (pData, *ppChunk, sizeof (mng_unknown_chunk));
  ((mng_unknown_chunkp)*ppChunk)->sHeader = *((mng_chunk_headerp)pHeader);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_UNKNOWN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_OPTIMIZE_CHUNKINITFREE */

/* ************************************************************************** */
/* *                                                                        * */
/* * Chunk specific cleanup routines                                        * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_OPTIMIZE_CHUNKINITFREE
FREE_CHUNK_HDR (mng_free_general)
{
  MNG_FREEX (pData, pHeader, ((mng_chunk_headerp)pHeader)->iChunksize);
  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
FREE_CHUNK_HDR (mng_free_ihdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IHDR, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_ihdr));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
FREE_CHUNK_HDR (mng_free_plte)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PLTE, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_plte));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PLTE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

FREE_CHUNK_HDR (mng_free_idat)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IDAT, MNG_LC_START);
#endif

  if (((mng_idatp)pHeader)->iDatasize)
    MNG_FREEX (pData, ((mng_idatp)pHeader)->pData,
                      ((mng_idatp)pHeader)->iDatasize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_idat));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IDAT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
FREE_CHUNK_HDR (mng_free_iend)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IEND, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_iend));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
FREE_CHUNK_HDR (mng_free_trns)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_TRNS, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_trns));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_TRNS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_gAMA
FREE_CHUNK_HDR (mng_free_gama)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_GAMA, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_gama));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_GAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_cHRM
FREE_CHUNK_HDR (mng_free_chrm)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_CHRM, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_chrm));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_CHRM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_sRGB
FREE_CHUNK_HDR (mng_free_srgb)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SRGB, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_srgb));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
FREE_CHUNK_HDR (mng_free_iccp)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ICCP, MNG_LC_START);
#endif

  if (((mng_iccpp)pHeader)->iNamesize)
    MNG_FREEX (pData, ((mng_iccpp)pHeader)->zName,
                      ((mng_iccpp)pHeader)->iNamesize + 1);

  if (((mng_iccpp)pHeader)->iProfilesize)
    MNG_FREEX (pData, ((mng_iccpp)pHeader)->pProfile,
                      ((mng_iccpp)pHeader)->iProfilesize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_iccp));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ICCP, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tEXt
FREE_CHUNK_HDR (mng_free_text)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_TEXT, MNG_LC_START);
#endif

  if (((mng_textp)pHeader)->iKeywordsize)
    MNG_FREEX (pData, ((mng_textp)pHeader)->zKeyword,
                      ((mng_textp)pHeader)->iKeywordsize + 1);

  if (((mng_textp)pHeader)->iTextsize)
    MNG_FREEX (pData, ((mng_textp)pHeader)->zText,
                      ((mng_textp)pHeader)->iTextsize + 1);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_text));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_TEXT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_zTXt
FREE_CHUNK_HDR (mng_free_ztxt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ZTXT, MNG_LC_START);
#endif

  if (((mng_ztxtp)pHeader)->iKeywordsize)
    MNG_FREEX (pData, ((mng_ztxtp)pHeader)->zKeyword,
                      ((mng_ztxtp)pHeader)->iKeywordsize + 1);

  if (((mng_ztxtp)pHeader)->iTextsize)
    MNG_FREEX (pData, ((mng_ztxtp)pHeader)->zText,
                      ((mng_ztxtp)pHeader)->iTextsize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_ztxt));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ZTXT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */
#ifndef MNG_SKIPCHUNK_iTXt
FREE_CHUNK_HDR (mng_free_itxt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ITXT, MNG_LC_START);
#endif

  if (((mng_itxtp)pHeader)->iKeywordsize)
    MNG_FREEX (pData, ((mng_itxtp)pHeader)->zKeyword,
                      ((mng_itxtp)pHeader)->iKeywordsize + 1);

  if (((mng_itxtp)pHeader)->iLanguagesize)
    MNG_FREEX (pData, ((mng_itxtp)pHeader)->zLanguage,
                      ((mng_itxtp)pHeader)->iLanguagesize + 1);

  if (((mng_itxtp)pHeader)->iTranslationsize)
    MNG_FREEX (pData, ((mng_itxtp)pHeader)->zTranslation,
                      ((mng_itxtp)pHeader)->iTranslationsize + 1);

  if (((mng_itxtp)pHeader)->iTextsize)
    MNG_FREEX (pData, ((mng_itxtp)pHeader)->zText,
                      ((mng_itxtp)pHeader)->iTextsize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_itxt));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ITXT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
FREE_CHUNK_HDR (mng_free_mpng)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MPNG, MNG_LC_START);
#endif

  if (((mng_mpngp)pHeader)->iFramessize)
    MNG_FREEX (pData, ((mng_mpngp)pHeader)->pFrames,
                      ((mng_mpngp)pHeader)->iFramessize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_mpng));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MPNG, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */
#ifdef MNG_INCLUDE_ANG_PROPOSAL
FREE_CHUNK_HDR (mng_free_adat)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ADAT, MNG_LC_START);
#endif

  if (((mng_adatp)pHeader)->iTilessize)
    MNG_FREEX (pData, ((mng_adatp)pHeader)->pTiles, ((mng_adatp)pHeader)->iTilessize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_adat));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ADAT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_bKGD
FREE_CHUNK_HDR (mng_free_bkgd)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_BKGD, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_bkgd));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_BKGD, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_pHYs
FREE_CHUNK_HDR (mng_free_phys)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PHYS, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_phys));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PHYS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_sBIT
FREE_CHUNK_HDR (mng_free_sbit)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SBIT, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_sbit));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SBIT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
FREE_CHUNK_HDR (mng_free_splt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SPLT, MNG_LC_START);
#endif

  if (((mng_spltp)pHeader)->iNamesize)
    MNG_FREEX (pData, ((mng_spltp)pHeader)->zName,
                      ((mng_spltp)pHeader)->iNamesize + 1);

  if (((mng_spltp)pHeader)->iEntrycount)
    MNG_FREEX (pData, ((mng_spltp)pHeader)->pEntries,
                      ((mng_spltp)pHeader)->iEntrycount *
                      (((mng_spltp)pHeader)->iSampledepth * 3 + sizeof (mng_uint16)) );

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_splt));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SPLT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_hIST
FREE_CHUNK_HDR (mng_free_hist)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_HIST, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_hist));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_HIST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_tIME
FREE_CHUNK_HDR (mng_free_time)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_TIME, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_time));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_TIME, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
FREE_CHUNK_HDR (mng_free_mhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MHDR, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_mhdr));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
FREE_CHUNK_HDR (mng_free_mend)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MEND, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_mend));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
FREE_CHUNK_HDR (mng_free_loop)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_LOOP, MNG_LC_START);
#endif

#ifndef MNG_NO_LOOP_SIGNALS_SUPPORTED
  if (((mng_loopp)pHeader)->iCount)
    MNG_FREEX (pData, ((mng_loopp)pHeader)->pSignals,
                      ((mng_loopp)pHeader)->iCount * sizeof (mng_uint32) );
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_loop));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_LOOP, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
FREE_CHUNK_HDR (mng_free_endl)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ENDL, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_endl));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ENDL, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_DEFI
FREE_CHUNK_HDR (mng_free_defi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DEFI, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_defi));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_BASI
FREE_CHUNK_HDR (mng_free_basi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_BASI, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_basi));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_BASI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_CLON
FREE_CHUNK_HDR (mng_free_clon)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_CLON, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_clon));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_CLON, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
FREE_CHUNK_HDR (mng_free_past)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PAST, MNG_LC_START);
#endif

  if (((mng_pastp)pHeader)->iCount)
    MNG_FREEX (pData, ((mng_pastp)pHeader)->pSources,
                      ((mng_pastp)pHeader)->iCount * sizeof (mng_past_source) );

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_past));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PAST, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
FREE_CHUNK_HDR (mng_free_disc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DISC, MNG_LC_START);
#endif

  if (((mng_discp)pHeader)->iCount)
    MNG_FREEX (pData, ((mng_discp)pHeader)->pObjectids,
                      ((mng_discp)pHeader)->iCount * sizeof (mng_uint16) );

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_disc));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DISC, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_BACK
FREE_CHUNK_HDR (mng_free_back)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_BACK, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_back));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_BACK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
FREE_CHUNK_HDR (mng_free_fram)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_FRAM, MNG_LC_START);
#endif

  if (((mng_framp)pHeader)->iNamesize)
    MNG_FREEX (pData, ((mng_framp)pHeader)->zName,
                      ((mng_framp)pHeader)->iNamesize + 1);

  if (((mng_framp)pHeader)->iCount)
    MNG_FREEX (pData, ((mng_framp)pHeader)->pSyncids,
                      ((mng_framp)pHeader)->iCount * sizeof (mng_uint32) );

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_fram));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_FRAM, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_MOVE
FREE_CHUNK_HDR (mng_free_move)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MOVE, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_move));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MOVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_CLIP
FREE_CHUNK_HDR (mng_free_clip)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_CLIP, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_clip));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_CLIP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_SHOW
FREE_CHUNK_HDR (mng_free_show)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SHOW, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_show));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SHOW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_TERM
FREE_CHUNK_HDR (mng_free_term)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_TERM, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_term));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_TERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
FREE_CHUNK_HDR (mng_free_save)
{
  mng_save_entryp pEntry = ((mng_savep)pHeader)->pEntries;
  mng_uint32      iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SAVE, MNG_LC_START);
#endif

  for (iX = 0; iX < ((mng_savep)pHeader)->iCount; iX++)
  {
    if (pEntry->iNamesize)
      MNG_FREEX (pData, pEntry->zName, pEntry->iNamesize);

    pEntry = pEntry + sizeof (mng_save_entry);
  }

  if (((mng_savep)pHeader)->iCount)
    MNG_FREEX (pData, ((mng_savep)pHeader)->pEntries,
                      ((mng_savep)pHeader)->iCount * sizeof (mng_save_entry) );

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_save));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SAVE, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
FREE_CHUNK_HDR (mng_free_seek)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SEEK, MNG_LC_START);
#endif

  if (((mng_seekp)pHeader)->iNamesize)
    MNG_FREEX (pData, ((mng_seekp)pHeader)->zName,
                      ((mng_seekp)pHeader)->iNamesize + 1);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_seek));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_SEEK, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_eXPI
FREE_CHUNK_HDR (mng_free_expi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_EXPI, MNG_LC_START);
#endif

  if (((mng_expip)pHeader)->iNamesize)
    MNG_FREEX (pData, ((mng_expip)pHeader)->zName,
                      ((mng_expip)pHeader)->iNamesize + 1);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_expi));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_EXPI, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_fPRI
FREE_CHUNK_HDR (mng_free_fpri)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_FPRI, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_fpri));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_FPRI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
FREE_CHUNK_HDR (mng_free_need)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_NEED, MNG_LC_START);
#endif

  if (((mng_needp)pHeader)->iKeywordssize)
    MNG_FREEX (pData, ((mng_needp)pHeader)->zKeywords,
                      ((mng_needp)pHeader)->iKeywordssize + 1);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_need));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_NEED, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_pHYg
FREE_CHUNK_HDR (mng_free_phyg)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PHYG, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_phyg));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PHYG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifdef MNG_INCLUDE_JNG
FREE_CHUNK_HDR (mng_free_jhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_JHDR, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_jhdr));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_JHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
FREE_CHUNK_HDR (mng_free_jdaa)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_JDAA, MNG_LC_START);
#endif

  if (((mng_jdaap)pHeader)->iDatasize)
    MNG_FREEX (pData, ((mng_jdaap)pHeader)->pData,
                      ((mng_jdaap)pHeader)->iDatasize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_jdaa));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_JDAA, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
FREE_CHUNK_HDR (mng_free_jdat)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_JDAT, MNG_LC_START);
#endif

  if (((mng_jdatp)pHeader)->iDatasize)
    MNG_FREEX (pData, ((mng_jdatp)pHeader)->pData,
                      ((mng_jdatp)pHeader)->iDatasize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_jdat));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_JDAT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifdef MNG_INCLUDE_JNG
FREE_CHUNK_HDR (mng_free_jsep)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_JSEP, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_jsep));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_JSEP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_NO_DELTA_PNG
FREE_CHUNK_HDR (mng_free_dhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DHDR, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_dhdr));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_NO_DELTA_PNG
FREE_CHUNK_HDR (mng_free_prom)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PROM, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_prom));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PROM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_NO_DELTA_PNG
FREE_CHUNK_HDR (mng_free_ipng)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IPNG, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_ipng));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_NO_DELTA_PNG
FREE_CHUNK_HDR (mng_free_pplt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PPLT, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_pplt));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_PPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
FREE_CHUNK_HDR (mng_free_ijng)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IJNG, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_ijng));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IJNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
FREE_CHUNK_HDR (mng_free_drop)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DROP, MNG_LC_START);
#endif

  if (((mng_dropp)pHeader)->iCount)
    MNG_FREEX (pData, ((mng_dropp)pHeader)->pChunknames,
                      ((mng_dropp)pHeader)->iCount * sizeof (mng_chunkid) );

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_drop));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DROP, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
FREE_CHUNK_HDR (mng_free_dbyk)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DBYK, MNG_LC_START);
#endif

  if (((mng_dbykp)pHeader)->iKeywordssize)
    MNG_FREEX (pData, ((mng_dbykp)pHeader)->zKeywords,
                      ((mng_dbykp)pHeader)->iKeywordssize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_dbyk));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_DBYK, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
FREE_CHUNK_HDR (mng_free_ordr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ORDR, MNG_LC_START);
#endif

  if (((mng_ordrp)pHeader)->iCount)
    MNG_FREEX (pData, ((mng_ordrp)pHeader)->pEntries,
                      ((mng_ordrp)pHeader)->iCount * sizeof (mng_ordr_entry) );

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_ordr));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ORDR, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
#ifndef MNG_SKIPCHUNK_MAGN
FREE_CHUNK_HDR (mng_free_magn)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MAGN, MNG_LC_START);
#endif

  MNG_FREEX (pData, pHeader, sizeof (mng_magn));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MAGN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_evNT
FREE_CHUNK_HDR (mng_free_evnt)
{
  mng_evnt_entryp pEntry = ((mng_evntp)pHeader)->pEntries;
  mng_uint32      iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_EVNT, MNG_LC_START);
#endif

  for (iX = 0; iX < ((mng_evntp)pHeader)->iCount; iX++)
  {
    if (pEntry->iSegmentnamesize)
      MNG_FREEX (pData, pEntry->zSegmentname, pEntry->iSegmentnamesize+1);

    pEntry++;
  }

  if (((mng_evntp)pHeader)->iCount)
    MNG_FREEX (pData, ((mng_evntp)pHeader)->pEntries,
                      ((mng_evntp)pHeader)->iCount * sizeof (mng_evnt_entry) );

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_evnt));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_EVNT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}
#endif

/* ************************************************************************** */

FREE_CHUNK_HDR (mng_free_unknown)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_UNKNOWN, MNG_LC_START);
#endif

  if (((mng_unknown_chunkp)pHeader)->iDatasize)
    MNG_FREEX (pData, ((mng_unknown_chunkp)pHeader)->pData,
                      ((mng_unknown_chunkp)pHeader)->iDatasize);

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  MNG_FREEX (pData, pHeader, sizeof (mng_unknown_chunk));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_UNKNOWN, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_CHUNKINITFREE
  return MNG_NOERROR;
#else
  return mng_free_general(pData, pHeader);
#endif
}

/* ************************************************************************** */
/* *                                                                        * */
/* * Chunk specific copy routines                                           * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_WRITE_PROCS

/* ************************************************************************** */

#ifdef MNG_OPTIMIZE_CHUNKASSIGN
ASSIGN_CHUNK_HDR (mng_assign_general)
{
  mng_ptr    pSrc = (mng_uint8p)pChunkfrom + sizeof (mng_chunk_header);
  mng_ptr    pDst = (mng_uint8p)pChunkto   + sizeof (mng_chunk_header);
  mng_size_t iLen = ((mng_chunk_headerp)pChunkfrom)->iChunksize - sizeof (mng_chunk_header);

  MNG_COPY (pDst, pSrc, iLen);

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
ASSIGN_CHUNK_HDR (mng_assign_ihdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IHDR, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_IHDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_ihdrp)pChunkto)->iWidth       = ((mng_ihdrp)pChunkfrom)->iWidth;
  ((mng_ihdrp)pChunkto)->iHeight      = ((mng_ihdrp)pChunkfrom)->iHeight;
  ((mng_ihdrp)pChunkto)->iBitdepth    = ((mng_ihdrp)pChunkfrom)->iBitdepth;
  ((mng_ihdrp)pChunkto)->iColortype   = ((mng_ihdrp)pChunkfrom)->iColortype;
  ((mng_ihdrp)pChunkto)->iCompression = ((mng_ihdrp)pChunkfrom)->iCompression;
  ((mng_ihdrp)pChunkto)->iFilter      = ((mng_ihdrp)pChunkfrom)->iFilter;
  ((mng_ihdrp)pChunkto)->iInterlace   = ((mng_ihdrp)pChunkfrom)->iInterlace;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
ASSIGN_CHUNK_HDR (mng_assign_plte)
{
  mng_uint32 iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PLTE, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_PLTE)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_pltep)pChunkto)->bEmpty      = ((mng_pltep)pChunkfrom)->bEmpty;
  ((mng_pltep)pChunkto)->iEntrycount = ((mng_pltep)pChunkfrom)->iEntrycount;

  for (iX = 0; iX < ((mng_pltep)pChunkto)->iEntrycount; iX++)
    ((mng_pltep)pChunkto)->aEntries [iX] = ((mng_pltep)pChunkfrom)->aEntries [iX];

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PLTE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

ASSIGN_CHUNK_HDR (mng_assign_idat)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IDAT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_IDAT)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_idatp)pChunkto)->bEmpty    = ((mng_idatp)pChunkfrom)->bEmpty;
  ((mng_idatp)pChunkto)->iDatasize = ((mng_idatp)pChunkfrom)->iDatasize;

  if (((mng_idatp)pChunkto)->iDatasize)
  {
    MNG_ALLOC (pData, ((mng_idatp)pChunkto)->pData, ((mng_idatp)pChunkto)->iDatasize);
    MNG_COPY  (((mng_idatp)pChunkto)->pData, ((mng_idatp)pChunkfrom)->pData,
               ((mng_idatp)pChunkto)->iDatasize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
ASSIGN_CHUNK_HDR (mng_assign_iend)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IEND, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_IEND)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
ASSIGN_CHUNK_HDR (mng_assign_trns)
{
  mng_uint32 iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_TRNS, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_tRNS)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_trnsp)pChunkto)->bEmpty  = ((mng_trnsp)pChunkfrom)->bEmpty;
  ((mng_trnsp)pChunkto)->bGlobal = ((mng_trnsp)pChunkfrom)->bGlobal;
  ((mng_trnsp)pChunkto)->iType   = ((mng_trnsp)pChunkfrom)->iType;
  ((mng_trnsp)pChunkto)->iCount  = ((mng_trnsp)pChunkfrom)->iCount;
  ((mng_trnsp)pChunkto)->iGray   = ((mng_trnsp)pChunkfrom)->iGray;
  ((mng_trnsp)pChunkto)->iRed    = ((mng_trnsp)pChunkfrom)->iRed;
  ((mng_trnsp)pChunkto)->iGreen  = ((mng_trnsp)pChunkfrom)->iGreen;
  ((mng_trnsp)pChunkto)->iBlue   = ((mng_trnsp)pChunkfrom)->iBlue;
  ((mng_trnsp)pChunkto)->iRawlen = ((mng_trnsp)pChunkfrom)->iRawlen;

  for (iX = 0; iX < ((mng_trnsp)pChunkto)->iCount; iX++)
    ((mng_trnsp)pChunkto)->aEntries [iX] = ((mng_trnsp)pChunkfrom)->aEntries [iX];

  for (iX = 0; iX < ((mng_trnsp)pChunkto)->iRawlen; iX++)
    ((mng_trnsp)pChunkto)->aRawdata [iX] = ((mng_trnsp)pChunkfrom)->aRawdata [iX];

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_TRNS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_gAMA
ASSIGN_CHUNK_HDR (mng_assign_gama)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_GAMA, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_gAMA)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_gamap)pChunkto)->bEmpty = ((mng_gamap)pChunkfrom)->bEmpty;
  ((mng_gamap)pChunkto)->iGamma = ((mng_gamap)pChunkfrom)->iGamma;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_GAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_cHRM
ASSIGN_CHUNK_HDR (mng_assign_chrm)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_CHRM, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_cHRM)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_chrmp)pChunkto)->bEmpty       = ((mng_chrmp)pChunkfrom)->bEmpty;
  ((mng_chrmp)pChunkto)->iWhitepointx = ((mng_chrmp)pChunkfrom)->iWhitepointx;
  ((mng_chrmp)pChunkto)->iWhitepointy = ((mng_chrmp)pChunkfrom)->iWhitepointy;
  ((mng_chrmp)pChunkto)->iRedx        = ((mng_chrmp)pChunkfrom)->iRedx;
  ((mng_chrmp)pChunkto)->iRedy        = ((mng_chrmp)pChunkfrom)->iRedy;
  ((mng_chrmp)pChunkto)->iGreenx      = ((mng_chrmp)pChunkfrom)->iGreenx;
  ((mng_chrmp)pChunkto)->iGreeny      = ((mng_chrmp)pChunkfrom)->iGreeny;
  ((mng_chrmp)pChunkto)->iBluex       = ((mng_chrmp)pChunkfrom)->iBluex;
  ((mng_chrmp)pChunkto)->iBluey       = ((mng_chrmp)pChunkfrom)->iBluey;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_CHRM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_sRGB
ASSIGN_CHUNK_HDR (mng_assign_srgb)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SRGB, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_sRGB)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_srgbp)pChunkto)->iRenderingintent = ((mng_srgbp)pChunkfrom)->iRenderingintent;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
ASSIGN_CHUNK_HDR (mng_assign_iccp)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ICCP, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_iCCP)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_iccpp)pChunkto)->bEmpty       = ((mng_iccpp)pChunkfrom)->bEmpty;
  ((mng_iccpp)pChunkto)->iNamesize    = ((mng_iccpp)pChunkfrom)->iNamesize;
  ((mng_iccpp)pChunkto)->iCompression = ((mng_iccpp)pChunkfrom)->iCompression;
  ((mng_iccpp)pChunkto)->iProfilesize = ((mng_iccpp)pChunkfrom)->iProfilesize;

  if (((mng_iccpp)pChunkto)->iNamesize)
  {
    MNG_ALLOC (pData, ((mng_iccpp)pChunkto)->zName, ((mng_iccpp)pChunkto)->iNamesize);
    MNG_COPY  (((mng_iccpp)pChunkto)->zName, ((mng_iccpp)pChunkfrom)->zName,
               ((mng_iccpp)pChunkto)->iNamesize);
  }

  if (((mng_iccpp)pChunkto)->iProfilesize)
  {
    MNG_ALLOC (pData, ((mng_iccpp)pChunkto)->pProfile, ((mng_iccpp)pChunkto)->iProfilesize);
    MNG_COPY  (((mng_iccpp)pChunkto)->pProfile, ((mng_iccpp)pChunkfrom)->pProfile,
               ((mng_iccpp)pChunkto)->iProfilesize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ICCP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tEXt
ASSIGN_CHUNK_HDR (mng_assign_text)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_TEXT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_tEXt)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_textp)pChunkto)->iKeywordsize = ((mng_textp)pChunkfrom)->iKeywordsize;
  ((mng_textp)pChunkto)->iTextsize    = ((mng_textp)pChunkfrom)->iTextsize;

  if (((mng_textp)pChunkto)->iKeywordsize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunkto)->zKeyword, ((mng_textp)pChunkto)->iKeywordsize);
    MNG_COPY  (((mng_itxtp)pChunkto)->zKeyword, ((mng_textp)pChunkfrom)->zKeyword,
               ((mng_itxtp)pChunkto)->iKeywordsize);
  }

  if (((mng_textp)pChunkto)->iTextsize)
  {
    MNG_ALLOC (pData, ((mng_textp)pChunkto)->zText, ((mng_textp)pChunkto)->iTextsize);
    MNG_COPY  (((mng_textp)pChunkto)->zText, ((mng_textp)pChunkfrom)->zText,
               ((mng_textp)pChunkto)->iTextsize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_TEXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_zTXt
ASSIGN_CHUNK_HDR (mng_assign_ztxt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ZTXT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_zTXt)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_ztxtp)pChunkto)->iKeywordsize = ((mng_ztxtp)pChunkfrom)->iKeywordsize;
  ((mng_ztxtp)pChunkto)->iCompression = ((mng_ztxtp)pChunkfrom)->iCompression;
  ((mng_ztxtp)pChunkto)->iTextsize    = ((mng_ztxtp)pChunkfrom)->iTextsize;

  if (((mng_ztxtp)pChunkto)->iKeywordsize)
  {
    MNG_ALLOC (pData, ((mng_ztxtp)pChunkto)->zKeyword, ((mng_ztxtp)pChunkto)->iKeywordsize);
    MNG_COPY  (((mng_ztxtp)pChunkto)->zKeyword, ((mng_ztxtp)pChunkfrom)->zKeyword,
               ((mng_ztxtp)pChunkto)->iKeywordsize);
  }

  if (((mng_ztxtp)pChunkto)->iTextsize)
  {
    MNG_ALLOC (pData, ((mng_ztxtp)pChunkto)->zText, ((mng_ztxtp)pChunkto)->iTextsize);
    MNG_COPY  (((mng_ztxtp)pChunkto)->zText, ((mng_ztxtp)pChunkfrom)->zText,
               ((mng_ztxtp)pChunkto)->iTextsize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ZTXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iTXt
ASSIGN_CHUNK_HDR (mng_assign_itxt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ITXT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_iTXt)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_itxtp)pChunkto)->iKeywordsize       = ((mng_itxtp)pChunkfrom)->iKeywordsize;
  ((mng_itxtp)pChunkto)->iCompressionflag   = ((mng_itxtp)pChunkfrom)->iCompressionflag;
  ((mng_itxtp)pChunkto)->iCompressionmethod = ((mng_itxtp)pChunkfrom)->iCompressionmethod;
  ((mng_itxtp)pChunkto)->iLanguagesize      = ((mng_itxtp)pChunkfrom)->iLanguagesize;
  ((mng_itxtp)pChunkto)->iTranslationsize   = ((mng_itxtp)pChunkfrom)->iTranslationsize;
  ((mng_itxtp)pChunkto)->iTextsize          = ((mng_itxtp)pChunkfrom)->iTextsize;

  if (((mng_itxtp)pChunkto)->iKeywordsize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunkto)->zKeyword, ((mng_itxtp)pChunkto)->iKeywordsize);
    MNG_COPY  (((mng_itxtp)pChunkto)->zKeyword, ((mng_itxtp)pChunkfrom)->zKeyword,
               ((mng_itxtp)pChunkto)->iKeywordsize);
  }

  if (((mng_itxtp)pChunkto)->iTextsize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunkto)->zLanguage, ((mng_itxtp)pChunkto)->iLanguagesize);
    MNG_COPY  (((mng_itxtp)pChunkto)->zLanguage, ((mng_itxtp)pChunkfrom)->zLanguage,
               ((mng_itxtp)pChunkto)->iLanguagesize);
  }

  if (((mng_itxtp)pChunkto)->iTextsize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunkto)->zTranslation, ((mng_itxtp)pChunkto)->iTranslationsize);
    MNG_COPY  (((mng_itxtp)pChunkto)->zTranslation, ((mng_itxtp)pChunkfrom)->zTranslation,
               ((mng_itxtp)pChunkto)->iTranslationsize);
  }

  if (((mng_itxtp)pChunkto)->iTextsize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunkto)->zText, ((mng_itxtp)pChunkto)->iTextsize);
    MNG_COPY  (((mng_itxtp)pChunkto)->zText, ((mng_itxtp)pChunkfrom)->zText,
               ((mng_itxtp)pChunkto)->iTextsize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ITXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_bKGD
ASSIGN_CHUNK_HDR (mng_assign_bkgd)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_BKGD, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_bKGD)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_bkgdp)pChunkto)->bEmpty = ((mng_bkgdp)pChunkfrom)->bEmpty;
  ((mng_bkgdp)pChunkto)->iType  = ((mng_bkgdp)pChunkfrom)->iType;
  ((mng_bkgdp)pChunkto)->iIndex = ((mng_bkgdp)pChunkfrom)->iIndex;
  ((mng_bkgdp)pChunkto)->iGray  = ((mng_bkgdp)pChunkfrom)->iGray;
  ((mng_bkgdp)pChunkto)->iRed   = ((mng_bkgdp)pChunkfrom)->iRed;
  ((mng_bkgdp)pChunkto)->iGreen = ((mng_bkgdp)pChunkfrom)->iGreen;
  ((mng_bkgdp)pChunkto)->iBlue  = ((mng_bkgdp)pChunkfrom)->iBlue;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_BKGD, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_pHYs
ASSIGN_CHUNK_HDR (mng_assign_phys)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PHYS, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_pHYs)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_physp)pChunkto)->bEmpty = ((mng_physp)pChunkfrom)->bEmpty;
  ((mng_physp)pChunkto)->iSizex = ((mng_physp)pChunkfrom)->iSizex;
  ((mng_physp)pChunkto)->iSizey = ((mng_physp)pChunkfrom)->iSizey;
  ((mng_physp)pChunkto)->iUnit  = ((mng_physp)pChunkfrom)->iUnit;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PHYS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_sBIT
ASSIGN_CHUNK_HDR (mng_assign_sbit)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SBIT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_sBIT)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_sbitp)pChunkto)->bEmpty    = ((mng_sbitp)pChunkfrom)->bEmpty;
  ((mng_sbitp)pChunkto)->iType     = ((mng_sbitp)pChunkfrom)->iType;
  ((mng_sbitp)pChunkto)->aBits [0] = ((mng_sbitp)pChunkfrom)->aBits [0];
  ((mng_sbitp)pChunkto)->aBits [1] = ((mng_sbitp)pChunkfrom)->aBits [1];
  ((mng_sbitp)pChunkto)->aBits [2] = ((mng_sbitp)pChunkfrom)->aBits [2];
  ((mng_sbitp)pChunkto)->aBits [3] = ((mng_sbitp)pChunkfrom)->aBits [3];

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SBIT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
ASSIGN_CHUNK_HDR (mng_assign_splt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SPLT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_sPLT)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_spltp)pChunkto)->bEmpty       = ((mng_spltp)pChunkfrom)->bEmpty;
  ((mng_spltp)pChunkto)->iNamesize    = ((mng_spltp)pChunkfrom)->iNamesize;
  ((mng_spltp)pChunkto)->iSampledepth = ((mng_spltp)pChunkfrom)->iSampledepth;
  ((mng_spltp)pChunkto)->iEntrycount  = ((mng_spltp)pChunkfrom)->iEntrycount;
  ((mng_spltp)pChunkto)->pEntries     = ((mng_spltp)pChunkfrom)->pEntries;

  if (((mng_spltp)pChunkto)->iNamesize)
  {
    MNG_ALLOC (pData, ((mng_spltp)pChunkto)->zName, ((mng_spltp)pChunkto)->iNamesize);
    MNG_COPY  (((mng_spltp)pChunkto)->zName, ((mng_spltp)pChunkfrom)->zName,
               ((mng_spltp)pChunkto)->iNamesize);
  }

  if (((mng_spltp)pChunkto)->iEntrycount)
  {
    mng_uint32 iLen = ((mng_spltp)pChunkto)->iEntrycount *
                      (((mng_spltp)pChunkto)->iSampledepth * 3 + sizeof (mng_uint16));

    MNG_ALLOC (pData, ((mng_spltp)pChunkto)->pEntries, iLen);
    MNG_COPY  (((mng_spltp)pChunkto)->pEntries, ((mng_spltp)pChunkfrom)->pEntries, iLen);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_hIST
ASSIGN_CHUNK_HDR (mng_assign_hist)
{
  mng_uint32 iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_HIST, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_hIST)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_histp)pChunkto)->iEntrycount = ((mng_histp)pChunkfrom)->iEntrycount;

  for (iX = 0; iX < ((mng_histp)pChunkto)->iEntrycount; iX++)
    ((mng_histp)pChunkto)->aEntries [iX] = ((mng_histp)pChunkfrom)->aEntries [iX];

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_HIST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_tIME
ASSIGN_CHUNK_HDR (mng_assign_time)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_TIME, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_tIME)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_timep)pChunkto)->iYear   = ((mng_timep)pChunkfrom)->iYear;
  ((mng_timep)pChunkto)->iMonth  = ((mng_timep)pChunkfrom)->iMonth;
  ((mng_timep)pChunkto)->iDay    = ((mng_timep)pChunkfrom)->iDay;
  ((mng_timep)pChunkto)->iHour   = ((mng_timep)pChunkfrom)->iHour;
  ((mng_timep)pChunkto)->iMinute = ((mng_timep)pChunkfrom)->iMinute;
  ((mng_timep)pChunkto)->iSecond = ((mng_timep)pChunkfrom)->iSecond;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_TIME, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
ASSIGN_CHUNK_HDR (mng_assign_mhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MHDR, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_mhdrp)pChunkto)->iWidth      = ((mng_mhdrp)pChunkfrom)->iWidth;
  ((mng_mhdrp)pChunkto)->iHeight     = ((mng_mhdrp)pChunkfrom)->iHeight;
  ((mng_mhdrp)pChunkto)->iTicks      = ((mng_mhdrp)pChunkfrom)->iTicks;
  ((mng_mhdrp)pChunkto)->iLayercount = ((mng_mhdrp)pChunkfrom)->iLayercount;
  ((mng_mhdrp)pChunkto)->iFramecount = ((mng_mhdrp)pChunkfrom)->iFramecount;
  ((mng_mhdrp)pChunkto)->iPlaytime   = ((mng_mhdrp)pChunkfrom)->iPlaytime;
  ((mng_mhdrp)pChunkto)->iSimplicity = ((mng_mhdrp)pChunkfrom)->iSimplicity;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
ASSIGN_CHUNK_HDR (mng_assign_mend)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MEND, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_MEND)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
ASSIGN_CHUNK_HDR (mng_assign_loop)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_LOOP, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_LOOP)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_loopp)pChunkto)->iLevel       = ((mng_loopp)pChunkfrom)->iLevel;
  ((mng_loopp)pChunkto)->iRepeat      = ((mng_loopp)pChunkfrom)->iRepeat;
  ((mng_loopp)pChunkto)->iTermination = ((mng_loopp)pChunkfrom)->iTermination;
  ((mng_loopp)pChunkto)->iItermin     = ((mng_loopp)pChunkfrom)->iItermin;
  ((mng_loopp)pChunkto)->iItermax     = ((mng_loopp)pChunkfrom)->iItermax;
  ((mng_loopp)pChunkto)->iCount       = ((mng_loopp)pChunkfrom)->iCount;

#ifndef MNG_NO_LOOP_SIGNALS_SUPPORTED
  if (((mng_loopp)pChunkto)->iCount)
  {
    mng_uint32 iLen = ((mng_loopp)pChunkto)->iCount * sizeof (mng_uint32);
    MNG_ALLOC (pData, ((mng_loopp)pChunkto)->pSignals, iLen);
    MNG_COPY  (((mng_loopp)pChunkto)->pSignals, ((mng_loopp)pChunkfrom)->pSignals, iLen);
  }
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_LOOP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
ASSIGN_CHUNK_HDR (mng_assign_endl)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ENDL, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_ENDL)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_endlp)pChunkto)->iLevel = ((mng_endlp)pChunkfrom)->iLevel;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ENDL, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_DEFI
ASSIGN_CHUNK_HDR (mng_assign_defi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DEFI, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_DEFI)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_defip)pChunkto)->iObjectid     = ((mng_defip)pChunkfrom)->iObjectid;
  ((mng_defip)pChunkto)->bHasdonotshow = ((mng_defip)pChunkfrom)->bHasdonotshow;
  ((mng_defip)pChunkto)->iDonotshow    = ((mng_defip)pChunkfrom)->iDonotshow;
  ((mng_defip)pChunkto)->bHasconcrete  = ((mng_defip)pChunkfrom)->bHasconcrete;
  ((mng_defip)pChunkto)->iConcrete     = ((mng_defip)pChunkfrom)->iConcrete;
  ((mng_defip)pChunkto)->bHasloca      = ((mng_defip)pChunkfrom)->bHasloca;
  ((mng_defip)pChunkto)->iXlocation    = ((mng_defip)pChunkfrom)->iXlocation;
  ((mng_defip)pChunkto)->iYlocation    = ((mng_defip)pChunkfrom)->iYlocation;
  ((mng_defip)pChunkto)->bHasclip      = ((mng_defip)pChunkfrom)->bHasclip;
  ((mng_defip)pChunkto)->iLeftcb       = ((mng_defip)pChunkfrom)->iLeftcb;
  ((mng_defip)pChunkto)->iRightcb      = ((mng_defip)pChunkfrom)->iRightcb;
  ((mng_defip)pChunkto)->iTopcb        = ((mng_defip)pChunkfrom)->iTopcb;
  ((mng_defip)pChunkto)->iBottomcb     = ((mng_defip)pChunkfrom)->iBottomcb;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_BASI
ASSIGN_CHUNK_HDR (mng_assign_basi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_BASI, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_BASI)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_basip)pChunkto)->iWidth       = ((mng_basip)pChunkfrom)->iWidth;
  ((mng_basip)pChunkto)->iHeight      = ((mng_basip)pChunkfrom)->iHeight;
  ((mng_basip)pChunkto)->iBitdepth    = ((mng_basip)pChunkfrom)->iBitdepth;
  ((mng_basip)pChunkto)->iColortype   = ((mng_basip)pChunkfrom)->iColortype;
  ((mng_basip)pChunkto)->iCompression = ((mng_basip)pChunkfrom)->iCompression;
  ((mng_basip)pChunkto)->iFilter      = ((mng_basip)pChunkfrom)->iFilter;
  ((mng_basip)pChunkto)->iInterlace   = ((mng_basip)pChunkfrom)->iInterlace;
  ((mng_basip)pChunkto)->iRed         = ((mng_basip)pChunkfrom)->iRed;
  ((mng_basip)pChunkto)->iGreen       = ((mng_basip)pChunkfrom)->iGreen;
  ((mng_basip)pChunkto)->iBlue        = ((mng_basip)pChunkfrom)->iBlue;
  ((mng_basip)pChunkto)->iAlpha       = ((mng_basip)pChunkfrom)->iAlpha;
  ((mng_basip)pChunkto)->iViewable    = ((mng_basip)pChunkfrom)->iViewable;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_BASI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_CLON
ASSIGN_CHUNK_HDR (mng_assign_clon)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_CLON, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_CLON)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_clonp)pChunkto)->iSourceid     = ((mng_clonp)pChunkfrom)->iSourceid;
  ((mng_clonp)pChunkto)->iCloneid      = ((mng_clonp)pChunkfrom)->iCloneid;
  ((mng_clonp)pChunkto)->iClonetype    = ((mng_clonp)pChunkfrom)->iClonetype;
#ifdef MNG_OPTIMIZE_CHUNKREADER
  ((mng_clonp)pChunkto)->bHasdonotshow = ((mng_clonp)pChunkfrom)->bHasdonotshow;
#endif
  ((mng_clonp)pChunkto)->iDonotshow    = ((mng_clonp)pChunkfrom)->iDonotshow;
  ((mng_clonp)pChunkto)->iConcrete     = ((mng_clonp)pChunkfrom)->iConcrete;
  ((mng_clonp)pChunkto)->bHasloca      = ((mng_clonp)pChunkfrom)->bHasloca;
  ((mng_clonp)pChunkto)->iLocationtype = ((mng_clonp)pChunkfrom)->iLocationtype;
  ((mng_clonp)pChunkto)->iLocationx    = ((mng_clonp)pChunkfrom)->iLocationx;
  ((mng_clonp)pChunkto)->iLocationy    = ((mng_clonp)pChunkfrom)->iLocationy;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_CLON, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
ASSIGN_CHUNK_HDR (mng_assign_past)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PAST, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_PAST)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_pastp)pChunkto)->iDestid     = ((mng_pastp)pChunkfrom)->iDestid;
  ((mng_pastp)pChunkto)->iTargettype = ((mng_pastp)pChunkfrom)->iTargettype;
  ((mng_pastp)pChunkto)->iTargetx    = ((mng_pastp)pChunkfrom)->iTargetx;
  ((mng_pastp)pChunkto)->iTargety    = ((mng_pastp)pChunkfrom)->iTargety;
  ((mng_pastp)pChunkto)->iCount      = ((mng_pastp)pChunkfrom)->iCount;

  if (((mng_pastp)pChunkto)->iCount)
  {
    mng_uint32 iLen = ((mng_pastp)pChunkto)->iCount * sizeof (mng_past_source);

    MNG_ALLOC (pData, ((mng_pastp)pChunkto)->pSources, iLen);
    MNG_COPY  (((mng_pastp)pChunkto)->pSources, ((mng_pastp)pChunkfrom)->pSources, iLen);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PAST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
ASSIGN_CHUNK_HDR (mng_assign_disc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DISC, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_DISC)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_discp)pChunkto)->iCount = ((mng_discp)pChunkfrom)->iCount;

  if (((mng_discp)pChunkto)->iCount)
  {
    mng_uint32 iLen = ((mng_discp)pChunkto)->iCount * sizeof (mng_uint16);

    MNG_ALLOC (pData, ((mng_discp)pChunkto)->pObjectids, iLen);
    MNG_COPY  (((mng_discp)pChunkto)->pObjectids, ((mng_discp)pChunkfrom)->pObjectids, iLen);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DISC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_BACK
ASSIGN_CHUNK_HDR (mng_assign_back)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_BACK, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_BACK)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_backp)pChunkto)->iRed       = ((mng_backp)pChunkfrom)->iRed;
  ((mng_backp)pChunkto)->iGreen     = ((mng_backp)pChunkfrom)->iGreen;
  ((mng_backp)pChunkto)->iBlue      = ((mng_backp)pChunkfrom)->iBlue;
  ((mng_backp)pChunkto)->iMandatory = ((mng_backp)pChunkfrom)->iMandatory;
  ((mng_backp)pChunkto)->iImageid   = ((mng_backp)pChunkfrom)->iImageid;
  ((mng_backp)pChunkto)->iTile      = ((mng_backp)pChunkfrom)->iTile;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_BACK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
ASSIGN_CHUNK_HDR (mng_assign_fram)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_FRAM, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_FRAM)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_framp)pChunkto)->bEmpty          = ((mng_framp)pChunkfrom)->bEmpty;
  ((mng_framp)pChunkto)->iMode           = ((mng_framp)pChunkfrom)->iMode;
  ((mng_framp)pChunkto)->iNamesize       = ((mng_framp)pChunkfrom)->iNamesize;
  ((mng_framp)pChunkto)->iChangedelay    = ((mng_framp)pChunkfrom)->iChangedelay;
  ((mng_framp)pChunkto)->iChangetimeout  = ((mng_framp)pChunkfrom)->iChangetimeout;
  ((mng_framp)pChunkto)->iChangeclipping = ((mng_framp)pChunkfrom)->iChangeclipping;
  ((mng_framp)pChunkto)->iChangesyncid   = ((mng_framp)pChunkfrom)->iChangesyncid;
  ((mng_framp)pChunkto)->iDelay          = ((mng_framp)pChunkfrom)->iDelay;
  ((mng_framp)pChunkto)->iTimeout        = ((mng_framp)pChunkfrom)->iTimeout;
  ((mng_framp)pChunkto)->iBoundarytype   = ((mng_framp)pChunkfrom)->iBoundarytype;
  ((mng_framp)pChunkto)->iBoundaryl      = ((mng_framp)pChunkfrom)->iBoundaryl;
  ((mng_framp)pChunkto)->iBoundaryr      = ((mng_framp)pChunkfrom)->iBoundaryr;
  ((mng_framp)pChunkto)->iBoundaryt      = ((mng_framp)pChunkfrom)->iBoundaryt;
  ((mng_framp)pChunkto)->iBoundaryb      = ((mng_framp)pChunkfrom)->iBoundaryb;
  ((mng_framp)pChunkto)->iCount          = ((mng_framp)pChunkfrom)->iCount;

  if (((mng_framp)pChunkto)->iNamesize)
  {
    MNG_ALLOC (pData, ((mng_framp)pChunkto)->zName, ((mng_framp)pChunkto)->iNamesize);
    MNG_COPY  (((mng_framp)pChunkto)->zName, ((mng_framp)pChunkfrom)->zName,
               ((mng_framp)pChunkto)->iNamesize);
  }

  if (((mng_framp)pChunkto)->iCount)
  {
    mng_uint32 iLen = ((mng_framp)pChunkto)->iCount * sizeof (mng_uint32);

    MNG_ALLOC (pData, ((mng_framp)pChunkto)->pSyncids, iLen);
    MNG_COPY  (((mng_framp)pChunkto)->pSyncids, ((mng_framp)pChunkfrom)->pSyncids, iLen);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_FRAM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_MOVE
ASSIGN_CHUNK_HDR (mng_assign_move)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MOVE, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_MOVE)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_movep)pChunkto)->iFirstid  = ((mng_movep)pChunkfrom)->iFirstid;
  ((mng_movep)pChunkto)->iLastid   = ((mng_movep)pChunkfrom)->iLastid;
  ((mng_movep)pChunkto)->iMovetype = ((mng_movep)pChunkfrom)->iMovetype;
  ((mng_movep)pChunkto)->iMovex    = ((mng_movep)pChunkfrom)->iMovex;
  ((mng_movep)pChunkto)->iMovey    = ((mng_movep)pChunkfrom)->iMovey;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MOVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_CLIP
ASSIGN_CHUNK_HDR (mng_assign_clip)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_CLIP, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_CLIP)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_clipp)pChunkto)->iFirstid  = ((mng_clipp)pChunkfrom)->iFirstid;
  ((mng_clipp)pChunkto)->iLastid   = ((mng_clipp)pChunkfrom)->iLastid;
  ((mng_clipp)pChunkto)->iCliptype = ((mng_clipp)pChunkfrom)->iCliptype;
  ((mng_clipp)pChunkto)->iClipl    = ((mng_clipp)pChunkfrom)->iClipl;
  ((mng_clipp)pChunkto)->iClipr    = ((mng_clipp)pChunkfrom)->iClipr;
  ((mng_clipp)pChunkto)->iClipt    = ((mng_clipp)pChunkfrom)->iClipt;
  ((mng_clipp)pChunkto)->iClipb    = ((mng_clipp)pChunkfrom)->iClipb;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_CLIP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_SHOW
ASSIGN_CHUNK_HDR (mng_assign_show)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SHOW, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_SHOW)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_showp)pChunkto)->bEmpty   = ((mng_showp)pChunkfrom)->bEmpty;
  ((mng_showp)pChunkto)->iFirstid = ((mng_showp)pChunkfrom)->iFirstid;
  ((mng_showp)pChunkto)->iLastid  = ((mng_showp)pChunkfrom)->iLastid;
  ((mng_showp)pChunkto)->iMode    = ((mng_showp)pChunkfrom)->iMode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SHOW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_TERM
ASSIGN_CHUNK_HDR (mng_assign_term)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_TERM, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_TERM)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_termp)pChunkto)->iTermaction = ((mng_termp)pChunkfrom)->iTermaction;
  ((mng_termp)pChunkto)->iIteraction = ((mng_termp)pChunkfrom)->iIteraction;
  ((mng_termp)pChunkto)->iDelay      = ((mng_termp)pChunkfrom)->iDelay;
  ((mng_termp)pChunkto)->iItermax    = ((mng_termp)pChunkfrom)->iItermax;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_TERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
ASSIGN_CHUNK_HDR (mng_assign_save)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SAVE, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_SAVE)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_savep)pChunkto)->bEmpty      = ((mng_savep)pChunkfrom)->bEmpty;
  ((mng_savep)pChunkto)->iOffsettype = ((mng_savep)pChunkfrom)->iOffsettype;
  ((mng_savep)pChunkto)->iCount      = ((mng_savep)pChunkfrom)->iCount;

  if (((mng_savep)pChunkto)->iCount)
  {
    mng_uint32      iX;
    mng_save_entryp pEntry;
    mng_uint32      iLen = ((mng_savep)pChunkto)->iCount * sizeof (mng_save_entry);

    MNG_ALLOC (pData, ((mng_savep)pChunkto)->pEntries, iLen);
    MNG_COPY  (((mng_savep)pChunkto)->pEntries, ((mng_savep)pChunkfrom)->pEntries, iLen);

    pEntry = ((mng_savep)pChunkto)->pEntries;

    for (iX = 0; iX < ((mng_savep)pChunkto)->iCount; iX++)
    {
      if (pEntry->iNamesize)
      {
        mng_pchar pTemp = pEntry->zName;

        MNG_ALLOC (pData, pEntry->zName, pEntry->iNamesize);
        MNG_COPY  (pEntry->zName, pTemp, pEntry->iNamesize);
      }
      else
      {
        pEntry->zName = MNG_NULL;
      }

      pEntry++;
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
ASSIGN_CHUNK_HDR (mng_assign_seek)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SEEK, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_SEEK)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_seekp)pChunkto)->iNamesize = ((mng_seekp)pChunkfrom)->iNamesize;

  if (((mng_seekp)pChunkto)->iNamesize)
  {
    MNG_ALLOC (pData, ((mng_seekp)pChunkto)->zName, ((mng_seekp)pChunkto)->iNamesize);
    MNG_COPY  (((mng_seekp)pChunkto)->zName, ((mng_seekp)pChunkfrom)->zName,
               ((mng_seekp)pChunkto)->iNamesize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_SEEK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_eXPI
ASSIGN_CHUNK_HDR (mng_assign_expi)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_EXPI, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_eXPI)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_expip)pChunkto)->iSnapshotid = ((mng_expip)pChunkfrom)->iSnapshotid;
  ((mng_expip)pChunkto)->iNamesize   = ((mng_expip)pChunkfrom)->iNamesize;

  if (((mng_expip)pChunkto)->iNamesize)
  {
    MNG_ALLOC (pData, ((mng_expip)pChunkto)->zName, ((mng_expip)pChunkto)->iNamesize);
    MNG_COPY  (((mng_expip)pChunkto)->zName, ((mng_expip)pChunkfrom)->zName,
               ((mng_expip)pChunkto)->iNamesize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_EXPI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_fPRI
ASSIGN_CHUNK_HDR (mng_assign_fpri)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_FPRI, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_fPRI)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_fprip)pChunkto)->iDeltatype = ((mng_fprip)pChunkfrom)->iDeltatype;
  ((mng_fprip)pChunkto)->iPriority  = ((mng_fprip)pChunkfrom)->iPriority;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_FPRI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
ASSIGN_CHUNK_HDR (mng_assign_need)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_NEED, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_nEED)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_needp)pChunkto)->iKeywordssize = ((mng_needp)pChunkfrom)->iKeywordssize;

  if (((mng_needp)pChunkto)->iKeywordssize)
  {
    MNG_ALLOC (pData, ((mng_needp)pChunkto)->zKeywords, ((mng_needp)pChunkto)->iKeywordssize);
    MNG_COPY  (((mng_needp)pChunkto)->zKeywords, ((mng_needp)pChunkfrom)->zKeywords,
               ((mng_needp)pChunkto)->iKeywordssize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_NEED, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_pHYg
ASSIGN_CHUNK_HDR (mng_assign_phyg)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PHYG, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_pHYg)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_phygp)pChunkto)->bEmpty = ((mng_phygp)pChunkfrom)->bEmpty;
  ((mng_phygp)pChunkto)->iSizex = ((mng_phygp)pChunkfrom)->iSizex;
  ((mng_phygp)pChunkto)->iSizey = ((mng_phygp)pChunkfrom)->iSizey;
  ((mng_phygp)pChunkto)->iUnit  = ((mng_phygp)pChunkfrom)->iUnit;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PHYG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifdef MNG_INCLUDE_JNG
ASSIGN_CHUNK_HDR (mng_assign_jhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_JHDR, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_JHDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_jhdrp)pChunkto)->iWidth            = ((mng_jhdrp)pChunkfrom)->iWidth;
  ((mng_jhdrp)pChunkto)->iHeight           = ((mng_jhdrp)pChunkfrom)->iHeight;
  ((mng_jhdrp)pChunkto)->iColortype        = ((mng_jhdrp)pChunkfrom)->iColortype;
  ((mng_jhdrp)pChunkto)->iImagesampledepth = ((mng_jhdrp)pChunkfrom)->iImagesampledepth;
  ((mng_jhdrp)pChunkto)->iImagecompression = ((mng_jhdrp)pChunkfrom)->iImagecompression;
  ((mng_jhdrp)pChunkto)->iImageinterlace   = ((mng_jhdrp)pChunkfrom)->iImageinterlace;
  ((mng_jhdrp)pChunkto)->iAlphasampledepth = ((mng_jhdrp)pChunkfrom)->iAlphasampledepth;
  ((mng_jhdrp)pChunkto)->iAlphacompression = ((mng_jhdrp)pChunkfrom)->iAlphacompression;
  ((mng_jhdrp)pChunkto)->iAlphafilter      = ((mng_jhdrp)pChunkfrom)->iAlphafilter;
  ((mng_jhdrp)pChunkto)->iAlphainterlace   = ((mng_jhdrp)pChunkfrom)->iAlphainterlace;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_JHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
ASSIGN_CHUNK_HDR (mng_assign_jdaa)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_JDAA, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_JDAA)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_jdaap)pChunkto)->bEmpty    = ((mng_jdaap)pChunkfrom)->bEmpty;
  ((mng_jdaap)pChunkto)->iDatasize = ((mng_jdaap)pChunkfrom)->iDatasize;

  if (((mng_jdaap)pChunkto)->iDatasize)
  {
    MNG_ALLOC (pData, ((mng_jdaap)pChunkto)->pData, ((mng_jdaap)pChunkto)->iDatasize);
    MNG_COPY  (((mng_jdaap)pChunkto)->pData, ((mng_jdaap)pChunkfrom)->pData,
               ((mng_jdaap)pChunkto)->iDatasize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_JDAA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
ASSIGN_CHUNK_HDR (mng_assign_jdat)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_JDAT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_JDAT)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_jdatp)pChunkto)->bEmpty    = ((mng_jdatp)pChunkfrom)->bEmpty;
  ((mng_jdatp)pChunkto)->iDatasize = ((mng_jdatp)pChunkfrom)->iDatasize;

  if (((mng_jdatp)pChunkto)->iDatasize)
  {
    MNG_ALLOC (pData, ((mng_jdatp)pChunkto)->pData, ((mng_jdatp)pChunkto)->iDatasize);
    MNG_COPY  (((mng_jdatp)pChunkto)->pData, ((mng_jdatp)pChunkfrom)->pData,
               ((mng_jdatp)pChunkto)->iDatasize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_JDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifdef MNG_INCLUDE_JNG
ASSIGN_CHUNK_HDR (mng_assign_jsep)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_JSEP, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_JSEP)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_JSEP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_NO_DELTA_PNG
ASSIGN_CHUNK_HDR (mng_assign_dhdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DHDR, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_DHDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_dhdrp)pChunkto)->iObjectid    = ((mng_dhdrp)pChunkfrom)->iObjectid;
  ((mng_dhdrp)pChunkto)->iImagetype   = ((mng_dhdrp)pChunkfrom)->iImagetype;
  ((mng_dhdrp)pChunkto)->iDeltatype   = ((mng_dhdrp)pChunkfrom)->iDeltatype;
  ((mng_dhdrp)pChunkto)->iBlockwidth  = ((mng_dhdrp)pChunkfrom)->iBlockwidth;
  ((mng_dhdrp)pChunkto)->iBlockheight = ((mng_dhdrp)pChunkfrom)->iBlockheight;
  ((mng_dhdrp)pChunkto)->iBlockx      = ((mng_dhdrp)pChunkfrom)->iBlockx;
  ((mng_dhdrp)pChunkto)->iBlocky      = ((mng_dhdrp)pChunkfrom)->iBlocky;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_NO_DELTA_PNG
ASSIGN_CHUNK_HDR (mng_assign_prom)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PROM, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_PROM)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_promp)pChunkto)->iColortype   = ((mng_promp)pChunkfrom)->iColortype;
  ((mng_promp)pChunkto)->iSampledepth = ((mng_promp)pChunkfrom)->iSampledepth;
  ((mng_promp)pChunkto)->iFilltype    = ((mng_promp)pChunkfrom)->iFilltype;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PROM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_NO_DELTA_PNG
ASSIGN_CHUNK_HDR (mng_assign_ipng)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IPNG, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_IPNG)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_NO_DELTA_PNG
ASSIGN_CHUNK_HDR (mng_assign_pplt)
{
  mng_uint32 iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PPLT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_PPLT)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_ppltp)pChunkto)->iDeltatype = ((mng_ppltp)pChunkfrom)->iDeltatype;
  ((mng_ppltp)pChunkto)->iCount     = ((mng_ppltp)pChunkfrom)->iCount;

  for (iX = 0; iX < ((mng_ppltp)pChunkto)->iCount; iX++)
    ((mng_ppltp)pChunkto)->aEntries [iX] = ((mng_ppltp)pChunkfrom)->aEntries [iX];

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_PPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
ASSIGN_CHUNK_HDR (mng_assign_ijng)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IJNG, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_IJNG)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_IJNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
ASSIGN_CHUNK_HDR (mng_assign_drop)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DROP, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_DROP)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_dropp)pChunkto)->iCount = ((mng_dropp)pChunkfrom)->iCount;

  if (((mng_dropp)pChunkto)->iCount)
  {
    mng_uint32 iLen = ((mng_dropp)pChunkto)->iCount * sizeof (mng_uint32);

    MNG_ALLOC (pData, ((mng_dropp)pChunkto)->pChunknames, iLen);
    MNG_COPY  (((mng_dropp)pChunkto)->pChunknames, ((mng_dropp)pChunkfrom)->pChunknames, iLen);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DROP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
ASSIGN_CHUNK_HDR (mng_assign_dbyk)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DBYK, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_DBYK)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_dbykp)pChunkto)->iChunkname    = ((mng_dbykp)pChunkfrom)->iChunkname;
  ((mng_dbykp)pChunkto)->iPolarity     = ((mng_dbykp)pChunkfrom)->iPolarity;
  ((mng_dbykp)pChunkto)->iKeywordssize = ((mng_dbykp)pChunkfrom)->iKeywordssize;

  if (((mng_dbykp)pChunkto)->iKeywordssize)
  {
    MNG_ALLOC (pData, ((mng_dbykp)pChunkto)->zKeywords, ((mng_dbykp)pChunkto)->iKeywordssize);
    MNG_COPY  (((mng_dbykp)pChunkto)->zKeywords, ((mng_dbykp)pChunkfrom)->zKeywords,
               ((mng_dbykp)pChunkto)->iKeywordssize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_DBYK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
ASSIGN_CHUNK_HDR (mng_assign_ordr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ORDR, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_ORDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_ordrp)pChunkto)->iCount = ((mng_ordrp)pChunkfrom)->iCount;

  if (((mng_ordrp)pChunkto)->iCount)
  {
    mng_uint32 iLen = ((mng_ordrp)pChunkto)->iCount * sizeof (mng_ordr_entry);

    MNG_ALLOC (pData, ((mng_ordrp)pChunkto)->pEntries, iLen);
    MNG_COPY  (((mng_ordrp)pChunkto)->pEntries, ((mng_ordrp)pChunkfrom)->pEntries, iLen);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ORDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKASSIGN
#ifndef MNG_SKIPCHUNK_MAGN
ASSIGN_CHUNK_HDR (mng_assign_magn)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MAGN, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_MAGN)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_magnp)pChunkto)->iFirstid = ((mng_magnp)pChunkfrom)->iFirstid;
  ((mng_magnp)pChunkto)->iLastid  = ((mng_magnp)pChunkfrom)->iLastid;
  ((mng_magnp)pChunkto)->iMethodX = ((mng_magnp)pChunkfrom)->iMethodX;
  ((mng_magnp)pChunkto)->iMX      = ((mng_magnp)pChunkfrom)->iMX;
  ((mng_magnp)pChunkto)->iMY      = ((mng_magnp)pChunkfrom)->iMY;
  ((mng_magnp)pChunkto)->iML      = ((mng_magnp)pChunkfrom)->iML;
  ((mng_magnp)pChunkto)->iMR      = ((mng_magnp)pChunkfrom)->iMR;
  ((mng_magnp)pChunkto)->iMT      = ((mng_magnp)pChunkfrom)->iMT;
  ((mng_magnp)pChunkto)->iMB      = ((mng_magnp)pChunkfrom)->iMB;
  ((mng_magnp)pChunkto)->iMethodY = ((mng_magnp)pChunkfrom)->iMethodY;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MAGN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
ASSIGN_CHUNK_HDR (mng_assign_mpng)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MPNG, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_mpNG)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_mpngp)pChunkto)->iFramewidth        = ((mng_mpngp)pChunkfrom)->iFramewidth;
  ((mng_mpngp)pChunkto)->iFrameheight       = ((mng_mpngp)pChunkfrom)->iFrameheight;
  ((mng_mpngp)pChunkto)->iNumplays          = ((mng_mpngp)pChunkfrom)->iNumplays;
  ((mng_mpngp)pChunkto)->iTickspersec       = ((mng_mpngp)pChunkfrom)->iTickspersec;
  ((mng_mpngp)pChunkto)->iCompressionmethod = ((mng_mpngp)pChunkfrom)->iCompressionmethod;
  ((mng_mpngp)pChunkto)->iFramessize        = ((mng_mpngp)pChunkfrom)->iFramessize;

  if (((mng_mpngp)pChunkto)->iFramessize)
  {
    MNG_ALLOC (pData, ((mng_mpngp)pChunkto)->pFrames, ((mng_mpngp)pChunkto)->iFramessize);
    MNG_COPY  (((mng_mpngp)pChunkto)->pFrames, ((mng_mpngp)pChunkfrom)->pFrames,
               ((mng_mpngp)pChunkto)->iFramessize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_MPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
ASSIGN_CHUNK_HDR (mng_assign_ahdr)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_AHDR, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_ahDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_ahdrp)pChunkto)->iNumframes   = ((mng_ahdrp)pChunkfrom)->iNumframes;
  ((mng_ahdrp)pChunkto)->iTickspersec = ((mng_ahdrp)pChunkfrom)->iTickspersec;
  ((mng_ahdrp)pChunkto)->iNumplays    = ((mng_ahdrp)pChunkfrom)->iNumplays;
  ((mng_ahdrp)pChunkto)->iTilewidth   = ((mng_ahdrp)pChunkfrom)->iTilewidth;
  ((mng_ahdrp)pChunkto)->iTileheight  = ((mng_ahdrp)pChunkfrom)->iTileheight;
  ((mng_ahdrp)pChunkto)->iInterlace   = ((mng_ahdrp)pChunkfrom)->iInterlace;
  ((mng_ahdrp)pChunkto)->iStillused   = ((mng_ahdrp)pChunkfrom)->iStillused;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_AHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
ASSIGN_CHUNK_HDR (mng_assign_adat)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ADAT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_adAT)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_adatp)pChunkto)->iTilessize = ((mng_adatp)pChunkfrom)->iTilessize;

  if (((mng_adatp)pChunkto)->iTilessize)
  {
    MNG_ALLOC (pData, ((mng_adatp)pChunkto)->pTiles, ((mng_adatp)pChunkto)->iTilessize);
    MNG_COPY  (((mng_adatp)pChunkto)->pTiles, ((mng_adatp)pChunkfrom)->pTiles,
               ((mng_adatp)pChunkto)->iTilessize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_ADAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_evNT
ASSIGN_CHUNK_HDR (mng_assign_evnt)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_EVNT, MNG_LC_START);
#endif

  if (((mng_chunk_headerp)pChunkfrom)->iChunkname != MNG_UINT_evNT)
    MNG_ERROR (pData, MNG_WRONGCHUNK); /* ouch */

  ((mng_evntp)pChunkto)->iCount = ((mng_evntp)pChunkfrom)->iCount;

  if (((mng_evntp)pChunkto)->iCount)
  {
    mng_uint32      iX;
    mng_evnt_entryp pEntry;
    mng_uint32      iLen = ((mng_evntp)pChunkto)->iCount * sizeof (mng_evnt_entry);

    MNG_ALLOC (pData, ((mng_evntp)pChunkto)->pEntries, iLen);
    MNG_COPY  (((mng_evntp)pChunkto)->pEntries, ((mng_evntp)pChunkfrom)->pEntries, iLen);

    pEntry = ((mng_evntp)pChunkto)->pEntries;

    for (iX = 0; iX < ((mng_evntp)pChunkto)->iCount; iX++)
    {
      if (pEntry->iSegmentnamesize)
      {
        mng_pchar pTemp = pEntry->zSegmentname;

        MNG_ALLOC (pData, pEntry->zSegmentname, pEntry->iSegmentnamesize+1);
        MNG_COPY  (pEntry->zSegmentname, pTemp, pEntry->iSegmentnamesize);
      }
      else
      {
        pEntry->zSegmentname = MNG_NULL;
      }

      pEntry++;
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_EVNT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

ASSIGN_CHUNK_HDR (mng_assign_unknown)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_UNKNOWN, MNG_LC_START);
#endif

  ((mng_unknown_chunkp)pChunkto)->iDatasize = ((mng_unknown_chunkp)pChunkfrom)->iDatasize;

  if (((mng_unknown_chunkp)pChunkto)->iDatasize)
  {
    MNG_ALLOC (pData, ((mng_unknown_chunkp)pChunkto)->pData, ((mng_unknown_chunkp)pChunkto)->iDatasize);
    MNG_COPY  (((mng_unknown_chunkp)pChunkto)->pData, ((mng_unknown_chunkp)pChunkfrom)->pData,
               ((mng_unknown_chunkp)pChunkto)->iDatasize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_ASSIGN_UNKNOWN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_INCLUDE_WRITE_PROCS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

