/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_display.c          copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Display management (implementation)                        * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the display management routines          * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/11/2000 - G.Juyn                                * */
/* *             - added callback error-reporting support                   * */
/* *             - fixed frame_delay misalignment                           * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - added sanity check for frozen status                     * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *             0.5.1 - 05/13/2000 - G.Juyn                                * */
/* *             - changed display_mend to reset state to initial or SAVE   * */
/* *             - added eMNGma hack (will be removed in 1.0.0 !!!)         * */
/* *             - added TERM animation object pointer (easier reference)   * */
/* *             - added process_save & process_seek routines               * */
/* *             0.5.1 - 05/14/2000 - G.Juyn                                * */
/* *             - added save_state and restore_state for SAVE/SEEK/TERM    * */
/* *               processing                                               * */
/* *                                                                        * */
/* *             0.5.2 - 05/20/2000 - G.Juyn                                * */
/* *             - added JNG support (JHDR/JDAT)                            * */
/* *             0.5.2 - 05/23/2000 - G.Juyn                                * */
/* *             - fixed problem with DEFI clipping                         * */
/* *             0.5.2 - 05/30/2000 - G.Juyn                                * */
/* *             - added delta-image support (DHDR,PROM,IPNG,IJNG)          * */
/* *             0.5.2 - 05/31/2000 - G.Juyn                                * */
/* *             - fixed pointer confusion (contributed by Tim Rowley)      * */
/* *             0.5.2 - 06/03/2000 - G.Juyn                                * */
/* *             - fixed makeup for Linux gcc compile                       * */
/* *             0.5.2 - 06/05/2000 - G.Juyn                                * */
/* *             - added support for RGB8_A8 canvasstyle                    * */
/* *             0.5.2 - 06/09/2000 - G.Juyn                                * */
/* *             - fixed timer-handling to run with Mozilla (Tim Rowley)    * */
/* *             0.5.2 - 06/10/2000 - G.Juyn                                * */
/* *             - fixed some compilation-warnings (contrib Jason Morris)   * */
/* *                                                                        * */
/* *             0.5.3 - 06/12/2000 - G.Juyn                                * */
/* *             - fixed display of stored JNG images                       * */
/* *             0.5.3 - 06/13/2000 - G.Juyn                                * */
/* *             - fixed problem with BASI-IEND as object 0                 * */
/* *             0.5.3 - 06/16/2000 - G.Juyn                                * */
/* *             - changed progressive-display processing                   * */
/* *             0.5.3 - 06/17/2000 - G.Juyn                                * */
/* *             - changed delta-image processing                           * */
/* *             0.5.3 - 06/20/2000 - G.Juyn                                * */
/* *             - fixed some minor stuff                                   * */
/* *             0.5.3 - 06/21/2000 - G.Juyn                                * */
/* *             - added speed-modifier to timing routine                   * */
/* *             0.5.3 - 06/22/2000 - G.Juyn                                * */
/* *             - added support for PPLT chunk processing                  * */
/* *             0.5.3 - 06/29/2000 - G.Juyn                                * */
/* *             - swapped refresh parameters                               * */
/* *                                                                        * */
/* *             0.9.0 - 06/30/2000 - G.Juyn                                * */
/* *             - changed refresh parameters to 'x,y,width,height'         * */
/* *                                                                        * */
/* *             0.9.1 - 07/07/2000 - G.Juyn                                * */
/* *             - implemented support for freeze/reset/resume & go_xxxx    * */
/* *             0.9.1 - 07/08/2000 - G.Juyn                                * */
/* *             - added support for improved timing                        * */
/* *             0.9.1 - 07/14/2000 - G.Juyn                                * */
/* *             - changed EOF processing behavior                          * */
/* *             - fixed TERM delay processing                              * */
/* *             0.9.1 - 07/15/2000 - G.Juyn                                * */
/* *             - fixed freeze & reset processing                          * */
/* *             0.9.1 - 07/16/2000 - G.Juyn                                * */
/* *             - fixed storage of images during mng_read()                * */
/* *             - fixed support for mng_display() after mng_read()         * */
/* *             0.9.1 - 07/24/2000 - G.Juyn                                * */
/* *             - fixed reading of still-images                            * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/07/2000 - G.Juyn                                * */
/* *             - B111300 - fixup for improved portability                 * */
/* *             0.9.3 - 08/21/2000 - G.Juyn                                * */
/* *             - fixed TERM processing delay of 0 msecs                   * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 09/10/2000 - G.Juyn                                * */
/* *             - fixed problem with no refresh after TERM                 * */
/* *             - fixed DEFI behavior                                      * */
/* *             0.9.3 - 09/16/2000 - G.Juyn                                * */
/* *             - fixed timing & refresh behavior for single PNG/JNG       * */
/* *             0.9.3 - 09/19/2000 - G.Juyn                                * */
/* *             - refixed timing & refresh behavior for single PNG/JNG     * */
/* *             0.9.3 - 10/02/2000 - G.Juyn                                * */
/* *             - fixed timing again (this is getting boring...)           * */
/* *             - refixed problem with no refresh after TERM               * */
/* *             0.9.3 - 10/16/2000 - G.Juyn                                * */
/* *             - added JDAA chunk                                         * */
/* *             0.9.3 - 10/17/2000 - G.Juyn                                * */
/* *             - fixed support for bKGD                                   * */
/* *             0.9.3 - 10/18/2000 - G.Juyn                                * */
/* *             - fixed delta-processing behavior                          * */
/* *             0.9.3 - 10/19/2000 - G.Juyn                                * */
/* *             - added storage for pixel-/alpha-sampledepth for delta's   * */
/* *             0.9.3 - 10/27/2000 - G.Juyn                                * */
/* *             - fixed separate read() & display() processing             * */
/* *                                                                        * */
/* *             0.9.4 - 10/31/2000 - G.Juyn                                * */
/* *             - fixed possible loop in display_resume() (Thanks Vova!)   * */
/* *             0.9.4 - 11/20/2000 - G.Juyn                                * */
/* *             - fixed unwanted repetition in mng_readdisplay()           * */
/* *             0.9.4 - 11/24/2000 - G.Juyn                                * */
/* *             - moved restore of object 0 to libmng_display              * */
/* *             - added restore of object 0 to TERM processing !!!         * */
/* *             - fixed TERM delay processing                              * */
/* *             - fixed TERM end processing (count = 0)                    * */
/* *             0.9.4 - 12/16/2000 - G.Juyn                                * */
/* *             - fixed mixup of data- & function-pointers (thanks Dimitri)* */
/* *             0.9.4 -  1/18/2001 - G.Juyn                                * */
/* *             - removed test filter-methods 1 & 65                       * */
/* *             - set default level-set for filtertype=64 to all zeroes    * */
/* *                                                                        * */
/* *             0.9.5 -  1/20/2001 - G.Juyn                                * */
/* *             - fixed compiler-warnings Mozilla (thanks Tim)             * */
/* *             0.9.5 -  1/23/2001 - G.Juyn                                * */
/* *             - fixed timing-problem with switching framing_modes        * */
/* *                                                                        * */
/* *             1.0.1 - 02/08/2001 - G.Juyn                                * */
/* *             - added MEND processing callback                           * */
/* *             1.0.1 - 02/13/2001 - G.Juyn                                * */
/* *             - fixed first FRAM_MODE=4 timing problem                   * */
/* *             1.0.1 - 04/21/2001 - G.Juyn                                * */
/* *             - fixed memory-leak for JNGs with alpha (Thanks Gregg!)    * */
/* *             - added BGRA8 canvas with premultiplied alpha              * */
/* *                                                                        * */
/* *             1.0.2 - 06/25/2001 - G.Juyn                                * */
/* *             - fixed memory-leak with delta-images (Thanks Michael!)    * */
/* *                                                                        * */
/* *             1.0.5 - 08/15/2002 - G.Juyn                                * */
/* *             - completed PROM support                                   * */
/* *             - completed delta-image support                            * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             1.0.5 - 09/13/2002 - G.Juyn                                * */
/* *             - fixed read/write of MAGN chunk                           * */
/* *             1.0.5 - 09/15/2002 - G.Juyn                                * */
/* *             - fixed LOOP iteration=0 special case                      * */
/* *             1.0.5 - 09/19/2002 - G.Juyn                                * */
/* *             - fixed color-correction for restore-background handling   * */
/* *             - optimized restore-background for bKGD cases              * */
/* *             - cleaned up some old stuff                                * */
/* *             1.0.5 - 09/20/2002 - G.Juyn                                * */
/* *             - finished support for BACK image & tiling                 * */
/* *             - added support for PAST                                   * */
/* *             1.0.5 - 09/22/2002 - G.Juyn                                * */
/* *             - added bgrx8 canvas (filler byte)                         * */
/* *             1.0.5 - 10/05/2002 - G.Juyn                                * */
/* *             - fixed dropping mix of frozen/unfrozen objects            * */
/* *             1.0.5 - 10/07/2002 - G.Juyn                                * */
/* *             - added proposed change in handling of TERM- & if-delay    * */
/* *             - added another fix for misplaced TERM chunk               * */
/* *             - completed support for condition=2 in TERM chunk          * */
/* *             1.0.5 - 10/18/2002 - G.Juyn                                * */
/* *             - fixed clipping-problem with BACK tiling (Thanks Sakura!) * */
/* *             1.0.5 - 10/20/2002 - G.Juyn                                * */
/* *             - fixed processing for multiple objects in MAGN            * */
/* *             - fixed display of visible target of PAST operation        * */
/* *             1.0.5 - 10/30/2002 - G.Juyn                                * */
/* *             - modified TERM/MEND processing for max(1, TERM_delay,     * */
/* *               interframe_delay)                                        * */
/* *             1.0.5 - 11/04/2002 - G.Juyn                                * */
/* *             - fixed layer- & frame-counting during read()              * */
/* *             - fixed goframe/golayer/gotime processing                  * */
/* *             1.0.5 - 01/19/2003 - G.Juyn                                * */
/* *             - B654627 - fixed SEGV when no gettickcount callback       * */
/* *             - B664383 - fixed typo                                     * */
/* *             - finalized changes in TERM/final_delay to elected proposal* */
/* *                                                                        * */
/* *             1.0.6 - 05/11/2003 - G. Juyn                               * */
/* *             - added conditionals around canvas update routines         * */
/* *             1.0.6 - 05/25/2003 - G.R-P                                 * */
/* *             - added MNG_SKIPCHUNK_cHNK footprint optimizations         * */
/* *             1.0.6 - 07/07/2003 - G.R-P                                 * */
/* *             - added conditionals around some JNG-supporting code       * */
/* *             - added conditionals around 16-bit supporting code         * */
/* *             - reversed some loops to use decrementing counter          * */
/* *             - combined init functions into one function                * */
/* *             1.0.6 - 07/10/2003 - G.R-P                                 * */
/* *             - replaced nested switches with simple init setup function * */
/* *             1.0.6 - 07/29/2003 - G.R-P                                 * */
/* *             - added conditionals around PAST chunk support             * */
/* *             1.0.6 - 08/17/2003 - G.R-P                                 * */
/* *             - added conditionals around non-VLC chunk support          * */
/* *                                                                        * */
/* *             1.0.7 - 11/27/2003 - R.A                                   * */
/* *             - added CANVAS_RGB565 and CANVAS_BGR565                    * */
/* *             1.0.7 - 12/06/2003 - R.A                                   * */
/* *             - added CANVAS_RGBA565 and CANVAS_BGRA565                  * */
/* *             1.0.7 - 01/25/2004 - J.S                                   * */
/* *             - added premultiplied alpha canvas' for RGBA, ARGB, ABGR   * */
/* *                                                                        * */
/* *             1.0.8 - 03/31/2004 - G.Juyn                                * */
/* *             - fixed problem with PAST usage where source > dest        * */
/* *             1.0.8 - 05/04/2004 - G.R-P.                                * */
/* *             - fixed misplaced 16-bit conditionals                      * */
/* *                                                                        * */
/* *             1.0.9 - 09/18/2004 - G.R-P.                                * */
/* *             - revised some SKIPCHUNK conditionals                      * */
/* *             1.0.9 - 10/10/2004 - G.R-P.                                * */
/* *             - added MNG_NO_1_2_4BIT_SUPPORT                            * */
/* *             1.0.9 - 10/14/2004 - G.Juyn                                * */
/* *             - added bgr565_a8 canvas-style (thanks to J. Elvander)     * */
/* *             1.0.9 - 12/11/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_DISPLAYCALLS              * */
/* *             1.0.9 - 12/20/2004 - G.Juyn                                * */
/* *             - cleaned up macro-invocations (thanks to D. Airlie)       * */
/* *                                                                        * */
/* *             1.0.10 - 07/06/2005 - G.R-P.                               * */
/* *             - added more SKIPCHUNK conditionals                        * */
/* *             1.0.10 - 12/28/2005 - G.R-P.                               * */
/* *             - added missing SKIPCHUNK_MAGN conditional                 * */
/* *             1.0.10 - 03/07/2006 - (thanks to W. Manthey)               * */
/* *             - added CANVAS_RGB555 and CANVAS_BGR555                    * */
/* *             1.0.10 - 04/08/2007 - G.Juyn                               * */
/* *             - fixed several compiler warnings                          * */
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
#include "libmng_chunks.h"
#include "libmng_objects.h"
#include "libmng_object_prc.h"
#include "libmng_memory.h"
#include "libmng_zlib.h"
#include "libmng_jpeg.h"
#include "libmng_cms.h"
#include "libmng_pixels.h"
#include "libmng_display.h"

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_DISPLAY_PROCS

/* ************************************************************************** */

MNG_LOCAL mng_retcode set_delay (mng_datap  pData,
                                 mng_uint32 iInterval)
{
  if (!iInterval)                      /* at least 1 msec please! */
    iInterval = 1;

  if (pData->bRunning)                 /* only when really displaying */
    if (!pData->fSettimer ((mng_handle)pData, iInterval))
      MNG_ERROR (pData, MNG_APPTIMERERROR);

#ifdef MNG_SUPPORT_DYNAMICMNG
  if ((!pData->bDynamic) || (pData->bRunning))
#else
  if (pData->bRunning)
#endif
    pData->bTimerset = MNG_TRUE;       /* and indicate so */

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_uint32 calculate_delay (mng_datap  pData,
                                      mng_uint32 iDelay)
{
  mng_uint32 iTicks   = pData->iTicks;
  mng_uint32 iWaitfor = 1;             /* default non-MNG delay */

  if (!iTicks)                         /* tick_count not specified ? */
    if (pData->eImagetype == mng_it_mng)
      iTicks = 1000;

  if (iTicks)
  {
    switch (pData->iSpeed)             /* honor speed modifier */
    {
      case mng_st_fast :
        {
          iWaitfor = (mng_uint32)(( 500 * iDelay) / iTicks);
          break;
        }
      case mng_st_slow :
        {
          iWaitfor = (mng_uint32)((3000 * iDelay) / iTicks);
          break;
        }
      case mng_st_slowest :
        {
          iWaitfor = (mng_uint32)((8000 * iDelay) / iTicks);
          break;
        }
      default :
        {
          iWaitfor = (mng_uint32)((1000 * iDelay) / iTicks);
        }
    }
  }

  return iWaitfor;
}

/* ************************************************************************** */
/* *                                                                        * */
/* * Progressive display refresh - does the call to the refresh callback    * */
/* * and sets the timer to allow the app to perform the actual refresh to   * */
/* * the screen (eg. process its main message-loop)                         * */
/* *                                                                        * */
/* ************************************************************************** */

mng_retcode mng_display_progressive_refresh (mng_datap  pData,
                                             mng_uint32 iInterval)
{
  {                                    /* let the app refresh first ? */
    if ((pData->bRunning) && (!pData->bSkipping) &&
        (pData->iUpdatetop < pData->iUpdatebottom) && (pData->iUpdateleft < pData->iUpdateright))
    {
      if (!pData->fRefresh (((mng_handle)pData),
                            pData->iUpdateleft, pData->iUpdatetop,
                            pData->iUpdateright  - pData->iUpdateleft,
                            pData->iUpdatebottom - pData->iUpdatetop))
        MNG_ERROR (pData, MNG_APPMISCERROR);

      pData->iUpdateleft   = 0;        /* reset update-region */
      pData->iUpdateright  = 0;
      pData->iUpdatetop    = 0;
      pData->iUpdatebottom = 0;        /* reset refreshneeded indicator */
      pData->bNeedrefresh  = MNG_FALSE;
                                       /* interval requested ? */
      if ((!pData->bFreezing) && (iInterval))
      {                                /* setup the timer */
        mng_retcode iRetcode = set_delay (pData, iInterval);

        if (iRetcode)                  /* on error bail out */
          return iRetcode;
      }
    }
  }

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* *                                                                        * */
/* * Generic display routines                                               * */
/* *                                                                        * */
/* ************************************************************************** */

MNG_LOCAL mng_retcode interframe_delay (mng_datap pData)
{
  mng_uint32  iWaitfor = 0;
  mng_uint32  iInterval;
  mng_uint32  iRuninterval;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INTERFRAME_DELAY, MNG_LC_START);
#endif

  {
#ifndef MNG_SKIPCHUNK_FRAM
    if (pData->iFramedelay > 0)        /* real delay ? */
    {                                  /* let the app refresh first ? */
      if ((pData->bRunning) && (!pData->bSkipping) &&
          (pData->iUpdatetop < pData->iUpdatebottom) && (pData->iUpdateleft < pData->iUpdateright))
        if (!pData->fRefresh (((mng_handle)pData),
                              pData->iUpdateleft,  pData->iUpdatetop,
                              pData->iUpdateright - pData->iUpdateleft,
                              pData->iUpdatebottom - pData->iUpdatetop))
          MNG_ERROR (pData, MNG_APPMISCERROR);

      pData->iUpdateleft   = 0;        /* reset update-region */
      pData->iUpdateright  = 0;
      pData->iUpdatetop    = 0;
      pData->iUpdatebottom = 0;        /* reset refreshneeded indicator */
      pData->bNeedrefresh  = MNG_FALSE;

#ifndef MNG_SKIPCHUNK_TERM
      if (pData->bOnlyfirstframe)      /* only processing first frame after TERM ? */
      {
        pData->iFramesafterTERM++;
                                       /* did we do a frame yet ? */
        if (pData->iFramesafterTERM > 1)
        {                              /* then that's it; just stop right here ! */
          pData->pCurraniobj = MNG_NULL;
          pData->bRunning    = MNG_FALSE;

          return MNG_NOERROR;
        }
      }
#endif

      if (pData->fGettickcount)
      {                                /* get current tickcount */
        pData->iRuntime = pData->fGettickcount ((mng_handle)pData);
                                       /* calculate interval since last sync-point */
        if (pData->iRuntime < pData->iSynctime)
          iRuninterval    = pData->iRuntime + ~pData->iSynctime + 1;
        else
          iRuninterval    = pData->iRuntime - pData->iSynctime;
                                       /* calculate actual run-time */
        if (pData->iRuntime < pData->iStarttime)
          pData->iRuntime = pData->iRuntime + ~pData->iStarttime + 1;
        else
          pData->iRuntime = pData->iRuntime - pData->iStarttime;
      }
      else
      {
        iRuninterval = 0;
      }

      iWaitfor = calculate_delay (pData, pData->iFramedelay);

      if (iWaitfor > iRuninterval)     /* delay necessary ? */
        iInterval = iWaitfor - iRuninterval;
      else
        iInterval = 1;                 /* force app to process messageloop */
                                       /* set the timer ? */
      if (((pData->bRunning) || (pData->bSearching) || (pData->bReading)) &&
          (!pData->bSkipping))
      {
        iRetcode = set_delay (pData, iInterval);

        if (iRetcode)                  /* on error bail out */
          return iRetcode;
      }
    }

    if (!pData->bSkipping)             /* increase frametime in advance */
      pData->iFrametime = pData->iFrametime + iWaitfor;
                                       /* setup for next delay */
    pData->iFramedelay = pData->iNextdelay;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INTERFRAME_DELAY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL void set_display_routine (mng_datap pData)
{                                        /* actively running ? */
  if (((pData->bRunning) || (pData->bSearching)) && (!pData->bSkipping))
  {
    switch (pData->iCanvasstyle)         /* determine display routine */
    {
#ifndef MNG_SKIPCANVAS_RGB8
      case MNG_CANVAS_RGB8    : { pData->fDisplayrow = (mng_fptr)mng_display_rgb8;     break; }
#endif
#ifndef MNG_SKIPCANVAS_RGBA8
      case MNG_CANVAS_RGBA8   : { pData->fDisplayrow = (mng_fptr)mng_display_rgba8;    break; }
#endif
#ifndef MNG_SKIPCANVAS_RGBA8_PM
      case MNG_CANVAS_RGBA8_PM: { pData->fDisplayrow = (mng_fptr)mng_display_rgba8_pm; break; }
#endif
#ifndef MNG_SKIPCANVAS_ARGB8
      case MNG_CANVAS_ARGB8   : { pData->fDisplayrow = (mng_fptr)mng_display_argb8;    break; }
#endif
#ifndef MNG_SKIPCANVAS_ARGB8_PM
      case MNG_CANVAS_ARGB8_PM: { pData->fDisplayrow = (mng_fptr)mng_display_argb8_pm; break; }
#endif
#ifndef MNG_SKIPCANVAS_RGB8_A8
      case MNG_CANVAS_RGB8_A8 : { pData->fDisplayrow = (mng_fptr)mng_display_rgb8_a8;  break; }
#endif
#ifndef MNG_SKIPCANVAS_BGR8
      case MNG_CANVAS_BGR8    : { pData->fDisplayrow = (mng_fptr)mng_display_bgr8;     break; }
#endif
#ifndef MNG_SKIPCANVAS_BGRX8
      case MNG_CANVAS_BGRX8   : { pData->fDisplayrow = (mng_fptr)mng_display_bgrx8;    break; }
#endif
#ifndef MNG_SKIPCANVAS_BGRA8
      case MNG_CANVAS_BGRA8   : { pData->fDisplayrow = (mng_fptr)mng_display_bgra8;    break; }
#endif
#ifndef MNG_SKIPCANVAS_BGRA8_PM
      case MNG_CANVAS_BGRA8_PM: { pData->fDisplayrow = (mng_fptr)mng_display_bgra8_pm; break; }
#endif
#ifndef MNG_SKIPCANVAS_ABGR8
      case MNG_CANVAS_ABGR8   : { pData->fDisplayrow = (mng_fptr)mng_display_abgr8;    break; }
#endif
#ifndef MNG_SKIPCANVAS_ABGR8_PM
      case MNG_CANVAS_ABGR8_PM: { pData->fDisplayrow = (mng_fptr)mng_display_abgr8_pm; break; }
#endif
#ifndef MNG_SKIPCANVAS_RGB565
      case MNG_CANVAS_RGB565  : { pData->fDisplayrow = (mng_fptr)mng_display_rgb565;   break; }
#endif
#ifndef MNG_SKIPCANVAS_RGBA565
      case MNG_CANVAS_RGBA565 : { pData->fDisplayrow = (mng_fptr)mng_display_rgba565;  break; }
#endif
#ifndef MNG_SKIPCANVAS_BGR565
      case MNG_CANVAS_BGR565  : { pData->fDisplayrow = (mng_fptr)mng_display_bgr565;   break; }
#endif
#ifndef MNG_SKIPCANVAS_BGRA565
      case MNG_CANVAS_BGRA565 : { pData->fDisplayrow = (mng_fptr)mng_display_bgra565;  break; }
#endif
#ifndef MNG_SKIPCANVAS_BGR565_A8
      case MNG_CANVAS_BGR565_A8 : { pData->fDisplayrow = (mng_fptr)mng_display_bgr565_a8;  break; }
#endif
#ifndef MNG_SKIPCANVAS_RGB555
      case MNG_CANVAS_RGB555  : { pData->fDisplayrow = (mng_fptr)mng_display_rgb555;  break; }
#endif
#ifndef MNG_SKIPCANVAS_BGR555
      case MNG_CANVAS_BGR555  : { pData->fDisplayrow = (mng_fptr)mng_display_bgr555;  break; }
#endif

#ifndef MNG_NO_16BIT_SUPPORT
/*      case MNG_CANVAS_RGB16   : { pData->fDisplayrow = (mng_fptr)mng_display_rgb16;    break; } */
/*      case MNG_CANVAS_RGBA16  : { pData->fDisplayrow = (mng_fptr)mng_display_rgba16;   break; } */
/*      case MNG_CANVAS_ARGB16  : { pData->fDisplayrow = (mng_fptr)mng_display_argb16;   break; } */
/*      case MNG_CANVAS_BGR16   : { pData->fDisplayrow = (mng_fptr)mng_display_bgr16;    break; } */
/*      case MNG_CANVAS_BGRA16  : { pData->fDisplayrow = (mng_fptr)mng_display_bgra16;   break; } */
/*      case MNG_CANVAS_ABGR16  : { pData->fDisplayrow = (mng_fptr)mng_display_abgr16;   break; } */
#endif
/*      case MNG_CANVAS_INDEX8  : { pData->fDisplayrow = (mng_fptr)mng_display_index8;   break; } */
/*      case MNG_CANVAS_INDEXA8 : { pData->fDisplayrow = (mng_fptr)mng_display_indexa8;  break; } */
/*      case MNG_CANVAS_AINDEX8 : { pData->fDisplayrow = (mng_fptr)mng_display_aindex8;  break; } */
/*      case MNG_CANVAS_GRAY8   : { pData->fDisplayrow = (mng_fptr)mng_display_gray8;    break; } */
/*      case MNG_CANVAS_AGRAY8  : { pData->fDisplayrow = (mng_fptr)mng_display_agray8;   break; } */
/*      case MNG_CANVAS_GRAYA8  : { pData->fDisplayrow = (mng_fptr)mng_display_graya8;   break; } */
#ifndef MNG_NO_16BIT_SUPPORT
/*      case MNG_CANVAS_GRAY16  : { pData->fDisplayrow = (mng_fptr)mng_display_gray16;   break; } */
/*      case MNG_CANVAS_GRAYA16 : { pData->fDisplayrow = (mng_fptr)mng_display_graya16;  break; } */
/*      case MNG_CANVAS_AGRAY16 : { pData->fDisplayrow = (mng_fptr)mng_display_agray16;  break; } */
#endif
/*      case MNG_CANVAS_DX15    : { pData->fDisplayrow = (mng_fptr)mng_display_dx15;     break; } */
/*      case MNG_CANVAS_DX16    : { pData->fDisplayrow = (mng_fptr)mng_display_dx16;     break; } */
    }
  }

  return;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode load_bkgdlayer (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_LOAD_BKGDLAYER, MNG_LC_START);
#endif
                                       /* actively running ? */
  if (((pData->bRunning) || (pData->bSearching)) && (!pData->bSkipping))
  {
    mng_int32   iY;
    mng_retcode iRetcode;
    mng_bool    bColorcorr   = MNG_FALSE;
                                       /* save values */
    mng_int32   iDestl       = pData->iDestl;
    mng_int32   iDestr       = pData->iDestr;
    mng_int32   iDestt       = pData->iDestt;
    mng_int32   iDestb       = pData->iDestb;
    mng_int32   iSourcel     = pData->iSourcel;
    mng_int32   iSourcer     = pData->iSourcer;
    mng_int32   iSourcet     = pData->iSourcet;
    mng_int32   iSourceb     = pData->iSourceb;
    mng_int8    iPass        = pData->iPass;
    mng_int32   iRow         = pData->iRow;
    mng_int32   iRowinc      = pData->iRowinc;
    mng_int32   iCol         = pData->iCol;
    mng_int32   iColinc      = pData->iColinc;
    mng_int32   iRowsamples  = pData->iRowsamples;
    mng_int32   iRowsize     = pData->iRowsize;
    mng_uint8p  pPrevrow     = pData->pPrevrow;
    mng_uint8p  pRGBArow     = pData->pRGBArow;
    mng_bool    bIsRGBA16    = pData->bIsRGBA16;
    mng_bool    bIsOpaque    = pData->bIsOpaque;
    mng_fptr    fCorrectrow  = pData->fCorrectrow;
    mng_fptr    fDisplayrow  = pData->fDisplayrow;
    mng_fptr    fRetrieverow = pData->fRetrieverow;
    mng_objectp pCurrentobj  = pData->pCurrentobj;
    mng_objectp pRetrieveobj = pData->pRetrieveobj;

    pData->iDestl   = 0;               /* determine clipping region */
    pData->iDestt   = 0;
    pData->iDestr   = pData->iWidth;
    pData->iDestb   = pData->iHeight;

#ifndef MNG_SKIPCHUNK_FRAM
    if (pData->bFrameclipping)         /* frame clipping specified ? */
    {
      pData->iDestl = MAX_COORD (pData->iDestl,  pData->iFrameclipl);
      pData->iDestt = MAX_COORD (pData->iDestt,  pData->iFrameclipt);
      pData->iDestr = MIN_COORD (pData->iDestr,  pData->iFrameclipr);
      pData->iDestb = MIN_COORD (pData->iDestb,  pData->iFrameclipb);
    }
#endif
                                       /* anything to clear ? */
    if ((pData->iDestr >= pData->iDestl) && (pData->iDestb >= pData->iDestt))
    {
      pData->iPass       = -1;         /* these are the object's dimensions now */
      pData->iRow        = 0;
      pData->iRowinc     = 1;
      pData->iCol        = 0;
      pData->iColinc     = 1;
      pData->iRowsamples = pData->iWidth;
      pData->iRowsize    = pData->iRowsamples << 2;
      pData->bIsRGBA16   = MNG_FALSE;  /* let's keep it simple ! */
      pData->bIsOpaque   = MNG_TRUE;

      pData->iSourcel    = 0;          /* source relative to destination */
      pData->iSourcer    = pData->iDestr - pData->iDestl;
      pData->iSourcet    = 0;
      pData->iSourceb    = pData->iDestb - pData->iDestt;

      set_display_routine (pData);     /* determine display routine */
                                       /* default restore using preset BG color */
      pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_bgcolor;

#ifndef MNG_SKIPCHUNK_bKGD
      if (((pData->eImagetype == mng_it_png) || (pData->eImagetype == mng_it_jng)) &&
          (pData->bUseBKGD))
      {                                /* prefer bKGD in PNG/JNG */
        if (!pData->pCurrentobj)
          pData->pCurrentobj = pData->pObjzero;

        if (((mng_imagep)pData->pCurrentobj)->pImgbuf->bHasBKGD)
        {
          pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_bkgd;
          bColorcorr          = MNG_TRUE;
        }
      }
#endif

      if (pData->fGetbkgdline)         /* background-canvas-access callback set ? */
      {
        switch (pData->iBkgdstyle)
        {
#ifndef MNG_SKIPCANVAS_RGB8
          case MNG_CANVAS_RGB8    : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_rgb8;    break; }
#endif
#ifndef MNG_SKIPCANVAS_BGR8
          case MNG_CANVAS_BGR8    : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_bgr8;    break; }
#endif
#ifndef MNG_SKIPCANVAS_BGRX8
          case MNG_CANVAS_BGRX8   : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_bgrx8;   break; }
#endif
#ifndef MNG_SKIPCANVAS_BGR565
          case MNG_CANVAS_BGR565  : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_bgr565;  break; }
#endif
#ifndef MNG_SKIPCANVAS_RGB565
          case MNG_CANVAS_RGB565  : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_rgb565;  break; }
#endif
#ifndef MNG_NO_16BIT_SUPPORT
  /*        case MNG_CANVAS_RGB16   : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_rgb16;   break; } */
  /*        case MNG_CANVAS_BGR16   : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_bgr16;   break; } */
#endif
  /*        case MNG_CANVAS_INDEX8  : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_index8;  break; } */
  /*        case MNG_CANVAS_GRAY8   : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_gray8;   break; } */
#ifndef MNG_NO_16BIT_SUPPORT
  /*        case MNG_CANVAS_GRAY16  : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_gray16;  break; } */
#endif
  /*        case MNG_CANVAS_DX15    : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_dx15;    break; } */
  /*        case MNG_CANVAS_DX16    : { pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_dx16;    break; } */
        }
      }

#ifndef MNG_SKIPCHUNK_BACK
      if (pData->bHasBACK)
      {                                /* background image ? */
        if ((pData->iBACKmandatory & 0x02) && (pData->iBACKimageid))
        {
          pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_backcolor;
          bColorcorr          = MNG_TRUE;
        }
        else                           /* background color ? */
        if (pData->iBACKmandatory & 0x01)
        {
          pData->fRestbkgdrow = (mng_fptr)mng_restore_bkgd_backcolor;
          bColorcorr          = MNG_TRUE;
        }
      }
#endif

      pData->fCorrectrow = MNG_NULL;   /* default no color-correction */

      if (bColorcorr)                  /* do we have to do color-correction ? */
      {
#ifdef MNG_NO_CMS
        iRetcode = MNG_NOERROR;
#else
#if defined(MNG_FULL_CMS)              /* determine color-management routine */
        iRetcode = mng_init_full_cms   (pData, MNG_TRUE, MNG_FALSE, MNG_FALSE);
#elif defined(MNG_GAMMA_ONLY)
        iRetcode = mng_init_gamma_only (pData, MNG_TRUE, MNG_FALSE, MNG_FALSE);
#elif defined(MNG_APP_CMS)
        iRetcode = mng_init_app_cms    (pData, MNG_TRUE, MNG_FALSE, MNG_FALSE);
#endif
        if (iRetcode)                  /* on error bail out */
          return iRetcode;
#endif /* MNG_NO_CMS */
      }
                                       /* get a temporary row-buffer */
      MNG_ALLOC (pData, pData->pRGBArow, pData->iRowsize);

      iY       = pData->iDestt;        /* this is where we start */
      iRetcode = MNG_NOERROR;          /* so far, so good */

      while ((!iRetcode) && (iY < pData->iDestb))
      {                                /* restore a background row */
        iRetcode = ((mng_restbkgdrow)pData->fRestbkgdrow) (pData);
                                       /* color correction ? */
        if ((!iRetcode) && (pData->fCorrectrow))
          iRetcode = ((mng_correctrow)pData->fCorrectrow) (pData);

        if (!iRetcode)                 /* so... display it */
          iRetcode = ((mng_displayrow)pData->fDisplayrow) (pData);

        if (!iRetcode)
          iRetcode = mng_next_row (pData);

        iY++;                          /* and next line */
      }
                                       /* drop the temporary row-buffer */
      MNG_FREE (pData, pData->pRGBArow, pData->iRowsize);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;

#if defined(MNG_FULL_CMS)              /* cleanup cms stuff */
      if (bColorcorr)                  /* did we do color-correction ? */
      {
        iRetcode = mng_clear_cms (pData);

        if (iRetcode)                  /* on error bail out */
          return iRetcode;
      }
#endif
#ifndef MNG_SKIPCHUNK_BACK
                                       /* background image ? */
      if ((pData->bHasBACK) && (pData->iBACKmandatory & 0x02) && (pData->iBACKimageid))
      {
        mng_imagep pImage;
                                       /* let's find that object then */
        pData->pRetrieveobj = mng_find_imageobject (pData, pData->iBACKimageid);
        pImage              = (mng_imagep)pData->pRetrieveobj;
                                       /* exists, viewable and visible ? */
        if ((pImage) && (pImage->bViewable) && (pImage->bVisible))
        {                              /* will it fall within the target region ? */
          if ((pImage->iPosx < pData->iDestr) && (pImage->iPosy < pData->iDestb)             &&
              ((pData->iBACKtile) ||
               ((pImage->iPosx + (mng_int32)pImage->pImgbuf->iWidth  >= pData->iDestl) &&
                (pImage->iPosy + (mng_int32)pImage->pImgbuf->iHeight >= pData->iDestt)    )) &&
              ((!pImage->bClipped) ||
               ((pImage->iClipl <= pImage->iClipr) && (pImage->iClipt <= pImage->iClipb)     &&
                (pImage->iClipl < pData->iDestr)   && (pImage->iClipr >= pData->iDestl)      &&
                (pImage->iClipt < pData->iDestb)   && (pImage->iClipb >= pData->iDestt)         )))
          {                            /* right; we've got ourselves something to do */
            if (pImage->bClipped)      /* clip output region with image's clipping region ? */
            {
              if (pImage->iClipl > pData->iDestl)
                pData->iDestl = pImage->iClipl;
              if (pImage->iClipr < pData->iDestr)
                pData->iDestr = pImage->iClipr;
              if (pImage->iClipt > pData->iDestt)
                pData->iDestt = pImage->iClipt;
              if (pImage->iClipb < pData->iDestb)
                pData->iDestb = pImage->iClipb;
            }
                                       /* image offset does some extra clipping too ! */
            if (pImage->iPosx > pData->iDestl)
              pData->iDestl = pImage->iPosx;
            if (pImage->iPosy > pData->iDestt)
              pData->iDestt = pImage->iPosy;

            if (!pData->iBACKtile)     /* without tiling further clipping is needed */
            {
              if (pImage->iPosx + (mng_int32)pImage->pImgbuf->iWidth  < pData->iDestr)
                pData->iDestr = pImage->iPosx + (mng_int32)pImage->pImgbuf->iWidth;
              if (pImage->iPosy + (mng_int32)pImage->pImgbuf->iHeight < pData->iDestb)
                pData->iDestb = pImage->iPosy + (mng_int32)pImage->pImgbuf->iHeight;
            }
            
            pData->iSourcel    = 0;    /* source relative to destination */
            pData->iSourcer    = pData->iDestr - pData->iDestl;
            pData->iSourcet    = 0;
            pData->iSourceb    = pData->iDestb - pData->iDestt;
                                       /* 16-bit background ? */

#ifdef MNG_NO_16BIT_SUPPORT
            pData->bIsRGBA16   = MNG_FALSE;
#else
            pData->bIsRGBA16      = (mng_bool)(pImage->pImgbuf->iBitdepth > 8);
#endif
                                       /* let restore routine know the offsets !!! */
            pData->iBackimgoffsx  = pImage->iPosx;
            pData->iBackimgoffsy  = pImage->iPosy;
            pData->iBackimgwidth  = pImage->pImgbuf->iWidth;
            pData->iBackimgheight = pImage->pImgbuf->iHeight;
            pData->iRow           = 0; /* start at the top again !! */
                                       /* determine background object retrieval routine */
            switch (pImage->pImgbuf->iColortype)
            {
              case  0 : {
#ifndef MNG_NO_16BIT_SUPPORT
                          if (pImage->pImgbuf->iBitdepth > 8)
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_g16;
                          else
#endif
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_g8;

                          pData->bIsOpaque      = (mng_bool)(!pImage->pImgbuf->bHasTRNS);
                          break;
                        }

              case  2 : {
#ifndef MNG_NO_16BIT_SUPPORT
                          if (pImage->pImgbuf->iBitdepth > 8)
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb16;
                          else
#endif
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb8;

                          pData->bIsOpaque      = (mng_bool)(!pImage->pImgbuf->bHasTRNS);
                          break;
                        }

              case  3 : { pData->fRetrieverow   = (mng_fptr)mng_retrieve_idx8;
                          pData->bIsOpaque      = (mng_bool)(!pImage->pImgbuf->bHasTRNS);
                          break;
                        }

              case  4 : { 
#ifndef MNG_NO_16BIT_SUPPORT
			if (pImage->pImgbuf->iBitdepth > 8)
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_ga16;
                          else
#endif
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_ga8;

                          pData->bIsOpaque      = MNG_FALSE;
                          break;
                        }

              case  6 : {
#ifndef MNG_NO_16BIT_SUPPORT
                          if (pImage->pImgbuf->iBitdepth > 8)
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba16;
                          else
#endif
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba8;

                          pData->bIsOpaque      = MNG_FALSE;
                          break;
                        }

              case  8 : {
#ifndef MNG_NO_16BIT_SUPPORT
                          if (pImage->pImgbuf->iBitdepth > 8)
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_g16;
                          else
#endif
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_g8;

                          pData->bIsOpaque      = MNG_TRUE;
                          break;
                        }

              case 10 : {
#ifndef MNG_NO_16BIT_SUPPORT
                          if (pImage->pImgbuf->iBitdepth > 8)
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb16;
                          else
#endif
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb8;

                          pData->bIsOpaque      = MNG_TRUE;
                          break;
                        }

              case 12 : {
#ifndef MNG_NO_16BIT_SUPPORT
                          if (pImage->pImgbuf->iBitdepth > 8)
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_ga16;
                          else
#endif
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_ga8;

                          pData->bIsOpaque      = MNG_FALSE;
                          break;
                        }

              case 14 : {
#ifndef MNG_NO_16BIT_SUPPORT
                          if (pImage->pImgbuf->iBitdepth > 8)
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba16;
                          else
#endif
                            pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba8;

                          pData->bIsOpaque      = MNG_FALSE;
                          break;
                        }
            }

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
            if (iRetcode)              /* on error bail out */
              return iRetcode;
#endif /* MNG_NO_CMS */
                                       /* get temporary row-buffers */
            MNG_ALLOC (pData, pData->pPrevrow, pData->iRowsize);
            MNG_ALLOC (pData, pData->pRGBArow, pData->iRowsize);

            iY       = pData->iDestt;  /* this is where we start */
            iRetcode = MNG_NOERROR;    /* so far, so good */

            while ((!iRetcode) && (iY < pData->iDestb))
            {                          /* restore a background row */
              iRetcode = mng_restore_bkgd_backimage (pData);
                                       /* color correction ? */
              if ((!iRetcode) && (pData->fCorrectrow))
                iRetcode = ((mng_correctrow)pData->fCorrectrow) (pData);

              if (!iRetcode)           /* so... display it */
                iRetcode = ((mng_displayrow)pData->fDisplayrow) (pData);

              if (!iRetcode)
                iRetcode = mng_next_row (pData);

              iY++;                    /* and next line */
            }
                                       /* drop temporary row-buffers */
            MNG_FREE (pData, pData->pRGBArow, pData->iRowsize);
            MNG_FREE (pData, pData->pPrevrow, pData->iRowsize);

            if (iRetcode)              /* on error bail out */
              return iRetcode;

#if defined(MNG_FULL_CMS)              /* cleanup cms stuff */
            iRetcode = mng_clear_cms (pData);

            if (iRetcode)              /* on error bail out */
              return iRetcode;
#endif
          }
        }
      }
#endif
    }

    pData->iDestl       = iDestl;      /* restore values */
    pData->iDestr       = iDestr;
    pData->iDestt       = iDestt;
    pData->iDestb       = iDestb;
    pData->iSourcel     = iSourcel;
    pData->iSourcer     = iSourcer;
    pData->iSourcet     = iSourcet;
    pData->iSourceb     = iSourceb;
    pData->iPass        = iPass;
    pData->iRow         = iRow;
    pData->iRowinc      = iRowinc;
    pData->iCol         = iCol;
    pData->iColinc      = iColinc;
    pData->iRowsamples  = iRowsamples;
    pData->iRowsize     = iRowsize;
    pData->pPrevrow     = pPrevrow;
    pData->pRGBArow     = pRGBArow;
    pData->bIsRGBA16    = bIsRGBA16;
    pData->bIsOpaque    = bIsOpaque;
    pData->fCorrectrow  = fCorrectrow;
    pData->fDisplayrow  = fDisplayrow; 
    pData->fRetrieverow = fRetrieverow;
    pData->pCurrentobj  = pCurrentobj;
    pData->pRetrieveobj = pRetrieveobj;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_LOAD_BKGDLAYER, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode clear_canvas (mng_datap pData)
{
  mng_int32   iY;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CLEAR_CANVAS, MNG_LC_START);
#endif

  pData->iDestl      = 0;              /* clipping region is full canvas! */
  pData->iDestt      = 0;
  pData->iDestr      = pData->iWidth;
  pData->iDestb      = pData->iHeight;

  pData->iSourcel    = 0;              /* source is same as destination */
  pData->iSourcer    = pData->iWidth;
  pData->iSourcet    = 0;
  pData->iSourceb    = pData->iHeight;

  pData->iPass       = -1;             /* these are the object's dimensions now */
  pData->iRow        = 0;
  pData->iRowinc     = 1;
  pData->iCol        = 0;
  pData->iColinc     = 1;
  pData->iRowsamples = pData->iWidth;
  pData->iRowsize    = pData->iRowsamples << 2;
  pData->bIsRGBA16   = MNG_FALSE;      /* let's keep it simple ! */
  pData->bIsOpaque   = MNG_TRUE;

  set_display_routine (pData);         /* determine display routine */
                                       /* get a temporary row-buffer */
                                       /* it's transparent black by default!! */
  MNG_ALLOC (pData, pData->pRGBArow, pData->iRowsize);

  iY       = pData->iDestt;            /* this is where we start */
  iRetcode = MNG_NOERROR;              /* so far, so good */

  while ((!iRetcode) && (iY < pData->iDestb))
  {                                    /* clear a row then */
    iRetcode = ((mng_displayrow)pData->fDisplayrow) (pData);

    if (!iRetcode)
      iRetcode = mng_next_row (pData); /* adjust variables for next row */

    iY++;                              /* and next line */
  }
                                       /* drop the temporary row-buffer */
  MNG_FREE (pData, pData->pRGBArow, pData->iRowsize);

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CLEAR_CANVAS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode next_frame (mng_datap  pData,
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
{
  mng_retcode iRetcode = MNG_NOERROR;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_NEXT_FRAME, MNG_LC_START);
#endif

  if (!pData->iBreakpoint)             /* no previous break here ? */
  {
#ifndef MNG_SKIPCHUNK_FRAM
    mng_uint8 iOldmode = pData->iFramemode;
                                       /* interframe delay required ? */
    if ((iOldmode == 2) || (iOldmode == 4))
    {
      if ((pData->iFrameseq) && (iFramemode != 1) && (iFramemode != 3))
        iRetcode = interframe_delay (pData);
      else
        pData->iFramedelay = pData->iNextdelay;
    }
    else
    {                                  /* delay before inserting background layer? */
      if ((pData->bFramedone) && (iFramemode == 4))
        iRetcode = interframe_delay (pData);
    }

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
                                       /* now we'll assume we're in the next frame! */
    if (iFramemode)                    /* save the new framing mode ? */
    {
      pData->iFRAMmode  = iFramemode;
      pData->iFramemode = iFramemode;
    }
    else                               /* reload default */
      pData->iFramemode = pData->iFRAMmode;

    if (iChangedelay)                  /* delay changed ? */
    {
      pData->iNextdelay = iDelay;      /* for *after* next subframe */

      if ((iOldmode == 2) || (iOldmode == 4))
        pData->iFramedelay = pData->iFRAMdelay;

      if (iChangedelay == 2)           /* also overall ? */
        pData->iFRAMdelay = iDelay;
    }
    else
    {                                  /* reload default */
      pData->iNextdelay = pData->iFRAMdelay;
    }

    if (iChangetimeout)                /* timeout changed ? */
    {                                  /* for next subframe */
      pData->iFrametimeout = iTimeout;

      if ((iChangetimeout == 2) ||     /* also overall ? */
          (iChangetimeout == 4) ||
          (iChangetimeout == 6) ||
          (iChangetimeout == 8))
        pData->iFRAMtimeout = iTimeout;
    }
    else                               /* reload default */
      pData->iFrametimeout = pData->iFRAMtimeout;

    if (iChangeclipping)               /* clipping changed ? */
    {
      pData->bFrameclipping = MNG_TRUE;

      if (!iCliptype)                  /* absolute ? */
      {
        pData->iFrameclipl = iClipl;
        pData->iFrameclipr = iClipr;
        pData->iFrameclipt = iClipt;
        pData->iFrameclipb = iClipb;
      }
      else                             /* relative */
      {
        pData->iFrameclipl = pData->iFrameclipl + iClipl;
        pData->iFrameclipr = pData->iFrameclipr + iClipr;
        pData->iFrameclipt = pData->iFrameclipt + iClipt;
        pData->iFrameclipb = pData->iFrameclipb + iClipb;
      }

      if (iChangeclipping == 2)        /* also overall ? */
      {
        pData->bFRAMclipping = MNG_TRUE;

        if (!iCliptype)                /* absolute ? */
        {
          pData->iFRAMclipl = iClipl;
          pData->iFRAMclipr = iClipr;
          pData->iFRAMclipt = iClipt;
          pData->iFRAMclipb = iClipb;
        }
        else                           /* relative */
        {
          pData->iFRAMclipl = pData->iFRAMclipl + iClipl;
          pData->iFRAMclipr = pData->iFRAMclipr + iClipr;
          pData->iFRAMclipt = pData->iFRAMclipt + iClipt;
          pData->iFRAMclipb = pData->iFRAMclipb + iClipb;
        }
      }
    }
    else
    {                                  /* reload defaults */
      pData->bFrameclipping = pData->bFRAMclipping;
      pData->iFrameclipl    = pData->iFRAMclipl;
      pData->iFrameclipr    = pData->iFRAMclipr;
      pData->iFrameclipt    = pData->iFRAMclipt;
      pData->iFrameclipb    = pData->iFRAMclipb;
    }
#endif
  }

  if (!pData->bTimerset)               /* timer still off ? */
  {
    if (
#ifndef MNG_SKIPCHUNK_FRAM
       (pData->iFramemode == 4) ||    /* insert background layer after a new frame */
#endif
        (!pData->iLayerseq))           /* and certainly before the very first layer */
      iRetcode = load_bkgdlayer (pData);

    if (iRetcode)                      /* on error bail out */
      return iRetcode;

    pData->iFrameseq++;                /* count the frame ! */
    pData->bFramedone = MNG_TRUE;      /* and indicate we've done one */
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_NEXT_FRAME, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode next_layer (mng_datap pData)
{
  mng_imagep  pImage;
  mng_retcode iRetcode = MNG_NOERROR;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_NEXT_LAYER, MNG_LC_START);
#endif

#ifndef MNG_SKIPCHUNK_FRAM
  if (!pData->iBreakpoint)             /* no previous break here ? */
  {                                    /* interframe delay required ? */
    if ((pData->eImagetype == mng_it_mng) && (pData->iLayerseq) &&
        ((pData->iFramemode == 1) || (pData->iFramemode == 3)))
      iRetcode = interframe_delay (pData);
    else
      pData->iFramedelay = pData->iNextdelay;

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }
#endif

  if (!pData->bTimerset)               /* timer still off ? */
  {
    if (!pData->iLayerseq)             /* restore background for the very first layer ? */
    {                                  /* wait till IDAT/JDAT for PNGs & JNGs !!! */
      if ((pData->eImagetype == mng_it_png) || (pData->eImagetype == mng_it_jng))
        pData->bRestorebkgd = MNG_TRUE;
      else
      {                                /* for MNG we do it right away */
        iRetcode = load_bkgdlayer (pData);
        pData->iLayerseq++;            /* and it counts as a layer then ! */
      }
    }
#ifndef MNG_SKIPCHUNK_FRAM
    else
    if (pData->iFramemode == 3)        /* restore background for each layer ? */
      iRetcode = load_bkgdlayer (pData);
#endif

    if (iRetcode)                      /* on error bail out */
      return iRetcode;

#ifndef MNG_NO_DELTA_PNG
    if (pData->bHasDHDR)               /* processing a delta-image ? */
      pImage = (mng_imagep)pData->pDeltaImage;
    else
#endif
      pImage = (mng_imagep)pData->pCurrentobj;

    if (!pImage)                       /* not an active object ? */
      pImage = (mng_imagep)pData->pObjzero;
                                       /* determine display rectangle */
    pData->iDestl   = MAX_COORD ((mng_int32)0,   pImage->iPosx);
    pData->iDestt   = MAX_COORD ((mng_int32)0,   pImage->iPosy);
                                       /* is it a valid buffer ? */
    if ((pImage->pImgbuf->iWidth) && (pImage->pImgbuf->iHeight))
    {
      pData->iDestr = MIN_COORD ((mng_int32)pData->iWidth,
                                 pImage->iPosx + (mng_int32)pImage->pImgbuf->iWidth );
      pData->iDestb = MIN_COORD ((mng_int32)pData->iHeight,
                                 pImage->iPosy + (mng_int32)pImage->pImgbuf->iHeight);
    }
    else                               /* it's a single image ! */
    {
      pData->iDestr = MIN_COORD ((mng_int32)pData->iWidth,
                                 (mng_int32)pData->iDatawidth );
      pData->iDestb = MIN_COORD ((mng_int32)pData->iHeight,
                                 (mng_int32)pData->iDataheight);
    }

#ifndef MNG_SKIPCHUNK_FRAM
    if (pData->bFrameclipping)         /* frame clipping specified ? */
    {
      pData->iDestl = MAX_COORD (pData->iDestl,  pData->iFrameclipl);
      pData->iDestt = MAX_COORD (pData->iDestt,  pData->iFrameclipt);
      pData->iDestr = MIN_COORD (pData->iDestr,  pData->iFrameclipr);
      pData->iDestb = MIN_COORD (pData->iDestb,  pData->iFrameclipb);
    }
#endif

    if (pImage->bClipped)              /* is the image clipped itself ? */
    {
      pData->iDestl = MAX_COORD (pData->iDestl,  pImage->iClipl);
      pData->iDestt = MAX_COORD (pData->iDestt,  pImage->iClipt);
      pData->iDestr = MIN_COORD (pData->iDestr,  pImage->iClipr);
      pData->iDestb = MIN_COORD (pData->iDestb,  pImage->iClipb);
    }
                                       /* determine source starting point */
    pData->iSourcel = MAX_COORD ((mng_int32)0,   pData->iDestl - pImage->iPosx);
    pData->iSourcet = MAX_COORD ((mng_int32)0,   pData->iDestt - pImage->iPosy);

    if ((pImage->pImgbuf->iWidth) && (pImage->pImgbuf->iHeight))
    {                                  /* and maximum size  */
      pData->iSourcer = MIN_COORD ((mng_int32)pImage->pImgbuf->iWidth,
                                   pData->iSourcel + pData->iDestr - pData->iDestl);
      pData->iSourceb = MIN_COORD ((mng_int32)pImage->pImgbuf->iHeight,
                                   pData->iSourcet + pData->iDestb - pData->iDestt);
    }
    else                               /* it's a single image ! */
    {
      pData->iSourcer = pData->iSourcel + pData->iDestr - pData->iDestl;
      pData->iSourceb = pData->iSourcet + pData->iDestb - pData->iDestt;
    }

    pData->iLayerseq++;                /* count the layer ! */
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_NEXT_LAYER, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_display_image (mng_datap  pData,
                               mng_imagep pImage,
                               mng_bool   bLayeradvanced)
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DISPLAY_IMAGE, MNG_LC_START);
#endif
                                       /* actively running ? */
#ifndef MNG_SKIPCHUNK_MAGN
  if (((pData->bRunning) || (pData->bSearching)) && (!pData->bSkipping))
  {
    if ( (!pData->iBreakpoint) &&      /* needs magnification ? */
         ( (pImage->iMAGN_MethodX) || (pImage->iMAGN_MethodY) ) )
    {
      iRetcode = mng_magnify_imageobject (pData, pImage);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
    }
  }
#endif

  pData->pRetrieveobj = pImage;        /* so retrieve-row and color-correction can find it */

  if (!bLayeradvanced)                 /* need to advance the layer ? */
  {
    mng_imagep pSave    = pData->pCurrentobj;
    pData->pCurrentobj  = pImage;
    next_layer (pData);                /* advance to next layer */
    pData->pCurrentobj  = pSave;
  }
                                       /* need to restore the background ? */
  if ((!pData->bTimerset) && (pData->bRestorebkgd))
  {
    mng_imagep pSave    = pData->pCurrentobj;
    pData->pCurrentobj  = pImage;
    pData->bRestorebkgd = MNG_FALSE;
    iRetcode            = load_bkgdlayer (pData);
    pData->pCurrentobj  = pSave;

    if (iRetcode)                      /* on error bail out */
      return iRetcode;

    pData->iLayerseq++;                /* and it counts as a layer then ! */
  }
                                       /* actively running ? */
  if (((pData->bRunning) || (pData->bSearching)) && (!pData->bSkipping))
  {
    if (!pData->bTimerset)             /* all systems still go ? */
    {
      pData->iBreakpoint = 0;          /* let's make absolutely sure... */
                                       /* anything to display ? */
      if ((pData->iDestr >= pData->iDestl) && (pData->iDestb >= pData->iDestt))
      {
        mng_int32 iY;

        set_display_routine (pData);   /* determine display routine */
                                       /* and image-buffer retrieval routine */
        switch (pImage->pImgbuf->iColortype)
        {
          case  0 : {
#ifndef MNG_NO_16BIT_SUPPORT
                      if (pImage->pImgbuf->iBitdepth > 8)
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_g16;
                      else
#endif
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_g8;

                      pData->bIsOpaque      = (mng_bool)(!pImage->pImgbuf->bHasTRNS);
                      break;
                    }

          case  2 : {
#ifndef MNG_NO_16BIT_SUPPORT
                      if (pImage->pImgbuf->iBitdepth > 8)
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb16;
                      else
#endif
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb8;

                      pData->bIsOpaque      = (mng_bool)(!pImage->pImgbuf->bHasTRNS);
                      break;
                    }


          case  3 : { pData->fRetrieverow   = (mng_fptr)mng_retrieve_idx8;
                      pData->bIsOpaque      = (mng_bool)(!pImage->pImgbuf->bHasTRNS);
                      break;
                    }


          case  4 : {
#ifndef MNG_NO_16BIT_SUPPORT
                      if (pImage->pImgbuf->iBitdepth > 8)
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_ga16;
                      else
#endif
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_ga8;

                      pData->bIsOpaque      = MNG_FALSE;
                      break;
                    }


          case  6 : {
#ifndef MNG_NO_16BIT_SUPPORT
                      if (pImage->pImgbuf->iBitdepth > 8)
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba16;
                      else
#endif
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba8;

                      pData->bIsOpaque      = MNG_FALSE;
                      break;
                    }

          case  8 : {
#ifndef MNG_NO_16BIT_SUPPORT
                      if (pImage->pImgbuf->iBitdepth > 8)
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_g16;
                      else
#endif
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_g8;

                      pData->bIsOpaque      = MNG_TRUE;
                      break;
                    }

          case 10 : {
#ifndef MNG_NO_16BIT_SUPPORT
                      if (pImage->pImgbuf->iBitdepth > 8)
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb16;
                      else
#endif
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb8;

                      pData->bIsOpaque      = MNG_TRUE;
                      break;
                    }


          case 12 : {
#ifndef MNG_NO_16BIT_SUPPORT
                      if (pImage->pImgbuf->iBitdepth > 8)
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_ga16;
                      else
#endif
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_ga8;

                      pData->bIsOpaque      = MNG_FALSE;
                      break;
                    }


          case 14 : {
#ifndef MNG_NO_16BIT_SUPPORT
                      if (pImage->pImgbuf->iBitdepth > 8)
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba16;
                      else
#endif
                        pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba8;

                      pData->bIsOpaque      = MNG_FALSE;
                      break;
                    }

        }

        pData->iPass       = -1;       /* these are the object's dimensions now */
        pData->iRow        = pData->iSourcet;
        pData->iRowinc     = 1;
        pData->iCol        = 0;
        pData->iColinc     = 1;
        pData->iRowsamples = pImage->pImgbuf->iWidth;
        pData->iRowsize    = pData->iRowsamples << 2;
        pData->bIsRGBA16   = MNG_FALSE;
                                       /* adjust for 16-bit object ? */
#ifndef MNG_NO_16BIT_SUPPORT
        if (pImage->pImgbuf->iBitdepth > 8)
        {
          pData->bIsRGBA16 = MNG_TRUE;
          pData->iRowsize  = pData->iRowsamples << 3;
        }
#endif

        pData->fCorrectrow = MNG_NULL; /* default no color-correction */

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
        if (iRetcode)                  /* on error bail out */
          return iRetcode;
#endif /* MNG_NO_CMS */
                                       /* get a temporary row-buffer */
        MNG_ALLOC (pData, pData->pRGBArow, pData->iRowsize);

        iY = pData->iSourcet;          /* this is where we start */

        while ((!iRetcode) && (iY < pData->iSourceb))
        {                              /* get a row */
          iRetcode = ((mng_retrieverow)pData->fRetrieverow) (pData);
                                       /* color correction ? */
          if ((!iRetcode) && (pData->fCorrectrow))
            iRetcode = ((mng_correctrow)pData->fCorrectrow) (pData);

          if (!iRetcode)               /* so... display it */
            iRetcode = ((mng_displayrow)pData->fDisplayrow) (pData);

          if (!iRetcode)               /* adjust variables for next row */
            iRetcode = mng_next_row (pData);

          iY++;                        /* and next line */
        }
                                       /* drop the temporary row-buffer */
        MNG_FREE (pData, pData->pRGBArow, pData->iRowsize);

        if (iRetcode)                  /* on error bail out */
          return iRetcode;

#if defined(MNG_FULL_CMS)              /* cleanup cms stuff */
        iRetcode = mng_clear_cms (pData);

        if (iRetcode)                  /* on error bail out */
          return iRetcode;
#endif
      }
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_DISPLAY_IMAGE, MNG_LC_END);
#endif

  return MNG_NOERROR;                  /* whehehe, this is good ! */
}

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_execute_delta_image (mng_datap  pData,
                                     mng_imagep pTarget,
                                     mng_imagep pDelta)
{
  mng_imagedatap pBuftarget = pTarget->pImgbuf;
  mng_imagedatap pBufdelta  = pDelta->pImgbuf;
  mng_uint32     iY;
  mng_retcode    iRetcode;
  mng_ptr        pSaveRGBA;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_EXECUTE_DELTA_IMAGE, MNG_LC_START);
#endif
                                       /* actively running ? */
  if (((pData->bRunning) || (pData->bSearching)) && (!pData->bSkipping))
  {
    if (pBufdelta->bHasPLTE)           /* palette in delta ? */
    {
      mng_uint32 iX;
                                       /* new palette larger than old one ? */
      if ((!pBuftarget->bHasPLTE) || (pBuftarget->iPLTEcount < pBufdelta->iPLTEcount))
        pBuftarget->iPLTEcount = pBufdelta->iPLTEcount;
                                       /* it's definitely got a PLTE now */
      pBuftarget->bHasPLTE = MNG_TRUE;

      for (iX = 0; iX < pBufdelta->iPLTEcount; iX++)
      {
        pBuftarget->aPLTEentries[iX].iRed   = pBufdelta->aPLTEentries[iX].iRed;
        pBuftarget->aPLTEentries[iX].iGreen = pBufdelta->aPLTEentries[iX].iGreen;
        pBuftarget->aPLTEentries[iX].iBlue  = pBufdelta->aPLTEentries[iX].iBlue;
      }
    }

    if (pBufdelta->bHasTRNS)           /* cheap transparency in delta ? */
    {
      switch (pData->iColortype)       /* drop it into the target */
      {
        case 0: {                      /* gray */
                  pBuftarget->iTRNSgray  = pBufdelta->iTRNSgray;
                  pBuftarget->iTRNSred   = 0;
                  pBuftarget->iTRNSgreen = 0;
                  pBuftarget->iTRNSblue  = 0;
                  pBuftarget->iTRNScount = 0;
                  break;
                }
        case 2: {                      /* rgb */
                  pBuftarget->iTRNSgray  = 0;
                  pBuftarget->iTRNSred   = pBufdelta->iTRNSred;
                  pBuftarget->iTRNSgreen = pBufdelta->iTRNSgreen;
                  pBuftarget->iTRNSblue  = pBufdelta->iTRNSblue;
                  pBuftarget->iTRNScount = 0;
                  break;
                }
        case 3: {                      /* indexed */
                  pBuftarget->iTRNSgray  = 0;
                  pBuftarget->iTRNSred   = 0;
                  pBuftarget->iTRNSgreen = 0;
                  pBuftarget->iTRNSblue  = 0;
                                       /* existing range smaller than new one ? */
                  if ((!pBuftarget->bHasTRNS) || (pBuftarget->iTRNScount < pBufdelta->iTRNScount))
                    pBuftarget->iTRNScount = pBufdelta->iTRNScount;

                  MNG_COPY (pBuftarget->aTRNSentries, pBufdelta->aTRNSentries, pBufdelta->iTRNScount);
                  break;
                }
      }

      pBuftarget->bHasTRNS = MNG_TRUE; /* tell it it's got a tRNS now */
    }

#ifndef MNG_SKIPCHUNK_bKGD
    if (pBufdelta->bHasBKGD)           /* bkgd in source ? */
    {                                  /* drop it onto the target */
      pBuftarget->bHasBKGD   = MNG_TRUE;
      pBuftarget->iBKGDindex = pBufdelta->iBKGDindex;
      pBuftarget->iBKGDgray  = pBufdelta->iBKGDgray;
      pBuftarget->iBKGDred   = pBufdelta->iBKGDred;
      pBuftarget->iBKGDgreen = pBufdelta->iBKGDgreen;
      pBuftarget->iBKGDblue  = pBufdelta->iBKGDblue;
    }
#endif

    if (pBufdelta->bHasGAMA)           /* gamma in source ? */
    {
      pBuftarget->bHasGAMA = MNG_TRUE; /* drop it onto the target */
      pBuftarget->iGamma   = pBufdelta->iGamma;
    }

#ifndef MNG_SKIPCHUNK_cHRM
    if (pBufdelta->bHasCHRM)           /* chroma in delta ? */
    {                                  /* drop it onto the target */
      pBuftarget->bHasCHRM       = MNG_TRUE;
      pBuftarget->iWhitepointx   = pBufdelta->iWhitepointx;
      pBuftarget->iWhitepointy   = pBufdelta->iWhitepointy;
      pBuftarget->iPrimaryredx   = pBufdelta->iPrimaryredx;
      pBuftarget->iPrimaryredy   = pBufdelta->iPrimaryredy;
      pBuftarget->iPrimarygreenx = pBufdelta->iPrimarygreenx;
      pBuftarget->iPrimarygreeny = pBufdelta->iPrimarygreeny;
      pBuftarget->iPrimarybluex  = pBufdelta->iPrimarybluex;
      pBuftarget->iPrimarybluey  = pBufdelta->iPrimarybluey;
    }
#endif

#ifndef MNG_SKIPCHUNK_sRGB
    if (pBufdelta->bHasSRGB)           /* sRGB in delta ? */
    {                                  /* drop it onto the target */
      pBuftarget->bHasSRGB         = MNG_TRUE;
      pBuftarget->iRenderingintent = pBufdelta->iRenderingintent;
    }
#endif

#ifndef MNG_SKIPCHUNK_iCCP
    if (pBufdelta->bHasICCP)           /* ICC profile in delta ? */
    {
      pBuftarget->bHasICCP = MNG_TRUE; /* drop it onto the target */

      if (pBuftarget->pProfile)        /* profile existed ? */
        MNG_FREEX (pData, pBuftarget->pProfile, pBuftarget->iProfilesize);
                                       /* allocate a buffer & copy it */
      MNG_ALLOC (pData, pBuftarget->pProfile, pBufdelta->iProfilesize);
      MNG_COPY  (pBuftarget->pProfile, pBufdelta->pProfile, pBufdelta->iProfilesize);
                                       /* store its length as well */
      pBuftarget->iProfilesize = pBufdelta->iProfilesize;
    }
#endif
                                       /* need to execute delta pixels ? */
    if ((!pData->bDeltaimmediate) && (pData->iDeltatype != MNG_DELTATYPE_NOCHANGE))
    {
      pData->fScalerow = MNG_NULL;     /* not needed by default */

      switch (pBufdelta->iBitdepth)    /* determine scaling routine */
      {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
        case  1 : {
                    switch (pBuftarget->iBitdepth)
                    {
                      case  2 : { pData->fScalerow = (mng_fptr)mng_scale_g1_g2;  break; }
                      case  4 : { pData->fScalerow = (mng_fptr)mng_scale_g1_g4;  break; }

                      case  8 : { pData->fScalerow = (mng_fptr)mng_scale_g1_g8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                      case 16 : { pData->fScalerow = (mng_fptr)mng_scale_g1_g16; break; }
#endif
                    }
                    break;
                  }

        case  2 : {
                    switch (pBuftarget->iBitdepth)
                    {
                      case  1 : { pData->fScalerow = (mng_fptr)mng_scale_g2_g1;  break; }
                      case  4 : { pData->fScalerow = (mng_fptr)mng_scale_g2_g4;  break; }
                      case  8 : { pData->fScalerow = (mng_fptr)mng_scale_g2_g8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                      case 16 : { pData->fScalerow = (mng_fptr)mng_scale_g2_g16; break; }
#endif
                    }
                    break;
                  }

        case  4 : {
                    switch (pBuftarget->iBitdepth)
                    {
                      case  1 : { pData->fScalerow = (mng_fptr)mng_scale_g4_g1;  break; }
                      case  2 : { pData->fScalerow = (mng_fptr)mng_scale_g4_g2;  break; }
                      case  8 : { pData->fScalerow = (mng_fptr)mng_scale_g4_g8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
                      case 16 : { pData->fScalerow = (mng_fptr)mng_scale_g4_g16; break; }
#endif
                    }
                    break;
                  }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */

        case  8 : {
                    switch (pBufdelta->iColortype)
                    {
                      case  0 : ;
                      case  3 : ;
                      case  8 : {
                                  switch (pBuftarget->iBitdepth)
                                  {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
                                    case  1 : { pData->fScalerow = (mng_fptr)mng_scale_g8_g1;  break; }
                                    case  2 : { pData->fScalerow = (mng_fptr)mng_scale_g8_g2;  break; }
                                    case  4 : { pData->fScalerow = (mng_fptr)mng_scale_g8_g4;  break; }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
#ifndef MNG_NO_16BIT_SUPPORT
                                    case 16 : { pData->fScalerow = (mng_fptr)mng_scale_g8_g16; break; }
#endif
                                  }
                                  break;
                                }
                      case  2 : ;
                      case 10 : {
#ifndef MNG_NO_16BIT_SUPPORT
                                  if (pBuftarget->iBitdepth == 16)
                                    pData->fScalerow = (mng_fptr)mng_scale_rgb8_rgb16;
#endif
                                  break;
                                }
                      case  4 : ;
                      case 12 : {
#ifndef MNG_NO_16BIT_SUPPORT
                                  if (pBuftarget->iBitdepth == 16)
                                    pData->fScalerow = (mng_fptr)mng_scale_ga8_ga16;
#endif
                                  break;
                                }
                      case  6 : ;
                      case 14 : {
#ifndef MNG_NO_16BIT_SUPPORT
                                  if (pBuftarget->iBitdepth == 16)
                                    pData->fScalerow = (mng_fptr)mng_scale_rgba8_rgba16;
#endif
                                  break;
                                }
                    }
                    break;
                  }

#ifndef MNG_NO_16BIT_SUPPORT
        case 16 : {
                    switch (pBufdelta->iColortype)
                    {
                      case  0 : ;
                      case  3 : ;
                      case  8 : {
                                  switch (pBuftarget->iBitdepth)
                                  {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
                                    case 1 : { pData->fScalerow = (mng_fptr)mng_scale_g16_g1; break; }
                                    case 2 : { pData->fScalerow = (mng_fptr)mng_scale_g16_g2; break; }
                                    case 4 : { pData->fScalerow = (mng_fptr)mng_scale_g16_g4; break; }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
                                    case 8 : { pData->fScalerow = (mng_fptr)mng_scale_g16_g8; break; }
                                  }
                                  break;
                                }
                      case  2 : ;
                      case 10 : {
                                  if (pBuftarget->iBitdepth == 8)
                                    pData->fScalerow = (mng_fptr)mng_scale_rgb16_rgb8;
                                  break;
                                }
                      case  4 : ;
                      case 12 : {
                                  if (pBuftarget->iBitdepth == 8)
                                    pData->fScalerow = (mng_fptr)mng_scale_ga16_ga8;
                                  break;
                                }
                      case  6 : ;
                      case 14 : {
                                  if (pBuftarget->iBitdepth == 8)
                                    pData->fScalerow = (mng_fptr)mng_scale_rgba16_rgba8;
                                  break;
                                }
                    }
                    break;
                  }
#endif

      }

      pData->fDeltarow = MNG_NULL;     /* let's assume there's nothing to do */

      switch (pBuftarget->iColortype)  /* determine delta processing routine */
      {
        case  0 : ;
        case  8 : {                     /* gray */
                    if ((pData->iDeltatype == MNG_DELTATYPE_REPLACE          ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 0) || (pBufdelta->iColortype == 3) ||
                          (pBufdelta->iColortype == 8))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
                          case  1 : { pData->fDeltarow = (mng_fptr)mng_delta_g1_g1;   break; }
                          case  2 : { pData->fDeltarow = (mng_fptr)mng_delta_g2_g2;   break; }
                          case  4 : { pData->fDeltarow = (mng_fptr)mng_delta_g4_g4;   break; }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_g8_g8;   break; }
#ifndef MNG_NO_16BIT_SUPPORT
                          case 16 : { pData->fDeltarow = (mng_fptr)mng_delta_g16_g16; break; }
#endif
                        }
                      }
                    }

                    break;
                  }

        case  2 : ;
        case 10 : {                     /* rgb */
                    if ((pData->iDeltatype == MNG_DELTATYPE_REPLACE          ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 2) || (pBufdelta->iColortype == 10))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_rgb8_rgb8;   break; }
#ifndef MNG_NO_16BIT_SUPPORT
                          case 16 : { pData->fDeltarow = (mng_fptr)mng_delta_rgb16_rgb16; break; }
#endif
                        }
                      }
                    }

                    break;
                  }

        case  3 : {                     /* indexed; abuse gray routines */
                    if ((pData->iDeltatype == MNG_DELTATYPE_REPLACE          ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 0) || (pBufdelta->iColortype == 3))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
                          case  1 : { pData->fDeltarow = (mng_fptr)mng_delta_g1_g1; break; }
                          case  2 : { pData->fDeltarow = (mng_fptr)mng_delta_g2_g2; break; }
                          case  4 : { pData->fDeltarow = (mng_fptr)mng_delta_g4_g4; break; }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_g8_g8; break; }
                        }
                      }
                    }

                    break;
                  }

        case  4 : ;
        case 12 : {                     /* gray + alpha */
                    if ((pData->iDeltatype == MNG_DELTATYPE_REPLACE          ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 4) || (pBufdelta->iColortype == 12))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_ga8_ga8;   break; }
#ifndef MNG_NO_16BIT_SUPPORT
                          case 16 : { pData->fDeltarow = (mng_fptr)mng_delta_ga16_ga16; break; }
#endif
                        }
                      }
                    }
                    else
                    if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKCOLORADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKCOLORREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 0) || (pBufdelta->iColortype == 3) ||
                          (pBufdelta->iColortype == 8))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_ga8_g8;   break; }
#ifndef MNG_NO_16BIT_SUPPORT
                          case 16 : { pData->fDeltarow = (mng_fptr)mng_delta_ga16_g16; break; }
#endif
                        }
                      }
                    }
                    else
                    if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKALPHAADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKALPHAREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 0) || (pBufdelta->iColortype == 3))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_ga8_a8;   break; }
#ifndef MNG_NO_16BIT_SUPPORT
                          case 16 : { pData->fDeltarow = (mng_fptr)mng_delta_ga16_a16; break; }
#endif
                        }
                      }
                    }

                    break;
                  }

        case  6 : ;
        case 14 : {                     /* rgb + alpha */
                    if ((pData->iDeltatype == MNG_DELTATYPE_REPLACE          ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 6) || (pBufdelta->iColortype == 14))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_rgba8_rgba8;   break; }
#ifndef MNG_NO_16BIT_SUPPORT
                          case 16 : { pData->fDeltarow = (mng_fptr)mng_delta_rgba16_rgba16; break; }
#endif
                        }
                      }
                    }
                    else
                    if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKCOLORADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKCOLORREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 2) || (pBufdelta->iColortype == 10))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_rgba8_rgb8;   break; }
#ifndef MNG_NO_16BIT_SUPPORT
                          case 16 : { pData->fDeltarow = (mng_fptr)mng_delta_rgba16_rgb16; break; }
#endif
                        }
                      }
                    }
                    else
                    if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKALPHAADD    ) ||
                        (pData->iDeltatype == MNG_DELTATYPE_BLOCKALPHAREPLACE)    )
                    {
                      if ((pBufdelta->iColortype == 0) || (pBufdelta->iColortype == 3))
                      {
                        switch (pBuftarget->iBitdepth)
                        {
                          case  8 : { pData->fDeltarow = (mng_fptr)mng_delta_rgba8_a8;   break; }
#ifndef MNG_NO_16BIT_SUPPORT
                          case 16 : { pData->fDeltarow = (mng_fptr)mng_delta_rgba16_a16; break; }
#endif
                        }
                      }
                    }

                    break;
                  }

      }

      if (pData->fDeltarow)            /* do we need to take action ? */
      {
        pData->iPass        = -1;      /* setup row dimensions and stuff */
        pData->iRow         = pData->iDeltaBlocky;
        pData->iRowinc      = 1;
        pData->iCol         = pData->iDeltaBlockx;
        pData->iColinc      = 1;
        pData->iRowsamples  = pBufdelta->iWidth;
        pData->iRowsize     = pBuftarget->iRowsize;
                                       /* indicate where to retrieve & where to store */
        pData->pRetrieveobj = (mng_objectp)pDelta;
        pData->pStoreobj    = (mng_objectp)pTarget;

        pSaveRGBA = pData->pRGBArow;   /* save current temp-buffer! */
                                       /* get a temporary row-buffer */
        MNG_ALLOC (pData, pData->pRGBArow, (pBufdelta->iRowsize << 1));

        iY       = 0;                  /* this is where we start */
        iRetcode = MNG_NOERROR;        /* still oke for now */

        while ((!iRetcode) && (iY < pBufdelta->iHeight))
        {                              /* get a row */
          mng_uint8p pWork = pBufdelta->pImgdata + (iY * pBufdelta->iRowsize);

          MNG_COPY (pData->pRGBArow, pWork, pBufdelta->iRowsize);

          if (pData->fScalerow)        /* scale it (if necessary) */
            iRetcode = ((mng_scalerow)pData->fScalerow) (pData);

          if (!iRetcode)               /* and... execute it */
            iRetcode = ((mng_deltarow)pData->fDeltarow) (pData);

          if (!iRetcode)               /* adjust variables for next row */
            iRetcode = mng_next_row (pData);

          iY++;                        /* and next line */
        }
                                       /* drop the temporary row-buffer */
        MNG_FREE (pData, pData->pRGBArow, (pBufdelta->iRowsize << 1));
        pData->pRGBArow = pSaveRGBA;   /* restore saved temp-buffer! */

        if (iRetcode)                  /* on error bail out */
          return iRetcode;

      }
      else
        MNG_ERROR (pData, MNG_INVALIDDELTA);

    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_EXECUTE_DELTA_IMAGE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_NO_DELTA_PNG */

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
MNG_LOCAL mng_retcode save_state (mng_datap pData)
{
  mng_savedatap pSave;
  mng_imagep    pImage;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_SAVE_STATE, MNG_LC_START);
#endif

  if (pData->pSavedata)                /* sanity check */
    MNG_ERROR (pData, MNG_INTERNALERROR);
                                       /* get a buffer for saving */
  MNG_ALLOC (pData, pData->pSavedata, sizeof (mng_savedata));

  pSave = pData->pSavedata;            /* address it more directly */
                                       /* and copy global data from the main struct */
#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
  pSave->bHasglobalPLTE       = pData->bHasglobalPLTE;
  pSave->bHasglobalTRNS       = pData->bHasglobalTRNS;
  pSave->bHasglobalGAMA       = pData->bHasglobalGAMA;
  pSave->bHasglobalCHRM       = pData->bHasglobalCHRM;
  pSave->bHasglobalSRGB       = pData->bHasglobalSRGB;
  pSave->bHasglobalICCP       = pData->bHasglobalICCP;
  pSave->bHasglobalBKGD       = pData->bHasglobalBKGD;
#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

#ifndef MNG_SKIPCHUNK_BACK
  pSave->iBACKred             = pData->iBACKred;
  pSave->iBACKgreen           = pData->iBACKgreen;
  pSave->iBACKblue            = pData->iBACKblue;
  pSave->iBACKmandatory       = pData->iBACKmandatory;
  pSave->iBACKimageid         = pData->iBACKimageid;
  pSave->iBACKtile            = pData->iBACKtile;
#endif

#ifndef MNG_SKIPCHUNK_FRAM
  pSave->iFRAMmode            = pData->iFRAMmode;
  pSave->iFRAMdelay           = pData->iFRAMdelay;
  pSave->iFRAMtimeout         = pData->iFRAMtimeout;
  pSave->bFRAMclipping        = pData->bFRAMclipping;
  pSave->iFRAMclipl           = pData->iFRAMclipl;
  pSave->iFRAMclipr           = pData->iFRAMclipr;
  pSave->iFRAMclipt           = pData->iFRAMclipt;
  pSave->iFRAMclipb           = pData->iFRAMclipb;
#endif

  pSave->iGlobalPLTEcount     = pData->iGlobalPLTEcount;

  MNG_COPY (pSave->aGlobalPLTEentries, pData->aGlobalPLTEentries, sizeof (mng_rgbpaltab));

  pSave->iGlobalTRNSrawlen    = pData->iGlobalTRNSrawlen;
  MNG_COPY (pSave->aGlobalTRNSrawdata, pData->aGlobalTRNSrawdata, 256);

  pSave->iGlobalGamma         = pData->iGlobalGamma;

#ifndef MNG_SKIPCHUNK_cHRM
  pSave->iGlobalWhitepointx   = pData->iGlobalWhitepointx;
  pSave->iGlobalWhitepointy   = pData->iGlobalWhitepointy;
  pSave->iGlobalPrimaryredx   = pData->iGlobalPrimaryredx;
  pSave->iGlobalPrimaryredy   = pData->iGlobalPrimaryredy;
  pSave->iGlobalPrimarygreenx = pData->iGlobalPrimarygreenx;
  pSave->iGlobalPrimarygreeny = pData->iGlobalPrimarygreeny;
  pSave->iGlobalPrimarybluex  = pData->iGlobalPrimarybluex;
  pSave->iGlobalPrimarybluey  = pData->iGlobalPrimarybluey;
#endif

#ifndef MNG_SKIPCHUNK_sRGB
  pSave->iGlobalRendintent    = pData->iGlobalRendintent;
#endif

#ifndef MNG_SKIPCHUNK_iCCP
  pSave->iGlobalProfilesize   = pData->iGlobalProfilesize;

  if (pSave->iGlobalProfilesize)       /* has a profile ? */
  {                                    /* then copy that ! */
    MNG_ALLOC (pData, pSave->pGlobalProfile, pSave->iGlobalProfilesize);
    MNG_COPY (pSave->pGlobalProfile, pData->pGlobalProfile, pSave->iGlobalProfilesize);
  }
#endif

#ifndef MNG_SKIPCHUNK_bKGD
  pSave->iGlobalBKGDred       = pData->iGlobalBKGDred;
  pSave->iGlobalBKGDgreen     = pData->iGlobalBKGDgreen;
  pSave->iGlobalBKGDblue      = pData->iGlobalBKGDblue;
#endif

                                       /* freeze current image objects */
  pImage = (mng_imagep)pData->pFirstimgobj;

  while (pImage)
  {                                    /* freeze the object AND its buffer */
    pImage->bFrozen          = MNG_TRUE;
    pImage->pImgbuf->bFrozen = MNG_TRUE;
                                       /* neeeext */
    pImage = (mng_imagep)pImage->sHeader.pNext;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_SAVE_STATE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

mng_retcode mng_reset_objzero (mng_datap pData)
{
  mng_imagep  pImage   = (mng_imagep)pData->pObjzero;
  mng_retcode iRetcode = mng_reset_object_details (pData, pImage, 0, 0, 0,
                                                   0, 0, 0, 0, MNG_TRUE);

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

  pImage->bVisible             = MNG_TRUE;
  pImage->bViewable            = MNG_TRUE;
  pImage->iPosx                = 0;
  pImage->iPosy                = 0;
  pImage->bClipped             = MNG_FALSE;
  pImage->iClipl               = 0;
  pImage->iClipr               = 0;
  pImage->iClipt               = 0;
  pImage->iClipb               = 0;
#ifndef MNG_SKIPCHUNK_MAGN
  pImage->iMAGN_MethodX        = 0;
  pImage->iMAGN_MethodY        = 0;
  pImage->iMAGN_MX             = 0;
  pImage->iMAGN_MY             = 0;
  pImage->iMAGN_ML             = 0;
  pImage->iMAGN_MR             = 0;
  pImage->iMAGN_MT             = 0;
  pImage->iMAGN_MB             = 0;
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

MNG_LOCAL mng_retcode restore_state (mng_datap pData)
{
#ifndef MNG_SKIPCHUNK_SAVE
  mng_savedatap pSave;
#endif
  mng_imagep    pImage;
  mng_retcode   iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_RESTORE_STATE, MNG_LC_START);
#endif
                                       /* restore object 0 status !!! */
  iRetcode = mng_reset_objzero (pData);

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* fresh cycle; fake no frames done yet */
  pData->bFramedone             = MNG_FALSE;

#ifndef MNG_SKIPCHUNK_SAVE
  if (pData->pSavedata)                /* do we have a saved state ? */
  {
    pSave = pData->pSavedata;          /* address it more directly */
                                       /* and copy it back to the main struct */
#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
    pData->bHasglobalPLTE       = pSave->bHasglobalPLTE;
    pData->bHasglobalTRNS       = pSave->bHasglobalTRNS;
    pData->bHasglobalGAMA       = pSave->bHasglobalGAMA;
    pData->bHasglobalCHRM       = pSave->bHasglobalCHRM;
    pData->bHasglobalSRGB       = pSave->bHasglobalSRGB;
    pData->bHasglobalICCP       = pSave->bHasglobalICCP;
    pData->bHasglobalBKGD       = pSave->bHasglobalBKGD;
#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

#ifndef MNG_SKIPCHUNK_BACK
    pData->iBACKred             = pSave->iBACKred;
    pData->iBACKgreen           = pSave->iBACKgreen;
    pData->iBACKblue            = pSave->iBACKblue;
    pData->iBACKmandatory       = pSave->iBACKmandatory;
    pData->iBACKimageid         = pSave->iBACKimageid;
    pData->iBACKtile            = pSave->iBACKtile;
#endif

#ifndef MNG_SKIPCHUNK_FRAM
    pData->iFRAMmode            = pSave->iFRAMmode;
/*    pData->iFRAMdelay           = pSave->iFRAMdelay; */
    pData->iFRAMtimeout         = pSave->iFRAMtimeout;
    pData->bFRAMclipping        = pSave->bFRAMclipping;
    pData->iFRAMclipl           = pSave->iFRAMclipl;
    pData->iFRAMclipr           = pSave->iFRAMclipr;
    pData->iFRAMclipt           = pSave->iFRAMclipt;
    pData->iFRAMclipb           = pSave->iFRAMclipb;
                                       /* NOOOOOOOOOOOO */
/*    pData->iFramemode           = pSave->iFRAMmode;
    pData->iFramedelay          = pSave->iFRAMdelay;
    pData->iFrametimeout        = pSave->iFRAMtimeout;
    pData->bFrameclipping       = pSave->bFRAMclipping;
    pData->iFrameclipl          = pSave->iFRAMclipl;
    pData->iFrameclipr          = pSave->iFRAMclipr;
    pData->iFrameclipt          = pSave->iFRAMclipt;
    pData->iFrameclipb          = pSave->iFRAMclipb; */

/*    pData->iNextdelay           = pSave->iFRAMdelay; */
    pData->iNextdelay           = pData->iFramedelay;
#endif

    pData->iGlobalPLTEcount     = pSave->iGlobalPLTEcount;
    MNG_COPY (pData->aGlobalPLTEentries, pSave->aGlobalPLTEentries, sizeof (mng_rgbpaltab));

    pData->iGlobalTRNSrawlen    = pSave->iGlobalTRNSrawlen;
    MNG_COPY (pData->aGlobalTRNSrawdata, pSave->aGlobalTRNSrawdata, 256);

    pData->iGlobalGamma         = pSave->iGlobalGamma;

#ifndef MNG_SKIPCHUNK_cHRM
    pData->iGlobalWhitepointx   = pSave->iGlobalWhitepointx;
    pData->iGlobalWhitepointy   = pSave->iGlobalWhitepointy;
    pData->iGlobalPrimaryredx   = pSave->iGlobalPrimaryredx;
    pData->iGlobalPrimaryredy   = pSave->iGlobalPrimaryredy;
    pData->iGlobalPrimarygreenx = pSave->iGlobalPrimarygreenx;
    pData->iGlobalPrimarygreeny = pSave->iGlobalPrimarygreeny;
    pData->iGlobalPrimarybluex  = pSave->iGlobalPrimarybluex;
    pData->iGlobalPrimarybluey  = pSave->iGlobalPrimarybluey;
#endif

    pData->iGlobalRendintent    = pSave->iGlobalRendintent;

#ifndef MNG_SKIPCHUNK_iCCP
    pData->iGlobalProfilesize   = pSave->iGlobalProfilesize;

    if (pData->iGlobalProfilesize)     /* has a profile ? */
    {                                  /* then copy that ! */
      MNG_ALLOC (pData, pData->pGlobalProfile, pData->iGlobalProfilesize);
      MNG_COPY (pData->pGlobalProfile, pSave->pGlobalProfile, pData->iGlobalProfilesize);
    }
#endif

#ifndef MNG_SKIPCHUNK_bKGD
    pData->iGlobalBKGDred       = pSave->iGlobalBKGDred;
    pData->iGlobalBKGDgreen     = pSave->iGlobalBKGDgreen;
    pData->iGlobalBKGDblue      = pSave->iGlobalBKGDblue;
#endif
  }
  else                                 /* no saved-data; so reset the lot */
#endif /* SKIPCHUNK_SAVE */
  {
#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
    pData->bHasglobalPLTE       = MNG_FALSE;
    pData->bHasglobalTRNS       = MNG_FALSE;
    pData->bHasglobalGAMA       = MNG_FALSE;
    pData->bHasglobalCHRM       = MNG_FALSE;
    pData->bHasglobalSRGB       = MNG_FALSE;
    pData->bHasglobalICCP       = MNG_FALSE;
    pData->bHasglobalBKGD       = MNG_FALSE;
#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

#ifndef MNG_SKIPCHUNK_TERM
    if (!pData->bMisplacedTERM)        /* backward compatible ugliness !!! */
    {
      pData->iBACKred           = 0;
      pData->iBACKgreen         = 0;
      pData->iBACKblue          = 0;
      pData->iBACKmandatory     = 0;
      pData->iBACKimageid       = 0;
      pData->iBACKtile          = 0;
    }
#endif

#ifndef MNG_SKIPCHUNK_FRAM
    pData->iFRAMmode            = 1;
/*    pData->iFRAMdelay           = 1; */
    pData->iFRAMtimeout         = 0x7fffffffl;
    pData->bFRAMclipping        = MNG_FALSE;
    pData->iFRAMclipl           = 0;
    pData->iFRAMclipr           = 0;
    pData->iFRAMclipt           = 0;
    pData->iFRAMclipb           = 0;
                                       /* NOOOOOOOOOOOO */
/*    pData->iFramemode           = 1;
    pData->iFramedelay          = 1;
    pData->iFrametimeout        = 0x7fffffffl;
    pData->bFrameclipping       = MNG_FALSE;
    pData->iFrameclipl          = 0;
    pData->iFrameclipr          = 0;
    pData->iFrameclipt          = 0;
    pData->iFrameclipb          = 0; */

/*    pData->iNextdelay           = 1; */
    pData->iNextdelay           = pData->iFramedelay;
#endif

    pData->iGlobalPLTEcount     = 0;

    pData->iGlobalTRNSrawlen    = 0;

    pData->iGlobalGamma         = 0;

#ifndef MNG_SKIPCHUNK_cHRM
    pData->iGlobalWhitepointx   = 0;
    pData->iGlobalWhitepointy   = 0;
    pData->iGlobalPrimaryredx   = 0;
    pData->iGlobalPrimaryredy   = 0;
    pData->iGlobalPrimarygreenx = 0;
    pData->iGlobalPrimarygreeny = 0;
    pData->iGlobalPrimarybluex  = 0;
    pData->iGlobalPrimarybluey  = 0;
#endif

    pData->iGlobalRendintent    = 0;

#ifndef MNG_SKIPCHUNK_iCCP
    if (pData->iGlobalProfilesize)     /* free a previous profile ? */
      MNG_FREE (pData, pData->pGlobalProfile, pData->iGlobalProfilesize);

    pData->iGlobalProfilesize   = 0;
#endif

#ifndef MNG_SKIPCHUNK_bKGD
    pData->iGlobalBKGDred       = 0;
    pData->iGlobalBKGDgreen     = 0;
    pData->iGlobalBKGDblue      = 0;
#endif
  }

#ifndef MNG_SKIPCHUNK_TERM
  if (!pData->bMisplacedTERM)          /* backward compatible ugliness !!! */
  {
    pImage = (mng_imagep)pData->pFirstimgobj;
                                       /* drop un-frozen image objects */
    while (pImage)
    {
      mng_imagep pNext = (mng_imagep)pImage->sHeader.pNext;

      if (!pImage->bFrozen)            /* is it un-frozen ? */
      {
        mng_imagep pPrev = (mng_imagep)pImage->sHeader.pPrev;

        if (pPrev)                     /* unlink it */
          pPrev->sHeader.pNext = pNext;
        else
          pData->pFirstimgobj  = pNext;

        if (pNext)
          pNext->sHeader.pPrev = pPrev;
        else
          pData->pLastimgobj   = pPrev;

        if (pImage->pImgbuf->bFrozen)  /* buffer frozen ? */
        {
          if (pImage->pImgbuf->iRefcount < 2)
            MNG_ERROR (pData, MNG_INTERNALERROR);
                                       /* decrease ref counter */
          pImage->pImgbuf->iRefcount--;
                                       /* just cleanup the object then */
          MNG_FREEX (pData, pImage, sizeof (mng_image));
        }
        else
        {                              /* free the image buffer */
          iRetcode = mng_free_imagedataobject (pData, pImage->pImgbuf);
                                       /* and cleanup the object */
          MNG_FREEX (pData, pImage, sizeof (mng_image));

          if (iRetcode)                /* on error bail out */
            return iRetcode;
        }
      }

      pImage = pNext;                  /* neeeext */
    }
  }
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_RESTORE_STATE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* *                                                                        * */
/* * General display processing routine                                     * */
/* *                                                                        * */
/* ************************************************************************** */

mng_retcode mng_process_display (mng_datap pData)
{
  mng_retcode iRetcode = MNG_NOERROR;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY, MNG_LC_START);
#endif

  if (!pData->iBreakpoint)             /* not broken previously ? */
  {
    if ((pData->iRequestframe) || (pData->iRequestlayer) || (pData->iRequesttime))
    {
      pData->bSearching = MNG_TRUE;    /* indicate we're searching */

      iRetcode = clear_canvas (pData); /* make the canvas virgin black ?!? */

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
                                       /* let's start from the top, shall we */
      pData->pCurraniobj = pData->pFirstaniobj;
    }
  }

  do                                   /* process the objects */
  {
    if (pData->bSearching)             /* clear timer-flag when searching !!! */
      pData->bTimerset = MNG_FALSE;
                                       /* do we need to finish something first ? */
    if ((pData->iBreakpoint) && (pData->iBreakpoint < 99))
    {
      switch (pData->iBreakpoint)      /* return to broken display routine */
      {
#ifndef MNG_SKIPCHUNK_FRAM
        case  1 : { iRetcode = mng_process_display_fram2 (pData); break; }
#endif
#ifndef MNG_SKIPCHUNK_SHOW
        case  3 : ;                    /* same as 4 !!! */
        case  4 : { iRetcode = mng_process_display_show  (pData); break; }
#endif
#ifndef MNG_SKIPCHUNK_CLON
        case  5 : { iRetcode = mng_process_display_clon2 (pData); break; }
#endif
#ifndef MNG_SKIPCHUNK_MAGN
        case  9 : { iRetcode = mng_process_display_magn2 (pData); break; }
        case 10 : { iRetcode = mng_process_display_mend2 (pData); break; }
#endif
#ifndef MNG_SKIPCHUNK_PAST
        case 11 : { iRetcode = mng_process_display_past2 (pData); break; }
#endif
        default : MNG_ERROR (pData, MNG_INTERNALERROR);
      }
    }
    else
    {
      if (pData->pCurraniobj)
        iRetcode = ((mng_object_headerp)pData->pCurraniobj)->fProcess (pData, pData->pCurraniobj);
    }

    if (!pData->bTimerset)             /* reset breakpoint flag ? */
      pData->iBreakpoint = 0;
                                       /* can we advance to next object ? */
    if ((!iRetcode) && (pData->pCurraniobj) &&
        (!pData->bTimerset) && (!pData->bSectionwait))
    {
      pData->pCurraniobj = ((mng_object_headerp)pData->pCurraniobj)->pNext;
                                       /* MEND processing to be done ? */
      if ((pData->eImagetype == mng_it_mng) && (!pData->pCurraniobj))
        iRetcode = mng_process_display_mend (pData);

      if (!pData->pCurraniobj)         /* refresh after last image ? */
        pData->bNeedrefresh = MNG_TRUE;
    }

    if (pData->bSearching)             /* are we looking for something ? */
    {
      if ((pData->iRequestframe) && (pData->iRequestframe <= pData->iFrameseq))
      {
        pData->iRequestframe = 0;      /* found the frame ! */
        pData->bSearching    = MNG_FALSE;
      }
      else
      if ((pData->iRequestlayer) && (pData->iRequestlayer <= pData->iLayerseq))
      {
        pData->iRequestlayer = 0;      /* found the layer ! */
        pData->bSearching    = MNG_FALSE;
      }
      else
      if ((pData->iRequesttime) && (pData->iRequesttime <= pData->iFrametime))
      {
        pData->iRequesttime  = 0;      /* found the playtime ! */
        pData->bSearching    = MNG_FALSE;
      }
    }
  }                                    /* until error or a break or no more objects */
  while ((!iRetcode) && (pData->pCurraniobj) &&
         (((pData->bRunning) && (!pData->bTimerset)) || (pData->bSearching)) &&
         (!pData->bSectionwait) && (!pData->bFreezing));

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* refresh needed ? */
  if ((!pData->bTimerset) && (pData->bNeedrefresh))
  {
    iRetcode = mng_display_progressive_refresh (pData, 1);

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }
                                       /* timer break ? */
  if ((pData->bTimerset) && (!pData->iBreakpoint))
    pData->iBreakpoint = 99;
  else
  if (!pData->bTimerset)
    pData->iBreakpoint = 0;            /* reset if no timer break */

  if ((!pData->bTimerset) && (!pData->pCurraniobj))
    pData->bRunning = MNG_FALSE;       /* all done now ! */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* *                                                                        * */
/* * Chunk display processing routines                                      * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
png_imgtype mng_png_imgtype(mng_uint8 colortype, mng_uint8 bitdepth)
{
  png_imgtype ret;
  switch (bitdepth)
  {
#ifndef MNG_NO_1_2_4BIT_SUPPORT
    case 1:
    {
      png_imgtype imgtype[]={png_g1,png_none,png_none,png_idx1};
      ret=imgtype[colortype];
      break;
    }
    case 2:
    {
      png_imgtype imgtype[]={png_g2,png_none,png_none,png_idx2};
      ret=imgtype[colortype];
      break;
    }
    case 4:
    {
      png_imgtype imgtype[]={png_g4,png_none,png_none,png_idx4};
      ret=imgtype[colortype];
      break;
    }
#endif
    case 8:
    {
      png_imgtype imgtype[]={png_g8,png_none,png_rgb8,png_idx8,png_ga8,
          png_none,png_rgba8};
      ret=imgtype[colortype];
      break;
    }
#ifndef MNG_NO_16BIT_SUPPORT
    case 16:
    {
      png_imgtype imgtype[]={png_g16,png_none,png_rgb16,png_none,png_ga16,
          png_none,png_rgba16};
      ret=imgtype[colortype];
      break;
    }
#endif
    default:
      ret=png_none;
      break;
  }
  return (ret);
}
#endif /* MNG_OPTIMIZE_FOOTPRINT_INIT */

/* ************************************************************************** */

mng_retcode mng_process_display_ihdr (mng_datap pData)
{                                      /* address the current "object" if any */
  mng_imagep pImage = (mng_imagep)pData->pCurrentobj;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IHDR, MNG_LC_START);
#endif

  if (!pData->bHasDHDR)
  {
    pData->fInitrowproc = MNG_NULL;    /* do nothing by default */
    pData->fDisplayrow  = MNG_NULL;
    pData->fCorrectrow  = MNG_NULL;
    pData->fStorerow    = MNG_NULL;
    pData->fProcessrow  = MNG_NULL;
    pData->fDifferrow   = MNG_NULL;
    pData->pStoreobj    = MNG_NULL;
  }

  if (!pData->iBreakpoint)             /* not previously broken ? */
  {
    mng_retcode iRetcode = MNG_NOERROR;

#ifndef MNG_NO_DELTA_PNG
    if (pData->bHasDHDR)               /* is a delta-image ? */
    {
      if (pData->iDeltatype == MNG_DELTATYPE_REPLACE)
        iRetcode = mng_reset_object_details (pData, (mng_imagep)pData->pDeltaImage,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iBitdepth, pData->iColortype,
                                             pData->iCompression, pData->iFilter,
                                             pData->iInterlace, MNG_TRUE);
      else
      if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
          (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
      {
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iPixelsampledepth = pData->iBitdepth;
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iAlphasampledepth = pData->iBitdepth;
      }
      else
      if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKALPHAADD    ) ||
          (pData->iDeltatype == MNG_DELTATYPE_BLOCKALPHAREPLACE)    )
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iAlphasampledepth = pData->iBitdepth;
      else
      if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKCOLORADD    ) ||
          (pData->iDeltatype == MNG_DELTATYPE_BLOCKCOLORREPLACE)    )
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iPixelsampledepth = pData->iBitdepth;

      if (!iRetcode)
      {                                /* process immediately if bitdepth & colortype are equal */
        pData->bDeltaimmediate =
          (mng_bool)((pData->iBitdepth  == ((mng_imagep)pData->pDeltaImage)->pImgbuf->iBitdepth ) &&
                     (pData->iColortype == ((mng_imagep)pData->pDeltaImage)->pImgbuf->iColortype)    );
                                       /* be sure to reset object 0 */
        iRetcode = mng_reset_object_details (pData, (mng_imagep)pData->pObjzero,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iBitdepth, pData->iColortype,
                                             pData->iCompression, pData->iFilter,
                                             pData->iInterlace, MNG_TRUE);
      }
    }
    else
#endif
    {
      if (pImage)                      /* update object buffer ? */
        iRetcode = mng_reset_object_details (pData, pImage,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iBitdepth, pData->iColortype,
                                             pData->iCompression, pData->iFilter,
                                             pData->iInterlace, MNG_TRUE);
      else
        iRetcode = mng_reset_object_details (pData, (mng_imagep)pData->pObjzero,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iBitdepth, pData->iColortype,
                                             pData->iCompression, pData->iFilter,
                                             pData->iInterlace, MNG_TRUE);
    }

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }

#ifndef MNG_NO_DELTA_PNG
  if (!pData->bHasDHDR)
#endif
  {
    if (pImage)                        /* real object ? */
      pData->pStoreobj = pImage;       /* tell the row routines */
    else                               /* otherwise use object 0 */
      pData->pStoreobj = pData->pObjzero;

#if !defined(MNG_INCLUDE_MPNG_PROPOSAL) && !defined(MNG_INCLUDE_ANG_PROPOSAL)
    if (                               /* display "on-the-fly" ? */
#ifndef MNG_SKIPCHUNK_MAGN
         (((mng_imagep)pData->pStoreobj)->iMAGN_MethodX == 0) &&
         (((mng_imagep)pData->pStoreobj)->iMAGN_MethodY == 0) &&
#endif
         ( (pData->eImagetype == mng_it_png         ) ||
           (((mng_imagep)pData->pStoreobj)->bVisible)    )       )
    {
      next_layer (pData);              /* that's a new layer then ! */

      if (pData->bTimerset)            /* timer break ? */
        pData->iBreakpoint = 2;
      else
      {
        pData->iBreakpoint = 0;
                                       /* anything to display ? */
        if ((pData->iDestr > pData->iDestl) && (pData->iDestb > pData->iDestt))
          set_display_routine (pData); /* then determine display routine */
      }
    }
#endif
  }

  if (!pData->bTimerset)               /* no timer break ? */
  {
#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
    pData->fInitrowproc = (mng_fptr)mng_init_rowproc;
    pData->ePng_imgtype=mng_png_imgtype(pData->iColortype,pData->iBitdepth);
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

  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
mng_retcode mng_process_display_mpng (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MPNG, MNG_LC_START);
#endif

  pData->iAlphadepth = 8;              /* assume transparency !! */

  if (pData->fProcessheader)           /* inform the app (creating the output canvas) ? */
  {
    pData->iWidth  = ((mng_mpng_objp)pData->pMPNG)->iFramewidth;
    pData->iHeight = ((mng_mpng_objp)pData->pMPNG)->iFrameheight;

    if (!pData->fProcessheader (((mng_handle)pData), pData->iWidth, pData->iHeight))
      MNG_ERROR (pData, MNG_APPMISCERROR);
  }

  next_layer (pData);                  /* first mPNG layer then ! */
  pData->bTimerset   = MNG_FALSE;
  pData->iBreakpoint = 0;

  if ((pData->iDestr > pData->iDestl) && (pData->iDestb > pData->iDestt))
    set_display_routine (pData);       /* then determine display routine */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
mng_retcode mng_process_display_ang (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_ANG, MNG_LC_START);
#endif

  if (pData->fProcessheader)           /* inform the app (creating the output canvas) ? */
  {
    if (!pData->fProcessheader (((mng_handle)pData), pData->iWidth, pData->iHeight))
      MNG_ERROR (pData, MNG_APPMISCERROR);
  }

  next_layer (pData);                  /* first mPNG layer then ! */
  pData->bTimerset   = MNG_FALSE;
  pData->iBreakpoint = 0;

  if ((pData->iDestr > pData->iDestl) && (pData->iDestb > pData->iDestt))
    set_display_routine (pData);       /* then determine display routine */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_ANG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_idat (mng_datap  pData,
                                      mng_uint32 iRawlen,
                                      mng_uint8p pRawdata)
#else
mng_retcode mng_process_display_idat (mng_datap  pData)
#endif
{
  mng_retcode iRetcode = MNG_NOERROR;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IDAT, MNG_LC_START);
#endif

#if defined(MNG_INCLUDE_MPNG_PROPOSAL) || defined(MNG_INCLUDE_ANG_PROPOSAL) 
  if ((pData->eImagetype == mng_it_png) && (pData->iLayerseq <= 0))
  {
    if (pData->fProcessheader)         /* inform the app (creating the output canvas) ? */
      if (!pData->fProcessheader (((mng_handle)pData), pData->iWidth, pData->iHeight))
        MNG_ERROR (pData, MNG_APPMISCERROR);

    next_layer (pData);                /* first regular PNG layer then ! */
    pData->bTimerset   = MNG_FALSE;
    pData->iBreakpoint = 0;

    if ((pData->iDestr > pData->iDestl) && (pData->iDestb > pData->iDestt))
      set_display_routine (pData);     /* then determine display routine */
  }
#endif

  if (pData->bRestorebkgd)             /* need to restore the background ? */
  {
    pData->bRestorebkgd = MNG_FALSE;
    iRetcode            = load_bkgdlayer (pData);

    if (iRetcode)                      /* on error bail out */
      return iRetcode;

    pData->iLayerseq++;                /* and it counts as a layer then ! */
  }

  if (pData->fInitrowproc)             /* need to initialize row processing? */
  {
    iRetcode = ((mng_initrowproc)pData->fInitrowproc) (pData);
    pData->fInitrowproc = MNG_NULL;    /* only call this once !!! */
  }

  if ((!iRetcode) && (!pData->bInflating))
                                       /* initialize inflate */
    iRetcode = mngzlib_inflateinit (pData);

  if (!iRetcode)                       /* all ok? then inflate, my man */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    iRetcode = mngzlib_inflaterows (pData, iRawlen, pRawdata);
#else
    iRetcode = mngzlib_inflaterows (pData, pData->iRawlen, pData->pRawdata);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;
    
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_process_display_iend (mng_datap pData)
{
  mng_retcode iRetcode, iRetcode2;
  mng_bool bDodisplay = MNG_FALSE;
  mng_bool bMagnify   = MNG_FALSE;
  mng_bool bCleanup   = (mng_bool)(pData->iBreakpoint != 0);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IEND, MNG_LC_START);
#endif

#ifdef MNG_INCLUDE_JNG                 /* progressive+alpha JNG can be displayed now */
  if ( (pData->bHasJHDR                                         ) &&
       ( (pData->bJPEGprogressive) || (pData->bJPEGprogressive2)) &&
       ( (pData->eImagetype == mng_it_jng         ) ||
         (((mng_imagep)pData->pStoreobj)->bVisible)             ) &&
       ( (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAYA ) ||
         (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLORA)    )    )
    bDodisplay = MNG_TRUE;
#endif

#ifndef MNG_SKIPCHUNK_MAGN
  if ( (pData->pStoreobj) &&           /* on-the-fly magnification ? */
       ( (((mng_imagep)pData->pStoreobj)->iMAGN_MethodX) ||
         (((mng_imagep)pData->pStoreobj)->iMAGN_MethodY)    ) )
    bMagnify = MNG_TRUE;
#endif

  if ((pData->bHasBASI) ||             /* was it a BASI stream */
      (bDodisplay)      ||             /* or should we display the JNG */
#ifndef MNG_SKIPCHUNK_MAGN
      (bMagnify)        ||             /* or should we magnify it */
#endif
                                       /* or did we get broken here last time ? */
      ((pData->iBreakpoint) && (pData->iBreakpoint != 8)))
  {
    mng_imagep pImage = (mng_imagep)pData->pCurrentobj;

    if (!pImage)                       /* or was it object 0 ? */
      pImage = (mng_imagep)pData->pObjzero;
                                       /* display it now then ? */
    if ((pImage->bVisible) && (pImage->bViewable))
    {                                  /* ok, so do it */
      iRetcode = mng_display_image (pData, pImage, bDodisplay);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;

      if (pData->bTimerset)            /* timer break ? */
        pData->iBreakpoint = 6;
    }
  }
#ifndef MNG_NO_DELTA_PNG
  else
  if ((pData->bHasDHDR) ||             /* was it a DHDR stream */
      (pData->iBreakpoint == 8))       /* or did we get broken here last time ? */
  {
    mng_imagep pImage = (mng_imagep)pData->pDeltaImage;

    if (!pData->iBreakpoint)
    {                                  /* perform the delta operations needed */
      iRetcode = mng_execute_delta_image (pData, pImage, (mng_imagep)pData->pObjzero);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
    }
                                       /* display it now then ? */
    if ((pImage->bVisible) && (pImage->bViewable))
    {                                  /* ok, so do it */
      iRetcode = mng_display_image (pData, pImage, MNG_FALSE);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;

      if (pData->bTimerset)            /* timer break ? */
        pData->iBreakpoint = 8;
    }
  }
#endif

  if (!pData->bTimerset)               /* can we continue ? */
  {
    pData->iBreakpoint = 0;            /* clear this flag now ! */


#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    if (pData->eImagetype == mng_it_mpng)
    {
      pData->pCurraniobj = pData->pFirstaniobj;
    } else
#endif
#ifdef MNG_INCLUDE_ANG_PROPOSAL
    if (pData->eImagetype == mng_it_ang)
    {
      pData->pCurraniobj = pData->pFirstaniobj;
    } else
#endif
    {                                  /* cleanup object 0 */
      mng_reset_object_details (pData, (mng_imagep)pData->pObjzero,
                                0, 0, 0, 0, 0, 0, 0, MNG_TRUE);
    }

    if (pData->bInflating)             /* if we've been inflating */
    {                                  /* cleanup row-processing, */
      iRetcode  = mng_cleanup_rowproc (pData);
                                       /* also cleanup inflate! */
      iRetcode2 = mngzlib_inflatefree (pData);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
      if (iRetcode2)
        return iRetcode2;
    }

#ifdef MNG_INCLUDE_JNG
    if (pData->bJPEGdecompress)        /* if we've been decompressing JDAT */
    {                                  /* cleanup row-processing, */
      iRetcode  = mng_cleanup_rowproc (pData);
                                       /* also cleanup decompress! */
      iRetcode2 = mngjpeg_decompressfree (pData);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
      if (iRetcode2)
        return iRetcode2;
    }

    if (pData->bJPEGdecompress2)       /* if we've been decompressing JDAA */
    {                                  /* cleanup row-processing, */
      iRetcode  = mng_cleanup_rowproc (pData);
                                       /* also cleanup decompress! */
      iRetcode2 = mngjpeg_decompressfree2 (pData);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
      if (iRetcode2)
        return iRetcode2;
    }
#endif

    if (bCleanup)                      /* if we got broken last time we need to cleanup */
    {
      pData->bHasIHDR = MNG_FALSE;     /* IEND signals the end for most ... */
      pData->bHasBASI = MNG_FALSE;
      pData->bHasDHDR = MNG_FALSE;
#ifdef MNG_INCLUDE_JNG
      pData->bHasJHDR = MNG_FALSE;
      pData->bHasJSEP = MNG_FALSE;
      pData->bHasJDAA = MNG_FALSE;
      pData->bHasJDAT = MNG_FALSE;
#endif
      pData->bHasPLTE = MNG_FALSE;
      pData->bHasTRNS = MNG_FALSE;
      pData->bHasGAMA = MNG_FALSE;
      pData->bHasCHRM = MNG_FALSE;
      pData->bHasSRGB = MNG_FALSE;
      pData->bHasICCP = MNG_FALSE;
      pData->bHasBKGD = MNG_FALSE;
      pData->bHasIDAT = MNG_FALSE;
    }
                                       /* if the image was displayed on the fly, */
                                       /* we'll have to make the app refresh */
    if ((pData->eImagetype != mng_it_mng) && (pData->fDisplayrow))
      pData->bNeedrefresh = MNG_TRUE;
     
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

/* change in the MNG spec with regards to TERM delay & interframe_delay
   as proposed by Adam M. Costello (option 4) and finalized by official vote
   during december 2002 / check the 'mng-list' archives for more details */

mng_retcode mng_process_display_mend (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MEND, MNG_LC_START);
#endif

#ifdef MNG_SUPPORT_DYNAMICMNG
  if (pData->bStopafterseek)           /* need to stop after this ? */
  {
    pData->bFreezing      = MNG_TRUE;  /* stop processing on this one */
    pData->bRunningevent  = MNG_FALSE;
    pData->bStopafterseek = MNG_FALSE;
    pData->bNeedrefresh   = MNG_TRUE;  /* make sure the last bit is displayed ! */
  }
#endif

#ifndef MNG_SKIPCHUNK_TERM
                                       /* TERM processed ? */
  if ((pData->bDisplaying) && (pData->bRunning) &&
      (pData->bHasTERM) && (pData->pTermaniobj))
  {
    mng_retcode   iRetcode;
    mng_ani_termp pTERM;
                                       /* get the right animation object ! */
    pTERM = (mng_ani_termp)pData->pTermaniobj;

    pData->iIterations++;              /* increase iteration count */

    switch (pTERM->iTermaction)        /* determine what to do! */
    {
      case 0 : {                       /* show last frame indefinitly */
                 break;                /* piece of cake, that is... */
               }

      case 1 : {                       /* cease displaying anything */
                                       /* max(1, TERM delay, interframe_delay) */
#ifndef MNG_SKIPCHUNK_FRAM
                 if (pTERM->iDelay > pData->iFramedelay)
                   pData->iFramedelay = pTERM->iDelay;
                 if (!pData->iFramedelay)
                   pData->iFramedelay = 1;
#endif

                 iRetcode = interframe_delay (pData);
                                       /* no interframe_delay? then fake it */
                 if ((!iRetcode) && (!pData->bTimerset))
                   iRetcode = set_delay (pData, 1);

                 if (iRetcode)
                   return iRetcode;

                 pData->iBreakpoint = 10;
                 break;
               }

      case 2 : {                       /* show first image after TERM */
                 iRetcode = restore_state (pData);

                 if (iRetcode)         /* on error bail out */
                   return iRetcode;
                                       /* notify the app ? */
                 if (pData->fProcessmend)
                   if (!pData->fProcessmend ((mng_handle)pData, pData->iIterations, 0))
                     MNG_ERROR (pData, MNG_APPMISCERROR);

                                       /* show first frame after TERM chunk */
                 pData->pCurraniobj      = pTERM;
                 pData->bOnlyfirstframe  = MNG_TRUE;
                 pData->iFramesafterTERM = 0;

                                       /* max(1, TERM delay, interframe_delay) */
#ifndef MNG_SKIPCHUNK_FRAM
                 if (pTERM->iDelay > pData->iFramedelay)
                   pData->iFramedelay = pTERM->iDelay;
                 if (!pData->iFramedelay)
                   pData->iFramedelay = 1;
#endif

                 break;
               }

      case 3 : {                       /* repeat */
                 if ((pTERM->iItermax) && (pTERM->iItermax < 0x7FFFFFFF))
                   pTERM->iItermax--;

                 if (pTERM->iItermax)  /* go back to TERM ? */
                 {                     /* restore to initial or SAVE state */
                   iRetcode = restore_state (pData);

                   if (iRetcode)       /* on error bail out */
                     return iRetcode;
                                       /* notify the app ? */
                   if (pData->fProcessmend)
                     if (!pData->fProcessmend ((mng_handle)pData,
                                               pData->iIterations, pTERM->iItermax))
                       MNG_ERROR (pData, MNG_APPMISCERROR);

                                       /* restart from TERM chunk */
                   pData->pCurraniobj = pTERM;

                   if (pTERM->iDelay)  /* set the delay (?) */
                   {
                                       /* max(1, TERM delay, interframe_delay) */
#ifndef MNG_SKIPCHUNK_FRAM
                     if (pTERM->iDelay > pData->iFramedelay)
                       pData->iFramedelay = pTERM->iDelay;
                     if (!pData->iFramedelay)
                       pData->iFramedelay = 1;
#endif

                     pData->bNeedrefresh = MNG_TRUE;
                   }
                 }
                 else
                 {
                   switch (pTERM->iIteraction)
                   {
                     case 0 : {        /* show last frame indefinitly */
                                break; /* piece of cake, that is... */
                              }

                     case 1 : {        /* cease displaying anything */
                                       /* max(1, TERM delay, interframe_delay) */
#ifndef MNG_SKIPCHUNK_FRAM
                                if (pTERM->iDelay > pData->iFramedelay)
                                  pData->iFramedelay = pTERM->iDelay;
                                if (!pData->iFramedelay)
                                  pData->iFramedelay = 1;
#endif

                                iRetcode = interframe_delay (pData);
                                       /* no interframe_delay? then fake it */
                                if ((!iRetcode) && (!pData->bTimerset))
                                  iRetcode = set_delay (pData, 1);

                                if (iRetcode)
                                  return iRetcode;

                                pData->iBreakpoint = 10;
                                break;
                              }

                     case 2 : {        /* show first image after TERM */
                                iRetcode = restore_state (pData);
                                       /* on error bail out */
                                if (iRetcode)
                                  return iRetcode;
                                       /* notify the app ? */
                                if (pData->fProcessmend)
                                  if (!pData->fProcessmend ((mng_handle)pData,
                                                            pData->iIterations, 0))
                                    MNG_ERROR (pData, MNG_APPMISCERROR);

                                       /* show first frame after TERM chunk */
                                pData->pCurraniobj      = pTERM;
                                pData->bOnlyfirstframe  = MNG_TRUE;
                                pData->iFramesafterTERM = 0;
                                       /* max(1, TERM delay, interframe_delay) */
#ifndef MNG_SKIPCHUNK_FRAM
                                if (pTERM->iDelay > pData->iFramedelay)
                                  pData->iFramedelay = pTERM->iDelay;
                                if (!pData->iFramedelay)
                                  pData->iFramedelay = 1;
#endif

                                break;
                              }
                   }
                 }

                 break;
               }
    }
  }
#endif /* MNG_SKIPCHUNK_TERM */
                                       /* just reading ? */
  if ((!pData->bDisplaying) && (pData->bReading))
    if (pData->fProcessmend)           /* inform the app ? */
      if (!pData->fProcessmend ((mng_handle)pData, 0, 0))
        MNG_ERROR (pData, MNG_APPMISCERROR);

  if (!pData->pCurraniobj)             /* always let the app refresh at the end ! */
    pData->bNeedrefresh = MNG_TRUE;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_process_display_mend2 (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MEND, MNG_LC_START);
#endif

#ifndef MNG_SKIPCHUNK_FRAM
  pData->bFrameclipping = MNG_FALSE;   /* nothing to do but restore the app background */
#endif
  load_bkgdlayer (pData);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DEFI
mng_retcode mng_process_display_defi (mng_datap pData)
{
  mng_imagep pImage;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_DEFI, MNG_LC_START);
#endif

  if (!pData->iDEFIobjectid)           /* object id=0 ? */
  {
    pImage             = (mng_imagep)pData->pObjzero;

    if (pData->bDEFIhasdonotshow)
      pImage->bVisible = (mng_bool)(pData->iDEFIdonotshow == 0);

    if (pData->bDEFIhasloca)
    {
      pImage->iPosx    = pData->iDEFIlocax;
      pImage->iPosy    = pData->iDEFIlocay;
    }

    if (pData->bDEFIhasclip)
    {
      pImage->bClipped = pData->bDEFIhasclip;
      pImage->iClipl   = pData->iDEFIclipl;
      pImage->iClipr   = pData->iDEFIclipr;
      pImage->iClipt   = pData->iDEFIclipt;
      pImage->iClipb   = pData->iDEFIclipb;
    }

    pData->pCurrentobj = 0;            /* not a real object ! */
  }
  else
  {                                    /* already exists ? */
    pImage = (mng_imagep)mng_find_imageobject (pData, pData->iDEFIobjectid);

    if (!pImage)                       /* if not; create new */
    {
      mng_retcode iRetcode = mng_create_imageobject (pData, pData->iDEFIobjectid,
                                                     (mng_bool)(pData->iDEFIconcrete == 1),
                                                     (mng_bool)(pData->iDEFIdonotshow == 0),
                                                     MNG_FALSE, 0, 0, 0, 0, 0, 0, 0,
                                                     pData->iDEFIlocax, pData->iDEFIlocay,
                                                     pData->bDEFIhasclip,
                                                     pData->iDEFIclipl, pData->iDEFIclipr,
                                                     pData->iDEFIclipt, pData->iDEFIclipb,
                                                     &pImage);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
    }
    else
    {                                  /* exists; then set new info */
      if (pData->bDEFIhasdonotshow)
        pImage->bVisible = (mng_bool)(pData->iDEFIdonotshow == 0);

      pImage->bViewable  = MNG_FALSE;

      if (pData->bDEFIhasloca)
      {
        pImage->iPosx    = pData->iDEFIlocax;
        pImage->iPosy    = pData->iDEFIlocay;
      }

      if (pData->bDEFIhasclip)
      {
        pImage->bClipped = pData->bDEFIhasclip;
        pImage->iClipl   = pData->iDEFIclipl;
        pImage->iClipr   = pData->iDEFIclipr;
        pImage->iClipt   = pData->iDEFIclipt;
        pImage->iClipb   = pData->iDEFIclipb;
      }

      if (pData->bDEFIhasconcrete)
        pImage->pImgbuf->bConcrete = (mng_bool)(pData->iDEFIconcrete == 1);
    }

    pData->pCurrentobj = pImage;       /* others may want to know this */
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_DEFI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_BASI
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_basi (mng_datap  pData,
                                      mng_uint16 iRed,
                                      mng_uint16 iGreen,
                                      mng_uint16 iBlue,
                                      mng_bool   bHasalpha,
                                      mng_uint16 iAlpha,
                                      mng_uint8  iViewable)
#else
mng_retcode mng_process_display_basi (mng_datap  pData)
#endif
{                                      /* address the current "object" if any */
  mng_imagep     pImage = (mng_imagep)pData->pCurrentobj;
  mng_uint8p     pWork;
  mng_uint32     iX;
  mng_imagedatap pBuf;
  mng_retcode    iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_BASI, MNG_LC_START);
#endif

  if (!pImage)                         /* or is it an "on-the-fly" image ? */
    pImage = (mng_imagep)pData->pObjzero;
                                       /* address the object-buffer */
  pBuf               = pImage->pImgbuf;

  pData->fDisplayrow = MNG_NULL;       /* do nothing by default */
  pData->fCorrectrow = MNG_NULL;
  pData->fStorerow   = MNG_NULL;
  pData->fProcessrow = MNG_NULL;
                                       /* set parms now that they're known */
  iRetcode = mng_reset_object_details (pData, pImage, pData->iDatawidth,
                                       pData->iDataheight, pData->iBitdepth,
                                       pData->iColortype, pData->iCompression,
                                       pData->iFilter, pData->iInterlace, MNG_FALSE);
  if (iRetcode)                        /* on error bail out */
    return iRetcode;
                                       /* save the viewable flag */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  pImage->bViewable = (mng_bool)(iViewable == 1);
#else
  pImage->bViewable = (mng_bool)(pData->iBASIviewable == 1);
#endif
  pBuf->bViewable   = pImage->bViewable;
  pData->pStoreobj  = pImage;          /* let row-routines know which object */

  pWork = pBuf->pImgdata;              /* fill the object-buffer with the specified
                                          "color" sample */
  switch (pData->iColortype)           /* depending on color_type & bit_depth */
  {
    case 0 : {                         /* gray */
#ifndef MNG_NO_16BIT_SUPPORT
               if (pData->iBitdepth == 16)
               {
#ifdef MNG_DECREMENT_LOOPS
                 for (iX = pData->iDatawidth * pData->iDataheight;
                    iX > 0;iX--)
#else
                 for (iX = 0; iX < pData->iDatawidth * pData->iDataheight; iX++)
#endif
                 {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   mng_put_uint16 (pWork, iRed);
#else
                   mng_put_uint16 (pWork, pData->iBASIred);
#endif
                   pWork += 2;
                 }
               }
               else
#endif
               {
#ifdef MNG_DECREMENT_LOOPS
                 for (iX = pData->iDatawidth * pData->iDataheight;
                    iX > 0;iX--)
#else
                 for (iX = 0; iX < pData->iDatawidth * pData->iDataheight; iX++)
#endif
                 {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   *pWork = (mng_uint8)iRed;
#else
                   *pWork = (mng_uint8)pData->iBASIred;
#endif
                   pWork++;
                 }
               }
                                       /* force tRNS ? */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
               if ((bHasalpha) && (!iAlpha))
#else
               if ((pData->bBASIhasalpha) && (!pData->iBASIalpha))
#endif
               {
                 pBuf->bHasTRNS  = MNG_TRUE;
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                 pBuf->iTRNSgray = iRed;
#else
                 pBuf->iTRNSgray = pData->iBASIred;
#endif
               }

               break;
             }

    case 2 : {                         /* rgb */
#ifndef MNG_NO_16BIT_SUPPORT
               if (pData->iBitdepth == 16)
               {
#ifdef MNG_DECREMENT_LOOPS
                 for (iX = pData->iDatawidth * pData->iDataheight;
                    iX > 0;iX--)
#else
                 for (iX = 0; iX < pData->iDatawidth * pData->iDataheight; iX++)
#endif
                 {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   mng_put_uint16 (pWork,   iRed  );
                   mng_put_uint16 (pWork+2, iGreen);
                   mng_put_uint16 (pWork+4, iBlue );
#else
                   mng_put_uint16 (pWork,   pData->iBASIred  );
                   mng_put_uint16 (pWork+2, pData->iBASIgreen);
                   mng_put_uint16 (pWork+4, pData->iBASIblue );
#endif
                   pWork += 6;
                 }
               }
               else
#endif
               {
#ifdef MNG_DECREMENT_LOOPS
                 for (iX = pData->iDatawidth * pData->iDataheight;
                    iX > 0;iX--)
#else
                 for (iX = 0; iX < pData->iDatawidth * pData->iDataheight; iX++)
#endif
                 {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   *pWork     = (mng_uint8)iRed;
                   *(pWork+1) = (mng_uint8)iGreen;
                   *(pWork+2) = (mng_uint8)iBlue;
#else
                   *pWork     = (mng_uint8)pData->iBASIred;
                   *(pWork+1) = (mng_uint8)pData->iBASIgreen;
                   *(pWork+2) = (mng_uint8)pData->iBASIblue;
#endif
                   pWork += 3;
                 }
               }
                                       /* force tRNS ? */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
               if ((bHasalpha) && (!iAlpha))
#else
               if ((pData->bBASIhasalpha) && (!pData->iBASIalpha))
#endif
               {
                 pBuf->bHasTRNS   = MNG_TRUE;
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                 pBuf->iTRNSred   = iRed;
                 pBuf->iTRNSgreen = iGreen;
                 pBuf->iTRNSblue  = iBlue;
#else
                 pBuf->iTRNSred   = pData->iBASIred;
                 pBuf->iTRNSgreen = pData->iBASIgreen;
                 pBuf->iTRNSblue  = pData->iBASIblue;
#endif
               }

               break;
             }

    case 3 : {                         /* indexed */
               pBuf->bHasPLTE = MNG_TRUE;

               switch (pData->iBitdepth)
               {
                 case 1  : { pBuf->iPLTEcount =   2; break; }
                 case 2  : { pBuf->iPLTEcount =   4; break; }
                 case 4  : { pBuf->iPLTEcount =  16; break; }
                 case 8  : { pBuf->iPLTEcount = 256; break; }
                 default : { pBuf->iPLTEcount =   1; break; }
               }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
               pBuf->aPLTEentries [0].iRed   = (mng_uint8)iRed;
               pBuf->aPLTEentries [0].iGreen = (mng_uint8)iGreen;
               pBuf->aPLTEentries [0].iBlue  = (mng_uint8)iBlue;
#else
               pBuf->aPLTEentries [0].iRed   = (mng_uint8)pData->iBASIred;
               pBuf->aPLTEentries [0].iGreen = (mng_uint8)pData->iBASIgreen;
               pBuf->aPLTEentries [0].iBlue  = (mng_uint8)pData->iBASIblue;
#endif

               for (iX = 1; iX < pBuf->iPLTEcount; iX++)
               {
                 pBuf->aPLTEentries [iX].iRed   = 0;
                 pBuf->aPLTEentries [iX].iGreen = 0;
                 pBuf->aPLTEentries [iX].iBlue  = 0;
               }
                                       /* force tRNS ? */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
               if ((bHasalpha) && (iAlpha < 255))
#else
               if ((pData->bBASIhasalpha) && (pData->iBASIalpha < 255))
#endif
               {
                 pBuf->bHasTRNS         = MNG_TRUE;
                 pBuf->iTRNScount       = 1;
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                 pBuf->aTRNSentries [0] = (mng_uint8)iAlpha;
#else
                 pBuf->aTRNSentries [0] = (mng_uint8)pData->iBASIalpha;
#endif
               }

               break;
             }

    case 4 : {                         /* gray+alpha */
#ifndef MNG_NO_16BIT_SUPPORT
               if (pData->iBitdepth == 16)
               {
#ifdef MNG_DECREMENT_LOOPS
                 for (iX = pData->iDatawidth * pData->iDataheight;
                    iX > 0;iX--)
#else
                 for (iX = 0; iX < pData->iDatawidth * pData->iDataheight; iX++)
#endif
                 {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   mng_put_uint16 (pWork,   iRed);
                   mng_put_uint16 (pWork+2, iAlpha);
#else
                   mng_put_uint16 (pWork,   pData->iBASIred);
                   mng_put_uint16 (pWork+2, pData->iBASIalpha);
#endif
                   pWork += 4;
                 }
               }
               else
#endif
               {
#ifdef MNG_DECREMENT_LOOPS
                 for (iX = pData->iDatawidth * pData->iDataheight;
                    iX > 0;iX--)
#else
                 for (iX = 0; iX < pData->iDatawidth * pData->iDataheight; iX++)
#endif
                 {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   *pWork     = (mng_uint8)iRed;
                   *(pWork+1) = (mng_uint8)iAlpha;
#else
                   *pWork     = (mng_uint8)pData->iBASIred;
                   *(pWork+1) = (mng_uint8)pData->iBASIalpha;
#endif
                   pWork += 2;
                 }
               }

               break;
             }

    case 6 : {                         /* rgb+alpha */
#ifndef MNG_NO_16BIT_SUPPORT
               if (pData->iBitdepth == 16)
               {
#ifdef MNG_DECREMENT_LOOPS
                 for (iX = pData->iDatawidth * pData->iDataheight;
                    iX > 0;iX--)
#else
                 for (iX = 0; iX < pData->iDatawidth * pData->iDataheight; iX++)
#endif
                 {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   mng_put_uint16 (pWork,   iRed);
                   mng_put_uint16 (pWork+2, iGreen);
                   mng_put_uint16 (pWork+4, iBlue);
                   mng_put_uint16 (pWork+6, iAlpha);
#else
                   mng_put_uint16 (pWork,   pData->iBASIred);
                   mng_put_uint16 (pWork+2, pData->iBASIgreen);
                   mng_put_uint16 (pWork+4, pData->iBASIblue);
                   mng_put_uint16 (pWork+6, pData->iBASIalpha);
#endif
                   pWork += 8;
                 }
               }
               else
#endif
               {
#ifdef MNG_DECREMENT_LOOPS
                 for (iX = pData->iDatawidth * pData->iDataheight;
                    iX > 0;iX--)
#else
                 for (iX = 0; iX < pData->iDatawidth * pData->iDataheight; iX++)
#endif
                 {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   *pWork     = (mng_uint8)iRed;
                   *(pWork+1) = (mng_uint8)iGreen;
                   *(pWork+2) = (mng_uint8)iBlue;
                   *(pWork+3) = (mng_uint8)iAlpha;
#else
                   *pWork     = (mng_uint8)pData->iBASIred;
                   *(pWork+1) = (mng_uint8)pData->iBASIgreen;
                   *(pWork+2) = (mng_uint8)pData->iBASIblue;
                   *(pWork+3) = (mng_uint8)pData->iBASIalpha;
#endif
                   pWork += 4;
                 }
               }

               break;
             }

  }

#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
  pData->fInitrowproc = (mng_fptr)mng_init_rowproc;
  pData->ePng_imgtype=mng_png_imgtype(pData->iColortype,pData->iBitdepth);
#else
  switch (pData->iColortype)           /* determine row initialization routine */
  {                                    /* just to accomodate IDAT if it arrives */
    case 0 : {                         /* gray */
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
    case 2 : {                         /* rgb */
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
    case 3 : {                         /* indexed */
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
    case 4 : {                         /* gray+alpha */
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
    case 6 : {                         /* rgb+alpha */
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

  pData->iFilterofs = 0;               /* determine filter characteristics */
  pData->iLevel0    = 0;               /* default levels */
  pData->iLevel1    = 0;
  pData->iLevel2    = 0;
  pData->iLevel3    = 0;

#ifdef FILTER192
  if (pData->iFilter == 0xC0)          /* leveling & differing ? */
  {
    switch (pData->iColortype)
    {
      case 0 : {
#ifndef MNG_NO_16BIT_SUPPORT
                 if (pData->iBitdepth <= 8)
#endif
                   pData->iFilterofs = 1;
#ifndef MNG_NO_16BIT_SUPPORT
                 else
                   pData->iFilterofs = 2;
#endif

                 break;
               }
      case 2 : {
#ifndef MNG_NO_16BIT_SUPPORT
                 if (pData->iBitdepth <= 8)
#endif
                   pData->iFilterofs = 3;
#ifndef MNG_NO_16BIT_SUPPORT
                 else
                   pData->iFilterofs = 6;
#endif

                 break;
               }
      case 3 : {
                 pData->iFilterofs = 1;
                 break;
               }
      case 4 : {
#ifndef MNG_NO_16BIT_SUPPORT
                 if (pData->iBitdepth <= 8)
#endif
                   pData->iFilterofs = 2;
#ifndef MNG_NO_16BIT_SUPPORT
                 else
                   pData->iFilterofs = 4;
#endif

                 break;
               }
      case 6 : {
#ifndef MNG_NO_16BIT_SUPPORT
                 if (pData->iBitdepth <= 8)
#endif
                   pData->iFilterofs = 4;
#ifndef MNG_NO_16BIT_SUPPORT
                 else
                   pData->iFilterofs = 8;
#endif

                 break;
               }
    }
  }
#endif

#ifdef FILTER193
  if (pData->iFilter == 0xC1)          /* no adaptive filtering ? */
    pData->iPixelofs = pData->iFilterofs;
  else
#endif
    pData->iPixelofs = pData->iFilterofs + 1;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_BASI, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLON
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_clon (mng_datap  pData,
                                      mng_uint16 iSourceid,
                                      mng_uint16 iCloneid,
                                      mng_uint8  iClonetype,
                                      mng_bool   bHasdonotshow,
                                      mng_uint8  iDonotshow,
                                      mng_uint8  iConcrete,
                                      mng_bool   bHasloca,
                                      mng_uint8  iLocationtype,
                                      mng_int32  iLocationx,
                                      mng_int32  iLocationy)
#else
mng_retcode mng_process_display_clon (mng_datap  pData)
#endif
{
  mng_imagep  pSource, pClone;
  mng_bool    bVisible, bAbstract;
  mng_retcode iRetcode = MNG_NOERROR;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_CLON, MNG_LC_START);
#endif
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                                       /* locate the source object first */
  pSource = mng_find_imageobject (pData, iSourceid);
                                       /* check if the clone exists */
  pClone  = mng_find_imageobject (pData, iCloneid);
#else
                                       /* locate the source object first */
  pSource = mng_find_imageobject (pData, pData->iCLONsourceid);
                                       /* check if the clone exists */
  pClone  = mng_find_imageobject (pData, pData->iCLONcloneid);
#endif

  if (!pSource)                        /* source must exist ! */
    MNG_ERROR (pData, MNG_OBJECTUNKNOWN);

  if (pClone)                          /* clone must not exist ! */
    MNG_ERROR (pData, MNG_OBJECTEXISTS);

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  if (bHasdonotshow)                   /* DoNotShow flag filled ? */
    bVisible = (mng_bool)(iDonotshow == 0);
  else
    bVisible = pSource->bVisible;
#else
  if (pData->bCLONhasdonotshow)        /* DoNotShow flag filled ? */
    bVisible = (mng_bool)(pData->iCLONdonotshow == 0);
  else
    bVisible = pSource->bVisible;
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  bAbstract  = (mng_bool)(iConcrete == 1);
#else
  bAbstract  = (mng_bool)(pData->iCLONconcrete == 1);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  switch (iClonetype)                  /* determine action to take */
  {
    case 0 : {                         /* full clone */
               iRetcode = mng_clone_imageobject (pData, iCloneid, MNG_FALSE,
                                                 bVisible, bAbstract, bHasloca,
                                                 iLocationtype, iLocationx, iLocationy,
                                                 pSource, &pClone);
               break;
             }

    case 1 : {                         /* partial clone */
               iRetcode = mng_clone_imageobject (pData, iCloneid, MNG_TRUE,
                                                 bVisible, bAbstract, bHasloca,
                                                 iLocationtype, iLocationx, iLocationy,
                                                 pSource, &pClone);
               break;
             }

    case 2 : {                         /* renumber object */
               iRetcode = mng_renum_imageobject (pData, pSource, iCloneid,
                                                 bVisible, bAbstract, bHasloca,
                                                 iLocationtype, iLocationx, iLocationy);
               pClone   = pSource;
               break;
             }

  }
#else
  switch (pData->iCLONclonetype)       /* determine action to take */
  {
    case 0 : {                         /* full clone */
               iRetcode = mng_clone_imageobject (pData, pData->iCLONcloneid, MNG_FALSE,
                                                 bVisible, bAbstract,
                                                 pData->bCLONhasloca, pData->iCLONlocationtype,
                                                 pData->iCLONlocationx, pData->iCLONlocationy,
                                                 pSource, &pClone);
               break;
             }

    case 1 : {                         /* partial clone */
               iRetcode = mng_clone_imageobject (pData, pData->iCLONcloneid, MNG_TRUE,
                                                 bVisible, bAbstract,
                                                 pData->bCLONhasloca, pData->iCLONlocationtype,
                                                 pData->iCLONlocationx, pData->iCLONlocationy,
                                                 pSource, &pClone);
               break;
             }

    case 2 : {                         /* renumber object */
               iRetcode = mng_renum_imageobject (pData, pSource, pData->iCLONcloneid,
                                                 bVisible, bAbstract,
                                                 pData->bCLONhasloca, pData->iCLONlocationtype,
                                                 pData->iCLONlocationx, pData->iCLONlocationy);
               pClone   = pSource;
               break;
             }

  }
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

                                       /* display on the fly ? */
  if ((pClone->bViewable) && (pClone->bVisible))
  {
    pData->pLastclone = pClone;        /* remember in case of timer break ! */
                                       /* display it */
    mng_display_image (pData, pClone, MNG_FALSE);

    if (pData->bTimerset)              /* timer break ? */
      pData->iBreakpoint = 5;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_CLON, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_process_display_clon2 (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_CLON, MNG_LC_START);
#endif
                                       /* only called after timer break ! */
  mng_display_image (pData, (mng_imagep)pData->pLastclone, MNG_FALSE);
  pData->iBreakpoint = 0;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_CLON, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DISC
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_disc (mng_datap   pData,
                                      mng_uint32  iCount,
                                      mng_uint16p pIds)
#else
mng_retcode mng_process_display_disc (mng_datap   pData)
#endif
{
  mng_uint32 iX;
  mng_imagep pImage;
  mng_uint32 iRetcode;
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_DISC, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  if (iCount)                          /* specific list ? */
#else
  if (pData->iDISCcount)               /* specific list ? */
#endif
  {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    mng_uint16p pWork = pIds;
#else
    mng_uint16p pWork = pData->pDISCids;
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
#ifdef MNG_DECREMENT_LOOPS             /* iterate the list */
    for (iX = iCount; iX > 0; iX--)
#else
    for (iX = 0; iX < iCount; iX++)
#endif
#else
#ifdef MNG_DECREMENT_LOOPS             /* iterate the list */
    for (iX = pData->iDISCcount; iX > 0; iX--)
#else
    for (iX = 0; iX < pData->iDISCcount; iX++)
#endif
#endif
    {
      pImage = mng_find_imageobject (pData, *pWork++);

      if (pImage)                      /* found the object ? */
      {                                /* then drop it */
        iRetcode = mng_free_imageobject (pData, pImage);

        if (iRetcode)                  /* on error bail out */
          return iRetcode;
      }
    }
  }
  else                                 /* empty: drop all un-frozen objects */
  {
    mng_imagep pNext = (mng_imagep)pData->pFirstimgobj;

    while (pNext)                      /* any left ? */
    {
      pImage = pNext;
      pNext  = pImage->sHeader.pNext;

      if (!pImage->bFrozen)            /* not frozen ? */
      {                                /* then drop it */
        iRetcode = mng_free_imageobject (pData, pImage);
                       
        if (iRetcode)                  /* on error bail out */
          return iRetcode;
      }
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_DISC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_FRAM
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_fram (mng_datap  pData,
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
mng_retcode mng_process_display_fram (mng_datap  pData)
#endif
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_FRAM, MNG_LC_START);
#endif
                                       /* advance a frame then */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iRetcode = next_frame (pData, iFramemode, iChangedelay, iDelay,
                         iChangetimeout, iTimeout, iChangeclipping,
                         iCliptype, iClipl, iClipr, iClipt, iClipb);
#else
  iRetcode = next_frame (pData, pData->iTempFramemode, pData->iTempChangedelay,
                         pData->iTempDelay, pData->iTempChangetimeout,
                         pData->iTempTimeout, pData->iTempChangeclipping,
                         pData->iTempCliptype, pData->iTempClipl, pData->iTempClipr,
                         pData->iTempClipt, pData->iTempClipb);
#endif

  if (pData->bTimerset)                /* timer break ? */
    pData->iBreakpoint = 1;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_FRAM, MNG_LC_END);
#endif

  return iRetcode;
}

/* ************************************************************************** */

mng_retcode mng_process_display_fram2 (mng_datap pData)
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_FRAM, MNG_LC_START);
#endif
                                       /* again; after the break */
  iRetcode = next_frame (pData, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  pData->iBreakpoint = 0;              /* not again! */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_FRAM, MNG_LC_END);
#endif

  return iRetcode;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MOVE
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_move (mng_datap  pData,
                                      mng_uint16 iFromid,
                                      mng_uint16 iToid,
                                      mng_uint8  iMovetype,
                                      mng_int32  iMovex,
                                      mng_int32  iMovey)
#else
mng_retcode mng_process_display_move (mng_datap  pData)
#endif
{
  mng_uint16 iX;
  mng_imagep pImage;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MOVE, MNG_LC_START);
#endif
                                       /* iterate the list */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  for (iX = iFromid; iX <= iToid; iX++)
#else
  for (iX = pData->iMOVEfromid; iX <= pData->iMOVEtoid; iX++)
#endif
  {
    if (!iX)                           /* object id=0 ? */
      pImage = (mng_imagep)pData->pObjzero;
    else
      pImage = mng_find_imageobject (pData, iX);

    if (pImage)                        /* object exists ? */
    {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      switch (iMovetype)
#else
      switch (pData->iMOVEmovetype)
#endif
      {
        case 0 : {                     /* absolute */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   pImage->iPosx = iMovex;
                   pImage->iPosy = iMovey;
#else
                   pImage->iPosx = pData->iMOVEmovex;
                   pImage->iPosy = pData->iMOVEmovey;
#endif
                   break;
                 }
        case 1 : {                     /* relative */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   pImage->iPosx = pImage->iPosx + iMovex;
                   pImage->iPosy = pImage->iPosy + iMovey;
#else
                   pImage->iPosx = pImage->iPosx + pData->iMOVEmovex;
                   pImage->iPosy = pImage->iPosy + pData->iMOVEmovey;
#endif
                   break;
                 }
      }
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MOVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_CLIP
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_clip (mng_datap  pData,
                                      mng_uint16 iFromid,
                                      mng_uint16 iToid,
                                      mng_uint8  iCliptype,
                                      mng_int32  iClipl,
                                      mng_int32  iClipr,
                                      mng_int32  iClipt,
                                      mng_int32  iClipb)
#else
mng_retcode mng_process_display_clip (mng_datap  pData)
#endif
{
  mng_uint16 iX;
  mng_imagep pImage;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_CLIP, MNG_LC_START);
#endif
                                       /* iterate the list */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  for (iX = iFromid; iX <= iToid; iX++)
#else
  for (iX = pData->iCLIPfromid; iX <= pData->iCLIPtoid; iX++)
#endif
  {
    if (!iX)                           /* object id=0 ? */
      pImage = (mng_imagep)pData->pObjzero;
    else
      pImage = mng_find_imageobject (pData, iX);

    if (pImage)                        /* object exists ? */
    {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      switch (iCliptype)
#else
      switch (pData->iCLIPcliptype)
#endif
      {
        case 0 : {                     /* absolute */
                   pImage->bClipped = MNG_TRUE;
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   pImage->iClipl   = iClipl;
                   pImage->iClipr   = iClipr;
                   pImage->iClipt   = iClipt;
                   pImage->iClipb   = iClipb;
#else
                   pImage->iClipl   = pData->iCLIPclipl;
                   pImage->iClipr   = pData->iCLIPclipr;
                   pImage->iClipt   = pData->iCLIPclipt;
                   pImage->iClipb   = pData->iCLIPclipb;
#endif
                   break;
                 }
        case 1 : {                    /* relative */
                   pImage->bClipped = MNG_TRUE;
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   pImage->iClipl   = pImage->iClipl + iClipl;
                   pImage->iClipr   = pImage->iClipr + iClipr;
                   pImage->iClipt   = pImage->iClipt + iClipt;
                   pImage->iClipb   = pImage->iClipb + iClipb;
#else
                   pImage->iClipl   = pImage->iClipl + pData->iCLIPclipl;
                   pImage->iClipr   = pImage->iClipr + pData->iCLIPclipr;
                   pImage->iClipt   = pImage->iClipt + pData->iCLIPclipt;
                   pImage->iClipb   = pImage->iClipb + pData->iCLIPclipb;
#endif
                   break;
                 }
      }
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_CLIP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SHOW
mng_retcode mng_process_display_show (mng_datap pData)
{
  mng_int16  iX, iS, iFrom, iTo;
  mng_imagep pImage;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_SHOW, MNG_LC_START);
#endif

  /* TODO: optimization for the cases where "abs (iTo - iFrom)" is rather high;
     especially where ((iFrom==1) && (iTo==65535)); eg. an empty SHOW !!! */

  if (pData->iBreakpoint == 3)         /* previously broken during cycle-mode ? */
  {
    pImage = mng_find_imageobject (pData, pData->iSHOWnextid);
                 
    if (pImage)                        /* still there ? */
      mng_display_image (pData, pImage, MNG_FALSE);

    pData->iBreakpoint = 0;            /* let's not go through this again! */
  }
  else
  {
    if (pData->iBreakpoint)            /* previously broken at other point ? */
    {                                  /* restore last parms */
      iFrom = (mng_int16)pData->iSHOWfromid;
      iTo   = (mng_int16)pData->iSHOWtoid;
      iX    = (mng_int16)pData->iSHOWnextid;
      iS    = (mng_int16)pData->iSHOWskip;
    }
    else
    {                                  /* regular sequence ? */
      if (pData->iSHOWtoid >= pData->iSHOWfromid)
        iS  = 1;
      else                             /* reverse sequence ! */
        iS  = -1;

      iFrom = (mng_int16)pData->iSHOWfromid;
      iTo   = (mng_int16)pData->iSHOWtoid;
      iX    = iFrom;

      pData->iSHOWfromid = (mng_uint16)iFrom;
      pData->iSHOWtoid   = (mng_uint16)iTo;
      pData->iSHOWskip   = iS;
    }
                                       /* cycle mode ? */
    if ((pData->iSHOWmode == 6) || (pData->iSHOWmode == 7))
    {
      mng_uint16 iTrigger = 0;
      mng_uint16 iFound   = 0;
      mng_uint16 iPass    = 0;
      mng_imagep pFound   = 0;

      do
      {
        iPass++;                       /* lets prevent endless loops when there
                                          are no potential candidates in the list! */

        if (iS > 0)                    /* forward ? */
        {
          for (iX = iFrom; iX <= iTo; iX += iS)
          {
            pImage = mng_find_imageobject (pData, (mng_uint16)iX);
                         
            if (pImage)                /* object exists ? */
            {
              if (iFound)              /* already found a candidate ? */
                pImage->bVisible = MNG_FALSE;
              else
              if (iTrigger)            /* found the trigger ? */
              {
                pImage->bVisible = MNG_TRUE;
                iFound           = iX;
                pFound           = pImage;
              }
              else
              if (pImage->bVisible)    /* ok, this is the trigger */
              {
                pImage->bVisible = MNG_FALSE;
                iTrigger         = iX;
              }
            }
          }
        }
        else
        {
          for (iX = iFrom; iX >= iTo; iX += iS)
          {
            pImage = mng_find_imageobject (pData, (mng_uint16)iX);
                         
            if (pImage)                /* object exists ? */
            {
              if (iFound)              /* already found a candidate ? */
                pImage->bVisible = MNG_FALSE;
              else
              if (iTrigger)            /* found the trigger ? */
              {
                pImage->bVisible = MNG_TRUE;
                iFound           = iX;
                pFound           = pImage;
              }
              else
              if (pImage->bVisible)    /* ok, this is the trigger */
              {
                pImage->bVisible = MNG_FALSE;
                iTrigger         = iX;
              }
            }
          }
        }

        if (!iTrigger)                 /* did not find a trigger ? */
          iTrigger = 1;                /* then fake it so the first image
                                          gets nominated */
      }                                /* cycle back to beginning ? */
      while ((iPass < 2) && (iTrigger) && (!iFound));

      pData->iBreakpoint = 0;          /* just a sanity precaution */
                                       /* display it ? */
      if ((pData->iSHOWmode == 6) && (pFound))
      {
        mng_display_image (pData, pFound, MNG_FALSE);

        if (pData->bTimerset)          /* timer set ? */
        {
          pData->iBreakpoint = 3;
          pData->iSHOWnextid = iFound; /* save it for after the break */
        }
      }
    }
    else
    {
      do
      {
        pImage = mng_find_imageobject (pData, iX);
                     
        if (pImage)                    /* object exists ? */
        {
          if (pData->iBreakpoint)      /* did we get broken last time ? */
          {                            /* could only happen in the display routine */
            mng_display_image (pData, pImage, MNG_FALSE);
            pData->iBreakpoint = 0;    /* only once inside this loop please ! */
          }
          else
          {
            switch (pData->iSHOWmode)  /* do what ? */
            {
              case 0 : {
                         pImage->bVisible = MNG_TRUE;
                         mng_display_image (pData, pImage, MNG_FALSE);
                         break;
                       }
              case 1 : {
                         pImage->bVisible = MNG_FALSE;
                         break;
                       }
              case 2 : {
                         if (pImage->bVisible)
                           mng_display_image (pData, pImage, MNG_FALSE);
                         break;
                       }
              case 3 : {
                         pImage->bVisible = MNG_TRUE;
                         break;
                       }
              case 4 : {
                         pImage->bVisible = (mng_bool)(!pImage->bVisible);
                         if (pImage->bVisible)
                           mng_display_image (pData, pImage, MNG_FALSE);
                         break;
                       }
              case 5 : {
                         pImage->bVisible = (mng_bool)(!pImage->bVisible);
                       }
            }
          }
        }

        if (!pData->bTimerset)         /* next ? */
          iX += iS;

      }                                /* continue ? */
      while ((!pData->bTimerset) && (((iS > 0) && (iX <= iTo)) ||
                                     ((iS < 0) && (iX >= iTo))    ));

      if (pData->bTimerset)            /* timer set ? */
      {
        pData->iBreakpoint = 4;
        pData->iSHOWnextid = iX;       /* save for next time */
      }
      else
        pData->iBreakpoint = 0;
        
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_SHOW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode mng_process_display_save (mng_datap pData)
{
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_SAVE, MNG_LC_START);
#endif

  iRetcode = save_state (pData);       /* save the current state */

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_SAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode mng_process_display_seek (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_SEEK, MNG_LC_START);
#endif

#ifdef MNG_SUPPORT_DYNAMICMNG
  if (pData->bStopafterseek)           /* need to stop after this SEEK ? */
  {
    pData->bFreezing      = MNG_TRUE;  /* stop processing on this one */
    pData->bRunningevent  = MNG_FALSE;
    pData->bStopafterseek = MNG_FALSE;
    pData->bNeedrefresh   = MNG_TRUE;  /* make sure the last bit is displayed ! */
  }
  else
#endif
  {                                    /* restore the initial or SAVE state */
    mng_retcode iRetcode = restore_state (pData);

    if (iRetcode)                      /* on error bail out */
      return iRetcode;

#ifdef MNG_SUPPORT_DYNAMICMNG
                                       /* stop after next SEEK ? */
    if ((pData->bDynamic) || (pData->bRunningevent))
      pData->bStopafterseek = MNG_TRUE;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_SEEK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
mng_retcode mng_process_display_jhdr (mng_datap pData)
{                                      /* address the current "object" if any */
  mng_imagep  pImage   = (mng_imagep)pData->pCurrentobj;
  mng_retcode iRetcode = MNG_NOERROR;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_JHDR, MNG_LC_START);
#endif

  if (!pData->bHasDHDR)
  {
    pData->fInitrowproc  = MNG_NULL;   /* do nothing by default */
    pData->fDisplayrow   = MNG_NULL;
    pData->fCorrectrow   = MNG_NULL;
    pData->fStorerow     = MNG_NULL;
    pData->fProcessrow   = MNG_NULL;
    pData->fDifferrow    = MNG_NULL;
    pData->fStorerow2    = MNG_NULL;
    pData->fStorerow3    = MNG_NULL;

    pData->pStoreobj     = MNG_NULL;   /* initialize important work-parms */

    pData->iJPEGrow      = 0;
    pData->iJPEGalpharow = 0;
    pData->iJPEGrgbrow   = 0;
    pData->iRowmax       = 0;          /* so init_rowproc does the right thing ! */
  }

  if (!pData->iBreakpoint)             /* not previously broken ? */
  {
#ifndef MNG_NO_DELTA_PNG
    if (pData->bHasDHDR)               /* delta-image ? */
    {
      if (pData->iDeltatype == MNG_DELTATYPE_REPLACE)
      {
        iRetcode = mng_reset_object_details (pData, (mng_imagep)pData->pDeltaImage,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iJHDRimgbitdepth, pData->iJHDRcolortype,
                                             pData->iJHDRalphacompression, pData->iJHDRalphafilter,
                                             pData->iJHDRalphainterlace, MNG_TRUE);

        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iAlphabitdepth    = pData->iJHDRalphabitdepth;
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iJHDRcompression  = pData->iJHDRimgcompression;
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iJHDRinterlace    = pData->iJHDRimginterlace;
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iAlphasampledepth = pData->iJHDRalphabitdepth;
      }
      else
      if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
          (pData->iDeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
      {
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iPixelsampledepth = pData->iJHDRimgbitdepth;
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iAlphasampledepth = pData->iJHDRalphabitdepth;
      }
      else
      if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKALPHAADD    ) ||
          (pData->iDeltatype == MNG_DELTATYPE_BLOCKALPHAREPLACE)    )
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iAlphasampledepth = pData->iJHDRalphabitdepth;
      else
      if ((pData->iDeltatype == MNG_DELTATYPE_BLOCKCOLORADD    ) ||
          (pData->iDeltatype == MNG_DELTATYPE_BLOCKCOLORREPLACE)    )
        ((mng_imagep)pData->pDeltaImage)->pImgbuf->iPixelsampledepth = pData->iJHDRimgbitdepth;
        
    }
    else
#endif /* MNG_NO_DELTA_PNG */
    {
      if (pImage)                      /* update object buffer ? */
      {
        iRetcode = mng_reset_object_details (pData, pImage,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iJHDRimgbitdepth, pData->iJHDRcolortype,
                                             pData->iJHDRalphacompression, pData->iJHDRalphafilter,
                                             pData->iJHDRalphainterlace, MNG_TRUE);

        pImage->pImgbuf->iAlphabitdepth    = pData->iJHDRalphabitdepth;
        pImage->pImgbuf->iJHDRcompression  = pData->iJHDRimgcompression;
        pImage->pImgbuf->iJHDRinterlace    = pData->iJHDRimginterlace;
        pImage->pImgbuf->iAlphasampledepth = pData->iJHDRalphabitdepth;
      }
      else                             /* update object 0 */
      {
        iRetcode = mng_reset_object_details (pData, (mng_imagep)pData->pObjzero,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iJHDRimgbitdepth, pData->iJHDRcolortype,
                                             pData->iJHDRalphacompression, pData->iJHDRalphafilter,
                                             pData->iJHDRalphainterlace, MNG_TRUE);

        ((mng_imagep)pData->pObjzero)->pImgbuf->iAlphabitdepth    = pData->iJHDRalphabitdepth;
        ((mng_imagep)pData->pObjzero)->pImgbuf->iJHDRcompression  = pData->iJHDRimgcompression;
        ((mng_imagep)pData->pObjzero)->pImgbuf->iJHDRinterlace    = pData->iJHDRimginterlace;
        ((mng_imagep)pData->pObjzero)->pImgbuf->iAlphasampledepth = pData->iJHDRalphabitdepth;
      }
    }

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }

  if (!pData->bHasDHDR)
  {                                    /* we're always storing a JPEG */
    if (pImage)                        /* real object ? */
      pData->pStoreobj = pImage;       /* tell the row routines */
    else                               /* otherwise use object 0 */
      pData->pStoreobj = pData->pObjzero;
                                       /* display "on-the-fly" ? */
    if (
#ifndef MNG_SKIPCHUNK_MAGN
         ( ((mng_imagep)pData->pStoreobj)->iMAGN_MethodX == 0) &&
         ( ((mng_imagep)pData->pStoreobj)->iMAGN_MethodY == 0) &&
#endif
         ( (pData->eImagetype == mng_it_jng         ) ||
           (((mng_imagep)pData->pStoreobj)->bVisible)    )       )
    {
      next_layer (pData);              /* that's a new layer then ! */

      pData->iBreakpoint = 0;

      if (pData->bTimerset)            /* timer break ? */
        pData->iBreakpoint = 7;
      else
      if (pData->bRunning)             /* still running ? */
      {                                /* anything to display ? */
        if ((pData->iDestr > pData->iDestl) && (pData->iDestb > pData->iDestt))
        {
          set_display_routine (pData); /* then determine display routine */
                                       /* display from the object we store in */
          pData->pRetrieveobj = pData->pStoreobj;
        }
      }
    }
  }

  if (!pData->bTimerset)               /* no timer break ? */
  {                                    /* default row initialization ! */
#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
    pData->ePng_imgtype=png_none;
#endif
    pData->fInitrowproc = (mng_fptr)mng_init_rowproc;

    if ((!pData->bHasDHDR) || (pData->iDeltatype == MNG_DELTATYPE_REPLACE))
    {                                  /* 8-bit JPEG ? */
      if (pData->iJHDRimgbitdepth == 8)
      {                                /* intermediate row is 8-bit deep */
        pData->bIsRGBA16   = MNG_FALSE;
        pData->iRowsamples = pData->iDatawidth;

        switch (pData->iJHDRcolortype) /* determine pixel processing routines */
        {
          case MNG_COLORTYPE_JPEGGRAY :
               {
                 pData->fStorerow2   = (mng_fptr)mng_store_jpeg_g8;
                 pData->fRetrieverow = (mng_fptr)mng_retrieve_g8;
                 pData->bIsOpaque    = MNG_TRUE;
                 break;
               }
          case MNG_COLORTYPE_JPEGCOLOR :
               {
                 pData->fStorerow2   = (mng_fptr)mng_store_jpeg_rgb8;
                 pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb8;
                 pData->bIsOpaque    = MNG_TRUE;
                 break;
               }
          case MNG_COLORTYPE_JPEGGRAYA :
               {
                 pData->fStorerow2   = (mng_fptr)mng_store_jpeg_ga8;
                 pData->fRetrieverow = (mng_fptr)mng_retrieve_ga8;
                 pData->bIsOpaque    = MNG_FALSE;
                 break;
               }
          case MNG_COLORTYPE_JPEGCOLORA :
               {
                 pData->fStorerow2   = (mng_fptr)mng_store_jpeg_rgba8;
                 pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba8;
                 pData->bIsOpaque    = MNG_FALSE;
                 break;
               }
        }
      }
#ifndef MNG_NO_16BIT_SUPPORT
      else
      {
        pData->bIsRGBA16 = MNG_TRUE;   /* intermediate row is 16-bit deep */

        /* TODO: 12-bit JPEG */
        /* TODO: 8- + 12-bit JPEG (eg. type=20) */

      }
#endif
                                       /* possible IDAT alpha-channel ? */
      if (pData->iJHDRalphacompression == MNG_COMPRESSION_DEFLATE)
      {
                                       /* determine alpha processing routine */
#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
        pData->fInitrowproc = (mng_fptr)mng_init_rowproc;
#endif
        switch (pData->iJHDRalphabitdepth)
        {
#ifndef MNG_OPTIMIZE_FOOTPRINT_INIT
#ifndef MNG_NO_1_2_4BIT_SUPPORT
          case  1 : { pData->fInitrowproc = (mng_fptr)mng_init_jpeg_a1_ni;  break; }
          case  2 : { pData->fInitrowproc = (mng_fptr)mng_init_jpeg_a2_ni;  break; }
          case  4 : { pData->fInitrowproc = (mng_fptr)mng_init_jpeg_a4_ni;  break; }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
          case  8 : { pData->fInitrowproc = (mng_fptr)mng_init_jpeg_a8_ni;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
          case 16 : { pData->fInitrowproc = (mng_fptr)mng_init_jpeg_a16_ni; break; }
#endif
#else
#ifndef MNG_NO_1_2_4BIT_SUPPORT
          case  1 : { pData->ePng_imgtype = png_jpeg_a1;  break; }
          case  2 : { pData->ePng_imgtype = png_jpeg_a2;  break; }
          case  4 : { pData->ePng_imgtype = png_jpeg_a4;  break; }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
          case  8 : { pData->ePng_imgtype = png_jpeg_a8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
          case 16 : { pData->ePng_imgtype = png_jpeg_a16; break; }
#endif
#endif
        }
      }
      else                             /* possible JDAA alpha-channel ? */
      if (pData->iJHDRalphacompression == MNG_COMPRESSION_BASELINEJPEG)
      {                                /* 8-bit JPEG ? */
        if (pData->iJHDRimgbitdepth == 8)
        {
          if (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAYA)
            pData->fStorerow3 = (mng_fptr)mng_store_jpeg_g8_alpha;
          else
          if (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLORA)
            pData->fStorerow3 = (mng_fptr)mng_store_jpeg_rgb8_alpha;
        }
        else
        {
          /* TODO: 12-bit JPEG with 8-bit JDAA */
        }
      }
                                       /* initialize JPEG library */
      iRetcode = mngjpeg_initialize (pData);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
    }
    else
    {                                  /* must be alpha add/replace !! */
      if ((pData->iDeltatype != MNG_DELTATYPE_BLOCKALPHAADD    ) &&
          (pData->iDeltatype != MNG_DELTATYPE_BLOCKALPHAREPLACE)    )
        MNG_ERROR (pData, MNG_INVDELTATYPE);
                                       /* determine alpha processing routine */
#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
        pData->fInitrowproc = (mng_fptr)mng_init_rowproc;
#endif
      switch (pData->iJHDRalphabitdepth)
      {
#ifndef MNG_OPTIMIZE_FOOTPRINT_INIT
#ifndef MNG_NO_1_2_4BIT_SUPPORT
        case  1 : { pData->fInitrowproc = (mng_fptr)mng_init_g1_ni;  break; }
        case  2 : { pData->fInitrowproc = (mng_fptr)mng_init_g2_ni;  break; }
        case  4 : { pData->fInitrowproc = (mng_fptr)mng_init_g4_ni;  break; }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
        case  8 : { pData->fInitrowproc = (mng_fptr)mng_init_g8_ni;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
        case 16 : { pData->fInitrowproc = (mng_fptr)mng_init_g16_ni; break; }
#endif
#else
#ifndef MNG_NO_1_2_4BIT_SUPPORT
        case  1 : { pData->ePng_imgtype = png_jpeg_a1;  break; }
        case  2 : { pData->ePng_imgtype = png_jpeg_a2;  break; }
        case  4 : { pData->ePng_imgtype = png_jpeg_a4;  break; }
#endif /* MNG_NO_1_2_4BIT_SUPPORT */
        case  8 : { pData->ePng_imgtype = png_jpeg_a8;  break; }
#ifndef MNG_NO_16BIT_SUPPORT
        case 16 : { pData->ePng_imgtype = png_jpeg_a16; break; }
#endif
#endif /* MNG_OPTIMIZE_FOOTPRINT_INIT */
      }
    }

    pData->iFilterofs = 0;             /* determine filter characteristics */
    pData->iLevel0    = 0;             /* default levels */
    pData->iLevel1    = 0;    
    pData->iLevel2    = 0;
    pData->iLevel3    = 0;

#ifdef FILTER192                       /* leveling & differing ? */
    if (pData->iJHDRalphafilter == 0xC0)
    {
       if (pData->iJHDRalphabitdepth <= 8)
         pData->iFilterofs = 1;
       else
         pData->iFilterofs = 2;

    }
#endif
#ifdef FILTER193                       /* no adaptive filtering ? */
    if (pData->iJHDRalphafilter == 0xC1)
      pData->iPixelofs = pData->iFilterofs;
    else
#endif
      pData->iPixelofs = pData->iFilterofs + 1;

  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_JHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_jdaa (mng_datap  pData,
                                      mng_uint32 iRawlen,
                                      mng_uint8p pRawdata)
#else
mng_retcode mng_process_display_jdaa (mng_datap  pData)
#endif
{
  mng_retcode iRetcode = MNG_NOERROR;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_JDAA, MNG_LC_START);
#endif

  if (!pData->bJPEGdecompress2)        /* if we're not decompressing already */
  {
    if (pData->fInitrowproc)           /* initialize row-processing? */
    {
      iRetcode = ((mng_initrowproc)pData->fInitrowproc) (pData);
      pData->fInitrowproc = MNG_NULL;  /* only call this once !!! */
    }

    if (!iRetcode)                     /* initialize decompress */
      iRetcode = mngjpeg_decompressinit2 (pData);
  }

  if (!iRetcode)                       /* all ok? then decompress, my man */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    iRetcode = mngjpeg_decompressdata2 (pData, iRawlen, pRawdata);
#else
    iRetcode = mngjpeg_decompressdata2 (pData, pData->iRawlen, pData->pRawdata);
#endif

  if (iRetcode)
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_JDAA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_jdat (mng_datap  pData,
                                      mng_uint32 iRawlen,
                                      mng_uint8p pRawdata)
#else
mng_retcode mng_process_display_jdat (mng_datap  pData)
#endif
{
  mng_retcode iRetcode = MNG_NOERROR;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_JDAT, MNG_LC_START);
#endif

  if (pData->bRestorebkgd)             /* need to restore the background ? */
  {
    pData->bRestorebkgd = MNG_FALSE;
    iRetcode            = load_bkgdlayer (pData);

    pData->iLayerseq++;                /* and it counts as a layer then ! */

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
  }

  if (!pData->bJPEGdecompress)         /* if we're not decompressing already */
  {
    if (pData->fInitrowproc)           /* initialize row-processing? */
    {
      iRetcode = ((mng_initrowproc)pData->fInitrowproc) (pData);
      pData->fInitrowproc = MNG_NULL;  /* only call this once !!! */
    }

    if (!iRetcode)                     /* initialize decompress */
      iRetcode = mngjpeg_decompressinit (pData);
  }

  if (!iRetcode)                       /* all ok? then decompress, my man */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    iRetcode = mngjpeg_decompressdata (pData, iRawlen, pRawdata);
#else
    iRetcode = mngjpeg_decompressdata (pData, pData->iRawlen, pData->pRawdata);
#endif

  if (iRetcode)
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_JDAT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_dhdr (mng_datap  pData,
                                      mng_uint16 iObjectid,
                                      mng_uint8  iImagetype,
                                      mng_uint8  iDeltatype,
                                      mng_uint32 iBlockwidth,
                                      mng_uint32 iBlockheight,
                                      mng_uint32 iBlockx,
                                      mng_uint32 iBlocky)
#else
mng_retcode mng_process_display_dhdr (mng_datap  pData)
#endif
{
  mng_imagep  pImage;
  mng_retcode iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_DHDR, MNG_LC_START);
#endif

  pData->fInitrowproc     = MNG_NULL;  /* do nothing by default */
  pData->fDisplayrow      = MNG_NULL;
  pData->fCorrectrow      = MNG_NULL;
  pData->fStorerow        = MNG_NULL;
  pData->fProcessrow      = MNG_NULL;
  pData->pStoreobj        = MNG_NULL;

  pData->fDeltagetrow     = MNG_NULL;
  pData->fDeltaaddrow     = MNG_NULL;
  pData->fDeltareplacerow = MNG_NULL;
  pData->fDeltaputrow     = MNG_NULL;

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  pImage = mng_find_imageobject (pData, iObjectid);
#else
  pImage = mng_find_imageobject (pData, pData->iDHDRobjectid);
#endif

  if (pImage)                          /* object exists ? */
  {
    if (pImage->pImgbuf->bConcrete)    /* is it concrete ? */
    {                                  /* previous magnification to be done ? */
#ifndef MNG_SKIPCHUNK_MAGN
      if ((pImage->iMAGN_MethodX) || (pImage->iMAGN_MethodY))
      {
        iRetcode = mng_magnify_imageobject (pData, pImage);
                       
        if (iRetcode)                  /* on error bail out */
          return iRetcode;
      }
#endif
                                       /* save delta fields */
      pData->pDeltaImage           = (mng_ptr)pImage;
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      pData->iDeltaImagetype       = iImagetype;
      pData->iDeltatype            = iDeltatype;
      pData->iDeltaBlockwidth      = iBlockwidth;
      pData->iDeltaBlockheight     = iBlockheight;
      pData->iDeltaBlockx          = iBlockx;
      pData->iDeltaBlocky          = iBlocky;
#else
      pData->iDeltaImagetype       = pData->iDHDRimagetype;
      pData->iDeltatype            = pData->iDHDRdeltatype;
      pData->iDeltaBlockwidth      = pData->iDHDRblockwidth;
      pData->iDeltaBlockheight     = pData->iDHDRblockheight;
      pData->iDeltaBlockx          = pData->iDHDRblockx;
      pData->iDeltaBlocky          = pData->iDHDRblocky;
#endif
                                       /* restore target-object fields */
      pData->iDatawidth            = pImage->pImgbuf->iWidth;
      pData->iDataheight           = pImage->pImgbuf->iHeight;
      pData->iBitdepth             = pImage->pImgbuf->iBitdepth;
      pData->iColortype            = pImage->pImgbuf->iColortype;
      pData->iCompression          = pImage->pImgbuf->iCompression;
      pData->iFilter               = pImage->pImgbuf->iFilter;
      pData->iInterlace            = pImage->pImgbuf->iInterlace;

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      if ((iDeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
          (iDeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
        pData->iBitdepth           = pImage->pImgbuf->iPixelsampledepth;
      else
      if ((iDeltatype == MNG_DELTATYPE_BLOCKALPHAADD    ) ||
          (iDeltatype == MNG_DELTATYPE_BLOCKALPHAREPLACE)    )
        pData->iBitdepth           = pImage->pImgbuf->iAlphasampledepth;
      else
      if ((iDeltatype == MNG_DELTATYPE_BLOCKCOLORADD    ) ||
          (iDeltatype == MNG_DELTATYPE_BLOCKCOLORREPLACE)    )
        pData->iBitdepth           = pImage->pImgbuf->iPixelsampledepth;
#else
      if ((pData->iDHDRdeltatype == MNG_DELTATYPE_BLOCKPIXELADD    ) ||
          (pData->iDHDRdeltatype == MNG_DELTATYPE_BLOCKPIXELREPLACE)    )
        pData->iBitdepth           = pImage->pImgbuf->iPixelsampledepth;
      else
      if ((pData->iDHDRdeltatype == MNG_DELTATYPE_BLOCKALPHAADD    ) ||
          (pData->iDHDRdeltatype == MNG_DELTATYPE_BLOCKALPHAREPLACE)    )
        pData->iBitdepth           = pImage->pImgbuf->iAlphasampledepth;
      else
      if ((pData->iDHDRdeltatype == MNG_DELTATYPE_BLOCKCOLORADD    ) ||
          (pData->iDHDRdeltatype == MNG_DELTATYPE_BLOCKCOLORREPLACE)    )
        pData->iBitdepth           = pImage->pImgbuf->iPixelsampledepth;
#endif

#ifdef MNG_INCLUDE_JNG
      pData->iJHDRimgbitdepth      = pImage->pImgbuf->iBitdepth;
      pData->iJHDRcolortype        = pImage->pImgbuf->iColortype;
      pData->iJHDRimgcompression   = pImage->pImgbuf->iJHDRcompression;
      pData->iJHDRimginterlace     = pImage->pImgbuf->iJHDRinterlace;
      pData->iJHDRalphacompression = pImage->pImgbuf->iCompression;
      pData->iJHDRalphafilter      = pImage->pImgbuf->iFilter;
      pData->iJHDRalphainterlace   = pImage->pImgbuf->iInterlace;
      pData->iJHDRalphabitdepth    = pImage->pImgbuf->iAlphabitdepth;
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                                       /* block size specified ? */
      if (iDeltatype != MNG_DELTATYPE_NOCHANGE)
      {                                /* block entirely within target ? */
        if (iDeltatype != MNG_DELTATYPE_REPLACE)
        {
          if (((iBlockx + iBlockwidth ) > pData->iDatawidth ) ||
              ((iBlocky + iBlockheight) > pData->iDataheight)    )
            MNG_ERROR (pData, MNG_INVALIDBLOCK);
        }

        pData->iDatawidth          = iBlockwidth;
        pData->iDataheight         = iBlockheight;
      }
#else
                                       /* block size specified ? */
      if (pData->iDHDRdeltatype != MNG_DELTATYPE_NOCHANGE)
      {                                /* block entirely within target ? */
        if (pData->iDHDRdeltatype != MNG_DELTATYPE_REPLACE)
        {
          if (((pData->iDHDRblockx + pData->iDHDRblockwidth ) > pData->iDatawidth ) ||
              ((pData->iDHDRblocky + pData->iDHDRblockheight) > pData->iDataheight)    )
            MNG_ERROR (pData, MNG_INVALIDBLOCK);
        }

        pData->iDatawidth          = pData->iDHDRblockwidth;
        pData->iDataheight         = pData->iDHDRblockheight;
      }
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      switch (iDeltatype)              /* determine nr of delta-channels */
#else
      switch (pData->iDHDRdeltatype)   /* determine nr of delta-channels */
#endif
      {
         case MNG_DELTATYPE_BLOCKALPHAADD : ;
         case MNG_DELTATYPE_BLOCKALPHAREPLACE :
              {
#ifdef MNG_INCLUDE_JNG
                if ((pData->iColortype     == MNG_COLORTYPE_GRAYA    ) ||
                    (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAYA)    )
                {
                  pData->iColortype     = MNG_COLORTYPE_GRAY;
                  pData->iJHDRcolortype = MNG_COLORTYPE_JPEGGRAY;
                }
                else
                if ((pData->iColortype     == MNG_COLORTYPE_RGBA      ) ||
                    (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLORA)    )
                {
                  pData->iColortype     = MNG_COLORTYPE_GRAY;
                  pData->iJHDRcolortype = MNG_COLORTYPE_JPEGGRAY;
                }
#else
                if (pData->iColortype      == MNG_COLORTYPE_GRAYA)
                  pData->iColortype     = MNG_COLORTYPE_GRAY;
                else
                if (pData->iColortype      == MNG_COLORTYPE_RGBA)
                  pData->iColortype     = MNG_COLORTYPE_GRAY;
#endif
                else                   /* target has no alpha; that sucks! */
                  MNG_ERROR (pData, MNG_TARGETNOALPHA);

                break;
              }

         case MNG_DELTATYPE_BLOCKCOLORADD : ;
         case MNG_DELTATYPE_BLOCKCOLORREPLACE :
              {
#ifdef MNG_INCLUDE_JNG
                if ((pData->iColortype     == MNG_COLORTYPE_GRAYA    ) ||
                    (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAYA)    )
                {
                  pData->iColortype     = MNG_COLORTYPE_GRAY;
                  pData->iJHDRcolortype = MNG_COLORTYPE_JPEGGRAY;
                }
                else
                if ((pData->iColortype     == MNG_COLORTYPE_RGBA      ) ||
                    (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLORA)    )
                {
                  pData->iColortype     = MNG_COLORTYPE_RGB;
                  pData->iJHDRcolortype = MNG_COLORTYPE_JPEGCOLOR;
                }
#else
                if (pData->iColortype == MNG_COLORTYPE_GRAYA)
                  pData->iColortype = MNG_COLORTYPE_GRAY;
                else
                if (pData->iColortype == MNG_COLORTYPE_RGBA)
                  pData->iColortype = MNG_COLORTYPE_RGB;
#endif                  
                else                   /* target has no alpha; that sucks! */
                  MNG_ERROR (pData, MNG_TARGETNOALPHA);

                break;
              }

      }
                                       /* full image replace ? */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      if (iDeltatype == MNG_DELTATYPE_REPLACE)
#else
      if (pData->iDHDRdeltatype == MNG_DELTATYPE_REPLACE)
#endif
      {
        iRetcode = mng_reset_object_details (pData, pImage,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iBitdepth, pData->iColortype,
                                             pData->iCompression, pData->iFilter,
                                             pData->iInterlace, MNG_FALSE);

        if (iRetcode)                  /* on error bail out */
          return iRetcode;

        pData->pStoreobj = pImage;     /* and store straight into this object */
      }
      else
      {
        mng_imagedatap pBufzero, pBuf;
                                       /* we store in object 0 and process it later */
        pData->pStoreobj = pData->pObjzero;
                                       /* make sure to initialize object 0 then */
        iRetcode = mng_reset_object_details (pData, (mng_imagep)pData->pObjzero,
                                             pData->iDatawidth, pData->iDataheight,
                                             pData->iBitdepth, pData->iColortype,
                                             pData->iCompression, pData->iFilter,
                                             pData->iInterlace, MNG_TRUE);

        if (iRetcode)                  /* on error bail out */
          return iRetcode;

        pBuf     = pImage->pImgbuf;    /* copy possible palette & cheap transparency */
        pBufzero = ((mng_imagep)pData->pObjzero)->pImgbuf;

        pBufzero->bHasPLTE = pBuf->bHasPLTE;
        pBufzero->bHasTRNS = pBuf->bHasTRNS;

        if (pBufzero->bHasPLTE)        /* copy palette ? */
        {
          mng_uint32 iX;

          pBufzero->iPLTEcount = pBuf->iPLTEcount;

          for (iX = 0; iX < pBuf->iPLTEcount; iX++)
          {
            pBufzero->aPLTEentries [iX].iRed   = pBuf->aPLTEentries [iX].iRed;
            pBufzero->aPLTEentries [iX].iGreen = pBuf->aPLTEentries [iX].iGreen;
            pBufzero->aPLTEentries [iX].iBlue  = pBuf->aPLTEentries [iX].iBlue;
          }
        }

        if (pBufzero->bHasTRNS)        /* copy cheap transparency ? */
        {
          pBufzero->iTRNSgray  = pBuf->iTRNSgray;
          pBufzero->iTRNSred   = pBuf->iTRNSred;
          pBufzero->iTRNSgreen = pBuf->iTRNSgreen;
          pBufzero->iTRNSblue  = pBuf->iTRNSblue;
          pBufzero->iTRNScount = pBuf->iTRNScount;

          MNG_COPY (pBufzero->aTRNSentries, pBuf->aTRNSentries,
                    sizeof (pBufzero->aTRNSentries));
        }
                                       /* process immediately if bitdepth & colortype are equal */
        pData->bDeltaimmediate =
          (mng_bool)((pData->bDisplaying) && (!pData->bSkipping) &&
                     ((pData->bRunning) || (pData->bSearching)) &&
                     (pData->iBitdepth  == ((mng_imagep)pData->pDeltaImage)->pImgbuf->iBitdepth ) &&
                     (pData->iColortype == ((mng_imagep)pData->pDeltaImage)->pImgbuf->iColortype)    );
      }
 
#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
  pData->fInitrowproc = (mng_fptr)mng_init_rowproc;
  pData->ePng_imgtype = mng_png_imgtype (pData->iColortype, pData->iBitdepth);
#else
      switch (pData->iColortype)       /* determine row initialization routine */
      {
        case 0 : {                     /* gray */
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
        case 2 : {                     /* rgb */
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
        case 3 : {                     /* indexed */
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
        case 4 : {                     /* gray+alpha */
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
        case 6 : {                     /* rgb+alpha */
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
    }
    else
      MNG_ERROR (pData, MNG_OBJNOTCONCRETE);

  }
  else
    MNG_ERROR (pData, MNG_OBJECTUNKNOWN);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_DHDR, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_prom (mng_datap  pData,
                                      mng_uint8  iBitdepth,
                                      mng_uint8  iColortype,
                                      mng_uint8  iFilltype)
#else
mng_retcode mng_process_display_prom (mng_datap  pData)
#endif
{
  mng_imagep     pImage;
  mng_imagedatap pBuf;
  mng_retcode    iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_PROM, MNG_LC_START);
#endif

  if (!pData->pDeltaImage)             /* gotta have this now! */
    MNG_ERROR (pData, MNG_INVALIDDELTA);

  pImage = (mng_imagep)pData->pDeltaImage;
  pBuf   = pImage->pImgbuf;
                                       /* can't demote bitdepth! */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  if (iBitdepth < pBuf->iBitdepth)
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

  if ( ((pBuf->iColortype == MNG_COLORTYPE_GRAY      ) &&
        (iColortype       != MNG_COLORTYPE_GRAY      ) &&
        (iColortype       != MNG_COLORTYPE_GRAYA     ) &&
        (iColortype       != MNG_COLORTYPE_RGB       ) &&
        (iColortype       != MNG_COLORTYPE_RGBA      )    ) ||
       ((pBuf->iColortype == MNG_COLORTYPE_GRAYA     ) &&
        (iColortype       != MNG_COLORTYPE_GRAYA     ) &&
        (iColortype       != MNG_COLORTYPE_RGBA      )    ) ||
       ((pBuf->iColortype == MNG_COLORTYPE_RGB       ) &&
        (iColortype       != MNG_COLORTYPE_RGB       ) &&
        (iColortype       != MNG_COLORTYPE_RGBA      )    ) ||
       ((pBuf->iColortype == MNG_COLORTYPE_RGBA      ) &&
        (iColortype       != MNG_COLORTYPE_RGBA      )    ) ||
#ifdef MNG_INCLUDE_JNG
       ((pBuf->iColortype == MNG_COLORTYPE_JPEGGRAY  ) &&
        (iColortype       != MNG_COLORTYPE_JPEGGRAY  ) &&
        (iColortype       != MNG_COLORTYPE_JPEGCOLOR ) &&
        (iColortype       != MNG_COLORTYPE_JPEGGRAYA ) &&
        (iColortype       != MNG_COLORTYPE_JPEGCOLORA)    ) ||
       ((pBuf->iColortype == MNG_COLORTYPE_JPEGCOLOR ) &&
        (iColortype       != MNG_COLORTYPE_JPEGCOLOR ) &&
        (iColortype       != MNG_COLORTYPE_JPEGCOLORA)    ) ||
       ((pBuf->iColortype == MNG_COLORTYPE_JPEGGRAYA ) &&
        (iColortype       != MNG_COLORTYPE_JPEGGRAYA ) &&
        (iColortype       != MNG_COLORTYPE_JPEGCOLORA)    ) ||
       ((pBuf->iColortype == MNG_COLORTYPE_JPEGCOLORA) &&
        (iColortype       != MNG_COLORTYPE_JPEGCOLORA)    ) ||
#endif
       ((pBuf->iColortype == MNG_COLORTYPE_INDEXED   ) &&
        (iColortype       != MNG_COLORTYPE_INDEXED   ) &&
        (iColortype       != MNG_COLORTYPE_RGB       ) &&
        (iColortype       != MNG_COLORTYPE_RGBA      )    )    )
    MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);

  iRetcode = mng_promote_imageobject (pData, pImage, iBitdepth, iColortype, iFilltype);
#else
  if (pData->iPROMbitdepth < pBuf->iBitdepth)
    MNG_ERROR (pData, MNG_INVALIDBITDEPTH);

  if ( ((pBuf->iColortype      == MNG_COLORTYPE_GRAY      ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_GRAY      ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_GRAYA     ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_RGB       ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_RGBA      )    ) ||
       ((pBuf->iColortype      == MNG_COLORTYPE_GRAYA     ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_GRAYA     ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_RGBA      )    ) ||
       ((pBuf->iColortype      == MNG_COLORTYPE_RGB       ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_RGB       ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_RGBA      )    ) ||
       ((pBuf->iColortype      == MNG_COLORTYPE_RGBA      ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_RGBA      )    ) ||
#ifdef MNG_INCLUDE_JNG
       ((pBuf->iColortype      == MNG_COLORTYPE_JPEGGRAY  ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGGRAY  ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGCOLOR ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGGRAYA ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGCOLORA)    ) ||
       ((pBuf->iColortype      == MNG_COLORTYPE_JPEGCOLOR ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGCOLOR ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGCOLORA)    ) ||
       ((pBuf->iColortype      == MNG_COLORTYPE_JPEGGRAYA ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGGRAYA ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGCOLORA)    ) ||
       ((pBuf->iColortype      == MNG_COLORTYPE_JPEGCOLORA) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_JPEGCOLORA)    ) ||
#endif
       ((pBuf->iColortype      == MNG_COLORTYPE_INDEXED   ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_INDEXED   ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_RGB       ) &&
        (pData->iPROMcolortype != MNG_COLORTYPE_RGBA      )    )    )
    MNG_ERROR (pData, MNG_INVALIDCOLORTYPE);

  iRetcode = mng_promote_imageobject (pData, pImage, pData->iPROMbitdepth,
                                      pData->iPROMcolortype, pData->iPROMfilltype);
#endif

  if (iRetcode)                        /* on error bail out */
    return iRetcode;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_PROM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
mng_retcode mng_process_display_ipng (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IPNG, MNG_LC_START);
#endif
                                       /* indicate it for what it is now */
  pData->iDeltaImagetype = MNG_IMAGETYPE_PNG;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IPNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifdef MNG_INCLUDE_JNG
mng_retcode mng_process_display_ijng (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IJNG, MNG_LC_START);
#endif
                                       /* indicate it for what it is now */
  pData->iDeltaImagetype = MNG_IMAGETYPE_JNG;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_IJNG, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_pplt (mng_datap      pData,
                                      mng_uint8      iType,
                                      mng_uint32     iCount,
                                      mng_palette8ep paIndexentries,
                                      mng_uint8p     paAlphaentries,
                                      mng_uint8p     paUsedentries)
#else
mng_retcode mng_process_display_pplt (mng_datap      pData)
#endif
{
  mng_uint32     iX;
  mng_imagep     pImage = (mng_imagep)pData->pObjzero;
  mng_imagedatap pBuf   = pImage->pImgbuf;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_PPLT, MNG_LC_START);
#endif

#ifdef MNG_DECREMENT_LOOPS
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  iX = iCount;
#else
  iX = pData->iPPLTcount;
#endif
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  switch (iType)
#else
  switch (pData->iPPLTtype)
#endif
  {
    case MNG_DELTATYPE_REPLACERGB :
      {
#ifdef MNG_DECREMENT_LOOPS
        for (; iX > 0;iX--)
#else
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
        for (iX = 0; iX < iCount; iX++)
#else
        for (iX = 0; iX < pData->iPPLTcount; iX++)
#endif
#endif
        {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
          if (paUsedentries [iX])
          {
            pBuf->aPLTEentries [iX].iRed   = paIndexentries [iX].iRed;
            pBuf->aPLTEentries [iX].iGreen = paIndexentries [iX].iGreen;
            pBuf->aPLTEentries [iX].iBlue  = paIndexentries [iX].iBlue;
          }
#else
          if (pData->paPPLTusedentries [iX])
          {
            pBuf->aPLTEentries [iX].iRed   = pData->paPPLTindexentries [iX].iRed;
            pBuf->aPLTEentries [iX].iGreen = pData->paPPLTindexentries [iX].iGreen;
            pBuf->aPLTEentries [iX].iBlue  = pData->paPPLTindexentries [iX].iBlue;
          }
#endif
        }

        break;
      }
    case MNG_DELTATYPE_DELTARGB :
      {
#ifdef MNG_DECREMENT_LOOPS
        for (; iX > 0;iX--)
#else
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
        for (iX = 0; iX < iCount; iX++)
#else
        for (iX = 0; iX < pData->iPPLTcount; iX++)
#endif
#endif
        {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
          if (paUsedentries [iX])
          {
            pBuf->aPLTEentries [iX].iRed   =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iRed   +
                                           paIndexentries [iX].iRed  );
            pBuf->aPLTEentries [iX].iGreen =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iGreen +
                                           paIndexentries [iX].iGreen);
            pBuf->aPLTEentries [iX].iBlue  =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iBlue  +
                                           paIndexentries [iX].iBlue );
          }
#else
          if (pData->paPPLTusedentries [iX])
          {
            pBuf->aPLTEentries [iX].iRed   =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iRed   +
                                           pData->paPPLTindexentries [iX].iRed  );
            pBuf->aPLTEentries [iX].iGreen =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iGreen +
                                           pData->paPPLTindexentries [iX].iGreen);
            pBuf->aPLTEentries [iX].iBlue  =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iBlue  +
                                           pData->paPPLTindexentries [iX].iBlue );
          }
#endif
        }

        break;
      }
    case MNG_DELTATYPE_REPLACEALPHA :
      {
#ifdef MNG_DECREMENT_LOOPS
        for (; iX > 0;iX--)
#else
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
        for (iX = 0; iX < iCount; iX++)
#else
        for (iX = 0; iX < pData->iPPLTcount; iX++)
#endif
#endif
        {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
          if (paUsedentries [iX])
            pBuf->aTRNSentries [iX] = paAlphaentries [iX];
        }
#else
          if (pData->paPPLTusedentries [iX])
            pBuf->aTRNSentries [iX] = pData->paPPLTalphaentries [iX];
        }
#endif

        break;
      }
    case MNG_DELTATYPE_DELTAALPHA :
      {
#ifdef MNG_DECREMENT_LOOPS
        for (; iX > 0;iX--)
#else
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
        for (iX = 0; iX < iCount; iX++)
#else
        for (iX = 0; iX < pData->iPPLTcount; iX++)
#endif
#endif
        {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
          if (paUsedentries [iX])
            pBuf->aTRNSentries [iX] =
                               (mng_uint8)(pBuf->aTRNSentries [iX] +
                                           paAlphaentries [iX]);
#else
          if (pData->paPPLTusedentries [iX])
            pBuf->aTRNSentries [iX] =
                               (mng_uint8)(pBuf->aTRNSentries [iX] +
                                           pData->paPPLTalphaentries [iX]);
#endif
        }

        break;
      }
    case MNG_DELTATYPE_REPLACERGBA :
      {
#ifdef MNG_DECREMENT_LOOPS
        for (; iX > 0;iX--)
#else
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
        for (iX = 0; iX < iCount; iX++)
#else
        for (iX = 0; iX < pData->iPPLTcount; iX++)
#endif
#endif
        {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
          if (paUsedentries [iX])
          {
            pBuf->aPLTEentries [iX].iRed   = paIndexentries [iX].iRed;
            pBuf->aPLTEentries [iX].iGreen = paIndexentries [iX].iGreen;
            pBuf->aPLTEentries [iX].iBlue  = paIndexentries [iX].iBlue;
            pBuf->aTRNSentries [iX]        = paAlphaentries [iX];
          }
#else
          if (pData->paPPLTusedentries [iX])
          {
            pBuf->aPLTEentries [iX].iRed   = pData->paPPLTindexentries [iX].iRed;
            pBuf->aPLTEentries [iX].iGreen = pData->paPPLTindexentries [iX].iGreen;
            pBuf->aPLTEentries [iX].iBlue  = pData->paPPLTindexentries [iX].iBlue;
            pBuf->aTRNSentries [iX]        = pData->paPPLTalphaentries [iX];
          }
#endif
        }

        break;
      }
    case MNG_DELTATYPE_DELTARGBA :
      {
#ifdef MNG_DECREMENT_LOOPS
        for (; iX > 0;iX--)
#else
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
        for (iX = 0; iX < iCount; iX++)
#else
        for (iX = 0; iX < pData->iPPLTcount; iX++)
#endif
#endif
        {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
          if (paUsedentries [iX])
          {
            pBuf->aPLTEentries [iX].iRed   =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iRed   +
                                           paIndexentries [iX].iRed  );
            pBuf->aPLTEentries [iX].iGreen =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iGreen +
                                           paIndexentries [iX].iGreen);
            pBuf->aPLTEentries [iX].iBlue  =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iBlue  +
                                           paIndexentries [iX].iBlue );
            pBuf->aTRNSentries [iX] =
                               (mng_uint8)(pBuf->aTRNSentries [iX] +
                                           paAlphaentries [iX]);
          }
#else
          if (pData->paPPLTusedentries [iX])
          {
            pBuf->aPLTEentries [iX].iRed   =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iRed   +
                                           pData->paPPLTindexentries [iX].iRed  );
            pBuf->aPLTEentries [iX].iGreen =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iGreen +
                                           pData->paPPLTindexentries [iX].iGreen);
            pBuf->aPLTEentries [iX].iBlue  =
                               (mng_uint8)(pBuf->aPLTEentries [iX].iBlue  +
                                           pData->paPPLTindexentries [iX].iBlue );
            pBuf->aTRNSentries [iX] =
                               (mng_uint8)(pBuf->aTRNSentries [iX] +
                                           pData->paPPLTalphaentries [iX]);
          }
#endif
        }

        break;
      }
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  if ((iType != MNG_DELTATYPE_REPLACERGB) && (iType != MNG_DELTATYPE_DELTARGB))
#else
  if ((pData->iPPLTtype != MNG_DELTATYPE_REPLACERGB) &&
      (pData->iPPLTtype != MNG_DELTATYPE_DELTARGB  )    )
#endif
  {
    if (pBuf->bHasTRNS)
    {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      if (iCount > pBuf->iTRNScount)
        pBuf->iTRNScount = iCount;
#else
      if (pData->iPPLTcount > pBuf->iTRNScount)
        pBuf->iTRNScount = pData->iPPLTcount;
#endif
    }
    else
    {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      pBuf->iTRNScount = iCount;
      pBuf->bHasTRNS   = MNG_TRUE;
#else
      pBuf->iTRNScount = pData->iPPLTcount;
      pBuf->bHasTRNS   = MNG_TRUE;
#endif
    }
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  if ((iType != MNG_DELTATYPE_REPLACEALPHA) && (iType != MNG_DELTATYPE_DELTAALPHA))
#else
  if ((pData->iPPLTtype != MNG_DELTATYPE_REPLACEALPHA) &&
      (pData->iPPLTtype != MNG_DELTATYPE_DELTAALPHA  )    )
#endif
  {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    if (iCount > pBuf->iPLTEcount)
      pBuf->iPLTEcount = iCount;
#else
    if (pData->iPPLTcount > pBuf->iPLTEcount)
      pBuf->iPLTEcount = pData->iPPLTcount;
#endif
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_PPLT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_MAGN
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_magn (mng_datap  pData,
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
mng_retcode mng_process_display_magn (mng_datap  pData)
#endif
{
  mng_uint16 iX;
  mng_imagep pImage;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MAGN, MNG_LC_START);
#endif
                                       /* iterate the object-ids */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  for (iX = iFirstid; iX <= iLastid; iX++)
#else
  for (iX = pData->iMAGNfirstid; iX <= pData->iMAGNlastid; iX++)
#endif
  {
    if (iX == 0)                       /* process object 0 ? */
    {
      pImage = (mng_imagep)pData->pObjzero;

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      pImage->iMAGN_MethodX = iMethodX;
      pImage->iMAGN_MethodY = iMethodY;
      pImage->iMAGN_MX      = iMX;
      pImage->iMAGN_MY      = iMY;
      pImage->iMAGN_ML      = iML;
      pImage->iMAGN_MR      = iMR;
      pImage->iMAGN_MT      = iMT;
      pImage->iMAGN_MB      = iMB;
#else
      pImage->iMAGN_MethodX = pData->iMAGNmethodX;
      pImage->iMAGN_MethodY = pData->iMAGNmethodY;
      pImage->iMAGN_MX      = pData->iMAGNmX;
      pImage->iMAGN_MY      = pData->iMAGNmY;
      pImage->iMAGN_ML      = pData->iMAGNmL;
      pImage->iMAGN_MR      = pData->iMAGNmR;
      pImage->iMAGN_MT      = pData->iMAGNmT;
      pImage->iMAGN_MB      = pData->iMAGNmB;
#endif
    }
    else
    {
      pImage = mng_find_imageobject (pData, iX);
                                       /* object exists & is not frozen ? */
      if ((pImage) && (!pImage->bFrozen))
      {                                /* previous magnification to be done ? */
        if ((pImage->iMAGN_MethodX) || (pImage->iMAGN_MethodY))
        {
          mng_retcode iRetcode = mng_magnify_imageobject (pData, pImage);
          if (iRetcode)                /* on error bail out */
            return iRetcode;
        }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
        pImage->iMAGN_MethodX = iMethodX;
        pImage->iMAGN_MethodY = iMethodY;
        pImage->iMAGN_MX      = iMX;
        pImage->iMAGN_MY      = iMY;
        pImage->iMAGN_ML      = iML;
        pImage->iMAGN_MR      = iMR;
        pImage->iMAGN_MT      = iMT;
        pImage->iMAGN_MB      = iMB;
#else
        pImage->iMAGN_MethodX = pData->iMAGNmethodX;
        pImage->iMAGN_MethodY = pData->iMAGNmethodY;
        pImage->iMAGN_MX      = pData->iMAGNmX;
        pImage->iMAGN_MY      = pData->iMAGNmY;
        pImage->iMAGN_ML      = pData->iMAGNmL;
        pImage->iMAGN_MR      = pData->iMAGNmR;
        pImage->iMAGN_MT      = pData->iMAGNmT;
        pImage->iMAGN_MB      = pData->iMAGNmB;
#endif
      }
    }
  }

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  pData->iMAGNfromid = iFirstid;
  pData->iMAGNtoid   = iLastid;
  iX                 = iFirstid;
#else
  pData->iMAGNfromid = pData->iMAGNfirstid;
  pData->iMAGNtoid   = pData->iMAGNlastid;
  iX                 = pData->iMAGNfirstid;
#endif
                                       /* iterate again for showing */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  while ((iX <= iLastid) && (!pData->bTimerset))
#else
  while ((iX <= pData->iMAGNlastid) && (!pData->bTimerset))
#endif
  {
    pData->iMAGNcurrentid = iX;

    if (iX)                            /* only real objects ! */
    {
      pImage = mng_find_imageobject (pData, iX);
                                       /* object exists & is not frozen  &
                                          is visible & is viewable ? */
      if ((pImage) && (!pImage->bFrozen) &&
          (pImage->bVisible) && (pImage->bViewable))
      {
        mng_retcode iRetcode = mng_display_image (pData, pImage, MNG_FALSE);
        if (iRetcode)
          return iRetcode;
      }
    }

    iX++;
  }

  if (pData->bTimerset)                /* broken ? */
    pData->iBreakpoint = 9;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MAGN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mng_process_display_magn2 (mng_datap pData)
{
  mng_uint16 iX;
  mng_imagep pImage;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MAGN, MNG_LC_START);
#endif

  iX = pData->iMAGNcurrentid;
                                       /* iterate again for showing */
  while ((iX <= pData->iMAGNtoid) && (!pData->bTimerset))
  {
    pData->iMAGNcurrentid = iX;

    if (iX)                            /* only real objects ! */
    {
      pImage = mng_find_imageobject (pData, iX);
                                       /* object exists & is not frozen  &
                                          is visible & is viewable ? */
      if ((pImage) && (!pImage->bFrozen) &&
          (pImage->bVisible) && (pImage->bViewable))
      {
        mng_retcode iRetcode = mng_display_image (pData, pImage, MNG_FALSE);
        if (iRetcode)
          return iRetcode;
      }
    }

    iX++;
  }

  if (pData->bTimerset)                /* broken ? */
    pData->iBreakpoint = 9;
  else
    pData->iBreakpoint = 0;            /* not again ! */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_MAGN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
mng_retcode mng_process_display_past (mng_datap  pData,
                                      mng_uint16 iTargetid,
                                      mng_uint8  iTargettype,
                                      mng_int32  iTargetx,
                                      mng_int32  iTargety,
                                      mng_uint32 iCount,
                                      mng_ptr    pSources)
#else
mng_retcode mng_process_display_past (mng_datap  pData)
#endif
{
  mng_retcode      iRetcode = MNG_NOERROR;
  mng_imagep       pTargetimg;
  mng_imagep       pSourceimg;
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  mng_past_sourcep pSource = (mng_past_sourcep)pSources;
#else
  mng_past_sourcep pSource = (mng_past_sourcep)pData->pPASTsources;
#endif
  mng_uint32       iX      = 0;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_PAST, MNG_LC_START);
#endif

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
  if (iTargetid)                       /* a real destination object ? */
#else
  if (pData->iPASTtargetid)            /* a real destination object ? */
#endif
  {                                    /* let's find it then */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    pTargetimg = (mng_imagep)mng_find_imageobject (pData, iTargetid);
#else
    pTargetimg = (mng_imagep)mng_find_imageobject (pData, pData->iPASTtargetid);
#endif

    if (!pTargetimg)                   /* if it doesn't exists; do a barf */
      MNG_ERROR (pData, MNG_OBJECTUNKNOWN);
                                       /* it's gotta be abstract !!! */
    if (pTargetimg->pImgbuf->bConcrete)
      MNG_ERROR (pData, MNG_OBJNOTABSTRACT);
                                       /* we want 32-/64-bit RGBA to play with ! */
    if ((pTargetimg->pImgbuf->iBitdepth <= MNG_BITDEPTH_8)          ||
        (pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_GRAY)    ||
        (pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_RGB)     ||
        (pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_INDEXED) ||
        (pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_GRAYA)      )
      iRetcode = mng_promote_imageobject (pData, pTargetimg, MNG_BITDEPTH_8,
                                          MNG_COLORTYPE_RGBA,
                                          MNG_FILLMETHOD_LEFTBITREPLICATE);
    else
    if ((pTargetimg->pImgbuf->iBitdepth > MNG_BITDEPTH_8)              &&
        ((pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_GRAY)  ||
         (pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_RGB)   ||
         (pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_GRAYA)    )   )
      iRetcode = mng_promote_imageobject (pData, pTargetimg, MNG_BITDEPTH_16,
                                          MNG_COLORTYPE_RGBA,
                                          MNG_FILLMETHOD_LEFTBITREPLICATE);
#ifdef MNG_INCLUDE_JNG
    else
    if ((pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_JPEGGRAY)  ||
        (pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_JPEGCOLOR) ||
        (pTargetimg->pImgbuf->iColortype ==  MNG_COLORTYPE_JPEGGRAYA)    )
      iRetcode = mng_promote_imageobject (pData, pTargetimg,
                                          pTargetimg->pImgbuf->iBitdepth,
                                          MNG_COLORTYPE_JPEGCOLORA,
                                          MNG_FILLMETHOD_LEFTBITREPLICATE);
#endif

    if (iRetcode)                      /* on error bail out */
      return iRetcode;
                                       /* make it really abstract ? */
    if (!pTargetimg->pImgbuf->bCorrected)
    {
      iRetcode = mng_colorcorrect_object (pData, pTargetimg);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
    }
  }
  else
  {                                    /* pasting into object 0 !!! */
    pTargetimg = (mng_imagep)pData->pObjzero;
                                       /* is it usable ??? */
    if ((pTargetimg->bClipped) &&
        (pTargetimg->iClipr > pTargetimg->iPosx) &&
        (pTargetimg->iClipb > pTargetimg->iPosy))
    {
                                       /* make it 32-bit RGBA please !!! */
      iRetcode = mng_reset_object_details (pData, pTargetimg,
                                           pTargetimg->iClipr - pTargetimg->iPosx,
                                           pTargetimg->iClipb - pTargetimg->iPosy,
                                           MNG_BITDEPTH_8, MNG_COLORTYPE_RGBA,
                                           0, 0, 0, MNG_FALSE);

      if (iRetcode)                    /* on error bail out */
        return iRetcode;
    }
    else
      pTargetimg = MNG_NULL;           /* clipped beyond visibility ! */
  }

  if (pTargetimg)                      /* usable destination ? */
  {
    mng_int32      iSourceY;
    mng_int32      iSourceYinc;
    mng_int32      iSourcerowsize;
    mng_int32      iSourcesamples;
    mng_bool       bSourceRGBA16;
    mng_int32      iTargetY;
    mng_int32      iTargetrowsize;
    mng_int32      iTargetsamples;
    mng_bool       bTargetRGBA16 = MNG_FALSE;
    mng_int32      iTemprowsize;
    mng_imagedatap pBuf;
#ifndef MNG_SKIPCHUNK_MAGN
                                       /* needs magnification ? */
    if ((pTargetimg->iMAGN_MethodX) || (pTargetimg->iMAGN_MethodY))
      iRetcode = mng_magnify_imageobject (pData, pTargetimg);
#endif

    if (!iRetcode)                     /* still ok ? */
    {
      bTargetRGBA16 = (mng_bool)(pTargetimg->pImgbuf->iBitdepth > 8);

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
      switch (iTargettype)             /* determine target x/y */
#else
      switch (pData->iPASTtargettype)  /* determine target x/y */
#endif
      {
        case 0 : {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   pData->iPastx = iTargetx;
                   pData->iPasty = iTargety;
#else
                   pData->iPastx = pData->iPASTtargetx;
                   pData->iPasty = pData->iPASTtargety;
#endif
                   break;
                 }

        case 1 : {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   pData->iPastx = pTargetimg->iPastx + iTargetx;
                   pData->iPasty = pTargetimg->iPasty + iTargety;
#else
                   pData->iPastx = pTargetimg->iPastx + pData->iPASTtargetx;
                   pData->iPasty = pTargetimg->iPasty + pData->iPASTtargety;
#endif
                   break;
                 }

        case 2 : {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
                   pData->iPastx += iTargetx;
                   pData->iPasty += iTargety;
#else
                   pData->iPastx += pData->iPASTtargetx;
                   pData->iPasty += pData->iPASTtargety;
#endif
                   break;
                 }
      }
                                       /* save for next time ... */
      pTargetimg->iPastx      = pData->iPastx;
      pTargetimg->iPasty      = pData->iPasty;
                                       /* address destination for row-routines */
      pData->pStoreobj        = (mng_objectp)pTargetimg;
      pData->pStorebuf        = (mng_objectp)pTargetimg->pImgbuf;
    }
                                       /* process the sources one by one */
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    while ((!iRetcode) && (iX < iCount))
#else
    while ((!iRetcode) && (iX < pData->iPASTcount))
#endif
    {                                  /* find the little bastards first */
      pSourceimg              = (mng_imagep)mng_find_imageobject (pData, pSource->iSourceid);
                                       /* exists and viewable? */
      if ((pSourceimg) && (pSourceimg->bViewable))
      {                                /* needs magnification ? */
#ifndef MNG_SKIPCHUNK_MAGN
        if ((pSourceimg->iMAGN_MethodX) || (pSourceimg->iMAGN_MethodY))
          iRetcode = mng_magnify_imageobject (pData, pSourceimg);
#endif

        if (!iRetcode)                 /* still ok ? */
        {
          pBuf                = (mng_imagedatap)pSourceimg->pImgbuf;
                                       /* address source for row-routines */
          pData->pRetrieveobj = (mng_objectp)pSourceimg;

          pData->iPass        = -1;    /* init row-processing variables */
          pData->iRowinc      = 1;
          pData->iColinc      = 1;
          pData->iPixelofs    = 0;
          iSourcesamples      = (mng_int32)pBuf->iWidth;
          iSourcerowsize      = pBuf->iRowsize;
          bSourceRGBA16       = (mng_bool)(pBuf->iBitdepth > 8);
                                       /* make sure the delta-routines do the right thing */
          pData->iDeltatype   = MNG_DELTATYPE_BLOCKPIXELREPLACE;

          switch (pBuf->iColortype)
          {
            case  0 : { 
#ifndef MNG_NO_16BIT_SUPPORT
                         if (bSourceRGBA16)
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_g16;
                        else
#endif
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_g8;

                        pData->bIsOpaque      = (mng_bool)(!pBuf->bHasTRNS);
                        break;
                      }

            case  2 : {
#ifndef MNG_NO_16BIT_SUPPORT
                        if (bSourceRGBA16)
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb16;
                        else
#endif
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb8;

                        pData->bIsOpaque      = (mng_bool)(!pBuf->bHasTRNS);
                        break;
                      }


            case  3 : { pData->fRetrieverow   = (mng_fptr)mng_retrieve_idx8;
                        pData->bIsOpaque      = (mng_bool)(!pBuf->bHasTRNS);
                        break;
                      }


            case  4 : {
#ifndef MNG_NO_16BIT_SUPPORT
                        if (bSourceRGBA16)
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_ga16;
                        else
#endif
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_ga8;

                        pData->bIsOpaque      = MNG_FALSE;
                        break;
                      }


            case  6 : {
#ifndef MNG_NO_16BIT_SUPPORT
                         if (bSourceRGBA16)
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba16;
                        else
#endif
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba8;

                        pData->bIsOpaque      = MNG_FALSE;
                        break;
                      }

            case  8 : {
#ifndef MNG_NO_16BIT_SUPPORT
                         if (bSourceRGBA16)
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_g16;
                        else
#endif
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_g8;

                        pData->bIsOpaque      = MNG_TRUE;
                        break;
                      }

            case 10 : {
#ifndef MNG_NO_16BIT_SUPPORT
                         if (bSourceRGBA16)
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb16;
                        else
#endif
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_rgb8;

                        pData->bIsOpaque      = MNG_TRUE;
                        break;
                      }


            case 12 : {
#ifndef MNG_NO_16BIT_SUPPORT
                         if (bSourceRGBA16)
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_ga16;
                        else
#endif
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_ga8;

                        pData->bIsOpaque      = MNG_FALSE;
                        break;
                      }


            case 14 : {
#ifndef MNG_NO_16BIT_SUPPORT
                         if (bSourceRGBA16)
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba16;
                        else
#endif
                          pData->fRetrieverow = (mng_fptr)mng_retrieve_rgba8;

                        pData->bIsOpaque      = MNG_FALSE;
                        break;
                      }
          }
                                       /* determine scaling */
#ifndef MNG_NO_16BIT_SUPPORT
#ifndef MNG_NO_DELTA_PNG
          if ((!bSourceRGBA16) && (bTargetRGBA16))
            pData->fScalerow = (mng_fptr)mng_scale_rgba8_rgba16;
          else
          if ((bSourceRGBA16) && (!bTargetRGBA16))
            pData->fScalerow = (mng_fptr)mng_scale_rgba16_rgba8;
          else
#endif
#endif
            pData->fScalerow = MNG_NULL;

                                       /* default no color-correction */
          pData->fCorrectrow = MNG_NULL;

#if defined(MNG_FULL_CMS)              /* determine color-management routine */
          iRetcode = mng_init_full_cms   (pData, MNG_FALSE, MNG_FALSE, MNG_TRUE);
#elif defined(MNG_GAMMA_ONLY)
          iRetcode = mng_init_gamma_only (pData, MNG_FALSE, MNG_FALSE, MNG_TRUE);
#elif defined(MNG_APP_CMS)
          iRetcode = mng_init_app_cms    (pData, MNG_FALSE, MNG_FALSE, MNG_TRUE);
#endif
        }

        if (!iRetcode)                 /* still ok ? */
        {  
          pData->fFliprow = MNG_NULL;  /* no flipping or tiling by default */
          pData->fTilerow = MNG_NULL;
                                       /* but perhaps we do have to ... */
          switch (pSource->iOrientation)
          {
            case 2 : ;
            case 4 : {
#ifndef MNG_NO_16BIT_SUPPORT
                       if (bTargetRGBA16)
                         pData->fFliprow = (mng_fptr)mng_flip_rgba16;
                       else
#endif
                         pData->fFliprow = (mng_fptr)mng_flip_rgba8;
                       break;
                     }

            case 8 : {
#ifndef MNG_NO_16BIT_SUPPORT
                       if (bTargetRGBA16)
                         pData->fTilerow = (mng_fptr)mng_tile_rgba16;
                       else
#endif
                         pData->fTilerow = (mng_fptr)mng_tile_rgba8;
                       break;
                     }
          }
                                       /* determine composition routine */
                                       /* note that we're abusing the delta-routine setup !!! */
          switch (pSource->iComposition)
          {
            case 0 : {                 /* composite over */
#ifndef MNG_NO_16BIT_SUPPORT
                       if (bTargetRGBA16)
                         pData->fDeltarow = (mng_fptr)mng_composeover_rgba16;
                       else
#endif
                         pData->fDeltarow = (mng_fptr)mng_composeover_rgba8;
                       break;
                     }

            case 1 : {                 /* replace */
#ifndef MNG_NO_16BIT_SUPPORT
                       if (bTargetRGBA16)
                         pData->fDeltarow = (mng_fptr)mng_delta_rgba16_rgba16;
                       else
#endif
                         pData->fDeltarow = (mng_fptr)mng_delta_rgba8_rgba8;
                       break;
                     }

            case 2 : {                 /* composite under */
#ifndef MNG_NO_16BIT_SUPPORT
                       if (bTargetRGBA16)
                         pData->fDeltarow = (mng_fptr)mng_composeunder_rgba16;
                       else
#endif
                         pData->fDeltarow = (mng_fptr)mng_composeunder_rgba8;
                       break;
                     }
          }
                                       /* determine offsets & clipping */
          if (pSource->iOffsettype == 1)
          {
            pData->iDestl          = pData->iPastx + pSource->iOffsetx;
            pData->iDestt          = pData->iPasty + pSource->iOffsety;
          }
          else
          {
            pData->iDestl          = pSource->iOffsetx;
            pData->iDestt          = pSource->iOffsety;
          }

          pData->iDestr            = (mng_int32)pTargetimg->pImgbuf->iWidth;
          pData->iDestb            = (mng_int32)pTargetimg->pImgbuf->iHeight;
                                       /* take the source dimension into account ? */
          if (pSource->iOrientation != 8)
          {
            pData->iDestr          = MIN_COORD (pData->iDestr, pData->iDestl + (mng_int32)pBuf->iWidth);
            pData->iDestb          = MIN_COORD (pData->iDestb, pData->iDestt + (mng_int32)pBuf->iHeight);
          }
                                       /* source clipping */
          if (pSource->iBoundarytype == 1)
          {
            if (pData->iDestl < pData->iPastx + pSource->iBoundaryl)
              pData->iSourcel      = pData->iPastx + pSource->iBoundaryl - pData->iDestl;
            else
              pData->iSourcel      = 0;

            if (pData->iDestt < pData->iPasty + pSource->iBoundaryt)
              pData->iSourcet      = pData->iPasty + pSource->iBoundaryt - pData->iDestt;
            else
              pData->iSourcet      = 0;

            pData->iDestl          = MAX_COORD (pData->iDestl, pData->iPastx + pSource->iBoundaryl);
            pData->iDestt          = MAX_COORD (pData->iDestt, pData->iPasty + pSource->iBoundaryt);
            pData->iDestr          = MIN_COORD (pData->iDestr, pData->iPastx + pSource->iBoundaryr);
            pData->iDestb          = MIN_COORD (pData->iDestb, pData->iPasty + pSource->iBoundaryb);
          }
          else
          {
            if (pData->iDestl < pSource->iBoundaryl)
              pData->iSourcel      = pSource->iBoundaryl - pData->iDestl;
            else
              pData->iSourcel      = 0;

            if (pData->iDestt < pSource->iBoundaryt)
              pData->iSourcet      = pSource->iBoundaryt - pData->iDestt;
            else
              pData->iSourcet      = 0;

            pData->iDestl          = MAX_COORD (pData->iDestl, pSource->iBoundaryl);
            pData->iDestt          = MAX_COORD (pData->iDestt, pSource->iBoundaryt);
            pData->iDestr          = MIN_COORD (pData->iDestr, pSource->iBoundaryr);
            pData->iDestb          = MIN_COORD (pData->iDestb, pSource->iBoundaryb);
          }

          if (pData->iSourcel)         /* indent source ? */
          {
#ifndef MNG_NO_16BIT_SUPPORT
             if (bTargetRGBA16)        /* abuse tiling routine to shift source-pixels */
               pData->fTilerow = (mng_fptr)mng_tile_rgba16;
             else
#endif
               pData->fTilerow = (mng_fptr)mng_tile_rgba8;
          }
                                       /* anything to display ? */
          if ((pData->iDestl <= pData->iDestr) && (pData->iDestt <= pData->iDestb))
          {                            /* init variables for the loop */
            if ((pSource->iOrientation == 2) || (pSource->iOrientation == 6))
            {
              iSourceY             = (mng_int32)pBuf->iHeight - 1 - pData->iSourcet;
              iSourceYinc          = -1;
            }
            else
            {
              iSourceY             = pData->iSourcet;
              iSourceYinc          = 1;
            }

            iTargetY               = pData->iDestt;
            pData->iCol            = pData->iDestl;

            iTargetsamples         = pData->iDestr - pData->iDestl;

#ifndef MNG_NO_16BIT_SUPPORT
            if (bTargetRGBA16)
              iTargetrowsize       = (iTargetsamples << 3);
            else
#endif
              iTargetrowsize       = (iTargetsamples << 2);

                                       /* get temporary work-buffers */
            if (iSourcerowsize > iTargetrowsize)
              iTemprowsize         = iSourcerowsize << 1;
            else
              iTemprowsize         = iTargetrowsize << 1;
            MNG_ALLOC (pData, pData->pRGBArow, iTemprowsize);
            MNG_ALLOC (pData, pData->pWorkrow, iTemprowsize);

            while ((!iRetcode) && (iTargetY < pData->iDestb))
            {                          /* get a row */
              pData->iRow          = iSourceY;
              pData->iRowsamples   = iSourcesamples;
              pData->iRowsize      = iSourcerowsize;
              pData->bIsRGBA16     = bSourceRGBA16;
              iRetcode             = ((mng_retrieverow)pData->fRetrieverow) (pData);
                                       /* scale it (if necessary) */
              if ((!iRetcode) && (pData->fScalerow))
                iRetcode           = ((mng_scalerow)pData->fScalerow) (pData);

              pData->bIsRGBA16     = bTargetRGBA16;
                                       /* color correction (if necessary) */
              if ((!iRetcode) && (pData->fCorrectrow))
                iRetcode           = ((mng_correctrow)pData->fCorrectrow) (pData);
                                       /* flipping (if necessary) */
              if ((!iRetcode) && (pData->fFliprow))
                iRetcode           = ((mng_fliprow)pData->fFliprow) (pData);
                                       /* tiling (if necessary) */
              if ((!iRetcode) && (pData->fTilerow))
                iRetcode           = ((mng_tilerow)pData->fTilerow) (pData);

              if (!iRetcode)           /* and paste..... */
              {
                pData->iRow        = iTargetY;
                pData->iRowsamples = iTargetsamples;
                pData->iRowsize    = iTargetrowsize;
                iRetcode           = ((mng_deltarow)pData->fDeltarow) (pData);
              }

              iSourceY += iSourceYinc; /* and next line */

              if (iSourceY < 0)
                iSourceY = (mng_int32)pBuf->iHeight - 1;
              else
              if (iSourceY >= (mng_int32)pBuf->iHeight)
                iSourceY = 0;

              iTargetY++;
            }
                                       /* drop the temporary row-buffer */
            MNG_FREEX (pData, pData->pWorkrow, iTemprowsize);
            MNG_FREEX (pData, pData->pRGBArow, iTemprowsize);
          }

#if defined(MNG_FULL_CMS)              /* cleanup cms stuff */
          if (!iRetcode)
            iRetcode = mng_clear_cms (pData);
#endif
        }

        pSource++;                     /* neeeeext */
        iX++;
      }
    }

    if (iRetcode)                      /* on error bail out */
      return iRetcode;

#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    if (!iTargetid)                    /* did we paste into object 0 ? */
#else
    if (!pData->iPASTtargetid)         /* did we paste into object 0 ? */
#endif
    {                                  /* display it then ! */
      iRetcode = mng_display_image (pData, pTargetimg, MNG_FALSE);
      if (iRetcode)                    /* on error bail out */
        return iRetcode;
    }
    else
    {                                  /* target is visible & viewable ? */
      if ((pTargetimg->bVisible) && (pTargetimg->bViewable))
      {
        iRetcode = mng_display_image (pData, pTargetimg, MNG_FALSE);
        if (iRetcode)
          return iRetcode;
      }
    }  
  }

  if (pData->bTimerset)                /* broken ? */
  {
#ifndef MNG_OPTIMIZE_DISPLAYCALLS
    pData->iPASTid     = iTargetid;
#else
    pData->iPASTid     = pData->iPASTtargetid;
#endif
    pData->iBreakpoint = 11;
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_PAST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SKIPCHUNK_PAST */

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
mng_retcode mng_process_display_past2 (mng_datap pData)
{
  mng_retcode iRetcode;
  mng_imagep  pTargetimg;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_PAST, MNG_LC_START);
#endif

  if (pData->iPASTid)                  /* a real destination object ? */
    pTargetimg = (mng_imagep)mng_find_imageobject (pData, pData->iPASTid);
  else                                 /* otherwise object 0 */
    pTargetimg = (mng_imagep)pData->pObjzero;

  iRetcode = mng_display_image (pData, pTargetimg, MNG_FALSE);
  if (iRetcode)
    return iRetcode;

  pData->iBreakpoint = 0;              /* only once */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_PROCESS_DISPLAY_PAST, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SKIPCHUNK_PAST */

/* ************************************************************************** */

#endif /* MNG_INCLUDE_DISPLAY_PROCS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */


