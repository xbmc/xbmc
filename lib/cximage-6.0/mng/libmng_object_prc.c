/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_object_prc.c       copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Object processing routines (implementation)                * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the internal object processing routines  * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *                                                                        * */
/* *             0.5.2 - 05/20/2000 - G.Juyn                                * */
/* *             - fixed to support JNG objects                             * */
/* *             0.5.2 - 05/24/2000 - G.Juyn                                * */
/* *             - added support for global color-chunks in animation       * */
/* *             - added support for global PLTE,tRNS,bKGD in animation     * */
/* *             - added SAVE & SEEK animation objects                      * */
/* *             0.5.2 - 05/29/2000 - G.Juyn                                * */
/* *             - added initialization of framenr/layernr/playtime         * */
/* *             - changed ani_object create routines not to return the     * */
/* *               created object (wasn't necessary)                        * */
/* *             0.5.2 - 05/30/2000 - G.Juyn                                * */
/* *             - added object promotion routine (PROM handling)           * */
/* *             - added ani-object routines for delta-image processing     * */
/* *             - added compression/filter/interlace fields to             * */
/* *               object-buffer for delta-image processing                 * */
/* *                                                                        * */
/* *             0.5.3 - 06/17/2000 - G.Juyn                                * */
/* *             - changed support for delta-image processing               * */
/* *             0.5.3 - 06/20/2000 - G.Juyn                                * */
/* *             - fixed some small things (as precaution)                  * */
/* *             0.5.3 - 06/21/2000 - G.Juyn                                * */
/* *             - added processing of PLTE/tRNS & color-info for           * */
/* *               delta-images in the ani_objects chain                    * */
/* *             0.5.3 - 06/22/2000 - G.Juyn                                * */
/* *             - added support for PPLT chunk                             * */
/* *                                                                        * */
/* *             0.9.1 - 07/07/2000 - G.Juyn                                * */
/* *             - added support for freeze/restart/resume & go_xxxx        * */
/* *             0.9.1 - 07/16/2000 - G.Juyn                                * */
/* *             - fixed support for mng_display() after mng_read()         * */
/* *                                                                        * */
/* *             0.9.2 - 07/29/2000 - G.Juyn                                * */
/* *             - fixed small bugs in display processing                   * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/07/2000 - G.Juyn                                * */
/* *             - B111300 - fixup for improved portability                 * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 09/10/2000 - G.Juyn                                * */
/* *             - fixed DEFI behavior                                      * */
/* *             0.9.3 - 10/17/2000 - G.Juyn                                * */
/* *             - added valid-flag to stored objects for read() / display()* */
/* *             - added routine to discard "invalid" objects               * */
/* *             0.9.3 - 10/18/2000 - G.Juyn                                * */
/* *             - fixed delta-processing behavior                          * */
/* *             0.9.3 - 10/19/2000 - G.Juyn                                * */
/* *             - added storage for pixel-/alpha-sampledepth for delta's   * */
/* *                                                                        * */
/* *             0.9.4 -  1/18/2001 - G.Juyn                                * */
/* *             - removed "old" MAGN methods 3 & 4                         * */
/* *             - added "new" MAGN methods 3, 4 & 5                        * */
/* *                                                                        * */
/* *             0.9.5 -  1/22/2001 - G.Juyn                                * */
/* *             - B129681 - fixed compiler warnings SGI/Irix               * */
/* *                                                                        * */
/* *             1.0.2 - 06/23/2001 - G.Juyn                                * */
/* *             - added optimization option for MNG-video playback         * */
/* *                                                                        * */
/* *             1.0.5 - 08/15/2002 - G.Juyn                                * */
/* *             - completed PROM support                                   * */
/* *             1.0.5 - 08/16/2002 - G.Juyn                                * */
/* *             - completed MAGN support (16-bit functions)                * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             1.0.5 - 09/13/2002 - G.Juyn                                * */
/* *             - fixed read/write of MAGN chunk                           * */
/* *             1.0.5 - 09/15/2002 - G.Juyn                                * */
/* *             - added event handling for dynamic MNG                     * */
/* *             1.0.5 - 09/20/2002 - G.Juyn                                * */
/* *             - added support for PAST                                   * */
/* *             1.0.5 - 09/23/2002 - G.Juyn                                * */
/* *             - fixed reset_object_detail to clear old buffer            * */
/* *             - added in-memory color-correction of abstract images      * */
/* *             1.0.5 - 10/05/2002 - G.Juyn                                * */
/* *             - fixed problem with cloned objects marked as invalid      * */
/* *             - fixed problem cloning frozen object_buffers              * */
/* *             1.0.5 - 10/07/2002 - G.Juyn                                * */
/* *             - fixed DISC support                                       * */
/* *             1.0.5 - 11/04/2002 - G.Juyn                                * */
/* *             - fixed goframe/golayer/gotime processing                  * */
/* *             1.0.5 - 11/07/2002 - G.Juyn                                * */
/* *             - fixed magnification bug with object 0                    * */
/* *             1.0.5 - 01/19/2003 - G.Juyn                                * */
/* *             - B664911 - fixed buffer overflow during init              * */
/* *                                                                        * */
/* *             1.0.6 - 04/19/2003 - G.Juyn                                * */
/* *             - fixed problem with infinite loops during readdisplay()   * */
/* *             1.0.6 - 05/25/2003 - G.R-P                                 * */
/* *             - added MNG_SKIPCHUNK_cHNK footprint optimizations         * */
/* *             1.0.6 - 06/09/2003 - G. R-P                                * */
/* *             - added conditionals around 8-bit magn routines            * */
/* *             1.0.6 - 07/07/2003 - G.R-P                                 * */
/* *             - added conditionals around some JNG-supporting code       * */
/* *             - removed conditionals around 8-bit magn routines          * */
/* *             - added conditionals around delta-png and 16-bit code      * */
/* *             1.0.6 - 07/14/2003 - G.R-P                                 * */
/* *             - added MNG_NO_LOOP_SIGNALS_SUPPORTED conditional          * */
/* *             1.0.6 - 07/29/2003 - G.Juyn                                * */
/* *             - fixed invalid test in promote_imageobject                * */
/* *             1.0.6 - 07/29/2003 - G.R-P.                                * */
/* *             - added conditionals around PAST chunk support             * */
/* *             1.0.6 - 08/17/2003 - G.R-P.                                * */
/* *             - added conditionals around MAGN chunk support             * */
/* *                                                                        * */
/* *             1.0.7 - 03/21/2004 - G.Juyn                                * */
/* *             - fixed some 64-bit platform compiler warnings             * */
/* *                                                                        * */
/* *             1.0.9 - 10/10/2004 - G.R-P.                                * */
/* *             - added MNG_NO_1_2_4BIT_SUPPORT support                    * */
/* *             1.0.9 - 12/05/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_OBJCLEANUP                * */
/* *             1.0.9 - 12/11/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_DISPLAYCALLS              * */
/* *             1.0.9 - 12/31/2004 - G.R-P.                                * */
/* *             - fixed warnings about possible uninitialized pointers     * */
/* *             1.0.9 - 01/02/2005 - G.Juyn                                * */
/* *             - fixing some compiler-warnings                            * */
/* *                                                                        * */
/* *             1.0.10 - 02/07/2005 - G.Juyn                               * */
/* *             - fixed some compiler-warnings                             * */
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
#include "libmng_objects.h"
#include "libmng_display.h"
#include "libmng_pixels.h"
#include "libmng_object_prc.h"
#include "libmng_cms.h"

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_DISPLAY_PROCS

/* ************************************************************************** */
/* *                                                                        * */
/* * Generic object routines                                                * */
/* *                                                                        * */
/* ************************************************************************** */

mng_retcode mng_drop_invalid_objects (mng_datap pData)
{
  mng_objectp       pObject;
  mng_objectp       pNext;
  mng_cleanupobject fCleanup;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DROP_INVALID_OBJECTS, MNG_LC_START);
#endif

  pObject = pData->pFirstimgobj;       /* get first stored image-object (if any) */

  while (pObject)                      /* more objects to check ? */
  {
    pNext = ((mng_object_headerp)pObject)->pNext;
                                       /* invalid ? */
    if (!((mng_imagep)pObject)->bValid)
    {                                  /* call appropriate cleanup */
      fCleanup = ((mng_object_headerp)pObject)->fCleanup;
      fCleanup (pData, pObject);
    }

    pObject = pNext;                   /* neeeext */
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DROP_INVALID_OBJECTS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifdef MNG_OPTIMIZE_OBJCLEANUP
MNG_LOCAL mng_retcode create_obj_general (mng_datap          pData,
                                          mng_size_t         iObjsize,
                                          mng_cleanupobject  fCleanup,
                                          mng_processobject  fProcess,
                                          mng_ptr            *ppObject)
{
  mng_object_headerp pWork;

  MNG_ALLOC (pData, pWork, iObjsize);

  pWork->fCleanup = fCleanup;
  pWork->fProcess = fProcess;
  pWork->iObjsize = iObjsize;
  *ppObject       = (mng_ptr)pWork;

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode mng_free_obj_general (mng_datap   pData,
                                            mng_objectp pObject)
{
  MNG_FREEX (pData, pObject, ((mng_object_headerp)pObject)->iObjsize);
  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */
/* *                                                                        * */
/* * Image-data-object routines                                             * */
/* *                                                                        * */
/* * these handle the "object buffer" as defined by the MNG specification   * */
/* *                                                                        * */
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
                                        mng_imagedatap *ppObject)
{
  mng_imagedatap pImagedata;
  mng_uint32 iSamplesize = 0;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_IMGDATAOBJECT, MNG_LC_START);
#endif
                                       /* get a buffer */
#ifdef MNG_OPTIMIZE_OBJCLEANUP
  {
    mng_ptr pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_imagedata),
                                               (mng_cleanupobject)mng_free_imagedataobject,
                                               MNG_NULL, &pTemp);
    if (iRetcode)
      return iRetcode;
    pImagedata = (mng_imagedatap)pTemp;
  }
#else
  MNG_ALLOC (pData, pImagedata, sizeof (mng_imagedata));
                                       /* fill the appropriate fields */
  pImagedata->sHeader.fCleanup   = (mng_cleanupobject)mng_free_imagedataobject;
  pImagedata->sHeader.fProcess   = MNG_NULL;
#endif
  pImagedata->iRefcount          = 1;
  pImagedata->bFrozen            = MNG_FALSE;
  pImagedata->bConcrete          = bConcrete;
  pImagedata->bViewable          = bViewable;
  pImagedata->iWidth             = iWidth;
  pImagedata->iHeight            = iHeight;
  pImagedata->iBitdepth          = iBitdepth;
  pImagedata->iColortype         = iColortype;
  pImagedata->iCompression       = iCompression;
  pImagedata->iFilter            = iFilter;
  pImagedata->iInterlace         = iInterlace;
  pImagedata->bCorrected         = MNG_FALSE;
  pImagedata->iAlphabitdepth     = 0;
  pImagedata->iJHDRcompression   = 0;
  pImagedata->iJHDRinterlace     = 0;
  pImagedata->iPixelsampledepth  = iBitdepth;
  pImagedata->iAlphasampledepth  = iBitdepth;
                                       /* determine samplesize from color_type/bit_depth */
  switch (iColortype)                  /* for < 8-bit samples we just reserve 8 bits */
  {
    case  0  : ;                       /* gray */
    case  8  : {                       /* JPEG gray */
#ifndef MNG_NO_16BIT_SUPPORT
                 if (iBitdepth > 8)
                   iSamplesize = 2;
                 else
#endif
                   iSamplesize = 1;

                 break;
               }
    case  2  : ;                       /* rgb */
    case 10  : {                       /* JPEG rgb */
#ifndef MNG_NO_16BIT_SUPPORT
                 if (iBitdepth > 8)
                   iSamplesize = 6;
                 else
#endif
                   iSamplesize = 3;

                 break;
               }
    case  3  : {                       /* indexed */
                 iSamplesize = 1;
                 break;
               }
    case  4  : ;                       /* gray+alpha */
    case 12  : {                       /* JPEG gray+alpha */
#ifndef MNG_NO_16BIT_SUPPORT
                 if (iBitdepth > 8)
                   iSamplesize = 4;
                 else
#endif
                   iSamplesize = 2;

                 break;
               }
    case  6  : ;                       /* rgb+alpha */
    case 14  : {                       /* JPEG rgb+alpha */
#ifndef MNG_NO_16BIT_SUPPORT
                 if (iBitdepth > 8)
                   iSamplesize = 8;
                 else
#endif
                   iSamplesize = 4;

                 break;
               }
  }
                                       /* make sure we remember all this */
  pImagedata->iSamplesize  = iSamplesize;
  pImagedata->iRowsize     = iSamplesize * iWidth;
  pImagedata->iImgdatasize = pImagedata->iRowsize * iHeight;

  if (pImagedata->iImgdatasize)        /* need a buffer ? */
  {                                    /* so allocate it */
    MNG_ALLOCX (pData, pImagedata->pImgdata, pImagedata->iImgdatasize);

    if (!pImagedata->pImgdata)         /* enough memory ? */
    {
      MNG_FREEX (pData, pImagedata, sizeof (mng_imagedata));
      MNG_ERROR (pData, MNG_OUTOFMEMORY);
    }
  }
                                       /* check global stuff */
  pImagedata->bHasGAMA           = pData->bHasglobalGAMA;
#ifndef MNG_SKIPCHUNK_cHRM
  pImagedata->bHasCHRM           = pData->bHasglobalCHRM;
#endif
  pImagedata->bHasSRGB           = pData->bHasglobalSRGB;
#ifndef MNG_SKIPCHUNK_iCCP
  pImagedata->bHasICCP           = pData->bHasglobalICCP;
#endif
#ifndef MNG_SKIPCHUNK_bKGD
  pImagedata->bHasBKGD           = pData->bHasglobalBKGD;
#endif

  if (pData->bHasglobalGAMA)           /* global gAMA present ? */
    pImagedata->iGamma           = pData->iGlobalGamma;

#ifndef MNG_SKIPCHUNK_cHRM
  if (pData->bHasglobalCHRM)           /* global cHRM present ? */
  {
    pImagedata->iWhitepointx     = pData->iGlobalWhitepointx;
    pImagedata->iWhitepointy     = pData->iGlobalWhitepointy;
    pImagedata->iPrimaryredx     = pData->iGlobalPrimaryredx;
    pImagedata->iPrimaryredy     = pData->iGlobalPrimaryredy;
    pImagedata->iPrimarygreenx   = pData->iGlobalPrimarygreenx;
    pImagedata->iPrimarygreeny   = pData->iGlobalPrimarygreeny;
    pImagedata->iPrimarybluex    = pData->iGlobalPrimarybluex;
    pImagedata->iPrimarybluey    = pData->iGlobalPrimarybluey;
  }
#endif

  if (pData->bHasglobalSRGB)           /* glbal sRGB present ? */
    pImagedata->iRenderingintent = pData->iGlobalRendintent;

#ifndef MNG_SKIPCHUNK_iCCP
  if (pData->bHasglobalICCP)           /* glbal iCCP present ? */
  {
    pImagedata->iProfilesize     = pData->iGlobalProfilesize;

    if (pImagedata->iProfilesize)
    {
      MNG_ALLOCX (pData, pImagedata->pProfile, pImagedata->iProfilesize);

      if (!pImagedata->pProfile)       /* enough memory ? */
      {
        MNG_FREEX (pData, pImagedata->pImgdata, pImagedata->iImgdatasize);
        MNG_FREEX (pData, pImagedata, sizeof (mng_imagedata));
        MNG_ERROR (pData, MNG_OUTOFMEMORY);
      }

      MNG_COPY  (pImagedata->pProfile, pData->pGlobalProfile, pImagedata->iProfilesize);
    }
  }
#endif

#ifndef MNG_SKIPCHUNK_bKGD
  if (pData->bHasglobalBKGD)           /* global bKGD present ? */
  {
    pImagedata->iBKGDred         = pData->iGlobalBKGDred;
    pImagedata->iBKGDgreen       = pData->iGlobalBKGDgreen;
    pImagedata->iBKGDblue        = pData->iGlobalBKGDblue;
  }
#endif

  *ppObject = pImagedata;              /* return it */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_IMGDATAOBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_free_imagedataobject   (mng_datap      pData,
                                        mng_imagedatap pImagedata)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IMGDATAOBJECT, MNG_LC_START);
#endif

  if (pImagedata->iRefcount)           /* decrease reference count */
    pImagedata->iRefcount--;

  if (!pImagedata->iRefcount)          /* reached zero ? */
  {
#ifndef MNG_SKIPCHUNK_iCCP
    if (pImagedata->iProfilesize)      /* stored an iCCP profile ? */
      MNG_FREEX (pData, pImagedata->pProfile, pImagedata->iProfilesize);
#endif
    if (pImagedata->iImgdatasize)      /* sample-buffer present ? */
      MNG_FREEX (pData, pImagedata->pImgdata, pImagedata->iImgdatasize);
                                       /* drop the buffer */
    MNG_FREEX (pData, pImagedata, sizeof (mng_imagedata));
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IMGDATAOBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_clone_imagedataobject  (mng_datap      pData,
                                        mng_bool       bConcrete,
                                        mng_imagedatap pSource,
                                        mng_imagedatap *ppClone)
{
  mng_imagedatap pNewdata;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CLONE_IMGDATAOBJECT, MNG_LC_START);
#endif
                                       /* get a buffer */
  MNG_ALLOC (pData, pNewdata, sizeof (mng_imagedata));
                                       /* blatently copy the original buffer */
  MNG_COPY (pNewdata, pSource, sizeof (mng_imagedata));

  pNewdata->iRefcount = 1;             /* only the reference count */
  pNewdata->bConcrete = bConcrete;     /* and concrete-flag are different */
  pNewdata->bFrozen   = MNG_FALSE;

  if (pNewdata->iImgdatasize)          /* sample buffer present ? */
  {
    MNG_ALLOCX (pData, pNewdata->pImgdata, pNewdata->iImgdatasize);

    if (!pNewdata->pImgdata)           /* not enough memory ? */
    {
      MNG_FREEX (pData, pNewdata, sizeof (mng_imagedata));
      MNG_ERROR (pData, MNG_OUTOFMEMORY);
    }
                                       /* make a copy */
    MNG_COPY (pNewdata->pImgdata, pSource->pImgdata, pNewdata->iImgdatasize);
  }

#ifndef MNG_SKIPCHUNK_iCCP
  if (pNewdata->iProfilesize)          /* iCCP profile present ? */
  {
    MNG_ALLOCX (pData, pNewdata->pProfile, pNewdata->iProfilesize);

    if (!pNewdata->pProfile)           /* enough memory ? */
    {
      MNG_FREEX (pData, pNewdata, sizeof (mng_imagedata));
      MNG_ERROR (pData, MNG_OUTOFMEMORY);
    }
                                       /* make a copy */
    MNG_COPY (pNewdata->pProfile, pSource->pProfile, pNewdata->iProfilesize);
  }
#endif

  *ppClone = pNewdata;                 /* return the clone */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CLONE_IMGDATAOBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* *                                                                        * */
/* * Image-object routines                                                  * */
/* *                                                                        * */
/* * these handle the "object" as defined by the MNG specification          * */
/* *                                                                        * */
/* ************************************************************************** */

mng_retcode mng_create_imageobject (mng_datap  pData,
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
                                    mng_imagep *ppObject)
{
  mng_imagep     pImage;
  mng_imagep     pPrev, pNext;
  mng_retcode    iRetcode;
  mng_imagedatap pImgbuf;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_IMGOBJECT, MNG_LC_START);
#endif
                                       /* get a buffer */
  MNG_ALLOC (pData, pImage, sizeof (mng_image));
                                       /* now get a new "object buffer" */
  iRetcode = mng_create_imagedataobject (pData, bConcrete, bViewable,
                                         iWidth, iHeight, iBitdepth, iColortype,
                                         iCompression, iFilter, iInterlace,
                                         &pImgbuf);

  if (iRetcode)                        /* on error bail out */
  {
    MNG_FREEX (pData, pImage, sizeof (mng_image));
    return iRetcode;
  }
                                       /* fill the appropriate fields */
  pImage->sHeader.fCleanup = (mng_cleanupobject)mng_free_imageobject;
  pImage->sHeader.fProcess = MNG_NULL;
#ifdef MNG_OPTIMIZE_OBJCLEANUP
  pImage->sHeader.iObjsize = sizeof (mng_image);
#endif
  pImage->iId              = iId;
  pImage->bFrozen          = MNG_FALSE;
  pImage->bVisible         = bVisible;
  pImage->bViewable        = bViewable;
  pImage->bValid           = (mng_bool)((pData->bDisplaying) &&
                                        ((pData->bRunning) || (pData->bSearching)) &&
                                        (!pData->bFreezing));
  pImage->iPosx            = iPosx;
  pImage->iPosy            = iPosy;
  pImage->bClipped         = bClipped;
  pImage->iClipl           = iClipl;
  pImage->iClipr           = iClipr;
  pImage->iClipt           = iClipt;
  pImage->iClipb           = iClipb;
#ifndef MNG_SKIPCHUNK_MAGN
  pImage->iMAGN_MethodX    = 0;
  pImage->iMAGN_MethodY    = 0;
  pImage->iMAGN_MX         = 0;
  pImage->iMAGN_MY         = 0;
  pImage->iMAGN_ML         = 0;
  pImage->iMAGN_MR         = 0;
  pImage->iMAGN_MT         = 0;
  pImage->iMAGN_MB         = 0;
#endif
#ifndef MNG_SKIPCHUNK_PAST
  pImage->iPastx           = 0;
  pImage->iPasty           = 0;
#endif
  pImage->pImgbuf          = pImgbuf;

  if (iId)                             /* only if not object 0 ! */
  {                                    /* find previous lower object-id */
    pPrev = (mng_imagep)pData->pLastimgobj;

    while ((pPrev) && (pPrev->iId > iId))
      pPrev = (mng_imagep)pPrev->sHeader.pPrev;

    if (pPrev)                         /* found it ? */
    {
      pImage->sHeader.pPrev = pPrev;   /* than link it in place */
      pImage->sHeader.pNext = pPrev->sHeader.pNext;
      pPrev->sHeader.pNext  = pImage;
    }
    else                               /* if not found, it becomes the first ! */
    {
      pImage->sHeader.pNext = pData->pFirstimgobj;
      pData->pFirstimgobj   = pImage;
    }

    pNext                   = (mng_imagep)pImage->sHeader.pNext;

    if (pNext)
      pNext->sHeader.pPrev  = pImage;
    else
      pData->pLastimgobj    = pImage;
    
  }  

  *ppObject = pImage;                  /* and return the new buffer */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_IMGOBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;                  /* okido */
}

/* ************************************************************************** */

mng_retcode mng_free_imageobject (mng_datap  pData,
                                  mng_imagep pImage)
{
  mng_retcode    iRetcode;
  mng_imagep     pPrev   = pImage->sHeader.pPrev;
  mng_imagep     pNext   = pImage->sHeader.pNext;
  mng_imagedatap pImgbuf = pImage->pImgbuf;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IMGOBJECT, MNG_LC_START);
#endif

  if (pImage->iId)                     /* not for object 0 */
  {
    if (pPrev)                         /* unlink from the list first ! */
      pPrev->sHeader.pNext = pImage->sHeader.pNext;
    else
      pData->pFirstimgobj  = pImage->sHeader.pNext;

    if (pNext)
      pNext->sHeader.pPrev = pImage->sHeader.pPrev;
    else
      pData->pLastimgobj   = pImage->sHeader.pPrev;

  }
                                       /* unlink the image-data buffer */
  iRetcode = mng_free_imagedataobject (pData, pImgbuf);
                                       /* drop its own buffer */
  MNG_FREEX (pData, pImage, sizeof (mng_image));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_IMGOBJECT, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

mng_imagep mng_find_imageobject (mng_datap  pData,
                                 mng_uint16 iId)
{
  mng_imagep pImage = (mng_imagep)pData->pFirstimgobj;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (pData, MNG_FN_FIND_IMGOBJECT, MNG_LC_START);
#endif
                                       /* look up the right id */
  while ((pImage) && (pImage->iId != iId))
    pImage = (mng_imagep)pImage->sHeader.pNext;

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
  if ((!pImage) && (pData->eImagetype == mng_it_mpng))
    pImage = pData->pObjzero;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (pData, MNG_FN_FIND_IMGOBJECT, MNG_LC_END);
#endif

  return pImage;
}

/* ************************************************************************** */

mng_retcode mng_clone_imageobject (mng_datap  pData,
                                   mng_uint16 iId,
                                   mng_bool   bPartial,
                                   mng_bool   bVisible,
                                   mng_bool   bAbstract,
                                   mng_bool   bHasloca,
                                   mng_uint8  iLocationtype,
                                   mng_int32  iLocationx,
                                   mng_int32  iLocationy,
                                   mng_imagep pSource,
                                   mng_imagep *ppClone)
{
  mng_imagep     pNew;
  mng_imagep     pPrev, pNext;
  mng_retcode    iRetcode;
  mng_imagedatap pImgbuf;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CLONE_IMGOBJECT, MNG_LC_START);
#endif

#ifndef MNG_SKIPCHUNK_MAGN
  if ((pSource->iId) &&                /* needs magnification ? */
      ((pSource->iMAGN_MethodX) || (pSource->iMAGN_MethodY)))
  {
    iRetcode = mng_magnify_imageobject (pData, pSource);

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }
#endif
                                       /* get a buffer */
#ifdef MNG_OPTIMIZE_OBJCLEANUP
  {
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_image),
                                               (mng_cleanupobject)mng_free_imageobject,
                                               MNG_NULL, &pTemp);
    if (iRetcode)
      return iRetcode;
    pNew = (mng_imagep)pTemp;
  }
#else
  MNG_ALLOC (pData, pNew, sizeof (mng_image));
                                       /* fill or copy the appropriate fields */
  pNew->sHeader.fCleanup = (mng_cleanupobject)mng_free_imageobject;
  pNew->sHeader.fProcess = MNG_NULL;
#endif
  pNew->iId              = iId;
  pNew->bFrozen          = MNG_FALSE;
  pNew->bVisible         = bVisible;
  pNew->bViewable        = pSource->bViewable;
  pNew->bValid           = MNG_TRUE;

  if (bHasloca)                        /* location info available ? */
  {
    if (iLocationtype == 0)            /* absolute position ? */
    {
      pNew->iPosx        = iLocationx;
      pNew->iPosy        = iLocationy;
    }
    else                               /* relative */
    {
      pNew->iPosx        = pSource->iPosx + iLocationx;
      pNew->iPosy        = pSource->iPosy + iLocationy;
    }
  }
  else                                 /* copy from source */
  {
    pNew->iPosx          = pSource->iPosx;
    pNew->iPosy          = pSource->iPosy;
  }
                                       /* copy clipping info */
  pNew->bClipped         = pSource->bClipped;
  pNew->iClipl           = pSource->iClipl;
  pNew->iClipr           = pSource->iClipr;
  pNew->iClipt           = pSource->iClipt;
  pNew->iClipb           = pSource->iClipb;
#ifndef MNG_SKIPCHUNK_MAGN
                                       /* copy magnification info */
/*  pNew->iMAGN_MethodX    = pSource->iMAGN_MethodX;     LET'S NOT !!!!!!
  pNew->iMAGN_MethodY    = pSource->iMAGN_MethodY;
  pNew->iMAGN_MX         = pSource->iMAGN_MX;
  pNew->iMAGN_MY         = pSource->iMAGN_MY;
  pNew->iMAGN_ML         = pSource->iMAGN_ML;
  pNew->iMAGN_MR         = pSource->iMAGN_MR;
  pNew->iMAGN_MT         = pSource->iMAGN_MT;
  pNew->iMAGN_MB         = pSource->iMAGN_MB; */
#endif

#ifndef MNG_SKIPCHUNK_PAST
  pNew->iPastx           = 0;          /* initialize PAST info */
  pNew->iPasty           = 0;
#endif

  if (iId)                             /* not for object 0 */
  {                                    /* find previous lower object-id */
    pPrev = (mng_imagep)pData->pLastimgobj;
    while ((pPrev) && (pPrev->iId > iId))
      pPrev = (mng_imagep)pPrev->sHeader.pPrev;

    if (pPrev)                         /* found it ? */
    {
      pNew->sHeader.pPrev  = pPrev;    /* than link it in place */
      pNew->sHeader.pNext  = pPrev->sHeader.pNext;
      pPrev->sHeader.pNext = pNew;
    }
    else                               /* if not found, it becomes the first ! */
    {
      pNew->sHeader.pNext  = pData->pFirstimgobj;
      pData->pFirstimgobj  = pNew;
    }

    pNext                  = (mng_imagep)pNew->sHeader.pNext;

    if (pNext)
      pNext->sHeader.pPrev = pNew;
    else
      pData->pLastimgobj   = pNew;

  }

  if (bPartial)                        /* partial clone ? */
  {
    pNew->pImgbuf = pSource->pImgbuf;  /* use the same object buffer */
    pNew->pImgbuf->iRefcount++;        /* and increase the reference count */
  }
  else                                 /* create a full clone ! */
  {
    mng_bool bConcrete = MNG_FALSE;    /* it's abstract by default (?) */

    if (!bAbstract)                    /* determine concreteness from source ? */
      bConcrete = pSource->pImgbuf->bConcrete;
                                       /* create a full clone ! */
    iRetcode = mng_clone_imagedataobject (pData, bConcrete, pSource->pImgbuf, &pImgbuf);

    if (iRetcode)                      /* on error bail out */
    {
      MNG_FREEX (pData, pNew, sizeof (mng_image));
      return iRetcode;
    }

    pNew->pImgbuf = pImgbuf;           /* and remember it */
  }

  *ppClone = pNew;                     /* return it */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CLONE_IMGOBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_renum_imageobject (mng_datap  pData,
                                   mng_imagep pSource,
                                   mng_uint16 iId,
                                   mng_bool   bVisible,
                                   mng_bool   bAbstract,
                                   mng_bool   bHasloca,
                                   mng_uint8  iLocationtype,
                                   mng_int32  iLocationx,
                                   mng_int32  iLocationy)
{
  mng_imagep pPrev, pNext;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_RENUM_IMGOBJECT, MNG_LC_START);
#endif

  pSource->bVisible  = bVisible;       /* store the new visibility */

  if (bHasloca)                        /* location info available ? */
  {
    if (iLocationtype == 0)            /* absolute position ? */
    {
      pSource->iPosx = iLocationx;
      pSource->iPosy = iLocationy;
    }
    else                               /* relative */
    {
      pSource->iPosx = pSource->iPosx + iLocationx;
      pSource->iPosy = pSource->iPosy + iLocationy;
    }
  }

  if (iId)                             /* not for object 0 */
  {                                    /* find previous lower object-id */
    pPrev = (mng_imagep)pData->pLastimgobj;
    while ((pPrev) && (pPrev->iId > iId))
      pPrev = (mng_imagep)pPrev->sHeader.pPrev;
                                       /* different from current ? */
    if (pPrev != (mng_imagep)pSource->sHeader.pPrev)
    {
      if (pSource->sHeader.pPrev)      /* unlink from current position !! */
        ((mng_imagep)pSource->sHeader.pPrev)->sHeader.pNext = pSource->sHeader.pNext;
      else
        pData->pFirstimgobj                                 = pSource->sHeader.pNext;

      if (pSource->sHeader.pNext)
        ((mng_imagep)pSource->sHeader.pNext)->sHeader.pPrev = pSource->sHeader.pPrev;
      else
        pData->pLastimgobj                                  = pSource->sHeader.pPrev;

      if (pPrev)                       /* found the previous ? */
      {                                /* than link it in place */
        pSource->sHeader.pPrev = pPrev;
        pSource->sHeader.pNext = pPrev->sHeader.pNext;
        pPrev->sHeader.pNext   = pSource;
      }
      else                             /* if not found, it becomes the first ! */
      {
        pSource->sHeader.pNext = pData->pFirstimgobj;
        pData->pFirstimgobj    = pSource;
      }

      pNext                    = (mng_imagep)pSource->sHeader.pNext;

      if (pNext)
        pNext->sHeader.pPrev   = pSource;
      else
        pData->pLastimgobj     = pSource;

    }
  }

  pSource->iId = iId;                  /* now set the new id! */

  if (bAbstract)                       /* force it to abstract ? */
    pSource->pImgbuf->bConcrete = MNG_FALSE;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_RENUM_IMGOBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_reset_object_details (mng_datap  pData,
                                      mng_imagep pImage,
                                      mng_uint32 iWidth,
                                      mng_uint32 iHeight,
                                      mng_uint8  iBitdepth,
                                      mng_uint8  iColortype,
                                      mng_uint8  iCompression,
                                      mng_uint8  iFilter,
                                      mng_uint8  iInterlace,
                                      mng_bool   bResetall)
{
  mng_imagedatap pBuf  = pImage->pImgbuf;
  mng_uint32     iSamplesize = 0;
  mng_uint32     iRowsize;
  mng_uint32     iImgdatasize;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_RESET_OBJECTDETAILS, MNG_LC_START);
#endif

  pBuf->iWidth         = iWidth;       /* set buffer characteristics */
  pBuf->iHeight        = iHeight;
  pBuf->iBitdepth      = iBitdepth;
  pBuf->iColortype     = iColortype;
  pBuf->iCompression   = iCompression;
  pBuf->iFilter        = iFilter;
  pBuf->iInterlace     = iInterlace;
  pBuf->bCorrected     = MNG_FALSE;
  pBuf->iAlphabitdepth = 0;
                                       /* determine samplesize from color_type/bit_depth */
  switch (iColortype)                  /* for < 8-bit samples we just reserve 8 bits */
  {
    case  0  : ;                       /* gray */
    case  8  : {                       /* JPEG gray */
#ifndef MNG_NO_16BIT_SUPPORT
                 if (iBitdepth > 8)
                   iSamplesize = 2;
                 else
#endif
                   iSamplesize = 1;

                 break;
               }
    case  2  : ;                       /* rgb */
    case 10  : {                       /* JPEG rgb */
#ifndef MNG_NO_16BIT_SUPPORT
                 if (iBitdepth > 8)
                   iSamplesize = 6;
                 else
#endif
                   iSamplesize = 3;

                 break;
               }
    case  3  : {                       /* indexed */
                 iSamplesize = 1;
                 break;
               }
    case  4  : ;                       /* gray+alpha */
    case 12  : {                       /* JPEG gray+alpha */
#ifndef MNG_NO_16BIT_SUPPORT
                 if (iBitdepth > 8)
                   iSamplesize = 4;
                 else
#endif
                   iSamplesize = 2;

                 break;
               }
    case  6  : ;                       /* rgb+alpha */
    case 14  : {                       /* JPEG rgb+alpha */
#ifndef MNG_NO_16BIT_SUPPORT
                 if (iBitdepth > 8)
                   iSamplesize = 8;
                 else
#endif
                   iSamplesize = 4;

                 break;
               }
  }

  iRowsize     = iSamplesize * iWidth;
  iImgdatasize = iRowsize    * iHeight;
                                       /* buffer size changed ? */
  if (iImgdatasize != pBuf->iImgdatasize)
  {                                    /* drop the old one */
    MNG_FREE (pData, pBuf->pImgdata, pBuf->iImgdatasize);

    if (iImgdatasize)                  /* allocate new sample-buffer ? */
      MNG_ALLOC (pData, pBuf->pImgdata, iImgdatasize);
  }
  else
  {
    if (iImgdatasize)                  /* clear old buffer */
    {
      mng_uint8p pTemp = pBuf->pImgdata;
      mng_uint32 iX;
      
      for (iX = 0; iX < (iImgdatasize & (mng_uint32)(~3L)); iX += 4)
      {
        *((mng_uint32p)pTemp) = 0x00000000l;
        pTemp += 4;
      }

      while (pTemp < (pBuf->pImgdata + iImgdatasize))
      {
        *pTemp = 0;
        pTemp++;
      }
    }
  }

  pBuf->iSamplesize  = iSamplesize;    /* remember new sizes */
  pBuf->iRowsize     = iRowsize;
  pBuf->iImgdatasize = iImgdatasize;

  if (!pBuf->iPixelsampledepth)        /* set delta sampledepths if empty */
    pBuf->iPixelsampledepth = iBitdepth;
  if (!pBuf->iAlphasampledepth)
    pBuf->iAlphasampledepth = iBitdepth;
                                       /* dimension set and clipping not ? */
  if ((iWidth) && (iHeight) && (!pImage->bClipped))
  {
    pImage->iClipl   = 0;              /* set clipping to dimension by default */
    pImage->iClipr   = iWidth;
    pImage->iClipt   = 0;
    pImage->iClipb   = iHeight;
  }

#ifndef MNG_SKIPCHUNK_MAGN
  if (pImage->iId)                     /* reset magnification info ? */
  {
    pImage->iMAGN_MethodX = 0;
    pImage->iMAGN_MethodY = 0;
    pImage->iMAGN_MX      = 0;
    pImage->iMAGN_MY      = 0;
    pImage->iMAGN_ML      = 0;
    pImage->iMAGN_MR      = 0;
    pImage->iMAGN_MT      = 0;
    pImage->iMAGN_MB      = 0;
  }
#endif

  if (bResetall)                       /* reset the other characteristics ? */
  {
#ifndef MNG_SKIPCHUNK_PAST
    pImage->iPastx = 0;
    pImage->iPasty = 0;
#endif

    pBuf->bHasPLTE = MNG_FALSE;
    pBuf->bHasTRNS = MNG_FALSE;
    pBuf->bHasGAMA = pData->bHasglobalGAMA;
#ifndef MNG_SKIPCHUNK_cHRM
    pBuf->bHasCHRM = pData->bHasglobalCHRM;
#endif
    pBuf->bHasSRGB = pData->bHasglobalSRGB;
#ifndef MNG_SKIPCHUNK_iCCP
    pBuf->bHasICCP = pData->bHasglobalICCP;
#endif
#ifndef MNG_SKIPCHUNK_bKGD
    pBuf->bHasBKGD = pData->bHasglobalBKGD;
#endif

#ifndef MNG_SKIPCHUNK_iCCP
    if (pBuf->iProfilesize)            /* drop possibly old ICC profile */
    {
      MNG_FREE (pData, pBuf->pProfile, pBuf->iProfilesize);
      pBuf->iProfilesize     = 0;
    }  
#endif

    if (pData->bHasglobalGAMA)         /* global gAMA present ? */
      pBuf->iGamma           = pData->iGlobalGamma;

#ifndef MNG_SKIPCHUNK_cHRM
    if (pData->bHasglobalCHRM)         /* global cHRM present ? */
    {
      pBuf->iWhitepointx     = pData->iGlobalWhitepointx;
      pBuf->iWhitepointy     = pData->iGlobalWhitepointy;
      pBuf->iPrimaryredx     = pData->iGlobalPrimaryredx;
      pBuf->iPrimaryredy     = pData->iGlobalPrimaryredy;
      pBuf->iPrimarygreenx   = pData->iGlobalPrimarygreenx;
      pBuf->iPrimarygreeny   = pData->iGlobalPrimarygreeny;
      pBuf->iPrimarybluex    = pData->iGlobalPrimarybluex;
      pBuf->iPrimarybluey    = pData->iGlobalPrimarybluey;
    }
#endif

    if (pData->bHasglobalSRGB)           /* global sRGB present ? */
      pBuf->iRenderingintent = pData->iGlobalRendintent;

#ifndef MNG_SKIPCHUNK_iCCP
    if (pData->bHasglobalICCP)           /* global iCCP present ? */
    {
      if (pData->iGlobalProfilesize)
      {
        MNG_ALLOC (pData, pBuf->pProfile, pData->iGlobalProfilesize);
        MNG_COPY  (pBuf->pProfile, pData->pGlobalProfile, pData->iGlobalProfilesize);
      }

      pBuf->iProfilesize     = pData->iGlobalProfilesize;
    }
#endif

#ifndef MNG_SKIPCHUNK_bKGD
    if (pData->bHasglobalBKGD)           /* global bKGD present ? */
    {
      pBuf->iBKGDred         = pData->iGlobalBKGDred;
      pBuf->iBKGDgreen       = pData->iGlobalBKGDgreen;
      pBuf->iBKGDblue        = pData->iGlobalBKGDblue;
    }
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_RESET_OBJECTDETAILS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#if !defined(MNG_NO_DELTA_PNG) || !defined(MNG_SKIPCHUNK_PAST) || !defined(MNG_SKIPCHUNK_MAGN)
mng_retcode mng_promote_imageobject (mng_datap  pData,
                                     mng_imagep pImage,
                                     mng_uint8  iBitdepth,
                                     mng_uint8  iColortype,
                                     mng_uint8  iFilltype)
{
  mng_retcode    iRetcode       = MNG_NOERROR;
  mng_imagedatap pBuf           = pImage->pImgbuf;
  mng_uint32     iW             = pBuf->iWidth;
  mng_uint32     iH             = pBuf->iHeight;
  mng_uint8p     pNewbuf;
  mng_uint32     iNewbufsize;
  mng_uint32     iNewrowsize;
  mng_uint32     iNewsamplesize = pBuf->iSamplesize;
  mng_uint32     iY;
  mng_uint8      iTempdepth;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROMOTE_IMGOBJECT, MNG_LC_START);
#endif

#ifdef MNG_NO_1_2_4BIT_SUPPORT
  if (iBitdepth < 8)
    iBitdepth=8;
  if (pBuf->iBitdepth < 8)
    pBuf->iBitdepth=8;
#endif
#ifdef MNG_NO_16BIT_SUPPORT
  if (iBitdepth > 8)
    iBitdepth=8;
  if (pBuf->iBitdepth > 8)
    pBuf->iBitdepth=8;
#endif

  pData->fPromoterow    = MNG_NULL;    /* init promotion fields */
  pData->fPromBitdepth  = MNG_NULL;
  pData->iPromColortype = iColortype;
  pData->iPromBitdepth  = iBitdepth;
  pData->iPromFilltype  = iFilltype;

  if (iBitdepth != pBuf->iBitdepth)    /* determine bitdepth promotion */
  {
    if (pBuf->iColortype == MNG_COLORTYPE_INDEXED)
      iTempdepth = 8;
    else
      iTempdepth = pBuf->iBitdepth;

#ifndef MNG_NO_DELTA_PNG
    if (iFilltype == MNG_FILLMETHOD_ZEROFILL)
    {
      switch (iTempdepth)
      {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
        case 1 : {
                   switch (iBitdepth)
                   {
                     case  2 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_1_2;  break; }
                     case  4 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_1_4;  break; }
                     case  8 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_1_8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                     case 16 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_1_16; break; }
#endif
                   }
                   break;
                 }
        case 2 : {
                   switch (iBitdepth)
                   {
                     case  4 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_2_4;  break; }
                     case  8 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_2_8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                     case 16 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_2_16; break; }
#endif
                   }
                   break;
                 }
        case 4 : {
                   switch (iBitdepth)
                   {
                     case  8 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_4_8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                     case 16 : { pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_4_16; break; }
#endif
                   }
                   break;
                 }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
        case 8 : {
#ifndef MNG_NO_16BIT_SUPPORT
                   if (iBitdepth == 16)
                     pData->fPromBitdepth = (mng_fptr)mng_promote_zerofill_8_16;
#endif
                   break;
                 }
      }
    }
    else
#endif
    {
      switch (iTempdepth)
      {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
        case 1 : {
                   switch (iBitdepth)
                   {
                     case  2 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_1_2;  break; }
                     case  4 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_1_4;  break; }
                     case  8 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_1_8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                     case 16 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_1_16; break; }
#endif
                   }
                   break;
                 }
        case 2 : {
                   switch (iBitdepth)
                   {
                     case  4 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_2_4;  break; }
                     case  8 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_2_8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                     case 16 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_2_16; break; }
#endif
                   }
                   break;
                 }
        case 4 : {
                   switch (iBitdepth)
                   {
                     case  8 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_4_8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                     case 16 : { pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_4_16; break; }
#endif
                   }
                   break;
                 }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
        case 8 : {
#ifndef MNG_NO_16BIT_SUPPORT
                   if (iBitdepth == 16)
                     pData->fPromBitdepth = (mng_fptr)mng_promote_replicate_8_16;
#endif
                   break;
                 }
      }
    }
  }
                                       /* g -> g */
  if ((pBuf->iColortype == MNG_COLORTYPE_GRAY) &&
      (iColortype == MNG_COLORTYPE_GRAY))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_g8_g16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_g8_g8;
    }

    iNewsamplesize = 1;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 2;
#endif
  }
  else                                 /* g -> ga */
  if ((pBuf->iColortype == MNG_COLORTYPE_GRAY) &&
      (iColortype == MNG_COLORTYPE_GRAYA))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_g8_ga16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_g8_ga8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_g16_ga16;
#endif

    iNewsamplesize = 2;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 4;
#endif
  }
  else                                 /* g -> rgb */
  if ((pBuf->iColortype == MNG_COLORTYPE_GRAY) &&
      (iColortype == MNG_COLORTYPE_RGB))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_g8_rgb16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_g8_rgb8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_g16_rgb16;
#endif

    iNewsamplesize = 3;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 6;
#endif
  }
  else                                 /* g -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_GRAY) &&
      (iColortype == MNG_COLORTYPE_RGBA))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_g8_rgba16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_g8_rgba8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_g16_rgba16;
#endif

    iNewsamplesize = 4;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 8;
#endif
  }
  else                                 /* ga -> ga */
  if ((pBuf->iColortype == MNG_COLORTYPE_GRAYA) &&
      (iColortype == MNG_COLORTYPE_GRAYA))
  {
    iNewsamplesize = 2;
#ifndef MNG_NO_16BIT_SUPPORT
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_ga8_ga16;
    if (iBitdepth == 16)
      iNewsamplesize = 4;
#endif
  }
  else                                 /* ga -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_GRAYA) &&
      (iColortype == MNG_COLORTYPE_RGBA))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_ga8_rgba16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_ga8_rgba8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_ga16_rgba16;
#endif

    iNewsamplesize = 4;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 8;
#endif
  }
  else                                 /* rgb -> rgb */
  if ((pBuf->iColortype == MNG_COLORTYPE_RGB) &&
      (iColortype == MNG_COLORTYPE_RGB))
  {
    iNewsamplesize = 3;
#ifndef MNG_NO_16BIT_SUPPORT
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_rgb8_rgb16;
    if (iBitdepth == 16)
      iNewsamplesize = 6;
#endif
  }
  else                                 /* rgb -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_RGB) &&
      (iColortype == MNG_COLORTYPE_RGBA))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_rgb8_rgba16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_rgb8_rgba8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_rgb16_rgba16;
#endif

    iNewsamplesize = 4;
#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 8;
#endif
  }
  else                                 /* indexed -> rgb */
  if ((pBuf->iColortype == MNG_COLORTYPE_INDEXED) &&
      (iColortype == MNG_COLORTYPE_RGB))
  {
#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)
      pData->fPromoterow = (mng_fptr)mng_promote_idx8_rgb16;
    else
#endif
      pData->fPromoterow = (mng_fptr)mng_promote_idx8_rgb8;

    iNewsamplesize = 3;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 6;
#endif
  }
  else                                 /* indexed -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_INDEXED) &&
      (iColortype == MNG_COLORTYPE_RGBA))
  {
#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)
      pData->fPromoterow = (mng_fptr)mng_promote_idx8_rgba16;
    else
#endif
      pData->fPromoterow = (mng_fptr)mng_promote_idx8_rgba8;

    iNewsamplesize = 4;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 8;
#endif
  }
  else                                 /* rgba -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_RGBA) &&
      (iColortype == MNG_COLORTYPE_RGBA))
  {
    iNewsamplesize = 4;
#ifndef MNG_NO_16BIT_SUPPORT
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_rgba8_rgba16;
    }
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 8;
#endif
  }
#ifdef MNG_INCLUDE_JNG
  else                                 /* JPEG g -> g */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGGRAY) &&
      (iColortype == MNG_COLORTYPE_JPEGGRAY))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_g8_g16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_g8_g8;
    }

    iNewsamplesize = 1;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 2;
#endif
  }
  else                                 /* JPEG g -> ga */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGGRAY) &&
      (iColortype == MNG_COLORTYPE_JPEGGRAYA))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_g8_ga16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_g8_ga8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_g16_ga16;
#endif

    iNewsamplesize = 2;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 4;
#endif
  }
  else                                 /* JPEG g -> rgb */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGGRAY) &&
      (iColortype == MNG_COLORTYPE_JPEGCOLOR))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_g8_rgb16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_g8_rgb8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_g16_rgb16;
#endif

    iNewsamplesize = 3;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 6;
#endif
  }
  else                                 /* JPEG g -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGGRAY) &&
      (iColortype == MNG_COLORTYPE_JPEGCOLORA))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_g8_rgba16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_g8_rgba8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_g16_rgba16;
#endif

    iNewsamplesize = 4;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 8;
#endif
  }
  else                                 /* JPEG ga -> ga */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGGRAYA) &&
      (iColortype == MNG_COLORTYPE_JPEGGRAYA))
  {
    iNewsamplesize = 2;
#ifndef MNG_NO_16BIT_SUPPORT
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_ga8_ga16;
    if (iBitdepth == 16)
      iNewsamplesize = 4;
#endif

  }
  else                                 /* JPEG ga -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGGRAYA) &&
      (iColortype == MNG_COLORTYPE_JPEGCOLORA))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_ga8_rgba16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_ga8_rgba8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_ga16_rgba16;
#endif

    iNewsamplesize = 4;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 8;
#endif
  }
  else                                 /* JPEG rgb -> rgb */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGCOLOR) &&
      (iColortype == MNG_COLORTYPE_JPEGCOLOR))
  {
    iNewsamplesize = 3;
#ifndef MNG_NO_16BIT_SUPPORT
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_rgb8_rgb16;
    if (iBitdepth == 16)
      iNewsamplesize = 6;
#endif

  }
  else                                 /* JPEG rgb -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGCOLOR) &&
      (iColortype == MNG_COLORTYPE_JPEGCOLORA))
  {
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
    {
#ifndef MNG_NO_16BIT_SUPPORT
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_rgb8_rgba16;
      else
#endif
        pData->fPromoterow = (mng_fptr)mng_promote_rgb8_rgba8;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    else                               /* source = 16 bits */
      pData->fPromoterow = (mng_fptr)mng_promote_rgb16_rgba16;
#endif

    iNewsamplesize = 4;

#ifndef MNG_NO_16BIT_SUPPORT
    if (iBitdepth == 16)               /* 16-bit wide ? */
      iNewsamplesize = 8;
#endif
  }
  else                                 /* JPEG rgba -> rgba */
  if ((pBuf->iColortype == MNG_COLORTYPE_JPEGCOLORA) &&
      (iColortype == MNG_COLORTYPE_JPEGCOLORA))
  {
    iNewsamplesize = 4;
#ifndef MNG_NO_16BIT_SUPPORT
    if (pBuf->iBitdepth <= 8)          /* source <= 8 bits */
      if (iBitdepth == 16)
        pData->fPromoterow = (mng_fptr)mng_promote_rgba8_rgba16;
    if (iBitdepth == 16)
      iNewsamplesize = 8;
#endif
  }
#endif /* JNG */

  /* found a proper promotion ? */
  if (pData->fPromoterow)
  {
    pData->pPromBuf    = (mng_ptr)pBuf;
    pData->iPromWidth  = pBuf->iWidth;
    iNewrowsize        = iW * iNewsamplesize;
    iNewbufsize        = iH * iNewrowsize;

    MNG_ALLOC (pData, pNewbuf, iNewbufsize);

    pData->pPromSrc    = (mng_ptr)pBuf->pImgdata;
    pData->pPromDst    = (mng_ptr)pNewbuf;
    iY                 = 0;

    while ((!iRetcode) && (iY < iH))
    {
      iRetcode         = ((mng_promoterow)pData->fPromoterow) (pData);
      pData->pPromSrc  = (mng_uint8p)pData->pPromSrc + pBuf->iRowsize;
      pData->pPromDst  = (mng_uint8p)pData->pPromDst + iNewrowsize;
/*      pData->pPromSrc  = (mng_ptr)((mng_uint32)pData->pPromSrc + pBuf->iRowsize); */
/*      pData->pPromDst  = (mng_ptr)((mng_uint32)pData->pPromDst + iNewrowsize); */
      iY++;
    }

    MNG_FREEX (pData, pBuf->pImgdata, pBuf->iImgdatasize);

    pBuf->iBitdepth    = iBitdepth;
    pBuf->iColortype   = iColortype;
    pBuf->iSamplesize  = iNewsamplesize;
    pBuf->iRowsize     = iNewrowsize;
    pBuf->iImgdatasize = iNewbufsize;
    pBuf->pImgdata     = pNewbuf;
    pBuf->bHasPLTE     = MNG_FALSE;
    pBuf->iPLTEcount   = 0;
    pBuf->bHasTRNS     = MNG_FALSE;
    pBuf->iTRNScount   = 0;

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROMOTE_IMGOBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
mng_retcode mng_magnify_imageobject (mng_datap  pData,
                                     mng_imagep pImage)
{
  mng_uint8p     pNewdata;
  mng_uint8p     pSrcline1;
  mng_uint8p     pSrcline2;
  mng_uint8p     pTempline;
  mng_uint8p     pDstline;
  mng_uint32     iNewrowsize;
  mng_uint32     iNewsize;
  mng_uint32     iY;
  mng_int32      iS, iM;
  mng_retcode    iRetcode;

  mng_imagedatap pBuf      = pImage->pImgbuf;
  mng_uint32     iNewW     = pBuf->iWidth;
  mng_uint32     iNewH     = pBuf->iHeight;
  mng_magnify_x  fMagnifyX = MNG_NULL;
  mng_magnify_y  fMagnifyY = MNG_NULL;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_MAGNIFY_IMGOBJECT, MNG_LC_START);
#endif

  if (pBuf->iColortype == MNG_COLORTYPE_INDEXED)           /* indexed color ? */
  {                                    /* concrete buffer ? */
    if ((pBuf->bConcrete) && (pImage->iId))
      MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);

#ifndef MNG_OPTIMIZE_FOOTPRINT_MAGN
    if (pBuf->iTRNScount)              /* with transparency ? */
      iRetcode = mng_promote_imageobject (pData, pImage, 8, 6, 0);
    else
      iRetcode = mng_promote_imageobject (pData, pImage, 8, 2, 0);

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
#endif
  }

#ifdef MNG_OPTIMIZE_FOOTPRINT_MAGN
  /* Promote everything to RGBA, using fill method 0 (LBR) */
  iRetcode = mng_promote_imageobject (pData, pImage, 8, 6, 0);           
  if (iRetcode)                      /* on error bail out */
    return iRetcode;
#endif

  if (pImage->iMAGN_MethodX)           /* determine new width */
  {
    if (pImage->iMAGN_MethodX == 1)
    {
      iNewW   = pImage->iMAGN_ML;
      if (pBuf->iWidth  > 1)
        iNewW = iNewW + pImage->iMAGN_MR;
      if (pBuf->iWidth  > 2)
        iNewW = iNewW + (pBuf->iWidth  - 2) * (pImage->iMAGN_MX);
    }
    else
    {
      iNewW   = pBuf->iWidth  + pImage->iMAGN_ML - 1;
      if (pBuf->iWidth  > 2)
        iNewW = iNewW + pImage->iMAGN_MR - 1;
      if (pBuf->iWidth  > 3)
        iNewW = iNewW + (pBuf->iWidth  - 3) * (pImage->iMAGN_MX - 1);
    }
  }

  if (pImage->iMAGN_MethodY)           /* determine new height */
  {
    if (pImage->iMAGN_MethodY == 1)
    {
      iNewH   = pImage->iMAGN_MT;
      if (pBuf->iHeight > 1)
        iNewH = iNewH + pImage->iMAGN_ML;
      if (pBuf->iHeight > 2)
        iNewH = iNewH + (pBuf->iHeight - 2) * (pImage->iMAGN_MY);
    }
    else
    {
      iNewH   = pBuf->iHeight + pImage->iMAGN_MT - 1;
      if (pBuf->iHeight > 2)
        iNewH = iNewH + pImage->iMAGN_MB - 1;
      if (pBuf->iHeight > 3)
        iNewH = iNewH + (pBuf->iHeight - 3) * (pImage->iMAGN_MY - 1);
    }
  }
                                       /* get new buffer */
  iNewrowsize  = iNewW * pBuf->iSamplesize;
  iNewsize     = iNewH * iNewrowsize;

  MNG_ALLOC (pData, pNewdata, iNewsize);

  switch (pBuf->iColortype)            /* determine magnification routines */
  {
#ifndef MNG_OPTIMIZE_FOOTPRINT_MAGN
    case  0 : ;
    case  8 : {
                if (pBuf->iBitdepth <= 8)
                {
                  switch (pImage->iMAGN_MethodX)
                  {
                    case 1  : { fMagnifyX = mng_magnify_g8_x1; break; }
                    case 2  : { fMagnifyX = mng_magnify_g8_x2; break; }
                    case 3  : { fMagnifyX = mng_magnify_g8_x3; break; }
                    case 4  : { fMagnifyX = mng_magnify_g8_x2; break; }
                    case 5  : { fMagnifyX = mng_magnify_g8_x3; break; }
                  }

                  switch (pImage->iMAGN_MethodY)
                  {
                    case 1  : { fMagnifyY = mng_magnify_g8_y1; break; }
                    case 2  : { fMagnifyY = mng_magnify_g8_y2; break; }
                    case 3  : { fMagnifyY = mng_magnify_g8_y3; break; }
                    case 4  : { fMagnifyY = mng_magnify_g8_y2; break; }
                    case 5  : { fMagnifyY = mng_magnify_g8_y3; break; }
                  }
                }
#ifndef MNG_NO_16BIT_SUPPORT
                else
                {
                  switch (pImage->iMAGN_MethodX)
                  {
                    case 1  : { fMagnifyX = mng_magnify_g16_x1; break; }
                    case 2  : { fMagnifyX = mng_magnify_g16_x2; break; }
                    case 3  : { fMagnifyX = mng_magnify_g16_x3; break; }
                    case 4  : { fMagnifyX = mng_magnify_g16_x2; break; }
                    case 5  : { fMagnifyX = mng_magnify_g16_x3; break; }
                  }

                  switch (pImage->iMAGN_MethodY)
                  {
                    case 1  : { fMagnifyY = mng_magnify_g16_y1; break; }
                    case 2  : { fMagnifyY = mng_magnify_g16_y2; break; }
                    case 3  : { fMagnifyY = mng_magnify_g16_y3; break; }
                    case 4  : { fMagnifyY = mng_magnify_g16_y2; break; }
                    case 5  : { fMagnifyY = mng_magnify_g16_y3; break; }
                  }
                }
#endif

                break;
              }

    case  2 : ;
    case 10 : {
                if (pBuf->iBitdepth <= 8)
                {
                  switch (pImage->iMAGN_MethodX)
                  {
                    case 1  : { fMagnifyX = mng_magnify_rgb8_x1; break; }
                    case 2  : { fMagnifyX = mng_magnify_rgb8_x2; break; }
                    case 3  : { fMagnifyX = mng_magnify_rgb8_x3; break; }
                    case 4  : { fMagnifyX = mng_magnify_rgb8_x2; break; }
                    case 5  : { fMagnifyX = mng_magnify_rgb8_x3; break; }
                  }

                  switch (pImage->iMAGN_MethodY)
                  {
                    case 1  : { fMagnifyY = mng_magnify_rgb8_y1; break; }
                    case 2  : { fMagnifyY = mng_magnify_rgb8_y2; break; }
                    case 3  : { fMagnifyY = mng_magnify_rgb8_y3; break; }
                    case 4  : { fMagnifyY = mng_magnify_rgb8_y2; break; }
                    case 5  : { fMagnifyY = mng_magnify_rgb8_y3; break; }
                  }
                }
#ifndef MNG_NO_16BIT_SUPPORT
                else
                {
                  switch (pImage->iMAGN_MethodX)
                  {
                    case 1  : { fMagnifyX = mng_magnify_rgb16_x1; break; }
                    case 2  : { fMagnifyX = mng_magnify_rgb16_x2; break; }
                    case 3  : { fMagnifyX = mng_magnify_rgb16_x3; break; }
                    case 4  : { fMagnifyX = mng_magnify_rgb16_x2; break; }
                    case 5  : { fMagnifyX = mng_magnify_rgb16_x3; break; }
                  }

                  switch (pImage->iMAGN_MethodY)
                  {
                    case 1  : { fMagnifyY = mng_magnify_rgb16_y1; break; }
                    case 2  : { fMagnifyY = mng_magnify_rgb16_y2; break; }
                    case 3  : { fMagnifyY = mng_magnify_rgb16_y3; break; }
                    case 4  : { fMagnifyY = mng_magnify_rgb16_y2; break; }
                    case 5  : { fMagnifyY = mng_magnify_rgb16_y3; break; }
                  }
                }
#endif

                break;
              }

    case  4 : ;
    case 12 : {
                if (pBuf->iBitdepth <= 8)
                {
                  switch (pImage->iMAGN_MethodX)
                  {
                    case 1  : { fMagnifyX = mng_magnify_ga8_x1; break; }
                    case 2  : { fMagnifyX = mng_magnify_ga8_x2; break; }
                    case 3  : { fMagnifyX = mng_magnify_ga8_x3; break; }
                    case 4  : { fMagnifyX = mng_magnify_ga8_x4; break; }
                    case 5  : { fMagnifyX = mng_magnify_ga8_x5; break; }
                  }

                  switch (pImage->iMAGN_MethodY)
                  {
                    case 1  : { fMagnifyY = mng_magnify_ga8_y1; break; }
                    case 2  : { fMagnifyY = mng_magnify_ga8_y2; break; }
                    case 3  : { fMagnifyY = mng_magnify_ga8_y3; break; }
                    case 4  : { fMagnifyY = mng_magnify_ga8_y4; break; }
                    case 5  : { fMagnifyY = mng_magnify_ga8_y5; break; }
                  }
                }
#ifndef MNG_NO_16BIT_SUPPORT
                else
                {
                  switch (pImage->iMAGN_MethodX)
                  {
                    case 1  : { fMagnifyX = mng_magnify_ga16_x1; break; }
                    case 2  : { fMagnifyX = mng_magnify_ga16_x2; break; }
                    case 3  : { fMagnifyX = mng_magnify_ga16_x3; break; }
                    case 4  : { fMagnifyX = mng_magnify_ga16_x4; break; }
                    case 5  : { fMagnifyX = mng_magnify_ga16_x5; break; }
                  }

                  switch (pImage->iMAGN_MethodY)
                  {
                    case 1  : { fMagnifyY = mng_magnify_ga16_y1; break; }
                    case 2  : { fMagnifyY = mng_magnify_ga16_y2; break; }
                    case 3  : { fMagnifyY = mng_magnify_ga16_y3; break; }
                    case 4  : { fMagnifyY = mng_magnify_ga16_y4; break; }
                    case 5  : { fMagnifyY = mng_magnify_ga16_y5; break; }
                  }
                }
#endif

                break;
              }
#endif

    case  6 : ;
    case 14 : {
                if (pBuf->iBitdepth <= 8)
                {
                  switch (pImage->iMAGN_MethodX)
                  {
                    case 1  : { fMagnifyX = mng_magnify_rgba8_x1; break; }
                    case 2  : { fMagnifyX = mng_magnify_rgba8_x2; break; }
                    case 3  : { fMagnifyX = mng_magnify_rgba8_x3; break; }
                    case 4  : { fMagnifyX = mng_magnify_rgba8_x4; break; }
                    case 5  : { fMagnifyX = mng_magnify_rgba8_x5; break; }
                  }

                  switch (pImage->iMAGN_MethodY)
                  {
                    case 1  : { fMagnifyY = mng_magnify_rgba8_y1; break; }
                    case 2  : { fMagnifyY = mng_magnify_rgba8_y2; break; }
                    case 3  : { fMagnifyY = mng_magnify_rgba8_y3; break; }
                    case 4  : { fMagnifyY = mng_magnify_rgba8_y4; break; }
                    case 5  : { fMagnifyY = mng_magnify_rgba8_y5; break; }
                  }
                }
#ifndef MNG_NO_16BIT_SUPPORT
#ifndef MNG_OPTIMIZE_FOOTPRINT_MAGN
                else
                {
                  switch (pImage->iMAGN_MethodX)
                  {
                    case 1  : { fMagnifyX = mng_magnify_rgba16_x1; break; }
                    case 2  : { fMagnifyX = mng_magnify_rgba16_x2; break; }
                    case 3  : { fMagnifyX = mng_magnify_rgba16_x3; break; }
                    case 4  : { fMagnifyX = mng_magnify_rgba16_x4; break; }
                    case 5  : { fMagnifyX = mng_magnify_rgba16_x5; break; }
                  }

                  switch (pImage->iMAGN_MethodY)
                  {
                    case 1  : { fMagnifyY = mng_magnify_rgba16_y1; break; }
                    case 2  : { fMagnifyY = mng_magnify_rgba16_y2; break; }
                    case 3  : { fMagnifyY = mng_magnify_rgba16_y3; break; }
                    case 4  : { fMagnifyY = mng_magnify_rgba16_y4; break; }
                    case 5  : { fMagnifyY = mng_magnify_rgba16_y5; break; }
                  }
                }
#endif
#endif
                break;
              }
  }

  pSrcline1 = pBuf->pImgdata;          /* initialize row-loop variables */
  pDstline  = pNewdata;
                                       /* allocate temporary row */
  MNG_ALLOC (pData, pTempline, iNewrowsize);

  for (iY = 0; iY < pBuf->iHeight; iY++)
  {
    pSrcline2 = pSrcline1 + pBuf->iRowsize;

    if (fMagnifyX)                     /* magnifying in X-direction ? */
    {
      iRetcode = fMagnifyX (pData, pImage->iMAGN_MX,
                            pImage->iMAGN_ML, pImage->iMAGN_MR,
                            pBuf->iWidth, pSrcline1, pDstline);

      if (iRetcode)                    /* on error bail out */
      {
        MNG_FREEX (pData, pTempline, iNewrowsize);
        MNG_FREEX (pData, pNewdata,  iNewsize);
        return iRetcode;
      }
    }
    else
    {
      MNG_COPY (pDstline, pSrcline1, iNewrowsize);
    }

    pDstline += iNewrowsize;
                                       /* magnifying in Y-direction ? */
    if ((fMagnifyY) &&
        ((iY < pBuf->iHeight - 1) || (pBuf->iHeight == 1) || (pImage->iMAGN_MethodY == 1)))
    {
      if (iY == 0)                     /* first interval ? */
      {
        if (pBuf->iHeight == 1)        /* single row ? */
          pSrcline2 = MNG_NULL;

        iM = (mng_int32)pImage->iMAGN_MT;
      }
      else                             /* last interval ? */
      if (((pImage->iMAGN_MethodY == 1) && (iY == (pBuf->iHeight - 1))) ||
          ((pImage->iMAGN_MethodY != 1) && (iY == (pBuf->iHeight - 2)))    )
        iM = (mng_int32)pImage->iMAGN_MB;
      else                             /* middle interval */
        iM = (mng_int32)pImage->iMAGN_MY;

      for (iS = 1; iS < iM; iS++)
      {
        iRetcode = fMagnifyY (pData, iS, iM, pBuf->iWidth,
                              pSrcline1, pSrcline2, pTempline);

        if (iRetcode)                  /* on error bail out */
        {
          MNG_FREEX (pData, pTempline, iNewrowsize);
          MNG_FREEX (pData, pNewdata,  iNewsize);
          return iRetcode;
        }

        if (fMagnifyX)                   /* magnifying in X-direction ? */
        {
          iRetcode = fMagnifyX (pData, pImage->iMAGN_MX,
                                pImage->iMAGN_ML, pImage->iMAGN_MR,
                                pBuf->iWidth, pTempline, pDstline);

          if (iRetcode)                  /* on error bail out */
          {
            MNG_FREEX (pData, pTempline, iNewrowsize);
            MNG_FREEX (pData, pNewdata,  iNewsize);
            return iRetcode;
          }
        }
        else
        {
          MNG_COPY (pDstline, pTempline, iNewrowsize);
        }

        pDstline  += iNewrowsize;
      }
    }

    pSrcline1 += pBuf->iRowsize;
  }
                                       /* drop temporary row */
  MNG_FREEX (pData, pTempline, iNewrowsize);
                                       /* drop old pixel-data */
  MNG_FREEX (pData, pBuf->pImgdata, pBuf->iImgdatasize);

  pBuf->pImgdata     = pNewdata;       /* save new buffer dimensions */
  pBuf->iRowsize     = iNewrowsize;
  pBuf->iImgdatasize = iNewsize;
  pBuf->iWidth       = iNewW;
  pBuf->iHeight      = iNewH;

  if (pImage->iId)                     /* real object ? */
  {
    pImage->iMAGN_MethodX = 0;         /* it's done; don't do it again !!! */
    pImage->iMAGN_MethodY = 0;
    pImage->iMAGN_MX      = 0;
    pImage->iMAGN_MY      = 0;
    pImage->iMAGN_ML      = 0;
    pImage->iMAGN_MR      = 0;
    pImage->iMAGN_MT      = 0;
    pImage->iMAGN_MB      = 0;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_MAGNIFY_IMGOBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_colorcorrect_object (mng_datap  pData,
                                     mng_imagep pImage)
{
  mng_imagedatap pBuf = pImage->pImgbuf;
  mng_retcode    iRetcode;
  mng_uint32     iY;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_COLORCORRECT_OBJECT, MNG_LC_START);
#endif

#ifdef MNG_INCLUDE_JNG
  if ((pBuf->iBitdepth < 8) ||         /* we need 8- or 16-bit RGBA !!! */
      ((pBuf->iColortype != MNG_COLORTYPE_RGBA      ) &&
       (pBuf->iColortype != MNG_COLORTYPE_JPEGCOLORA)    ))
#else
  if (pBuf->iBitdepth < 8)         /* we need 8- or 16-bit RGBA !!! */
#endif
    MNG_ERROR (pData, MNG_OBJNOTABSTRACT);

  if (!pBuf->bCorrected)               /* only if not already done ! */
  {                                    /* so the row routines now to find it */
    pData->pRetrieveobj   = (mng_objectp)pImage;
    pData->pStoreobj      = (mng_objectp)pImage;
    pData->pStorebuf      = (mng_objectp)pImage->pImgbuf;

#ifndef MNG_NO_16BIT_SUPPORT
    if (pBuf->iBitdepth > 8)
    {
      pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba16;
      pData->fStorerow    = (mng_fptr)mng_store_rgba16;
    }
    else
#endif
    {
      pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba8;
      pData->fStorerow    = (mng_fptr)mng_store_rgba8;
    }

    pData->bIsOpaque      = MNG_FALSE;

    pData->iPass          = -1;        /* these are the object's dimensions now */
    pData->iRow           = 0;
    pData->iRowinc        = 1;
    pData->iCol           = 0;
    pData->iColinc        = 1;
    pData->iRowsamples    = pBuf->iWidth;
    pData->iRowsize       = pData->iRowsamples << 2;
    pData->iPixelofs      = 0;
    pData->bIsRGBA16      = MNG_FALSE;
                                       /* adjust for 16-bit object ? */
#ifndef MNG_NO_16BIT_SUPPORT
    if (pBuf->iBitdepth > 8)
    {
      pData->bIsRGBA16    = MNG_TRUE;
      pData->iRowsize     = pData->iRowsamples << 3;
    }
#endif

    pData->fCorrectrow    = MNG_NULL;  /* default no color-correction */

#ifdef MNG_NO_CMS
    iRetcode = MNG_NOERROR;
#else
#if defined(MNG_FULL_CMS)              /* determine color-management routine */
    iRetcode = mng_init_full_cms   (pData, MNG_FALSE, MNG_FALSE, MNG_TRUE);
#elif defined(MNG_GAMMA_ONLY)
    iRetcode = mng_init_gamma_only (pData, MNG_FALSE, MNG_FALSE, MNG_TRUE);
#elif defined(MNG_APP_CMS)
    iRetcode = mng_init_app_cms    (pData, MNG_FALSE, MNG_FALSE, MNG_TRUE);
#endif
    if (iRetcode)                      /* on error bail out */
      return iRetcode;
#endif /* MNG_NO_CMS */

    if (pData->fCorrectrow)            /* really correct something ? */
    {                                  /* get a temporary row-buffer */
      MNG_ALLOC (pData, pData->pRGBArow, pData->iRowsize);

      pData->pWorkrow = pData->pRGBArow;
      iY              = 0;             /* start from the top */

      while ((!iRetcode) && (iY < pBuf->iHeight))
      {                                /* get a row */
        iRetcode = ((mng_retrieverow)pData->fRetrieverow) (pData);

        if (!iRetcode)                 /* color correct it */
          iRetcode = ((mng_correctrow)pData->fCorrectrow) (pData);

        if (!iRetcode)                 /* store it back ! */
          iRetcode = ((mng_storerow)pData->fStorerow) (pData);

        if (!iRetcode)                 /* adjust variables for next row */
          iRetcode = mng_next_row (pData);

        iY++;                          /* and next line */
      }
                                       /* drop the temporary row-buffer */
      MNG_FREEX (pData, pData->pRGBArow, pData->iRowsize);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;

#if defined(MNG_FULL_CMS)              /* cleanup cms stuff */
      iRetcode = mng_clear_cms (pData);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
#endif
    }

    pBuf->bCorrected = MNG_TRUE;       /* let's not go through that again ! */
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_COLORCORRECT_OBJECT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* *                                                                        * */
/* * Animation-object routines                                              * */
/* *                                                                        * */
/* * these handle the animation objects used to re-run parts of a MNG.      * */
/* * eg. during LOOP or TERM processing                                     * */
/* *                                                                        * */
/* ************************************************************************** */

void mng_add_ani_object (mng_datap          pData,
                         mng_object_headerp pObject)
{
  mng_object_headerp pLast = (mng_object_headerp)pData->pLastaniobj;

  if (pLast)                           /* link it as last in the chain */
  {
    pObject->pPrev      = pLast;
    pLast->pNext        = pObject;
  }
  else
  {
    pObject->pPrev      = MNG_NULL;    /* be on the safe side */
    pData->pFirstaniobj = pObject;
  }

  pObject->pNext        = MNG_NULL;    /* be on the safe side */
  pData->pLastaniobj    = pObject;
                                       /* keep track for jumping */
  pObject->iFramenr     = pData->iFrameseq;
  pObject->iLayernr     = pData->iLayerseq;
  pObject->iPlaytime    = pData->iFrametime;
                                       /* save restart object ? */
  if ((pData->bDisplaying) && (!pData->bRunning) && (!pData->pCurraniobj))
    pData->pCurraniobj  = pObject;

  return;
}

/* ************************************************************************** */
/* ************************************************************************** */

mng_retcode mng_create_ani_image (mng_datap pData)
{
  mng_ani_imagep pImage;
  mng_imagep     pCurrent;
  mng_retcode    iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_IMAGE, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifndef MNG_NO_DELTA_PNG
    if (pData->bHasDHDR)               /* processing delta-image ? */
      pCurrent = (mng_imagep)pData->pObjzero;
    else                               /* get the current object */
#endif
      pCurrent = (mng_imagep)pData->pCurrentobj;

    if (!pCurrent)                     /* otherwise object 0 */
      pCurrent = (mng_imagep)pData->pObjzero;
                                       /* now just clone the object !!! */
    iRetcode  = mng_clone_imageobject (pData, 0, MNG_FALSE, pCurrent->bVisible,
                                       MNG_FALSE, MNG_FALSE, 0, 0, 0, pCurrent,
                                       &pImage);

    if (iRetcode)                      /* on error bail out */
      return iRetcode;

    pImage->sHeader.fCleanup = mng_free_ani_image;
    pImage->sHeader.fProcess = mng_process_ani_image;

    mng_add_ani_object (pData, (mng_object_headerp)pImage);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_IMAGE, MNG_LC_END);
#endif

  return MNG_NOERROR;                  /* okido */
}

/* ************************************************************************** */

mng_retcode mng_free_ani_image (mng_datap   pData,
                                mng_objectp pObject)
{
  mng_ani_imagep pImage = (mng_ani_imagep)pObject;
  mng_imagedatap pImgbuf = pImage->pImgbuf;
  mng_retcode    iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_IMAGE, MNG_LC_START);
#endif
                                       /* unlink the image-data buffer */
  iRetcode = mng_free_imagedataobject (pData, pImgbuf);
                                       /* drop its own buffer */
  MNG_FREEX (pData, pImage, sizeof (mng_ani_image));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_IMAGE, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

mng_retcode mng_process_ani_image (mng_datap   pData,
                                   mng_objectp pObject)
{
  mng_retcode    iRetcode = MNG_NOERROR;
  mng_ani_imagep pImage   = (mng_imagep)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_IMAGE, MNG_LC_START);
#endif

#ifndef MNG_NO_DELTA_PNG
  if (pData->bHasDHDR)                 /* processing delta-image ? */
  {
    mng_imagep pDelta = (mng_imagep)pData->pDeltaImage;

    if (!pData->iBreakpoint)           /* only execute if not broken before */
    {                                  /* make sure to process pixels as well */
      pData->bDeltaimmediate = MNG_FALSE;
                                       /* execute the delta process */
      iRetcode = mng_execute_delta_image (pData, pDelta, (mng_imagep)pObject);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
    }
                                       /* now go and shoot it off (if required) */
    if ((pDelta->bVisible) && (pDelta->bViewable))
      iRetcode = mng_display_image (pData, pDelta, MNG_FALSE);

    if (!pData->bTimerset)
      pData->bHasDHDR = MNG_FALSE;     /* this image signifies IEND !! */

  }
  else
#endif
  if (pData->pCurrentobj)              /* active object ? */
  {
    mng_imagep     pCurrent = (mng_imagep)pData->pCurrentobj;
    mng_imagedatap pBuf     = pCurrent->pImgbuf;

    if (!pData->iBreakpoint)           /* don't copy it again ! */
    {
      if (pBuf->iImgdatasize)          /* buffer present in active object ? */
                                       /* then drop it */
        MNG_FREE (pData, pBuf->pImgdata, pBuf->iImgdatasize);

#ifndef MNG_SKIPCHUNK_iCCP
      if (pBuf->iProfilesize)          /* iCCP profile present ? */
                                       /* then drop it */
        MNG_FREE (pData, pBuf->pProfile, pBuf->iProfilesize);
#endif
                                       /* now blatently copy the animation buffer */
      MNG_COPY (pBuf, pImage->pImgbuf, sizeof (mng_imagedata));
                                       /* copy viewability */
      pCurrent->bViewable = pImage->bViewable;

      if (pBuf->iImgdatasize)          /* sample buffer present ? */
      {                                /* then make a copy */
        MNG_ALLOC (pData, pBuf->pImgdata, pBuf->iImgdatasize);
        MNG_COPY (pBuf->pImgdata, pImage->pImgbuf->pImgdata, pBuf->iImgdatasize);
      }

#ifndef MNG_SKIPCHUNK_iCCP
      if (pBuf->iProfilesize)          /* iCCP profile present ? */
      {                                /* then make a copy */
        MNG_ALLOC (pData, pBuf->pProfile, pBuf->iProfilesize);
        MNG_COPY (pBuf->pProfile, pImage->pImgbuf->pProfile, pBuf->iProfilesize);
      }
#endif
    }
                                       /* now go and shoot it off (if required) */
    if ((pCurrent->bVisible) && (pCurrent->bViewable))
      iRetcode = mng_display_image (pData, pCurrent, MNG_FALSE);
  }
  else
  {
    mng_imagep     pObjzero = (mng_imagep)pData->pObjzero;
    mng_imagedatap pBuf     = pObjzero->pImgbuf;

    if (!pData->iBreakpoint)           /* don't copy it again ! */
    {
      if (pBuf->iImgdatasize)          /* buffer present in active object ? */
                                       /* then drop it */
        MNG_FREE (pData, pBuf->pImgdata, pBuf->iImgdatasize);

#ifndef MNG_SKIPCHUNK_iCCP
      if (pBuf->iProfilesize)          /* iCCP profile present ? */
                                       /* then drop it */
        MNG_FREE (pData, pBuf->pProfile, pBuf->iProfilesize);
#endif
                                       /* now blatently copy the animation buffer */
      MNG_COPY (pBuf, pImage->pImgbuf, sizeof (mng_imagedata));
                                       /* copy viewability */
      pObjzero->bViewable = pImage->bViewable;

      if (pBuf->iImgdatasize)          /* sample buffer present ? */
      {                                /* then make a copy */
        MNG_ALLOC (pData, pBuf->pImgdata, pBuf->iImgdatasize);
        MNG_COPY (pBuf->pImgdata, pImage->pImgbuf->pImgdata, pBuf->iImgdatasize);
      }

#ifndef MNG_SKIPCHUNK_iCCP
      if (pBuf->iProfilesize)          /* iCCP profile present ? */
      {                                /* then make a copy */
        MNG_ALLOC (pData, pBuf->pProfile, pBuf->iProfilesize);
        MNG_COPY (pBuf->pProfile, pImage->pImgbuf->pProfile, pBuf->iProfilesize);
      }
#endif
    }
                                       /* now go and show it */
    iRetcode = mng_display_image (pData, pObjzero, MNG_FALSE);
  }

  if (!iRetcode)                       /* all's well ? */
  {
    if (pData->bTimerset)              /* timer break ? */
      pData->iBreakpoint = 99;         /* fictive number; no more processing needed! */
    else
      pData->iBreakpoint = 0;          /* else clear it */
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_IMAGE, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_plte (mng_datap      pData,
                                 mng_uint32     iEntrycount,
                                 mng_palette8ep paEntries)
#else
mng_retcode mng_create_ani_plte (mng_datap      pData)
#endif
{
  mng_ani_pltep pPLTE;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_PLTE, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_plte),
                                               mng_free_obj_general,
                                               mng_process_ani_plte,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pPLTE = (mng_ani_pltep)pTemp;
#else
    MNG_ALLOC (pData, pPLTE, sizeof (mng_ani_plte));

    pPLTE->sHeader.fCleanup = mng_free_ani_plte;
    pPLTE->sHeader.fProcess = mng_process_ani_plte;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pPLTE);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pPLTE->iEntrycount = iEntrycount;
    MNG_COPY (pPLTE->aEntries, paEntries, sizeof (pPLTE->aEntries));
#else
    pPLTE->iEntrycount = pData->iGlobalPLTEcount;
    MNG_COPY (pPLTE->aEntries, pData->aGlobalPLTEentries, sizeof (pPLTE->aEntries));
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_PLTE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_plte (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_PLTE, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_plte));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_PLTE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_plte (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_pltep pPLTE = (mng_ani_pltep)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_PLTE, MNG_LC_START);
#endif

  pData->bHasglobalPLTE   = MNG_TRUE;
  pData->iGlobalPLTEcount = pPLTE->iEntrycount;

  MNG_COPY (pData->aGlobalPLTEentries, pPLTE->aEntries, sizeof (pPLTE->aEntries));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_PLTE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_trns (mng_datap    pData,
                                 mng_uint32   iRawlen,
                                 mng_uint8p   pRawdata)
#else
mng_retcode mng_create_ani_trns (mng_datap    pData)
#endif
{
  mng_ani_trnsp pTRNS;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_TRNS, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_trns),
                                               mng_free_obj_general,
                                               mng_process_ani_trns,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pTRNS = (mng_ani_trnsp)pTemp;
#else
    MNG_ALLOC (pData, pTRNS, sizeof (mng_ani_trns));

    pTRNS->sHeader.fCleanup = mng_free_ani_trns;
    pTRNS->sHeader.fProcess = mng_process_ani_trns;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pTRNS);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pTRNS->iRawlen = iRawlen;
    MNG_COPY (pTRNS->aRawdata, pRawdata, sizeof (pTRNS->aRawdata));
#else
    pTRNS->iRawlen = pData->iGlobalTRNSrawlen;
    MNG_COPY (pTRNS->aRawdata, pData->aGlobalTRNSrawdata, sizeof (pTRNS->aRawdata));
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_TRNS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_trns (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_TRNS, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_trns));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_TRNS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_trns (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_trnsp pTRNS = (mng_ani_trnsp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_TRNS, MNG_LC_START);
#endif

  pData->bHasglobalTRNS    = MNG_TRUE;
  pData->iGlobalTRNSrawlen = pTRNS->iRawlen;

  MNG_COPY (pData->aGlobalTRNSrawdata, pTRNS->aRawdata, sizeof (pTRNS->aRawdata));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_TRNS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_gAMA
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_gama (mng_datap  pData,
                                 mng_bool   bEmpty,
                                 mng_uint32 iGamma)
#else
mng_retcode mng_create_ani_gama (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_gamap pGAMA;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_GAMA, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_gama),
                                               mng_free_obj_general,
                                               mng_process_ani_gama,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pGAMA = (mng_ani_gamap)pTemp;
#else
    MNG_ALLOC (pData, pGAMA, sizeof (mng_ani_gama));

    pGAMA->sHeader.fCleanup = mng_free_ani_gama;
    pGAMA->sHeader.fProcess = mng_process_ani_gama;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pGAMA);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pGAMA->bEmpty = bEmpty;
    pGAMA->iGamma = iGamma;
#else
    pGAMA->bEmpty = ((mng_gamap)pChunk)->bEmpty;
    pGAMA->iGamma = ((mng_gamap)pChunk)->iGamma;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_GAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_gama (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_GAMA, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_gama));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_GAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_gama (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_gamap pGAMA = (mng_ani_gamap)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_GAMA, MNG_LC_START);
#endif

  if (pGAMA->bEmpty)                   /* empty chunk ? */
  {                                    /* clear global gAMA */
    pData->bHasglobalGAMA = MNG_FALSE;
    pData->iGlobalGamma   = 0;
  }
  else
  {                                    /* set global gAMA */
    pData->bHasglobalGAMA = MNG_TRUE;
    pData->iGlobalGamma   = pGAMA->iGamma;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_GAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_cHRM
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_chrm (mng_datap  pData,
                                 mng_bool   bEmpty,
                                 mng_uint32 iWhitepointx,
                                 mng_uint32 iWhitepointy,
                                 mng_uint32 iRedx,
                                 mng_uint32 iRedy,
                                 mng_uint32 iGreenx,
                                 mng_uint32 iGreeny,
                                 mng_uint32 iBluex,
                                 mng_uint32 iBluey)
#else
mng_retcode mng_create_ani_chrm (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
//  mng_ptr       pTemp;
  mng_ani_chrmp pCHRM;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_CHRM, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_chrm),
                                               mng_free_obj_general,
                                               mng_process_ani_chrm,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pCHRM = (mng_ani_chrmp)pTemp;
#else
    MNG_ALLOC (pData, pCHRM, sizeof (mng_ani_chrm));

    pCHRM->sHeader.fCleanup = mng_free_ani_chrm;
    pCHRM->sHeader.fProcess = mng_process_ani_chrm;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pCHRM);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pCHRM->bEmpty       = bEmpty;
    pCHRM->iWhitepointx = iWhitepointx;
    pCHRM->iWhitepointy = iWhitepointy;
    pCHRM->iRedx        = iRedx;
    pCHRM->iRedy        = iRedy;
    pCHRM->iGreenx      = iGreenx;
    pCHRM->iGreeny      = iGreeny;
    pCHRM->iBluex       = iBluex;
    pCHRM->iBluey       = iBluey;
#else
    pCHRM->bEmpty       = ((mng_chrmp)pChunk)->bEmpty;
    pCHRM->iWhitepointx = ((mng_chrmp)pChunk)->iWhitepointx;
    pCHRM->iWhitepointy = ((mng_chrmp)pChunk)->iWhitepointy;
    pCHRM->iRedx        = ((mng_chrmp)pChunk)->iRedx;
    pCHRM->iRedy        = ((mng_chrmp)pChunk)->iRedy;
    pCHRM->iGreenx      = ((mng_chrmp)pChunk)->iGreenx;
    pCHRM->iGreeny      = ((mng_chrmp)pChunk)->iGreeny;
    pCHRM->iBluex       = ((mng_chrmp)pChunk)->iBluex;
    pCHRM->iBluey       = ((mng_chrmp)pChunk)->iBluey;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_CHRM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_chrm (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_CHRM, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_chrm));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_CHRM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_chrm (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_chrmp pCHRM = (mng_ani_chrmp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_CHRM, MNG_LC_START);
#endif

  if (pCHRM->bEmpty)                   /* empty chunk ? */
  {                                    /* clear global cHRM */
    pData->bHasglobalCHRM       = MNG_FALSE;
    pData->iGlobalWhitepointx   = 0;
    pData->iGlobalWhitepointy   = 0;
    pData->iGlobalPrimaryredx   = 0;
    pData->iGlobalPrimaryredy   = 0;
    pData->iGlobalPrimarygreenx = 0;
    pData->iGlobalPrimarygreeny = 0;
    pData->iGlobalPrimarybluex  = 0;
    pData->iGlobalPrimarybluey  = 0;
  }
  else
  {                                    /* set global cHRM */
    pData->bHasglobalCHRM       = MNG_TRUE;
    pData->iGlobalWhitepointx   = pCHRM->iWhitepointx;
    pData->iGlobalWhitepointy   = pCHRM->iWhitepointy;
    pData->iGlobalPrimaryredx   = pCHRM->iRedx;
    pData->iGlobalPrimaryredy   = pCHRM->iRedy;
    pData->iGlobalPrimarygreenx = pCHRM->iGreenx;
    pData->iGlobalPrimarygreeny = pCHRM->iGreeny;
    pData->iGlobalPrimarybluex  = pCHRM->iBluex;
    pData->iGlobalPrimarybluey  = pCHRM->iBluey;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_CHRM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sRGB
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_srgb (mng_datap pData,
                                 mng_bool  bEmpty,
                                 mng_uint8 iRenderingintent)
#else
mng_retcode mng_create_ani_srgb (mng_datap pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_srgbp pSRGB;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_SRGB, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_srgb),
                                               mng_free_obj_general,
                                               mng_process_ani_srgb,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pSRGB = (mng_ani_srgbp)pTemp;
#else
    MNG_ALLOC (pData, pSRGB, sizeof (mng_ani_srgb));

    pSRGB->sHeader.fCleanup = mng_free_ani_srgb;
    pSRGB->sHeader.fProcess = mng_process_ani_srgb;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pSRGB);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pSRGB->bEmpty           = bEmpty;
    pSRGB->iRenderingintent = iRenderingintent;
#else
    pSRGB->bEmpty           = ((mng_srgbp)pChunk)->bEmpty;
    pSRGB->iRenderingintent = ((mng_srgbp)pChunk)->iRenderingintent;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_SRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_srgb (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_SRGB, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_srgb));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_SRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_srgb (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_srgbp pSRGB = (mng_ani_srgbp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_SRGB, MNG_LC_START);
#endif

  if (pSRGB->bEmpty)                   /* empty chunk ? */
  {                                    /* clear global sRGB */
    pData->bHasglobalSRGB    = MNG_FALSE;
    pData->iGlobalRendintent = 0;
  }
  else
  {                                    /* set global sRGB */
    pData->bHasglobalSRGB    = MNG_TRUE;
    pData->iGlobalRendintent = pSRGB->iRenderingintent;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_SRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_iccp (mng_datap  pData,
                                 mng_bool   bEmpty,
                                 mng_uint32 iProfilesize,
                                 mng_ptr    pProfile)
#else
mng_retcode mng_create_ani_iccp (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
//  mng_ptr       pTemp;
  mng_ani_iccpp pICCP;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_ICCP, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_iccp),
                                               mng_free_ani_iccp,
                                               mng_process_ani_iccp,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pICCP = (mng_ani_iccpp)pTemp;
#else
    MNG_ALLOC (pData, pICCP, sizeof (mng_ani_iccp));

    pICCP->sHeader.fCleanup = mng_free_ani_iccp;
    pICCP->sHeader.fProcess = mng_process_ani_iccp;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pICCP);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pICCP->bEmpty       = bEmpty;
    pICCP->iProfilesize = iProfilesize;

    if (iProfilesize)
    {
      MNG_ALLOC (pData, pICCP->pProfile, iProfilesize);
      MNG_COPY (pICCP->pProfile, pProfile, iProfilesize);
    }
#else
    pICCP->bEmpty       = ((mng_iccpp)pChunk)->bEmpty;
    pICCP->iProfilesize = ((mng_iccpp)pChunk)->iProfilesize;

    if (pICCP->iProfilesize)
    {
      MNG_ALLOC (pData, pICCP->pProfile, pICCP->iProfilesize);
      MNG_COPY (pICCP->pProfile, ((mng_iccpp)pChunk)->pProfile, pICCP->iProfilesize);
    }
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_ICCP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_free_ani_iccp (mng_datap   pData,
                               mng_objectp pObject)
{
  mng_ani_iccpp pICCP = (mng_ani_iccpp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_ICCP, MNG_LC_START);
#endif

  if (pICCP->iProfilesize)
    MNG_FREEX (pData, pICCP->pProfile, pICCP->iProfilesize);

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  MNG_FREEX (pData, pObject, sizeof (mng_ani_iccp));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_ICCP, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  return MNG_NOERROR;
#else
  return mng_free_obj_general(pData, pObject);
#endif
}

/* ************************************************************************** */

mng_retcode mng_process_ani_iccp (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_iccpp pICCP = (mng_ani_iccpp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_ICCP, MNG_LC_START);
#endif

  if (pICCP->bEmpty)                   /* empty chunk ? */
  {                                    /* clear global iCCP */
    pData->bHasglobalICCP = MNG_FALSE;

    if (pData->iGlobalProfilesize)
      MNG_FREEX (pData, pData->pGlobalProfile, pData->iGlobalProfilesize);

    pData->iGlobalProfilesize = 0;
    pData->pGlobalProfile     = MNG_NULL;
  }
  else
  {                                    /* set global iCCP */
    pData->bHasglobalICCP     = MNG_TRUE;
    pData->iGlobalProfilesize = pICCP->iProfilesize;

    if (pICCP->iProfilesize)
    {
      MNG_ALLOC (pData, pData->pGlobalProfile, pICCP->iProfilesize);
      MNG_COPY (pData->pGlobalProfile, pICCP->pProfile, pICCP->iProfilesize);
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_ICCP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_bKGD
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_bkgd (mng_datap  pData,
                                 mng_uint16 iRed,
                                 mng_uint16 iGreen,
                                 mng_uint16 iBlue)
#else
mng_retcode mng_create_ani_bkgd (mng_datap  pData)
#endif
{
//  mng_ptr       pTemp;
  mng_ani_bkgdp pBKGD;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_BKGD, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_bkgd),
                                               mng_free_obj_general,
                                               mng_process_ani_bkgd,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pBKGD = (mng_ani_bkgdp)pTemp;
#else
    MNG_ALLOC (pData, pBKGD, sizeof (mng_ani_bkgd));

    pBKGD->sHeader.fCleanup = mng_free_ani_bkgd;
    pBKGD->sHeader.fProcess = mng_process_ani_bkgd;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pBKGD);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pBKGD->iRed   = iRed;
    pBKGD->iGreen = iGreen;
    pBKGD->iBlue  = iBlue;
#else
    pBKGD->iRed   = pData->iGlobalBKGDred;
    pBKGD->iGreen = pData->iGlobalBKGDgreen;
    pBKGD->iBlue  = pData->iGlobalBKGDblue;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_BKGD, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_bkgd (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_BKGD, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_bkgd));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_BKGD, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_bkgd (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_bkgdp pBKGD = (mng_ani_bkgdp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_BKGD, MNG_LC_START);
#endif

  pData->bHasglobalBKGD   = MNG_TRUE;
  pData->iGlobalBKGDred   = pBKGD->iRed;
  pData->iGlobalBKGDgreen = pBKGD->iGreen;
  pData->iGlobalBKGDblue  = pBKGD->iBlue;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_BKGD, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_LOOP
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_loop (mng_datap   pData,
                                 mng_uint8   iLevel,
                                 mng_uint32  iRepeatcount,
                                 mng_uint8   iTermcond,
                                 mng_uint32  iItermin,
                                 mng_uint32  iItermax,
                                 mng_uint32  iCount,
                                 mng_uint32p pSignals)
#else
mng_retcode mng_create_ani_loop (mng_datap   pData,
                                 mng_chunkp  pChunk)
#endif
{
  mng_ani_loopp pLOOP;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_LOOP, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_loop),
                                               mng_free_ani_loop,
                                               mng_process_ani_loop,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pLOOP = (mng_ani_loopp)pTemp;
#else
    MNG_ALLOC (pData, pLOOP, sizeof (mng_ani_loop));

    pLOOP->sHeader.fCleanup = mng_free_ani_loop;
    pLOOP->sHeader.fProcess = mng_process_ani_loop;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pLOOP);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pLOOP->iLevel       = iLevel;
    pLOOP->iRepeatcount = iRepeatcount;
    pLOOP->iTermcond    = iTermcond;
    pLOOP->iItermin     = iItermin;
    pLOOP->iItermax     = iItermax;
    pLOOP->iCount       = iCount;

#ifndef MNG_NO_LOOP_SIGNALS_SUPPORTED
    if (iCount)
    {
      MNG_ALLOC (pData, pLOOP->pSignals, (iCount << 1));
      MNG_COPY (pLOOP->pSignals, pSignals, (iCount << 1));
    }
#endif
#else /* MNG_OPTIMIZE_CHUNKREADER */
    pLOOP->iLevel       = ((mng_loopp)pChunk)->iLevel;
    pLOOP->iRepeatcount = ((mng_loopp)pChunk)->iRepeat;
    pLOOP->iTermcond    = ((mng_loopp)pChunk)->iTermination;
    pLOOP->iItermin     = ((mng_loopp)pChunk)->iItermin;
    pLOOP->iItermax     = ((mng_loopp)pChunk)->iItermax;
    pLOOP->iCount       = ((mng_loopp)pChunk)->iCount;

#ifndef MNG_NO_LOOP_SIGNALS_SUPPORTED
    if (pLOOP->iCount)
    {
      MNG_ALLOC (pData, pLOOP->pSignals, (pLOOP->iCount << 1));
      MNG_COPY (pLOOP->pSignals, ((mng_loopp)pChunk)->pSignals, (pLOOP->iCount << 1));
    }
#endif
#endif /* MNG_OPTIMIZE_CHUNKREADER */
                                         /* running counter starts with repeat_count */
    pLOOP->iRunningcount = pLOOP->iRepeatcount;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_LOOP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_free_ani_loop (mng_datap   pData,
                               mng_objectp pObject)
{
#ifndef MNG_NO_LOOP_SIGNALS_SUPPORTED
  mng_ani_loopp pLOOP = (mng_ani_loopp)pObject;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_LOOP, MNG_LC_START);
#endif

#ifndef MNG_NO_LOOP_SIGNALS_SUPPORTED
  if (pLOOP->iCount)                   /* drop signal buffer ? */
    MNG_FREEX (pData, pLOOP->pSignals, (pLOOP->iCount << 1));
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  MNG_FREEX (pData, pObject, sizeof (mng_ani_loop));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_LOOP, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  return MNG_NOERROR;
#else
  return mng_free_obj_general(pData, pObject);
#endif
}

/* ************************************************************************** */

mng_retcode mng_process_ani_loop (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_loopp pLOOP = (mng_ani_loopp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_LOOP, MNG_LC_START);
#endif
                                       /* just reset the running counter */
  pLOOP->iRunningcount = pLOOP->iRepeatcount;
                                       /* iteration=0 means we're skipping ! */
  if ((!pData->bSkipping) && (pLOOP->iRepeatcount == 0))
    pData->bSkipping = MNG_TRUE;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_LOOP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* ************************************************************************** */

mng_retcode mng_create_ani_endl (mng_datap pData,
                                 mng_uint8 iLevel)
{
  mng_ani_endlp pENDL;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_ENDL, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
    mng_retcode iRetcode;
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_endl),
                                               mng_free_obj_general,
                                               mng_process_ani_endl,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pENDL = (mng_ani_endlp)pTemp;
#else
    MNG_ALLOC (pData, pENDL, sizeof (mng_ani_endl));

    pENDL->sHeader.fCleanup = mng_free_ani_endl;
    pENDL->sHeader.fProcess = mng_process_ani_endl;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pENDL);

    pENDL->iLevel = iLevel;

    iRetcode = mng_process_ani_endl (pData, (mng_objectp)pENDL);
    if (iRetcode)
      return iRetcode;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_ENDL, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_endl (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_ENDL, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_endl));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_ENDL, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_endl (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_endlp pENDL = (mng_ani_endlp)pObject;
  mng_ani_loopp pLOOP;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_ENDL, MNG_LC_START);
#endif

  if (((pData->bDisplaying) && ((pData->bRunning) || (pData->bSearching))) ||
      (pData->bReading)                                                       )
  {
    pLOOP = pENDL->pLOOP;              /* determine matching LOOP */

    if (!pLOOP)                        /* haven't got it yet ? */
    {                                  /* go and look back in the list */
      pLOOP = (mng_ani_loopp)pENDL->sHeader.pPrev;

      while ((pLOOP) &&
             ((pLOOP->sHeader.fCleanup != mng_free_ani_loop) ||
              (pLOOP->iLevel           != pENDL->iLevel)        ))
        pLOOP = pLOOP->sHeader.pPrev;
    }
                                       /* got it now ? */
    if ((pLOOP) && (pLOOP->iLevel == pENDL->iLevel))
    {
      pENDL->pLOOP = pLOOP;            /* save for next time ! */
                                       /* decrease running counter ? */
      if ((pLOOP->iRunningcount) && (pLOOP->iRunningcount < 0x7fffffffL))
        pLOOP->iRunningcount--;

      if ((!pData->bDisplaying) && (pData->bReading) &&
          (pLOOP->iRunningcount >= 0x7fffffffL))
      {
        pData->iTotalframes   = 0x7fffffffL;
        pData->iTotallayers   = 0x7fffffffL;
        pData->iTotalplaytime = 0x7fffffffL;
      }
      else
      {
        /* TODO: we're cheating out on the termination_condition,
           iteration_min, iteration_max and possible signals;
           the code is just not ready for that can of worms.... */

        if (!pLOOP->iRunningcount)     /* reached zero ? */
        {                              /* was this the outer LOOP ? */
          if (pData->pFirstaniobj == (mng_objectp)pLOOP)  /* TODO: THIS IS WRONG!! */
            pData->bHasLOOP = MNG_FALSE;
        }
        else
        {
          if (pData->pCurraniobj)      /* was we processing objects ? */
            pData->pCurraniobj = pLOOP;/* then restart with LOOP */
          else                         /* else restart behind LOOP !!! */
            pData->pCurraniobj = pLOOP->sHeader.pNext;
        }
      }
                                       /* does this match a 'skipping' LOOP? */
      if ((pData->bSkipping) && (pLOOP->iRepeatcount == 0))
        pData->bSkipping = MNG_FALSE;
    }
    else
      MNG_ERROR (pData, MNG_NOMATCHINGLOOP);

  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_ENDL, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DEFI
mng_retcode mng_create_ani_defi (mng_datap pData)
{               
  mng_ani_defip pDEFI;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_DEFI, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_defi),
                                               mng_free_obj_general,
                                               mng_process_ani_defi,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pDEFI = (mng_ani_defip)pTemp;
#else
    MNG_ALLOC (pData, pDEFI, sizeof (mng_ani_defi));

    pDEFI->sHeader.fCleanup = mng_free_ani_defi;
    pDEFI->sHeader.fProcess = mng_process_ani_defi;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pDEFI);

    pDEFI->iId              = pData->iDEFIobjectid;
    pDEFI->bHasdonotshow    = pData->bDEFIhasdonotshow;
    pDEFI->iDonotshow       = pData->iDEFIdonotshow;
    pDEFI->bHasconcrete     = pData->bDEFIhasconcrete;
    pDEFI->iConcrete        = pData->iDEFIconcrete;
    pDEFI->bHasloca         = pData->bDEFIhasloca;
    pDEFI->iLocax           = pData->iDEFIlocax;
    pDEFI->iLocay           = pData->iDEFIlocay;
    pDEFI->bHasclip         = pData->bDEFIhasclip;
    pDEFI->iClipl           = pData->iDEFIclipl;
    pDEFI->iClipr           = pData->iDEFIclipr;
    pDEFI->iClipt           = pData->iDEFIclipt;
    pDEFI->iClipb           = pData->iDEFIclipb;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_defi (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_DEFI, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_defi));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_defi (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_defip pDEFI = (mng_ani_defip)pObject;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_DEFI, MNG_LC_START);
#endif

  pData->iDEFIobjectid     = pDEFI->iId;
  pData->bDEFIhasdonotshow = pDEFI->bHasdonotshow;
  pData->iDEFIdonotshow    = pDEFI->iDonotshow;
  pData->bDEFIhasconcrete  = pDEFI->bHasconcrete;
  pData->iDEFIconcrete     = pDEFI->iConcrete;
  pData->bDEFIhasloca      = pDEFI->bHasloca;
  pData->iDEFIlocax        = pDEFI->iLocax;
  pData->iDEFIlocay        = pDEFI->iLocay;
  pData->bDEFIhasclip      = pDEFI->bHasclip;
  pData->iDEFIclipl        = pDEFI->iClipl;
  pData->iDEFIclipr        = pDEFI->iClipr;
  pData->iDEFIclipt        = pDEFI->iClipt;
  pData->iDEFIclipb        = pDEFI->iClipb;

  iRetcode = mng_process_display_defi (pData);

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BASI
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_basi (mng_datap  pData,
                                 mng_uint16 iRed,
                                 mng_uint16 iGreen,
                                 mng_uint16 iBlue,
                                 mng_bool   bHasalpha,
                                 mng_uint16 iAlpha,
                                 mng_uint8  iViewable)
#else
mng_retcode mng_create_ani_basi (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_basip pBASI;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_BASI, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_basi),
                                   mng_free_obj_general,
                                   mng_process_ani_basi,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pBASI = (mng_ani_basip)pTemp;
#else
    MNG_ALLOC (pData, pBASI, sizeof (mng_ani_basi));

    pBASI->sHeader.fCleanup = mng_free_ani_basi;
    pBASI->sHeader.fProcess = mng_process_ani_basi;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pBASI);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pBASI->iRed      = iRed;
    pBASI->iGreen    = iGreen;
    pBASI->iBlue     = iBlue;
    pBASI->bHasalpha = bHasalpha;
    pBASI->iAlpha    = iAlpha;
    pBASI->iViewable = iViewable;
#else
    pBASI->iRed      = ((mng_basip)pChunk)->iRed;
    pBASI->iGreen    = ((mng_basip)pChunk)->iGreen;
    pBASI->iBlue     = ((mng_basip)pChunk)->iBlue;
    pBASI->bHasalpha = ((mng_basip)pChunk)->bHasalpha;
    pBASI->iAlpha    = ((mng_basip)pChunk)->iAlpha;
    pBASI->iViewable = ((mng_basip)pChunk)->iViewable;
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_basi (pData, iRed, iGreen, iBlue,
                                       bHasalpha, iAlpha, iViewable);
#else
  iRetcode = mng_process_display_basi (pData,
                                       ((mng_basip)pChunk)->iRed,
                                       ((mng_basip)pChunk)->iGreen,
                                       ((mng_basip)pChunk)->iBlue,
                                       ((mng_basip)pChunk)->bHasalpha,
                                       ((mng_basip)pChunk)->iAlpha,
                                       ((mng_basip)pChunk)->iViewable);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iBASIred      = iRed;
  pData->iBASIgreen    = iGreen;
  pData->iBASIblue     = iBlue;
  pData->bBASIhasalpha = bHasalpha;
  pData->iBASIalpha    = iAlpha;
  pData->iBASIviewable = iViewable;
#else
  pData->iBASIred      = ((mng_basip)pChunk)->iRed;
  pData->iBASIgreen    = ((mng_basip)pChunk)->iGreen;
  pData->iBASIblue     = ((mng_basip)pChunk)->iBlue;
  pData->bBASIhasalpha = ((mng_basip)pChunk)->bHasalpha;
  pData->iBASIalpha    = ((mng_basip)pChunk)->iAlpha;
  pData->iBASIviewable = ((mng_basip)pChunk)->iViewable;
#endif

  iRetcode = mng_process_display_basi (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_BASI, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_basi (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_BASI, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_basi));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_BASI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_basi (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_basip pBASI = (mng_ani_basip)pObject;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_BASI, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_basi (pData, pBASI->iRed, pBASI->iGreen, pBASI->iBlue,
                                       pBASI->bHasalpha, pBASI->iAlpha, pBASI->iViewable);
#else
  pData->iBASIred      = pBASI->iRed;
  pData->iBASIgreen    = pBASI->iGreen;
  pData->iBASIblue     = pBASI->iBlue;
  pData->bBASIhasalpha = pBASI->bHasalpha;
  pData->iBASIalpha    = pBASI->iAlpha;
  pData->iBASIviewable = pBASI->iViewable;

  iRetcode = mng_process_display_basi (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_BASI, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLON
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_clon (mng_datap  pData,
                                 mng_uint16 iSourceid,
                                 mng_uint16 iCloneid,
                                 mng_uint8  iClonetype,
                                 mng_bool   bHasdonotshow,
                                 mng_uint8  iDonotshow,
                                 mng_uint8  iConcrete,
                                 mng_bool   bHasloca,
                                 mng_uint8  iLocatype,
                                 mng_int32  iLocax,
                                 mng_int32  iLocay)
#else
mng_retcode mng_create_ani_clon (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_clonp pCLON;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_CLON, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_clon),
                                   mng_free_obj_general,
                                   mng_process_ani_clon,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pCLON = (mng_ani_clonp)pTemp;
#else
    MNG_ALLOC (pData, pCLON, sizeof (mng_ani_clon));

    pCLON->sHeader.fCleanup = mng_free_ani_clon;
    pCLON->sHeader.fProcess = mng_process_ani_clon;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pCLON);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pCLON->iSourceid     = iSourceid;
    pCLON->iCloneid      = iCloneid;
    pCLON->iClonetype    = iClonetype;
    pCLON->bHasdonotshow = bHasdonotshow;
    pCLON->iDonotshow    = iDonotshow;
    pCLON->iConcrete     = iConcrete;
    pCLON->bHasloca      = bHasloca;
    pCLON->iLocatype     = iLocatype;
    pCLON->iLocax        = iLocax;
    pCLON->iLocay        = iLocay;
#else
    pCLON->iSourceid     = ((mng_clonp)pChunk)->iSourceid;
    pCLON->iCloneid      = ((mng_clonp)pChunk)->iCloneid;
    pCLON->iClonetype    = ((mng_clonp)pChunk)->iClonetype;
    pCLON->bHasdonotshow = ((mng_clonp)pChunk)->bHasdonotshow;
    pCLON->iDonotshow    = ((mng_clonp)pChunk)->iDonotshow;
    pCLON->iConcrete     = ((mng_clonp)pChunk)->iConcrete;
    pCLON->bHasloca      = ((mng_clonp)pChunk)->bHasloca;
    pCLON->iLocatype     = ((mng_clonp)pChunk)->iLocationtype;
    pCLON->iLocax        = ((mng_clonp)pChunk)->iLocationx;
    pCLON->iLocay        = ((mng_clonp)pChunk)->iLocationy;
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_clon (pData, iSourceid, iCloneid, iClonetype,
                                       bHasdonotshow, iDonotshow, iConcrete,
                                       bHasloca, iLocatype, iLocax, iLocay);
#else
  iRetcode = mng_process_display_clon (pData,
                                       ((mng_clonp)pChunk)->iSourceid,
                                       ((mng_clonp)pChunk)->iCloneid,
                                       ((mng_clonp)pChunk)->iClonetype,
                                       ((mng_clonp)pChunk)->bHasdonotshow,
                                       ((mng_clonp)pChunk)->iDonotshow,
                                       ((mng_clonp)pChunk)->iConcrete,
                                       ((mng_clonp)pChunk)->bHasloca,
                                       ((mng_clonp)pChunk)->iLocationtype,
                                       ((mng_clonp)pChunk)->iLocationx,
                                       ((mng_clonp)pChunk)->iLocationy);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iCLONsourceid     = iSourceid;
  pData->iCLONcloneid      = iCloneid;
  pData->iCLONclonetype    = iClonetype;
  pData->bCLONhasdonotshow = bHasdonotshow;
  pData->iCLONdonotshow    = iDonotshow;
  pData->iCLONconcrete     = iConcrete;
  pData->bCLONhasloca      = bHasloca;
  pData->iCLONlocationtype = iLocatype;
  pData->iCLONlocationx    = iLocax;
  pData->iCLONlocationy    = iLocay;
#else
  pData->iCLONsourceid     = ((mng_clonp)pChunk)->iSourceid;
  pData->iCLONcloneid      = ((mng_clonp)pChunk)->iCloneid;
  pData->iCLONclonetype    = ((mng_clonp)pChunk)->iClonetype;
  pData->bCLONhasdonotshow = ((mng_clonp)pChunk)->bHasdonotshow;
  pData->iCLONdonotshow    = ((mng_clonp)pChunk)->iDonotshow;
  pData->iCLONconcrete     = ((mng_clonp)pChunk)->iConcrete;
  pData->bCLONhasloca      = ((mng_clonp)pChunk)->bHasloca;
  pData->iCLONlocationtype = ((mng_clonp)pChunk)->iLocationtype;
  pData->iCLONlocationx    = ((mng_clonp)pChunk)->iLocationx;
  pData->iCLONlocationy    = ((mng_clonp)pChunk)->iLocationy;
#endif

  iRetcode = mng_process_display_clon (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_CLON, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_clon (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_CLON, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_clon));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_CLON, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_clon (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_clonp pCLON = (mng_ani_clonp)pObject;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_CLON, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_clon (pData, pCLON->iSourceid, pCLON->iCloneid,
                                       pCLON->iClonetype, pCLON->bHasdonotshow,
                                       pCLON->iDonotshow, pCLON->iConcrete,
                                       pCLON->bHasloca, pCLON->iLocatype,
                                       pCLON->iLocax, pCLON->iLocay);
#else
  pData->iCLONcloneid      = pCLON->iCloneid;
  pData->iCLONsourceid     = pCLON->iSourceid;
  pData->iCLONclonetype    = pCLON->iClonetype;
  pData->bCLONhasdonotshow = pCLON->bHasdonotshow;
  pData->iCLONdonotshow    = pCLON->iDonotshow;
  pData->iCLONconcrete     = pCLON->iConcrete;
  pData->bCLONhasloca      = pCLON->bHasloca;
  pData->iCLONlocationtype = pCLON->iLocatype;
  pData->iCLONlocationx    = pCLON->iLocax;
  pData->iCLONlocationy    = pCLON->iLocay;

  iRetcode = mng_process_display_clon (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_CLON, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BACK
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_back (mng_datap  pData,
                                 mng_uint16 iRed,
                                 mng_uint16 iGreen,
                                 mng_uint16 iBlue,
                                 mng_uint8  iMandatory,
                                 mng_uint16 iImageid,
                                 mng_uint8  iTile)
#else
mng_retcode mng_create_ani_back (mng_datap  pData)
#endif
{
  mng_ani_backp pBACK;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_BACK, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_back),
                                               mng_free_obj_general,
                                               mng_process_ani_back,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pBACK = (mng_ani_backp)pTemp;
#else
    MNG_ALLOC (pData, pBACK, sizeof (mng_ani_back));

    pBACK->sHeader.fCleanup = mng_free_ani_back;
    pBACK->sHeader.fProcess = mng_process_ani_back;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pBACK);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pBACK->iRed       = iRed;
    pBACK->iGreen     = iGreen;
    pBACK->iBlue      = iBlue;
    pBACK->iMandatory = iMandatory;
    pBACK->iImageid   = iImageid;
    pBACK->iTile      = iTile;
#else
    pBACK->iRed       = pData->iBACKred;      
    pBACK->iGreen     = pData->iBACKgreen;
    pBACK->iBlue      = pData->iBACKblue;
    pBACK->iMandatory = pData->iBACKmandatory;
    pBACK->iImageid   = pData->iBACKimageid;
    pBACK->iTile      = pData->iBACKtile;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_BACK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_back (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_BACK, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_back));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_BACK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_back (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_backp pBACK = (mng_ani_backp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_BACK, MNG_LC_START);
#endif

  pData->iBACKred       = pBACK->iRed;
  pData->iBACKgreen     = pBACK->iGreen;
  pData->iBACKblue      = pBACK->iBlue;
  pData->iBACKmandatory = pBACK->iMandatory;
  pData->iBACKimageid   = pBACK->iImageid;
  pData->iBACKtile      = pBACK->iTile;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_BACK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_fram (mng_datap  pData,
                                 mng_uint8  iFramemode,
                                 mng_uint8  iChangedelay,
                                 mng_uint32 iDelay,
                                 mng_uint8  iChangetimeout,
                                 mng_uint32 iTimeout,
                                 mng_uint8  iChangeclipping,
                                 mng_uint8  iCliptype,
                                 mng_int32  iClipl,
                                 mng_int32  iClipr,
                                 mng_int32  iClipt,
                                 mng_int32  iClipb)
#else
mng_retcode mng_create_ani_fram (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_framp pFRAM;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_FRAM, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_fram),
                                   mng_free_obj_general,
                                   mng_process_ani_fram,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pFRAM = (mng_ani_framp)pTemp;
#else
    MNG_ALLOC (pData, pFRAM, sizeof (mng_ani_fram));

    pFRAM->sHeader.fCleanup = mng_free_ani_fram;
    pFRAM->sHeader.fProcess = mng_process_ani_fram;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pFRAM);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pFRAM->iFramemode      = iFramemode;
    pFRAM->iChangedelay    = iChangedelay;
    pFRAM->iDelay          = iDelay;
    pFRAM->iChangetimeout  = iChangetimeout;
    pFRAM->iTimeout        = iTimeout;
    pFRAM->iChangeclipping = iChangeclipping;
    pFRAM->iCliptype       = iCliptype;
    pFRAM->iClipl          = iClipl;
    pFRAM->iClipr          = iClipr;
    pFRAM->iClipt          = iClipt;
    pFRAM->iClipb          = iClipb;
#else
    pFRAM->iFramemode      = ((mng_framp)pChunk)->iMode;
    pFRAM->iChangedelay    = ((mng_framp)pChunk)->iChangedelay;
    pFRAM->iDelay          = ((mng_framp)pChunk)->iDelay;
    pFRAM->iChangetimeout  = ((mng_framp)pChunk)->iChangetimeout;
    pFRAM->iTimeout        = ((mng_framp)pChunk)->iTimeout;
    pFRAM->iChangeclipping = ((mng_framp)pChunk)->iChangeclipping;
    pFRAM->iCliptype       = ((mng_framp)pChunk)->iBoundarytype;
    pFRAM->iClipl          = ((mng_framp)pChunk)->iBoundaryl;
    pFRAM->iClipr          = ((mng_framp)pChunk)->iBoundaryr;
    pFRAM->iClipt          = ((mng_framp)pChunk)->iBoundaryt;
    pFRAM->iClipb          = ((mng_framp)pChunk)->iBoundaryb;
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_fram (pData, iFramemode,
                                       iChangedelay, iDelay,
                                       iChangetimeout, iTimeout,
                                       iChangeclipping, iCliptype,
                                       iClipl, iClipr,
                                       iClipt, iClipb);
#else
  iRetcode = mng_process_display_fram (pData,
                                       ((mng_framp)pChunk)->iMode,
                                       ((mng_framp)pChunk)->iChangedelay,
                                       ((mng_framp)pChunk)->iDelay,
                                       ((mng_framp)pChunk)->iChangetimeout,
                                       ((mng_framp)pChunk)->iTimeout,
                                       ((mng_framp)pChunk)->iChangeclipping,
                                       ((mng_framp)pChunk)->iBoundarytype,
                                       ((mng_framp)pChunk)->iBoundaryl,
                                       ((mng_framp)pChunk)->iBoundaryr,
                                       ((mng_framp)pChunk)->iBoundaryt,
                                       ((mng_framp)pChunk)->iBoundaryb);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iTempFramemode      = iFramemode;
  pData->iTempChangedelay    = iChangedelay;
  pData->iTempDelay          = iDelay;
  pData->iTempChangetimeout  = iChangetimeout;
  pData->iTempTimeout        = iTimeout;
  pData->iTempChangeclipping = iChangeclipping;
  pData->iTempCliptype       = iCliptype;
  pData->iTempClipl          = iClipl;
  pData->iTempClipr          = iClipr;
  pData->iTempClipt          = iClipt;
  pData->iTempClipb          = iClipb;
#else
  pData->iTempFramemode      = ((mng_framp)pChunk)->iMode;
  pData->iTempChangedelay    = ((mng_framp)pChunk)->iChangedelay;
  pData->iTempDelay          = ((mng_framp)pChunk)->iDelay;
  pData->iTempChangetimeout  = ((mng_framp)pChunk)->iChangetimeout;
  pData->iTempTimeout        = ((mng_framp)pChunk)->iTimeout;
  pData->iTempChangeclipping = ((mng_framp)pChunk)->iChangeclipping;
  pData->iTempCliptype       = ((mng_framp)pChunk)->iBoundarytype;
  pData->iTempClipl          = ((mng_framp)pChunk)->iBoundaryl;
  pData->iTempClipr          = ((mng_framp)pChunk)->iBoundaryr;
  pData->iTempClipt          = ((mng_framp)pChunk)->iBoundaryt;
  pData->iTempClipb          = ((mng_framp)pChunk)->iBoundaryb;
#endif

  iRetcode = mng_process_display_fram (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_FRAM, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_fram (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_FRAM, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_fram));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_FRAM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_fram (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_framp pFRAM = (mng_ani_framp)pObject;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_FRAM, MNG_LC_START);
#endif

  if (pData->iBreakpoint)              /* previously broken ? */
  {
    iRetcode           = mng_process_display_fram2 (pData);
    pData->iBreakpoint = 0;            /* not again */
  }
  else
  {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    iRetcode = mng_process_display_fram (pData, pFRAM->iFramemode,
                                         pFRAM->iChangedelay, pFRAM->iDelay,
                                         pFRAM->iChangetimeout, pFRAM->iTimeout,
                                         pFRAM->iChangeclipping, pFRAM->iCliptype,
                                         pFRAM->iClipl, pFRAM->iClipr,
                                         pFRAM->iClipt, pFRAM->iClipb);
#else
    pData->iTempFramemode      = pFRAM->iFramemode;
    pData->iTempChangedelay    = pFRAM->iChangedelay;
    pData->iTempDelay          = pFRAM->iDelay;
    pData->iTempChangetimeout  = pFRAM->iChangetimeout;
    pData->iTempTimeout        = pFRAM->iTimeout;
    pData->iTempChangeclipping = pFRAM->iChangeclipping;
    pData->iTempCliptype       = pFRAM->iCliptype;
    pData->iTempClipl          = pFRAM->iClipl;
    pData->iTempClipr          = pFRAM->iClipr;
    pData->iTempClipt          = pFRAM->iClipt;
    pData->iTempClipb          = pFRAM->iClipb;

    iRetcode = mng_process_display_fram (pData);
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_FRAM, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MOVE
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_move (mng_datap  pData,
                                 mng_uint16 iFirstid,
                                 mng_uint16 iLastid,
                                 mng_uint8  iType,
                                 mng_int32  iLocax,
                                 mng_int32  iLocay)
#else
mng_retcode mng_create_ani_move (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_movep pMOVE;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_MOVE, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_move),
                                   mng_free_obj_general,
                                   mng_process_ani_move,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pMOVE = (mng_ani_movep)pTemp;
#else
    MNG_ALLOC (pData, pMOVE, sizeof (mng_ani_move));

    pMOVE->sHeader.fCleanup = mng_free_ani_move;
    pMOVE->sHeader.fProcess = mng_process_ani_move;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pMOVE);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pMOVE->iFirstid = iFirstid;
    pMOVE->iLastid  = iLastid;
    pMOVE->iType    = iType;
    pMOVE->iLocax   = iLocax;
    pMOVE->iLocay   = iLocay;
#else
    pMOVE->iFirstid = ((mng_movep)pChunk)->iFirstid;
    pMOVE->iLastid  = ((mng_movep)pChunk)->iLastid;
    pMOVE->iType    = ((mng_movep)pChunk)->iMovetype;
    pMOVE->iLocax   = ((mng_movep)pChunk)->iMovex;
    pMOVE->iLocay   = ((mng_movep)pChunk)->iMovey;
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_move (pData, iFirstid, iLastid,
                                       iType, iLocax, iLocay);
#else
  iRetcode = mng_process_display_move (pData,
                                       ((mng_movep)pChunk)->iFirstid,
                                       ((mng_movep)pChunk)->iLastid,
                                       ((mng_movep)pChunk)->iMovetype,
                                       ((mng_movep)pChunk)->iMovex,
                                       ((mng_movep)pChunk)->iMovey);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iMOVEfromid   = iFirstid;
  pData->iMOVEtoid     = iLastid;
  pData->iMOVEmovetype = iType;
  pData->iMOVEmovex    = iLocax;
  pData->iMOVEmovey    = iLocay;
#else
  pData->iMOVEfromid   = ((mng_movep)pChunk)->iFirstid;
  pData->iMOVEtoid     = ((mng_movep)pChunk)->iLastid;
  pData->iMOVEmovetype = ((mng_movep)pChunk)->iMovetype;
  pData->iMOVEmovex    = ((mng_movep)pChunk)->iMovex;
  pData->iMOVEmovey    = ((mng_movep)pChunk)->iMovey;
#endif

  iRetcode = mng_process_display_move (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_MOVE, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_move (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_MOVE, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_move));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_MOVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_move (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_retcode   iRetcode;
  mng_ani_movep pMOVE = (mng_ani_movep)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_MOVE, MNG_LC_START);
#endif
                                       /* re-process the MOVE chunk */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_move (pData, pMOVE->iFirstid, pMOVE->iLastid,
                                       pMOVE->iType, pMOVE->iLocax, pMOVE->iLocay);
#else
  pData->iMOVEfromid   = pMOVE->iFirstid;
  pData->iMOVEtoid     = pMOVE->iLastid;
  pData->iMOVEmovetype = pMOVE->iType;
  pData->iMOVEmovex    = pMOVE->iLocax;
  pData->iMOVEmovey    = pMOVE->iLocay;

  iRetcode = mng_process_display_move (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_MOVE, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLIP
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_clip (mng_datap  pData,
                                 mng_uint16 iFirstid,
                                 mng_uint16 iLastid,
                                 mng_uint8  iType,
                                 mng_int32  iClipl,
                                 mng_int32  iClipr,
                                 mng_int32  iClipt,
                                 mng_int32  iClipb)
#else
mng_retcode mng_create_ani_clip (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_clipp pCLIP;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_CLIP, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_clip),
                                   mng_free_obj_general,
                                   mng_process_ani_clip,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pCLIP = (mng_ani_clipp)pTemp;
#else
    MNG_ALLOC (pData, pCLIP, sizeof (mng_ani_clip));

    pCLIP->sHeader.fCleanup = mng_free_ani_clip;
    pCLIP->sHeader.fProcess = mng_process_ani_clip;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pCLIP);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pCLIP->iFirstid = iFirstid;
    pCLIP->iLastid  = iLastid;
    pCLIP->iType    = iType;
    pCLIP->iClipl   = iClipl;
    pCLIP->iClipr   = iClipr;
    pCLIP->iClipt   = iClipt;
    pCLIP->iClipb   = iClipb;
#else
    pCLIP->iFirstid = ((mng_clipp)pChunk)->iFirstid;
    pCLIP->iLastid  = ((mng_clipp)pChunk)->iLastid;
    pCLIP->iType    = ((mng_clipp)pChunk)->iCliptype;
    pCLIP->iClipl   = ((mng_clipp)pChunk)->iClipl;
    pCLIP->iClipr   = ((mng_clipp)pChunk)->iClipr;
    pCLIP->iClipt   = ((mng_clipp)pChunk)->iClipt;
    pCLIP->iClipb   = ((mng_clipp)pChunk)->iClipb;
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_clip (pData, iFirstid, iLastid,
                                       iType, iClipl, iClipr,
                                       iClipt, iClipb);
#else
  iRetcode = mng_process_display_clip (pData,
                                       ((mng_clipp)pChunk)->iFirstid,
                                       ((mng_clipp)pChunk)->iLastid,
                                       ((mng_clipp)pChunk)->iCliptype,
                                       ((mng_clipp)pChunk)->iClipl,
                                       ((mng_clipp)pChunk)->iClipr,
                                       ((mng_clipp)pChunk)->iClipt,
                                       ((mng_clipp)pChunk)->iClipb);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iCLIPfromid   = iFirstid;
  pData->iCLIPtoid     = iLastid;
  pData->iCLIPcliptype = iType;
  pData->iCLIPclipl    = iClipl;
  pData->iCLIPclipr    = iClipr;
  pData->iCLIPclipt    = iClipt;
  pData->iCLIPclipb    = iClipb;
#else
  pData->iCLIPfromid   = ((mng_clipp)pChunk)->iFirstid;
  pData->iCLIPtoid     = ((mng_clipp)pChunk)->iLastid;
  pData->iCLIPcliptype = ((mng_clipp)pChunk)->iCliptype;
  pData->iCLIPclipl    = ((mng_clipp)pChunk)->iClipl;
  pData->iCLIPclipr    = ((mng_clipp)pChunk)->iClipr;
  pData->iCLIPclipt    = ((mng_clipp)pChunk)->iClipt;
  pData->iCLIPclipb    = ((mng_clipp)pChunk)->iClipb;
#endif

  iRetcode = mng_process_display_clip (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_CLIP, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_clip (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_CLIP, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_clip));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_CLIP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_clip (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_retcode   iRetcode;
  mng_ani_clipp pCLIP = (mng_ani_clipp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_CLIP, MNG_LC_START);
#endif
                                       /* re-process the CLIP chunk */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_clip (pData, pCLIP->iFirstid, pCLIP->iLastid,
                                       pCLIP->iType, pCLIP->iClipl, pCLIP->iClipr,
                                       pCLIP->iClipt, pCLIP->iClipb);
#else
  pData->iCLIPfromid   = pCLIP->iFirstid;
  pData->iCLIPtoid     = pCLIP->iLastid;
  pData->iCLIPcliptype = pCLIP->iType;
  pData->iCLIPclipl    = pCLIP->iClipl;
  pData->iCLIPclipr    = pCLIP->iClipr;
  pData->iCLIPclipt    = pCLIP->iClipt;
  pData->iCLIPclipb    = pCLIP->iClipb;

  iRetcode = mng_process_display_clip (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_CLIP, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SHOW
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_show (mng_datap  pData,
                                 mng_uint16 iFirstid,
                                 mng_uint16 iLastid,
                                 mng_uint8  iMode)
#else
mng_retcode mng_create_ani_show (mng_datap  pData)
#endif
{
  mng_ani_showp pSHOW;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_SHOW, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_show),
                                               mng_free_obj_general,
                                               mng_process_ani_show,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pSHOW = (mng_ani_showp)pTemp;
#else
    MNG_ALLOC (pData, pSHOW, sizeof (mng_ani_show));

    pSHOW->sHeader.fCleanup = mng_free_ani_show;
    pSHOW->sHeader.fProcess = mng_process_ani_show;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pSHOW);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pSHOW->iFirstid = iFirstid;
    pSHOW->iLastid  = iLastid;
    pSHOW->iMode    = iMode;
#else
    pSHOW->iFirstid = pData->iSHOWfromid;
    pSHOW->iLastid  = pData->iSHOWtoid;
    pSHOW->iMode    = pData->iSHOWmode;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_SHOW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_show (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_SHOW, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_show));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_SHOW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_show (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_retcode   iRetcode;
  mng_ani_showp pSHOW = (mng_ani_showp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_SHOW, MNG_LC_START);
#endif

  if (pData->iBreakpoint)              /* returning from breakpoint ? */
  {
    iRetcode           = mng_process_display_show (pData);
  }
  else
  {                                    /* "re-run" SHOW chunk */
    pData->iSHOWmode   = pSHOW->iMode;
    pData->iSHOWfromid = pSHOW->iFirstid;
    pData->iSHOWtoid   = pSHOW->iLastid;

    iRetcode           = mng_process_display_show (pData);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_SHOW, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_TERM
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_term (mng_datap  pData,
                                 mng_uint8  iTermaction,
                                 mng_uint8  iIteraction,
                                 mng_uint32 iDelay,
                                 mng_uint32 iItermax)
#else
mng_retcode mng_create_ani_term (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_termp pTERM;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_TERM, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_term),
                                               mng_free_obj_general,
                                               mng_process_ani_term,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pTERM = (mng_ani_termp)pTemp;
#else
    MNG_ALLOC (pData, pTERM, sizeof (mng_ani_term));

    pTERM->sHeader.fCleanup = mng_free_ani_term;
    pTERM->sHeader.fProcess = mng_process_ani_term;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pTERM);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pTERM->iTermaction = iTermaction;
    pTERM->iIteraction = iIteraction;
    pTERM->iDelay      = iDelay;
    pTERM->iItermax    = iItermax;
#else
    pTERM->iTermaction = ((mng_termp)pChunk)->iTermaction;
    pTERM->iIteraction = ((mng_termp)pChunk)->iIteraction;
    pTERM->iDelay      = ((mng_termp)pChunk)->iDelay;
    pTERM->iItermax    = ((mng_termp)pChunk)->iItermax;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_TERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_term (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_TERM, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_term));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_TERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_term (mng_datap   pData,
                                  mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_TERM, MNG_LC_START);
#endif

  /* dummy: no action required! */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_TERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_create_ani_save (mng_datap pData)
{
//  mng_ptr       pTemp;
  mng_ani_savep pSAVE;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_SAVE, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_save),
                                               mng_free_obj_general,
                                               mng_process_ani_save,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pSAVE = (mng_ani_savep)pTemp;
#else
    MNG_ALLOC (pData, pSAVE, sizeof (mng_ani_save));

    pSAVE->sHeader.fCleanup = mng_free_ani_save;
    pSAVE->sHeader.fProcess = mng_process_ani_save;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pSAVE);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_SAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_save (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_SAVE, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_save));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_SAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_save (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_SAVE, MNG_LC_START);
#endif

  iRetcode = mng_process_display_save (pData);

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_SAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_seek (mng_datap  pData,
                                 mng_uint32 iSegmentnamesize,
                                 mng_pchar  zSegmentname)
#else
mng_retcode mng_create_ani_seek (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
//  mng_ptr       pTemp;
  mng_ani_seekp pSEEK;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_SEEK, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_seek),
                                               mng_free_ani_seek,
                                               mng_process_ani_seek,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pSEEK = (mng_ani_seekp)pTemp;
#else
    MNG_ALLOC (pData, pSEEK, sizeof (mng_ani_seek));

    pSEEK->sHeader.fCleanup = mng_free_ani_seek;
    pSEEK->sHeader.fProcess = mng_process_ani_seek;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pSEEK);

    pData->pLastseek = (mng_objectp)pSEEK;

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pSEEK->iSegmentnamesize = iSegmentnamesize;
    if (iSegmentnamesize)
    {
      MNG_ALLOC (pData, pSEEK->zSegmentname, iSegmentnamesize + 1);
      MNG_COPY (pSEEK->zSegmentname, zSegmentname, iSegmentnamesize);
    }
#else
    pSEEK->iSegmentnamesize = ((mng_seekp)pChunk)->iNamesize;
    if (pSEEK->iSegmentnamesize)
    {
      MNG_ALLOC (pData, pSEEK->zSegmentname, pSEEK->iSegmentnamesize + 1);
      MNG_COPY (pSEEK->zSegmentname, ((mng_seekp)pChunk)->zName, pSEEK->iSegmentnamesize);
    }
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_SEEK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_free_ani_seek (mng_datap   pData,
                               mng_objectp pObject)
{
  mng_ani_seekp pSEEK = (mng_ani_seekp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_SEEK, MNG_LC_START);
#endif

  if (pSEEK->iSegmentnamesize)
    MNG_FREEX (pData, pSEEK->zSegmentname, pSEEK->iSegmentnamesize + 1);

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  MNG_FREEX (pData, pObject, sizeof (mng_ani_seek));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_SEEK, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  return MNG_NOERROR;
#else
  return mng_free_obj_general(pData, pObject);
#endif
}

/* ************************************************************************** */

mng_retcode mng_process_ani_seek (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_seekp pSEEK = (mng_ani_seekp)pObject;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_SEEK, MNG_LC_START);
#endif

#ifdef MNG_SUPPORT_DYNAMICMNG
  if (!pData->bStopafterseek)          /* can we really process this one ? */
#endif  
  {
    pData->pLastseek = pObject;

    if (pData->fProcessseek)           /* inform the app ? */
    {
      mng_bool  bOke;
      mng_pchar zName;

      MNG_ALLOC (pData, zName, pSEEK->iSegmentnamesize + 1);

      if (pSEEK->iSegmentnamesize)
        MNG_COPY (zName, pSEEK->zSegmentname, pSEEK->iSegmentnamesize);

      bOke = pData->fProcessseek ((mng_handle)pData, zName);

      MNG_FREEX (pData, zName, pSEEK->iSegmentnamesize + 1);

      if (!bOke)
        MNG_ERROR (pData, MNG_APPMISCERROR);
    }
  }

  iRetcode = mng_process_display_seek (pData);

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_SEEK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_dhdr (mng_datap  pData,
                                 mng_uint16 iObjectid,
                                 mng_uint8  iImagetype,
                                 mng_uint8  iDeltatype,
                                 mng_uint32 iBlockwidth,
                                 mng_uint32 iBlockheight,
                                 mng_uint32 iBlockx,
                                 mng_uint32 iBlocky)
#else
mng_retcode mng_create_ani_dhdr (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_dhdrp pDHDR;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_DHDR, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_dhdr),
                                   mng_free_obj_general,
                                   mng_process_ani_dhdr,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pDHDR = (mng_ani_dhdrp)pTemp;
#else
    MNG_ALLOC (pData, pDHDR, sizeof (mng_ani_dhdr));

    pDHDR->sHeader.fCleanup = mng_free_ani_dhdr;
    pDHDR->sHeader.fProcess = mng_process_ani_dhdr;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pDHDR);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pDHDR->iObjectid    = iObjectid;
    pDHDR->iImagetype   = iImagetype;
    pDHDR->iDeltatype   = iDeltatype;
    pDHDR->iBlockwidth  = iBlockwidth;
    pDHDR->iBlockheight = iBlockheight;
    pDHDR->iBlockx      = iBlockx;
    pDHDR->iBlocky      = iBlocky;
#else
    pDHDR->iObjectid    = ((mng_dhdrp)pChunk)->iObjectid;
    pDHDR->iImagetype   = ((mng_dhdrp)pChunk)->iImagetype;
    pDHDR->iDeltatype   = ((mng_dhdrp)pChunk)->iDeltatype;
    pDHDR->iBlockwidth  = ((mng_dhdrp)pChunk)->iBlockwidth;
    pDHDR->iBlockheight = ((mng_dhdrp)pChunk)->iBlockheight;
    pDHDR->iBlockx      = ((mng_dhdrp)pChunk)->iBlockx;
    pDHDR->iBlocky      = ((mng_dhdrp)pChunk)->iBlocky;
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_dhdr (pData, iObjectid,
                                       iImagetype, iDeltatype,
                                       iBlockwidth, iBlockheight,
                                       iBlockx, iBlocky);
#else
  iRetcode = mng_process_display_dhdr (pData,
                                       ((mng_dhdrp)pChunk)->iObjectid,
                                       ((mng_dhdrp)pChunk)->iImagetype,
                                       ((mng_dhdrp)pChunk)->iDeltatype,
                                       ((mng_dhdrp)pChunk)->iBlockwidth,
                                       ((mng_dhdrp)pChunk)->iBlockheight,
                                       ((mng_dhdrp)pChunk)->iBlockx,
                                       ((mng_dhdrp)pChunk)->iBlocky);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iDHDRobjectid    = iObjectid;
  pData->iDHDRimagetype   = iImagetype;
  pData->iDHDRdeltatype   = iDeltatype;
  pData->iDHDRblockwidth  = iBlockwidth;
  pData->iDHDRblockheight = iBlockheight;
  pData->iDHDRblockx      = iBlockx;
  pData->iDHDRblocky      = iBlocky;
#else
  pData->iDHDRobjectid    = ((mng_dhdrp)pChunk)->iObjectid;
  pData->iDHDRimagetype   = ((mng_dhdrp)pChunk)->iImagetype;
  pData->iDHDRdeltatype   = ((mng_dhdrp)pChunk)->iDeltatype;
  pData->iDHDRblockwidth  = ((mng_dhdrp)pChunk)->iBlockwidth;
  pData->iDHDRblockheight = ((mng_dhdrp)pChunk)->iBlockheight;
  pData->iDHDRblockx      = ((mng_dhdrp)pChunk)->iBlockx;
  pData->iDHDRblocky      = ((mng_dhdrp)pChunk)->iBlocky;
#endif

  iRetcode = mng_process_display_dhdr (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_DHDR, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_dhdr (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_DHDR, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_dhdr));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_DHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_dhdr (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_dhdrp pDHDR = (mng_ani_dhdrp)pObject;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_DHDR, MNG_LC_START);
#endif

  pData->bHasDHDR = MNG_TRUE;          /* let everyone know we're inside a DHDR */

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_dhdr (pData, pDHDR->iObjectid,
                                       pDHDR->iImagetype, pDHDR->iDeltatype,
                                       pDHDR->iBlockwidth, pDHDR->iBlockheight,
                                       pDHDR->iBlockx, pDHDR->iBlocky);
#else
  pData->iDHDRobjectid    = pDHDR->iObjectid;
  pData->iDHDRimagetype   = pDHDR->iImagetype;
  pData->iDHDRdeltatype   = pDHDR->iDeltatype;
  pData->iDHDRblockwidth  = pDHDR->iBlockwidth;
  pData->iDHDRblockheight = pDHDR->iBlockheight;
  pData->iDHDRblockx      = pDHDR->iBlockx;
  pData->iDHDRblocky      = pDHDR->iBlocky;

  iRetcode = mng_process_display_dhdr (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_DHDR, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_prom (mng_datap pData,
                                 mng_uint8 iBitdepth,
                                 mng_uint8 iColortype,
                                 mng_uint8 iFilltype)
#else
mng_retcode mng_create_ani_prom (mng_datap pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_promp pPROM=NULL;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_PROM, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_prom),
                                   mng_free_obj_general,
                                   mng_process_ani_prom,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pPROM = (mng_ani_promp)pTemp;
#else
    MNG_ALLOC (pData, pPROM, sizeof (mng_ani_prom));

    pPROM->sHeader.fCleanup = mng_free_ani_prom;
    pPROM->sHeader.fProcess = mng_process_ani_prom;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pPROM);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pPROM->iBitdepth  = iBitdepth;
    pPROM->iColortype = iColortype;
    pPROM->iFilltype  = iFilltype;
#else
    pPROM->iBitdepth  = ((mng_promp)pChunk)->iSampledepth;
    pPROM->iColortype = ((mng_promp)pChunk)->iColortype;
    pPROM->iFilltype  = ((mng_promp)pChunk)->iFilltype;
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_prom (pData, iBitdepth,
                                       iColortype, iFilltype);
#else
  iRetcode = mng_process_display_prom (pData,
                                       ((mng_promp)pChunk)->iSampledepth,
                                       ((mng_promp)pChunk)->iColortype,
                                       ((mng_promp)pChunk)->iFilltype);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iPROMbitdepth  = iBitdepth;
  pData->iPROMcolortype = iColortype;
  pData->iPROMfilltype  = iFilltype;
#else
  pData->iPROMbitdepth  = ((mng_promp)pChunk)->iSampledepth;
  pData->iPROMcolortype = ((mng_promp)pChunk)->iColortype;
  pData->iPROMfilltype  = ((mng_promp)pChunk)->iFilltype;
#endif

  iRetcode = mng_process_display_prom (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_PROM, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_prom (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_PROM, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_prom));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_PROM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_prom (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_promp pPROM = (mng_ani_promp)pObject;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_PROM, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_prom (pData, pPROM->iBitdepth,
                                       pPROM->iColortype, pPROM->iFilltype);
#else
  pData->iPROMbitdepth  = pPROM->iBitdepth;
  pData->iPROMcolortype = pPROM->iColortype;
  pData->iPROMfilltype  = pPROM->iFilltype;

  iRetcode = mng_process_display_prom (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_PROM, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_create_ani_ipng (mng_datap pData)
{
  mng_ani_ipngp pIPNG;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_IPNG, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_ipng),
                                               mng_free_obj_general,
                                               mng_process_ani_ipng,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pIPNG = (mng_ani_ipngp)pTemp;
#else
    MNG_ALLOC (pData, pIPNG, sizeof (mng_ani_ipng));

    pIPNG->sHeader.fCleanup = mng_free_ani_ipng;
    pIPNG->sHeader.fProcess = mng_process_ani_ipng;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pIPNG);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_IPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_ipng (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_IPNG, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_ipng));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_IPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_ipng (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_IPNG, MNG_LC_START);
#endif

  iRetcode = mng_process_display_ipng (pData);

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_IPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
mng_retcode mng_create_ani_ijng (mng_datap pData)
{
  mng_ani_ijngp pIJNG;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_IJNG, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_ani_ijng),
                                               mng_free_obj_general,
                                               mng_process_ani_ijng,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pIJNG = (mng_ani_ijngp)pTemp;
#else
    MNG_ALLOC (pData, pIJNG, sizeof (mng_ani_ijng));

    pIJNG->sHeader.fCleanup = mng_free_ani_ijng;
    pIJNG->sHeader.fProcess = mng_process_ani_ijng;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pIJNG);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_IJNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_ijng (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_IJNG, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_ijng));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_IJNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_ijng (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_IJNG, MNG_LC_START);
#endif

  iRetcode = mng_process_display_ijng (pData);

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_IJNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_create_ani_pplt (mng_datap      pData,
                                 mng_uint8      iType,
                                 mng_uint32     iCount,
                                 mng_palette8ep paIndexentries,
                                 mng_uint8p     paAlphaentries,
                                 mng_uint8p     paUsedentries)
{
  mng_ani_ppltp pPPLT;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_PPLT, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_pplt),
                                   mng_free_obj_general,
                                   mng_process_ani_pplt,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pPPLT = (mng_ani_ppltp)pTemp;
#else
    MNG_ALLOC (pData, pPPLT, sizeof (mng_ani_pplt));

    pPPLT->sHeader.fCleanup = mng_free_ani_pplt;
    pPPLT->sHeader.fProcess = mng_process_ani_pplt;
#endif

    pPPLT->iType            = iType;
    pPPLT->iCount           = iCount;

    MNG_COPY (pPPLT->aIndexentries, paIndexentries, sizeof (pPPLT->aIndexentries));
    MNG_COPY (pPPLT->aAlphaentries, paAlphaentries, sizeof (pPPLT->aAlphaentries));
    MNG_COPY (pPPLT->aUsedentries,  paUsedentries,  sizeof (pPPLT->aUsedentries ));

    mng_add_ani_object (pData, (mng_object_headerp)pPPLT);
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_pplt (pData, iType, iCount,
                                       paIndexentries, paAlphaentries, paUsedentries);
#else
  pData->iPPLTtype          = iType;
  pData->iPPLTcount         = iCount;
  pData->paPPLTindexentries = paIndexentries;
  pData->paPPLTalphaentries = paAlphaentries;
  pData->paPPLTusedentries  = paUsedentries;

  iRetcode = mng_process_display_pplt (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_PPLT, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_pplt (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_PPLT, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_pplt));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_PPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_pplt (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_ppltp pPPLT = (mng_ani_ppltp)pObject;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_PPLT, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_pplt (pData, pPPLT->iType, pPPLT->iCount,
                                       pPPLT->aIndexentries, pPPLT->aAlphaentries,
                                       pPPLT->aUsedentries);
#else
  pData->iPPLTtype          = pPPLT->iType;
  pData->iPPLTcount         = pPPLT->iCount;
  pData->paPPLTindexentries = &pPPLT->aIndexentries;
  pData->paPPLTalphaentries = &pPPLT->aAlphaentries;
  pData->paPPLTusedentries  = &pPPLT->aUsedentries;

  iRetcode = mng_process_display_pplt (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_PPLT, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_magn (mng_datap  pData,
                                 mng_uint16 iFirstid,
                                 mng_uint16 iLastid,
                                 mng_uint8  iMethodX,
                                 mng_uint16 iMX,
                                 mng_uint16 iMY,
                                 mng_uint16 iML,
                                 mng_uint16 iMR,
                                 mng_uint16 iMT,
                                 mng_uint16 iMB,
                                 mng_uint8  iMethodY)
#else
mng_retcode mng_create_ani_magn (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_magnp pMAGN=NULL;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_MAGN, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_magn),
                                   mng_free_obj_general,
                                   mng_process_ani_magn,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pMAGN = (mng_ani_magnp)pTemp;
#else
    MNG_ALLOC (pData, pMAGN, sizeof (mng_ani_magn));

    pMAGN->sHeader.fCleanup = mng_free_ani_magn;
    pMAGN->sHeader.fProcess = mng_process_ani_magn;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pMAGN);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pMAGN->iFirstid = iFirstid;
    pMAGN->iLastid  = iLastid;
    pMAGN->iMethodX = iMethodX;
    pMAGN->iMX      = iMX;
    pMAGN->iMY      = iMY;
    pMAGN->iML      = iML;
    pMAGN->iMR      = iMR;
    pMAGN->iMT      = iMT;
    pMAGN->iMB      = iMB;
    pMAGN->iMethodY = iMethodY;
#else
    pMAGN->iFirstid = ((mng_magnp)pChunk)->iFirstid;
    pMAGN->iLastid  = ((mng_magnp)pChunk)->iLastid;
    pMAGN->iMethodX = ((mng_magnp)pChunk)->iMethodX;
    pMAGN->iMX      = ((mng_magnp)pChunk)->iMX;
    pMAGN->iMY      = ((mng_magnp)pChunk)->iMY;
    pMAGN->iML      = ((mng_magnp)pChunk)->iML;
    pMAGN->iMR      = ((mng_magnp)pChunk)->iMR;
    pMAGN->iMT      = ((mng_magnp)pChunk)->iMT;
    pMAGN->iMB      = ((mng_magnp)pChunk)->iMB;
    pMAGN->iMethodY = ((mng_magnp)pChunk)->iMethodY;
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_magn (pData, pMAGN->iFirstid, pMAGN->iLastid,
                                       pMAGN->iMethodX, pMAGN->iMX, pMAGN->iMY,
                                       pMAGN->iML, pMAGN->iMR, pMAGN->iMT,
                                       pMAGN->iMB, pMAGN->iMethodY);
#else
  iRetcode = mng_process_display_magn (pData,
                                       ((mng_magnp)pChunk)->iFirstid,
                                       ((mng_magnp)pChunk)->iLastid,
                                       ((mng_magnp)pChunk)->iMethodX,
                                       ((mng_magnp)pChunk)->iMX,
                                       ((mng_magnp)pChunk)->iMY,
                                       ((mng_magnp)pChunk)->iML,
                                       ((mng_magnp)pChunk)->iMR,
                                       ((mng_magnp)pChunk)->iMT,
                                       ((mng_magnp)pChunk)->iMB,
                                       ((mng_magnp)pChunk)->iMethodY);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iMAGNfirstid = iFirstid;
  pData->iMAGNlastid  = iLastid;
  pData->iMAGNmethodX = iMethodX;
  pData->iMAGNmX      = iMX;
  pData->iMAGNmY      = iMY;
  pData->iMAGNmL      = iML;
  pData->iMAGNmR      = iMR;
  pData->iMAGNmT      = iMT;
  pData->iMAGNmB      = iMB;
  pData->iMAGNmethodY = iMethodY;
#else
  pData->iMAGNfirstid = ((mng_magnp)pChunk)->iFirstid;
  pData->iMAGNlastid  = ((mng_magnp)pChunk)->iLastid;
  pData->iMAGNmethodX = ((mng_magnp)pChunk)->iMethodX;
  pData->iMAGNmX      = ((mng_magnp)pChunk)->iMX;
  pData->iMAGNmY      = ((mng_magnp)pChunk)->iMY;
  pData->iMAGNmL      = ((mng_magnp)pChunk)->iML;
  pData->iMAGNmR      = ((mng_magnp)pChunk)->iMR;
  pData->iMAGNmT      = ((mng_magnp)pChunk)->iMT;
  pData->iMAGNmB      = ((mng_magnp)pChunk)->iMB;
  pData->iMAGNmethodY = ((mng_magnp)pChunk)->iMethodY;
#endif

  iRetcode = mng_process_display_magn (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_MAGN, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_OBJCLEANUP
mng_retcode mng_free_ani_magn (mng_datap   pData,
                               mng_objectp pObject)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_MAGN, MNG_LC_START);
#endif

  MNG_FREEX (pData, pObject, sizeof (mng_ani_magn));

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_MAGN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_process_ani_magn (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_magnp pMAGN = (mng_ani_magnp)pObject;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_MAGN, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_magn (pData, pMAGN->iFirstid, pMAGN->iLastid,
                                       pMAGN->iMethodX, pMAGN->iMX, pMAGN->iMY,
                                       pMAGN->iML, pMAGN->iMR, pMAGN->iMT,
                                       pMAGN->iMB, pMAGN->iMethodY);
#else
  pData->iMAGNfirstid = pMAGN->iFirstid;
  pData->iMAGNlastid  = pMAGN->iLastid;
  pData->iMAGNmethodX = pMAGN->iMethodX;
  pData->iMAGNmX      = pMAGN->iMX;
  pData->iMAGNmY      = pMAGN->iMY;
  pData->iMAGNmL      = pMAGN->iML;
  pData->iMAGNmR      = pMAGN->iMR;
  pData->iMAGNmT      = pMAGN->iMT;
  pData->iMAGNmB      = pMAGN->iMB;
  pData->iMAGNmethodY = pMAGN->iMethodY;

  iRetcode = mng_process_display_magn (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_MAGN, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_past (mng_datap  pData,
                                 mng_uint16 iTargetid,
                                 mng_uint8  iTargettype,
                                 mng_int32  iTargetx,
                                 mng_int32  iTargety,
                                 mng_uint32 iCount,
                                 mng_ptr    pSources)
#else
mng_retcode mng_create_ani_past (mng_datap  pData,
                                 mng_chunkp pChunk)
#endif
{
  mng_ani_pastp pPAST;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_PAST, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_past),
                                   mng_free_ani_past,
                                   mng_process_ani_past,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pPAST = (mng_ani_pastp)pTemp;
#else
    MNG_ALLOC (pData, pPAST, sizeof (mng_ani_past));

    pPAST->sHeader.fCleanup = mng_free_ani_past;
    pPAST->sHeader.fProcess = mng_process_ani_past;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pPAST);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pPAST->iTargetid   = iTargetid;
    pPAST->iTargettype = iTargettype;
    pPAST->iTargetx    = iTargetx;
    pPAST->iTargety    = iTargety;
    pPAST->iCount      = iCount;

    if (iCount)
    {
      MNG_ALLOC (pData, pPAST->pSources, (iCount * sizeof (mng_past_source)));
      MNG_COPY (pPAST->pSources, pSources, (iCount * sizeof (mng_past_source)));
    }
#else
    pPAST->iTargetid   = ((mng_pastp)pChunk)->iDestid;
    pPAST->iTargettype = ((mng_pastp)pChunk)->iTargettype;
    pPAST->iTargetx    = ((mng_pastp)pChunk)->iTargetx;
    pPAST->iTargety    = ((mng_pastp)pChunk)->iTargety;
    pPAST->iCount      = ((mng_pastp)pChunk)->iCount;

    if (pPAST->iCount)
    {
      mng_size_t iSize = (mng_size_t)(pPAST->iCount * sizeof (mng_past_source));
      MNG_ALLOC (pData, pPAST->pSources, iSize);
      MNG_COPY (pPAST->pSources, ((mng_pastp)pChunk)->pSources, iSize);
    }
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_past (pData, iTargetid, iTargettype,
                                       iTargetx, iTargety,
                                       iCount, pSources);
#else
  iRetcode = mng_process_display_past (pData,
                                       ((mng_pastp)pChunk)->iDestid,
                                       ((mng_pastp)pChunk)->iTargettype,
                                       ((mng_pastp)pChunk)->iTargetx,
                                       ((mng_pastp)pChunk)->iTargety,
                                       ((mng_pastp)pChunk)->iCount,
                                       ((mng_pastp)pChunk)->pSources);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iPASTtargetid   = iTargetid;
  pData->iPASTtargettype = iTargettype;
  pData->iPASTtargetx    = iTargetx;
  pData->iPASTtargety    = iTargety;
  pData->iPASTcount      = iCount;
  pData->pPASTsources    = pSources;
#else
  pData->iPASTtargetid   = ((mng_pastp)pChunk)->iDestid;
  pData->iPASTtargettype = ((mng_pastp)pChunk)->iTargettype;
  pData->iPASTtargetx    = ((mng_pastp)pChunk)->iTargetx;
  pData->iPASTtargety    = ((mng_pastp)pChunk)->iTargety;
  pData->iPASTcount      = ((mng_pastp)pChunk)->iCount;
  pData->pPASTsources    = ((mng_pastp)pChunk)->pSources;
#endif

  iRetcode = mng_process_display_past (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_PAST, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_free_ani_past (mng_datap   pData,
                               mng_objectp pObject)
{
  mng_ani_pastp pPAST = (mng_ani_pastp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_PAST, MNG_LC_START);
#endif

  if (pPAST->iCount)
    MNG_FREEX (pData, pPAST->pSources, (pPAST->iCount * sizeof (mng_past_source)));

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  MNG_FREEX (pData, pObject, sizeof (mng_ani_past));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_PAST, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  return MNG_NOERROR;
#else
  return mng_free_obj_general(pData, pObject);
#endif
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_process_ani_past (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_pastp pPAST = (mng_ani_pastp)pObject;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_PAST, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_past (pData, pPAST->iTargetid, pPAST->iTargettype,
                                       pPAST->iTargetx, pPAST->iTargety,
                                       pPAST->iCount, pPAST->pSources);
#else
  pData->iPASTtargetid   = pPAST->iTargetid;
  pData->iPASTtargettype = pPAST->iTargettype;
  pData->iPASTtargetx    = pPAST->iTargetx;
  pData->iPASTtargety    = pPAST->iTargety;
  pData->iPASTcount      = pPAST->iCount;
  pData->pPASTsources    = pPAST->pSources;

  iRetcode = mng_process_display_past (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_PAST, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ani_disc (mng_datap   pData,
                                 mng_uint32  iCount,
                                 mng_uint16p pIds)
#else
mng_retcode mng_create_ani_disc (mng_datap   pData,
                                 mng_chunkp  pChunk)
#endif
{
  mng_ani_discp pDISC;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_DISC, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr pTemp;
    iRetcode = create_obj_general (pData, sizeof (mng_ani_disc),
                                   mng_free_ani_disc,
                                   mng_process_ani_disc,
                                   &pTemp);
    if (iRetcode)
      return iRetcode;
    pDISC = (mng_ani_discp)pTemp;
#else
    MNG_ALLOC (pData, pDISC, sizeof (mng_ani_disc));

    pDISC->sHeader.fCleanup = mng_free_ani_disc;
    pDISC->sHeader.fProcess = mng_process_ani_disc;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pDISC);

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pDISC->iCount = iCount;

    if (iCount)
    {
      MNG_ALLOC (pData, pDISC->pIds, (iCount << 1));
      MNG_COPY (pDISC->pIds, pIds, (iCount << 1));
    }
#else
    pDISC->iCount = ((mng_discp)pChunk)->iCount;

    if (pDISC->iCount)
    {
      mng_size_t iSize = (mng_size_t)(pDISC->iCount << 1);
      MNG_ALLOC (pData, pDISC->pIds, iSize);
      MNG_COPY (pDISC->pIds, ((mng_discp)pChunk)->pObjectids, iSize);
    }
#endif
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifndef MNG_OPTIMIZE_CHUNKREADER
  iRetcode = mng_process_display_disc (pData, iCount, pIds);
#else
  iRetcode = mng_process_display_disc (pData,
                                       ((mng_discp)pChunk)->iCount,
                                       ((mng_discp)pChunk)->pObjectids);
#endif
#else
#ifndef MNG_OPTIMIZE_CHUNKREADER
  pData->iDISCcount = iCount;
  pData->pDISCids   = pIds;
#else
  pData->iDISCcount = ((mng_discp)pChunk)->iCount;
  pData->pDISCids   = ((mng_discp)pChunk)->pObjectids;
#endif

  iRetcode = mng_process_display_disc (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANI_DISC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_free_ani_disc (mng_datap   pData,
                               mng_objectp pObject)
{
  mng_ani_discp pDISC = (mng_ani_discp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_DISC, MNG_LC_START);
#endif

  if (pDISC->iCount)
    MNG_FREEX (pData, pDISC->pIds, (pDISC->iCount << 1));

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  MNG_FREEX (pData, pObject, sizeof (mng_ani_disc));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANI_DISC, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  return MNG_NOERROR;
#else
  return mng_free_obj_general(pData, pObject);
#endif
}

/* ************************************************************************** */

mng_retcode mng_process_ani_disc (mng_datap   pData,
                                  mng_objectp pObject)
{
  mng_ani_discp pDISC = (mng_ani_discp)pObject;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_DISC, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = mng_process_display_disc (pData, pDISC->iCount, pDISC->pIds);
#else
  pData->iDISCcount = pDISC->iCount;
  pData->pDISCids   = pDISC->pIds;

  iRetcode = mng_process_display_disc (pData);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANI_DISC, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */
/* ************************************************************************** */

#ifdef MNG_SUPPORT_DYNAMICMNG

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_event (mng_datap  pData,
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
#else
mng_retcode mng_create_event (mng_datap  pData,
                              mng_ptr    pEntry)
#endif
{
  mng_eventp pEvent;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_EVENT, MNG_LC_START);
#endif

  if (pData->bCacheplayback)           /* caching playback info ? */
  {
    mng_object_headerp pLast;

#ifdef MNG_OPTIMIZE_OBJCLEANUP
    mng_ptr     pTemp;
    mng_retcode iRetcode = create_obj_general (pData, sizeof (mng_event),
                                               mng_free_event,
                                               mng_process_event,
                                               &pTemp);
    if (iRetcode)
      return iRetcode;
    pEvent = (mng_eventp)pTemp;
#else
    MNG_ALLOC (pData, pEvent, sizeof (mng_event));

    pEvent->sHeader.fCleanup = mng_free_event;
    pEvent->sHeader.fProcess = mng_process_event;
#endif

#ifndef MNG_OPTIMIZE_CHUNKREADER
    pEvent->iEventtype       = iEventtype;
    pEvent->iMasktype        = iMasktype;
    pEvent->iLeft            = iLeft;
    pEvent->iRight           = iRight;
    pEvent->iTop             = iTop;
    pEvent->iBottom          = iBottom;
    pEvent->iObjectid        = iObjectid;
    pEvent->iIndex           = iIndex;
    pEvent->iSegmentnamesize = iSegmentnamesize;

    if (iSegmentnamesize)
    {
      MNG_ALLOC (pData, pEvent->zSegmentname, iSegmentnamesize+1);
      MNG_COPY (pEvent->zSegmentname, zSegmentname, iSegmentnamesize);
    }
#else
    pEvent->iEventtype       = ((mng_evnt_entryp)pEntry)->iEventtype;
    pEvent->iMasktype        = ((mng_evnt_entryp)pEntry)->iMasktype;
    pEvent->iLeft            = ((mng_evnt_entryp)pEntry)->iLeft;
    pEvent->iRight           = ((mng_evnt_entryp)pEntry)->iRight;
    pEvent->iTop             = ((mng_evnt_entryp)pEntry)->iTop;
    pEvent->iBottom          = ((mng_evnt_entryp)pEntry)->iBottom;
    pEvent->iObjectid        = ((mng_evnt_entryp)pEntry)->iObjectid;
    pEvent->iIndex           = ((mng_evnt_entryp)pEntry)->iIndex;
    pEvent->iSegmentnamesize = ((mng_evnt_entryp)pEntry)->iSegmentnamesize;

    if (pEvent->iSegmentnamesize)
    {
      MNG_ALLOC (pData, pEvent->zSegmentname, pEvent->iSegmentnamesize+1);
      MNG_COPY (pEvent->zSegmentname, ((mng_evnt_entryp)pEntry)->zSegmentname, pEvent->iSegmentnamesize);
    }
#endif
                                       /* fixup the double-linked list */
    pLast                    = (mng_object_headerp)pData->pLastevent;

    if (pLast)                         /* link it as last in the chain */
    {
      pEvent->sHeader.pPrev  = pLast;
      pLast->pNext           = pEvent;
    }
    else
    {
      pData->pFirstevent     = pEvent;
    }

    pData->pLastevent        = pEvent;
    pData->bDynamic          = MNG_TRUE;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_EVENT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_free_event (mng_datap   pData,
                            mng_objectp pObject)
{
  mng_eventp pEvent = (mng_eventp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_EVENT, MNG_LC_START);
#endif

  if (pEvent->iSegmentnamesize)
    MNG_FREEX (pData, pEvent->zSegmentname, pEvent->iSegmentnamesize + 1);

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  MNG_FREEX (pData, pEvent, sizeof (mng_event));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_EVENT, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  return MNG_NOERROR;
#else
  return mng_free_obj_general(pData, pObject);
#endif
}

/* ************************************************************************** */

mng_retcode mng_process_event (mng_datap   pData,
                               mng_objectp pObject)
{
#ifndef MNG_SKIPCHUNK_SEEK
  mng_eventp         pEvent  = (mng_eventp)pObject;
  mng_object_headerp pAni;
  mng_bool           bFound = MNG_FALSE;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_EVENT, MNG_LC_START);
#endif

#ifndef MNG_SKIPCHUNK_SEEK
  if (!pEvent->pSEEK)                  /* need to find SEEK first ? */
  {
    pAni = (mng_object_headerp)pData->pFirstaniobj;

    while ((pAni) && (!bFound))
    {
      if ((pAni->fCleanup == mng_free_ani_seek) &&
          (strcmp(pEvent->zSegmentname, ((mng_ani_seekp)pAni)->zSegmentname) == 0))
        bFound = MNG_TRUE;
      else
        pAni = (mng_object_headerp)pAni->pNext;
    }

    if (pAni)
      pEvent->pSEEK = (mng_ani_seekp)pAni;
  }

  if (pEvent->pSEEK)                   /* anything to do ? */
  {
    pEvent->iLastx = pData->iEventx;
    pEvent->iLasty = pData->iEventy;
                                       /* let's start from this SEEK then */
    pData->pCurraniobj   = (mng_objectp)pEvent->pSEEK;
    pData->bRunningevent = MNG_TRUE;
                                       /* wake-up the app ! */
    if (!pData->fSettimer ((mng_handle)pData, 5))
      MNG_ERROR (pData, MNG_APPTIMERERROR);

  }
  else
    MNG_ERROR (pData, MNG_SEEKNOTFOUND);
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_EVENT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_SUPPORT_DYNAMICMNG */

/* ************************************************************************** */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_mpng_obj (mng_datap  pData,
                                 mng_uint32 iFramewidth,
                                 mng_uint32 iFrameheight,
                                 mng_uint16 iNumplays,
                                 mng_uint16 iTickspersec,
                                 mng_uint32 iFramessize,
                                 mng_ptr    pFrames)
#else
mng_retcode mng_create_mpng_obj (mng_datap  pData,
                                 mng_ptr    pEntry)
#endif
{
  mng_mpng_objp pMPNG;
  mng_ptr       pTemp;
  mng_retcode   iRetcode;
  mng_uint8p    pFrame;
  mng_int32     iCnt, iMax;
  mng_uint32    iX, iY, iWidth, iHeight;
  mng_int32     iXoffset, iYoffset;
  mng_uint16    iTicks;
  mng_uint16    iDelay;
  mng_bool      bNewframe;
  mng_ani_loopp pLOOP;
  mng_ani_endlp pENDL;
  mng_ani_framp pFRAM;
  mng_ani_movep pMOVE;
  mng_ani_clipp pCLIP;
  mng_ani_showp pSHOW;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_MPNG_OBJ, MNG_LC_START);
#endif

#ifdef MNG_OPTIMIZE_OBJCLEANUP
  iRetcode = create_obj_general (pData, sizeof (mng_mpng_obj), mng_free_mpng_obj,
                                 mng_process_mpng_obj, &pTemp);
  if (iRetcode)
    return iRetcode;
  pMPNG = (mng_mpng_objp)pTemp;
#else
  MNG_ALLOC (pData, pMPNG, sizeof (mng_mpng_obj));

  pMPNG->sHeader.fCleanup = mng_free_mpng_obj;
  pMPNG->sHeader.fProcess = mng_process_mpng_obj;
#endif

#ifndef MNG_OPTIMIZE_CHUNKREADER
  pMPNG->iFramewidth  = iFramewidth;
  pMPNG->iFrameheight = iFrameheight;
  pMPNG->iNumplays    = iNumplays;
  pMPNG->iTickspersec = iTickspersec;
  pMPNG->iFramessize  = iFramessize;

  if (iFramessize)
  {
    MNG_ALLOC (pData, pMPNG->pFrames, iFramessize);
    MNG_COPY (pMPNG->pFrames, pFrames, iFramessize);
  }
#else
  pMPNG->iFramewidth  = ((mng_mpngp)pEntry)->iFramewidth;
  pMPNG->iFrameheight = ((mng_mpngp)pEntry)->iFrameheight;
  pMPNG->iNumplays    = ((mng_mpngp)pEntry)->iNumplays;
  pMPNG->iTickspersec = ((mng_mpngp)pEntry)->iTickspersec;
  pMPNG->iFramessize  = ((mng_mpngp)pEntry)->iFramessize;

  if (pMPNG->iFramessize)
  {
    MNG_ALLOC (pData, pMPNG->pFrames, pMPNG->iFramessize);
    MNG_COPY (pMPNG->pFrames, ((mng_mpngp)pEntry)->pFrames, pMPNG->iFramessize);
  }
#endif

  pData->pMPNG      = pMPNG;
  pData->eImagetype = mng_it_mpng;

  iRetcode = mng_process_display_mpng (pData);
  if (iRetcode)
    return iRetcode;

  /* now let's create the MNG animation directives from this */

  pFrame = (mng_uint8p)pMPNG->pFrames;
  iMax   = pMPNG->iFramessize / 26;
                                       /* set up MNG impersonation */
  pData->iTicks      = pMPNG->iTickspersec;
  pData->iLayercount = iMax;

  if (pMPNG->iNumplays != 1)           /* create a LOOP/ENDL pair ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_loop),
                                   mng_free_ani_loop, mng_process_ani_loop,
                                   &((mng_ptr)pLOOP));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pLOOP, sizeof (mng_ani_loop));

    pLOOP->sHeader.fCleanup = mng_free_ani_loop;
    pLOOP->sHeader.fProcess = mng_process_ani_loop;
#endif

    pLOOP->iLevel = 1;
    if (pMPNG->iNumplays)
      pLOOP->iRepeatcount = pMPNG->iNumplays;
    else
      pLOOP->iRepeatcount = 0xFFFFFFFFl;

    mng_add_ani_object (pData, (mng_object_headerp)pLOOP);
  }

  bNewframe = MNG_TRUE;                /* create the frame display objects */

  for (iCnt = 0; iCnt < iMax; iCnt++)
  {
    iX       = mng_get_uint32 (pFrame);
    iY       = mng_get_uint32 (pFrame+4);
    iWidth   = mng_get_uint32 (pFrame+8);
    iHeight  = mng_get_uint32 (pFrame+12);
    iXoffset = mng_get_int32  (pFrame+16);
    iYoffset = mng_get_int32  (pFrame+20);
    iTicks   = mng_get_uint16 (pFrame+24);

    iDelay = iTicks;
    if (!iDelay)
    {
      mng_uint8p pTemp = pFrame+26;
      mng_int32  iTemp = iCnt+1;

      while ((iTemp < iMax) && (!iDelay))
      {
        iDelay = mng_get_uint16 (pTemp+24);
        pTemp += 26;
        iTemp++;
      }
    }

    if (bNewframe)
    {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
      iRetcode = create_obj_general (pData, sizeof (mng_ani_fram),
                                     mng_free_obj_general, mng_process_ani_fram,
                                     &((mng_ptr)pFRAM));
      if (iRetcode)
        return iRetcode;
#else
      MNG_ALLOC (pData, pFRAM, sizeof (mng_ani_fram));

      pFRAM->sHeader.fCleanup = mng_free_ani_fram;
      pFRAM->sHeader.fProcess = mng_process_ani_fram;
#endif

      pFRAM->iFramemode   = 4;
      pFRAM->iChangedelay = 1;
      pFRAM->iDelay       = iDelay;

      mng_add_ani_object (pData, (mng_object_headerp)pFRAM);
    }

#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_move),
                                   mng_free_obj_general,
                                   mng_process_ani_move,
                                   &((mng_ptr)pMOVE));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pMOVE, sizeof (mng_ani_move));

    pMOVE->sHeader.fCleanup = mng_free_ani_move;
    pMOVE->sHeader.fProcess = mng_process_ani_move;
#endif

    pMOVE->iLocax   = iXoffset - (mng_int32)iX;
    pMOVE->iLocay   = iYoffset - (mng_int32)iY;

    mng_add_ani_object (pData, (mng_object_headerp)pMOVE);

#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_clip),
                                   mng_free_obj_general,
                                   mng_process_ani_clip,
                                   &((mng_ptr)pCLIP));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pCLIP, sizeof (mng_ani_clip));

    pCLIP->sHeader.fCleanup = mng_free_ani_clip;
    pCLIP->sHeader.fProcess = mng_process_ani_clip;
#endif

    pCLIP->iClipl = iXoffset;
    pCLIP->iClipr = iXoffset + (mng_int32)iWidth;
    pCLIP->iClipt = iYoffset;
    pCLIP->iClipb = iYoffset + (mng_int32)iHeight;

    mng_add_ani_object (pData, (mng_object_headerp)pCLIP);

#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_show),
                                   mng_free_obj_general, mng_process_ani_show,
                                   &((mng_ptr)pSHOW));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pSHOW, sizeof (mng_ani_show));

    pSHOW->sHeader.fCleanup = mng_free_ani_show;
    pSHOW->sHeader.fProcess = mng_process_ani_show;
#endif

    mng_add_ani_object (pData, (mng_object_headerp)pSHOW);

    bNewframe = (mng_bool)iTicks;
    pFrame += 26;
  }

  if (pMPNG->iNumplays != 1)           /* create a LOOP/ENDL pair ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_endl),
                                   mng_free_obj_general, mng_process_ani_endl,
                                   &((mng_ptr)pENDL));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pENDL, sizeof (mng_ani_endl));

    pENDL->sHeader.fCleanup = mng_free_ani_endl;
    pENDL->sHeader.fProcess = mng_process_ani_endl;
#endif

    pENDL->iLevel = 1;

    mng_add_ani_object (pData, (mng_object_headerp)pENDL);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_MPNG_OBJ, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_free_mpng_obj (mng_datap   pData,
                               mng_objectp pObject)
{
  mng_mpng_objp pMPNG = (mng_mpng_objp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MPNG_OBJ, MNG_LC_START);
#endif

  if (pMPNG->iFramessize)
    MNG_FREEX (pData, pMPNG->pFrames, pMPNG->iFramessize);

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  MNG_FREEX (pData, pMPNG, sizeof (mng_mpng_obj));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_MPNG_OBJ, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  return MNG_NOERROR;
#else
  return mng_free_obj_general(pData, pObject);
#endif
}

/* ************************************************************************** */

mng_retcode mng_process_mpng_obj (mng_datap   pData,
                                  mng_objectp pObject)
{
  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_INCLUDE_MPNG_PROPOSAL */

/* ************************************************************************** */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_CHUNKREADER
mng_retcode mng_create_ang_obj (mng_datap  pData,
                                mng_uint32 iNumframes,
                                mng_uint32 iTickspersec,
                                mng_uint32 iNumplays,
                                mng_uint32 iTilewidth,
                                mng_uint32 iTileheight,
                                mng_uint8  iInterlace,
                                mng_uint8  iStillused)
#else
mng_retcode mng_create_ang_obj (mng_datap  pData,
                                mng_ptr    pEntry)
#endif
{
  mng_ang_objp  pANG;
  mng_ptr       pTemp;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANG_OBJ, MNG_LC_START);
#endif

#ifdef MNG_OPTIMIZE_OBJCLEANUP
  iRetcode = create_obj_general (pData, sizeof (mng_ang_obj), mng_free_ang_obj,
                                 mng_process_ang_obj, &pTemp);
  if (iRetcode)
    return iRetcode;
  pANG = (mng_ang_objp)pTemp;
#else
  MNG_ALLOC (pData, pANG, sizeof (mng_ang_obj));

  pANG->sHeader.fCleanup = mng_free_ang_obj;
  pANG->sHeader.fProcess = mng_process_ang_obj;
#endif

#ifndef MNG_OPTIMIZE_CHUNKREADER
  pANG->iNumframes   = iNumframes;
  pANG->iTickspersec = iTickspersec;
  pANG->iNumplays    = iNumplays;
  pANG->iTilewidth   = iTilewidth;
  pANG->iTileheight  = iTileheight;
  pANG->iInterlace   = iInterlace;
  pANG->iStillused   = iStillused;
#else
  pANG->iNumframes   = ((mng_ahdrp)pEntry)->iNumframes;
  pANG->iTickspersec = ((mng_ahdrp)pEntry)->iTickspersec;
  pANG->iNumplays    = ((mng_ahdrp)pEntry)->iNumplays;
  pANG->iTilewidth   = ((mng_ahdrp)pEntry)->iTilewidth;
  pANG->iTileheight  = ((mng_ahdrp)pEntry)->iTileheight;
  pANG->iInterlace   = ((mng_ahdrp)pEntry)->iInterlace;
  pANG->iStillused   = ((mng_ahdrp)pEntry)->iStillused;
#endif

  pData->pANG       = pANG;
  pData->eImagetype = mng_it_ang;

  iRetcode = mng_process_display_ang (pData);
  if (iRetcode)
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CREATE_ANG_OBJ, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_free_ang_obj (mng_datap   pData,
                              mng_objectp pObject)
{
  mng_ang_objp pANG = (mng_ang_objp)pObject;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANG_OBJ, MNG_LC_START);
#endif

  if (pANG->iTilessize)
    MNG_FREEX (pData, pANG->pTiles, pANG->iTilessize);

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  MNG_FREEX (pData, pANG, sizeof (mng_ang_obj));
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FREE_ANG_OBJ, MNG_LC_END);
#endif

#ifndef MNG_OPTIMIZE_OBJCLEANUP
  return MNG_NOERROR;
#else
  return mng_free_obj_general(pData, pObject);
#endif
}

/* ************************************************************************** */

mng_retcode mng_process_ang_obj (mng_datap   pData,
                                 mng_objectp pObject)
{
  mng_ang_objp  pANG  = (mng_ang_objp)pObject;
  mng_uint8p    pTile = (mng_uint8p)pANG->pTiles;
  mng_retcode   iRetcode;
  mng_int32     iCnt, iMax;
  mng_uint32    iTicks;
  mng_int32     iXoffset, iYoffset;
  mng_uint8     iSource;
  mng_ani_loopp pLOOP;
  mng_ani_endlp pENDL;
  mng_ani_framp pFRAM;
  mng_ani_movep pMOVE;
  mng_ani_showp pSHOW;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANG_OBJ, MNG_LC_START);
#endif

  /* let's create the MNG animation directives from this */

  iMax = pANG->iNumframes;
                                       /* set up MNG impersonation */
  pData->iTicks      = pANG->iTickspersec;
  pData->iLayercount = iMax;

  if (pANG->iNumplays != 1)            /* create a LOOP/ENDL pair ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_loop),
                                   mng_free_ani_loop, mng_process_ani_loop,
                                   &((mng_ptr)pLOOP));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pLOOP, sizeof (mng_ani_loop));

    pLOOP->sHeader.fCleanup = mng_free_ani_loop;
    pLOOP->sHeader.fProcess = mng_process_ani_loop;
#endif

    pLOOP->iLevel = 1;
    if (pANG->iNumplays)
      pLOOP->iRepeatcount = pANG->iNumplays;
    else
      pLOOP->iRepeatcount = 0xFFFFFFFFl;

    mng_add_ani_object (pData, (mng_object_headerp)pLOOP);
  }

  for (iCnt = 0; iCnt < iMax; iCnt++)
  {
    iTicks   = mng_get_uint32 (pTile);
    iXoffset = mng_get_int32  (pTile+4);
    iYoffset = mng_get_int32  (pTile+8);
    iSource  = *(pTile+12);

#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_fram),
                                   mng_free_obj_general, mng_process_ani_fram,
                                   &((mng_ptr)pFRAM));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pFRAM, sizeof (mng_ani_fram));

    pFRAM->sHeader.fCleanup = mng_free_ani_fram;
    pFRAM->sHeader.fProcess = mng_process_ani_fram;
#endif

    pFRAM->iFramemode   = 4;
    pFRAM->iChangedelay = 1;
    pFRAM->iDelay       = iTicks;

    mng_add_ani_object (pData, (mng_object_headerp)pFRAM);

    if (!iSource)
    {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
      iRetcode = create_obj_general (pData, sizeof (mng_ani_move),
                                     mng_free_obj_general,
                                     mng_process_ani_move,
                                     &((mng_ptr)pMOVE));
      if (iRetcode)
        return iRetcode;
#else
      MNG_ALLOC (pData, pMOVE, sizeof (mng_ani_move));

      pMOVE->sHeader.fCleanup = mng_free_ani_move;
      pMOVE->sHeader.fProcess = mng_process_ani_move;
#endif

      pMOVE->iFirstid = 1;
      pMOVE->iLastid  = 1;
      pMOVE->iLocax   = -iXoffset;
      pMOVE->iLocay   = -iYoffset;

      mng_add_ani_object (pData, (mng_object_headerp)pMOVE);
    }

#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_show),
                                   mng_free_obj_general, mng_process_ani_show,
                                   &((mng_ptr)pSHOW));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pSHOW, sizeof (mng_ani_show));

    pSHOW->sHeader.fCleanup = mng_free_ani_show;
    pSHOW->sHeader.fProcess = mng_process_ani_show;
#endif

    if (iSource)
      pSHOW->iFirstid = 0;
    else
      pSHOW->iFirstid = 1;
    pSHOW->iLastid    = pSHOW->iFirstid;

    mng_add_ani_object (pData, (mng_object_headerp)pSHOW);

    pTile += sizeof(mng_adat_tile);
  }

  if (pANG->iNumplays != 1)            /* create a LOOP/ENDL pair ? */
  {
#ifdef MNG_OPTIMIZE_OBJCLEANUP
    iRetcode = create_obj_general (pData, sizeof (mng_ani_endl),
                                   mng_free_obj_general, mng_process_ani_endl,
                                   &((mng_ptr)pENDL));
    if (iRetcode)
      return iRetcode;
#else
    MNG_ALLOC (pData, pENDL, sizeof (mng_ani_endl));

    pENDL->sHeader.fCleanup = mng_free_ani_endl;
    pENDL->sHeader.fProcess = mng_process_ani_endl;
#endif

    pENDL->iLevel = 1;

    mng_add_ani_object (pData, (mng_object_headerp)pENDL);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_ANG_OBJ, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_INCLUDE_ANG_PROPOSAL */

/* ************************************************************************** */

#endif /* MNG_INCLUDE_DISPLAY_PROCS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

