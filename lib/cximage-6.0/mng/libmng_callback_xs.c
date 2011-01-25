/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_callback_xs.c      copyright (c) 2000-2004 G.Juyn   * */
/* * version   : 1.0.9                                                      * */
/* *                                                                        * */
/* * purpose   : callback get/set interface (implementation)                * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the callback get/set functions           * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - fixed calling convention                                 * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *                                                                        * */
/* *             0.5.2 - 05/31/2000 - G.Juyn                                * */
/* *             - fixed up punctuation (contribution by Tim Rowley)        * */
/* *             0.5.2 - 06/02/2000 - G.Juyn                                * */
/* *             - added getalphaline callback for RGB8_A8 canvasstyle      * */
/* *                                                                        * */
/* *             0.9.1 - 07/15/2000 - G.Juyn                                * */
/* *             - added callbacks for SAVE/SEEK processing                 * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 10/11/2000 - G.Juyn                                * */
/* *             - added support for nEED                                   * */
/* *             0.9.3 - 10/17/2000 - G.Juyn                                * */
/* *             - added callback to process non-critical unknown chunks    * */
/* *                                                                        * */
/* *             1.0.1 - 02/08/2001 - G.Juyn                                * */
/* *             - added MEND processing callback                           * */
/* *                                                                        * */
/* *             1.0.2 - 06/23/2001 - G.Juyn                                * */
/* *             - added processterm callback                               * */
/* *                                                                        * */
/* *             1.0.6 - 07/07/2003 - G. R-P                                * */
/* *             - added SKIPCHUNK feature                                  * */
/* *                                                                        * */
/* *             1.0.7 - 03/10/2004 - G.R-P                                 * */
/* *             - added conditionals around openstream/closestream         * */
/* *             1.0.7 - 03/19/2004 - G.R-P                                 * */
/* *             - fixed typo (MNG_SKIPCHUNK_SAVE -> MNG_SKIPCHUNK_nEED     * */
/* *                                                                        * */
/* *             1.0.8 - 04/10/2004 - G.Juyn                                * */
/* *             - added data-push mechanisms for specialized decoders      * */
/* *                                                                        * */
/* *             1.0.9 - 09/18/2004 - G.R-P.                                * */
/* *             - added two SKIPCHUNK_TERM conditionals                    * */
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

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */
/* *                                                                        * */
/* *  Callback set functions                                                * */
/* *                                                                        * */
/* ************************************************************************** */

#ifndef MNG_INTERNAL_MEMMNGMT
mng_retcode MNG_DECL mng_setcb_memalloc (mng_handle   hHandle,
                                         mng_memalloc fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_MEMALLOC, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fMemalloc = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_MEMALLOC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INTERNAL_MEMMNGMT */

/* ************************************************************************** */

#ifndef MNG_INTERNAL_MEMMNGMT
mng_retcode MNG_DECL mng_setcb_memfree (mng_handle  hHandle,
                                        mng_memfree fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_MEMFREE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fMemfree = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_MEMFREE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INTERNAL_MEMMNGMT */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_retcode MNG_DECL mng_setcb_releasedata (mng_handle      hHandle,
                                            mng_releasedata fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_RELEASEDATA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fReleasedata = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_RELEASEDATA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
#ifndef MNG_NO_OPEN_CLOSE_STREAM
mng_retcode MNG_DECL mng_setcb_openstream (mng_handle     hHandle,
                                           mng_openstream fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_OPENSTREAM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fOpenstream = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_OPENSTREAM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
#ifndef MNG_NO_OPEN_CLOSE_STREAM
mng_retcode MNG_DECL mng_setcb_closestream (mng_handle      hHandle,
                                            mng_closestream fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_CLOSESTREAM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fClosestream = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_CLOSESTREAM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_retcode MNG_DECL mng_setcb_readdata (mng_handle   hHandle,
                                         mng_readdata fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_READDATA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fReaddata = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_READDATA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_WRITE
mng_retcode MNG_DECL mng_setcb_writedata (mng_handle    hHandle,
                                          mng_writedata fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_WRITEDATA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fWritedata = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_WRITEDATA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_WRITE */

/* ************************************************************************** */

mng_retcode MNG_DECL mng_setcb_errorproc (mng_handle    hHandle,
                                          mng_errorproc fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_ERRORPROC, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fErrorproc = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_ERRORPROC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#ifdef MNG_SUPPORT_TRACE
mng_retcode MNG_DECL mng_setcb_traceproc (mng_handle    hHandle,
                                          mng_traceproc fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_TRACEPROC, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fTraceproc = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_TRACEPROC, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_TRACE */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_retcode MNG_DECL mng_setcb_processheader (mng_handle        hHandle,
                                              mng_processheader fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSHEADER, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessheader = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSHEADER, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_tEXt
mng_retcode MNG_DECL mng_setcb_processtext (mng_handle      hHandle,
                                            mng_processtext fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSTEXT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcesstext = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSTEXT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_SAVE
mng_retcode MNG_DECL mng_setcb_processsave (mng_handle      hHandle,
                                            mng_processsave fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSSAVE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcesssave = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSSAVE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_SEEK
mng_retcode MNG_DECL mng_setcb_processseek (mng_handle      hHandle,
                                            mng_processseek fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSSEEK, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessseek = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSSEEK, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_nEED
mng_retcode MNG_DECL mng_setcb_processneed (mng_handle      hHandle,
                                            mng_processneed fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSNEED, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessneed = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSNEED, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_retcode MNG_DECL mng_setcb_processmend (mng_handle      hHandle,
                                            mng_processmend fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSMEND, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessmend = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSMEND, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_retcode MNG_DECL mng_setcb_processunknown (mng_handle         hHandle,
                                               mng_processunknown fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSUNKNOWN, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessunknown = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSUNKNOWN, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_TERM
mng_retcode MNG_DECL mng_setcb_processterm (mng_handle      hHandle,
                                            mng_processterm fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSTERM, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessterm = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSTERM, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_retcode MNG_DECL mng_setcb_getcanvasline (mng_handle        hHandle,
                                              mng_getcanvasline fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_GETCANVASLINE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fGetcanvasline = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_GETCANVASLINE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_retcode MNG_DECL mng_setcb_getbkgdline (mng_handle      hHandle,
                                            mng_getbkgdline fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_GETBKGDLINE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fGetbkgdline = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_GETBKGDLINE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_retcode MNG_DECL mng_setcb_getalphaline (mng_handle       hHandle,
                                             mng_getalphaline fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_GETALPHALINE, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fGetalphaline = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_GETALPHALINE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_retcode MNG_DECL mng_setcb_refresh (mng_handle  hHandle,
                                        mng_refresh fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_REFRESH, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fRefresh = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_REFRESH, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_retcode MNG_DECL mng_setcb_gettickcount (mng_handle       hHandle,
                                             mng_gettickcount fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_GETTICKCOUNT, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fGettickcount = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_GETTICKCOUNT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_retcode MNG_DECL mng_setcb_settimer (mng_handle   hHandle,
                                         mng_settimer fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_SETTIMER, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fSettimer = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_SETTIMER, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
mng_retcode MNG_DECL mng_setcb_processgamma (mng_handle        hHandle,
                                             mng_processgamma  fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSGAMA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessgamma = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSGAMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
#ifndef MNG_SKIPCHUNK_cHRM
mng_retcode MNG_DECL mng_setcb_processchroma (mng_handle        hHandle,
                                              mng_processchroma fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSCHROMA, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcesschroma = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSCHROMA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
mng_retcode MNG_DECL mng_setcb_processsrgb (mng_handle      hHandle,
                                            mng_processsrgb fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSSRGB, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcesssrgb = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSSRGB, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
#ifndef MNG_SKIPCHUNK_iCCP
mng_retcode MNG_DECL mng_setcb_processiccp (mng_handle      hHandle,
                                            mng_processiccp fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSICCP, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessiccp = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSICCP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
mng_retcode MNG_DECL mng_setcb_processarow (mng_handle      hHandle,
                                            mng_processarow fProc)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSAROW, MNG_LC_START);
#endif

  MNG_VALIDHANDLE (hHandle)
  ((mng_datap)hHandle)->fProcessarow = fProc;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (((mng_datap)hHandle), MNG_FN_SETCB_PROCESSAROW, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */
/* *                                                                        * */
/* *  Callback get functions                                                * */
/* *                                                                        * */
/* ************************************************************************** */

#ifndef MNG_INTERNAL_MEMMNGMT
mng_memalloc MNG_DECL mng_getcb_memalloc (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_MEMALLOC, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_MEMALLOC, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fMemalloc;
}
#endif /* MNG_INTERNAL_MEMMNGMT */

/* ************************************************************************** */

#ifndef MNG_INTERNAL_MEMMNGMT
mng_memfree MNG_DECL mng_getcb_memfree (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_MEMFREE, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_MEMFREE, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fMemfree;
}
#endif /* MNG_INTERNAL_MEMMNGMT */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_releasedata MNG_DECL mng_getcb_releasedata (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_RELEASEDATA, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_RELEASEDATA, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fReleasedata;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_readdata MNG_DECL mng_getcb_readdata (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_READDATA, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_READDATA, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fReaddata;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
#ifndef MNG_NO_OPEN_CLOSE_STREAM
mng_openstream MNG_DECL mng_getcb_openstream (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_OPENSTREAM, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_OPENSTREAM, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fOpenstream;
}
#endif
#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
#ifndef MNG_NO_OPEN_CLOSE_STREAM
mng_closestream MNG_DECL mng_getcb_closestream (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_CLOSESTREAM, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_CLOSESTREAM, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fClosestream;
}
#endif
#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_WRITE
mng_writedata MNG_DECL mng_getcb_writedata (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_WRITEDATA, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_WRITEDATA, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fWritedata;
}
#endif /* MNG_SUPPORT_WRITE */

/* ************************************************************************** */

mng_errorproc MNG_DECL mng_getcb_errorproc (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_ERRORPROC, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_ERRORPROC, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fErrorproc;
}

/* ************************************************************************** */

#ifdef MNG_SUPPORT_TRACE
mng_traceproc MNG_DECL mng_getcb_traceproc (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_TRACEPROC, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_TRACEPROC, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fTraceproc;
}
#endif /* MNG_SUPPORT_TRACE */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_processheader MNG_DECL mng_getcb_processheader (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSHEADER, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSHEADER, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessheader;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_tEXt
mng_processtext MNG_DECL mng_getcb_processtext (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSTEXT, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSTEXT, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcesstext;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_SAVE
mng_processsave MNG_DECL mng_getcb_processsave (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSSAVE, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSSAVE, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcesssave;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_SEEK
mng_processseek MNG_DECL mng_getcb_processseek (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSSEEK, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSSEEK, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessseek;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_nEED
mng_processneed MNG_DECL mng_getcb_processneed (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSNEED, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSNEED, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessneed;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_processmend MNG_DECL mng_getcb_processmend (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSMEND, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSMEND, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessmend;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
mng_processunknown MNG_DECL mng_getcb_processunknown (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSUNKNOWN, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSUNKNOWN, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessunknown;
}
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_READ
#ifndef MNG_SKIPCHUNK_TERM
mng_processterm MNG_DECL mng_getcb_processterm (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSTERM, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSTERM, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessterm;
}
#endif
#endif /* MNG_SUPPORT_READ */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_getcanvasline MNG_DECL mng_getcb_getcanvasline (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_GETCANVASLINE, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_GETCANVASLINE, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fGetcanvasline;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_getbkgdline MNG_DECL mng_getcb_getbkgdline (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_GETBKGDLINE, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_GETBKGDLINE, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fGetbkgdline;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_getalphaline MNG_DECL mng_getcb_getalphaline (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_GETALPHALINE, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_GETALPHALINE, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fGetalphaline;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_refresh MNG_DECL mng_getcb_refresh (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_REFRESH, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_REFRESH, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fRefresh;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_gettickcount MNG_DECL mng_getcb_gettickcount (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_GETTICKCOUNT, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_GETTICKCOUNT, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fGettickcount;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#ifdef MNG_SUPPORT_DISPLAY
mng_settimer MNG_DECL mng_getcb_settimer (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_SETTIMER, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_SETTIMER, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fSettimer;
}
#endif /* MNG_SUPPORT_DISPLAY */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
mng_processgamma MNG_DECL mng_getcb_processgamma (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSGAMMA, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSGAMMA, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessgamma;
}
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
#ifndef MNG_SKIPCHUNK_cHRM
mng_processchroma MNG_DECL mng_getcb_processchroma (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSCHROMA, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSCHROMA, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcesschroma;
}
#endif
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
mng_processsrgb MNG_DECL mng_getcb_processsrgb (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSSRGB, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSSRGB, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcesssrgb;
}
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
#ifndef MNG_SKIPCHUNK_iCCP
mng_processiccp MNG_DECL mng_getcb_processiccp (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSICCP, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSICCP, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessiccp;
}
#endif
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_SUPPORT_DISPLAY) && defined(MNG_APP_CMS)
mng_processarow MNG_DECL mng_getcb_processarow (mng_handle hHandle)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSAROW, MNG_LC_START);
#endif

  MNG_VALIDHANDLEX (hHandle)

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEX (((mng_datap)hHandle), MNG_FN_GETCB_PROCESSAROW, MNG_LC_END);
#endif

  return ((mng_datap)hHandle)->fProcessarow;
}
#endif /* MNG_SUPPORT_DISPLAY && MNG_APP_CMS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */



