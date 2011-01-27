/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_filter.c           copyright (c) 2000-2004 G.Juyn   * */
/* * version   : 1.0.9                                                      * */
/* *                                                                        * */
/* * purpose   : Filtering routines (implementation)                        * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the filtering routines                   * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 09/07/2000 - G.Juyn                                * */
/* *             - added support for new filter_types                       * */
/* *                                                                        * */
/* *             1.0.5 - 08/07/2002 - G.Juyn                                * */
/* *             - added test-option for PNG filter method 193 (=no filter) * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *                                                                        * */
/* *             1.0.6 - 07/07/2003 - G.R-P                                 * */
/* *             - reversed some loops to use decrementing counter          * */
/* *                                                                        * */
/* *             1.0.9 - 12/20/2004 - G.Juyn                                * */
/* *             - cleaned up macro-invocations (thanks to D. Airlie)       * */
/* *                                                                        * */
/* ************************************************************************** */

#include "libmng.h"
#include "libmng_data.h"
#include "libmng_error.h"
#include "libmng_trace.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#include "libmng_filter.h"

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_FILTERS

/* ************************************************************************** */

MNG_LOCAL mng_retcode filter_sub (mng_datap pData)
{
  mng_uint32 iBpp;
  mng_uint8p pRawx;
  mng_uint8p pRawx_prev;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_SUB, MNG_LC_START);
#endif

  iBpp       = pData->iFilterbpp;
  pRawx      = pData->pWorkrow + pData->iPixelofs + iBpp;
  pRawx_prev = pData->pWorkrow + pData->iPixelofs;

  for (iX = iBpp; iX < pData->iRowsize; iX++)
  {
    *pRawx = (mng_uint8)(*pRawx + *pRawx_prev);
    pRawx++;
    pRawx_prev++;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_SUB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode filter_up (mng_datap pData)
{
  mng_uint8p pRawx;
  mng_uint8p pPriorx;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_UP, MNG_LC_START);
#endif

  pRawx   = pData->pWorkrow + pData->iPixelofs;
  pPriorx = pData->pPrevrow + pData->iPixelofs;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsize - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsize; iX++)
#endif
  {
    *pRawx = (mng_uint8)(*pRawx + *pPriorx);
    pRawx++;
    pPriorx++;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_UP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode filter_average (mng_datap pData)
{
  mng_int32  iBpp;
  mng_uint8p pRawx;
  mng_uint8p pRawx_prev;
  mng_uint8p pPriorx;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_AVERAGE, MNG_LC_START);
#endif

  iBpp       = pData->iFilterbpp;
  pRawx      = pData->pWorkrow + pData->iPixelofs;
  pPriorx    = pData->pPrevrow + pData->iPixelofs;
  pRawx_prev = pData->pWorkrow + pData->iPixelofs;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = iBpp - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < iBpp; iX++)
#endif
  {
    *pRawx = (mng_uint8)(*pRawx + ((*pPriorx) >> 1));
    pRawx++;
    pPriorx++;
  }

  for (iX = iBpp; iX < pData->iRowsize; iX++)
  {
    *pRawx = (mng_uint8)(*pRawx + ((*pRawx_prev + *pPriorx) >> 1));
    pRawx++;
    pPriorx++;
    pRawx_prev++;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_AVERAGE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode filter_paeth (mng_datap pData)
{
  mng_int32  iBpp;
  mng_uint8p pRawx;
  mng_uint8p pRawx_prev;
  mng_uint8p pPriorx;
  mng_uint8p pPriorx_prev;
  mng_int32  iX;
  mng_uint32 iA, iB, iC;
  mng_uint32 iP;
  mng_uint32 iPa, iPb, iPc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_PAETH, MNG_LC_START);
#endif

  iBpp         = pData->iFilterbpp;
  pRawx        = pData->pWorkrow + pData->iPixelofs;
  pPriorx      = pData->pPrevrow + pData->iPixelofs;
  pRawx_prev   = pData->pWorkrow + pData->iPixelofs;
  pPriorx_prev = pData->pPrevrow + pData->iPixelofs;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = iBpp - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < iBpp; iX++)
#endif
  {
    *pRawx = (mng_uint8)(*pRawx + *pPriorx);

    pRawx++;
    pPriorx++;
  }

  for (iX = iBpp; iX < pData->iRowsize; iX++)
  {
    iA  = (mng_uint32)*pRawx_prev;
    iB  = (mng_uint32)*pPriorx;
    iC  = (mng_uint32)*pPriorx_prev;
    iP  = iA + iB - iC;
    iPa = abs (iP - iA);
    iPb = abs (iP - iB);
    iPc = abs (iP - iC);

    if ((iPa <= iPb) && (iPa <= iPc))
      *pRawx = (mng_uint8)(*pRawx + iA);
    else
      if (iPb <= iPc)
        *pRawx = (mng_uint8)(*pRawx + iB);
      else
        *pRawx = (mng_uint8)(*pRawx + iC);

    pRawx++;
    pPriorx++;
    pRawx_prev++;
    pPriorx_prev++;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_PAETH, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_filter_a_row (mng_datap pData)
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_A_ROW, MNG_LC_START);
#endif

  switch (*(pData->pWorkrow + pData->iFilterofs))
  {
    case 1  : {
                iRetcode = filter_sub     (pData);
                break;
              }
    case 2  : {
                iRetcode = filter_up      (pData);
                break;
              }
    case 3  : {
                iRetcode = filter_average (pData);
                break;
              }
    case 4  : {
                iRetcode = filter_paeth   (pData);
                break;
              }

    default : iRetcode = MNG_INVALIDFILTER;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_FILTER_A_ROW, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */
/* ************************************************************************** */

#ifdef FILTER192
mng_retcode mng_init_rowdiffering (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ROWDIFFERING, MNG_LC_START);
#endif

  if (pData->iFilter == 0xC0)          /* has leveling parameters ? */
  {
    switch (pData->iColortype)         /* salvage leveling parameters */
    {
      case 0 : {                       /* gray */
                 if (pData->iBitdepth <= 8)
                   pData->iLevel0 = (mng_uint16)*pData->pWorkrow;
                 else
                   pData->iLevel0 = mng_get_uint16 (pData->pWorkrow);

                 break;
               }
      case 2 : {                       /* rgb */
                 if (pData->iBitdepth <= 8)
                 {
                   pData->iLevel0 = (mng_uint16)*pData->pWorkrow;
                   pData->iLevel1 = (mng_uint16)*(pData->pWorkrow+1);
                   pData->iLevel2 = (mng_uint16)*(pData->pWorkrow+2);
                 }
                 else
                 {
                   pData->iLevel0 = mng_get_uint16 (pData->pWorkrow);
                   pData->iLevel1 = mng_get_uint16 (pData->pWorkrow+2);
                   pData->iLevel2 = mng_get_uint16 (pData->pWorkrow+4);
                 }

                 break;
               }
      case 3 : {                       /* indexed */
                 pData->iLevel0 = (mng_uint16)*pData->pWorkrow;
                 break;
               }
      case 4 : {                       /* gray+alpha */
                 if (pData->iBitdepth <= 8)
                 {
                   pData->iLevel0 = (mng_uint16)*pData->pWorkrow;
                   pData->iLevel1 = (mng_uint16)*(pData->pWorkrow+1);
                 }
                 else
                 {
                   pData->iLevel0 = mng_get_uint16 (pData->pWorkrow);
                   pData->iLevel1 = mng_get_uint16 (pData->pWorkrow+2);
                 }

                 break;
               }
      case 6 : {                       /* rgb+alpha */
                 if (pData->iBitdepth <= 8)
                 {
                   pData->iLevel0 = (mng_uint16)*pData->pWorkrow;
                   pData->iLevel1 = (mng_uint16)*(pData->pWorkrow+1);
                   pData->iLevel2 = (mng_uint16)*(pData->pWorkrow+2);
                   pData->iLevel3 = (mng_uint16)*(pData->pWorkrow+3);
                 }
                 else
                 {
                   pData->iLevel0 = mng_get_uint16 (pData->pWorkrow);
                   pData->iLevel1 = mng_get_uint16 (pData->pWorkrow+2);
                   pData->iLevel2 = mng_get_uint16 (pData->pWorkrow+4);
                   pData->iLevel3 = mng_get_uint16 (pData->pWorkrow+6);
                 }

                 break;
               }
    }
  }
                                       /* shift the entire row back in place */
  pRawi = pData->pWorkrow + pData->iFilterofs;
  pRawo = pData->pWorkrow;

  for (iX = 0; iX < pData->iRowsize + pData->iPixelofs - pData->iFilterofs; iX++)
    *pRawo++ = *pRawi++;

  pData->iFilterofs = 0;               /* indicate so ! */

#ifdef FILTER193
  if (pData->iFilter == 0xC1)          /* no adaptive filtering ? */
    pData->iPixelofs = pData->iFilterofs;
  else
#endif
    pData->iPixelofs = pData->iFilterofs + 1;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_ROWDIFFERING, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_g1 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G1, MNG_LC_START);
#endif

  if (pData->iLevel0 & 0x01)           /* is it uneven level ? */
  {
    pRawi = pData->pWorkrow + pData->iPixelofs;
    pRawo = pData->pPrevrow + pData->iPixelofs;
                                       /* just invert every bit */
#ifdef MNG_DECREMENT_LOOPS
    for (iX = pData->iRowsize - 1; iX >= 0; iX--)
#else
    for (iX = 0; iX < pData->iRowsize; iX++)
#endif
      *pRawo++ = (mng_uint8)(~(*pRawi++));

  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G1, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_g2 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;
  mng_int32  iC, iS;
  mng_uint8  iB, iN, iQ;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G2, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;
  iC    = 0;
  iB    = 0;
  iN    = 0;
  iS    = 0;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    if (!iC)
    {
      iC = 4;
      iB = *pRawi++;
      iN = 0;
      iS = 8;
    }

    iS -= 2;
    iQ = (mng_uint8)(((iB >> iS) + pData->iLevel0) & 0x03);
    iN = (mng_uint8)((iN << 2) + iQ);
    iC--;

    if (!iC)
      *pRawo++ = iN;

  }

  if (iC)
    *pRawo = (mng_uint8)(iN << iS);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G2, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_g4 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;
  mng_int32  iC, iS;
  mng_uint8  iB, iN, iQ;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G4, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;
  iC    = 0;
  iB    = 0;
  iN    = 0;
  iS    = 0;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    if (!iC)
    {
      iC = 2;
      iB = *pRawi++;
      iN = 0;
      iS = 8;
    }

    iS -= 4;
    iQ = (mng_uint8)(((iB >> iS) + pData->iLevel0) & 0x0F);
    iN = (mng_uint8)((iN << 4) + iQ);
    iC--;

    if (!iC)
      *pRawo++ = iN;

  }

  if (iC)
    *pRawo = (mng_uint8)(iN << iS);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G4, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_g8 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G8, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *pRawo++ = (mng_uint8)(((mng_uint16)*pRawi + pData->iLevel0) & 0xFF);

    pRawi++;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G8, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_g16 (mng_datap pData)
{
  mng_uint16p pRawi, pRawo;
  mng_int32   iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G16, MNG_LC_START);
#endif

  pRawi = (mng_uint16p)(pData->pWorkrow + pData->iPixelofs);
  pRawo = (mng_uint16p)(pData->pPrevrow + pData->iPixelofs);

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *pRawo++ = (mng_uint16)(((mng_uint32)*pRawi + (mng_uint32)pData->iLevel0) & 0xFFFF);

    pRawi++;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_G16, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_rgb8 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_RGB8, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *(pRawo+1) = (mng_uint8)(((mng_uint16)*(pRawi+1) + pData->iLevel1) & 0xFF);
    *pRawo     = (mng_uint8)(((mng_uint16)*pRawi     + pData->iLevel0 +
                              (mng_uint16)*(pRawo+1)) & 0xFF);
    *(pRawo+2) = (mng_uint8)(((mng_uint16)*(pRawi+2) + pData->iLevel2 +
                              (mng_uint16)*(pRawo+1)) & 0xFF);

    pRawi += 3;
    pRawo += 3;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_RGB8, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_rgb16 (mng_datap pData)
{
  mng_uint16p pRawi, pRawo;
  mng_int32   iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_RGB16, MNG_LC_START);
#endif

  pRawi = (mng_uint16p)(pData->pWorkrow + pData->iPixelofs);
  pRawo = (mng_uint16p)(pData->pPrevrow + pData->iPixelofs);

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *(pRawo+1) = (mng_uint16)(((mng_uint32)*(pRawi+1) + (mng_uint32)pData->iLevel1) & 0xFFFF);
    *pRawo     = (mng_uint16)(((mng_uint32)*pRawi     + (mng_uint32)pData->iLevel0 +
                               (mng_uint32)*(pRawo+1)) & 0xFFFF);
    *(pRawo+2) = (mng_uint16)(((mng_uint32)*(pRawi+2) + (mng_uint32)pData->iLevel2 +
                               (mng_uint32)*(pRawo+1)) & 0xFFFF);

    pRawi += 3;
    pRawo += 3;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_RGB16, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_idx1 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_IDX1, MNG_LC_START);
#endif

  if (pData->iLevel0 & 0x01)           /* is it uneven level ? */
  {
    pRawi = pData->pWorkrow + pData->iPixelofs;
    pRawo = pData->pPrevrow + pData->iPixelofs;
                                       /* just invert every bit */
#ifdef MNG_DECREMENT_LOOPS
    for (iX = pData->iRowsize - 1; iX >= 0; iX--)
#else
    for (iX = 0; iX < pData->iRowsize; iX++)
#endif
      *pRawo++ = (mng_uint8)(~(*pRawi++));

  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_IDX1, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_idx2 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;
  mng_int32  iC, iS;
  mng_uint8  iB, iN, iQ;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_IDX2, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;
  iC    = 0;
  iB    = 0;
  iN    = 0;
  iS    = 0;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    if (!iC)
    {
      iC = 4;
      iB = *pRawi++;
      iN = 0;
      iS = 8;
    }

    iS -= 2;
    iQ = (mng_uint8)(((iB >> iS) + pData->iLevel0) & 0x03);
    iN = (mng_uint8)((iN << 2) + iQ);
    iC--;

    if (!iC)
      *pRawo++ = iN;

  }

  if (iC)
    *pRawo = (mng_uint8)(iN << iS);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_IDX2, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_idx4 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;
  mng_int32  iC, iS;
  mng_uint8  iB, iN, iQ;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_IDX4, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;
  iC    = 0;
  iB    = 0;
  iN    = 0;
  iS    = 0;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    if (!iC)
    {
      iC = 2;
      iB = *pRawi++;
      iN = 0;
      iS = 8;
    }

    iS -= 4;
    iQ = (mng_uint8)(((iB >> iS) + pData->iLevel0) & 0x0F);
    iN = (mng_uint8)((iN << 4) + iQ);
    iC--;

    if (!iC)
      *pRawo++ = iN;

  }

  if (iC)
    *pRawo = (mng_uint8)(iN << iS);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_IDX4, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_idx8 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_IDX8, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *pRawo++ = (mng_uint8)(((mng_uint16)*pRawi + pData->iLevel0) & 0xFF);

    pRawi++;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_IDX8, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_ga8 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_GA8, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *pRawo     = (mng_uint8)(((mng_uint16)*pRawi     + pData->iLevel0) & 0xFF);
    *(pRawo+1) = (mng_uint8)(((mng_uint16)*(pRawi+1) + pData->iLevel1) & 0xFF);

    pRawi += 2;
    pRawo += 2;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_GA8, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_ga16 (mng_datap pData)
{
  mng_uint16p pRawi, pRawo;
  mng_int32   iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_GA16, MNG_LC_START);
#endif

  pRawi = (mng_uint16p)(pData->pWorkrow + pData->iPixelofs);
  pRawo = (mng_uint16p)(pData->pPrevrow + pData->iPixelofs);

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *pRawo     = (mng_uint16)(((mng_uint32)*pRawi     + (mng_uint32)pData->iLevel0) & 0xFFFF);
    *(pRawo+1) = (mng_uint16)(((mng_uint32)*(pRawi+1) + (mng_uint32)pData->iLevel1) & 0xFFFF);

    pRawi += 2;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_GA16, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_rgba8 (mng_datap pData)
{
  mng_uint8p pRawi, pRawo;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_RGBA8, MNG_LC_START);
#endif

  pRawi = pData->pWorkrow + pData->iPixelofs;
  pRawo = pData->pPrevrow + pData->iPixelofs;

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *(pRawo+1) = (mng_uint8)(((mng_uint16)*(pRawi+1) + pData->iLevel1) & 0xFF);
    *pRawo     = (mng_uint8)(((mng_uint16)*pRawi     + pData->iLevel0 +
                              (mng_uint16)*(pRawo+1)) & 0xFF);
    *(pRawo+2) = (mng_uint8)(((mng_uint16)*(pRawi+2) + pData->iLevel2 +
                              (mng_uint16)*(pRawo+1)) & 0xFF);
    *(pRawo+3) = (mng_uint8)(((mng_uint16)*(pRawi+3) + pData->iLevel3) & 0xFF);

    pRawi += 4;
    pRawo += 4;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_RGBA8, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_differ_rgba16 (mng_datap pData)
{
  mng_uint16p pRawi, pRawo;
  mng_int32   iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_RGBA16, MNG_LC_START);
#endif

  pRawi = (mng_uint16p)(pData->pWorkrow + pData->iPixelofs);
  pRawo = (mng_uint16p)(pData->pPrevrow + pData->iPixelofs);

#ifdef MNG_DECREMENT_LOOPS
  for (iX = pData->iRowsamples - 1; iX >= 0; iX--)
#else
  for (iX = 0; iX < pData->iRowsamples; iX++)
#endif
  {
    *(pRawo+1) = (mng_uint16)(((mng_uint32)*(pRawi+1) + (mng_uint32)pData->iLevel1) & 0xFFFF);
    *pRawo     = (mng_uint16)(((mng_uint32)*pRawi     + (mng_uint32)pData->iLevel0 +
                               (mng_uint32)*(pRawo+1)) & 0xFFFF);
    *(pRawo+2) = (mng_uint16)(((mng_uint32)*(pRawi+2) + (mng_uint32)pData->iLevel2 +
                               (mng_uint32)*(pRawo+1)) & 0xFFFF);
    *(pRawo+3) = (mng_uint16)(((mng_uint32)*(pRawi+3) + (mng_uint32)pData->iLevel3) & 0xFFFF);

    pRawi += 4;
    pRawo += 4;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DIFFER_RGBA16, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* FILTER192 */

/* ************************************************************************** */

#endif /* MNG_INCLUDE_FILTERS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

