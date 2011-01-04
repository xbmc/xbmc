/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_trace.h            copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Trace functions (definition)                               * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of the trace functions                          * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - added chunk-access function trace-codes                  * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *             0.5.1 - 05/13/2000 - G.Juyn                                * */
/* *             - added save_state & restore_state trace-codes             * */
/* *             0.5.1 - 05/15/2000 - G.Juyn                                * */
/* *             - added getimgdata & putimgdata trace-codes                * */
/* *                                                                        * */
/* *             0.5.2 - 05/20/2000 - G.Juyn                                * */
/* *             - added JNG tracecodes                                     * */
/* *             0.5.2 - 05/23/2000 - G.Juyn                                * */
/* *             - added trace-table entry definition                       * */
/* *             0.5.2 - 05/24/2000 - G.Juyn                                * */
/* *             - added tracecodes for global animation color-chunks       * */
/* *             - added tracecodes for get/set of default ZLIB/IJG parms   * */
/* *             - added tracecodes for global PLTE,tRNS,bKGD               * */
/* *             0.5.2 - 05/30/2000 - G.Juyn                                * */
/* *             - added tracecodes for image-object promotion              * */
/* *             - added tracecodes for delta-image processing              * */
/* *             0.5.2 - 06/02/2000 - G.Juyn                                * */
/* *             - added tracecodes for getalphaline callback               * */
/* *             0.5.2 - 06/05/2000 - G.Juyn                                * */
/* *             - added tracecode for RGB8_A8 canvasstyle                  * */
/* *             0.5.2 - 06/06/2000 - G.Juyn                                * */
/* *             - added tracecode for mng_read_resume HLAPI function       * */
/* *                                                                        * */
/* *             0.5.3 - 06/06/2000 - G.Juyn                                * */
/* *             - added tracecodes for tracing JPEG progression            * */
/* *             0.5.3 - 06/21/2000 - G.Juyn                                * */
/* *             - added tracecodes for get/set speedtype                   * */
/* *             - added tracecodes for get imagelevel                      * */
/* *             0.5.3 - 06/22/2000 - G.Juyn                                * */
/* *             - added tracecode for delta-image processing               * */
/* *             - added tracecodes for PPLT chunk processing               * */
/* *                                                                        * */
/* *             0.9.1 - 07/07/2000 - G.Juyn                                * */
/* *             - added tracecodes for special display processing          * */
/* *             0.9.1 - 07/08/2000 - G.Juyn                                * */
/* *             - added tracecode for get/set suspensionmode               * */
/* *             - added tracecodes for get/set display variables           * */
/* *             - added tracecode for read_databuffer (I/O-suspension)     * */
/* *             0.9.1 - 07/15/2000 - G.Juyn                                * */
/* *             - added tracecodes for SAVE/SEEK callbacks                 * */
/* *             - added tracecodes for get/set sectionbreaks               * */
/* *             - added tracecode for special error routine                * */
/* *             0.9.1 - 07/19/2000 - G.Juyn                                * */
/* *             - added tracecode for updatemngheader                      * */
/* *                                                                        * */
/* *             0.9.2 - 07/31/2000 - G.Juyn                                * */
/* *             - added tracecodes for status_xxxxx functions              * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *             - added tracecode for updatemngsimplicity                  * */
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
/* *             1.0.6 - 07/14/2003 - G.Randers-Pehrson                     * */
/* *             - added conditionals around rarely used features           * */
/* *                                                                        * */
/* *             1.0.7 - 11/27/2003 - R.A                                   * */
/* *             - added CANVAS_RGB565 and CANVAS_BGR565                    * */
/* *             1.0.7 - 01/25/2004 - J.S                                   * */
/* *             - added premultiplied alpha canvas' for RGBA, ARGB, ABGR   * */
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

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

#ifndef _libmng_trace_h_
#define _libmng_trace_h_

/* ************************************************************************** */

#ifdef MNG_INCLUDE_TRACE_PROCS

/* ************************************************************************** */

/* TODO: add a trace-mask so certain functions can be excluded */

mng_retcode mng_trace (mng_datap  pData,
                       mng_uint32 iFunction,
                       mng_uint32 iLocation);

/* ************************************************************************** */

#define MNG_TRACE(D,F,L)  { mng_retcode iR = mng_trace (D,F,L); \
                            if (iR) return iR; }

#define MNG_TRACEB(D,F,L) { if (mng_trace (D,F,L)) return MNG_FALSE; }

#define MNG_TRACEX(D,F,L) { if (mng_trace (D,F,L)) return 0; }

/* ************************************************************************** */

#define MNG_LC_START                    1
#define MNG_LC_END                      2
#define MNG_LC_INITIALIZE               3
#define MNG_LC_CLEANUP                  4

/* ************************************************************************** */

#define MNG_LC_JPEG_CREATE_DECOMPRESS   101
#define MNG_LC_JPEG_READ_HEADER         102
#define MNG_LC_JPEG_START_DECOMPRESS    103
#define MNG_LC_JPEG_START_OUTPUT        104
#define MNG_LC_JPEG_READ_SCANLINES      105
#define MNG_LC_JPEG_FINISH_OUTPUT       106
#define MNG_LC_JPEG_FINISH_DECOMPRESS   107
#define MNG_LC_JPEG_DESTROY_DECOMPRESS  108

/* ************************************************************************** */

#define MNG_FN_INITIALIZE               1
#define MNG_FN_RESET                    2
#define MNG_FN_CLEANUP                  3
#define MNG_FN_READ                     4
#define MNG_FN_WRITE                    5
#define MNG_FN_CREATE                   6
#define MNG_FN_READDISPLAY              7
#define MNG_FN_DISPLAY                  8
#define MNG_FN_DISPLAY_RESUME           9
#define MNG_FN_DISPLAY_FREEZE          10
#define MNG_FN_DISPLAY_RESET           11
#ifndef MNG_NO_DISPLAY_GO_SUPPORTED
#define MNG_FN_DISPLAY_GOFRAME         12
#define MNG_FN_DISPLAY_GOLAYER         13
#define MNG_FN_DISPLAY_GOTIME          14
#endif
#define MNG_FN_GETLASTERROR            15
#define MNG_FN_READ_RESUME             16
#define MNG_FN_TRAPEVENT               17
#define MNG_FN_READ_PUSHDATA           18
#define MNG_FN_READ_PUSHSIG            19
#define MNG_FN_READ_PUSHCHUNK          20

#define MNG_FN_SETCB_MEMALLOC         101
#define MNG_FN_SETCB_MEMFREE          102
#define MNG_FN_SETCB_READDATA         103
#define MNG_FN_SETCB_WRITEDATA        104
#define MNG_FN_SETCB_ERRORPROC        105
#define MNG_FN_SETCB_TRACEPROC        106
#define MNG_FN_SETCB_PROCESSHEADER    107
#define MNG_FN_SETCB_PROCESSTEXT      108
#define MNG_FN_SETCB_GETCANVASLINE    109
#define MNG_FN_SETCB_GETBKGDLINE      110
#define MNG_FN_SETCB_REFRESH          111
#define MNG_FN_SETCB_GETTICKCOUNT     112
#define MNG_FN_SETCB_SETTIMER         113
#define MNG_FN_SETCB_PROCESSGAMMA     114
#define MNG_FN_SETCB_PROCESSCHROMA    115
#define MNG_FN_SETCB_PROCESSSRGB      116
#define MNG_FN_SETCB_PROCESSICCP      117
#define MNG_FN_SETCB_PROCESSAROW      118
#ifndef MNG_NO_OPEN_CLOSE_STREAM
#define MNG_FN_SETCB_OPENSTREAM       119
#define MNG_FN_SETCB_CLOSESTREAM      120
#endif
#define MNG_FN_SETCB_GETALPHALINE     121
#define MNG_FN_SETCB_PROCESSSAVE      122
#define MNG_FN_SETCB_PROCESSSEEK      123
#define MNG_FN_SETCB_PROCESSNEED      124
#define MNG_FN_SETCB_PROCESSUNKNOWN   125
#define MNG_FN_SETCB_PROCESSMEND      126
#define MNG_FN_SETCB_PROCESSTERM      127
#define MNG_FN_SETCB_RELEASEDATA      128

#define MNG_FN_GETCB_MEMALLOC         201
#define MNG_FN_GETCB_MEMFREE          202
#define MNG_FN_GETCB_READDATA         203
#define MNG_FN_GETCB_WRITEDATA        204
#define MNG_FN_GETCB_ERRORPROC        205
#define MNG_FN_GETCB_TRACEPROC        206
#define MNG_FN_GETCB_PROCESSHEADER    207
#define MNG_FN_GETCB_PROCESSTEXT      208
#define MNG_FN_GETCB_GETCANVASLINE    209
#define MNG_FN_GETCB_GETBKGDLINE      210
#define MNG_FN_GETCB_REFRESH          211
#define MNG_FN_GETCB_GETTICKCOUNT     212
#define MNG_FN_GETCB_SETTIMER         213
#define MNG_FN_GETCB_PROCESSGAMMA     214
#define MNG_FN_GETCB_PROCESSCHROMA    215
#define MNG_FN_GETCB_PROCESSSRGB      216
#define MNG_FN_GETCB_PROCESSICCP      217
#define MNG_FN_GETCB_PROCESSAROW      218
#ifndef MNG_NO_OPEN_CLOSE_STREAM
#define MNG_FN_GETCB_OPENSTREAM       219
#define MNG_FN_GETCB_CLOSESTREAM      220
#endif
#define MNG_FN_GETCB_GETALPHALINE     221
#define MNG_FN_GETCB_PROCESSSAVE      222
#define MNG_FN_GETCB_PROCESSSEEK      223
#define MNG_FN_GETCB_PROCESSNEED      224
#define MNG_FN_GETCB_PROCESSUNKNOWN   225
#define MNG_FN_GETCB_PROCESSMEND      226
#define MNG_FN_GETCB_PROCESSTERM      227
#define MNG_FN_GETCB_RELEASEDATA      228

#define MNG_FN_SET_USERDATA           301
#define MNG_FN_SET_CANVASSTYLE        302
#define MNG_FN_SET_BKGDSTYLE          303
#define MNG_FN_SET_BGCOLOR            304
#define MNG_FN_SET_STORECHUNKS        305
#define MNG_FN_SET_VIEWGAMMA          306
#define MNG_FN_SET_DISPLAYGAMMA       307
#define MNG_FN_SET_DFLTIMGGAMMA       308
#define MNG_FN_SET_SRGB               309
#define MNG_FN_SET_OUTPUTPROFILE      310
#define MNG_FN_SET_SRGBPROFILE        311
#define MNG_FN_SET_MAXCANVASWIDTH     312
#define MNG_FN_SET_MAXCANVASHEIGHT    313
#define MNG_FN_SET_MAXCANVASSIZE      314
#define MNG_FN_SET_ZLIB_LEVEL         315
#define MNG_FN_SET_ZLIB_METHOD        316
#define MNG_FN_SET_ZLIB_WINDOWBITS    317
#define MNG_FN_SET_ZLIB_MEMLEVEL      318
#define MNG_FN_SET_ZLIB_STRATEGY      319
#define MNG_FN_SET_ZLIB_MAXIDAT       320
#define MNG_FN_SET_JPEG_DCTMETHOD     321
#define MNG_FN_SET_JPEG_QUALITY       322
#define MNG_FN_SET_JPEG_SMOOTHING     323
#define MNG_FN_SET_JPEG_PROGRESSIVE   324
#define MNG_FN_SET_JPEG_OPTIMIZED     325
#define MNG_FN_SET_JPEG_MAXJDAT       326
#define MNG_FN_SET_SPEED              327
#define MNG_FN_SET_SUSPENSIONMODE     328
#define MNG_FN_SET_SECTIONBREAKS      329
#define MNG_FN_SET_USEBKGD            330
#define MNG_FN_SET_OUTPUTPROFILE2     331
#define MNG_FN_SET_SRGBPROFILE2       332
#define MNG_FN_SET_OUTPUTSRGB         333
#define MNG_FN_SET_SRGBIMPLICIT       334
#define MNG_FN_SET_CACHEPLAYBACK      335
#define MNG_FN_SET_DOPROGRESSIVE      336
#define MNG_FN_SET_CRCMODE            337

#define MNG_FN_GET_USERDATA           401
#define MNG_FN_GET_SIGTYPE            402
#define MNG_FN_GET_IMAGETYPE          403
#define MNG_FN_GET_IMAGEWIDTH         404
#define MNG_FN_GET_IMAGEHEIGHT        405
#define MNG_FN_GET_TICKS              406
#define MNG_FN_GET_FRAMECOUNT         407
#define MNG_FN_GET_LAYERCOUNT         408
#define MNG_FN_GET_PLAYTIME           409
#define MNG_FN_GET_SIMPLICITY         410
#define MNG_FN_GET_CANVASSTYLE        411
#define MNG_FN_GET_BKGDSTYLE          412
#define MNG_FN_GET_BGCOLOR            413
#define MNG_FN_GET_STORECHUNKS        414
#define MNG_FN_GET_VIEWGAMMA          415
#define MNG_FN_GET_DISPLAYGAMMA       416
#define MNG_FN_GET_DFLTIMGGAMMA       417
#define MNG_FN_GET_SRGB               418
#define MNG_FN_GET_MAXCANVASWIDTH     419
#define MNG_FN_GET_MAXCANVASHEIGHT    420
#define MNG_FN_GET_ZLIB_LEVEL         421
#define MNG_FN_GET_ZLIB_METHOD        422
#define MNG_FN_GET_ZLIB_WINDOWBITS    423
#define MNG_FN_GET_ZLIB_MEMLEVEL      424
#define MNG_FN_GET_ZLIB_STRATEGY      425
#define MNG_FN_GET_ZLIB_MAXIDAT       426
#define MNG_FN_GET_JPEG_DCTMETHOD     427
#define MNG_FN_GET_JPEG_QUALITY       428
#define MNG_FN_GET_JPEG_SMOOTHING     429
#define MNG_FN_GET_JPEG_PROGRESSIVE   430
#define MNG_FN_GET_JPEG_OPTIMIZED     431
#define MNG_FN_GET_JPEG_MAXJDAT       432
#define MNG_FN_GET_SPEED              433
#define MNG_FN_GET_IMAGELEVEL         434
#define MNG_FN_GET_SUSPENSIONMODE     435
#define MNG_FN_GET_STARTTIME          436
#define MNG_FN_GET_RUNTIME            437
#define MNG_FN_GET_CURRENTFRAME       438
#define MNG_FN_GET_CURRENTLAYER       439
#define MNG_FN_GET_CURRENTPLAYTIME    440
#define MNG_FN_GET_SECTIONBREAKS      441
#define MNG_FN_GET_ALPHADEPTH         442
#define MNG_FN_GET_BITDEPTH           443
#define MNG_FN_GET_COLORTYPE          444
#define MNG_FN_GET_COMPRESSION        445
#define MNG_FN_GET_FILTER             446
#define MNG_FN_GET_INTERLACE          447
#define MNG_FN_GET_ALPHABITDEPTH      448
#define MNG_FN_GET_ALPHACOMPRESSION   449
#define MNG_FN_GET_ALPHAFILTER        450
#define MNG_FN_GET_ALPHAINTERLACE     451
#define MNG_FN_GET_USEBKGD            452
#define MNG_FN_GET_REFRESHPASS        453
#define MNG_FN_GET_CACHEPLAYBACK      454
#define MNG_FN_GET_DOPROGRESSIVE      455
#define MNG_FN_GET_LASTBACKCHUNK      456
#define MNG_FN_GET_LASTSEEKNAME       457
#define MNG_FN_GET_TOTALFRAMES        458
#define MNG_FN_GET_TOTALLAYERS        459
#define MNG_FN_GET_TOTALPLAYTIME      460
#define MNG_FN_GET_CRCMODE            461
#define MNG_FN_GET_CURRFRAMDELAY      462

#define MNG_FN_STATUS_ERROR           481
#define MNG_FN_STATUS_READING         482
#define MNG_FN_STATUS_SUSPENDBREAK    483
#define MNG_FN_STATUS_CREATING        484
#define MNG_FN_STATUS_WRITING         485
#define MNG_FN_STATUS_DISPLAYING      486
#define MNG_FN_STATUS_RUNNING         487
#define MNG_FN_STATUS_TIMERBREAK      488
#define MNG_FN_STATUS_DYNAMIC         489
#define MNG_FN_STATUS_RUNNINGEVENT    490

/* ************************************************************************** */

#define MNG_FN_ITERATE_CHUNKS         601
#define MNG_FN_COPY_CHUNK             602

#define MNG_FN_GETCHUNK_IHDR          701
#define MNG_FN_GETCHUNK_PLTE          702
#define MNG_FN_GETCHUNK_IDAT          703
#define MNG_FN_GETCHUNK_IEND          704
#define MNG_FN_GETCHUNK_TRNS          705
#define MNG_FN_GETCHUNK_GAMA          706
#define MNG_FN_GETCHUNK_CHRM          707
#define MNG_FN_GETCHUNK_SRGB          708
#define MNG_FN_GETCHUNK_ICCP          709
#define MNG_FN_GETCHUNK_TEXT          710
#define MNG_FN_GETCHUNK_ZTXT          711
#define MNG_FN_GETCHUNK_ITXT          712
#define MNG_FN_GETCHUNK_BKGD          713
#define MNG_FN_GETCHUNK_PHYS          714
#define MNG_FN_GETCHUNK_SBIT          715
#define MNG_FN_GETCHUNK_SPLT          716
#define MNG_FN_GETCHUNK_HIST          717
#define MNG_FN_GETCHUNK_TIME          718
#define MNG_FN_GETCHUNK_MHDR          719
#define MNG_FN_GETCHUNK_MEND          720
#define MNG_FN_GETCHUNK_LOOP          721
#define MNG_FN_GETCHUNK_ENDL          722
#define MNG_FN_GETCHUNK_DEFI          723
#define MNG_FN_GETCHUNK_BASI          724
#define MNG_FN_GETCHUNK_CLON          725
#define MNG_FN_GETCHUNK_PAST          726
#define MNG_FN_GETCHUNK_DISC          727
#define MNG_FN_GETCHUNK_BACK          728
#define MNG_FN_GETCHUNK_FRAM          729
#define MNG_FN_GETCHUNK_MOVE          730
#define MNG_FN_GETCHUNK_CLIP          731
#define MNG_FN_GETCHUNK_SHOW          732
#define MNG_FN_GETCHUNK_TERM          733
#define MNG_FN_GETCHUNK_SAVE          734
#define MNG_FN_GETCHUNK_SEEK          735
#define MNG_FN_GETCHUNK_EXPI          736
#define MNG_FN_GETCHUNK_FPRI          737
#define MNG_FN_GETCHUNK_NEED          738
#define MNG_FN_GETCHUNK_PHYG          739
#define MNG_FN_GETCHUNK_JHDR          740
#define MNG_FN_GETCHUNK_JDAT          741
#define MNG_FN_GETCHUNK_JSEP          742
#define MNG_FN_GETCHUNK_DHDR          743
#define MNG_FN_GETCHUNK_PROM          744
#define MNG_FN_GETCHUNK_IPNG          745
#define MNG_FN_GETCHUNK_PPLT          746
#define MNG_FN_GETCHUNK_IJNG          747
#define MNG_FN_GETCHUNK_DROP          748
#define MNG_FN_GETCHUNK_DBYK          749
#define MNG_FN_GETCHUNK_ORDR          750
#define MNG_FN_GETCHUNK_UNKNOWN       751
#define MNG_FN_GETCHUNK_MAGN          752
#define MNG_FN_GETCHUNK_JDAA          753
#define MNG_FN_GETCHUNK_EVNT          754
#define MNG_FN_GETCHUNK_MPNG          755

#define MNG_FN_GETCHUNK_PAST_SRC      781
#define MNG_FN_GETCHUNK_SAVE_ENTRY    782
#define MNG_FN_GETCHUNK_PPLT_ENTRY    783
#define MNG_FN_GETCHUNK_ORDR_ENTRY    784
#define MNG_FN_GETCHUNK_EVNT_ENTRY    785
#define MNG_FN_GETCHUNK_MPNG_FRAME    786

#define MNG_FN_PUTCHUNK_IHDR          801
#define MNG_FN_PUTCHUNK_PLTE          802
#define MNG_FN_PUTCHUNK_IDAT          803
#define MNG_FN_PUTCHUNK_IEND          804
#define MNG_FN_PUTCHUNK_TRNS          805
#define MNG_FN_PUTCHUNK_GAMA          806
#define MNG_FN_PUTCHUNK_CHRM          807
#define MNG_FN_PUTCHUNK_SRGB          808
#define MNG_FN_PUTCHUNK_ICCP          809
#define MNG_FN_PUTCHUNK_TEXT          810
#define MNG_FN_PUTCHUNK_ZTXT          811
#define MNG_FN_PUTCHUNK_ITXT          812
#define MNG_FN_PUTCHUNK_BKGD          813
#define MNG_FN_PUTCHUNK_PHYS          814
#define MNG_FN_PUTCHUNK_SBIT          815
#define MNG_FN_PUTCHUNK_SPLT          816
#define MNG_FN_PUTCHUNK_HIST          817
#define MNG_FN_PUTCHUNK_TIME          818
#define MNG_FN_PUTCHUNK_MHDR          819
#define MNG_FN_PUTCHUNK_MEND          820
#define MNG_FN_PUTCHUNK_LOOP          821
#define MNG_FN_PUTCHUNK_ENDL          822
#define MNG_FN_PUTCHUNK_DEFI          823
#define MNG_FN_PUTCHUNK_BASI          824
#define MNG_FN_PUTCHUNK_CLON          825
#define MNG_FN_PUTCHUNK_PAST          826
#define MNG_FN_PUTCHUNK_DISC          827
#define MNG_FN_PUTCHUNK_BACK          828
#define MNG_FN_PUTCHUNK_FRAM          829
#define MNG_FN_PUTCHUNK_MOVE          830
#define MNG_FN_PUTCHUNK_CLIP          831
#define MNG_FN_PUTCHUNK_SHOW          832
#define MNG_FN_PUTCHUNK_TERM          833
#define MNG_FN_PUTCHUNK_SAVE          834
#define MNG_FN_PUTCHUNK_SEEK          835
#define MNG_FN_PUTCHUNK_EXPI          836
#define MNG_FN_PUTCHUNK_FPRI          837
#define MNG_FN_PUTCHUNK_NEED          838
#define MNG_FN_PUTCHUNK_PHYG          839
#define MNG_FN_PUTCHUNK_JHDR          840
#define MNG_FN_PUTCHUNK_JDAT          841
#define MNG_FN_PUTCHUNK_JSEP          842
#define MNG_FN_PUTCHUNK_DHDR          843
#define MNG_FN_PUTCHUNK_PROM          844
#define MNG_FN_PUTCHUNK_IPNG          845
#define MNG_FN_PUTCHUNK_PPLT          846
#define MNG_FN_PUTCHUNK_IJNG          847
#define MNG_FN_PUTCHUNK_DROP          848
#define MNG_FN_PUTCHUNK_DBYK          849
#define MNG_FN_PUTCHUNK_ORDR          850
#define MNG_FN_PUTCHUNK_UNKNOWN       851
#define MNG_FN_PUTCHUNK_MAGN          852
#define MNG_FN_PUTCHUNK_JDAA          853
#define MNG_FN_PUTCHUNK_EVNT          854
#define MNG_FN_PUTCHUNK_MPNG          855

#define MNG_FN_PUTCHUNK_PAST_SRC      881
#define MNG_FN_PUTCHUNK_SAVE_ENTRY    882
#define MNG_FN_PUTCHUNK_PPLT_ENTRY    883
#define MNG_FN_PUTCHUNK_ORDR_ENTRY    884
#define MNG_FN_PUTCHUNK_EVNT_ENTRY    885
#define MNG_FN_PUTCHUNK_MPNG_FRAME    886

/* ************************************************************************** */

#define MNG_FN_GETIMGDATA_SEQ         901
#define MNG_FN_GETIMGDATA_CHUNKSEQ    902
#define MNG_FN_GETIMGDATA_CHUNK       903

#define MNG_FN_PUTIMGDATA_IHDR        951
#define MNG_FN_PUTIMGDATA_JHDR        952
#define MNG_FN_PUTIMGDATA_BASI        953
#define MNG_FN_PUTIMGDATA_DHDR        954

#define MNG_FN_UPDATEMNGHEADER        981
#define MNG_FN_UPDATEMNGSIMPLICITY    982

/* ************************************************************************** */

#define MNG_FN_PROCESS_RAW_CHUNK     1001
#define MNG_FN_READ_GRAPHIC          1002
#define MNG_FN_DROP_CHUNKS           1003
#define MNG_FN_PROCESS_ERROR         1004
#define MNG_FN_CLEAR_CMS             1005
#define MNG_FN_DROP_OBJECTS          1006
#define MNG_FN_READ_CHUNK            1007
#define MNG_FN_LOAD_BKGDLAYER        1008
#define MNG_FN_NEXT_FRAME            1009
#define MNG_FN_NEXT_LAYER            1010
#define MNG_FN_INTERFRAME_DELAY      1011
#define MNG_FN_DISPLAY_IMAGE         1012
#define MNG_FN_DROP_IMGOBJECTS       1013
#define MNG_FN_DROP_ANIOBJECTS       1014
#define MNG_FN_INFLATE_BUFFER        1015
#define MNG_FN_DEFLATE_BUFFER        1016
#define MNG_FN_WRITE_RAW_CHUNK       1017
#define MNG_FN_WRITE_GRAPHIC         1018
#define MNG_FN_SAVE_STATE            1019
#define MNG_FN_RESTORE_STATE         1020
#define MNG_FN_DROP_SAVEDATA         1021
#define MNG_FN_EXECUTE_DELTA_IMAGE   1022
#define MNG_FN_PROCESS_DISPLAY       1023
#define MNG_FN_CLEAR_CANVAS          1024
#define MNG_FN_READ_DATABUFFER       1025
#define MNG_FN_STORE_ERROR           1026
#define MNG_FN_DROP_INVALID_OBJECTS  1027
#define MNG_FN_RELEASE_PUSHDATA      1028
#define MNG_FN_READ_DATA             1029
#define MNG_FN_READ_CHUNK_CRC        1030
#define MNG_FN_RELEASE_PUSHCHUNK     1031

/* ************************************************************************** */

#define MNG_FN_DISPLAY_RGB8          1101
#define MNG_FN_DISPLAY_RGBA8         1102
#define MNG_FN_DISPLAY_ARGB8         1103
#define MNG_FN_DISPLAY_BGR8          1104
#define MNG_FN_DISPLAY_BGRA8         1105
#define MNG_FN_DISPLAY_ABGR8         1106
#define MNG_FN_DISPLAY_RGB16         1107
#define MNG_FN_DISPLAY_RGBA16        1108
#define MNG_FN_DISPLAY_ARGB16        1109
#define MNG_FN_DISPLAY_BGR16         1110
#define MNG_FN_DISPLAY_BGRA16        1111
#define MNG_FN_DISPLAY_ABGR16        1112
#define MNG_FN_DISPLAY_INDEX8        1113
#define MNG_FN_DISPLAY_INDEXA8       1114
#define MNG_FN_DISPLAY_AINDEX8       1115
#define MNG_FN_DISPLAY_GRAY8         1116
#define MNG_FN_DISPLAY_GRAY16        1117
#define MNG_FN_DISPLAY_GRAYA8        1118
#define MNG_FN_DISPLAY_GRAYA16       1119
#define MNG_FN_DISPLAY_AGRAY8        1120
#define MNG_FN_DISPLAY_AGRAY16       1121
#define MNG_FN_DISPLAY_DX15          1122
#define MNG_FN_DISPLAY_DX16          1123
#define MNG_FN_DISPLAY_RGB8_A8       1124
#define MNG_FN_DISPLAY_BGRA8PM       1125
#define MNG_FN_DISPLAY_BGRX8         1126
#define MNG_FN_DISPLAY_RGB565        1127
#define MNG_FN_DISPLAY_RGBA565       1128
#define MNG_FN_DISPLAY_BGR565        1129
#define MNG_FN_DISPLAY_BGRA565       1130
#define MNG_FN_DISPLAY_RGBA8_PM      1131
#define MNG_FN_DISPLAY_ARGB8_PM      1132
#define MNG_FN_DISPLAY_ABGR8_PM      1133
#define MNG_FN_DISPLAY_BGR565_A8     1134
#define MNG_FN_DISPLAY_RGB555        1135
#define MNG_FN_DISPLAY_BGR555        1136

/* ************************************************************************** */

#define MNG_FN_INIT_FULL_CMS         1201
#define MNG_FN_CORRECT_FULL_CMS      1202
#define MNG_FN_INIT_GAMMA_ONLY       1204
#define MNG_FN_CORRECT_GAMMA_ONLY    1205
#define MNG_FN_CORRECT_APP_CMS       1206
#define MNG_FN_INIT_FULL_CMS_OBJ     1207
#define MNG_FN_INIT_GAMMA_ONLY_OBJ   1208
#define MNG_FN_INIT_APP_CMS          1209
#define MNG_FN_INIT_APP_CMS_OBJ      1210

/* ************************************************************************** */

#define MNG_FN_PROCESS_G1            1301
#define MNG_FN_PROCESS_G2            1302
#define MNG_FN_PROCESS_G4            1303
#define MNG_FN_PROCESS_G8            1304
#define MNG_FN_PROCESS_G16           1305
#define MNG_FN_PROCESS_RGB8          1306
#define MNG_FN_PROCESS_RGB16         1307
#define MNG_FN_PROCESS_IDX1          1308
#define MNG_FN_PROCESS_IDX2          1309
#define MNG_FN_PROCESS_IDX4          1310
#define MNG_FN_PROCESS_IDX8          1311
#define MNG_FN_PROCESS_GA8           1312
#define MNG_FN_PROCESS_GA16          1313
#define MNG_FN_PROCESS_RGBA8         1314
#define MNG_FN_PROCESS_RGBA16        1315

/* ************************************************************************** */

#define MNG_FN_INIT_G1_NI            1401
#define MNG_FN_INIT_G1_I             1402
#define MNG_FN_INIT_G2_NI            1403
#define MNG_FN_INIT_G2_I             1404
#define MNG_FN_INIT_G4_NI            1405
#define MNG_FN_INIT_G4_I             1406
#define MNG_FN_INIT_G8_NI            1407
#define MNG_FN_INIT_G8_I             1408
#define MNG_FN_INIT_G16_NI           1409
#define MNG_FN_INIT_G16_I            1410
#define MNG_FN_INIT_RGB8_NI          1411
#define MNG_FN_INIT_RGB8_I           1412
#define MNG_FN_INIT_RGB16_NI         1413
#define MNG_FN_INIT_RGB16_I          1414
#define MNG_FN_INIT_IDX1_NI          1415
#define MNG_FN_INIT_IDX1_I           1416
#define MNG_FN_INIT_IDX2_NI          1417
#define MNG_FN_INIT_IDX2_I           1418
#define MNG_FN_INIT_IDX4_NI          1419
#define MNG_FN_INIT_IDX4_I           1420
#define MNG_FN_INIT_IDX8_NI          1421
#define MNG_FN_INIT_IDX8_I           1422
#define MNG_FN_INIT_GA8_NI           1423
#define MNG_FN_INIT_GA8_I            1424
#define MNG_FN_INIT_GA16_NI          1425
#define MNG_FN_INIT_GA16_I           1426
#define MNG_FN_INIT_RGBA8_NI         1427
#define MNG_FN_INIT_RGBA8_I          1428
#define MNG_FN_INIT_RGBA16_NI        1429
#define MNG_FN_INIT_RGBA16_I         1430

#define MNG_FN_INIT_ROWPROC          1497
#define MNG_FN_NEXT_ROW              1498
#define MNG_FN_CLEANUP_ROWPROC       1499

/* ************************************************************************** */

#define MNG_FN_FILTER_A_ROW          1501
#define MNG_FN_FILTER_SUB            1502
#define MNG_FN_FILTER_UP             1503
#define MNG_FN_FILTER_AVERAGE        1504
#define MNG_FN_FILTER_PAETH          1505

#define MNG_FN_INIT_ROWDIFFERING     1551
#define MNG_FN_DIFFER_G1             1552
#define MNG_FN_DIFFER_G2             1553
#define MNG_FN_DIFFER_G4             1554
#define MNG_FN_DIFFER_G8             1555
#define MNG_FN_DIFFER_G16            1556
#define MNG_FN_DIFFER_RGB8           1557
#define MNG_FN_DIFFER_RGB16          1558
#define MNG_FN_DIFFER_IDX1           1559
#define MNG_FN_DIFFER_IDX2           1560
#define MNG_FN_DIFFER_IDX4           1561
#define MNG_FN_DIFFER_IDX8           1562
#define MNG_FN_DIFFER_GA8            1563
#define MNG_FN_DIFFER_GA16           1564
#define MNG_FN_DIFFER_RGBA8          1565
#define MNG_FN_DIFFER_RGBA16         1566

/* ************************************************************************** */

#define MNG_FN_CREATE_IMGDATAOBJECT  1601
#define MNG_FN_FREE_IMGDATAOBJECT    1602
#define MNG_FN_CLONE_IMGDATAOBJECT   1603
#define MNG_FN_CREATE_IMGOBJECT      1604
#define MNG_FN_FREE_IMGOBJECT        1605
#define MNG_FN_FIND_IMGOBJECT        1606
#define MNG_FN_CLONE_IMGOBJECT       1607
#define MNG_FN_RESET_OBJECTDETAILS   1608
#define MNG_FN_RENUM_IMGOBJECT       1609
#define MNG_FN_PROMOTE_IMGOBJECT     1610
#define MNG_FN_MAGNIFY_IMGOBJECT     1611
#define MNG_FN_COLORCORRECT_OBJECT   1612

/* ************************************************************************** */

#define MNG_FN_STORE_G1              1701
#define MNG_FN_STORE_G2              1702
#define MNG_FN_STORE_G4              1703
#define MNG_FN_STORE_G8              1704
#define MNG_FN_STORE_G16             1705
#define MNG_FN_STORE_RGB8            1706
#define MNG_FN_STORE_RGB16           1707
#define MNG_FN_STORE_IDX1            1708
#define MNG_FN_STORE_IDX2            1709
#define MNG_FN_STORE_IDX4            1710
#define MNG_FN_STORE_IDX8            1711
#define MNG_FN_STORE_GA8             1712
#define MNG_FN_STORE_GA16            1713
#define MNG_FN_STORE_RGBA8           1714
#define MNG_FN_STORE_RGBA16          1715

#define MNG_FN_RETRIEVE_G8           1751
#define MNG_FN_RETRIEVE_G16          1752
#define MNG_FN_RETRIEVE_RGB8         1753
#define MNG_FN_RETRIEVE_RGB16        1754
#define MNG_FN_RETRIEVE_IDX8         1755
#define MNG_FN_RETRIEVE_GA8          1756
#define MNG_FN_RETRIEVE_GA16         1757
#define MNG_FN_RETRIEVE_RGBA8        1758
#define MNG_FN_RETRIEVE_RGBA16       1759

#define MNG_FN_DELTA_G1              1771
#define MNG_FN_DELTA_G2              1772
#define MNG_FN_DELTA_G4              1773
#define MNG_FN_DELTA_G8              1774
#define MNG_FN_DELTA_G16             1775
#define MNG_FN_DELTA_RGB8            1776
#define MNG_FN_DELTA_RGB16           1777
#define MNG_FN_DELTA_IDX1            1778
#define MNG_FN_DELTA_IDX2            1779
#define MNG_FN_DELTA_IDX4            1780
#define MNG_FN_DELTA_IDX8            1781
#define MNG_FN_DELTA_GA8             1782
#define MNG_FN_DELTA_GA16            1783
#define MNG_FN_DELTA_RGBA8           1784
#define MNG_FN_DELTA_RGBA16          1785

/* ************************************************************************** */

#define MNG_FN_CREATE_ANI_LOOP       1801
#define MNG_FN_CREATE_ANI_ENDL       1802
#define MNG_FN_CREATE_ANI_DEFI       1803
#define MNG_FN_CREATE_ANI_BASI       1804
#define MNG_FN_CREATE_ANI_CLON       1805
#define MNG_FN_CREATE_ANI_PAST       1806
#define MNG_FN_CREATE_ANI_DISC       1807
#define MNG_FN_CREATE_ANI_BACK       1808
#define MNG_FN_CREATE_ANI_FRAM       1809
#define MNG_FN_CREATE_ANI_MOVE       1810
#define MNG_FN_CREATE_ANI_CLIP       1811
#define MNG_FN_CREATE_ANI_SHOW       1812
#define MNG_FN_CREATE_ANI_TERM       1813
#define MNG_FN_CREATE_ANI_SAVE       1814
#define MNG_FN_CREATE_ANI_SEEK       1815
#define MNG_FN_CREATE_ANI_GAMA       1816
#define MNG_FN_CREATE_ANI_CHRM       1817
#define MNG_FN_CREATE_ANI_SRGB       1818
#define MNG_FN_CREATE_ANI_ICCP       1819
#define MNG_FN_CREATE_ANI_PLTE       1820
#define MNG_FN_CREATE_ANI_TRNS       1821
#define MNG_FN_CREATE_ANI_BKGD       1822
#define MNG_FN_CREATE_ANI_DHDR       1823
#define MNG_FN_CREATE_ANI_PROM       1824
#define MNG_FN_CREATE_ANI_IPNG       1825
#define MNG_FN_CREATE_ANI_IJNG       1826
#define MNG_FN_CREATE_ANI_PPLT       1827
#define MNG_FN_CREATE_ANI_MAGN       1828

#define MNG_FN_CREATE_ANI_IMAGE      1891
#define MNG_FN_CREATE_EVENT          1892

/* ************************************************************************** */

#define MNG_FN_FREE_ANI_LOOP         1901
#define MNG_FN_FREE_ANI_ENDL         1902
#define MNG_FN_FREE_ANI_DEFI         1903
#define MNG_FN_FREE_ANI_BASI         1904
#define MNG_FN_FREE_ANI_CLON         1905
#define MNG_FN_FREE_ANI_PAST         1906
#define MNG_FN_FREE_ANI_DISC         1907
#define MNG_FN_FREE_ANI_BACK         1908
#define MNG_FN_FREE_ANI_FRAM         1909
#define MNG_FN_FREE_ANI_MOVE         1910
#define MNG_FN_FREE_ANI_CLIP         1911
#define MNG_FN_FREE_ANI_SHOW         1912
#define MNG_FN_FREE_ANI_TERM         1913
#define MNG_FN_FREE_ANI_SAVE         1914
#define MNG_FN_FREE_ANI_SEEK         1915
#define MNG_FN_FREE_ANI_GAMA         1916
#define MNG_FN_FREE_ANI_CHRM         1917
#define MNG_FN_FREE_ANI_SRGB         1918
#define MNG_FN_FREE_ANI_ICCP         1919
#define MNG_FN_FREE_ANI_PLTE         1920
#define MNG_FN_FREE_ANI_TRNS         1921
#define MNG_FN_FREE_ANI_BKGD         1922
#define MNG_FN_FREE_ANI_DHDR         1923
#define MNG_FN_FREE_ANI_PROM         1924
#define MNG_FN_FREE_ANI_IPNG         1925
#define MNG_FN_FREE_ANI_IJNG         1926
#define MNG_FN_FREE_ANI_PPLT         1927
#define MNG_FN_FREE_ANI_MAGN         1928

#define MNG_FN_FREE_ANI_IMAGE        1991
#define MNG_FN_FREE_EVENT            1992

/* ************************************************************************** */

#define MNG_FN_PROCESS_ANI_LOOP      2001
#define MNG_FN_PROCESS_ANI_ENDL      2002
#define MNG_FN_PROCESS_ANI_DEFI      2003
#define MNG_FN_PROCESS_ANI_BASI      2004
#define MNG_FN_PROCESS_ANI_CLON      2005
#define MNG_FN_PROCESS_ANI_PAST      2006
#define MNG_FN_PROCESS_ANI_DISC      2007
#define MNG_FN_PROCESS_ANI_BACK      2008
#define MNG_FN_PROCESS_ANI_FRAM      2009
#define MNG_FN_PROCESS_ANI_MOVE      2010
#define MNG_FN_PROCESS_ANI_CLIP      2011
#define MNG_FN_PROCESS_ANI_SHOW      2012
#define MNG_FN_PROCESS_ANI_TERM      2013
#define MNG_FN_PROCESS_ANI_SAVE      2014
#define MNG_FN_PROCESS_ANI_SEEK      2015
#define MNG_FN_PROCESS_ANI_GAMA      2016
#define MNG_FN_PROCESS_ANI_CHRM      2017
#define MNG_FN_PROCESS_ANI_SRGB      2018
#define MNG_FN_PROCESS_ANI_ICCP      2019
#define MNG_FN_PROCESS_ANI_PLTE      2020
#define MNG_FN_PROCESS_ANI_TRNS      2021
#define MNG_FN_PROCESS_ANI_BKGD      2022
#define MNG_FN_PROCESS_ANI_DHDR      2023
#define MNG_FN_PROCESS_ANI_PROM      2024
#define MNG_FN_PROCESS_ANI_IPNG      2025
#define MNG_FN_PROCESS_ANI_IJNG      2026
#define MNG_FN_PROCESS_ANI_PPLT      2027
#define MNG_FN_PROCESS_ANI_MAGN      2028

#define MNG_FN_PROCESS_ANI_IMAGE     2091
#define MNG_FN_PROCESS_EVENT         2092

/* ************************************************************************** */

#define MNG_FN_RESTORE_BACKIMAGE     2101
#define MNG_FN_RESTORE_BACKCOLOR     2102
#define MNG_FN_RESTORE_BGCOLOR       2103
#define MNG_FN_RESTORE_RGB8          2104
#define MNG_FN_RESTORE_BGR8          2105
#define MNG_FN_RESTORE_BKGD          2106
#define MNG_FN_RESTORE_BGRX8         2107
#define MNG_FN_RESTORE_RGB565        2108
#define MNG_FN_RESTORE_BGR565        2109

/* ************************************************************************** */

#define MNG_FN_INIT_IHDR             2201
#define MNG_FN_INIT_PLTE             2202
#define MNG_FN_INIT_IDAT             2203
#define MNG_FN_INIT_IEND             2204
#define MNG_FN_INIT_TRNS             2205
#define MNG_FN_INIT_GAMA             2206
#define MNG_FN_INIT_CHRM             2207
#define MNG_FN_INIT_SRGB             2208
#define MNG_FN_INIT_ICCP             2209
#define MNG_FN_INIT_TEXT             2210
#define MNG_FN_INIT_ZTXT             2211
#define MNG_FN_INIT_ITXT             2212
#define MNG_FN_INIT_BKGD             2213
#define MNG_FN_INIT_PHYS             2214
#define MNG_FN_INIT_SBIT             2215
#define MNG_FN_INIT_SPLT             2216
#define MNG_FN_INIT_HIST             2217
#define MNG_FN_INIT_TIME             2218
#define MNG_FN_INIT_MHDR             2219
#define MNG_FN_INIT_MEND             2220
#define MNG_FN_INIT_LOOP             2221
#define MNG_FN_INIT_ENDL             2222
#define MNG_FN_INIT_DEFI             2223
#define MNG_FN_INIT_BASI             2224
#define MNG_FN_INIT_CLON             2225
#define MNG_FN_INIT_PAST             2226
#define MNG_FN_INIT_DISC             2227
#define MNG_FN_INIT_BACK             2228
#define MNG_FN_INIT_FRAM             2229
#define MNG_FN_INIT_MOVE             2230
#define MNG_FN_INIT_CLIP             2231
#define MNG_FN_INIT_SHOW             2232
#define MNG_FN_INIT_TERM             2233
#define MNG_FN_INIT_SAVE             2234
#define MNG_FN_INIT_SEEK             2235
#define MNG_FN_INIT_EXPI             2236
#define MNG_FN_INIT_FPRI             2237
#define MNG_FN_INIT_NEED             2238
#define MNG_FN_INIT_PHYG             2239
#define MNG_FN_INIT_JHDR             2240
#define MNG_FN_INIT_JDAT             2241
#define MNG_FN_INIT_JSEP             2242
#define MNG_FN_INIT_DHDR             2243
#define MNG_FN_INIT_PROM             2244
#define MNG_FN_INIT_IPNG             2245
#define MNG_FN_INIT_PPLT             2246
#define MNG_FN_INIT_IJNG             2247
#define MNG_FN_INIT_DROP             2248
#define MNG_FN_INIT_DBYK             2249
#define MNG_FN_INIT_ORDR             2250
#define MNG_FN_INIT_UNKNOWN          2251
#define MNG_FN_INIT_MAGN             2252
#define MNG_FN_INIT_JDAA             2253
#define MNG_FN_INIT_EVNT             2254
#define MNG_FN_INIT_MPNG             2255

/* ************************************************************************** */

#define MNG_FN_ASSIGN_IHDR           2301
#define MNG_FN_ASSIGN_PLTE           2302
#define MNG_FN_ASSIGN_IDAT           2303
#define MNG_FN_ASSIGN_IEND           2304
#define MNG_FN_ASSIGN_TRNS           2305
#define MNG_FN_ASSIGN_GAMA           2306
#define MNG_FN_ASSIGN_CHRM           2307
#define MNG_FN_ASSIGN_SRGB           2308
#define MNG_FN_ASSIGN_ICCP           2309
#define MNG_FN_ASSIGN_TEXT           2310
#define MNG_FN_ASSIGN_ZTXT           2311
#define MNG_FN_ASSIGN_ITXT           2312
#define MNG_FN_ASSIGN_BKGD           2313
#define MNG_FN_ASSIGN_PHYS           2314
#define MNG_FN_ASSIGN_SBIT           2315
#define MNG_FN_ASSIGN_SPLT           2316
#define MNG_FN_ASSIGN_HIST           2317
#define MNG_FN_ASSIGN_TIME           2318
#define MNG_FN_ASSIGN_MHDR           2319
#define MNG_FN_ASSIGN_MEND           2320
#define MNG_FN_ASSIGN_LOOP           2321
#define MNG_FN_ASSIGN_ENDL           2322
#define MNG_FN_ASSIGN_DEFI           2323
#define MNG_FN_ASSIGN_BASI           2324
#define MNG_FN_ASSIGN_CLON           2325
#define MNG_FN_ASSIGN_PAST           2326
#define MNG_FN_ASSIGN_DISC           2327
#define MNG_FN_ASSIGN_BACK           2328
#define MNG_FN_ASSIGN_FRAM           2329
#define MNG_FN_ASSIGN_MOVE           2330
#define MNG_FN_ASSIGN_CLIP           2331
#define MNG_FN_ASSIGN_SHOW           2332
#define MNG_FN_ASSIGN_TERM           2333
#define MNG_FN_ASSIGN_SAVE           2334
#define MNG_FN_ASSIGN_SEEK           2335
#define MNG_FN_ASSIGN_EXPI           2336
#define MNG_FN_ASSIGN_FPRI           2337
#define MNG_FN_ASSIGN_NEED           2338
#define MNG_FN_ASSIGN_PHYG           2339
#define MNG_FN_ASSIGN_JHDR           2340
#define MNG_FN_ASSIGN_JDAT           2341
#define MNG_FN_ASSIGN_JSEP           2342
#define MNG_FN_ASSIGN_DHDR           2343
#define MNG_FN_ASSIGN_PROM           2344
#define MNG_FN_ASSIGN_IPNG           2345
#define MNG_FN_ASSIGN_PPLT           2346
#define MNG_FN_ASSIGN_IJNG           2347
#define MNG_FN_ASSIGN_DROP           2348
#define MNG_FN_ASSIGN_DBYK           2349
#define MNG_FN_ASSIGN_ORDR           2350
#define MNG_FN_ASSIGN_UNKNOWN        2351
#define MNG_FN_ASSIGN_MAGN           2352
#define MNG_FN_ASSIGN_JDAA           2353
#define MNG_FN_ASSIGN_EVNT           2354
#define MNG_FN_ASSIGN_MPNG           2355

/* ************************************************************************** */

#define MNG_FN_FREE_IHDR             2401
#define MNG_FN_FREE_PLTE             2402
#define MNG_FN_FREE_IDAT             2403
#define MNG_FN_FREE_IEND             2404
#define MNG_FN_FREE_TRNS             2405
#define MNG_FN_FREE_GAMA             2406
#define MNG_FN_FREE_CHRM             2407
#define MNG_FN_FREE_SRGB             2408
#define MNG_FN_FREE_ICCP             2409
#define MNG_FN_FREE_TEXT             2410
#define MNG_FN_FREE_ZTXT             2411
#define MNG_FN_FREE_ITXT             2412
#define MNG_FN_FREE_BKGD             2413
#define MNG_FN_FREE_PHYS             2414
#define MNG_FN_FREE_SBIT             2415
#define MNG_FN_FREE_SPLT             2416
#define MNG_FN_FREE_HIST             2417
#define MNG_FN_FREE_TIME             2418
#define MNG_FN_FREE_MHDR             2419
#define MNG_FN_FREE_MEND             2420
#define MNG_FN_FREE_LOOP             2421
#define MNG_FN_FREE_ENDL             2422
#define MNG_FN_FREE_DEFI             2423
#define MNG_FN_FREE_BASI             2424
#define MNG_FN_FREE_CLON             2425
#define MNG_FN_FREE_PAST             2426
#define MNG_FN_FREE_DISC             2427
#define MNG_FN_FREE_BACK             2428
#define MNG_FN_FREE_FRAM             2429
#define MNG_FN_FREE_MOVE             2430
#define MNG_FN_FREE_CLIP             2431
#define MNG_FN_FREE_SHOW             2432
#define MNG_FN_FREE_TERM             2433
#define MNG_FN_FREE_SAVE             2434
#define MNG_FN_FREE_SEEK             2435
#define MNG_FN_FREE_EXPI             2436
#define MNG_FN_FREE_FPRI             2437
#define MNG_FN_FREE_NEED             2438
#define MNG_FN_FREE_PHYG             2439
#define MNG_FN_FREE_JHDR             2440
#define MNG_FN_FREE_JDAT             2441
#define MNG_FN_FREE_JSEP             2442
#define MNG_FN_FREE_DHDR             2443
#define MNG_FN_FREE_PROM             2444
#define MNG_FN_FREE_IPNG             2445
#define MNG_FN_FREE_PPLT             2446
#define MNG_FN_FREE_IJNG             2447
#define MNG_FN_FREE_DROP             2448
#define MNG_FN_FREE_DBYK             2449
#define MNG_FN_FREE_ORDR             2450
#define MNG_FN_FREE_UNKNOWN          2451
#define MNG_FN_FREE_MAGN             2452
#define MNG_FN_FREE_JDAA             2453
#define MNG_FN_FREE_EVNT             2454
#define MNG_FN_FREE_MPNG             2455

/* ************************************************************************** */

#define MNG_FN_READ_IHDR             2601
#define MNG_FN_READ_PLTE             2602
#define MNG_FN_READ_IDAT             2603
#define MNG_FN_READ_IEND             2604
#define MNG_FN_READ_TRNS             2605
#define MNG_FN_READ_GAMA             2606
#define MNG_FN_READ_CHRM             2607
#define MNG_FN_READ_SRGB             2608
#define MNG_FN_READ_ICCP             2609
#define MNG_FN_READ_TEXT             2610
#define MNG_FN_READ_ZTXT             2611
#define MNG_FN_READ_ITXT             2612
#define MNG_FN_READ_BKGD             2613
#define MNG_FN_READ_PHYS             2614
#define MNG_FN_READ_SBIT             2615
#define MNG_FN_READ_SPLT             2616
#define MNG_FN_READ_HIST             2617
#define MNG_FN_READ_TIME             2618
#define MNG_FN_READ_MHDR             2619
#define MNG_FN_READ_MEND             2620
#define MNG_FN_READ_LOOP             2621
#define MNG_FN_READ_ENDL             2622
#define MNG_FN_READ_DEFI             2623
#define MNG_FN_READ_BASI             2624
#define MNG_FN_READ_CLON             2625
#define MNG_FN_READ_PAST             2626
#define MNG_FN_READ_DISC             2627
#define MNG_FN_READ_BACK             2628
#define MNG_FN_READ_FRAM             2629
#define MNG_FN_READ_MOVE             2630
#define MNG_FN_READ_CLIP             2631
#define MNG_FN_READ_SHOW             2632
#define MNG_FN_READ_TERM             2633
#define MNG_FN_READ_SAVE             2634
#define MNG_FN_READ_SEEK             2635
#define MNG_FN_READ_EXPI             2636
#define MNG_FN_READ_FPRI             2637
#define MNG_FN_READ_NEED             2638
#define MNG_FN_READ_PHYG             2639
#define MNG_FN_READ_JHDR             2640
#define MNG_FN_READ_JDAT             2641
#define MNG_FN_READ_JSEP             2642
#define MNG_FN_READ_DHDR             2643
#define MNG_FN_READ_PROM             2644
#define MNG_FN_READ_IPNG             2645
#define MNG_FN_READ_PPLT             2646
#define MNG_FN_READ_IJNG             2647
#define MNG_FN_READ_DROP             2648
#define MNG_FN_READ_DBYK             2649
#define MNG_FN_READ_ORDR             2650
#define MNG_FN_READ_UNKNOWN          2651
#define MNG_FN_READ_MAGN             2652
#define MNG_FN_READ_JDAA             2653
#define MNG_FN_READ_EVNT             2654
#define MNG_FN_READ_MPNG             2655

/* ************************************************************************** */

#define MNG_FN_WRITE_IHDR            2801
#define MNG_FN_WRITE_PLTE            2802
#define MNG_FN_WRITE_IDAT            2803
#define MNG_FN_WRITE_IEND            2804
#define MNG_FN_WRITE_TRNS            2805
#define MNG_FN_WRITE_GAMA            2806
#define MNG_FN_WRITE_CHRM            2807
#define MNG_FN_WRITE_SRGB            2808
#define MNG_FN_WRITE_ICCP            2809
#define MNG_FN_WRITE_TEXT            2810
#define MNG_FN_WRITE_ZTXT            2811
#define MNG_FN_WRITE_ITXT            2812
#define MNG_FN_WRITE_BKGD            2813
#define MNG_FN_WRITE_PHYS            2814
#define MNG_FN_WRITE_SBIT            2815
#define MNG_FN_WRITE_SPLT            2816
#define MNG_FN_WRITE_HIST            2817
#define MNG_FN_WRITE_TIME            2818
#define MNG_FN_WRITE_MHDR            2819
#define MNG_FN_WRITE_MEND            2820
#define MNG_FN_WRITE_LOOP            2821
#define MNG_FN_WRITE_ENDL            2822
#define MNG_FN_WRITE_DEFI            2823
#define MNG_FN_WRITE_BASI            2824
#define MNG_FN_WRITE_CLON            2825
#define MNG_FN_WRITE_PAST            2826
#define MNG_FN_WRITE_DISC            2827
#define MNG_FN_WRITE_BACK            2828
#define MNG_FN_WRITE_FRAM            2829
#define MNG_FN_WRITE_MOVE            2830
#define MNG_FN_WRITE_CLIP            2831
#define MNG_FN_WRITE_SHOW            2832
#define MNG_FN_WRITE_TERM            2833
#define MNG_FN_WRITE_SAVE            2834
#define MNG_FN_WRITE_SEEK            2835
#define MNG_FN_WRITE_EXPI            2836
#define MNG_FN_WRITE_FPRI            2837
#define MNG_FN_WRITE_NEED            2838
#define MNG_FN_WRITE_PHYG            2839
#define MNG_FN_WRITE_JHDR            2840
#define MNG_FN_WRITE_JDAT            2841
#define MNG_FN_WRITE_JSEP            2842
#define MNG_FN_WRITE_DHDR            2843
#define MNG_FN_WRITE_PROM            2844
#define MNG_FN_WRITE_IPNG            2845
#define MNG_FN_WRITE_PPLT            2846
#define MNG_FN_WRITE_IJNG            2847
#define MNG_FN_WRITE_DROP            2848
#define MNG_FN_WRITE_DBYK            2849
#define MNG_FN_WRITE_ORDR            2850
#define MNG_FN_WRITE_UNKNOWN         2851
#define MNG_FN_WRITE_MAGN            2852
#define MNG_FN_WRITE_JDAA            2853
#define MNG_FN_WRITE_EVNT            2854
#define MNG_FN_WRITE_MPNG            2855

/* ************************************************************************** */

#define MNG_FN_ZLIB_INITIALIZE       3001
#define MNG_FN_ZLIB_CLEANUP          3002
#define MNG_FN_ZLIB_INFLATEINIT      3003
#define MNG_FN_ZLIB_INFLATEROWS      3004
#define MNG_FN_ZLIB_INFLATEDATA      3005
#define MNG_FN_ZLIB_INFLATEFREE      3006
#define MNG_FN_ZLIB_DEFLATEINIT      3007
#define MNG_FN_ZLIB_DEFLATEROWS      3008
#define MNG_FN_ZLIB_DEFLATEDATA      3009
#define MNG_FN_ZLIB_DEFLATEFREE      3010

/* ************************************************************************** */

#define MNG_FN_PROCESS_DISPLAY_IHDR  3201
#define MNG_FN_PROCESS_DISPLAY_PLTE  3202
#define MNG_FN_PROCESS_DISPLAY_IDAT  3203
#define MNG_FN_PROCESS_DISPLAY_IEND  3204
#define MNG_FN_PROCESS_DISPLAY_TRNS  3205
#define MNG_FN_PROCESS_DISPLAY_GAMA  3206
#define MNG_FN_PROCESS_DISPLAY_CHRM  3207
#define MNG_FN_PROCESS_DISPLAY_SRGB  3208
#define MNG_FN_PROCESS_DISPLAY_ICCP  3209
#define MNG_FN_PROCESS_DISPLAY_BKGD  3210
#define MNG_FN_PROCESS_DISPLAY_PHYS  3211
#define MNG_FN_PROCESS_DISPLAY_SBIT  3212
#define MNG_FN_PROCESS_DISPLAY_SPLT  3213
#define MNG_FN_PROCESS_DISPLAY_HIST  3214
#define MNG_FN_PROCESS_DISPLAY_MHDR  3215
#define MNG_FN_PROCESS_DISPLAY_MEND  3216
#define MNG_FN_PROCESS_DISPLAY_LOOP  3217
#define MNG_FN_PROCESS_DISPLAY_ENDL  3218
#define MNG_FN_PROCESS_DISPLAY_DEFI  3219
#define MNG_FN_PROCESS_DISPLAY_BASI  3220
#define MNG_FN_PROCESS_DISPLAY_CLON  3221
#define MNG_FN_PROCESS_DISPLAY_PAST  3222
#define MNG_FN_PROCESS_DISPLAY_DISC  3223
#define MNG_FN_PROCESS_DISPLAY_BACK  3224
#define MNG_FN_PROCESS_DISPLAY_FRAM  3225
#define MNG_FN_PROCESS_DISPLAY_MOVE  3226
#define MNG_FN_PROCESS_DISPLAY_CLIP  3227
#define MNG_FN_PROCESS_DISPLAY_SHOW  3228
#define MNG_FN_PROCESS_DISPLAY_TERM  3229
#define MNG_FN_PROCESS_DISPLAY_SAVE  3230
#define MNG_FN_PROCESS_DISPLAY_SEEK  3231
#define MNG_FN_PROCESS_DISPLAY_EXPI  3232
#define MNG_FN_PROCESS_DISPLAY_FPRI  3233
#define MNG_FN_PROCESS_DISPLAY_NEED  3234
#define MNG_FN_PROCESS_DISPLAY_PHYG  3235
#define MNG_FN_PROCESS_DISPLAY_JHDR  3236
#define MNG_FN_PROCESS_DISPLAY_JDAT  3237
#define MNG_FN_PROCESS_DISPLAY_JSEP  3238
#define MNG_FN_PROCESS_DISPLAY_DHDR  3239
#define MNG_FN_PROCESS_DISPLAY_PROM  3240
#define MNG_FN_PROCESS_DISPLAY_IPNG  3241
#define MNG_FN_PROCESS_DISPLAY_PPLT  3242
#define MNG_FN_PROCESS_DISPLAY_IJNG  3243
#define MNG_FN_PROCESS_DISPLAY_DROP  3244
#define MNG_FN_PROCESS_DISPLAY_DBYK  3245
#define MNG_FN_PROCESS_DISPLAY_ORDR  3246
#define MNG_FN_PROCESS_DISPLAY_MAGN  3247
#define MNG_FN_PROCESS_DISPLAY_JDAA  3248

/* ************************************************************************** */

#define MNG_FN_JPEG_INITIALIZE       3401
#define MNG_FN_JPEG_CLEANUP          3402
#define MNG_FN_JPEG_DECOMPRESSINIT   3403
#define MNG_FN_JPEG_DECOMPRESSDATA   3404
#define MNG_FN_JPEG_DECOMPRESSFREE   3405

#define MNG_FN_STORE_JPEG_G8         3501
#define MNG_FN_STORE_JPEG_RGB8       3502
#define MNG_FN_STORE_JPEG_G12        3503
#define MNG_FN_STORE_JPEG_RGB12      3504
#define MNG_FN_STORE_JPEG_GA8        3505
#define MNG_FN_STORE_JPEG_RGBA8      3506
#define MNG_FN_STORE_JPEG_GA12       3507
#define MNG_FN_STORE_JPEG_RGBA12     3508
#define MNG_FN_STORE_JPEG_G8_ALPHA   3509
#define MNG_FN_STORE_JPEG_RGB8_ALPHA 3510

#define MNG_FN_INIT_JPEG_A1_NI       3511
#define MNG_FN_INIT_JPEG_A2_NI       3512
#define MNG_FN_INIT_JPEG_A4_NI       3513
#define MNG_FN_INIT_JPEG_A8_NI       3514
#define MNG_FN_INIT_JPEG_A16_NI      3515

#define MNG_FN_STORE_JPEG_G8_A1      3521
#define MNG_FN_STORE_JPEG_G8_A2      3522
#define MNG_FN_STORE_JPEG_G8_A4      3523
#define MNG_FN_STORE_JPEG_G8_A8      3524
#define MNG_FN_STORE_JPEG_G8_A16     3525

#define MNG_FN_STORE_JPEG_RGB8_A1    3531
#define MNG_FN_STORE_JPEG_RGB8_A2    3532
#define MNG_FN_STORE_JPEG_RGB8_A4    3533
#define MNG_FN_STORE_JPEG_RGB8_A8    3534
#define MNG_FN_STORE_JPEG_RGB8_A16   3535

#define MNG_FN_STORE_JPEG_G12_A1     3541
#define MNG_FN_STORE_JPEG_G12_A2     3542
#define MNG_FN_STORE_JPEG_G12_A4     3543
#define MNG_FN_STORE_JPEG_G12_A8     3544
#define MNG_FN_STORE_JPEG_G12_A16    3545

#define MNG_FN_STORE_JPEG_RGB12_A1   3551
#define MNG_FN_STORE_JPEG_RGB12_A2   3552
#define MNG_FN_STORE_JPEG_RGB12_A4   3553
#define MNG_FN_STORE_JPEG_RGB12_A8   3554
#define MNG_FN_STORE_JPEG_RGB12_A16  3555

#define MNG_FN_NEXT_JPEG_ALPHAROW    3591
#define MNG_FN_NEXT_JPEG_ROW         3592
#define MNG_FN_DISPLAY_JPEG_ROWS     3593

/* ************************************************************************** */

#define MNG_FN_MAGNIFY_G8_X1         3701
#define MNG_FN_MAGNIFY_G8_X2         3702
#define MNG_FN_MAGNIFY_RGB8_X1       3703
#define MNG_FN_MAGNIFY_RGB8_X2       3704
#define MNG_FN_MAGNIFY_GA8_X1        3705
#define MNG_FN_MAGNIFY_GA8_X2        3706
#define MNG_FN_MAGNIFY_GA8_X3        3707
#define MNG_FN_MAGNIFY_GA8_X4        3708
#define MNG_FN_MAGNIFY_RGBA8_X1      3709
#define MNG_FN_MAGNIFY_RGBA8_X2      3710
#define MNG_FN_MAGNIFY_RGBA8_X3      3711
#define MNG_FN_MAGNIFY_RGBA8_X4      3712
#define MNG_FN_MAGNIFY_G8_X3         3713
#define MNG_FN_MAGNIFY_RGB8_X3       3714
#define MNG_FN_MAGNIFY_GA8_X5        3715
#define MNG_FN_MAGNIFY_RGBA8_X5      3716

#define MNG_FN_MAGNIFY_G16_X1        3725
#define MNG_FN_MAGNIFY_G16_X2        3726
#define MNG_FN_MAGNIFY_RGB16_X1      3727
#define MNG_FN_MAGNIFY_RGB16_X2      3728
#define MNG_FN_MAGNIFY_GA16_X1       3729
#define MNG_FN_MAGNIFY_GA16_X2       3730
#define MNG_FN_MAGNIFY_GA16_X3       3731
#define MNG_FN_MAGNIFY_GA16_X4       3732
#define MNG_FN_MAGNIFY_RGBA16_X1     3733
#define MNG_FN_MAGNIFY_RGBA16_X2     3734
#define MNG_FN_MAGNIFY_RGBA16_X3     3735
#define MNG_FN_MAGNIFY_RGBA16_X4     3736
#define MNG_FN_MAGNIFY_G16_X3        3737
#define MNG_FN_MAGNIFY_RGB16_X3      3738
#define MNG_FN_MAGNIFY_GA16_X5       3739
#define MNG_FN_MAGNIFY_RGBA16_X5     3740

#define MNG_FN_MAGNIFY_G8_Y1         3751
#define MNG_FN_MAGNIFY_G8_Y2         3752
#define MNG_FN_MAGNIFY_RGB8_Y1       3753
#define MNG_FN_MAGNIFY_RGB8_Y2       3754
#define MNG_FN_MAGNIFY_GA8_Y1        3755
#define MNG_FN_MAGNIFY_GA8_Y2        3756
#define MNG_FN_MAGNIFY_GA8_Y3        3757
#define MNG_FN_MAGNIFY_GA8_Y4        3758
#define MNG_FN_MAGNIFY_RGBA8_Y1      3759
#define MNG_FN_MAGNIFY_RGBA8_Y2      3760
#define MNG_FN_MAGNIFY_RGBA8_Y3      3761
#define MNG_FN_MAGNIFY_RGBA8_Y4      3762
#define MNG_FN_MAGNIFY_G8_Y3         3763
#define MNG_FN_MAGNIFY_RGB8_Y3       3764
#define MNG_FN_MAGNIFY_GA8_Y5        3765
#define MNG_FN_MAGNIFY_RGBA8_Y5      3766

#define MNG_FN_MAGNIFY_G16_Y1        3775
#define MNG_FN_MAGNIFY_G16_Y2        3776
#define MNG_FN_MAGNIFY_RGB16_Y1      3777
#define MNG_FN_MAGNIFY_RGB16_Y2      3778
#define MNG_FN_MAGNIFY_GA16_Y1       3779
#define MNG_FN_MAGNIFY_GA16_Y2       3780
#define MNG_FN_MAGNIFY_GA16_Y3       3781
#define MNG_FN_MAGNIFY_GA16_Y4       3782
#define MNG_FN_MAGNIFY_RGBA16_Y1     3783
#define MNG_FN_MAGNIFY_RGBA16_Y2     3784
#define MNG_FN_MAGNIFY_RGBA16_Y3     3785
#define MNG_FN_MAGNIFY_RGBA16_Y4     3786
#define MNG_FN_MAGNIFY_G16_Y3        3787
#define MNG_FN_MAGNIFY_RGB16_Y3      3788
#define MNG_FN_MAGNIFY_GA16_Y5       3789
#define MNG_FN_MAGNIFY_RGBA16_Y5     3790

/* ************************************************************************** */

#define MNG_FN_DELTA_G1_G1           3801
#define MNG_FN_DELTA_G2_G2           3802
#define MNG_FN_DELTA_G4_G4           3803
#define MNG_FN_DELTA_G8_G8           3804
#define MNG_FN_DELTA_G16_G16         3805
#define MNG_FN_DELTA_RGB8_RGB8       3806
#define MNG_FN_DELTA_RGB16_RGB16     3807
#define MNG_FN_DELTA_GA8_GA8         3808
#define MNG_FN_DELTA_GA8_G8          3809
#define MNG_FN_DELTA_GA8_A8          3810
#define MNG_FN_DELTA_GA16_GA16       3811
#define MNG_FN_DELTA_GA16_G16        3812
#define MNG_FN_DELTA_GA16_A16        3813
#define MNG_FN_DELTA_RGBA8_RGBA8     3814
#define MNG_FN_DELTA_RGBA8_RGB8      3815
#define MNG_FN_DELTA_RGBA8_A8        3816
#define MNG_FN_DELTA_RGBA16_RGBA16   3817
#define MNG_FN_DELTA_RGBA16_RGB16    3818
#define MNG_FN_DELTA_RGBA16_A16      3819

#define MNG_FN_PROMOTE_G8_G8         3901
#define MNG_FN_PROMOTE_G8_G16        3902
#define MNG_FN_PROMOTE_G16_G16       3903
#define MNG_FN_PROMOTE_G8_GA8        3904
#define MNG_FN_PROMOTE_G8_GA16       3905
#define MNG_FN_PROMOTE_G16_GA16      3906
#define MNG_FN_PROMOTE_G8_RGB8       3907
#define MNG_FN_PROMOTE_G8_RGB16      3908
#define MNG_FN_PROMOTE_G16_RGB16     3909
#define MNG_FN_PROMOTE_G8_RGBA8      3910
#define MNG_FN_PROMOTE_G8_RGBA16     3911
#define MNG_FN_PROMOTE_G16_RGBA16    3912
#define MNG_FN_PROMOTE_GA8_GA16      3913
#define MNG_FN_PROMOTE_GA8_RGBA8     3914
#define MNG_FN_PROMOTE_GA8_RGBA16    3915
#define MNG_FN_PROMOTE_GA16_RGBA16   3916
#define MNG_FN_PROMOTE_RGB8_RGB16    3917
#define MNG_FN_PROMOTE_RGB8_RGBA8    3918
#define MNG_FN_PROMOTE_RGB8_RGBA16   3919
#define MNG_FN_PROMOTE_RGB16_RGBA16  3920
#define MNG_FN_PROMOTE_RGBA8_RGBA16  3921
#define MNG_FN_PROMOTE_IDX8_RGB8     3922
#define MNG_FN_PROMOTE_IDX8_RGB16    3923
#define MNG_FN_PROMOTE_IDX8_RGBA8    3924
#define MNG_FN_PROMOTE_IDX8_RGBA16   3925

#define MNG_FN_SCALE_G1_G2           4001
#define MNG_FN_SCALE_G1_G4           4002
#define MNG_FN_SCALE_G1_G8           4003
#define MNG_FN_SCALE_G1_G16          4004
#define MNG_FN_SCALE_G2_G4           4005
#define MNG_FN_SCALE_G2_G8           4006
#define MNG_FN_SCALE_G2_G16          4007
#define MNG_FN_SCALE_G4_G8           4008
#define MNG_FN_SCALE_G4_G16          4009
#define MNG_FN_SCALE_G8_G16          4010
#define MNG_FN_SCALE_GA8_GA16        4011
#define MNG_FN_SCALE_RGB8_RGB16      4012
#define MNG_FN_SCALE_RGBA8_RGBA16    4013

#define MNG_FN_SCALE_G2_G1           4021
#define MNG_FN_SCALE_G4_G1           4022
#define MNG_FN_SCALE_G8_G1           4023
#define MNG_FN_SCALE_G16_G1          4024
#define MNG_FN_SCALE_G4_G2           4025
#define MNG_FN_SCALE_G8_G2           4026
#define MNG_FN_SCALE_G16_G2          4027
#define MNG_FN_SCALE_G8_G4           4028
#define MNG_FN_SCALE_G16_G4          4029
#define MNG_FN_SCALE_G16_G8          4030
#define MNG_FN_SCALE_GA16_GA8        4031
#define MNG_FN_SCALE_RGB16_RGB8      4032
#define MNG_FN_SCALE_RGBA16_RGBA8    4033

#define MNG_FN_COMPOSEOVER_RGBA8     4501
#define MNG_FN_COMPOSEOVER_RGBA16    4502
#define MNG_FN_COMPOSEUNDER_RGBA8    4503
#define MNG_FN_COMPOSEUNDER_RGBA16   4504

#define MNG_FN_FLIP_RGBA8            4521
#define MNG_FN_FLIP_RGBA16           4522
#define MNG_FN_TILE_RGBA8            4523
#define MNG_FN_TILE_RGBA16           4524

/* ************************************************************************** */
/* *                                                                        * */
/* * Trace string-table entry                                               * */
/* *                                                                        * */
/* ************************************************************************** */

typedef struct {
                 mng_uint32 iFunction;
                 mng_pchar  zTracetext;
               } mng_trace_entry;
typedef mng_trace_entry const * mng_trace_entryp;

/* ************************************************************************** */

#endif /* MNG_INCLUDE_TRACE_PROCS */

/* ************************************************************************** */

#endif /* _libmng_trace_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

