/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_chunk_xs.c         copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : chunk access functions (implementation)                    * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the chunk access functions               * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/06/2000 - G.Juyn                                * */
/* *             - changed and filled iterate-chunk function                * */
/* *             0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - fixed calling convention                                 * */
/* *             - added getchunk functions                                 * */
/* *             - added putchunk functions                                 * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/11/2000 - G.Juyn                                * */
/* *             - added empty-chunk put-routines                           * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *             0.5.1 - 05/15/2000 - G.Juyn                                * */
/* *             - added getimgdata & putimgdata functions                  * */
/* *                                                                        * */
/* *             0.5.2 - 05/19/2000 - G.Juyn                                * */
/* *             - B004 - fixed problem with MNG_SUPPORT_WRITE not defined  * */
/* *               also for MNG_SUPPORT_WRITE without MNG_INCLUDE_JNG       * */
/* *             - Cleaned up some code regarding mixed support             * */
/* *                                                                        * */
/* *             0.9.1 - 07/19/2000 - G.Juyn                                * */
/* *             - fixed creation-code                                      * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *             - added function to set simplicity field                   * */
/* *             - fixed putchunk_unknown() function                        * */
/* *                                                                        * */
/* *             0.9.3 - 08/07/2000 - G.Juyn                                * */
/* *             - B111300 - fixup for improved portability                 * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 10/20/2000 - G.Juyn                                * */
/* *             - fixed putchunk_plte() to set bEmpty parameter            * */
/* *                                                                        * */
/* *             0.9.5 - 01/25/2001 - G.Juyn                                * */
/* *             - fixed some small compiler warnings (thanks Nikki)        * */
/* *                                                                        * */
/* *             1.0.5 - 09/07/2002 - G.Juyn                                * */
/* *             - B578940 - unimplemented functions return errorcode       * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             - added HLAPI function to copy chunks                      * */
/* *             1.0.5 - 09/14/2002 - G.Juyn                                * */
/* *             - added event handling for dynamic MNG                     * */
/* *             1.0.5 - 10/07/2002 - G.Juyn                                * */
/* *             - added check for TERM placement during create/write       * */
/* *             1.0.5 - 11/28/2002 - G.Juyn                                * */
/* *             - fixed definition of iMethodX/Y for MAGN chunk            * */
/* *                                                                        * */
/* *             1.0.6 - 05/25/2003 - G.R-P                                 * */
/* *             - added MNG_SKIPCHUNK_cHNK footprint optimizations         * */
/* *             1.0.6 - 07/07/2003 - G.R-P                                 * */
/* *             - added MNG_NO_DELTA_PNG reduction and more SKIPCHUNK      * */
/* *               optimizations                                            * */
/* *             1.0.6 - 07/29/2003 - G.R-P                                 * */
/* *             - added conditionals around PAST chunk support             * */
/* *             1.0.6 - 08/17/2003 - G.R-P                                 * */
/* *             - added conditionals around non-VLC chunk support          * */
/* *                                                                        * */
/* *             1.0.8 - 04/01/2004 - G.Juyn                                * */
/* *             - added missing get-/put-chunk-jdaa                        * */
/* *             1.0.8 - 08/02/2004 - G.Juyn                                * */
/* *             - added conditional to allow easier writing of large MNG's * */
/* *                                                                        * */
/* *             1.0.9 - 09/17/2004 - G.R-P                                 * */
/* *             - added two more conditionals                              * */
/* *             1.0.9 - 09/25/2004 - G.Juyn                                * */
/* *             - replaced MNG_TWEAK_LARGE_FILES with permanent solution   * */
/* *             1.0.9 - 17/14/2004 - G.Juyn                                * */
/* *             - fixed PPLT getchunk/putchunk routines                    * */
/* *             1.0.9 - 12/05/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_CHUNKINITFREE             * */
/* *             1.0.9 - 12/20/2004 - G.Juyn                                * */
/* *             - cleaned up macro-invocations (thanks to D. Airlie)       * */
/* *                                                                        * */
/* *             1.0.10 - 04/08/2007 - G.Juyn                               * */
/* *             - added support for mPNG proposal                          * */
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
#ifdef MNG_OPTIMIZE_CHUNKREADER
#include "libmng_chunk_descr.h"
#endif
#include "libmng_chunk_prc.h"
#include "libmng_chunk_io.h"

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */

#ifdef MNG_ACCESS_CHUNKS

/* ************************************************************************** */

mng_retcode MNG_DECL mng_iterate_chunks (mng_handle       hHandle,
                                         mng_uint32       iChunkseq,
                                         mng_iteratechunk fProc)
{
  mng_uint32  iSeq;
  mng_chunkid iChunkname;
  mng_datap   pData;
  mng_chunkp  pChunk;
  mng_bool    bCont;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_ITERATE_CHUNKS, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = ((mng_datap)hHandle);        /* and make it addressable */

  iSeq   = 0;
  bCont  = MNG_TRUE;
  pChunk = pData->pFirstchunk;         /* get the first chunk */
                                       /* as long as there are some more */
  while ((pChunk) && (bCont))          /* and the app didn't signal a stop */
  {
    if (iSeq >= iChunkseq)             /* reached the first target ? */
    {                                  /* then call this and next ones back in... */
      iChunkname = ((mng_chunk_headerp)pChunk)->iChunkname;
      bCont      = fProc (hHandle, (mng_handle)pChunk, iChunkname, iSeq);
    }

    iSeq++;                            /* next one */
    pChunk = ((mng_chunk_headerp)pChunk)->pNext;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_ITERATE_CHUNKS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifdef MNG_SUPPORT_WRITE
mng_retcode MNG_DECL mng_copy_chunk (mng_handle hHandle,
                                     mng_handle hChunk,
                                     mng_handle hHandleOut)
{
  mng_datap   pDataOut;
  mng_chunkp  pChunk;
  mng_chunkp  pChunkOut;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_COPY_CHUNK, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handles */
  MNG_VALIDHANDLE (hHandleOut)

  pDataOut = (mng_datap)hHandleOut;    /* make outhandle addressable */
  pChunk   = (mng_chunkp)hChunk;       /* address the chunk */

  if (!pDataOut->bCreating)            /* aren't we creating a new file ? */
    MNG_ERROR (pDataOut, MNG_FUNCTIONINVALID)
                                       /* create a new chunk */
  iRetcode = ((mng_createchunk)((mng_chunk_headerp)pChunk)->fCreate)
                        (pDataOut, ((mng_chunk_headerp)pChunk), &pChunkOut);
  if (!iRetcode)                       /* assign the chunk-specific data */
    iRetcode = ((mng_assignchunk)((mng_chunk_headerp)pChunk)->fAssign)
                          (pDataOut, pChunkOut, pChunk);

  if (iRetcode)                        /* on error bail out */
    return iRetcode; 

  mng_add_chunk (pDataOut, pChunkOut); /* and put it in the output-stream */

                                       /* could it be the end of the chain ? */
  if (((mng_chunk_headerp)pChunkOut)->iChunkname == MNG_UINT_IEND)
  {
#ifdef MNG_INCLUDE_JNG
    if ((pDataOut->iFirstchunkadded == MNG_UINT_IHDR) ||
        (pDataOut->iFirstchunkadded == MNG_UINT_JHDR)    )
#else
    if (pDataOut->iFirstchunkadded == MNG_UINT_IHDR)
#endif
      pDataOut->bCreating = MNG_FALSE; /* right; this should be the last chunk !!! */
  }

  if (((mng_chunk_headerp)pChunkOut)->iChunkname == MNG_UINT_MEND)
    pDataOut->bCreating = MNG_FALSE;   /* definitely this should be the last !!! */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_COPY_CHUNK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_WRITE */

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_ihdr (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iWidth,
                                        mng_uint32 *iHeight,
                                        mng_uint8  *iBitdepth,
                                        mng_uint8  *iColortype,
                                        mng_uint8  *iCompression,
                                        mng_uint8  *iFilter,
                                        mng_uint8  *iInterlace)
{
  mng_datap pData;
  mng_ihdrp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_IHDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_ihdrp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_IHDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iWidth       = pChunk->iWidth;      /* fill the fields */
  *iHeight      = pChunk->iHeight;
  *iBitdepth    = pChunk->iBitdepth;
  *iColortype   = pChunk->iColortype;
  *iCompression = pChunk->iCompression;
  *iFilter      = pChunk->iFilter;
  *iInterlace   = pChunk->iInterlace;

                                       /* fill the chunk */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_IHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_plte (mng_handle   hHandle,
                                        mng_handle   hChunk,
                                        mng_uint32   *iCount,
                                        mng_palette8 *aPalette)
{
  mng_datap pData;
  mng_pltep pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PLTE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_pltep)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_PLTE)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iCount = pChunk->iEntrycount;       /* fill the fields */

  MNG_COPY (*aPalette, pChunk->aEntries, sizeof (mng_palette8));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PLTE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_idat (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iRawlen,
                                        mng_ptr    *pRawdata)
{
  mng_datap pData;
  mng_idatp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_IDAT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_idatp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_IDAT)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iRawlen  = pChunk->iDatasize;       /* fill the fields */
  *pRawdata = pChunk->pData;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_IDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_trns (mng_handle   hHandle,
                                        mng_handle   hChunk,
                                        mng_bool     *bEmpty,
                                        mng_bool     *bGlobal,
                                        mng_uint8    *iType,
                                        mng_uint32   *iCount,
                                        mng_uint8arr *aAlphas,
                                        mng_uint16   *iGray,
                                        mng_uint16   *iRed,
                                        mng_uint16   *iGreen,
                                        mng_uint16   *iBlue,
                                        mng_uint32   *iRawlen,
                                        mng_uint8arr *aRawdata)
{
  mng_datap pData;
  mng_trnsp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_TRNS, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_trnsp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_tRNS)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty   = pChunk->bEmpty;          /* fill the fields */
  *bGlobal  = pChunk->bGlobal;
  *iType    = pChunk->iType;
  *iCount   = pChunk->iCount;
  *iGray    = pChunk->iGray;
  *iRed     = pChunk->iRed;
  *iGreen   = pChunk->iGreen;
  *iBlue    = pChunk->iBlue;
  *iRawlen  = pChunk->iRawlen;

  MNG_COPY (*aAlphas,  pChunk->aEntries, sizeof (mng_uint8arr));
  MNG_COPY (*aRawdata, pChunk->aRawdata, sizeof (mng_uint8arr));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_TRNS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_gAMA
mng_retcode MNG_DECL mng_getchunk_gama (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint32 *iGamma)
{
  mng_datap pData;
  mng_gamap pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_GAMA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_gamap)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_gAMA)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty = pChunk->bEmpty;            /* fill the fields */
  *iGamma = pChunk->iGamma;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_GAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_cHRM
mng_retcode MNG_DECL mng_getchunk_chrm (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint32 *iWhitepointx,
                                        mng_uint32 *iWhitepointy,
                                        mng_uint32 *iRedx,
                                        mng_uint32 *iRedy,
                                        mng_uint32 *iGreenx,
                                        mng_uint32 *iGreeny,
                                        mng_uint32 *iBluex,
                                        mng_uint32 *iBluey)
{
  mng_datap pData;
  mng_chrmp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_CHRM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_chrmp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_cHRM)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty       = pChunk->bEmpty;      /* fill the fields */     
  *iWhitepointx = pChunk->iWhitepointx;
  *iWhitepointy = pChunk->iWhitepointy;
  *iRedx        = pChunk->iRedx;
  *iRedy        = pChunk->iRedy;
  *iGreenx      = pChunk->iGreenx;
  *iGreeny      = pChunk->iGreeny;
  *iBluex       = pChunk->iBluex;
  *iBluey       = pChunk->iBluey;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_CHRM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sRGB
mng_retcode MNG_DECL mng_getchunk_srgb (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint8  *iRenderingintent)
{
  mng_datap pData;
  mng_srgbp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SRGB, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_srgbp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_sRGB)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty           = pChunk->bEmpty;  /* fill the fields */        
  *iRenderingintent = pChunk->iRenderingintent;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
mng_retcode MNG_DECL mng_getchunk_iccp (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint32 *iNamesize,
                                        mng_pchar  *zName,
                                        mng_uint8  *iCompression,
                                        mng_uint32 *iProfilesize,
                                        mng_ptr    *pProfile)
{
  mng_datap pData;
  mng_iccpp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ICCP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_iccpp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_iCCP)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty       = pChunk->bEmpty;      /* fill the fields */     
  *iNamesize    = pChunk->iNamesize;
  *zName        = pChunk->zName;
  *iCompression = pChunk->iCompression;
  *iProfilesize = pChunk->iProfilesize;
  *pProfile     = pChunk->pProfile;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ICCP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tEXt
mng_retcode MNG_DECL mng_getchunk_text (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iKeywordsize,
                                        mng_pchar  *zKeyword,
                                        mng_uint32 *iTextsize,
                                        mng_pchar  *zText)
{
  mng_datap pData;
  mng_textp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_TEXT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_textp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_tEXt)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */
                                       /* fill the fields */
  *iKeywordsize = pChunk->iKeywordsize;
  *zKeyword     = pChunk->zKeyword;
  *iTextsize    = pChunk->iTextsize;
  *zText        = pChunk->zText;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_TEXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_zTXt
mng_retcode MNG_DECL mng_getchunk_ztxt (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iKeywordsize,
                                        mng_pchar  *zKeyword,
                                        mng_uint8  *iCompression,
                                        mng_uint32 *iTextsize,
                                        mng_pchar  *zText)
{
  mng_datap pData;
  mng_ztxtp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ZTXT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_ztxtp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_zTXt)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */
                                       /* fill the fields */
  *iKeywordsize = pChunk->iKeywordsize;
  *zKeyword     = pChunk->zKeyword;
  *iCompression = pChunk->iCompression;
  *iTextsize    = pChunk->iTextsize;
  *zText        = pChunk->zText;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ZTXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iTXt
mng_retcode MNG_DECL mng_getchunk_itxt (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iKeywordsize,
                                        mng_pchar  *zKeyword,
                                        mng_uint8  *iCompressionflag,
                                        mng_uint8  *iCompressionmethod,
                                        mng_uint32 *iLanguagesize,
                                        mng_pchar  *zLanguage,
                                        mng_uint32 *iTranslationsize,
                                        mng_pchar  *zTranslation,
                                        mng_uint32 *iTextsize,
                                        mng_pchar  *zText)
{
  mng_datap pData;
  mng_itxtp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ITXT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_itxtp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_iTXt)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */
                                       /* fill the fields */
  *iKeywordsize       = pChunk->iKeywordsize;
  *zKeyword           = pChunk->zKeyword;
  *iCompressionflag   = pChunk->iCompressionflag;
  *iCompressionmethod = pChunk->iCompressionmethod;
  *iLanguagesize      = pChunk->iLanguagesize;
  *zLanguage          = pChunk->zLanguage;
  *iTranslationsize   = pChunk->iTranslationsize;
  *zTranslation       = pChunk->zTranslation;
  *iTextsize          = pChunk->iTextsize;
  *zText              = pChunk->zText;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ITXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_bKGD
mng_retcode MNG_DECL mng_getchunk_bkgd (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint8  *iType,
                                        mng_uint8  *iIndex,
                                        mng_uint16 *iGray,
                                        mng_uint16 *iRed,
                                        mng_uint16 *iGreen,
                                        mng_uint16 *iBlue)
{
  mng_datap pData;
  mng_bkgdp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_BKGD, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_bkgdp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_bKGD)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty = pChunk->bEmpty;            /* fill the fields */
  *iType  = pChunk->iType;
  *iIndex = pChunk->iIndex;
  *iGray  = pChunk->iGray;
  *iRed   = pChunk->iRed;
  *iGreen = pChunk->iGreen;
  *iBlue  = pChunk->iBlue;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_BKGD, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYs
mng_retcode MNG_DECL mng_getchunk_phys (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint32 *iSizex,
                                        mng_uint32 *iSizey,
                                        mng_uint8  *iUnit)
{
  mng_datap pData;
  mng_physp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PHYS, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_physp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_pHYs)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty = pChunk->bEmpty;            /* fill the fields */
  *iSizex = pChunk->iSizex;
  *iSizey = pChunk->iSizey;
  *iUnit  = pChunk->iUnit;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PHYS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sBIT
mng_retcode MNG_DECL mng_getchunk_sbit (mng_handle    hHandle,
                                        mng_handle    hChunk,
                                        mng_bool      *bEmpty,
                                        mng_uint8     *iType,
                                        mng_uint8arr4 *aBits)
{
  mng_datap pData;
  mng_sbitp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SBIT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_sbitp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_sBIT)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty     = pChunk->bEmpty;
  *iType      = pChunk->iType;
  (*aBits)[0] = pChunk->aBits[0];
  (*aBits)[1] = pChunk->aBits[1];
  (*aBits)[2] = pChunk->aBits[2];
  (*aBits)[3] = pChunk->aBits[3];

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SBIT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
mng_retcode MNG_DECL mng_getchunk_splt (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint32 *iNamesize,
                                        mng_pchar  *zName,
                                        mng_uint8  *iSampledepth,
                                        mng_uint32 *iEntrycount,
                                        mng_ptr    *pEntries)
{
  mng_datap pData;
  mng_spltp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SPLT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_spltp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_sPLT)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty       = pChunk->bEmpty;      /* fill the fields */      
  *iNamesize    = pChunk->iNamesize;
  *zName        = pChunk->zName;
  *iSampledepth = pChunk->iSampledepth;
  *iEntrycount  = pChunk->iEntrycount;
  *pEntries     = pChunk->pEntries;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_hIST
mng_retcode MNG_DECL mng_getchunk_hist (mng_handle    hHandle,
                                        mng_handle    hChunk,
                                        mng_uint32    *iEntrycount,
                                        mng_uint16arr *aEntries)
{
  mng_datap pData;
  mng_histp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_HIST, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_histp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_hIST)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iEntrycount = pChunk->iEntrycount;  /* fill the fields */

  MNG_COPY (*aEntries, pChunk->aEntries, sizeof (mng_uint16arr));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_HIST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tIME
mng_retcode MNG_DECL mng_getchunk_time (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iYear,
                                        mng_uint8  *iMonth,
                                        mng_uint8  *iDay,
                                        mng_uint8  *iHour,
                                        mng_uint8  *iMinute,
                                        mng_uint8  *iSecond)
{
  mng_datap pData;
  mng_timep pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_TIME, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_timep)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_tIME)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iYear   = pChunk->iYear;            /* fill the fields */ 
  *iMonth  = pChunk->iMonth;
  *iDay    = pChunk->iDay;
  *iHour   = pChunk->iHour;
  *iMinute = pChunk->iMinute;
  *iSecond = pChunk->iSecond;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_TIME, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_mhdr (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iWidth,
                                        mng_uint32 *iHeight,
                                        mng_uint32 *iTicks,
                                        mng_uint32 *iLayercount,
                                        mng_uint32 *iFramecount,
                                        mng_uint32 *iPlaytime,
                                        mng_uint32 *iSimplicity)
{
  mng_datap pData;
  mng_mhdrp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MHDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_mhdrp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iWidth      = pChunk->iWidth;       /* fill the fields */   
  *iHeight     = pChunk->iHeight;
  *iTicks      = pChunk->iTicks;
  *iLayercount = pChunk->iLayercount;
  *iFramecount = pChunk->iFramecount;
  *iPlaytime   = pChunk->iPlaytime;
  *iSimplicity = pChunk->iSimplicity;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
mng_retcode MNG_DECL mng_getchunk_loop (mng_handle  hHandle,
                                        mng_handle  hChunk,
                                        mng_uint8   *iLevel,
                                        mng_uint32  *iRepeat,
                                        mng_uint8   *iTermination,
                                        mng_uint32  *iItermin,
                                        mng_uint32  *iItermax,
                                        mng_uint32  *iCount,
                                        mng_uint32p *pSignals)
{
  mng_datap pData;
  mng_loopp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_LOOP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_loopp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_LOOP)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iLevel       = pChunk->iLevel;      /* fill teh fields */
  *iRepeat      = pChunk->iRepeat;
  *iTermination = pChunk->iTermination;
  *iItermin     = pChunk->iItermin;
  *iItermax     = pChunk->iItermax;
  *iCount       = pChunk->iCount;
  *pSignals     = pChunk->pSignals;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_LOOP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_endl (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint8  *iLevel)
{
  mng_datap pData;
  mng_endlp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ENDL, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_endlp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_ENDL)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iLevel = pChunk->iLevel;            /* fill the field */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ENDL, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DEFI
mng_retcode MNG_DECL mng_getchunk_defi (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iObjectid,
                                        mng_uint8  *iDonotshow,
                                        mng_uint8  *iConcrete,
                                        mng_bool   *bHasloca,
                                        mng_int32  *iXlocation,
                                        mng_int32  *iYlocation,
                                        mng_bool   *bHasclip,
                                        mng_int32  *iLeftcb,
                                        mng_int32  *iRightcb,
                                        mng_int32  *iTopcb,
                                        mng_int32  *iBottomcb)
{
  mng_datap pData;
  mng_defip pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DEFI, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_defip)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_DEFI)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iObjectid  = pChunk->iObjectid;     /* fill the fields */
  *iDonotshow = pChunk->iDonotshow;
  *iConcrete  = pChunk->iConcrete;
  *bHasloca   = pChunk->bHasloca;
  *iXlocation = pChunk->iXlocation;
  *iYlocation = pChunk->iYlocation;
  *bHasclip   = pChunk->bHasclip;
  *iLeftcb    = pChunk->iLeftcb;
  *iRightcb   = pChunk->iRightcb;
  *iTopcb     = pChunk->iTopcb;
  *iBottomcb  = pChunk->iBottomcb;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BASI
mng_retcode MNG_DECL mng_getchunk_basi (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iWidth,
                                        mng_uint32 *iHeight,
                                        mng_uint8  *iBitdepth,
                                        mng_uint8  *iColortype,
                                        mng_uint8  *iCompression,
                                        mng_uint8  *iFilter,
                                        mng_uint8  *iInterlace,
                                        mng_uint16 *iRed,
                                        mng_uint16 *iGreen,
                                        mng_uint16 *iBlue,
                                        mng_uint16 *iAlpha,
                                        mng_uint8  *iViewable)
{
  mng_datap pData;
  mng_basip pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_BASI, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_basip)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_BASI)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iWidth       = pChunk->iWidth;      /* fill the fields */
  *iHeight      = pChunk->iHeight;
  *iBitdepth    = pChunk->iBitdepth;
  *iColortype   = pChunk->iColortype;
  *iCompression = pChunk->iCompression;
  *iFilter      = pChunk->iFilter;
  *iInterlace   = pChunk->iInterlace;
  *iRed         = pChunk->iRed;
  *iGreen       = pChunk->iGreen;
  *iBlue        = pChunk->iBlue;
  *iAlpha       = pChunk->iAlpha;
  *iViewable    = pChunk->iViewable;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_BASI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLON
mng_retcode MNG_DECL mng_getchunk_clon (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iSourceid,
                                        mng_uint16 *iCloneid,
                                        mng_uint8  *iClonetype,
                                        mng_uint8  *iDonotshow,
                                        mng_uint8  *iConcrete,
                                        mng_bool   *bHasloca,
                                        mng_uint8  *iLocationtype,
                                        mng_int32  *iLocationx,
                                        mng_int32  *iLocationy)
{
  mng_datap pData;
  mng_clonp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_CLON, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_clonp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_CLON)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iSourceid     = pChunk->iSourceid;  /* fill the fields */  
  *iCloneid      = pChunk->iCloneid;
  *iClonetype    = pChunk->iClonetype;
  *iDonotshow    = pChunk->iDonotshow;
  *iConcrete     = pChunk->iConcrete;
  *bHasloca      = pChunk->bHasloca;
  *iLocationtype = pChunk->iLocationtype;
  *iLocationx    = pChunk->iLocationx;
  *iLocationy    = pChunk->iLocationy;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_CLON, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode MNG_DECL mng_getchunk_past (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iDestid,
                                        mng_uint8  *iTargettype,
                                        mng_int32  *iTargetx,
                                        mng_int32  *iTargety,
                                        mng_uint32 *iCount)
{
  mng_datap pData;
  mng_pastp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PAST, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_pastp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_PAST)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iDestid     = pChunk->iDestid;       /* fill the fields */
  *iTargettype = pChunk->iTargettype;
  *iTargetx    = pChunk->iTargetx;
  *iTargety    = pChunk->iTargety;
  *iCount      = pChunk->iCount;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PAST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode MNG_DECL mng_getchunk_past_src (mng_handle hHandle,
                                            mng_handle hChunk,
                                            mng_uint32 iEntry,
                                            mng_uint16 *iSourceid,
                                            mng_uint8  *iComposition,
                                            mng_uint8  *iOrientation,
                                            mng_uint8  *iOffsettype,
                                            mng_int32  *iOffsetx,
                                            mng_int32  *iOffsety,
                                            mng_uint8  *iBoundarytype,
                                            mng_int32  *iBoundaryl,
                                            mng_int32  *iBoundaryr,
                                            mng_int32  *iBoundaryt,
                                            mng_int32  *iBoundaryb)
{
  mng_datap        pData;
  mng_pastp        pChunk;
  mng_past_sourcep pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PAST_SRC, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_pastp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_PAST)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  if (iEntry >= pChunk->iCount)        /* valid index ? */
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)
                                       /* address the entry */
  pEntry         = pChunk->pSources + iEntry;

  *iSourceid     = pEntry->iSourceid;  /* fill the fields */
  *iComposition  = pEntry->iComposition;
  *iOrientation  = pEntry->iOrientation;
  *iOffsettype   = pEntry->iOffsettype;
  *iOffsetx      = pEntry->iOffsetx;
  *iOffsety      = pEntry->iOffsety;
  *iBoundarytype = pEntry->iBoundarytype;
  *iBoundaryl    = pEntry->iBoundaryl;
  *iBoundaryr    = pEntry->iBoundaryr;
  *iBoundaryt    = pEntry->iBoundaryt;
  *iBoundaryb    = pEntry->iBoundaryb;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PAST_SRC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
mng_retcode MNG_DECL mng_getchunk_disc (mng_handle  hHandle,
                                        mng_handle  hChunk,
                                        mng_uint32  *iCount,
                                        mng_uint16p *pObjectids)
{
  mng_datap pData;
  mng_discp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DISC, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_discp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_DISC)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iCount     = pChunk->iCount;        /* fill the fields */
  *pObjectids = pChunk->pObjectids;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DISC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BACK
mng_retcode MNG_DECL mng_getchunk_back (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iRed,
                                        mng_uint16 *iGreen,
                                        mng_uint16 *iBlue,
                                        mng_uint8  *iMandatory,
                                        mng_uint16 *iImageid,
                                        mng_uint8  *iTile)
{
  mng_datap pData;
  mng_backp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_BACK, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_backp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_BACK)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iRed       = pChunk->iRed;          /* fill the fields */
  *iGreen     = pChunk->iGreen;
  *iBlue      = pChunk->iBlue;
  *iMandatory = pChunk->iMandatory;
  *iImageid   = pChunk->iImageid;
  *iTile      = pChunk->iTile;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_BACK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
mng_retcode MNG_DECL mng_getchunk_fram (mng_handle  hHandle,
                                        mng_handle  hChunk,
                                        mng_bool    *bEmpty,
                                        mng_uint8   *iMode,
                                        mng_uint32  *iNamesize,
                                        mng_pchar   *zName,
                                        mng_uint8   *iChangedelay,
                                        mng_uint8   *iChangetimeout,
                                        mng_uint8   *iChangeclipping,
                                        mng_uint8   *iChangesyncid,
                                        mng_uint32  *iDelay,
                                        mng_uint32  *iTimeout,
                                        mng_uint8   *iBoundarytype,
                                        mng_int32   *iBoundaryl,
                                        mng_int32   *iBoundaryr,
                                        mng_int32   *iBoundaryt,
                                        mng_int32   *iBoundaryb,
                                        mng_uint32  *iCount,
                                        mng_uint32p *pSyncids)
{
  mng_datap pData;
  mng_framp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_FRAM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_framp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_FRAM)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty          = pChunk->bEmpty;   /* fill the fields */      
  *iMode           = pChunk->iMode;
  *iNamesize       = pChunk->iNamesize;
  *zName           = pChunk->zName;
  *iChangedelay    = pChunk->iChangedelay;
  *iChangetimeout  = pChunk->iChangetimeout;
  *iChangeclipping = pChunk->iChangeclipping;
  *iChangesyncid   = pChunk->iChangesyncid;
  *iDelay          = pChunk->iDelay;
  *iTimeout        = pChunk->iTimeout;
  *iBoundarytype   = pChunk->iBoundarytype;
  *iBoundaryl      = pChunk->iBoundaryl;
  *iBoundaryr      = pChunk->iBoundaryr;
  *iBoundaryt      = pChunk->iBoundaryt;
  *iBoundaryb      = pChunk->iBoundaryb;
  *iCount          = pChunk->iCount;
  *pSyncids        = pChunk->pSyncids;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_FRAM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MOVE
mng_retcode MNG_DECL mng_getchunk_move (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iFirstid,
                                        mng_uint16 *iLastid,
                                        mng_uint8  *iMovetype,
                                        mng_int32  *iMovex,
                                        mng_int32  *iMovey)
{
  mng_datap pData;
  mng_movep pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MOVE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_movep)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_MOVE)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iFirstid  = pChunk->iFirstid;       /* fill the fields */
  *iLastid   = pChunk->iLastid;
  *iMovetype = pChunk->iMovetype;
  *iMovex    = pChunk->iMovex;
  *iMovey    = pChunk->iMovey;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MOVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLIP
mng_retcode MNG_DECL mng_getchunk_clip (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iFirstid,
                                        mng_uint16 *iLastid,
                                        mng_uint8  *iCliptype,
                                        mng_int32  *iClipl,
                                        mng_int32  *iClipr,
                                        mng_int32  *iClipt,
                                        mng_int32  *iClipb)
{
  mng_datap pData;
  mng_clipp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_CLIP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_clipp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_CLIP)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iFirstid  = pChunk->iFirstid;       /* fill the fields */
  *iLastid   = pChunk->iLastid;
  *iCliptype = pChunk->iCliptype;
  *iClipl    = pChunk->iClipl;
  *iClipr    = pChunk->iClipr;
  *iClipt    = pChunk->iClipt;
  *iClipb    = pChunk->iClipb;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_CLIP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SHOW
mng_retcode MNG_DECL mng_getchunk_show (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint16 *iFirstid,
                                        mng_uint16 *iLastid,
                                        mng_uint8  *iMode)
{
  mng_datap pData;
  mng_showp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SHOW, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_showp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_SHOW)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty   = pChunk->bEmpty;          /* fill the fields */
  *iFirstid = pChunk->iFirstid;
  *iLastid  = pChunk->iLastid;
  *iMode    = pChunk->iMode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SHOW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_TERM
mng_retcode MNG_DECL mng_getchunk_term (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint8  *iTermaction,
                                        mng_uint8  *iIteraction,
                                        mng_uint32 *iDelay,
                                        mng_uint32 *iItermax)
{
  mng_datap pData;
  mng_termp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_TERM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_termp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_TERM)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iTermaction = pChunk->iTermaction;  /* fill the fields */
  *iIteraction = pChunk->iIteraction;
  *iDelay      = pChunk->iDelay;
  *iItermax    = pChunk->iItermax;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_TERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode MNG_DECL mng_getchunk_save (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint8  *iOffsettype,
                                        mng_uint32 *iCount)
{
  mng_datap pData;
  mng_savep pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SAVE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_savep)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_SAVE)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty      = pChunk->bEmpty;       /* fill the fields */
  *iOffsettype = pChunk->iOffsettype;
  *iCount      = pChunk->iCount;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_save_entry (mng_handle     hHandle,
                                              mng_handle     hChunk,
                                              mng_uint32     iEntry,
                                              mng_uint8      *iEntrytype,
                                              mng_uint32arr2 *iOffset,
                                              mng_uint32arr2 *iStarttime,
                                              mng_uint32     *iLayernr,
                                              mng_uint32     *iFramenr,
                                              mng_uint32     *iNamesize,
                                              mng_pchar      *zName)
{
  mng_datap       pData;
  mng_savep       pChunk;
  mng_save_entryp pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SAVE_ENTRY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_savep)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_SAVE)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  if (iEntry >= pChunk->iCount)        /* valid index ? */
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)

  pEntry  = pChunk->pEntries + iEntry; /* address the entry */
                                       /* fill the fields */
  *iEntrytype      = pEntry->iEntrytype;
  (*iOffset)[0]    = pEntry->iOffset[0];
  (*iOffset)[1]    = pEntry->iOffset[1];
  (*iStarttime)[0] = pEntry->iStarttime[0];
  (*iStarttime)[1] = pEntry->iStarttime[1];
  *iLayernr        = pEntry->iLayernr;
  *iFramenr        = pEntry->iFramenr;
  *iNamesize       = pEntry->iNamesize;
  *zName           = pEntry->zName;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SAVE_ENTRY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode MNG_DECL mng_getchunk_seek (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iNamesize,
                                        mng_pchar  *zName)
{
  mng_datap pData;
  mng_seekp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SEEK, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_seekp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_SEEK)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iNamesize = pChunk->iNamesize;      /* fill the fields */
  *zName     = pChunk->zName;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_SEEK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_eXPI
mng_retcode MNG_DECL mng_getchunk_expi (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iSnapshotid,
                                        mng_uint32 *iNamesize,
                                        mng_pchar  *zName)
{
  mng_datap pData;
  mng_expip pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_EXPI, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_expip)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_eXPI)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iSnapshotid = pChunk->iSnapshotid;  /* fill the fields */
  *iNamesize   = pChunk->iNamesize;
  *zName       = pChunk->zName;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_EXPI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_fPRI
mng_retcode MNG_DECL mng_getchunk_fpri (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint8  *iDeltatype,
                                        mng_uint8  *iPriority)
{
  mng_datap pData;
  mng_fprip pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_FPRI, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_fprip)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_fPRI)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iDeltatype = pChunk->iDeltatype;    /* fill the fields */
  *iPriority  = pChunk->iPriority;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_FPRI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
mng_retcode MNG_DECL mng_getchunk_need (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iKeywordssize,
                                        mng_pchar  *zKeywords)
{
  mng_datap pData;
  mng_needp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_NEED, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_needp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_nEED)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */
                                       /* fill the fields */
  *iKeywordssize = pChunk->iKeywordssize;
  *zKeywords     = pChunk->zKeywords;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_NEED, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYg
mng_retcode MNG_DECL mng_getchunk_phyg (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_bool   *bEmpty,
                                        mng_uint32 *iSizex,
                                        mng_uint32 *iSizey,
                                        mng_uint8  *iUnit)
{
  mng_datap pData;
  mng_phygp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PHYG, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_phygp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_pHYg)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *bEmpty = pChunk->bEmpty;            /* fill the fields */
  *iSizex = pChunk->iSizex;
  *iSizey = pChunk->iSizey;
  *iUnit  = pChunk->iUnit;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PHYG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

mng_retcode MNG_DECL mng_getchunk_jhdr (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iWidth,
                                        mng_uint32 *iHeight,
                                        mng_uint8  *iColortype,
                                        mng_uint8  *iImagesampledepth,
                                        mng_uint8  *iImagecompression,
                                        mng_uint8  *iImageinterlace,
                                        mng_uint8  *iAlphasampledepth,
                                        mng_uint8  *iAlphacompression,
                                        mng_uint8  *iAlphafilter,
                                        mng_uint8  *iAlphainterlace)
{
  mng_datap pData;
  mng_jhdrp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_JHDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_jhdrp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_JHDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iWidth            = pChunk->iWidth; /* fill the fields */          
  *iHeight           = pChunk->iHeight;
  *iColortype        = pChunk->iColortype;
  *iImagesampledepth = pChunk->iImagesampledepth;
  *iImagecompression = pChunk->iImagecompression;
  *iImageinterlace   = pChunk->iImageinterlace;
  *iAlphasampledepth = pChunk->iAlphasampledepth;
  *iAlphacompression = pChunk->iAlphacompression;
  *iAlphafilter      = pChunk->iAlphafilter;
  *iAlphainterlace   = pChunk->iAlphainterlace;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_JHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

mng_retcode MNG_DECL mng_getchunk_jdat (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iRawlen,
                                        mng_ptr    *pRawdata)
{
  mng_datap pData;
  mng_jdatp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_JDAT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_jdatp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_JDAT)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iRawlen  = pChunk->iDatasize;       /* fill the fields */
  *pRawdata = pChunk->pData;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_JDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

mng_retcode MNG_DECL mng_getchunk_jdaa (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iRawlen,
                                        mng_ptr    *pRawdata)
{
  mng_datap pData;
  mng_jdaap pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_JDAA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_jdaap)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_JDAA)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iRawlen  = pChunk->iDatasize;       /* fill the fields */
  *pRawdata = pChunk->pData;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_JDAA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_getchunk_dhdr (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iObjectid,
                                        mng_uint8  *iImagetype,
                                        mng_uint8  *iDeltatype,
                                        mng_uint32 *iBlockwidth,
                                        mng_uint32 *iBlockheight,
                                        mng_uint32 *iBlockx,
                                        mng_uint32 *iBlocky)
{
  mng_datap pData;
  mng_dhdrp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DHDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_dhdrp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_DHDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iObjectid    = pChunk->iObjectid;   /* fill the fields */
  *iImagetype   = pChunk->iImagetype;
  *iDeltatype   = pChunk->iDeltatype;
  *iBlockwidth  = pChunk->iBlockwidth;
  *iBlockheight = pChunk->iBlockheight;
  *iBlockx      = pChunk->iBlockx;
  *iBlocky      = pChunk->iBlocky;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_getchunk_prom (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint8  *iColortype,
                                        mng_uint8  *iSampledepth,
                                        mng_uint8  *iFilltype)
{
  mng_datap pData;
  mng_promp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PROM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_promp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_PROM)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iColortype   = pChunk->iColortype;  /* fill the fields */
  *iSampledepth = pChunk->iSampledepth;
  *iFilltype    = pChunk->iFilltype;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PROM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_getchunk_pplt (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint8  *iDeltatype,
                                        mng_uint32 *iCount)
{
  mng_datap pData;
  mng_ppltp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PPLT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_ppltp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_PPLT)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iDeltatype = pChunk->iDeltatype;    /* fill the fields */
  *iCount     = pChunk->iCount;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_getchunk_pplt_entry (mng_handle hHandle,
                                              mng_handle hChunk,
                                              mng_uint32 iEntry,
                                              mng_uint16 *iRed,
                                              mng_uint16 *iGreen,
                                              mng_uint16 *iBlue,
                                              mng_uint16 *iAlpha,
                                              mng_bool   *bUsed)
{
  mng_datap       pData;
  mng_ppltp       pChunk;
  mng_pplt_entryp pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PPLT_ENTRY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_ppltp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_PPLT)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  if (iEntry >= pChunk->iCount)        /* valid index ? */
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)

  pEntry  = &pChunk->aEntries[iEntry]; /* address the entry */

  *iRed   = pEntry->iRed;              /* fill the fields */
  *iGreen = pEntry->iGreen;
  *iBlue  = pEntry->iBlue;
  *iAlpha = pEntry->iAlpha;
  *bUsed  = pEntry->bUsed;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_PPLT_ENTRY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_getchunk_drop (mng_handle   hHandle,
                                        mng_handle   hChunk,
                                        mng_uint32   *iCount,
                                        mng_chunkidp *pChunknames)
{
  mng_datap pData;
  mng_dropp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DROP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_dropp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_DROP)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iCount      = pChunk->iCount;       /* fill the fields */
  *pChunknames = pChunk->pChunknames;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DROP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
mng_retcode MNG_DECL mng_getchunk_dbyk (mng_handle  hHandle,
                                        mng_handle  hChunk,
                                        mng_chunkid *iChunkname,
                                        mng_uint8   *iPolarity,
                                        mng_uint32  *iKeywordssize,
                                        mng_pchar   *zKeywords)
{
  mng_datap pData;
  mng_dbykp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DBYK, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_dbykp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_DBYK)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iChunkname    = pChunk->iChunkname; /* fill the fields */  
  *iPolarity     = pChunk->iPolarity;
  *iKeywordssize = pChunk->iKeywordssize;
  *zKeywords     = pChunk->zKeywords;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_DBYK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
mng_retcode MNG_DECL mng_getchunk_ordr (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iCount)
{
  mng_datap pData;
  mng_ordrp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ORDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_ordrp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_ORDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iCount = pChunk->iCount;            /* fill the field */ 

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ORDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
mng_retcode MNG_DECL mng_getchunk_ordr_entry (mng_handle  hHandle,
                                              mng_handle  hChunk,
                                              mng_uint32  iEntry,
                                              mng_chunkid *iChunkname,
                                              mng_uint8   *iOrdertype)
{
  mng_datap       pData;
  mng_ordrp       pChunk;
  mng_ordr_entryp pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ORDR_ENTRY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_ordrp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_ORDR)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  if (iEntry >= pChunk->iCount)        /* valid index ? */
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)

  pEntry = pChunk->pEntries + iEntry;  /* address the proper entry */

  *iChunkname = pEntry->iChunkname;    /* fill the fields */
  *iOrdertype = pEntry->iOrdertype;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_ORDR_ENTRY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
mng_retcode MNG_DECL mng_getchunk_magn (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint16 *iFirstid,
                                        mng_uint16 *iLastid,
                                        mng_uint16 *iMethodX,
                                        mng_uint16 *iMX,
                                        mng_uint16 *iMY,
                                        mng_uint16 *iML,
                                        mng_uint16 *iMR,
                                        mng_uint16 *iMT,
                                        mng_uint16 *iMB,
                                        mng_uint16 *iMethodY)
{
  mng_datap pData;
  mng_magnp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MAGN, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_magnp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_MAGN)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iFirstid = pChunk->iFirstid;        /* fill the fields */
  *iLastid  = pChunk->iLastid;
  *iMethodX = (mng_uint16)pChunk->iMethodX;
  *iMX      = pChunk->iMX;
  *iMY      = pChunk->iMY;
  *iML      = pChunk->iML;
  *iMR      = pChunk->iMR;
  *iMT      = pChunk->iMT;
  *iMB      = pChunk->iMB;
  *iMethodY = (mng_uint16)pChunk->iMethodY;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MAGN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
MNG_EXT mng_retcode MNG_DECL mng_getchunk_mpng (mng_handle hHandle,
                                                mng_handle hChunk,
                                                mng_uint32 *iFramewidth,
                                                mng_uint32 *iFrameheight,
                                                mng_uint16 *iNumplays,
                                                mng_uint16 *iTickspersec,
                                                mng_uint8  *iCompressionmethod,
                                                mng_uint32 *iCount)
{
  mng_datap pData;
  mng_mpngp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MPNG, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_mpngp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_mpNG)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */
                                       /* fill the fields */
  *iFramewidth        = pChunk->iFramewidth;
  *iFrameheight       = pChunk->iFrameheight;
  *iNumplays          = pChunk->iNumplays;
  *iTickspersec       = pChunk->iTickspersec;
  *iCompressionmethod = pChunk->iCompressionmethod;
  *iCount             = pChunk->iFramessize / sizeof (mng_mpng_frame);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
MNG_EXT mng_retcode MNG_DECL mng_getchunk_mpng_frame (mng_handle hHandle,
                                                      mng_handle hChunk,
                                                      mng_uint32 iEntry,
                                                      mng_uint32 *iX,
                                                      mng_uint32 *iY,
                                                      mng_uint32 *iWidth,
                                                      mng_uint32 *iHeight,
                                                      mng_int32  *iXoffset,
                                                      mng_int32  *iYoffset,
                                                      mng_uint16 *iTicks)
{
  mng_datap       pData;
  mng_mpngp       pChunk;
  mng_mpng_framep pFrame;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MPNG_FRAME, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_mpngp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_mpNG)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */
                                       /* valid index ? */
  if (iEntry >= (pChunk->iFramessize / sizeof (mng_mpng_frame)))
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)

  pFrame  = pChunk->pFrames + iEntry;  /* address the entry */
                                       /* fill the fields */
  *iX        = pFrame->iX;
  *iY        = pFrame->iY;
  *iWidth    = pFrame->iWidth;
  *iHeight   = pFrame->iHeight;
  *iXoffset  = pFrame->iXoffset;
  *iYoffset  = pFrame->iYoffset;
  *iTicks    = pFrame->iTicks;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_MPNG_FRAME, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_evNT
mng_retcode MNG_DECL mng_getchunk_evnt (mng_handle hHandle,
                                        mng_handle hChunk,
                                        mng_uint32 *iCount)
{
  mng_datap pData;
  mng_evntp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_EVNT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_evntp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_evNT)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  *iCount = pChunk->iCount;            /* fill the fields */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_EVNT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_evnt_entry (mng_handle hHandle,
                                              mng_handle hChunk,
                                              mng_uint32 iEntry,
                                              mng_uint8  *iEventtype,
                                              mng_uint8  *iMasktype,
                                              mng_int32  *iLeft,
                                              mng_int32  *iRight,
                                              mng_int32  *iTop,
                                              mng_int32  *iBottom,
                                              mng_uint16 *iObjectid,
                                              mng_uint8  *iIndex,
                                              mng_uint32 *iSegmentnamesize,
                                              mng_pchar  *zSegmentname)
{
  mng_datap       pData;
  mng_evntp       pChunk;
  mng_evnt_entryp pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_EVNT_ENTRY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_evntp)hChunk;          /* address the chunk */

  if (pChunk->sHeader.iChunkname != MNG_UINT_evNT)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */

  if (iEntry >= pChunk->iCount)        /* valid index ? */
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)

  pEntry  = pChunk->pEntries + iEntry; /* address the entry */
                                       /* fill the fields */
  *iEventtype       = pEntry->iEventtype;
  *iMasktype        = pEntry->iMasktype;
  *iLeft            = pEntry->iLeft;    
  *iRight           = pEntry->iRight;
  *iTop             = pEntry->iTop;
  *iBottom          = pEntry->iBottom;
  *iObjectid        = pEntry->iObjectid;
  *iIndex           = pEntry->iIndex;
  *iSegmentnamesize = pEntry->iSegmentnamesize;
  *zSegmentname     = pEntry->zSegmentname;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_EVNT_ENTRY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getchunk_unknown (mng_handle  hHandle,
                                           mng_handle  hChunk,
                                           mng_chunkid *iChunkname,
                                           mng_uint32  *iRawlen,
                                           mng_ptr     *pRawdata)
{
  mng_datap          pData;
  mng_unknown_chunkp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_UNKNOWN, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData  = (mng_datap)hHandle;         /* and make it addressable */
  pChunk = (mng_unknown_chunkp)hChunk; /* address the chunk */

  if (pChunk->sHeader.fCleanup != mng_free_unknown)
    MNG_ERROR (pData, MNG_WRONGCHUNK)  /* ouch */
                                       /* fill the fields */
  *iChunkname = pChunk->sHeader.iChunkname;
  *iRawlen    = pChunk->iDatasize;
  *pRawdata   = pChunk->pData;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETCHUNK_UNKNOWN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_WRITE_PROCS

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_TERM
MNG_LOCAL mng_bool check_term (mng_datap   pData,
                               mng_chunkid iChunkname)
{
  mng_chunk_headerp pChunk = (mng_chunk_headerp)pData->pLastchunk;

  if (!pChunk)                         /* nothing added yet ? */
    return MNG_TRUE;
                                       /* last added chunk is TERM ? */
  if (pChunk->iChunkname != MNG_UINT_TERM)
    return MNG_TRUE;
                                       /* previous to last is MHDR ? */
  if ((pChunk->pPrev) && (((mng_chunk_headerp)pChunk->pPrev)->iChunkname == MNG_UINT_MHDR))
    return MNG_TRUE;

  if (iChunkname == MNG_UINT_SEEK)     /* new chunk to be added is SEEK ? */
    return MNG_TRUE;

  return MNG_FALSE;
}
#endif

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_ihdr (mng_handle hHandle,
                                        mng_uint32 iWidth,
                                        mng_uint32 iHeight,
                                        mng_uint8  iBitdepth,
                                        mng_uint8  iColortype,
                                        mng_uint8  iCompression,
                                        mng_uint8  iFilter,
                                        mng_uint8  iInterlace)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_IHDR, mng_init_general, mng_free_general, mng_read_ihdr, mng_write_ihdr, mng_assign_general, 0, 0, sizeof(mng_ihdr)};
#else
          {MNG_UINT_IHDR, mng_init_ihdr, mng_free_ihdr, mng_read_ihdr, mng_write_ihdr, mng_assign_ihdr, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IHDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_IHDR))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_ihdr (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_IHDR, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
  ((mng_ihdrp)pChunk)->iWidth       = iWidth;
  ((mng_ihdrp)pChunk)->iHeight      = iHeight;
  ((mng_ihdrp)pChunk)->iBitdepth    = iBitdepth;
  ((mng_ihdrp)pChunk)->iColortype   = iColortype;
  ((mng_ihdrp)pChunk)->iCompression = iCompression;
  ((mng_ihdrp)pChunk)->iFilter      = iFilter;
  ((mng_ihdrp)pChunk)->iInterlace   = iInterlace;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_plte (mng_handle   hHandle,
                                        mng_uint32   iCount,
                                        mng_palette8 aPalette)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_PLTE, mng_init_general, mng_free_general, mng_read_plte, mng_write_plte, mng_assign_general, 0, 0, sizeof(mng_plte)};
#else
          {MNG_UINT_PLTE, mng_init_plte, mng_free_plte, mng_read_plte, mng_write_plte, mng_assign_plte, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PLTE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_PLTE))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_plte (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_PLTE, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_pltep)pChunk)->iEntrycount = iCount;
  ((mng_pltep)pChunk)->bEmpty      = (mng_bool)(iCount == 0);

  MNG_COPY (((mng_pltep)pChunk)->aEntries, aPalette, sizeof (mng_palette8));

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PLTE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_idat (mng_handle hHandle,
                                        mng_uint32 iRawlen,
                                        mng_ptr    pRawdata)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_IDAT, mng_init_general, mng_free_idat, mng_read_idat, mng_write_idat, mng_assign_idat, 0, 0, sizeof(mng_idat)};
#else
          {MNG_UINT_IDAT, mng_init_idat, mng_free_idat, mng_read_idat, mng_write_idat, mng_assign_idat, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IDAT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_IDAT))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_idat (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_IDAT, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_idatp)pChunk)->bEmpty    = (mng_bool)(iRawlen == 0);
  ((mng_idatp)pChunk)->iDatasize = iRawlen;

  if (iRawlen)
  {
    MNG_ALLOC (pData, ((mng_idatp)pChunk)->pData, iRawlen);
    MNG_COPY (((mng_idatp)pChunk)->pData, pRawdata, iRawlen);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_iend (mng_handle hHandle)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_IEND, mng_init_general, mng_free_general, mng_read_iend, mng_write_iend, mng_assign_general, 0, 0, sizeof(mng_iend)};
#else
          {MNG_UINT_IEND, mng_init_iend, mng_free_iend, mng_read_iend, mng_write_iend, mng_assign_iend, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IEND, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_IEND))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_iend (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_IEND, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_INCLUDE_JNG
  if ((pData->iFirstchunkadded == MNG_UINT_IHDR) ||
      (pData->iFirstchunkadded == MNG_UINT_JHDR)    )
#else
  if (pData->iFirstchunkadded == MNG_UINT_IHDR)
#endif
    pData->bCreating = MNG_FALSE;      /* should be last chunk !!! */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_trns (mng_handle   hHandle,
                                        mng_bool     bEmpty,
                                        mng_bool     bGlobal,
                                        mng_uint8    iType,
                                        mng_uint32   iCount,
                                        mng_uint8arr aAlphas,
                                        mng_uint16   iGray,
                                        mng_uint16   iRed,
                                        mng_uint16   iGreen,
                                        mng_uint16   iBlue,
                                        mng_uint32   iRawlen,
                                        mng_uint8arr aRawdata)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_tRNS, mng_init_general, mng_free_general, mng_read_trns, mng_write_trns, mng_assign_general, 0, 0, sizeof(mng_trns)};
#else
          {MNG_UINT_tRNS, mng_init_trns, mng_free_trns, mng_read_trns, mng_write_trns, mng_assign_trns, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_TRNS, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_tRNS))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_trns (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_tRNS, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_trnsp)pChunk)->bEmpty   = bEmpty;
  ((mng_trnsp)pChunk)->bGlobal  = bGlobal;
  ((mng_trnsp)pChunk)->iType    = iType;
  ((mng_trnsp)pChunk)->iCount   = iCount;
  ((mng_trnsp)pChunk)->iGray    = iGray;
  ((mng_trnsp)pChunk)->iRed     = iRed;
  ((mng_trnsp)pChunk)->iGreen   = iGreen;
  ((mng_trnsp)pChunk)->iBlue    = iBlue;
  ((mng_trnsp)pChunk)->iRawlen  = iRawlen;

  MNG_COPY (((mng_trnsp)pChunk)->aEntries, aAlphas,  sizeof (mng_uint8arr));
  MNG_COPY (((mng_trnsp)pChunk)->aRawdata, aRawdata, sizeof (mng_uint8arr));

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_TRNS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_gAMA
mng_retcode MNG_DECL mng_putchunk_gama (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint32 iGamma)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_gAMA, mng_init_general, mng_free_general, mng_read_gama, mng_write_gama, mng_assign_general, 0, 0, sizeof(mng_gama)};
#else
          {MNG_UINT_gAMA, mng_init_gama, mng_free_gama, mng_read_gama, mng_write_gama, mng_assign_gama, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_GAMA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_gAMA))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_gama (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_gAMA, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_gamap)pChunk)->bEmpty = bEmpty;
  ((mng_gamap)pChunk)->iGamma = iGamma;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_GAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_cHRM
mng_retcode MNG_DECL mng_putchunk_chrm (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint32 iWhitepointx,
                                        mng_uint32 iWhitepointy,
                                        mng_uint32 iRedx,
                                        mng_uint32 iRedy,
                                        mng_uint32 iGreenx,
                                        mng_uint32 iGreeny,
                                        mng_uint32 iBluex,
                                        mng_uint32 iBluey)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_cHRM, mng_init_general, mng_free_general, mng_read_chrm, mng_write_chrm, mng_assign_general, 0, 0, sizeof(mng_chrm)};
#else
          {MNG_UINT_cHRM, mng_init_chrm, mng_free_chrm, mng_read_chrm, mng_write_chrm, mng_assign_chrm, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_CHRM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_cHRM))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_chrm (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_cHRM, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_chrmp)pChunk)->bEmpty       = bEmpty;
  ((mng_chrmp)pChunk)->iWhitepointx = iWhitepointx;
  ((mng_chrmp)pChunk)->iWhitepointy = iWhitepointy;
  ((mng_chrmp)pChunk)->iRedx        = iRedx;
  ((mng_chrmp)pChunk)->iRedy        = iRedy;
  ((mng_chrmp)pChunk)->iGreenx      = iGreenx;
  ((mng_chrmp)pChunk)->iGreeny      = iGreeny;
  ((mng_chrmp)pChunk)->iBluex       = iBluex;
  ((mng_chrmp)pChunk)->iBluey       = iBluey;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_CHRM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sRGB
mng_retcode MNG_DECL mng_putchunk_srgb (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint8  iRenderingintent)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_sRGB, mng_init_general, mng_free_general, mng_read_srgb, mng_write_srgb, mng_assign_general, 0, 0, sizeof(mng_srgb)};
#else
          {MNG_UINT_sRGB, mng_init_srgb, mng_free_srgb, mng_read_srgb, mng_write_srgb, mng_assign_srgb, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SRGB, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_sRGB))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_srgb (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_sRGB, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_srgbp)pChunk)->bEmpty           = bEmpty;
  ((mng_srgbp)pChunk)->iRenderingintent = iRenderingintent;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
mng_retcode MNG_DECL mng_putchunk_iccp (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint32 iNamesize,
                                        mng_pchar  zName,
                                        mng_uint8  iCompression,
                                        mng_uint32 iProfilesize,
                                        mng_ptr    pProfile)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_iCCP, mng_init_general, mng_free_iccp, mng_read_iccp, mng_write_iccp, mng_assign_iccp, 0, 0, sizeof(mng_iccp)};
#else
          {MNG_UINT_iCCP, mng_init_iccp, mng_free_iccp, mng_read_iccp, mng_write_iccp, mng_assign_iccp, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ICCP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_iCCP))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_iccp (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_iCCP, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_iccpp)pChunk)->bEmpty       = bEmpty;
  ((mng_iccpp)pChunk)->iNamesize    = iNamesize;
  ((mng_iccpp)pChunk)->iCompression = iCompression;
  ((mng_iccpp)pChunk)->iProfilesize = iProfilesize;

  if (iNamesize)
  {
    MNG_ALLOC (pData, ((mng_iccpp)pChunk)->zName, iNamesize + 1);
    MNG_COPY (((mng_iccpp)pChunk)->zName, zName, iNamesize);
  }

  if (iProfilesize)
  {
    MNG_ALLOC (pData, ((mng_iccpp)pChunk)->pProfile, iProfilesize);
    MNG_COPY (((mng_iccpp)pChunk)->pProfile, pProfile, iProfilesize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ICCP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tEXt
mng_retcode MNG_DECL mng_putchunk_text (mng_handle hHandle,
                                        mng_uint32 iKeywordsize,
                                        mng_pchar  zKeyword,
                                        mng_uint32 iTextsize,
                                        mng_pchar  zText)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_tEXt, mng_init_general, mng_free_text, mng_read_text, mng_write_text, mng_assign_text, 0, 0, sizeof(mng_text)};
#else
          {MNG_UINT_tEXt, mng_init_text, mng_free_text, mng_read_text, mng_write_text, mng_assign_text, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_TEXT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_tEXt))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_text (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_tEXt, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_textp)pChunk)->iKeywordsize = iKeywordsize;
  ((mng_textp)pChunk)->iTextsize    = iTextsize;

  if (iKeywordsize)
  {
    MNG_ALLOC (pData, ((mng_textp)pChunk)->zKeyword, iKeywordsize + 1);
    MNG_COPY (((mng_textp)pChunk)->zKeyword, zKeyword, iKeywordsize);
  }

  if (iTextsize)
  {
    MNG_ALLOC (pData, ((mng_textp)pChunk)->zText, iTextsize + 1);
    MNG_COPY (((mng_textp)pChunk)->zText, zText, iTextsize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_TEXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_zTXt
mng_retcode MNG_DECL mng_putchunk_ztxt (mng_handle hHandle,
                                        mng_uint32 iKeywordsize,
                                        mng_pchar  zKeyword,
                                        mng_uint8  iCompression,
                                        mng_uint32 iTextsize,
                                        mng_pchar  zText)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_zTXt, mng_init_general, mng_free_ztxt, mng_read_ztxt, mng_write_ztxt, mng_assign_ztxt, 0, 0, sizeof(mng_ztxt)};
#else
          {MNG_UINT_zTXt, mng_init_ztxt, mng_free_ztxt, mng_read_ztxt, mng_write_ztxt, mng_assign_ztxt, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ZTXT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_zTXt))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_ztxt (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_zTXt, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_ztxtp)pChunk)->iKeywordsize = iKeywordsize;
  ((mng_ztxtp)pChunk)->iCompression = iCompression;
  ((mng_ztxtp)pChunk)->iTextsize    = iTextsize;

  if (iKeywordsize)
  {
    MNG_ALLOC (pData, ((mng_ztxtp)pChunk)->zKeyword, iKeywordsize + 1);
    MNG_COPY (((mng_ztxtp)pChunk)->zKeyword, zKeyword, iKeywordsize);
  }

  if (iTextsize)
  {
    MNG_ALLOC (pData, ((mng_ztxtp)pChunk)->zText, iTextsize + 1);
    MNG_COPY  (((mng_ztxtp)pChunk)->zText, zText, iTextsize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ZTXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iTXt
mng_retcode MNG_DECL mng_putchunk_itxt (mng_handle hHandle,
                                        mng_uint32 iKeywordsize,
                                        mng_pchar  zKeyword,
                                        mng_uint8  iCompressionflag,
                                        mng_uint8  iCompressionmethod,
                                        mng_uint32 iLanguagesize,
                                        mng_pchar  zLanguage,
                                        mng_uint32 iTranslationsize,
                                        mng_pchar  zTranslation,
                                        mng_uint32 iTextsize,
                                        mng_pchar  zText)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_iTXt, mng_init_general, mng_free_itxt, mng_read_itxt, mng_write_itxt, mng_assign_itxt, 0, 0, sizeof(mng_itxt)};
#else
          {MNG_UINT_iTXt, mng_init_itxt, mng_free_itxt, mng_read_itxt, mng_write_itxt, mng_assign_itxt, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ITXT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_iTXt))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_itxt (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_iTXt, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_itxtp)pChunk)->iKeywordsize       = iKeywordsize;
  ((mng_itxtp)pChunk)->iCompressionflag   = iCompressionflag;
  ((mng_itxtp)pChunk)->iCompressionmethod = iCompressionmethod;
  ((mng_itxtp)pChunk)->iLanguagesize      = iLanguagesize;
  ((mng_itxtp)pChunk)->iTranslationsize   = iTranslationsize;
  ((mng_itxtp)pChunk)->iTextsize          = iTextsize;

  if (iKeywordsize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunk)->zKeyword, iKeywordsize + 1);
    MNG_COPY (((mng_itxtp)pChunk)->zKeyword, zKeyword, iKeywordsize);
  }

  if (iLanguagesize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunk)->zLanguage, iLanguagesize + 1);
    MNG_COPY (((mng_itxtp)pChunk)->zLanguage, zLanguage, iLanguagesize);
  }

  if (iTranslationsize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunk)->zTranslation, iTranslationsize + 1);
    MNG_COPY (((mng_itxtp)pChunk)->zTranslation, zTranslation, iTranslationsize);
  }

  if (iTextsize)
  {
    MNG_ALLOC (pData, ((mng_itxtp)pChunk)->zText, iTextsize + 1);
    MNG_COPY (((mng_itxtp)pChunk)->zText, zText, iTextsize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ITXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_bKGD
mng_retcode MNG_DECL mng_putchunk_bkgd (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint8  iType,
                                        mng_uint8  iIndex,
                                        mng_uint16 iGray,
                                        mng_uint16 iRed,
                                        mng_uint16 iGreen,
                                        mng_uint16 iBlue)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_bKGD, mng_init_general, mng_free_general, mng_read_bkgd, mng_write_bkgd, mng_assign_general, 0, 0, sizeof(mng_bkgd)};
#else
          {MNG_UINT_bKGD, mng_init_bkgd, mng_free_bkgd, mng_read_bkgd, mng_write_bkgd, mng_assign_bkgd, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_BKGD, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_bKGD))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_bkgd (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_bKGD, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_bkgdp)pChunk)->bEmpty = bEmpty;
  ((mng_bkgdp)pChunk)->iType  = iType;
  ((mng_bkgdp)pChunk)->iIndex = iIndex;
  ((mng_bkgdp)pChunk)->iGray  = iGray;
  ((mng_bkgdp)pChunk)->iRed   = iRed;
  ((mng_bkgdp)pChunk)->iGreen = iGreen;
  ((mng_bkgdp)pChunk)->iBlue  = iBlue;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_BKGD, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYs
mng_retcode MNG_DECL mng_putchunk_phys (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint32 iSizex,
                                        mng_uint32 iSizey,
                                        mng_uint8  iUnit)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_pHYs, mng_init_general, mng_free_general, mng_read_phys, mng_write_phys, mng_assign_general, 0, 0, sizeof(mng_phys)};
#else
          {MNG_UINT_pHYs, mng_init_phys, mng_free_phys, mng_read_phys, mng_write_phys, mng_assign_phys, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PHYS, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_pHYs))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_phys (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_pHYs, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_physp)pChunk)->bEmpty = bEmpty;
  ((mng_physp)pChunk)->iSizex = iSizex;
  ((mng_physp)pChunk)->iSizey = iSizey;
  ((mng_physp)pChunk)->iUnit  = iUnit;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PHYS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sBIT
mng_retcode MNG_DECL mng_putchunk_sbit (mng_handle    hHandle,
                                        mng_bool      bEmpty,
                                        mng_uint8     iType,
                                        mng_uint8arr4 aBits)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_sBIT, mng_init_general, mng_free_general, mng_read_sbit, mng_write_sbit, mng_assign_general, 0, 0, sizeof(mng_sbit)};
#else
          {MNG_UINT_sBIT, mng_init_sbit, mng_free_sbit, mng_read_sbit, mng_write_sbit, mng_assign_sbit, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SBIT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_sBIT))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_sbit (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_sBIT, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_sbitp)pChunk)->bEmpty   = bEmpty;
  ((mng_sbitp)pChunk)->iType    = iType;
  ((mng_sbitp)pChunk)->aBits[0] = aBits[0];
  ((mng_sbitp)pChunk)->aBits[1] = aBits[1];
  ((mng_sbitp)pChunk)->aBits[2] = aBits[2];
  ((mng_sbitp)pChunk)->aBits[3] = aBits[3];

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SBIT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
mng_retcode MNG_DECL mng_putchunk_splt (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint32 iNamesize,
                                        mng_pchar  zName,
                                        mng_uint8  iSampledepth,
                                        mng_uint32 iEntrycount,
                                        mng_ptr    pEntries)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_sPLT, mng_init_general, mng_free_splt, mng_read_splt, mng_write_splt, mng_assign_splt, 0, 0, sizeof(mng_splt)};
#else
          {MNG_UINT_sPLT, mng_init_splt, mng_free_splt, mng_read_splt, mng_write_splt, mng_assign_splt, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SPLT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_sPLT))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_splt (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_sPLT, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_spltp)pChunk)->bEmpty       = bEmpty;
  ((mng_spltp)pChunk)->iNamesize    = iNamesize;
  ((mng_spltp)pChunk)->iSampledepth = iSampledepth;
  ((mng_spltp)pChunk)->iEntrycount  = iEntrycount;

  if (iNamesize)
  {
    MNG_ALLOC (pData, ((mng_spltp)pChunk)->zName, iNamesize + 1);
    MNG_COPY (((mng_spltp)pChunk)->zName, zName, iNamesize);
  }

  if (iEntrycount)
  {
    mng_uint32 iSize = iEntrycount * ((iSampledepth >> 1) + 2);

    MNG_ALLOC (pData, ((mng_spltp)pChunk)->pEntries, iSize);
    MNG_COPY  (((mng_spltp)pChunk)->pEntries, pEntries, iSize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_hIST
mng_retcode MNG_DECL mng_putchunk_hist (mng_handle    hHandle,
                                        mng_uint32    iEntrycount,
                                        mng_uint16arr aEntries)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_hIST, mng_init_general, mng_free_general, mng_read_hist, mng_write_hist, mng_assign_general, 0, 0, sizeof(mng_hist)};
#else
          {MNG_UINT_hIST, mng_init_hist, mng_free_hist, mng_read_hist, mng_write_hist, mng_assign_hist, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_HIST, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_hIST))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_hist (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_hIST, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_histp)pChunk)->iEntrycount = iEntrycount;

  MNG_COPY (((mng_histp)pChunk)->aEntries, aEntries, sizeof (mng_uint16arr));

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_HIST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tIME
mng_retcode MNG_DECL mng_putchunk_time (mng_handle hHandle,
                                        mng_uint16 iYear,
                                        mng_uint8  iMonth,
                                        mng_uint8  iDay,
                                        mng_uint8  iHour,
                                        mng_uint8  iMinute,
                                        mng_uint8  iSecond)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_tIME, mng_init_general, mng_free_general, mng_read_time, mng_write_time, mng_assign_general, 0, 0, sizeof(mng_time)};
#else
          {MNG_UINT_tIME, mng_init_time, mng_free_time, mng_read_time, mng_write_time, mng_assign_time, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_TIME, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_tIME))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_time (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_tIME, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_timep)pChunk)->iYear   = iYear;
  ((mng_timep)pChunk)->iMonth  = iMonth;
  ((mng_timep)pChunk)->iDay    = iDay;
  ((mng_timep)pChunk)->iHour   = iHour;
  ((mng_timep)pChunk)->iMinute = iMinute;
  ((mng_timep)pChunk)->iSecond = iSecond;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_TIME, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_mhdr (mng_handle hHandle,
                                        mng_uint32 iWidth,
                                        mng_uint32 iHeight,
                                        mng_uint32 iTicks,
                                        mng_uint32 iLayercount,
                                        mng_uint32 iFramecount,
                                        mng_uint32 iPlaytime,
                                        mng_uint32 iSimplicity)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_MHDR, mng_init_general, mng_free_general, mng_read_mhdr, mng_write_mhdr, mng_assign_general, 0, 0, sizeof(mng_mhdr)};
#else
          {MNG_UINT_MHDR, mng_init_mhdr, mng_free_mhdr, mng_read_mhdr, mng_write_mhdr, mng_assign_mhdr, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MHDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must be very first! */
  if (pData->iFirstchunkadded != 0)
    MNG_ERROR (pData, MNG_SEQUENCEERROR)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_MHDR))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_mhdr (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_MHDR, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_mhdrp)pChunk)->iWidth      = iWidth;
  ((mng_mhdrp)pChunk)->iHeight     = iHeight;
  ((mng_mhdrp)pChunk)->iTicks      = iTicks;
  ((mng_mhdrp)pChunk)->iLayercount = iLayercount;
  ((mng_mhdrp)pChunk)->iFramecount = iFramecount;
  ((mng_mhdrp)pChunk)->iPlaytime   = iPlaytime;
  ((mng_mhdrp)pChunk)->iSimplicity = iSimplicity;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_mend (mng_handle hHandle)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_MEND, mng_init_general, mng_free_general, mng_read_mend, mng_write_mend, mng_assign_general, 0, 0, sizeof(mng_mend)};
#else
          {MNG_UINT_MEND, mng_init_mend, mng_free_mend, mng_read_mend, mng_write_mend, mng_assign_mend, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MEND, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_MEND))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_mend (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_MEND, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

  pData->bCreating = MNG_FALSE;        /* should be last chunk !!! */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
mng_retcode MNG_DECL mng_putchunk_loop (mng_handle  hHandle,
                                        mng_uint8   iLevel,
                                        mng_uint32  iRepeat,
                                        mng_uint8   iTermination,
                                        mng_uint32  iItermin,
                                        mng_uint32  iItermax,
                                        mng_uint32  iCount,
                                        mng_uint32p pSignals)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_LOOP, mng_init_general, mng_free_loop, mng_read_loop, mng_write_loop, mng_assign_loop, 0, 0, sizeof(mng_loop)};
#else
          {MNG_UINT_LOOP, mng_init_loop, mng_free_loop, mng_read_loop, mng_write_loop, mng_assign_loop, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_LOOP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_LOOP))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_loop (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_LOOP, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_loopp)pChunk)->iLevel       = iLevel;
  ((mng_loopp)pChunk)->iRepeat      = iRepeat;
  ((mng_loopp)pChunk)->iTermination = iTermination;
  ((mng_loopp)pChunk)->iItermin     = iItermin;
  ((mng_loopp)pChunk)->iItermax     = iItermax;
  ((mng_loopp)pChunk)->iCount       = iCount;
  ((mng_loopp)pChunk)->pSignals     = pSignals;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_LOOP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_endl (mng_handle hHandle,
                                        mng_uint8  iLevel)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_ENDL, mng_init_general, mng_free_general, mng_read_endl, mng_write_endl, mng_assign_general, 0, 0, sizeof(mng_endl)};
#else
          {MNG_UINT_ENDL, mng_init_endl, mng_free_endl, mng_read_endl, mng_write_endl, mng_assign_endl, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ENDL, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_ENDL))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_endl (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_ENDL, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_endlp)pChunk)->iLevel = iLevel;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ENDL, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DEFI
mng_retcode MNG_DECL mng_putchunk_defi (mng_handle hHandle,
                                        mng_uint16 iObjectid,
                                        mng_uint8  iDonotshow,
                                        mng_uint8  iConcrete,
                                        mng_bool   bHasloca,
                                        mng_int32  iXlocation,
                                        mng_int32  iYlocation,
                                        mng_bool   bHasclip,
                                        mng_int32  iLeftcb,
                                        mng_int32  iRightcb,
                                        mng_int32  iTopcb,
                                        mng_int32  iBottomcb)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_DEFI, mng_init_general, mng_free_general, mng_read_defi, mng_write_defi, mng_assign_general, 0, 0, sizeof(mng_defi)};
#else
          {MNG_UINT_DEFI, mng_init_defi, mng_free_defi, mng_read_defi, mng_write_defi, mng_assign_defi, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DEFI, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_DEFI))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_defi (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_DEFI, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_defip)pChunk)->iObjectid  = iObjectid;
  ((mng_defip)pChunk)->iDonotshow = iDonotshow;
  ((mng_defip)pChunk)->iConcrete  = iConcrete;
  ((mng_defip)pChunk)->bHasloca   = bHasloca;
  ((mng_defip)pChunk)->iXlocation = iXlocation;
  ((mng_defip)pChunk)->iYlocation = iYlocation;
  ((mng_defip)pChunk)->bHasclip   = bHasclip;
  ((mng_defip)pChunk)->iLeftcb    = iLeftcb;
  ((mng_defip)pChunk)->iRightcb   = iRightcb;
  ((mng_defip)pChunk)->iTopcb     = iTopcb;
  ((mng_defip)pChunk)->iBottomcb  = iBottomcb;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BASI
mng_retcode MNG_DECL mng_putchunk_basi (mng_handle hHandle,
                                        mng_uint32 iWidth,
                                        mng_uint32 iHeight,
                                        mng_uint8  iBitdepth,
                                        mng_uint8  iColortype,
                                        mng_uint8  iCompression,
                                        mng_uint8  iFilter,
                                        mng_uint8  iInterlace,
                                        mng_uint16 iRed,
                                        mng_uint16 iGreen,
                                        mng_uint16 iBlue,
                                        mng_uint16 iAlpha,
                                        mng_uint8  iViewable)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_BASI, mng_init_general, mng_free_general, mng_read_basi, mng_write_basi, mng_assign_general, 0, 0, sizeof(mng_basi)};
#else
          {MNG_UINT_BASI, mng_init_basi, mng_free_basi, mng_read_basi, mng_write_basi, mng_assign_basi, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_BASI, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_BASI))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_basi (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_BASI, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_basip)pChunk)->iWidth       = iWidth;
  ((mng_basip)pChunk)->iHeight      = iHeight;
  ((mng_basip)pChunk)->iBitdepth    = iBitdepth;
  ((mng_basip)pChunk)->iColortype   = iColortype;
  ((mng_basip)pChunk)->iCompression = iCompression;
  ((mng_basip)pChunk)->iFilter      = iFilter;
  ((mng_basip)pChunk)->iInterlace   = iInterlace;
  ((mng_basip)pChunk)->iRed         = iRed;
  ((mng_basip)pChunk)->iGreen       = iGreen;
  ((mng_basip)pChunk)->iBlue        = iBlue;
  ((mng_basip)pChunk)->iAlpha       = iAlpha;
  ((mng_basip)pChunk)->iViewable    = iViewable;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_BASI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLON
mng_retcode MNG_DECL mng_putchunk_clon (mng_handle hHandle,
                                        mng_uint16 iSourceid,
                                        mng_uint16 iCloneid,
                                        mng_uint8  iClonetype,
                                        mng_uint8  iDonotshow,
                                        mng_uint8  iConcrete,
                                        mng_bool   bHasloca,
                                        mng_uint8  iLocationtype,
                                        mng_int32  iLocationx,
                                        mng_int32  iLocationy)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_CLON, mng_init_general, mng_free_general, mng_read_clon, mng_write_clon, mng_assign_general, 0, 0, sizeof(mng_clon)};
#else
          {MNG_UINT_CLON, mng_init_clon, mng_free_clon, mng_read_clon, mng_write_clon, mng_assign_clon, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_CLON, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_CLON))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_clon (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_CLON, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_clonp)pChunk)->iSourceid     = iSourceid;
  ((mng_clonp)pChunk)->iCloneid      = iCloneid;
  ((mng_clonp)pChunk)->iClonetype    = iClonetype;
  ((mng_clonp)pChunk)->iDonotshow    = iDonotshow;
  ((mng_clonp)pChunk)->iConcrete     = iConcrete;
  ((mng_clonp)pChunk)->bHasloca      = bHasloca;
  ((mng_clonp)pChunk)->iLocationtype = iLocationtype;
  ((mng_clonp)pChunk)->iLocationx    = iLocationx;
  ((mng_clonp)pChunk)->iLocationy    = iLocationy;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_CLON, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode MNG_DECL mng_putchunk_past (mng_handle hHandle,
                                        mng_uint16 iDestid,
                                        mng_uint8  iTargettype,
                                        mng_int32  iTargetx,
                                        mng_int32  iTargety,
                                        mng_uint32 iCount)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_PAST, mng_init_general, mng_free_past, mng_read_past, mng_write_past, mng_assign_past, 0, 0, sizeof(mng_past)};
#else
          {MNG_UINT_PAST, mng_init_past, mng_free_past, mng_read_past, mng_write_past, mng_assign_past, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PAST, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_PAST))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_past (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_PAST, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_pastp)pChunk)->iDestid     = iDestid;
  ((mng_pastp)pChunk)->iTargettype = iTargettype;
  ((mng_pastp)pChunk)->iTargetx    = iTargetx;
  ((mng_pastp)pChunk)->iTargety    = iTargety;
  ((mng_pastp)pChunk)->iCount      = iCount;

  if (iCount)
    MNG_ALLOC (pData, ((mng_pastp)pChunk)->pSources, iCount * sizeof (mng_past_source));

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PAST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode MNG_DECL mng_putchunk_past_src (mng_handle hHandle,
                                            mng_uint32 iEntry,
                                            mng_uint16 iSourceid,
                                            mng_uint8  iComposition,
                                            mng_uint8  iOrientation,
                                            mng_uint8  iOffsettype,
                                            mng_int32  iOffsetx,
                                            mng_int32  iOffsety,
                                            mng_uint8  iBoundarytype,
                                            mng_int32  iBoundaryl,
                                            mng_int32  iBoundaryr,
                                            mng_int32  iBoundaryt,
                                            mng_int32  iBoundaryb)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_past_sourcep pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PAST_SRC, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)

  pChunk = pData->pLastchunk;          /* last one must have been PAST ! */

  if (((mng_chunk_headerp)pChunk)->iChunkname != MNG_UINT_PAST)
    MNG_ERROR (pData, MNG_NOCORRCHUNK)
                                       /* index out of bounds ? */
  if (iEntry >= ((mng_pastp)pChunk)->iCount)
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)
                                       /* address proper entry */
  pEntry = ((mng_pastp)pChunk)->pSources + iEntry;

  pEntry->iSourceid     = iSourceid;   /* fill entry */
  pEntry->iComposition  = iComposition;
  pEntry->iOrientation  = iOrientation;
  pEntry->iOffsettype   = iOffsettype;
  pEntry->iOffsetx      = iOffsetx;
  pEntry->iOffsety      = iOffsety;
  pEntry->iBoundarytype = iBoundarytype;
  pEntry->iBoundaryl    = iBoundaryl;
  pEntry->iBoundaryr    = iBoundaryr;
  pEntry->iBoundaryt    = iBoundaryt;
  pEntry->iBoundaryb    = iBoundaryb;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PAST_SRC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
mng_retcode MNG_DECL mng_putchunk_disc (mng_handle  hHandle,
                                        mng_uint32  iCount,
                                        mng_uint16p pObjectids)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_DISC, mng_init_general, mng_free_disc, mng_read_disc, mng_write_disc, mng_assign_disc, 0, 0, sizeof(mng_disc)};
#else
          {MNG_UINT_DISC, mng_init_disc, mng_free_disc, mng_read_disc, mng_write_disc, mng_assign_disc, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DISC, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_DISC))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_disc (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_DISC, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_discp)pChunk)->iCount = iCount;

  if (iCount)
  {
    mng_uint32 iSize = iCount * sizeof (mng_uint32);

    MNG_ALLOC (pData, ((mng_discp)pChunk)->pObjectids, iSize);
    MNG_COPY (((mng_discp)pChunk)->pObjectids, pObjectids, iSize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DISC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BACK
mng_retcode MNG_DECL mng_putchunk_back (mng_handle hHandle,
                                        mng_uint16 iRed,
                                        mng_uint16 iGreen,
                                        mng_uint16 iBlue,
                                        mng_uint8  iMandatory,
                                        mng_uint16 iImageid,
                                        mng_uint8  iTile)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_BACK, mng_init_general, mng_free_general, mng_read_back, mng_write_back, mng_assign_general, 0, 0, sizeof(mng_back)};
#else
          {MNG_UINT_BACK, mng_init_back, mng_free_back, mng_read_back, mng_write_back, mng_assign_back, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_BACK, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_BACK))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_back (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_BACK, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_backp)pChunk)->iRed       = iRed;
  ((mng_backp)pChunk)->iGreen     = iGreen;
  ((mng_backp)pChunk)->iBlue      = iBlue;
  ((mng_backp)pChunk)->iMandatory = iMandatory;
  ((mng_backp)pChunk)->iImageid   = iImageid;
  ((mng_backp)pChunk)->iTile      = iTile;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_BACK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
mng_retcode MNG_DECL mng_putchunk_fram (mng_handle  hHandle,
                                        mng_bool    bEmpty,
                                        mng_uint8   iMode,
                                        mng_uint32  iNamesize,
                                        mng_pchar   zName,
                                        mng_uint8   iChangedelay,
                                        mng_uint8   iChangetimeout,
                                        mng_uint8   iChangeclipping,
                                        mng_uint8   iChangesyncid,
                                        mng_uint32  iDelay,
                                        mng_uint32  iTimeout,
                                        mng_uint8   iBoundarytype,
                                        mng_int32   iBoundaryl,
                                        mng_int32   iBoundaryr,
                                        mng_int32   iBoundaryt,
                                        mng_int32   iBoundaryb,
                                        mng_uint32  iCount,
                                        mng_uint32p pSyncids)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_FRAM, mng_init_general, mng_free_fram, mng_read_fram, mng_write_fram, mng_assign_fram, 0, 0, sizeof(mng_fram)};
#else
          {MNG_UINT_FRAM, mng_init_fram, mng_free_fram, mng_read_fram, mng_write_fram, mng_assign_fram, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_FRAM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_FRAM))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_fram (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_FRAM, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_framp)pChunk)->bEmpty          = bEmpty;
  ((mng_framp)pChunk)->iMode           = iMode;
  ((mng_framp)pChunk)->iNamesize       = iNamesize;
  ((mng_framp)pChunk)->iChangedelay    = iChangedelay;
  ((mng_framp)pChunk)->iChangetimeout  = iChangetimeout;
  ((mng_framp)pChunk)->iChangeclipping = iChangeclipping;
  ((mng_framp)pChunk)->iChangesyncid   = iChangesyncid;
  ((mng_framp)pChunk)->iDelay          = iDelay;
  ((mng_framp)pChunk)->iTimeout        = iTimeout;
  ((mng_framp)pChunk)->iBoundarytype   = iBoundarytype;
  ((mng_framp)pChunk)->iBoundaryl      = iBoundaryl;
  ((mng_framp)pChunk)->iBoundaryr      = iBoundaryr;
  ((mng_framp)pChunk)->iBoundaryt      = iBoundaryt;
  ((mng_framp)pChunk)->iBoundaryb      = iBoundaryb;
  ((mng_framp)pChunk)->iCount          = iCount;

  if (iNamesize)
  {
    MNG_ALLOC (pData, ((mng_framp)pChunk)->zName, iNamesize + 1);
    MNG_COPY (((mng_framp)pChunk)->zName, zName, iNamesize);
  }

  if (iCount)
  {
    mng_uint32 iSize = iCount * sizeof (mng_uint32);

    MNG_ALLOC (pData, ((mng_framp)pChunk)->pSyncids, iSize);
    MNG_COPY (((mng_framp)pChunk)->pSyncids, pSyncids, iSize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_FRAM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MOVE
mng_retcode MNG_DECL mng_putchunk_move (mng_handle hHandle,
                                        mng_uint16 iFirstid,
                                        mng_uint16 iLastid,
                                        mng_uint8  iMovetype,
                                        mng_int32  iMovex,
                                        mng_int32  iMovey)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_MOVE, mng_init_general, mng_free_general, mng_read_move, mng_write_move, mng_assign_general, 0, 0, sizeof(mng_move)};
#else
          {MNG_UINT_MOVE, mng_init_move, mng_free_move, mng_read_move, mng_write_move, mng_assign_move, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MOVE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_MOVE))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_move (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_MOVE, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_movep)pChunk)->iFirstid  = iFirstid;
  ((mng_movep)pChunk)->iLastid   = iLastid;
  ((mng_movep)pChunk)->iMovetype = iMovetype;
  ((mng_movep)pChunk)->iMovex    = iMovex;
  ((mng_movep)pChunk)->iMovey    = iMovey;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MOVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLIP
mng_retcode MNG_DECL mng_putchunk_clip (mng_handle hHandle,
                                        mng_uint16 iFirstid,
                                        mng_uint16 iLastid,
                                        mng_uint8  iCliptype,
                                        mng_int32  iClipl,
                                        mng_int32  iClipr,
                                        mng_int32  iClipt,
                                        mng_int32  iClipb)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_CLIP, mng_init_general, mng_free_general, mng_read_clip, mng_write_clip, mng_assign_general, 0, 0, sizeof(mng_clip)};
#else
          {MNG_UINT_CLIP, mng_init_clip, mng_free_clip, mng_read_clip, mng_write_clip, mng_assign_clip, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_CLIP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_CLIP))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_clip (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_CLIP, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_clipp)pChunk)->iFirstid  = iFirstid;
  ((mng_clipp)pChunk)->iLastid   = iLastid;
  ((mng_clipp)pChunk)->iCliptype = iCliptype;
  ((mng_clipp)pChunk)->iClipl    = iClipl;
  ((mng_clipp)pChunk)->iClipr    = iClipr;
  ((mng_clipp)pChunk)->iClipt    = iClipt;
  ((mng_clipp)pChunk)->iClipb    = iClipb;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_CLIP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif


/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SHOW
mng_retcode MNG_DECL mng_putchunk_show (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint16 iFirstid,
                                        mng_uint16 iLastid,
                                        mng_uint8  iMode)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_SHOW, mng_init_general, mng_free_general, mng_read_show, mng_write_show, mng_assign_general, 0, 0, sizeof(mng_show)};
#else
          {MNG_UINT_SHOW, mng_init_show, mng_free_show, mng_read_show, mng_write_show, mng_assign_show, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SHOW, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_SHOW))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_show (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_SHOW, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_showp)pChunk)->bEmpty   = bEmpty;
  ((mng_showp)pChunk)->iFirstid = iFirstid;
  ((mng_showp)pChunk)->iLastid  = iLastid;
  ((mng_showp)pChunk)->iMode    = iMode;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SHOW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_TERM
mng_retcode MNG_DECL mng_putchunk_term (mng_handle hHandle,
                                        mng_uint8  iTermaction,
                                        mng_uint8  iIteraction,
                                        mng_uint32 iDelay,
                                        mng_uint32 iItermax)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_TERM, mng_init_general, mng_free_general, mng_read_term, mng_write_term, mng_assign_general, 0, 0, sizeof(mng_term)};
#else
          {MNG_UINT_TERM, mng_init_term, mng_free_term, mng_read_term, mng_write_term, mng_assign_term, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_TERM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_term (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_TERM, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_termp)pChunk)->iTermaction = iTermaction;
  ((mng_termp)pChunk)->iIteraction = iIteraction;
  ((mng_termp)pChunk)->iDelay      = iDelay;
  ((mng_termp)pChunk)->iItermax    = iItermax;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_TERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode MNG_DECL mng_putchunk_save (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint8  iOffsettype,
                                        mng_uint32 iCount)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_SAVE, mng_init_general, mng_free_save, mng_read_save, mng_write_save, mng_assign_save, 0, 0, sizeof(mng_save)};
#else
          {MNG_UINT_SAVE, mng_init_save, mng_free_save, mng_read_save, mng_write_save, mng_assign_save, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SAVE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_SAVE))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_save (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_SAVE, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_savep)pChunk)->bEmpty      = bEmpty;
  ((mng_savep)pChunk)->iOffsettype = iOffsettype;
  ((mng_savep)pChunk)->iCount      = iCount;

  if (iCount)
    MNG_ALLOC (pData, ((mng_savep)pChunk)->pEntries, iCount * sizeof (mng_save_entry));

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_save_entry (mng_handle     hHandle,
                                              mng_uint32     iEntry,
                                              mng_uint8      iEntrytype,
                                              mng_uint32arr2 iOffset,
                                              mng_uint32arr2 iStarttime,
                                              mng_uint32     iLayernr,
                                              mng_uint32     iFramenr,
                                              mng_uint32     iNamesize,
                                              mng_pchar      zName)
{
  mng_datap       pData;
  mng_chunkp      pChunk;
  mng_save_entryp pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SAVE_ENTRY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)

  pChunk = pData->pLastchunk;          /* last one must have been SAVE ! */

  if (((mng_chunk_headerp)pChunk)->iChunkname != MNG_UINT_SAVE)
    MNG_ERROR (pData, MNG_NOCORRCHUNK)
                                       /* index out of bounds ? */
  if (iEntry >= ((mng_savep)pChunk)->iCount)
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)
                                       /* address proper entry */
  pEntry = ((mng_savep)pChunk)->pEntries + iEntry;

  pEntry->iEntrytype    = iEntrytype;  /* fill entry */
  pEntry->iOffset[0]    = iOffset[0];
  pEntry->iOffset[1]    = iOffset[1];
  pEntry->iStarttime[0] = iStarttime[0];
  pEntry->iStarttime[1] = iStarttime[1];
  pEntry->iLayernr      = iLayernr;
  pEntry->iFramenr      = iFramenr;
  pEntry->iNamesize     = iNamesize;

  if (iNamesize)
  {
    MNG_ALLOC (pData, pEntry->zName, iNamesize + 1);
    MNG_COPY (pEntry->zName, zName, iNamesize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SAVE_ENTRY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode MNG_DECL mng_putchunk_seek (mng_handle hHandle,
                                        mng_uint32 iNamesize,
                                        mng_pchar  zName)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_SEEK, mng_init_general, mng_free_seek, mng_read_seek, mng_write_seek, mng_assign_seek, 0, 0, sizeof(mng_seek)};
#else
          {MNG_UINT_SEEK, mng_init_seek, mng_free_seek, mng_read_seek, mng_write_seek, mng_assign_seek, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SEEK, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_SEEK))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_seek (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_SEEK, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_seekp)pChunk)->iNamesize = iNamesize;

  if (iNamesize)
  {
    MNG_ALLOC (pData, ((mng_seekp)pChunk)->zName, iNamesize + 1);
    MNG_COPY (((mng_seekp)pChunk)->zName, zName, iNamesize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_SEEK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_eXPI
mng_retcode MNG_DECL mng_putchunk_expi (mng_handle hHandle,
                                        mng_uint16 iSnapshotid,
                                        mng_uint32 iNamesize,
                                        mng_pchar  zName)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_eXPI, mng_init_general, mng_free_expi, mng_read_expi, mng_write_expi, mng_assign_general, 0, 0, sizeof(mng_expi)};
#else
          {MNG_UINT_eXPI, mng_init_expi, mng_free_expi, mng_read_expi, mng_write_expi, mng_assign_expi, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_EXPI, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_eXPI))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_expi (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_eXPI, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_expip)pChunk)->iSnapshotid = iSnapshotid;
  ((mng_expip)pChunk)->iNamesize   = iNamesize;

  if (iNamesize)
  {
    MNG_ALLOC (pData, ((mng_expip)pChunk)->zName, iNamesize + 1);
    MNG_COPY (((mng_expip)pChunk)->zName, zName, iNamesize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_EXPI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_fPRI
mng_retcode MNG_DECL mng_putchunk_fpri (mng_handle hHandle,
                                        mng_uint8  iDeltatype,
                                        mng_uint8  iPriority)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_fPRI, mng_init_general, mng_free_general, mng_read_fpri, mng_write_fpri, mng_assign_general, 0, 0, sizeof(mng_fpri)};
#else
          {MNG_UINT_fPRI, mng_init_fpri, mng_free_fpri, mng_read_fpri, mng_write_fpri, mng_assign_fpri, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_FPRI, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_fPRI))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_fpri (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_fPRI, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_fprip)pChunk)->iDeltatype = iDeltatype;
  ((mng_fprip)pChunk)->iPriority  = iPriority;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_FPRI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
mng_retcode MNG_DECL mng_putchunk_need (mng_handle hHandle,
                                        mng_uint32 iKeywordssize,
                                        mng_pchar  zKeywords)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_nEED, mng_init_general, mng_free_need, mng_read_need, mng_write_need, mng_assign_need, 0, 0, sizeof(mng_need)};
#else
          {MNG_UINT_nEED, mng_init_need, mng_free_need, mng_read_need, mng_write_need, mng_assign_need, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_NEED, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_nEED))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_need (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_nEED, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_needp)pChunk)->iKeywordssize = iKeywordssize;

  if (iKeywordssize)
  {
    MNG_ALLOC (pData, ((mng_needp)pChunk)->zKeywords, iKeywordssize + 1);
    MNG_COPY (((mng_needp)pChunk)->zKeywords, zKeywords, iKeywordssize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_NEED, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYg
mng_retcode MNG_DECL mng_putchunk_phyg (mng_handle hHandle,
                                        mng_bool   bEmpty,
                                        mng_uint32 iSizex,
                                        mng_uint32 iSizey,
                                        mng_uint8  iUnit)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_pHYg, mng_init_general, mng_free_general, mng_read_phyg, mng_write_phyg, mng_assign_general, 0, 0, sizeof(mng_phyg)};
#else
          {MNG_UINT_pHYg, mng_init_phyg, mng_free_phyg, mng_read_phyg, mng_write_phyg, mng_assign_phyg, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PHYG, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_pHYg))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_phyg (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_pHYg, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_phygp)pChunk)->bEmpty = bEmpty;
  ((mng_phygp)pChunk)->iSizex = iSizex;
  ((mng_phygp)pChunk)->iSizey = iSizey;
  ((mng_phygp)pChunk)->iUnit  = iUnit;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PHYG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

mng_retcode MNG_DECL mng_putchunk_jhdr (mng_handle hHandle,
                                        mng_uint32 iWidth,
                                        mng_uint32 iHeight,
                                        mng_uint8  iColortype,
                                        mng_uint8  iImagesampledepth,
                                        mng_uint8  iImagecompression,
                                        mng_uint8  iImageinterlace,
                                        mng_uint8  iAlphasampledepth,
                                        mng_uint8  iAlphacompression,
                                        mng_uint8  iAlphafilter,
                                        mng_uint8  iAlphainterlace)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_JHDR, mng_init_general, mng_free_general, mng_read_jhdr, mng_write_jhdr, mng_assign_general, 0, 0, sizeof(mng_jhdr)};
#else
          {MNG_UINT_JHDR, mng_init_jhdr, mng_free_jhdr, mng_read_jhdr, mng_write_jhdr, mng_assign_jhdr, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_JHDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_JHDR))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_jhdr (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_JHDR, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_jhdrp)pChunk)->iWidth            = iWidth;
  ((mng_jhdrp)pChunk)->iHeight           = iHeight;
  ((mng_jhdrp)pChunk)->iColortype        = iColortype;
  ((mng_jhdrp)pChunk)->iImagesampledepth = iImagesampledepth;
  ((mng_jhdrp)pChunk)->iImagecompression = iImagecompression;
  ((mng_jhdrp)pChunk)->iImageinterlace   = iImageinterlace;
  ((mng_jhdrp)pChunk)->iAlphasampledepth = iAlphasampledepth;
  ((mng_jhdrp)pChunk)->iAlphacompression = iAlphacompression;
  ((mng_jhdrp)pChunk)->iAlphafilter      = iAlphafilter;
  ((mng_jhdrp)pChunk)->iAlphainterlace   = iAlphainterlace;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_JHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

mng_retcode MNG_DECL mng_putchunk_jdat (mng_handle hHandle,
                                        mng_uint32 iRawlen,
                                        mng_ptr    pRawdata)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_JDAT, mng_init_general, mng_free_jdat, mng_read_jdat, mng_write_jdat, mng_assign_jdat, 0, 0, sizeof(mng_jdat)};
#else
          {MNG_UINT_JDAT, mng_init_jdat, mng_free_jdat, mng_read_jdat, mng_write_jdat, mng_assign_jdat, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_JDAT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR or JHDR first! */
  if ((pData->iFirstchunkadded != MNG_UINT_MHDR) &&
      (pData->iFirstchunkadded != MNG_UINT_JHDR)    )
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_JDAT))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_jdat (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_JDAT, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_jdatp)pChunk)->iDatasize = iRawlen;

  if (iRawlen)
  {
    MNG_ALLOC (pData, ((mng_jdatp)pChunk)->pData, iRawlen);
    MNG_COPY (((mng_jdatp)pChunk)->pData, pRawdata, iRawlen);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_JDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

#endif /*  MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

mng_retcode MNG_DECL mng_putchunk_jdaa (mng_handle hHandle,
                                        mng_uint32 iRawlen,
                                        mng_ptr    pRawdata)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_JDAA, mng_init_general, mng_free_jdaa, mng_read_jdaa, mng_write_jdaa, mng_assign_jdaa, 0, 0, sizeof(mng_jdaa)};
#else
          {MNG_UINT_JDAA, mng_init_jdaa, mng_free_jdaa, mng_read_jdaa, mng_write_jdaa, mng_assign_jdaa, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_JDAA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR or JHDR first! */
  if ((pData->iFirstchunkadded != MNG_UINT_MHDR) &&
      (pData->iFirstchunkadded != MNG_UINT_JHDR)    )
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_JDAA))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_jdaa (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_JDAA, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_jdaap)pChunk)->iDatasize = iRawlen;

  if (iRawlen)
  {
    MNG_ALLOC (pData, ((mng_jdaap)pChunk)->pData, iRawlen);
    MNG_COPY (((mng_jdaap)pChunk)->pData, pRawdata, iRawlen);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_JDAA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

#endif /*  MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

mng_retcode MNG_DECL mng_putchunk_jsep (mng_handle hHandle)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_JSEP, mng_init_general, mng_free_general, mng_read_jsep, mng_write_jsep, mng_assign_general, 0, 0, sizeof(mng_jsep)};
#else
          {MNG_UINT_JSEP, mng_init_jsep, mng_free_jsep, mng_read_jsep, mng_write_jsep, mng_assign_jsep, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_JSEP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR or JHDR first! */
  if ((pData->iFirstchunkadded != MNG_UINT_MHDR) &&
      (pData->iFirstchunkadded != MNG_UINT_JHDR)    )
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_JSEP))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_jsep (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_JSEP, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_JSEP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_putchunk_dhdr (mng_handle hHandle,
                                        mng_uint16 iObjectid,
                                        mng_uint8  iImagetype,
                                        mng_uint8  iDeltatype,
                                        mng_uint32 iBlockwidth,
                                        mng_uint32 iBlockheight,
                                        mng_uint32 iBlockx,
                                        mng_uint32 iBlocky)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_DHDR, mng_init_general, mng_free_general, mng_read_dhdr, mng_write_dhdr, mng_assign_general, 0, 0, sizeof(mng_dhdr)};
#else
          {MNG_UINT_DHDR, mng_init_dhdr, mng_free_dhdr, mng_read_dhdr, mng_write_dhdr, mng_assign_dhdr, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DHDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_DHDR))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_dhdr (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_DHDR, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_dhdrp)pChunk)->iObjectid    = iObjectid;
  ((mng_dhdrp)pChunk)->iImagetype   = iImagetype;
  ((mng_dhdrp)pChunk)->iDeltatype   = iDeltatype;
  ((mng_dhdrp)pChunk)->iBlockwidth  = iBlockwidth;
  ((mng_dhdrp)pChunk)->iBlockheight = iBlockheight;
  ((mng_dhdrp)pChunk)->iBlockx      = iBlockx;
  ((mng_dhdrp)pChunk)->iBlocky      = iBlocky;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_putchunk_prom (mng_handle hHandle,
                                        mng_uint8  iColortype,
                                        mng_uint8  iSampledepth,
                                        mng_uint8  iFilltype)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_PROM, mng_init_general, mng_free_general, mng_read_prom, mng_write_prom, mng_assign_general, 0, 0, sizeof(mng_prom)};
#else
          {MNG_UINT_PROM, mng_init_prom, mng_free_prom, mng_read_prom, mng_write_prom, mng_assign_prom, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PROM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_PROM))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_prom (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_PROM, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_promp)pChunk)->iColortype   = iColortype;
  ((mng_promp)pChunk)->iSampledepth = iSampledepth;
  ((mng_promp)pChunk)->iFilltype    = iFilltype;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PROM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_putchunk_ipng (mng_handle hHandle)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_IPNG, mng_init_general, mng_free_general, mng_read_ipng, mng_write_ipng, mng_assign_general, 0, 0, sizeof(mng_ipng)};
#else
          {MNG_UINT_IPNG, mng_init_ipng, mng_free_ipng, mng_read_ipng, mng_write_ipng, mng_assign_ipng, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IPNG, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_IPNG))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_ipng (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_IPNG, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_putchunk_pplt (mng_handle hHandle,
                                        mng_uint8  iDeltatype,
                                        mng_uint32 iCount)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_PPLT, mng_init_general, mng_free_general, mng_read_pplt, mng_write_pplt, mng_assign_general, 0, 0, sizeof(mng_pplt)};
#else
          {MNG_UINT_PPLT, mng_init_pplt, mng_free_pplt, mng_read_pplt, mng_write_pplt, mng_assign_pplt, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PPLT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_PPLT))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_pplt (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_PPLT, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_ppltp)pChunk)->iDeltatype = iDeltatype;
  ((mng_ppltp)pChunk)->iCount     = iCount;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_putchunk_pplt_entry (mng_handle hHandle,
                                              mng_uint32 iEntry,
                                              mng_uint16 iRed,
                                              mng_uint16 iGreen,
                                              mng_uint16 iBlue,
                                              mng_uint16 iAlpha)
{
  mng_datap       pData;
  mng_chunkp      pChunk;
  mng_pplt_entryp pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PPLT_ENTRY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)

  pChunk = pData->pLastchunk;          /* last one must have been PPLT ! */

  if (((mng_chunk_headerp)pChunk)->iChunkname != MNG_UINT_PPLT)
    MNG_ERROR (pData, MNG_NOCORRCHUNK)

                                       /* index out of bounds ? */
  if (iEntry >= ((mng_ppltp)pChunk)->iCount)
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)
                                       /* address proper entry */
  pEntry = (mng_pplt_entryp)(((mng_ppltp)pChunk)->aEntries) + iEntry;

  pEntry->iRed   = (mng_uint8)iRed;    /* fill the entry */
  pEntry->iGreen = (mng_uint8)iGreen;
  pEntry->iBlue  = (mng_uint8)iBlue;
  pEntry->iAlpha = (mng_uint8)iAlpha;
  pEntry->bUsed  = MNG_TRUE;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_PPLT_ENTRY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
mng_retcode MNG_DECL mng_putchunk_ijng (mng_handle hHandle)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_IJNG, mng_init_general, mng_free_general, mng_read_ijng, mng_write_ijng, mng_assign_general, 0, 0, sizeof(mng_ijng)};
#else
          {MNG_UINT_IJNG, mng_init_ijng, mng_free_ijng, mng_read_ijng, mng_write_ijng, mng_assign_ijng, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IJNG, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_IJNG))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_ijng (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_IJNG, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_IJNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode MNG_DECL mng_putchunk_drop (mng_handle   hHandle,
                                        mng_uint32   iCount,
                                        mng_chunkidp pChunknames)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_DROP, mng_init_general, mng_free_drop, mng_read_drop, mng_write_drop, mng_assign_drop, 0, 0, sizeof(mng_drop)};
#else
          {MNG_UINT_DROP, mng_init_drop, mng_free_drop, mng_read_drop, mng_write_drop, mng_assign_drop, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DROP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_DROP))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_drop (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_DROP, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_dropp)pChunk)->iCount = iCount;

  if (iCount)
  {
    mng_uint32 iSize = iCount * sizeof (mng_chunkid);

    MNG_ALLOC (pData, ((mng_dropp)pChunk)->pChunknames, iSize);
    MNG_COPY (((mng_dropp)pChunk)->pChunknames, pChunknames, iSize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DROP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_DBYK
mng_retcode MNG_DECL mng_putchunk_dbyk (mng_handle  hHandle,
                                        mng_chunkid iChunkname,
                                        mng_uint8   iPolarity,
                                        mng_uint32  iKeywordssize,
                                        mng_pchar   zKeywords)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_DBYK, mng_init_general, mng_free_dbyk, mng_read_dbyk, mng_write_dbyk, mng_assign_dbyk, 0, 0, sizeof(mng_dbyk)};
#else
          {MNG_UINT_DBYK, mng_init_dbyk, mng_free_dbyk, mng_read_dbyk, mng_write_dbyk, mng_assign_dbyk, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DBYK, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_DBYK))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_dbyk (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_DBYK, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_dbykp)pChunk)->iChunkname    = iChunkname;
  ((mng_dbykp)pChunk)->iPolarity     = iPolarity;
  ((mng_dbykp)pChunk)->iKeywordssize = iKeywordssize;

  if (iKeywordssize)
  {
    MNG_ALLOC (pData, ((mng_dbykp)pChunk)->zKeywords, iKeywordssize + 1);
    MNG_COPY (((mng_dbykp)pChunk)->zKeywords, zKeywords, iKeywordssize);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_DBYK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
mng_retcode MNG_DECL mng_putchunk_ordr (mng_handle hHandle,
                                        mng_uint32 iCount)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_ORDR, mng_init_general, mng_free_ordr, mng_read_ordr, mng_write_ordr, mng_assign_ordr, 0, 0, sizeof(mng_ordr)};
#else
          {MNG_UINT_ORDR, mng_init_ordr, mng_free_ordr, mng_read_ordr, mng_write_ordr, mng_assign_ordr, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ORDR, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_ORDR))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_ordr (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_ORDR, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_ordrp)pChunk)->iCount = iCount;

  if (iCount)
    MNG_ALLOC (pData, ((mng_ordrp)pChunk)->pEntries, iCount * sizeof (mng_ordr_entry));

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ORDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_SKIPCHUNK_ORDR
mng_retcode MNG_DECL mng_putchunk_ordr_entry (mng_handle  hHandle,
                                              mng_uint32  iEntry,
                                              mng_chunkid iChunkname,
                                              mng_uint8   iOrdertype)
{
  mng_datap       pData;
  mng_chunkp      pChunk;
  mng_ordr_entryp pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ORDR_ENTRY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)

  pChunk = pData->pLastchunk;          /* last one must have been ORDR ! */

  if (((mng_chunk_headerp)pChunk)->iChunkname != MNG_UINT_ORDR)
    MNG_ERROR (pData, MNG_NOCORRCHUNK)
                                       /* index out of bounds ? */
  if (iEntry >= ((mng_ordrp)pChunk)->iCount)
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)
                                       /* address proper entry */
  pEntry = ((mng_ordrp)pChunk)->pEntries + iEntry;

  pEntry->iChunkname = iChunkname;     /* fill the entry */
  pEntry->iOrdertype = iOrdertype;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_ORDR_ENTRY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
mng_retcode MNG_DECL mng_putchunk_magn (mng_handle hHandle,
                                        mng_uint16 iFirstid,
                                        mng_uint16 iLastid,
                                        mng_uint16 iMethodX,
                                        mng_uint16 iMX,
                                        mng_uint16 iMY,
                                        mng_uint16 iML,
                                        mng_uint16 iMR,
                                        mng_uint16 iMT,
                                        mng_uint16 iMB,
                                        mng_uint16 iMethodY)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_MAGN, mng_init_general, mng_free_general, mng_read_magn, mng_write_magn, mng_assign_general, 0, 0, sizeof(mng_magn)};
#else
          {MNG_UINT_MAGN, mng_init_magn, mng_free_magn, mng_read_magn, mng_write_magn, mng_assign_magn, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MAGN, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_MAGN))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_magn (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_MAGN, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_magnp)pChunk)->iFirstid = iFirstid;
  ((mng_magnp)pChunk)->iLastid  = iLastid;
  ((mng_magnp)pChunk)->iMethodX = (mng_uint8)iMethodX;
  ((mng_magnp)pChunk)->iMX      = iMX;
  ((mng_magnp)pChunk)->iMY      = iMY;
  ((mng_magnp)pChunk)->iML      = iML;
  ((mng_magnp)pChunk)->iMR      = iMR;
  ((mng_magnp)pChunk)->iMT      = iMT;
  ((mng_magnp)pChunk)->iMB      = iMB;
  ((mng_magnp)pChunk)->iMethodY = (mng_uint8)iMethodY;

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MAGN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
MNG_EXT mng_retcode MNG_DECL mng_putchunk_mpng (mng_handle hHandle,
                                                mng_uint32 iFramewidth,
                                                mng_uint32 iFrameheight,
                                                mng_uint16 iNumplays,
                                                mng_uint16 iTickspersec,
                                                mng_uint8  iCompressionmethod,
                                                mng_uint32 iCount)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_mpNG, mng_init_general, mng_free_mpng, mng_read_mpng, mng_write_mpng, mng_assign_mpng, 0, 0, sizeof(mng_mpng)};
#else
          {MNG_UINT_mpNG, mng_init_mpng, mng_free_mpng, mng_read_mpng, mng_write_mpng, mng_assign_mpng, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MPNG, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a IHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_IHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_mpng (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_mpNG, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_mpngp)pChunk)->iFramewidth        = iFramewidth;
  ((mng_mpngp)pChunk)->iFrameheight       = iFrameheight;
  ((mng_mpngp)pChunk)->iNumplays          = iNumplays;
  ((mng_mpngp)pChunk)->iTickspersec       = iTickspersec;
  ((mng_mpngp)pChunk)->iCompressionmethod = iCompressionmethod;
  ((mng_mpngp)pChunk)->iFramessize        = iCount * sizeof (mng_mpng_frame);

  if (iCount)
    MNG_ALLOC (pData, ((mng_mpngp)pChunk)->pFrames, ((mng_mpngp)pChunk)->iFramessize);

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
MNG_EXT mng_retcode MNG_DECL mng_putchunk_mpng_frame (mng_handle hHandle,
                                                      mng_uint32 iEntry,
                                                      mng_uint32 iX,
                                                      mng_uint32 iY,
                                                      mng_uint32 iWidth,
                                                      mng_uint32 iHeight,
                                                      mng_int32  iXoffset,
                                                      mng_int32  iYoffset,
                                                      mng_uint16 iTicks)
{
  mng_datap       pData;
  mng_chunkp      pChunk;
  mng_mpng_framep pFrame;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MPNG_FRAME, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a IHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_IHDR)
    MNG_ERROR (pData, MNG_NOHEADER)

  pChunk = pData->pLastchunk;          /* last one must have been mpNG ! */

  if (((mng_chunk_headerp)pChunk)->iChunkname != MNG_UINT_mpNG)
    MNG_ERROR (pData, MNG_NOCORRCHUNK)
                                       /* index out of bounds ? */
  if (iEntry >= (((mng_mpngp)pChunk)->iFramessize / sizeof (mng_mpng_frame)))
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)
                                       /* address proper entry */
  pFrame = ((mng_mpngp)pChunk)->pFrames + iEntry;
                                       /* fill entry */
  pFrame->iX        = iX;
  pFrame->iY        = iY;
  pFrame->iWidth    = iWidth;
  pFrame->iHeight   = iHeight;
  pFrame->iXoffset  = iXoffset;
  pFrame->iYoffset  = iYoffset;
  pFrame->iTicks    = iTicks;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_MPNG_FRAME, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_evNT
mng_retcode MNG_DECL mng_putchunk_evnt (mng_handle hHandle,
                                        mng_uint32 iCount)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_evNT, mng_init_general, mng_free_evnt, mng_read_evnt, mng_write_evnt, mng_assign_evnt, 0, 0, sizeof(mng_evnt)};
#else
          {MNG_UINT_evNT, mng_init_evnt, mng_free_evnt, mng_read_evnt, mng_write_evnt, mng_assign_evnt, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_EVNT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, MNG_UINT_evNT))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_evnt (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_evNT, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_evntp)pChunk)->iCount = iCount;

  if (iCount)
    MNG_ALLOC (pData, ((mng_evntp)pChunk)->pEntries, iCount * sizeof (mng_evnt_entry));

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_EVNT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_evnt_entry (mng_handle hHandle,
                                              mng_uint32 iEntry,
                                              mng_uint8  iEventtype,
                                              mng_uint8  iMasktype,
                                              mng_int32  iLeft,
                                              mng_int32  iRight,
                                              mng_int32  iTop,
                                              mng_int32  iBottom,
                                              mng_uint16 iObjectid,
                                              mng_uint8  iIndex,
                                              mng_uint32 iSegmentnamesize,
                                              mng_pchar  zSegmentname)
{
  mng_datap       pData;
  mng_chunkp      pChunk;
  mng_evnt_entryp pEntry;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_EVNT_ENTRY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a MHDR first! */
  if (pData->iFirstchunkadded != MNG_UINT_MHDR)
    MNG_ERROR (pData, MNG_NOHEADER)

  pChunk = pData->pLastchunk;          /* last one must have been evNT ! */

  if (((mng_chunk_headerp)pChunk)->iChunkname != MNG_UINT_evNT)
    MNG_ERROR (pData, MNG_NOCORRCHUNK)
                                       /* index out of bounds ? */
  if (iEntry >= ((mng_evntp)pChunk)->iCount)
    MNG_ERROR (pData, MNG_INVALIDENTRYIX)
                                       /* address proper entry */
  pEntry = ((mng_evntp)pChunk)->pEntries + iEntry;
                                       /* fill entry */
  pEntry->iEventtype       = iEventtype;
  pEntry->iMasktype        = iMasktype;
  pEntry->iLeft            = iLeft;
  pEntry->iRight           = iRight;
  pEntry->iTop             = iTop;
  pEntry->iBottom          = iBottom;
  pEntry->iObjectid        = iObjectid;
  pEntry->iIndex           = iIndex;
  pEntry->iSegmentnamesize = iSegmentnamesize;

  if (iSegmentnamesize)
  {
    MNG_ALLOC (pData, pEntry->zSegmentname, iSegmentnamesize + 1);
    MNG_COPY (pEntry->zSegmentname, zSegmentname, iSegmentnamesize);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_EVNT_ENTRY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putchunk_unknown (mng_handle  hHandle,
                                           mng_chunkid iChunkname,
                                           mng_uint32  iRawlen,
                                           mng_ptr     pRawdata)
{
  mng_datap        pData;
  mng_chunkp       pChunk;
  mng_retcode      iRetcode;
#ifndef MNG_OPTIMIZE_CHUNKREADER
  mng_chunk_header sChunkheader =
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
          {MNG_UINT_HUH, mng_init_general, mng_free_unknown, mng_read_unknown, mng_write_unknown, mng_assign_unknown, 0, 0, sizeof(mng_unknown_chunk)};
#else
          {MNG_UINT_HUH, mng_init_unknown, mng_free_unknown, mng_read_unknown, mng_write_unknown, mng_assign_unknown, 0, 0};
#endif
#else
  mng_chunk_header sChunkheader;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_UNKNOWN, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must have had a header first! */
  if (pData->iFirstchunkadded == 0)
    MNG_ERROR (pData, MNG_NOHEADER)
                                       /* prevent misplaced TERM ! */
  if (!check_term (pData, iChunkname))
    MNG_ERROR (pData, MNG_TERMSEQERROR)
                                       /* create the chunk */
#ifndef MNG_OPTIMIZE_CHUNKREADER
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#else
  iRetcode = mng_init_unknown (pData, &sChunkheader, &pChunk);
#endif
#else
  mng_get_chunkheader(MNG_UINT_HUH, &sChunkheader);
  iRetcode = mng_init_general (pData, &sChunkheader, &pChunk);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fill the chunk */
  ((mng_unknown_chunkp)pChunk)->sHeader.iChunkname = iChunkname;
  ((mng_unknown_chunkp)pChunk)->iDatasize          = iRawlen;

  if (iRawlen)
  {
    MNG_ALLOC (pData, ((mng_unknown_chunkp)pChunk)->pData, iRawlen);
    MNG_COPY (((mng_unknown_chunkp)pChunk)->pData, pRawdata, iRawlen);
  }

  mng_add_chunk (pData, pChunk);       /* add it to the list */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTCHUNK_UNKNOWN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_INCLUDE_WRITE_PROCS */

/* ************************************************************************** */
/* ************************************************************************** */

mng_retcode MNG_DECL mng_getimgdata_seq (mng_handle        hHandle,
                                         mng_uint32        iSeqnr,
                                         mng_uint32        iCanvasstyle,
                                         mng_getcanvasline fGetcanvasline)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETIMGDATA_SEQ, MNG_LC_START);
#endif



#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETIMGDATA_SEQ, MNG_LC_END);
#endif

  return MNG_FNNOTIMPLEMENTED;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getimgdata_chunkseq (mng_handle        hHandle,
                                              mng_uint32        iSeqnr,
                                              mng_uint32        iCanvasstyle,
                                              mng_getcanvasline fGetcanvasline)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETIMGDATA_CHUNKSEQ, MNG_LC_START);
#endif



#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETIMGDATA_CHUNKSEQ, MNG_LC_END);
#endif

  return MNG_FNNOTIMPLEMENTED;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_getimgdata_chunk (mng_handle        hHandle,
                                           mng_handle        hChunk,
                                           mng_uint32        iCanvasstyle,
                                           mng_getcanvasline fGetcanvasline)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETIMGDATA_CHUNK, MNG_LC_START);
#endif



#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_GETIMGDATA_CHUNK, MNG_LC_END);
#endif

  return MNG_FNNOTIMPLEMENTED;
}

/* ************************************************************************** */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_WRITE_PROCS

/* ************************************************************************** */

mng_retcode MNG_DECL mng_putimgdata_ihdr (mng_handle        hHandle,
                                          mng_uint32        iWidth,
                                          mng_uint32        iHeight,
                                          mng_uint8         iColortype,
                                          mng_uint8         iBitdepth,
                                          mng_uint8         iCompression,
                                          mng_uint8         iFilter,
                                          mng_uint8         iInterlace,
                                          mng_uint32        iCanvasstyle,
                                          mng_getcanvasline fGetcanvasline)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTIMGDATA_IHDR, MNG_LC_START);
#endif



#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTIMGDATA_IHDR, MNG_LC_END);
#endif

  return MNG_FNNOTIMPLEMENTED;
}

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
mng_retcode MNG_DECL mng_putimgdata_jhdr (mng_handle        hHandle,
                                          mng_uint32        iWidth,
                                          mng_uint32        iHeight,
                                          mng_uint8         iColortype,
                                          mng_uint8         iBitdepth,
                                          mng_uint8         iCompression,
                                          mng_uint8         iInterlace,
                                          mng_uint8         iAlphaBitdepth,
                                          mng_uint8         iAlphaCompression,
                                          mng_uint8         iAlphaFilter,
                                          mng_uint8         iAlphaInterlace,
                                          mng_uint32        iCanvasstyle,
                                          mng_getcanvasline fGetcanvasline)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTIMGDATA_JHDR, MNG_LC_START);
#endif



#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_PUTIMGDATA_JHDR, MNG_LC_END);
#endif

  return MNG_FNNOTIMPLEMENTED;
}
#endif

/* ************************************************************************** */

mng_retcode MNG_DECL mng_updatemngheader (mng_handle hHandle,
                                          mng_uint32 iFramecount,
                                          mng_uint32 iLayercount,
                                          mng_uint32 iPlaytime)
{
  mng_datap  pData;
  mng_chunkp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_UPDATEMNGHEADER, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must be a MNG animation! */
  if ((pData->eImagetype != mng_it_mng) || (pData->iFirstchunkadded != MNG_UINT_MHDR))
    MNG_ERROR (pData, MNG_NOMHDR)

  pChunk = pData->pFirstchunk;         /* get the first chunk */
                                       /* and update the variables */
  ((mng_mhdrp)pChunk)->iFramecount = iFramecount;
  ((mng_mhdrp)pChunk)->iLayercount = iLayercount;
  ((mng_mhdrp)pChunk)->iPlaytime   = iPlaytime;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_UPDATEMNGHEADER, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode MNG_DECL mng_updatemngsimplicity (mng_handle hHandle,
                                              mng_uint32 iSimplicity)
{
  mng_datap  pData;
  mng_chunkp pChunk;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_UPDATEMNGSIMPLICITY, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)            /* check validity handle */
  pData = (mng_datap)hHandle;          /* and make it addressable */

  if (!pData->bCreating)               /* aren't we creating a new file ? */
    MNG_ERROR (pData, MNG_FUNCTIONINVALID)
                                       /* must be a MNG animation! */
  if ((pData->eImagetype != mng_it_mng) || (pData->iFirstchunkadded != MNG_UINT_MHDR))
    MNG_ERROR (pData, MNG_NOMHDR)

  pChunk = pData->pFirstchunk;         /* get the first chunk */
                                       /* and update the variable */
  ((mng_mhdrp)pChunk)->iSimplicity = iSimplicity;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_UPDATEMNGSIMPLICITY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_INCLUDE_WRITE_PROCS */

/* ************************************************************************** */

#endif /* MNG_ACCESS_CHUNKS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */



