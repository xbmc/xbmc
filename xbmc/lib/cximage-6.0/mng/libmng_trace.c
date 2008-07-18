/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_trace.c            copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Trace functions (implementation)                           * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the trace functions                      * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - added callback error-reporting support                   * */
/* *                                                                        * */
/* *             0.5.2 - 05/23/2000 - G.Juyn                                * */
/* *             - added trace telltale reporting                           * */
/* *             0.5.2 - 05/24/2000 - G.Juyn                                * */
/* *             - added tracestrings for global animation color-chunks     * */
/* *             - added tracestrings for get/set of default ZLIB/IJG parms * */
/* *             - added tracestrings for global PLTE,tRNS,bKGD             * */
/* *             0.5.2 - 05/30/2000 - G.Juyn                                * */
/* *             - added tracestrings for image-object promotion            * */
/* *             - added tracestrings for delta-image processing            * */
/* *             0.5.2 - 06/02/2000 - G.Juyn                                * */
/* *             - added tracestrings for getalphaline callback             * */
/* *             0.5.2 - 06/05/2000 - G.Juyn                                * */
/* *             - added tracestring for RGB8_A8 canvasstyle                * */
/* *             0.5.2 - 06/06/2000 - G.Juyn                                * */
/* *             - added tracestring for mng_read_resume HLAPI function     * */
/* *                                                                        * */
/* *             0.5.3 - 06/21/2000 - G.Juyn                                * */
/* *             - added tracestrings for get/set speedtype                 * */
/* *             - added tracestring for get imagelevel                     * */
/* *             0.5.3 - 06/22/2000 - G.Juyn                                * */
/* *             - added tracestring for delta-image processing             * */
/* *             - added tracestrings for PPLT chunk processing             * */
/* *                                                                        * */
/* *             0.9.1 - 07/07/2000 - G.Juyn                                * */
/* *             - added tracecodes for special display processing          * */
/* *             0.9.1 - 07/08/2000 - G.Juyn                                * */
/* *             - added tracestring for get/set suspensionmode             * */
/* *             - added tracestrings for get/set display variables         * */
/* *             - added tracecode for read_databuffer (I/O-suspension)     * */
/* *             0.9.1 - 07/15/2000 - G.Juyn                                * */
/* *             - added tracestrings for SAVE/SEEK callbacks               * */
/* *             - added tracestrings for get/set sectionbreaks             * */
/* *             - added tracestring for special error routine              * */
/* *             0.9.1 - 07/19/2000 - G.Juyn                                * */
/* *             - added tracestring for updatemngheader                    * */
/* *                                                                        * */
/* *             0.9.2 - 07/31/2000 - G.Juyn                                * */
/* *             - added tracestrings for status_xxxxx functions            * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *             - added tracestring for updatemngsimplicity                * */
/* *                                                                        * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 09/07/2000 - G.Juyn                                * */
/* *             - added support for new filter_types                       * */
/* *             0.9.3 - 10/10/2000 - G.Juyn                                * */
/* *             - added support for alpha-depth prediction                 * */
/* *             0.9.3 - 10/11/2000 - G.Juyn                                * */
/* *             - added JDAA chunk                                         * */
/* *             - added support for nEED                                   * */
/* *             0.9.3 - 10/16/2000 - G.Juyn                                * */
/* *             - added functions to retrieve PNG/JNG specific header-info * */
/* *             - added optional support for bKGD for PNG images           * */
/* *             0.9.3 - 10/17/2000 - G.Juyn                                * */
/* *             - added callback to process non-critical unknown chunks    * */
/* *             - added routine to discard "invalid" objects               * */
/* *             0.9.3 - 10/19/2000 - G.Juyn                                * */
/* *             - implemented delayed delta-processing                     * */
/* *             0.9.3 - 10/20/2000 - G.Juyn                                * */
/* *             - added get/set for bKGD preference setting                * */
/* *             0.9.3 - 10/21/2000 - G.Juyn                                * */
/* *             - added get function for interlace/progressive display     * */
/* *                                                                        * */
/* *             0.9.4 -  1/18/2001 - G.Juyn                                * */
/* *             - added "new" MAGN methods 3, 4 & 5                        * */
/* *                                                                        * */
/* *             1.0.1 - 02/08/2001 - G.Juyn                                * */
/* *             - added MEND processing callback                           * */
/* *             1.0.1 - 04/21/2001 - G.Juyn (code by G.Kelly)              * */
/* *             - added BGRA8 canvas with premultiplied alpha              * */
/* *             1.0.1 - 05/02/2001 - G.Juyn                                * */
/* *             - added "default" sRGB generation (Thanks Marti!)          * */
/* *                                                                        * */
/* *             1.0.2 - 06/23/2001 - G.Juyn                                * */
/* *             - added optimization option for MNG-video playback         * */
/* *             - added processterm callback                               * */
/* *             1.0.2 - 06/25/2001 - G.Juyn                                * */
/* *             - added option to turn off progressive refresh             * */
/* *                                                                        * */
/* *             1.0.3 - 08/06/2001 - G.Juyn                                * */
/* *             - added get function for last processed BACK chunk         * */
/* *                                                                        * */
/* *             1.0.5 - 08/15/2002 - G.Juyn                                * */
/* *             - completed PROM support                                   * */
/* *             - completed delta-image support                            * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             - added HLAPI function to copy chunks                      * */
/* *             1.0.5 - 09/14/2002 - G.Juyn                                * */
/* *             - added event handling for dynamic MNG                     * */
/* *             1.0.5 - 09/20/2002 - G.Juyn                                * */
/* *             - added support for PAST                                   * */
/* *             1.0.5 - 09/22/2002 - G.Juyn                                * */
/* *             - added bgrx8 canvas (filler byte)                         * */
/* *             1.0.5 - 09/23/2002 - G.Juyn                                * */
/* *             - added in-memory color-correction of abstract images      * */
/* *             - added compose over/under routines for PAST processing    * */
/* *             - added flip & tile routines for PAST processing           * */
/* *             1.0.5 - 10/09/2002 - G.Juyn                                * */
/* *             - fixed trace-constants for PAST chunk                     * */
/* *             1.0.5 - 11/07/2002 - G.Juyn                                * */
/* *             - added support to get totals after mng_read()             * */
/* *                                                                        * */
/* *             1.0.6 - 07/07/2003 - G.R-P                                 * */
/* *             - added conditionals around JNG and Delta-PNG code         * */
/* *             1.0.6 - 07/14/2003 - G.R-P                                 * */
/* *             - added conditionals around various unused functions       * */
/* *             1.0.6 - 07/29/2003 - G.R-P                                 * */
/* *             - added conditionals around PAST chunk support             * */
/* *                                                                        * */
/* *             1.0.7 - 11/27/2003 - R.A                                   * */
/* *             - added CANVAS_RGB565 and CANVAS_BGR565                    * */
/* *             1.0.7 - 01/25/2004 - J.S                                   * */
/* *             - added premultiplied alpha canvas' for RGBA, ARGB, ABGR   * */
/* *             1.0.7 - 03/07/2004 - G. Randers-Pehrson                    * */
/* *             - put gamma, cms-related declarations inside #ifdef        * */
/* *             1.0.7 - 03/10/2004 - G.R-P                                 * */
/* *             - added conditionals around openstream/closestream         * */
/* *                                                                        * */
/* *             1.0.8 - 04/02/2004 - G.Juyn                                * */
/* *             - added CRC existence & checking flags                     * */
/* *             1.0.8 - 04/11/2004 - G.Juyn                                * */
/* *             - added data-push mechanisms for specialized decoders      * */
/* *                                                                        * */
/* *             1.0.9 - 10/03/2004 - G.Juyn                                * */
/* *             - added function to retrieve current FRAM delay            * */
/* *             1.0.9 - 10/14/2004 - G.Juyn                                * */
/* *             - added bgr565_a8 canvas-style (thanks to J. Elvander)     * */
/* *                                                                        * */
/* *             1.0.10 - 04/08/2007 - G.Juyn                               * */
/* *             - added support for mPNG proposal                          * */
/* *             1.0.10 - 07/06/2007 - G.R-P bugfix by Lucas Quintana       * */
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

#ifdef MNG_INCLUDE_TRACE_PROCS

/* ************************************************************************** */

#ifdef MNG_INCLUDE_TRACE_STRINGS
MNG_LOCAL mng_trace_entry const trace_table [] =
  {
    {MNG_FN_INITIALIZE,                "initialize"},
    {MNG_FN_RESET,                     "reset"},
    {MNG_FN_CLEANUP,                   "cleanup"},
    {MNG_FN_READ,                      "read"},
    {MNG_FN_WRITE,                     "write"},
    {MNG_FN_CREATE,                    "create"},
    {MNG_FN_READDISPLAY,               "readdisplay"},
    {MNG_FN_DISPLAY,                   "display"},
    {MNG_FN_DISPLAY_RESUME,            "display_resume"},
    {MNG_FN_DISPLAY_FREEZE,            "display_freeze"},
    {MNG_FN_DISPLAY_RESET,             "display_reset"},
#ifndef MNG_NO_DISPLAY_GO_SUPPORTED
    {MNG_FN_DISPLAY_GOFRAME,           "display_goframe"},
    {MNG_FN_DISPLAY_GOLAYER,           "display_golayer"},
    {MNG_FN_DISPLAY_GOTIME,            "display_gotime"},
#endif
    {MNG_FN_GETLASTERROR,              "getlasterror"},
    {MNG_FN_READ_RESUME,               "read_resume"},
    {MNG_FN_TRAPEVENT,                 "trapevent"},
    {MNG_FN_READ_PUSHDATA,             "read_pushdata"},
    {MNG_FN_READ_PUSHSIG,              "read_pushsig"},
    {MNG_FN_READ_PUSHCHUNK,            "read_pushchunk"},

    {MNG_FN_SETCB_MEMALLOC,            "setcb_memalloc"},
    {MNG_FN_SETCB_MEMFREE,             "setcb_memfree"},
    {MNG_FN_SETCB_READDATA,            "setcb_readdata"},
    {MNG_FN_SETCB_WRITEDATA,           "setcb_writedata"},
    {MNG_FN_SETCB_ERRORPROC,           "setcb_errorproc"},
    {MNG_FN_SETCB_TRACEPROC,           "setcb_traceproc"},
    {MNG_FN_SETCB_PROCESSHEADER,       "setcb_processheader"},
    {MNG_FN_SETCB_PROCESSTEXT,         "setcb_processtext"},
    {MNG_FN_SETCB_GETCANVASLINE,       "setcb_getcanvasline"},
    {MNG_FN_SETCB_GETBKGDLINE,         "setcb_getbkgdline"},
    {MNG_FN_SETCB_REFRESH,             "setcb_refresh"},
    {MNG_FN_SETCB_GETTICKCOUNT,        "setcb_gettickcount"},
    {MNG_FN_SETCB_SETTIMER,            "setcb_settimer"},
    {MNG_FN_SETCB_PROCESSGAMMA,        "setcb_processgamma"},
    {MNG_FN_SETCB_PROCESSCHROMA,       "setcb_processchroma"},
    {MNG_FN_SETCB_PROCESSSRGB,         "setcb_processsrgb"},
    {MNG_FN_SETCB_PROCESSICCP,         "setcb_processiccp"},
    {MNG_FN_SETCB_PROCESSAROW,         "setcb_processarow"},
#ifndef MNG_NO_OPEN_CLOSE_STREAM
    {MNG_FN_SETCB_OPENSTREAM,          "setcb_openstream"},
    {MNG_FN_SETCB_CLOSESTREAM,         "setcb_closestream"},
#endif
    {MNG_FN_SETCB_GETALPHALINE,        "setcb_getalphaline"},
    {MNG_FN_SETCB_PROCESSSAVE,         "setcb_processsave"},
    {MNG_FN_SETCB_PROCESSSEEK,         "setcb_processseek"},
    {MNG_FN_SETCB_PROCESSNEED,         "setcb_processneed"},
    {MNG_FN_SETCB_PROCESSUNKNOWN,      "setcb_processunknown"},
    {MNG_FN_SETCB_PROCESSMEND,         "setcb_processmend"},
    {MNG_FN_SETCB_PROCESSTERM,         "setcb_processterm"},
    {MNG_FN_SETCB_RELEASEDATA,         "setcb_releasedata"},

    {MNG_FN_GETCB_MEMALLOC,            "getcb_memalloc"},
    {MNG_FN_GETCB_MEMFREE,             "getcb_memfree"},
    {MNG_FN_GETCB_READDATA,            "getcb_readdata,"},
    {MNG_FN_GETCB_WRITEDATA,           "getcb_writedata"},
    {MNG_FN_GETCB_ERRORPROC,           "getcb_errorproc"},
    {MNG_FN_GETCB_TRACEPROC,           "getcb_traceproc"},
    {MNG_FN_GETCB_PROCESSHEADER,       "getcb_processheader"},
    {MNG_FN_GETCB_PROCESSTEXT,         "getcb_processtext"},
    {MNG_FN_GETCB_GETCANVASLINE,       "getcb_getcanvasline"},
    {MNG_FN_GETCB_GETBKGDLINE,         "getcb_getbkgdline"},
    {MNG_FN_GETCB_REFRESH,             "getcb_refresh"},
    {MNG_FN_GETCB_GETTICKCOUNT,        "getcb_gettickcount"},
    {MNG_FN_GETCB_SETTIMER,            "getcb_settimer"},
    {MNG_FN_GETCB_PROCESSGAMMA,        "getcb_processgamma"},
    {MNG_FN_GETCB_PROCESSCHROMA,       "getcb_processchroma"},
    {MNG_FN_GETCB_PROCESSSRGB,         "getcb_processsrgb"},
    {MNG_FN_GETCB_PROCESSICCP,         "getcb_processiccp"},
    {MNG_FN_GETCB_PROCESSAROW,         "getcb_processarow"},
#ifndef MNG_NO_OPEN_CLOSE_STREAM
    {MNG_FN_GETCB_OPENSTREAM,          "getcb_openstream"},
    {MNG_FN_GETCB_CLOSESTREAM,         "getcb_closestream"},
#endif
    {MNG_FN_GETCB_GETALPHALINE,        "getcb_getalphaline"},
    {MNG_FN_GETCB_PROCESSSAVE,         "getcb_processsave"},
    {MNG_FN_GETCB_PROCESSSEEK,         "getcb_processseek"},
    {MNG_FN_GETCB_PROCESSNEED,         "getcb_processneed"},
    {MNG_FN_GETCB_PROCESSUNKNOWN,      "getcb_processunknown"},
    {MNG_FN_GETCB_PROCESSMEND,         "getcb_processmend"},
    {MNG_FN_GETCB_PROCESSTERM,         "getcb_processterm"},
    {MNG_FN_GETCB_RELEASEDATA,         "getcb_releasedata"},

    {MNG_FN_SET_USERDATA,              "set_userdata"},
    {MNG_FN_SET_CANVASSTYLE,           "set_canvasstyle"},
    {MNG_FN_SET_BKGDSTYLE,             "set_bkgdstyle"},
    {MNG_FN_SET_BGCOLOR,               "set_bgcolor"},
    {MNG_FN_SET_STORECHUNKS,           "set_storechunks"},
#if defined(MNG_FULL_CMS) || defined(MNG_GAMMA_ONLY) || defined(MNG_APP_CMS)
    {MNG_FN_SET_VIEWGAMMA,             "set_viewgamma"},
#ifndef MNG_NO_DFLT_INFO
    {MNG_FN_SET_DISPLAYGAMMA,          "set_displaygamma"},
#endif
    {MNG_FN_SET_DFLTIMGGAMMA,          "set_dfltimggamma"},
#endif
    {MNG_FN_SET_SRGB,                  "set_srgb"},
    {MNG_FN_SET_OUTPUTPROFILE,         "set_outputprofile"},
    {MNG_FN_SET_SRGBPROFILE,           "set_srgbprofile"},
#ifndef MNG_SKIP_MAXCANVAS
    {MNG_FN_SET_MAXCANVASWIDTH,        "set_maxcanvaswidth"},
    {MNG_FN_SET_MAXCANVASHEIGHT,       "set_maxcanvasheight"},
    {MNG_FN_SET_MAXCANVASSIZE,         "set_maxcanvassize"},
#endif
#ifndef MNG_NO_ACCESS_ZLIB
    {MNG_FN_SET_ZLIB_LEVEL,            "set_zlib_level"},
    {MNG_FN_SET_ZLIB_METHOD,           "set_zlib_method"},
    {MNG_FN_SET_ZLIB_WINDOWBITS,       "set_zlib_windowbits"},
    {MNG_FN_SET_ZLIB_MEMLEVEL,         "set_zlib_memlevel"},
    {MNG_FN_SET_ZLIB_STRATEGY,         "set_zlib_strategy"},
    {MNG_FN_SET_ZLIB_MAXIDAT,          "set_zlib_maxidat"},
#endif
#ifndef MNG_NO_ACCESS_JPEG
    {MNG_FN_SET_JPEG_DCTMETHOD,        "set_jpeg_dctmethod"},
    {MNG_FN_SET_JPEG_QUALITY,          "set_jpeg_quality"},
    {MNG_FN_SET_JPEG_SMOOTHING,        "set_jpeg_smoothing"},
    {MNG_FN_SET_JPEG_PROGRESSIVE,      "set_jpeg_progressive"},
    {MNG_FN_SET_JPEG_OPTIMIZED,        "set_jpeg_optimized"},
    {MNG_FN_SET_JPEG_MAXJDAT,          "set_jpeg_maxjdat"},
#endif
    {MNG_FN_SET_SPEED,                 "set_speed"},
    {MNG_FN_SET_SUSPENSIONMODE,        "set_suspensionmode"},
    {MNG_FN_SET_SECTIONBREAKS,         "set_sectionbreaks"},
    {MNG_FN_SET_USEBKGD,               "set_usebkgd"},
    {MNG_FN_SET_OUTPUTPROFILE2,        "set_outputprofile2"},
    {MNG_FN_SET_SRGBPROFILE2,          "set_srgbprofile2"},
    {MNG_FN_SET_OUTPUTSRGB,            "set_outputsrgb"},
    {MNG_FN_SET_SRGBIMPLICIT,          "set_srgbimplicit"},
    {MNG_FN_SET_CACHEPLAYBACK,         "set_cacheplayback"},
    {MNG_FN_SET_DOPROGRESSIVE,         "set_doprogressive"},
    {MNG_FN_SET_CRCMODE,               "set_crcmode"},

    {MNG_FN_GET_USERDATA,              "get_userdata"},
    {MNG_FN_GET_SIGTYPE,               "get_sigtype"},
    {MNG_FN_GET_IMAGETYPE,             "get_imagetype"},
    {MNG_FN_GET_IMAGEWIDTH,            "get_imagewidth"},
    {MNG_FN_GET_IMAGEHEIGHT,           "get_imageheight"},
    {MNG_FN_GET_TICKS,                 "get_ticks"},
    {MNG_FN_GET_FRAMECOUNT,            "get_framecount"},
    {MNG_FN_GET_LAYERCOUNT,            "get_layercount"},
    {MNG_FN_GET_PLAYTIME,              "get_playtime"},
    {MNG_FN_GET_SIMPLICITY,            "get_simplicity"},
    {MNG_FN_GET_CANVASSTYLE,           "get_canvasstyle"},
    {MNG_FN_GET_BKGDSTYLE,             "get_bkgdstyle"},
    {MNG_FN_GET_BGCOLOR,               "get_bgcolor"},
    {MNG_FN_GET_STORECHUNKS,           "get_storechunks"},
#if defined(MNG_FULL_CMS) || defined(MNG_GAMMA_ONLY) || defined(MNG_APP_CMS)
    {MNG_FN_GET_VIEWGAMMA,             "get_viewgamma"},
    {MNG_FN_GET_DISPLAYGAMMA,          "get_displaygamma"},
#ifndef MNG_NO_DFLT_INFO
    {MNG_FN_GET_DFLTIMGGAMMA,          "get_dfltimggamma"},
#endif
#endif
    {MNG_FN_GET_SRGB,                  "get_srgb"},
#ifndef MNG_SKIP_MAXCANVAS
    {MNG_FN_GET_MAXCANVASWIDTH,        "get_maxcanvaswidth"},
    {MNG_FN_GET_MAXCANVASHEIGHT,       "get_maxcanvasheight"},
#endif
#ifndef MNG_NO_ACCESS_ZLIB
    {MNG_FN_GET_ZLIB_LEVEL,            "get_zlib_level"},
    {MNG_FN_GET_ZLIB_METHOD,           "get_zlib_method"},
    {MNG_FN_GET_ZLIB_WINDOWBITS,       "get_zlib_windowbits"},
    {MNG_FN_GET_ZLIB_MEMLEVEL,         "get_zlib_memlevel"},
    {MNG_FN_GET_ZLIB_STRATEGY,         "get_zlib_strategy"},
    {MNG_FN_GET_ZLIB_MAXIDAT,          "get_zlib_maxidat"},
#endif
#ifndef MNG_NO_ACCESS_JPEG
    {MNG_FN_GET_JPEG_DCTMETHOD,        "get_jpeg_dctmethod"},
    {MNG_FN_GET_JPEG_QUALITY,          "get_jpeg_quality"},
    {MNG_FN_GET_JPEG_SMOOTHING,        "get_jpeg_smoothing"},
    {MNG_FN_GET_JPEG_PROGRESSIVE,      "get_jpeg_progressive"},
    {MNG_FN_GET_JPEG_OPTIMIZED,        "get_jpeg_optimized"},
    {MNG_FN_GET_JPEG_MAXJDAT,          "get_jpeg_maxjdat"},
#endif
    {MNG_FN_GET_SPEED,                 "get_speed"},
    {MNG_FN_GET_IMAGELEVEL,            "get_imagelevel"},
    {MNG_FN_GET_SUSPENSIONMODE,        "get_speed"},
    {MNG_FN_GET_STARTTIME,             "get_starttime"},
    {MNG_FN_GET_RUNTIME,               "get_runtime"},
#ifndef MNG_NO_CURRENT_INFO
    {MNG_FN_GET_CURRENTFRAME,          "get_currentframe"},
    {MNG_FN_GET_CURRENTLAYER,          "get_currentlayer"},
    {MNG_FN_GET_CURRENTPLAYTIME,       "get_currentplaytime"},
#endif
    {MNG_FN_GET_SECTIONBREAKS,         "get_sectionbreaks"},
    {MNG_FN_GET_ALPHADEPTH,            "get_alphadepth"},
    {MNG_FN_GET_BITDEPTH,              "get_bitdepth"},
    {MNG_FN_GET_COLORTYPE,             "get_colortype"},
    {MNG_FN_GET_COMPRESSION,           "get_compression"},
    {MNG_FN_GET_FILTER,                "get_filter"},
    {MNG_FN_GET_INTERLACE,             "get_interlace"},
    {MNG_FN_GET_ALPHABITDEPTH,         "get_alphabitdepth"},
    {MNG_FN_GET_ALPHACOMPRESSION,      "get_alphacompression"},
    {MNG_FN_GET_ALPHAFILTER,           "get_alphafilter"},
    {MNG_FN_GET_ALPHAINTERLACE,        "get_alphainterlace"},
    {MNG_FN_GET_USEBKGD,               "get_usebkgd"},
    {MNG_FN_GET_REFRESHPASS,           "get_refreshpass"},
    {MNG_FN_GET_CACHEPLAYBACK,         "get_cacheplayback"},
    {MNG_FN_GET_DOPROGRESSIVE,         "get_doprogressive"},
    {MNG_FN_GET_LASTBACKCHUNK,         "get_lastbackchunk"},
    {MNG_FN_GET_LASTSEEKNAME,          "get_lastseekname"},
#ifndef MNG_NO_CURRENT_INFO
    {MNG_FN_GET_TOTALFRAMES,           "get_totalframes"},
    {MNG_FN_GET_TOTALLAYERS,           "get_totallayers"},
    {MNG_FN_GET_TOTALPLAYTIME,         "get_totalplaytime"},
#endif
    {MNG_FN_GET_CRCMODE,               "get_crcmode"},
    {MNG_FN_GET_CURRFRAMDELAY,         "get_currframdelay"},

    {MNG_FN_STATUS_ERROR,              "status_error"},
    {MNG_FN_STATUS_READING,            "status_reading"},
    {MNG_FN_STATUS_SUSPENDBREAK,       "status_suspendbreak"},
    {MNG_FN_STATUS_CREATING,           "status_creating"},
    {MNG_FN_STATUS_WRITING,            "status_writing"},
    {MNG_FN_STATUS_DISPLAYING,         "status_displaying"},
    {MNG_FN_STATUS_RUNNING,            "status_running"},
    {MNG_FN_STATUS_TIMERBREAK,         "status_timerbreak"},
    {MNG_FN_STATUS_DYNAMIC,            "status_dynamic"},
    {MNG_FN_STATUS_RUNNINGEVENT,       "status_runningevent"},

    {MNG_FN_ITERATE_CHUNKS,            "iterate_chunks"},
    {MNG_FN_COPY_CHUNK,                "copy_chunk"},

    {MNG_FN_GETCHUNK_IHDR,             "getchunk_ihdr"},
    {MNG_FN_GETCHUNK_PLTE,             "getchunk_plte"},
    {MNG_FN_GETCHUNK_IDAT,             "getchunk_idat"},
    {MNG_FN_GETCHUNK_IEND,             "getchunk_iend"},
    {MNG_FN_GETCHUNK_TRNS,             "getchunk_trns"},
#ifndef MNG_SKIPCHUNK_gAMA
    {MNG_FN_GETCHUNK_GAMA,             "getchunk_gama"},
#endif
#ifndef MNG_SKIPCHUNK_cHRM
    {MNG_FN_GETCHUNK_CHRM,             "getchunk_chrm"},
#endif
#ifndef MNG_SKIPCHUNK_sRGB
    {MNG_FN_GETCHUNK_SRGB,             "getchunk_srgb"},
#endif
#ifndef MNG_SKIPCHUNK_iCCP
    {MNG_FN_GETCHUNK_ICCP,             "getchunk_iccp"},
#endif
#ifndef MNG_SKIPCHUNK_tEXt
    {MNG_FN_GETCHUNK_TEXT,             "getchunk_text"},
#endif
#ifndef MNG_SKIPCHUNK_zTXt
    {MNG_FN_GETCHUNK_ZTXT,             "getchunk_ztxt"},
#endif
#ifndef MNG_SKIPCHUNK_iTXt
    {MNG_FN_GETCHUNK_ITXT,             "getchunk_itxt"},
#endif
#ifndef MNG_SKIPCHUNK_bKGD
    {MNG_FN_GETCHUNK_BKGD,             "getchunk_bkgd"},
#endif
#ifndef MNG_SKIPCHUNK_pHYs
    {MNG_FN_GETCHUNK_PHYS,             "getchunk_phys"},
#endif
#ifndef MNG_SKIPCHUNK_sBIT
    {MNG_FN_GETCHUNK_SBIT,             "getchunk_sbit"},
#endif
#ifndef MNG_SKIPCHUNK_sPLT
    {MNG_FN_GETCHUNK_SPLT,             "getchunk_splt"},
#endif
#ifndef MNG_SKIPCHUNK_hIST
    {MNG_FN_GETCHUNK_HIST,             "getchunk_hist"},
#endif
#ifndef MNG_SKIPCHUNK_tIME
    {MNG_FN_GETCHUNK_TIME,             "getchunk_time"},
#endif
    {MNG_FN_GETCHUNK_MHDR,             "getchunk_mhdr"},
    {MNG_FN_GETCHUNK_MEND,             "getchunk_mend"},
#ifndef MNG_SKIPCHUNK_LOOP
    {MNG_FN_GETCHUNK_LOOP,             "getchunk_loop"},
    {MNG_FN_GETCHUNK_ENDL,             "getchunk_endl"},
#endif
    {MNG_FN_GETCHUNK_DEFI,             "getchunk_defi"},
#ifndef MNG_SKIPCHUNK_BASI
    {MNG_FN_GETCHUNK_BASI,             "getchunk_basi"},
#endif
    {MNG_FN_GETCHUNK_CLON,             "getchunk_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_GETCHUNK_PAST,             "getchunk_past"},
#endif
    {MNG_FN_GETCHUNK_DISC,             "getchunk_disc"},
    {MNG_FN_GETCHUNK_BACK,             "getchunk_back"},
    {MNG_FN_GETCHUNK_FRAM,             "getchunk_fram"},
    {MNG_FN_GETCHUNK_MOVE,             "getchunk_move"},
    {MNG_FN_GETCHUNK_CLIP,             "getchunk_clip"},
    {MNG_FN_GETCHUNK_SHOW,             "getchunk_show"},
    {MNG_FN_GETCHUNK_TERM,             "getchunk_term"},
#ifndef MNG_SKIPCHUNK_SAVE
    {MNG_FN_GETCHUNK_SAVE,             "getchunk_save"},
#endif
#ifndef MNG_SKIPCHUNK_SEEK
    {MNG_FN_GETCHUNK_SEEK,             "getchunk_seek"},
#endif
#ifndef MNG_SKIPCHUNK_eXPI
    {MNG_FN_GETCHUNK_EXPI,             "getchunk_expi"},
#endif
#ifndef MNG_SKIPCHUNK_fPRI
    {MNG_FN_GETCHUNK_FPRI,             "getchunk_fpri"},
#endif
#ifndef MNG_SKIPCHUNK_nEED
    {MNG_FN_GETCHUNK_NEED,             "getchunk_need"},
#endif
#ifndef MNG_SKIPCHUNK_pHYg
    {MNG_FN_GETCHUNK_PHYG,             "getchunk_phyg"},
#endif
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_GETCHUNK_JHDR,             "getchunk_jhdr"},
    {MNG_FN_GETCHUNK_JDAT,             "getchunk_jdat"},
    {MNG_FN_GETCHUNK_JSEP,             "getchunk_jsep"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_GETCHUNK_DHDR,             "getchunk_dhdr"},
    {MNG_FN_GETCHUNK_PROM,             "getchunk_prom"},
    {MNG_FN_GETCHUNK_IPNG,             "getchunk_ipng"},
    {MNG_FN_GETCHUNK_PPLT,             "getchunk_pplt"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_GETCHUNK_IJNG,             "getchunk_ijng"},
#endif
#ifndef MNG_SKIPCHUNK_DROP
    {MNG_FN_GETCHUNK_DROP,             "getchunk_drop"},
#endif
#ifndef MNG_SKIPCHUNK_DBYK
    {MNG_FN_GETCHUNK_DBYK,             "getchunk_dbyk"},
#endif
#ifndef MNG_SKIPCHUNK_ORDR
    {MNG_FN_GETCHUNK_ORDR,             "getchunk_ordr"},
#endif
#endif
    {MNG_FN_GETCHUNK_UNKNOWN,          "getchunk_unknown"},
    {MNG_FN_GETCHUNK_MAGN,             "getchunk_magn"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_GETCHUNK_JDAA,             "getchunk_jdaa"},
#endif
#ifndef MNG_SKIPCHUNK_evNT
    {MNG_FN_GETCHUNK_EVNT,             "getchunk_evnt"},
#endif
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_GETCHUNK_MPNG,             "getchunk_mpng"},
#endif

#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_GETCHUNK_PAST_SRC,         "getchunk_past_src"},
#endif
#ifndef MNG_SKIPCHUNK_SAVE
    {MNG_FN_GETCHUNK_SAVE_ENTRY,       "getchunk_save_entry"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_GETCHUNK_PPLT_ENTRY,       "getchunk_pplt_entry"},
    {MNG_FN_GETCHUNK_ORDR_ENTRY,       "getchunk_ordr_entry"},
#endif
#ifndef MNG_SKIPCHUNK_evNT
    {MNG_FN_GETCHUNK_EVNT_ENTRY,       "getchunk_evnt_entry"},
#endif
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_GETCHUNK_MPNG_FRAME,       "getchunk_mpng_frame"},
#endif

    {MNG_FN_PUTCHUNK_IHDR,             "putchunk_ihdr"},
    {MNG_FN_PUTCHUNK_PLTE,             "putchunk_plte"},
    {MNG_FN_PUTCHUNK_IDAT,             "putchunk_idat"},
    {MNG_FN_PUTCHUNK_IEND,             "putchunk_iend"},
    {MNG_FN_PUTCHUNK_TRNS,             "putchunk_trns"},
#ifndef MNG_SKIPCHUNK_gAMA
    {MNG_FN_PUTCHUNK_GAMA,             "putchunk_gama"},
#endif
#ifndef MNG_SKIPCHUNK_cHRM
    {MNG_FN_PUTCHUNK_CHRM,             "putchunk_chrm"},
#endif
#ifndef MNG_SKIPCHUNK_sRGB
    {MNG_FN_PUTCHUNK_SRGB,             "putchunk_srgb"},
#endif
#ifndef MNG_SKIPCHUNK_iCCP
    {MNG_FN_PUTCHUNK_ICCP,             "putchunk_iccp"},
#endif
#ifndef MNG_SKIPCHUNK_tEXt
    {MNG_FN_PUTCHUNK_TEXT,             "putchunk_text"},
#endif
#ifndef MNG_SKIPCHUNK_zTXt
    {MNG_FN_PUTCHUNK_ZTXT,             "putchunk_ztxt"},
#endif
#ifndef MNG_SKIPCHUNK_iTXt
    {MNG_FN_PUTCHUNK_ITXT,             "putchunk_itxt"},
#endif
#ifndef MNG_SKIPCHUNK_bKGD
    {MNG_FN_PUTCHUNK_BKGD,             "putchunk_bkgd"},
#endif
#ifndef MNG_SKIPCHUNK_pHYs
    {MNG_FN_PUTCHUNK_PHYS,             "putchunk_phys"},
#endif
#ifndef MNG_SKIPCHUNK_sBIT
    {MNG_FN_PUTCHUNK_SBIT,             "putchunk_sbit"},
#endif
#ifndef MNG_SKIPCHUNK_sPLT
    {MNG_FN_PUTCHUNK_SPLT,             "putchunk_splt"},
#endif
#ifndef MNG_SKIPCHUNK_hIST
    {MNG_FN_PUTCHUNK_HIST,             "putchunk_hist"},
#endif
#ifndef MNG_SKIPCHUNK_tIME
    {MNG_FN_PUTCHUNK_TIME,             "putchunk_time"},
#endif
    {MNG_FN_PUTCHUNK_MHDR,             "putchunk_mhdr"},
    {MNG_FN_PUTCHUNK_MEND,             "putchunk_mend"},
    {MNG_FN_PUTCHUNK_LOOP,             "putchunk_loop"},
    {MNG_FN_PUTCHUNK_ENDL,             "putchunk_endl"},
    {MNG_FN_PUTCHUNK_DEFI,             "putchunk_defi"},
    {MNG_FN_PUTCHUNK_BASI,             "putchunk_basi"},
    {MNG_FN_PUTCHUNK_CLON,             "putchunk_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_PUTCHUNK_PAST,             "putchunk_past"},
#endif
    {MNG_FN_PUTCHUNK_DISC,             "putchunk_disc"},
    {MNG_FN_PUTCHUNK_BACK,             "putchunk_back"},
    {MNG_FN_PUTCHUNK_FRAM,             "putchunk_fram"},
    {MNG_FN_PUTCHUNK_MOVE,             "putchunk_move"},
    {MNG_FN_PUTCHUNK_CLIP,             "putchunk_clip"},
    {MNG_FN_PUTCHUNK_SHOW,             "putchunk_show"},
    {MNG_FN_PUTCHUNK_TERM,             "putchunk_term"},
#ifndef MNG_SKIPCHUNK_SAVE
    {MNG_FN_PUTCHUNK_SAVE,             "putchunk_save"},
#endif
#ifndef MNG_SKIPCHUNK_SEEK
    {MNG_FN_PUTCHUNK_SEEK,             "putchunk_seek"},
#endif
#ifndef MNG_SKIPCHUNK_eXPI
    {MNG_FN_PUTCHUNK_EXPI,             "putchunk_expi"},
#endif
#ifndef MNG_SKIPCHUNK_fPRI
    {MNG_FN_PUTCHUNK_FPRI,             "putchunk_fpri"},
#endif
#ifndef MNG_SKIPCHUNK_nEED
    {MNG_FN_PUTCHUNK_NEED,             "putchunk_need"},
#endif
#ifndef MNG_SKIPCHUNK_pHYg
    {MNG_FN_PUTCHUNK_PHYG,             "putchunk_phyg"},
#endif
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_PUTCHUNK_JHDR,             "putchunk_jhdr"},
    {MNG_FN_PUTCHUNK_JDAT,             "putchunk_jdat"},
    {MNG_FN_PUTCHUNK_JSEP,             "putchunk_jsep"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_PUTCHUNK_DHDR,             "putchunk_dhdr"},
    {MNG_FN_PUTCHUNK_PROM,             "putchunk_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_PUTCHUNK_IPNG,             "putchunk_ipng"},
#endif
    {MNG_FN_PUTCHUNK_PPLT,             "putchunk_pplt"},
    {MNG_FN_PUTCHUNK_IJNG,             "putchunk_ijng"},
#ifndef MNG_SKIPCHUNK_DROP
    {MNG_FN_PUTCHUNK_DROP,             "putchunk_drop"},
#endif
#ifndef MNG_SKIPCHUNK_DBYK
    {MNG_FN_PUTCHUNK_DBYK,             "putchunk_dbyk"},
#endif
#ifndef MNG_SKIPCHUNK_ORDR
    {MNG_FN_PUTCHUNK_ORDR,             "putchunk_ordr"},
#endif
#endif
    {MNG_FN_PUTCHUNK_UNKNOWN,          "putchunk_unknown"},
    {MNG_FN_PUTCHUNK_MAGN,             "putchunk_magn"},
    {MNG_FN_PUTCHUNK_JDAA,             "putchunk_jdaa"},
#ifndef MNG_SKIPCHUNK_evNT
    {MNG_FN_PUTCHUNK_EVNT,             "putchunk_evnt"},
#endif
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_PUTCHUNK_MPNG,             "putchunk_mpng"},
#endif

#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_PUTCHUNK_PAST_SRC,         "putchunk_past_src"},
#endif
#ifndef MNG_SKIPCHUNK_SAVE
    {MNG_FN_PUTCHUNK_SAVE_ENTRY,       "putchunk_save_entry"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_PUTCHUNK_PPLT_ENTRY,       "putchunk_pplt_entry"},
#ifndef MNG_SKIPCHUNK_ORDR
    {MNG_FN_PUTCHUNK_ORDR_ENTRY,       "putchunk_ordr_entry"},
#endif
#endif
#ifndef MNG_SKIPCHUNK_evNT
    {MNG_FN_PUTCHUNK_EVNT_ENTRY,       "putchunk_evnt_entry"},
#endif
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_PUTCHUNK_MPNG_FRAME,       "putchunk_mpng_frame"},
#endif

    {MNG_FN_GETIMGDATA_SEQ,            "getimgdata_seq"},
    {MNG_FN_GETIMGDATA_CHUNKSEQ,       "getimgdata_chunkseq"},
    {MNG_FN_GETIMGDATA_CHUNK,          "getimgdata_chunk"},

    {MNG_FN_PUTIMGDATA_IHDR,           "putimgdata_ihdr"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_PUTIMGDATA_JHDR,           "putimgdata_jhdr"},
    {MNG_FN_PUTIMGDATA_BASI,           "putimgdata_basi"},
    {MNG_FN_PUTIMGDATA_DHDR,           "putimgdata_dhdr"},
#endif

    {MNG_FN_UPDATEMNGHEADER,           "updatemngheader"},
    {MNG_FN_UPDATEMNGSIMPLICITY,       "updatemngsimplicity"},

    {MNG_FN_PROCESS_RAW_CHUNK,         "process_raw_chunk"},
    {MNG_FN_READ_GRAPHIC,              "read_graphic"},
    {MNG_FN_DROP_CHUNKS,               "drop_chunks"},
    {MNG_FN_PROCESS_ERROR,             "process_error"},
    {MNG_FN_CLEAR_CMS,                 "clear_cms"},
    {MNG_FN_DROP_OBJECTS,              "drop_objects"},
    {MNG_FN_READ_CHUNK,                "read_chunk"},
    {MNG_FN_LOAD_BKGDLAYER,            "load_bkgdlayer"},
    {MNG_FN_NEXT_FRAME,                "next_frame"},
    {MNG_FN_NEXT_LAYER,                "next_layer"},
    {MNG_FN_INTERFRAME_DELAY,          "interframe_delay"},
    {MNG_FN_DISPLAY_IMAGE,             "display_image"},
    {MNG_FN_DROP_IMGOBJECTS,           "drop_imgobjects"},
    {MNG_FN_DROP_ANIOBJECTS,           "drop_aniobjects"},
    {MNG_FN_INFLATE_BUFFER,            "inflate_buffer"},
    {MNG_FN_DEFLATE_BUFFER,            "deflate_buffer"},
    {MNG_FN_WRITE_RAW_CHUNK,           "write_raw_chunk"},
    {MNG_FN_WRITE_GRAPHIC,             "write_graphic"},
    {MNG_FN_SAVE_STATE,                "save_state"},
    {MNG_FN_RESTORE_STATE,             "restore_state"},
    {MNG_FN_DROP_SAVEDATA,             "drop_savedata"},
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_EXECUTE_DELTA_IMAGE,       "execute_delta_image"},
#endif
    {MNG_FN_PROCESS_DISPLAY,           "process_display"},
    {MNG_FN_CLEAR_CANVAS,              "clear_canvas"},
    {MNG_FN_READ_DATABUFFER,           "read_databuffer"},
    {MNG_FN_STORE_ERROR,               "store_error"},
    {MNG_FN_DROP_INVALID_OBJECTS,      "drop_invalid_objects"},
    {MNG_FN_RELEASE_PUSHDATA,          "release_pushdata"},
    {MNG_FN_READ_DATA,                 "read_data"},
    {MNG_FN_READ_CHUNK_CRC,            "read_chunk_crc"},
    {MNG_FN_RELEASE_PUSHCHUNK,         "release_pushchunk"},

    {MNG_FN_DISPLAY_RGB8,              "display_rgb8"},
    {MNG_FN_DISPLAY_RGBA8,             "display_rgba8"},
    {MNG_FN_DISPLAY_ARGB8,             "display_argb8"},
    {MNG_FN_DISPLAY_BGR8,              "display_bgr8"},
    {MNG_FN_DISPLAY_BGRA8,             "display_bgra8"},
    {MNG_FN_DISPLAY_ABGR8,             "display_abgr8"},
    {MNG_FN_DISPLAY_RGB16,             "display_rgb16"},
    {MNG_FN_DISPLAY_RGBA16,            "display_rgba16"},
    {MNG_FN_DISPLAY_ARGB16,            "display_argb16"},
    {MNG_FN_DISPLAY_BGR16,             "display_bgr16"},
    {MNG_FN_DISPLAY_BGRA16,            "display_bgra16"},
    {MNG_FN_DISPLAY_ABGR16,            "display_abgr16"},
    {MNG_FN_DISPLAY_INDEX8,            "display_index8"},
    {MNG_FN_DISPLAY_INDEXA8,           "display_indexa8"},
    {MNG_FN_DISPLAY_AINDEX8,           "display_aindex8"},
    {MNG_FN_DISPLAY_GRAY8,             "display_gray8"},
    {MNG_FN_DISPLAY_GRAY16,            "display_gray16"},
    {MNG_FN_DISPLAY_GRAYA8,            "display_graya8"},
    {MNG_FN_DISPLAY_GRAYA16,           "display_graya16"},
    {MNG_FN_DISPLAY_AGRAY8,            "display_agray8"},
    {MNG_FN_DISPLAY_AGRAY16,           "display_agray16"},
    {MNG_FN_DISPLAY_DX15,              "display_dx15"},
    {MNG_FN_DISPLAY_DX16,              "display_dx16"},
    {MNG_FN_DISPLAY_RGB8_A8,           "display_rgb8_a8"},
    {MNG_FN_DISPLAY_BGRA8PM,           "display_bgra8_pm"},
    {MNG_FN_DISPLAY_BGRX8,             "display_bgrx8"},
    {MNG_FN_DISPLAY_RGB565,            "display_rgb565"},
    {MNG_FN_DISPLAY_RGBA565,           "display_rgba565"},
    {MNG_FN_DISPLAY_BGR565,            "display_bgr565"},
    {MNG_FN_DISPLAY_BGRA565,           "display_bgra565"},
    {MNG_FN_DISPLAY_RGBA8_PM,          "display_rgba8_pm"},
    {MNG_FN_DISPLAY_ARGB8_PM,          "display_argb8_pm"},
    {MNG_FN_DISPLAY_ABGR8_PM,          "display_abgr8_pm"},
    {MNG_FN_DISPLAY_BGR565_A8,         "display_bgr565_a8"},

    {MNG_FN_INIT_FULL_CMS,             "init_full_cms"},
    {MNG_FN_CORRECT_FULL_CMS,          "correct_full_cms"},
    {MNG_FN_INIT_GAMMA_ONLY,           "init_gamma_only"},
    {MNG_FN_CORRECT_GAMMA_ONLY,        "correct_gamma_only"},
    {MNG_FN_CORRECT_APP_CMS,           "correct_app_cms"},
    {MNG_FN_INIT_FULL_CMS_OBJ,         "init_full_cms_obj"},
    {MNG_FN_INIT_GAMMA_ONLY_OBJ,       "init_gamma_only_obj"},
    {MNG_FN_INIT_APP_CMS,              "init_app_cms"},
    {MNG_FN_INIT_APP_CMS_OBJ,          "init_app_cms_obj"},

    {MNG_FN_PROCESS_G1,                "process_g1"},
    {MNG_FN_PROCESS_G2,                "process_g2"},
    {MNG_FN_PROCESS_G4,                "process_g4"},
    {MNG_FN_PROCESS_G8,                "process_g8"},
    {MNG_FN_PROCESS_G16,               "process_g16"},
    {MNG_FN_PROCESS_RGB8,              "process_rgb8"},
    {MNG_FN_PROCESS_RGB16,             "process_rgb16"},
    {MNG_FN_PROCESS_IDX1,              "process_idx1"},
    {MNG_FN_PROCESS_IDX2,              "process_idx2"},
    {MNG_FN_PROCESS_IDX4,              "process_idx4"},
    {MNG_FN_PROCESS_IDX8,              "process_idx8"},
    {MNG_FN_PROCESS_GA8,               "process_ga8"},
    {MNG_FN_PROCESS_GA16,              "process_ga16"},
    {MNG_FN_PROCESS_RGBA8,             "process_rgba8"},
    {MNG_FN_PROCESS_RGBA16,            "process_rgba16"},

    {MNG_FN_INIT_G1_I,                 "init_g1_i"},
    {MNG_FN_INIT_G2_I,                 "init_g2_i"},
    {MNG_FN_INIT_G4_I,                 "init_g4_i"},
    {MNG_FN_INIT_G8_I,                 "init_g8_i"},
    {MNG_FN_INIT_G16_I,                "init_g16_i"},
    {MNG_FN_INIT_RGB8_I,               "init_rgb8_i"},
    {MNG_FN_INIT_RGB16_I,              "init_rgb16_i"},
    {MNG_FN_INIT_IDX1_I,               "init_idx1_i"},
    {MNG_FN_INIT_IDX2_I,               "init_idx2_i"},
    {MNG_FN_INIT_IDX4_I,               "init_idx4_i"},
    {MNG_FN_INIT_IDX8_I,               "init_idx8_i"},
    {MNG_FN_INIT_GA8_I,                "init_ga8_i"},
    {MNG_FN_INIT_GA16_I,               "init_ga16_i"},
    {MNG_FN_INIT_RGBA8_I,              "init_rgba8_i"},
    {MNG_FN_INIT_RGBA16_I,             "init_rgba16_i"},
#ifndef MNG_OPTIMIZE_FOOTPRINT_INIT
    {MNG_FN_INIT_G1_NI,                "init_g1_ni"},
    {MNG_FN_INIT_G2_NI,                "init_g2_ni"},
    {MNG_FN_INIT_G4_NI,                "init_g4_ni"},
    {MNG_FN_INIT_G8_NI,                "init_g8_ni"},
    {MNG_FN_INIT_G16_NI,               "init_g16_ni"},
    {MNG_FN_INIT_RGB8_NI,              "init_rgb8_ni"},
    {MNG_FN_INIT_RGB16_NI,             "init_rgb16_ni"},
    {MNG_FN_INIT_IDX1_NI,              "init_idx1_ni"},
    {MNG_FN_INIT_IDX2_NI,              "init_idx2_ni"},
    {MNG_FN_INIT_IDX4_NI,              "init_idx4_ni"},
    {MNG_FN_INIT_IDX8_NI,              "init_idx8_ni"},
    {MNG_FN_INIT_GA8_NI,               "init_ga8_ni"},
    {MNG_FN_INIT_GA16_NI,              "init_ga16_ni"},
    {MNG_FN_INIT_RGBA8_NI,             "init_rgba8_ni"},
    {MNG_FN_INIT_RGBA16_NI,            "init_rgba16_ni"},
#endif

    {MNG_FN_INIT_ROWPROC,              "init_rowproc"},
    {MNG_FN_NEXT_ROW,                  "next_row"},
    {MNG_FN_CLEANUP_ROWPROC,           "cleanup_rowproc"},

    {MNG_FN_FILTER_A_ROW,              "filter_a_row"},
    {MNG_FN_FILTER_SUB,                "filter_sub"},
    {MNG_FN_FILTER_UP,                 "filter_up"},
    {MNG_FN_FILTER_AVERAGE,            "filter_average"},
    {MNG_FN_FILTER_PAETH,              "filter_paeth"},

    {MNG_FN_INIT_ROWDIFFERING,         "init_rowdiffering"},
    {MNG_FN_DIFFER_G1,                 "differ_g1"},
    {MNG_FN_DIFFER_G2,                 "differ_g2"},
    {MNG_FN_DIFFER_G4,                 "differ_g4"},
    {MNG_FN_DIFFER_G8,                 "differ_g8"},
    {MNG_FN_DIFFER_G16,                "differ_g16"},
    {MNG_FN_DIFFER_RGB8,               "differ_rgb8"},
    {MNG_FN_DIFFER_RGB16,              "differ_rgb16"},
    {MNG_FN_DIFFER_IDX1,               "differ_idx1"},
    {MNG_FN_DIFFER_IDX2,               "differ_idx2"},
    {MNG_FN_DIFFER_IDX4,               "differ_idx4"},
    {MNG_FN_DIFFER_IDX8,               "differ_idx8"},
    {MNG_FN_DIFFER_GA8,                "differ_ga8"},
    {MNG_FN_DIFFER_GA16,               "differ_ga16"},
    {MNG_FN_DIFFER_RGBA8,              "differ_rgba8"},
    {MNG_FN_DIFFER_RGBA16,             "differ_rgba16"},

    {MNG_FN_CREATE_IMGDATAOBJECT,      "create_imgdataobject"},
    {MNG_FN_FREE_IMGDATAOBJECT,        "free_imgdataobject"},
    {MNG_FN_CLONE_IMGDATAOBJECT,       "clone_imgdataobject"},
    {MNG_FN_CREATE_IMGOBJECT,          "create_imgobject"},
    {MNG_FN_FREE_IMGOBJECT,            "free_imgobject"},
    {MNG_FN_FIND_IMGOBJECT,            "find_imgobject"},
    {MNG_FN_CLONE_IMGOBJECT,           "clone_imgobject"},
    {MNG_FN_RESET_OBJECTDETAILS,       "reset_objectdetails"},
    {MNG_FN_RENUM_IMGOBJECT,           "renum_imgobject"},
    {MNG_FN_PROMOTE_IMGOBJECT,         "promote_imgobject"},
    {MNG_FN_MAGNIFY_IMGOBJECT,         "magnify_imgobject"},
    {MNG_FN_COLORCORRECT_OBJECT,       "colorcorrect_object"},

    {MNG_FN_STORE_G1,                  "store_g1"},
    {MNG_FN_STORE_G2,                  "store_g2"},
    {MNG_FN_STORE_G4,                  "store_g4"},
    {MNG_FN_STORE_G8,                  "store_g8"},
    {MNG_FN_STORE_G16,                 "store_g16"},
    {MNG_FN_STORE_RGB8,                "store_rgb8"},
    {MNG_FN_STORE_RGB16,               "store_rgb16"},
    {MNG_FN_STORE_IDX1,                "store_idx1"},
    {MNG_FN_STORE_IDX2,                "store_idx2"},
    {MNG_FN_STORE_IDX4,                "store_idx4"},
    {MNG_FN_STORE_IDX8,                "store_idx8"},
    {MNG_FN_STORE_GA8,                 "store_ga8"},
    {MNG_FN_STORE_GA16,                "store_ga16"},
    {MNG_FN_STORE_RGBA8,               "store_rgba8"},
    {MNG_FN_STORE_RGBA16,              "store_rgba16"},

    {MNG_FN_RETRIEVE_G8,               "retrieve_g8"},
    {MNG_FN_RETRIEVE_G16,              "retrieve_g16"},
    {MNG_FN_RETRIEVE_RGB8,             "retrieve_rgb8"},
    {MNG_FN_RETRIEVE_RGB16,            "retrieve_rgb16"},
    {MNG_FN_RETRIEVE_IDX8,             "retrieve_idx8"},
    {MNG_FN_RETRIEVE_GA8,              "retrieve_ga8"},
    {MNG_FN_RETRIEVE_GA16,             "retrieve_ga16"},
    {MNG_FN_RETRIEVE_RGBA8,            "retrieve_rgba8"},
    {MNG_FN_RETRIEVE_RGBA16,           "retrieve_rgba16"},

#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_DELTA_G1,                  "delta_g1"},
    {MNG_FN_DELTA_G2,                  "delta_g2"},
    {MNG_FN_DELTA_G4,                  "delta_g4"},
    {MNG_FN_DELTA_G8,                  "delta_g8"},
    {MNG_FN_DELTA_G16,                 "delta_g16"},
    {MNG_FN_DELTA_RGB8,                "delta_rgb8"},
    {MNG_FN_DELTA_RGB16,               "delta_rgb16"},
    {MNG_FN_DELTA_IDX1,                "delta_idx1"},
    {MNG_FN_DELTA_IDX2,                "delta_idx2"},
    {MNG_FN_DELTA_IDX4,                "delta_idx4"},
    {MNG_FN_DELTA_IDX8,                "delta_idx8"},
    {MNG_FN_DELTA_GA8,                 "delta_ga8"},
    {MNG_FN_DELTA_GA16,                "delta_ga16"},
    {MNG_FN_DELTA_RGBA8,               "delta_rgba8"},
    {MNG_FN_DELTA_RGBA16,              "delta_rgba16"},
#endif

    {MNG_FN_CREATE_ANI_LOOP,           "create_ani_loop"},
    {MNG_FN_CREATE_ANI_ENDL,           "create_ani_endl"},
    {MNG_FN_CREATE_ANI_DEFI,           "create_ani_defi"},
    {MNG_FN_CREATE_ANI_BASI,           "create_ani_basi"},
    {MNG_FN_CREATE_ANI_CLON,           "create_ani_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_CREATE_ANI_PAST,           "create_ani_past"},
#endif
    {MNG_FN_CREATE_ANI_DISC,           "create_ani_disc"},
    {MNG_FN_CREATE_ANI_BACK,           "create_ani_back"},
    {MNG_FN_CREATE_ANI_FRAM,           "create_ani_fram"},
    {MNG_FN_CREATE_ANI_MOVE,           "create_ani_move"},
    {MNG_FN_CREATE_ANI_CLIP,           "create_ani_clip"},
    {MNG_FN_CREATE_ANI_SHOW,           "create_ani_show"},
    {MNG_FN_CREATE_ANI_TERM,           "create_ani_term"},
    {MNG_FN_CREATE_ANI_SAVE,           "create_ani_save"},
    {MNG_FN_CREATE_ANI_SEEK,           "create_ani_seek"},
    {MNG_FN_CREATE_ANI_GAMA,           "create_ani_gama"},
    {MNG_FN_CREATE_ANI_CHRM,           "create_ani_chrm"},
    {MNG_FN_CREATE_ANI_SRGB,           "create_ani_srgb"},
    {MNG_FN_CREATE_ANI_ICCP,           "create_ani_iccp"},
    {MNG_FN_CREATE_ANI_PLTE,           "create_ani_plte"},
    {MNG_FN_CREATE_ANI_TRNS,           "create_ani_trns"},
    {MNG_FN_CREATE_ANI_BKGD,           "create_ani_bkgd"},
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_CREATE_ANI_DHDR,           "create_ani_dhdr"},
    {MNG_FN_CREATE_ANI_PROM,           "create_ani_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_CREATE_ANI_IPNG,           "create_ani_ipng"},
#endif
    {MNG_FN_CREATE_ANI_IJNG,           "create_ani_ijng"},
    {MNG_FN_CREATE_ANI_PPLT,           "create_ani_pplt"},
#endif
    {MNG_FN_CREATE_ANI_MAGN,           "create_ani_magn"},

    {MNG_FN_CREATE_ANI_IMAGE,          "create_ani_image"},
    {MNG_FN_CREATE_EVENT,              "create_event"},

    {MNG_FN_FREE_ANI_LOOP,             "free_ani_loop"},
    {MNG_FN_FREE_ANI_ENDL,             "free_ani_endl"},
    {MNG_FN_FREE_ANI_DEFI,             "free_ani_defi"},
    {MNG_FN_FREE_ANI_BASI,             "free_ani_basi"},
    {MNG_FN_FREE_ANI_CLON,             "free_ani_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_FREE_ANI_PAST,             "free_ani_past"},
#endif
    {MNG_FN_FREE_ANI_DISC,             "free_ani_disc"},
    {MNG_FN_FREE_ANI_BACK,             "free_ani_back"},
    {MNG_FN_FREE_ANI_FRAM,             "free_ani_fram"},
    {MNG_FN_FREE_ANI_MOVE,             "free_ani_move"},
    {MNG_FN_FREE_ANI_CLIP,             "free_ani_clip"},
    {MNG_FN_FREE_ANI_SHOW,             "free_ani_show"},
    {MNG_FN_FREE_ANI_TERM,             "free_ani_term"},
    {MNG_FN_FREE_ANI_SAVE,             "free_ani_save"},
    {MNG_FN_FREE_ANI_SEEK,             "free_ani_seek"},
    {MNG_FN_FREE_ANI_GAMA,             "free_ani_gama"},
    {MNG_FN_FREE_ANI_CHRM,             "free_ani_chrm"},
    {MNG_FN_FREE_ANI_SRGB,             "free_ani_srgb"},
    {MNG_FN_FREE_ANI_ICCP,             "free_ani_iccp"},
    {MNG_FN_FREE_ANI_PLTE,             "free_ani_plte"},
    {MNG_FN_FREE_ANI_TRNS,             "free_ani_trns"},
    {MNG_FN_FREE_ANI_BKGD,             "free_ani_bkgd"},
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_FREE_ANI_DHDR,             "free_ani_dhdr"},
    {MNG_FN_FREE_ANI_PROM,             "free_ani_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_FREE_ANI_IPNG,             "free_ani_ipng"},
#endif
    {MNG_FN_FREE_ANI_IJNG,             "free_ani_ijng"},
    {MNG_FN_FREE_ANI_PPLT,             "free_ani_pplt"},
#endif
    {MNG_FN_FREE_ANI_MAGN,             "free_ani_magn"},

    {MNG_FN_FREE_ANI_IMAGE,            "free_ani_image"},
    {MNG_FN_FREE_EVENT,                "free_event"},

    {MNG_FN_PROCESS_ANI_LOOP,          "process_ani_loop"},
    {MNG_FN_PROCESS_ANI_ENDL,          "process_ani_endl"},
    {MNG_FN_PROCESS_ANI_DEFI,          "process_ani_defi"},
    {MNG_FN_PROCESS_ANI_BASI,          "process_ani_basi"},
    {MNG_FN_PROCESS_ANI_CLON,          "process_ani_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_PROCESS_ANI_PAST,          "process_ani_past"},
#endif
    {MNG_FN_PROCESS_ANI_DISC,          "process_ani_disc"},
    {MNG_FN_PROCESS_ANI_BACK,          "process_ani_back"},
    {MNG_FN_PROCESS_ANI_FRAM,          "process_ani_fram"},
    {MNG_FN_PROCESS_ANI_MOVE,          "process_ani_move"},
    {MNG_FN_PROCESS_ANI_CLIP,          "process_ani_clip"},
    {MNG_FN_PROCESS_ANI_SHOW,          "process_ani_show"},
    {MNG_FN_PROCESS_ANI_TERM,          "process_ani_term"},
    {MNG_FN_PROCESS_ANI_SAVE,          "process_ani_save"},
    {MNG_FN_PROCESS_ANI_SEEK,          "process_ani_seek"},
    {MNG_FN_PROCESS_ANI_GAMA,          "process_ani_gama"},
    {MNG_FN_PROCESS_ANI_CHRM,          "process_ani_chrm"},
    {MNG_FN_PROCESS_ANI_SRGB,          "process_ani_srgb"},
    {MNG_FN_PROCESS_ANI_ICCP,          "process_ani_iccp"},
    {MNG_FN_PROCESS_ANI_PLTE,          "process_ani_plte"},
    {MNG_FN_PROCESS_ANI_TRNS,          "process_ani_trns"},
    {MNG_FN_PROCESS_ANI_BKGD,          "process_ani_bkgd"},
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_PROCESS_ANI_DHDR,          "process_ani_dhdr"},
    {MNG_FN_PROCESS_ANI_PROM,          "process_ani_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_PROCESS_ANI_IPNG,          "process_ani_ipng"},
#endif
    {MNG_FN_PROCESS_ANI_IJNG,          "process_ani_ijng"},
    {MNG_FN_PROCESS_ANI_PPLT,          "process_ani_pplt"},
#endif
    {MNG_FN_PROCESS_ANI_MAGN,          "process_ani_magn"},

    {MNG_FN_PROCESS_ANI_IMAGE,         "process_ani_image"},
    {MNG_FN_PROCESS_EVENT,             "process_event"},

    {MNG_FN_RESTORE_BACKIMAGE,         "restore_backimage"},
    {MNG_FN_RESTORE_BACKCOLOR,         "restore_backcolor"},
    {MNG_FN_RESTORE_BGCOLOR,           "restore_bgcolor"},
    {MNG_FN_RESTORE_RGB8,              "restore_rgb8"},
    {MNG_FN_RESTORE_BGR8,              "restore_bgr8"},
    {MNG_FN_RESTORE_BKGD,              "restore_bkgd"},
    {MNG_FN_RESTORE_BGRX8,             "restore_bgrx8"},
    {MNG_FN_RESTORE_RGB565,            "restore_rgb565"},

    {MNG_FN_INIT_IHDR,                 "init_ihdr"},
    {MNG_FN_INIT_PLTE,                 "init_plte"},
    {MNG_FN_INIT_IDAT,                 "init_idat"},
    {MNG_FN_INIT_IEND,                 "init_iend"},
    {MNG_FN_INIT_TRNS,                 "init_trns"},
    {MNG_FN_INIT_GAMA,                 "init_gama"},
    {MNG_FN_INIT_CHRM,                 "init_chrm"},
    {MNG_FN_INIT_SRGB,                 "init_srgb"},
    {MNG_FN_INIT_ICCP,                 "init_iccp"},
    {MNG_FN_INIT_TEXT,                 "init_text"},
    {MNG_FN_INIT_ZTXT,                 "init_ztxt"},
    {MNG_FN_INIT_ITXT,                 "init_itxt"},
    {MNG_FN_INIT_BKGD,                 "init_bkgd"},
    {MNG_FN_INIT_PHYS,                 "init_phys"},
    {MNG_FN_INIT_SBIT,                 "init_sbit"},
    {MNG_FN_INIT_SPLT,                 "init_splt"},
    {MNG_FN_INIT_HIST,                 "init_hist"},
    {MNG_FN_INIT_TIME,                 "init_time"},
    {MNG_FN_INIT_MHDR,                 "init_mhdr"},
    {MNG_FN_INIT_MEND,                 "init_mend"},
    {MNG_FN_INIT_LOOP,                 "init_loop"},
    {MNG_FN_INIT_ENDL,                 "init_endl"},
    {MNG_FN_INIT_DEFI,                 "init_defi"},
    {MNG_FN_INIT_BASI,                 "init_basi"},
    {MNG_FN_INIT_CLON,                 "init_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_INIT_PAST,                 "init_past"},
#endif
    {MNG_FN_INIT_DISC,                 "init_disc"},
    {MNG_FN_INIT_BACK,                 "init_back"},
    {MNG_FN_INIT_FRAM,                 "init_fram"},
    {MNG_FN_INIT_MOVE,                 "init_move"},
    {MNG_FN_INIT_CLIP,                 "init_clip"},
    {MNG_FN_INIT_SHOW,                 "init_show"},
    {MNG_FN_INIT_TERM,                 "init_term"},
    {MNG_FN_INIT_SAVE,                 "init_save"},
    {MNG_FN_INIT_SEEK,                 "init_seek"},
    {MNG_FN_INIT_EXPI,                 "init_expi"},
    {MNG_FN_INIT_FPRI,                 "init_fpri"},
    {MNG_FN_INIT_NEED,                 "init_need"},
    {MNG_FN_INIT_PHYG,                 "init_phyg"},
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_INIT_JHDR,                 "init_jhdr"},
    {MNG_FN_INIT_JDAT,                 "init_jdat"},
    {MNG_FN_INIT_JSEP,                 "init_jsep"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_INIT_DHDR,                 "init_dhdr"},
    {MNG_FN_INIT_PROM,                 "init_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_INIT_IPNG,                 "init_ipng"},
#endif
    {MNG_FN_INIT_PPLT,                 "init_pplt"},
    {MNG_FN_INIT_IJNG,                 "init_ijng"},
    {MNG_FN_INIT_DROP,                 "init_drop"},
    {MNG_FN_INIT_DBYK,                 "init_dbyk"},
    {MNG_FN_INIT_ORDR,                 "init_ordr"},
#endif
    {MNG_FN_INIT_UNKNOWN,              "init_unknown"},
    {MNG_FN_INIT_MAGN,                 "init_magn"},
    {MNG_FN_INIT_JDAA,                 "init_jdaa"},
    {MNG_FN_INIT_EVNT,                 "init_evnt"},
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_INIT_MPNG,                 "init_mpng"},
#endif

    {MNG_FN_ASSIGN_IHDR,               "assign_ihdr"},
    {MNG_FN_ASSIGN_PLTE,               "assign_plte"},
    {MNG_FN_ASSIGN_IDAT,               "assign_idat"},
    {MNG_FN_ASSIGN_IEND,               "assign_iend"},
    {MNG_FN_ASSIGN_TRNS,               "assign_trns"},
    {MNG_FN_ASSIGN_GAMA,               "assign_gama"},
    {MNG_FN_ASSIGN_CHRM,               "assign_chrm"},
    {MNG_FN_ASSIGN_SRGB,               "assign_srgb"},
    {MNG_FN_ASSIGN_ICCP,               "assign_iccp"},
    {MNG_FN_ASSIGN_TEXT,               "assign_text"},
    {MNG_FN_ASSIGN_ZTXT,               "assign_ztxt"},
    {MNG_FN_ASSIGN_ITXT,               "assign_itxt"},
    {MNG_FN_ASSIGN_BKGD,               "assign_bkgd"},
    {MNG_FN_ASSIGN_PHYS,               "assign_phys"},
    {MNG_FN_ASSIGN_SBIT,               "assign_sbit"},
    {MNG_FN_ASSIGN_SPLT,               "assign_splt"},
    {MNG_FN_ASSIGN_HIST,               "assign_hist"},
    {MNG_FN_ASSIGN_TIME,               "assign_time"},
    {MNG_FN_ASSIGN_MHDR,               "assign_mhdr"},
    {MNG_FN_ASSIGN_MEND,               "assign_mend"},
    {MNG_FN_ASSIGN_LOOP,               "assign_loop"},
    {MNG_FN_ASSIGN_ENDL,               "assign_endl"},
    {MNG_FN_ASSIGN_DEFI,               "assign_defi"},
    {MNG_FN_ASSIGN_BASI,               "assign_basi"},
    {MNG_FN_ASSIGN_CLON,               "assign_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_ASSIGN_PAST,               "assign_past"},
#endif
    {MNG_FN_ASSIGN_DISC,               "assign_disc"},
    {MNG_FN_ASSIGN_BACK,               "assign_back"},
    {MNG_FN_ASSIGN_FRAM,               "assign_fram"},
    {MNG_FN_ASSIGN_MOVE,               "assign_move"},
    {MNG_FN_ASSIGN_CLIP,               "assign_clip"},
    {MNG_FN_ASSIGN_SHOW,               "assign_show"},
    {MNG_FN_ASSIGN_TERM,               "assign_term"},
    {MNG_FN_ASSIGN_SAVE,               "assign_save"},
    {MNG_FN_ASSIGN_SEEK,               "assign_seek"},
    {MNG_FN_ASSIGN_EXPI,               "assign_expi"},
    {MNG_FN_ASSIGN_FPRI,               "assign_fpri"},
    {MNG_FN_ASSIGN_NEED,               "assign_need"},
    {MNG_FN_ASSIGN_PHYG,               "assign_phyg"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_ASSIGN_JHDR,               "assign_jhdr"},
    {MNG_FN_ASSIGN_JDAT,               "assign_jdat"},
    {MNG_FN_ASSIGN_JSEP,               "assign_jsep"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_ASSIGN_DHDR,               "assign_dhdr"},
    {MNG_FN_ASSIGN_PROM,               "assign_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_ASSIGN_IPNG,               "assign_ipng"},
#endif
    {MNG_FN_ASSIGN_PPLT,               "assign_pplt"},
    {MNG_FN_ASSIGN_IJNG,               "assign_ijng"},
    {MNG_FN_ASSIGN_DROP,               "assign_drop"},
    {MNG_FN_ASSIGN_DBYK,               "assign_dbyk"},
    {MNG_FN_ASSIGN_ORDR,               "assign_ordr"},
#endif
    {MNG_FN_ASSIGN_UNKNOWN,            "assign_unknown"},
    {MNG_FN_ASSIGN_MAGN,               "assign_magn"},
    {MNG_FN_ASSIGN_JDAA,               "assign_jdaa"},
    {MNG_FN_ASSIGN_EVNT,               "assign_evnt"},
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_ASSIGN_MPNG,               "assign_mpng"},
#endif

    {MNG_FN_FREE_IHDR,                 "free_ihdr"},
    {MNG_FN_FREE_PLTE,                 "free_plte"},
    {MNG_FN_FREE_IDAT,                 "free_idat"},
    {MNG_FN_FREE_IEND,                 "free_iend"},
    {MNG_FN_FREE_TRNS,                 "free_trns"},
    {MNG_FN_FREE_GAMA,                 "free_gama"},
    {MNG_FN_FREE_CHRM,                 "free_chrm"},
    {MNG_FN_FREE_SRGB,                 "free_srgb"},
    {MNG_FN_FREE_ICCP,                 "free_iccp"},
    {MNG_FN_FREE_TEXT,                 "free_text"},
    {MNG_FN_FREE_ZTXT,                 "free_ztxt"},
    {MNG_FN_FREE_ITXT,                 "free_itxt"},
    {MNG_FN_FREE_BKGD,                 "free_bkgd"},
    {MNG_FN_FREE_PHYS,                 "free_phys"},
    {MNG_FN_FREE_SBIT,                 "free_sbit"},
    {MNG_FN_FREE_SPLT,                 "free_splt"},
    {MNG_FN_FREE_HIST,                 "free_hist"},
    {MNG_FN_FREE_TIME,                 "free_time"},
    {MNG_FN_FREE_MHDR,                 "free_mhdr"},
    {MNG_FN_FREE_MEND,                 "free_mend"},
    {MNG_FN_FREE_LOOP,                 "free_loop"},
    {MNG_FN_FREE_ENDL,                 "free_endl"},
    {MNG_FN_FREE_DEFI,                 "free_defi"},
    {MNG_FN_FREE_BASI,                 "free_basi"},
    {MNG_FN_FREE_CLON,                 "free_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_FREE_PAST,                 "free_past"},
#endif
    {MNG_FN_FREE_DISC,                 "free_disc"},
    {MNG_FN_FREE_BACK,                 "free_back"},
    {MNG_FN_FREE_FRAM,                 "free_fram"},
    {MNG_FN_FREE_MOVE,                 "free_move"},
    {MNG_FN_FREE_CLIP,                 "free_clip"},
    {MNG_FN_FREE_SHOW,                 "free_show"},
    {MNG_FN_FREE_TERM,                 "free_term"},
    {MNG_FN_FREE_SAVE,                 "free_save"},
    {MNG_FN_FREE_SEEK,                 "free_seek"},
    {MNG_FN_FREE_EXPI,                 "free_expi"},
    {MNG_FN_FREE_FPRI,                 "free_fpri"},
    {MNG_FN_FREE_NEED,                 "free_need"},
    {MNG_FN_FREE_PHYG,                 "free_phyg"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_FREE_JHDR,                 "free_jhdr"},
    {MNG_FN_FREE_JDAT,                 "free_jdat"},
    {MNG_FN_FREE_JSEP,                 "free_jsep"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_FREE_DHDR,                 "free_dhdr"},
    {MNG_FN_FREE_PROM,                 "free_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_FREE_IPNG,                 "free_ipng"},
#endif
    {MNG_FN_FREE_PPLT,                 "free_pplt"},
    {MNG_FN_FREE_IJNG,                 "free_ijng"},
    {MNG_FN_FREE_DROP,                 "free_drop"},
    {MNG_FN_FREE_DBYK,                 "free_dbyk"},
    {MNG_FN_FREE_ORDR,                 "free_ordr"},
#endif
    {MNG_FN_FREE_UNKNOWN,              "free_unknown"},
    {MNG_FN_FREE_MAGN,                 "free_magn"},
    {MNG_FN_FREE_JDAA,                 "free_jdaa"},
    {MNG_FN_FREE_EVNT,                 "free_evnt"},
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_FREE_MPNG,                 "free_mpng"},
#endif

    {MNG_FN_READ_IHDR,                 "read_ihdr"},
    {MNG_FN_READ_PLTE,                 "read_plte"},
    {MNG_FN_READ_IDAT,                 "read_idat"},
    {MNG_FN_READ_IEND,                 "read_iend"},
    {MNG_FN_READ_TRNS,                 "read_trns"},
    {MNG_FN_READ_GAMA,                 "read_gama"},
    {MNG_FN_READ_CHRM,                 "read_chrm"},
    {MNG_FN_READ_SRGB,                 "read_srgb"},
    {MNG_FN_READ_ICCP,                 "read_iccp"},
    {MNG_FN_READ_TEXT,                 "read_text"},
    {MNG_FN_READ_ZTXT,                 "read_ztxt"},
    {MNG_FN_READ_ITXT,                 "read_itxt"},
    {MNG_FN_READ_BKGD,                 "read_bkgd"},
    {MNG_FN_READ_PHYS,                 "read_phys"},
    {MNG_FN_READ_SBIT,                 "read_sbit"},
    {MNG_FN_READ_SPLT,                 "read_splt"},
    {MNG_FN_READ_HIST,                 "read_hist"},
    {MNG_FN_READ_TIME,                 "read_time"},
    {MNG_FN_READ_MHDR,                 "read_mhdr"},
    {MNG_FN_READ_MEND,                 "read_mend"},
    {MNG_FN_READ_LOOP,                 "read_loop"},
    {MNG_FN_READ_ENDL,                 "read_endl"},
    {MNG_FN_READ_DEFI,                 "read_defi"},
    {MNG_FN_READ_BASI,                 "read_basi"},
    {MNG_FN_READ_CLON,                 "read_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_READ_PAST,                 "read_past"},
#endif
    {MNG_FN_READ_DISC,                 "read_disc"},
    {MNG_FN_READ_BACK,                 "read_back"},
    {MNG_FN_READ_FRAM,                 "read_fram"},
    {MNG_FN_READ_MOVE,                 "read_move"},
    {MNG_FN_READ_CLIP,                 "read_clip"},
    {MNG_FN_READ_SHOW,                 "read_show"},
    {MNG_FN_READ_TERM,                 "read_term"},
    {MNG_FN_READ_SAVE,                 "read_save"},
    {MNG_FN_READ_SEEK,                 "read_seek"},
    {MNG_FN_READ_EXPI,                 "read_expi"},
    {MNG_FN_READ_FPRI,                 "read_fpri"},
    {MNG_FN_READ_NEED,                 "read_need"},
    {MNG_FN_READ_PHYG,                 "read_phyg"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_READ_JHDR,                 "read_jhdr"},
    {MNG_FN_READ_JDAT,                 "read_jdat"},
    {MNG_FN_READ_JSEP,                 "read_jsep"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_READ_DHDR,                 "read_dhdr"},
    {MNG_FN_READ_PROM,                 "read_prom"},
    {MNG_FN_READ_IPNG,                 "read_ipng"},
    {MNG_FN_READ_PPLT,                 "read_pplt"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_READ_IJNG,                 "read_ijng"},
#endif
    {MNG_FN_READ_DROP,                 "read_drop"},
    {MNG_FN_READ_DBYK,                 "read_dbyk"},
    {MNG_FN_READ_ORDR,                 "read_ordr"},
#endif
    {MNG_FN_READ_UNKNOWN,              "read_unknown"},
    {MNG_FN_READ_MAGN,                 "read_magn"},
    {MNG_FN_READ_JDAA,                 "read_jdaa"},
    {MNG_FN_READ_EVNT,                 "read_evnt"},
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_READ_MPNG,                 "read_mpng"},
#endif

    {MNG_FN_WRITE_IHDR,                "write_ihdr"},
    {MNG_FN_WRITE_PLTE,                "write_plte"},
    {MNG_FN_WRITE_IDAT,                "write_idat"},
    {MNG_FN_WRITE_IEND,                "write_iend"},
    {MNG_FN_WRITE_TRNS,                "write_trns"},
    {MNG_FN_WRITE_GAMA,                "write_gama"},
    {MNG_FN_WRITE_CHRM,                "write_chrm"},
    {MNG_FN_WRITE_SRGB,                "write_srgb"},
    {MNG_FN_WRITE_ICCP,                "write_iccp"},
    {MNG_FN_WRITE_TEXT,                "write_text"},
    {MNG_FN_WRITE_ZTXT,                "write_ztxt"},
    {MNG_FN_WRITE_ITXT,                "write_itxt"},
    {MNG_FN_WRITE_BKGD,                "write_bkgd"},
    {MNG_FN_WRITE_PHYS,                "write_phys"},
    {MNG_FN_WRITE_SBIT,                "write_sbit"},
    {MNG_FN_WRITE_SPLT,                "write_splt"},
    {MNG_FN_WRITE_HIST,                "write_hist"},
    {MNG_FN_WRITE_TIME,                "write_time"},
    {MNG_FN_WRITE_MHDR,                "write_mhdr"},
    {MNG_FN_WRITE_MEND,                "write_mend"},
    {MNG_FN_WRITE_LOOP,                "write_loop"},
    {MNG_FN_WRITE_ENDL,                "write_endl"},
    {MNG_FN_WRITE_DEFI,                "write_defi"},
    {MNG_FN_WRITE_BASI,                "write_basi"},
    {MNG_FN_WRITE_CLON,                "write_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_WRITE_PAST,                "write_past"},
#endif
    {MNG_FN_WRITE_DISC,                "write_disc"},
    {MNG_FN_WRITE_BACK,                "write_back"},
    {MNG_FN_WRITE_FRAM,                "write_fram"},
    {MNG_FN_WRITE_MOVE,                "write_move"},
    {MNG_FN_WRITE_CLIP,                "write_clip"},
    {MNG_FN_WRITE_SHOW,                "write_show"},
    {MNG_FN_WRITE_TERM,                "write_term"},
    {MNG_FN_WRITE_SAVE,                "write_save"},
    {MNG_FN_WRITE_SEEK,                "write_seek"},
    {MNG_FN_WRITE_EXPI,                "write_expi"},
    {MNG_FN_WRITE_FPRI,                "write_fpri"},
    {MNG_FN_WRITE_NEED,                "write_need"},
    {MNG_FN_WRITE_PHYG,                "write_phyg"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_WRITE_JHDR,                "write_jhdr"},
    {MNG_FN_WRITE_JDAT,                "write_jdat"},
    {MNG_FN_WRITE_JSEP,                "write_jsep"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_WRITE_DHDR,                "write_dhdr"},
    {MNG_FN_WRITE_PROM,                "write_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_WRITE_IPNG,                "write_ipng"},
#endif
    {MNG_FN_WRITE_PPLT,                "write_pplt"},
    {MNG_FN_WRITE_IJNG,                "write_ijng"},
    {MNG_FN_WRITE_DROP,                "write_drop"},
    {MNG_FN_WRITE_DBYK,                "write_dbyk"},
    {MNG_FN_WRITE_ORDR,                "write_ordr"},
#endif
    {MNG_FN_WRITE_UNKNOWN,             "write_unknown"},
    {MNG_FN_WRITE_MAGN,                "write_magn"},
    {MNG_FN_WRITE_JDAA,                "write_jdaa"},
    {MNG_FN_WRITE_EVNT,                "write_evnt"},
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
    {MNG_FN_WRITE_MPNG,                "write_mpng"},
#endif

    {MNG_FN_ZLIB_INITIALIZE,           "zlib_initialize"},
    {MNG_FN_ZLIB_CLEANUP,              "zlib_cleanup"},
    {MNG_FN_ZLIB_INFLATEINIT,          "zlib_inflateinit"},
    {MNG_FN_ZLIB_INFLATEROWS,          "zlib_inflaterows"},
    {MNG_FN_ZLIB_INFLATEDATA,          "zlib_inflatedata"},
    {MNG_FN_ZLIB_INFLATEFREE,          "zlib_inflatefree"},
    {MNG_FN_ZLIB_DEFLATEINIT,          "zlib_deflateinit"},
    {MNG_FN_ZLIB_DEFLATEROWS,          "zlib_deflaterows"},
    {MNG_FN_ZLIB_DEFLATEDATA,          "zlib_deflatedata"},
    {MNG_FN_ZLIB_DEFLATEFREE,          "zlib_deflatefree"},

    {MNG_FN_PROCESS_DISPLAY_IHDR,      "process_display_ihdr"},
    {MNG_FN_PROCESS_DISPLAY_PLTE,      "process_display_plte"},
    {MNG_FN_PROCESS_DISPLAY_IDAT,      "process_display_idat"},
    {MNG_FN_PROCESS_DISPLAY_IEND,      "process_display_iend"},
    {MNG_FN_PROCESS_DISPLAY_TRNS,      "process_display_trns"},
    {MNG_FN_PROCESS_DISPLAY_GAMA,      "process_display_gama"},
    {MNG_FN_PROCESS_DISPLAY_CHRM,      "process_display_chrm"},
    {MNG_FN_PROCESS_DISPLAY_SRGB,      "process_display_srgb"},
    {MNG_FN_PROCESS_DISPLAY_ICCP,      "process_display_iccp"},
    {MNG_FN_PROCESS_DISPLAY_BKGD,      "process_display_bkgd"},
    {MNG_FN_PROCESS_DISPLAY_PHYS,      "process_display_phys"},
    {MNG_FN_PROCESS_DISPLAY_SBIT,      "process_display_sbit"},
    {MNG_FN_PROCESS_DISPLAY_SPLT,      "process_display_splt"},
    {MNG_FN_PROCESS_DISPLAY_HIST,      "process_display_hist"},
    {MNG_FN_PROCESS_DISPLAY_MHDR,      "process_display_mhdr"},
    {MNG_FN_PROCESS_DISPLAY_MEND,      "process_display_mend"},
    {MNG_FN_PROCESS_DISPLAY_LOOP,      "process_display_loop"},
    {MNG_FN_PROCESS_DISPLAY_ENDL,      "process_display_endl"},
    {MNG_FN_PROCESS_DISPLAY_DEFI,      "process_display_defi"},
    {MNG_FN_PROCESS_DISPLAY_BASI,      "process_display_basi"},
    {MNG_FN_PROCESS_DISPLAY_CLON,      "process_display_clon"},
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_FN_PROCESS_DISPLAY_PAST,      "process_display_past"},
#endif
    {MNG_FN_PROCESS_DISPLAY_DISC,      "process_display_disc"},
    {MNG_FN_PROCESS_DISPLAY_BACK,      "process_display_back"},
    {MNG_FN_PROCESS_DISPLAY_FRAM,      "process_display_fram"},
    {MNG_FN_PROCESS_DISPLAY_MOVE,      "process_display_move"},
    {MNG_FN_PROCESS_DISPLAY_CLIP,      "process_display_clip"},
    {MNG_FN_PROCESS_DISPLAY_SHOW,      "process_display_show"},
    {MNG_FN_PROCESS_DISPLAY_TERM,      "process_display_term"},
    {MNG_FN_PROCESS_DISPLAY_SAVE,      "process_display_save"},
    {MNG_FN_PROCESS_DISPLAY_SEEK,      "process_display_seek"},
    {MNG_FN_PROCESS_DISPLAY_EXPI,      "process_display_expi"},
    {MNG_FN_PROCESS_DISPLAY_FPRI,      "process_display_fpri"},
    {MNG_FN_PROCESS_DISPLAY_NEED,      "process_display_need"},
    {MNG_FN_PROCESS_DISPLAY_PHYG,      "process_display_phyg"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_PROCESS_DISPLAY_JHDR,      "process_display_jhdr"},
    {MNG_FN_PROCESS_DISPLAY_JDAT,      "process_display_jdat"},
    {MNG_FN_PROCESS_DISPLAY_JSEP,      "process_display_jsep"},
#endif
#ifndef MNG_NO_DELTA_PNG
    {MNG_FN_PROCESS_DISPLAY_DHDR,      "process_display_dhdr"},
    {MNG_FN_PROCESS_DISPLAY_PROM,      "process_display_prom"},
#ifdef MNG_INCLUDE_JNG
    {MNG_FN_PROCESS_DISPLAY_IPNG,      "process_display_ipng"},
#endif
    {MNG_FN_PROCESS_DISPLAY_PPLT,      "process_display_pplt"},
    {MNG_FN_PROCESS_DISPLAY_IJNG,      "process_display_ijng"},
    {MNG_FN_PROCESS_DISPLAY_DROP,      "process_display_drop"},
    {MNG_FN_PROCESS_DISPLAY_DBYK,      "process_display_dbyk"},
    {MNG_FN_PROCESS_DISPLAY_ORDR,      "process_display_ordr"},
#endif
    {MNG_FN_PROCESS_DISPLAY_MAGN,      "process_display_magn"},
    {MNG_FN_PROCESS_DISPLAY_JDAA,      "process_display_jdaa"},

    {MNG_FN_JPEG_INITIALIZE,           "jpeg_initialize"},
    {MNG_FN_JPEG_CLEANUP,              "jpeg_cleanup"},
    {MNG_FN_JPEG_DECOMPRESSINIT,       "jpeg_decompressinit"},
    {MNG_FN_JPEG_DECOMPRESSDATA,       "jpeg_decompressdata"},
    {MNG_FN_JPEG_DECOMPRESSFREE,       "jpeg_decompressfree"},

    {MNG_FN_STORE_JPEG_G8,             "store_jpeg_g8"},
    {MNG_FN_STORE_JPEG_RGB8,           "store_jpeg_rgb8"},
    {MNG_FN_STORE_JPEG_G12,            "store_jpeg_g12"},
    {MNG_FN_STORE_JPEG_RGB12,          "store_jpeg_rgb12"},
    {MNG_FN_STORE_JPEG_GA8,            "store_jpeg_ga8"},
    {MNG_FN_STORE_JPEG_RGBA8,          "store_jpeg_rgba8"},
    {MNG_FN_STORE_JPEG_GA12,           "store_jpeg_ga12"},
    {MNG_FN_STORE_JPEG_RGBA12,         "store_jpeg_rgba12"},
    {MNG_FN_STORE_JPEG_G8_ALPHA,       "store_jpeg_g8_alpha"},
    {MNG_FN_STORE_JPEG_RGB8_ALPHA,     "store_jpeg_rgb8_alpha"},

    {MNG_FN_INIT_JPEG_A1_NI,           "init_jpeg_a1_ni"},
    {MNG_FN_INIT_JPEG_A2_NI,           "init_jpeg_a2_ni"},
    {MNG_FN_INIT_JPEG_A4_NI,           "init_jpeg_a4_ni"},
    {MNG_FN_INIT_JPEG_A8_NI,           "init_jpeg_a8_ni"},
    {MNG_FN_INIT_JPEG_A16_NI,          "init_jpeg_a16_ni"},

    {MNG_FN_STORE_JPEG_G8_A1,          "store_jpeg_g8_a1"},
    {MNG_FN_STORE_JPEG_G8_A2,          "store_jpeg_g8_a2"},
    {MNG_FN_STORE_JPEG_G8_A4,          "store_jpeg_g8_a4"},
    {MNG_FN_STORE_JPEG_G8_A8,          "store_jpeg_g8_a8"},
    {MNG_FN_STORE_JPEG_G8_A16,         "store_jpeg_g8_a16"},

    {MNG_FN_STORE_JPEG_RGB8_A1,        "store_jpeg_rgb8_a1"},
    {MNG_FN_STORE_JPEG_RGB8_A2,        "store_jpeg_rgb8_a2"},
    {MNG_FN_STORE_JPEG_RGB8_A4,        "store_jpeg_rgb8_a4"},
    {MNG_FN_STORE_JPEG_RGB8_A8,        "store_jpeg_rgb8_a8"},
    {MNG_FN_STORE_JPEG_RGB8_A16,       "store_jpeg_rgb8_a16"},

    {MNG_FN_STORE_JPEG_G12_A1,         "store_jpeg_g12_a1"},
    {MNG_FN_STORE_JPEG_G12_A2,         "store_jpeg_g12_a2"},
    {MNG_FN_STORE_JPEG_G12_A4,         "store_jpeg_g12_a4"},
    {MNG_FN_STORE_JPEG_G12_A8,         "store_jpeg_g12_a8"},
    {MNG_FN_STORE_JPEG_G12_A16,        "store_jpeg_g12_a16"},

    {MNG_FN_STORE_JPEG_RGB12_A1,       "store_jpeg_rgb12_a1"},
    {MNG_FN_STORE_JPEG_RGB12_A2,       "store_jpeg_rgb12_a2"},
    {MNG_FN_STORE_JPEG_RGB12_A4,       "store_jpeg_rgb12_a4"},
    {MNG_FN_STORE_JPEG_RGB12_A8,       "store_jpeg_rgb12_a8"},
    {MNG_FN_STORE_JPEG_RGB12_A16,      "store_jpeg_rgb12_a16"},

    {MNG_FN_NEXT_JPEG_ALPHAROW,        "next_jpeg_alpharow"},
    {MNG_FN_NEXT_JPEG_ROW,             "next_jpeg_row"},
    {MNG_FN_DISPLAY_JPEG_ROWS,         "display_jpeg_rows"},

    {MNG_FN_MAGNIFY_G8_X1,             "magnify_g8_x1"},
    {MNG_FN_MAGNIFY_G8_X2,             "magnify_g8_x2"},
    {MNG_FN_MAGNIFY_RGB8_X1,           "magnify_rgb8_x1"},
    {MNG_FN_MAGNIFY_RGB8_X2,           "magnify_rgb8_x2"},
    {MNG_FN_MAGNIFY_GA8_X1,            "magnify_ga8_x1"},
    {MNG_FN_MAGNIFY_GA8_X2,            "magnify_ga8_x2"},
    {MNG_FN_MAGNIFY_GA8_X3,            "magnify_ga8_x3"},
    {MNG_FN_MAGNIFY_GA8_X4,            "magnify_ga8_x4"},
    {MNG_FN_MAGNIFY_RGBA8_X1,          "magnify_rgba8_x1"},
    {MNG_FN_MAGNIFY_RGBA8_X2,          "magnify_rgba8_x2"},
    {MNG_FN_MAGNIFY_RGBA8_X3,          "magnify_rgba8_x3"},
    {MNG_FN_MAGNIFY_RGBA8_X4,          "magnify_rgba8_x4"},
    {MNG_FN_MAGNIFY_G8_X3,             "magnify_g8_x3"},
    {MNG_FN_MAGNIFY_RGB8_X3,           "magnify_rgb8_x3"},
    {MNG_FN_MAGNIFY_GA8_X5,            "magnify_ga8_x5"},
    {MNG_FN_MAGNIFY_RGBA8_X5,          "magnify_rgba8_x5"},

    {MNG_FN_MAGNIFY_G8_Y1,             "magnify_g8_y1"},
    {MNG_FN_MAGNIFY_G8_Y2,             "magnify_g8_y2"},
    {MNG_FN_MAGNIFY_RGB8_Y1,           "magnify_rgb8_y1"},
    {MNG_FN_MAGNIFY_RGB8_Y2,           "magnify_rgb8_y2"},
    {MNG_FN_MAGNIFY_GA8_Y1,            "magnify_ga8_y1"},
    {MNG_FN_MAGNIFY_GA8_Y2,            "magnify_ga8_y2"},
    {MNG_FN_MAGNIFY_GA8_Y3,            "magnify_ga8_y3"},
    {MNG_FN_MAGNIFY_GA8_Y4,            "magnify_ga8_y4"},
    {MNG_FN_MAGNIFY_RGBA8_Y1,          "magnify_rgba8_y1"},
    {MNG_FN_MAGNIFY_RGBA8_Y2,          "magnify_rgba8_y2"},
    {MNG_FN_MAGNIFY_RGBA8_Y3,          "magnify_rgba8_y3"},
    {MNG_FN_MAGNIFY_RGBA8_Y4,          "magnify_rgba8_y4"},
    {MNG_FN_MAGNIFY_G8_Y3,             "magnify_g8_y3"},
    {MNG_FN_MAGNIFY_RGB8_Y3,           "magnify_rgb8_y3"},
    {MNG_FN_MAGNIFY_GA8_Y5,            "magnify_ga8_y5"},
    {MNG_FN_MAGNIFY_RGBA8_Y5,          "magnify_rgba8_y5"},

    {MNG_FN_MAGNIFY_G8_X1,             "magnify_g8_x1"},
    {MNG_FN_MAGNIFY_G8_X2,             "magnify_g8_x2"},
    {MNG_FN_MAGNIFY_RGB8_X1,           "magnify_rgb8_x1"},
    {MNG_FN_MAGNIFY_RGB8_X2,           "magnify_rgb8_x2"},
    {MNG_FN_MAGNIFY_GA8_X1,            "magnify_ga8_x1"},
    {MNG_FN_MAGNIFY_GA8_X2,            "magnify_ga8_x2"},
    {MNG_FN_MAGNIFY_GA8_X3,            "magnify_ga8_x3"},
    {MNG_FN_MAGNIFY_GA8_X4,            "magnify_ga8_x4"},
    {MNG_FN_MAGNIFY_RGBA8_X1,          "magnify_rgba8_x1"},
    {MNG_FN_MAGNIFY_RGBA8_X2,          "magnify_rgba8_x2"},
    {MNG_FN_MAGNIFY_RGBA8_X3,          "magnify_rgba8_x3"},
    {MNG_FN_MAGNIFY_RGBA8_X4,          "magnify_rgba8_x4"},
    {MNG_FN_MAGNIFY_G8_X3,             "magnify_g8_x3"},
    {MNG_FN_MAGNIFY_RGB8_X3,           "magnify_rgb8_x3"},
    {MNG_FN_MAGNIFY_GA8_X5,            "magnify_ga8_x5"},
    {MNG_FN_MAGNIFY_RGBA8_X5,          "magnify_rgba8_x5"},

    {MNG_FN_MAGNIFY_G8_Y1,             "magnify_g8_y1"},
    {MNG_FN_MAGNIFY_G8_Y2,             "magnify_g8_y2"},
    {MNG_FN_MAGNIFY_RGB8_Y1,           "magnify_rgb8_y1"},
    {MNG_FN_MAGNIFY_RGB8_Y2,           "magnify_rgb8_y2"},
    {MNG_FN_MAGNIFY_GA8_Y1,            "magnify_ga8_y1"},
    {MNG_FN_MAGNIFY_GA8_Y2,            "magnify_ga8_y2"},
    {MNG_FN_MAGNIFY_GA8_Y3,            "magnify_ga8_y3"},
    {MNG_FN_MAGNIFY_GA8_Y4,            "magnify_ga8_y4"},
    {MNG_FN_MAGNIFY_RGBA8_Y1,          "magnify_rgba8_y1"},
    {MNG_FN_MAGNIFY_RGBA8_Y2,          "magnify_rgba8_y2"},
    {MNG_FN_MAGNIFY_RGBA8_Y3,          "magnify_rgba8_y3"},
    {MNG_FN_MAGNIFY_RGBA8_Y4,          "magnify_rgba8_y4"},
    {MNG_FN_MAGNIFY_G8_Y3,             "magnify_g8_y3"},
    {MNG_FN_MAGNIFY_RGB8_Y3,           "magnify_rgb8_y3"},
    {MNG_FN_MAGNIFY_GA8_Y5,            "magnify_ga8_y5"},
    {MNG_FN_MAGNIFY_RGBA8_Y5,          "magnify_rgba8_y5"},

    {MNG_FN_DELTA_G1_G1,               "delta_g1_g1"},
    {MNG_FN_DELTA_G2_G2,               "delta_g2_g2"},
    {MNG_FN_DELTA_G4_G4,               "delta_g4_g4"},
    {MNG_FN_DELTA_G8_G8,               "delta_g8_g8"},
    {MNG_FN_DELTA_G16_G16,             "delta_g16_g16"},
    {MNG_FN_DELTA_RGB8_RGB8,           "delta_rgb8_rgb8"},
    {MNG_FN_DELTA_RGB16_RGB16,         "delta_rgb16_rgb16"},
    {MNG_FN_DELTA_GA8_GA8,             "delta_ga8_ga8"},
    {MNG_FN_DELTA_GA8_G8,              "delta_ga8_g8"},
    {MNG_FN_DELTA_GA8_A8,              "delta_ga8_a8"},
    {MNG_FN_DELTA_GA16_GA16,           "delta_ga16_ga16"},
    {MNG_FN_DELTA_GA16_G16,            "delta_ga16_g16"},
    {MNG_FN_DELTA_GA16_A16,            "delta_ga16_a16"},
    {MNG_FN_DELTA_RGBA8_RGBA8,         "delta_rgba8_rgba8"},
    {MNG_FN_DELTA_RGBA8_RGB8,          "delta_rgba8_rgb8"},
    {MNG_FN_DELTA_RGBA8_A8,            "delta_rgba8_a8"},
    {MNG_FN_DELTA_RGBA16_RGBA16,       "delta_rgba16_rgba16"},
    {MNG_FN_DELTA_RGBA16_RGB16,        "delta_rgba16_rgb16"},
    {MNG_FN_DELTA_RGBA16_A16,          "delta_rgba16_a16"},

    {MNG_FN_PROMOTE_G8_G8,             "promote_g8_g8"},
    {MNG_FN_PROMOTE_G8_G16,            "promote_g8_g16"},
    {MNG_FN_PROMOTE_G16_G16,           "promote_g8_g16"},
    {MNG_FN_PROMOTE_G8_GA8,            "promote_g8_ga8"},
    {MNG_FN_PROMOTE_G8_GA16,           "promote_g8_ga16"},
    {MNG_FN_PROMOTE_G16_GA16,          "promote_g16_ga16"},
    {MNG_FN_PROMOTE_G8_RGB8,           "promote_g8_rgb8"},
    {MNG_FN_PROMOTE_G8_RGB16,          "promote_g8_rgb16"},
    {MNG_FN_PROMOTE_G16_RGB16,         "promote_g16_rgb16"},
    {MNG_FN_PROMOTE_G8_RGBA8,          "promote_g8_rgba8"},
    {MNG_FN_PROMOTE_G8_RGBA16,         "promote_g8_rgba16"},
    {MNG_FN_PROMOTE_G16_RGBA16,        "promote_g16_rgba16"},
    {MNG_FN_PROMOTE_GA8_GA16,          "promote_ga8_ga16"},
    {MNG_FN_PROMOTE_GA8_RGBA8,         "promote_ga8_rgba8"},
    {MNG_FN_PROMOTE_GA8_RGBA16,        "promote_ga8_rgba16"},
    {MNG_FN_PROMOTE_GA16_RGBA16,       "promote_ga16_rgba16"},
    {MNG_FN_PROMOTE_RGB8_RGB16,        "promote_rgb8_rgb16"},
    {MNG_FN_PROMOTE_RGB8_RGBA8,        "promote_rgb8_rgba8"},
    {MNG_FN_PROMOTE_RGB8_RGBA16,       "promote_rgb8_rgba16"},
    {MNG_FN_PROMOTE_RGB16_RGBA16,      "promote_rgb16_rgba16"},
    {MNG_FN_PROMOTE_RGBA8_RGBA16,      "promote_rgba8_rgba16"},
    {MNG_FN_PROMOTE_IDX8_RGB8,         "promote_idx8_rgb8"},
    {MNG_FN_PROMOTE_IDX8_RGB16,        "promote_idx8_rgb16"},
    {MNG_FN_PROMOTE_IDX8_RGBA8,        "promote_idx8_rgba8"},
    {MNG_FN_PROMOTE_IDX8_RGBA16,       "promote_idx8_rgba16"},

    {MNG_FN_SCALE_G1_G2,               "scale_g1_g2"},
    {MNG_FN_SCALE_G1_G4,               "scale_g1_g4"},
    {MNG_FN_SCALE_G1_G8,               "scale_g1_g8"},
    {MNG_FN_SCALE_G1_G16,              "scale_g1_g16"},
    {MNG_FN_SCALE_G2_G4,               "scale_g2_g4"},
    {MNG_FN_SCALE_G2_G8,               "scale_g2_g8"},
    {MNG_FN_SCALE_G2_G16,              "scale_g2_g16"},
    {MNG_FN_SCALE_G4_G8,               "scale_g4_g8"},
    {MNG_FN_SCALE_G4_G16,              "scale_g4_g16"},
    {MNG_FN_SCALE_G8_G16,              "scale_g8_g16"},
    {MNG_FN_SCALE_GA8_GA16,            "scale_ga8_ga16"},
    {MNG_FN_SCALE_RGB8_RGB16,          "scale_rgb8_rgb16"},
    {MNG_FN_SCALE_RGBA8_RGBA16,        "scale_rgba8_rgba16"},

    {MNG_FN_SCALE_G2_G1,               "scale_g2_g1"},
    {MNG_FN_SCALE_G4_G1,               "scale_g4_g1"},
    {MNG_FN_SCALE_G8_G1,               "scale_g8_g1"},
    {MNG_FN_SCALE_G16_G1,              "scale_g16_g1"},
    {MNG_FN_SCALE_G4_G2,               "scale_g4_g2"},
    {MNG_FN_SCALE_G8_G2,               "scale_g8_g2"},
    {MNG_FN_SCALE_G16_G2,              "scale_g16_g2"},
    {MNG_FN_SCALE_G8_G4,               "scale_g8_g4"},
    {MNG_FN_SCALE_G16_G4,              "scale_g16_g4"},
    {MNG_FN_SCALE_G16_G8,              "scale_g16_g8"},
    {MNG_FN_SCALE_GA16_GA8,            "scale_ga16_ga8"},
    {MNG_FN_SCALE_RGB16_RGB8,          "scale_rgb16_rgb8"},
    {MNG_FN_SCALE_RGBA16_RGBA8,        "scale_rgba16_rgba8"},

    {MNG_FN_COMPOSEOVER_RGBA8,         "composeover_rgba8"},
    {MNG_FN_COMPOSEOVER_RGBA16,        "composeover_rgba16"},
    {MNG_FN_COMPOSEUNDER_RGBA8,        "composeunder_rgba8"},
    {MNG_FN_COMPOSEUNDER_RGBA16,       "composeunder_rgba16"},

    {MNG_FN_FLIP_RGBA8,                "flip_rgba8"},
    {MNG_FN_FLIP_RGBA16,               "flip_rgba16"},
    {MNG_FN_TILE_RGBA8,                "tile_rgba8"},
    {MNG_FN_TILE_RGBA16,               "tile_rgba16"}

  };
#endif /* MNG_INCLUDE_TRACE_STINGS */

/* ************************************************************************** */

mng_retcode mng_trace (mng_datap  pData,
                       mng_uint32 iFunction,
                       mng_uint32 iLocation)
{
  mng_pchar zName = 0;                 /* bufferptr for tracestring */

  if ((pData == 0) || (pData->iMagic != MNG_MAGIC))
    return MNG_INVALIDHANDLE;          /* no good if the handle is corrupt */

  if (pData->fTraceproc)               /* report back to user ? */
  {
#ifdef MNG_INCLUDE_TRACE_STRINGS
    {                                  /* binary search variables */
      mng_int32        iTop, iLower, iUpper, iMiddle;
      mng_trace_entryp pEntry;         /* pointer to found entry */
                                       /* determine max index of table */
      iTop = (sizeof (trace_table) / sizeof (trace_table [0])) - 1;

      iLower  = 0;                     /* initialize binary search */
      iMiddle = iTop >> 1;             /* start in the middle */
      iUpper  = iTop;
      pEntry  = 0;                     /* no goods yet! */

      do                               /* the binary search itself */
        {
          if (trace_table [iMiddle].iFunction < iFunction)
            iLower = iMiddle + 1;
          else if (trace_table [iMiddle].iFunction > iFunction)
            iUpper = iMiddle - 1;
          else
          {
            pEntry = &trace_table [iMiddle];
            break;
          };

          iMiddle = (iLower + iUpper) >> 1;
        }
      while (iLower <= iUpper);

      if (pEntry)                      /* found it ? */
        zName = pEntry->zTracetext;

    }
#endif
                                       /* oke, now tell */
    if (!pData->fTraceproc (((mng_handle)pData), iFunction, iLocation, zName))
      return MNG_APPTRACEABORT;

  }

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_INCLUDE_TRACE_PROCS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

