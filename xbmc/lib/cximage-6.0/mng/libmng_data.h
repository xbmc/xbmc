/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_data.h             copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : main data structure definition                             * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of the library main data structure              * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/04/2000 - G.Juyn                                * */
/* *             - added CRC table to main structure (for thread-safety)    * */
/* *             0.5.1 - 05/06/2000 - G.Juyn                                * */
/* *             - added iPLTEentries for checking hIST-length              * */
/* *             0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed palette definition to exported palette-type      * */
/* *             - removed frozen indicator                                 * */
/* *             - added create/write indicators                            * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/13/2000 - G.Juyn                                * */
/* *             - added eMNGma hack (will be removed in 1.0.0 !!!)         * */
/* *             - added TERM animation object pointer (easier reference)   * */
/* *             - added saved-data structure for SAVE/SEEK processing      * */
/* *                                                                        * */
/* *             0.5.2 - 05/18/2000 - G.Juyn                                * */
/* *             - added fields for JNG support (IJG-based)                 * */
/* *             0.5.2 - 05/24/2000 - G.Juyn                                * */
/* *             - changed global tRNS definition                           * */
/* *             0.5.2 - 05/30/2000 - G.Juyn                                * */
/* *             - added delta-image fields                                 * */
/* *             0.5.2 - 06/01/2000 - G.Juyn                                * */
/* *             - added internal delta-image processing callbacks          * */
/* *             0.5.2 - 06/02/2000 - G.Juyn                                * */
/* *             - changed SWAP_ENDIAN to BIGENDIAN_SUPPORTED               * */
/* *               (contributed by Tim Rowley)                              * */
/* *             - added getalphaline callback for RGB8_A8 canvasstyle      * */
/* *             0.5.2 - 06/06/2000 - G.Juyn                                * */
/* *             - added parameter for delayed buffer-processing            * */
/* *                                                                        * */
/* *             0.5.3 - 06/16/2000 - G.Juyn                                * */
/* *             - added update-region parms for refresh calback            * */
/* *             - added Needrefresh parameter                              * */
/* *             0.5.3 - 06/17/2000 - G.Juyn                                * */
/* *             - added Deltaimmediate parm for faster delta-processing    * */
/* *             0.5.3 - 06/21/2000 - G.Juyn                                * */
/* *             - added Speed parameter to facilitate testing              * */
/* *             - added Imagelevel parameter for processtext callback      * */
/* *             0.5.3 - 06/26/2000 - G.Juyn                                * */
/* *             - changed userdata variable to mng_ptr                     * */
/* *                                                                        * */
/* *             0.9.1 - 07/07/2000 - G.Juyn                                * */
/* *             - added variables for go_xxxx processing                   * */
/* *             0.9.1 - 07/08/2000 - G.Juyn                                * */
/* *             - added variables for improved timing support              * */
/* *             0.9.1 - 07/15/2000 - G.Juyn                                * */
/* *             - added callbacks for SAVE/SEEK processing                 * */
/* *             - added variable for NEEDSECTIONWAIT breaks                * */
/* *             - added variable for freeze & reset processing             * */
/* *             0.9.1 - 07/17/2000 - G.Juyn                                * */
/* *             - fixed suspension-buffering for 32K+ chunks               * */
/* *                                                                        * */
/* *             0.9.2 - 07/29/2000 - G.Juyn                                * */
/* *             - removed Nextbackxxx fields (no longer used)              * */
/* *             0.9.2 - 07/31/2000 - G.Juyn                                * */
/* *             - fixed wrapping of suspension parameters                  * */
/* *             0.9.2 - 08/04/2000 - G.Juyn                                * */
/* *             - B111096 - fixed large-buffer read-suspension             * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 09/07/2000 - G.Juyn                                * */
/* *             - added support for new filter_types                       * */
/* *             0.9.3 - 09/10/2000 - G.Juyn                                * */
/* *             - fixed DEFI behavior                                      * */
/* *             0.9.3 - 10/10/2000 - G.Juyn                                * */
/* *             - added support for alpha-depth prediction                 * */
/* *             0.9.3 - 10/11/2000 - G.Juyn                                * */
/* *             - added support for nEED                                   * */
/* *             0.9.3 - 10/16/2000 - G.Juyn                                * */
/* *             - added optional support for bKGD for PNG images           * */
/* *             - added support for JDAA                                   * */
/* *             0.9.3 - 10/17/2000 - G.Juyn                                * */
/* *             - added callback to process non-critical unknown chunks    * */
/* *             - fixed support for bKGD                                   * */
/* *             0.9.3 - 10/19/2000 - G.Juyn                                * */
/* *             - implemented delayed delta-processing                     * */
/* *             0.9.4 - 12/16/2000 - G.Juyn                                * */
/* *             - fixed mixup of data- & function-pointers (thanks Dimitri)* */
/* *                                                                        * */
/* *             1.0.1 - 02/08/2001 - G.Juyn                                * */
/* *             - added MEND processing callback                           * */
/* *             1.0.1 - 02/13/2001 - G.Juyn                                * */
/* *             - fixed first FRAM_MODE=4 timing problem                   * */
/* *                                                                        * */
/* *             1.0.2 - 06/23/2001 - G.Juyn                                * */
/* *             - added optimization option for MNG-video playback         * */
/* *             - added processterm callback                               * */
/* *             1.0.2 - 06/25/2001 - G.Juyn                                * */
/* *             - added option to turn off progressive refresh             * */
/* *                                                                        * */
/* *             1.0.5 - 07/08/2002 - G.Juyn                                * */
/* *             - B578572 - removed eMNGma hack (thanks Dimitri!)          * */
/* *             1.0.5 - 07/16/2002 - G.Juyn                                * */
/* *             - B581625 - large chunks fail with suspension reads        * */
/* *             1.0.5 - 08/15/2002 - G.Juyn                                * */
/* *             - completed PROM support                                   * */
/* *             1.0.5 - 09/15/2002 - G.Juyn                                * */
/* *             - fixed LOOP iteration=0 special case                      * */
/* *             1.0.5 - 09/20/2002 - G.Juyn                                * */
/* *             - finished support for BACK image & tiling                 * */
/* *             1.0.5 - 10/07/2002 - G.Juyn                                * */
/* *             - added another fix for misplaced TERM chunk               * */
/* *             - completed support for condition=2 in TERM chunk          * */
/* *             1.0.5 - 10/20/2002 - G.Juyn                                * */
/* *             - fixed processing for multiple objects in MAGN            * */
/* *             - fixed display of visible target of PAST operation        * */
/* *             1.0.5 - 11/07/2002 - G.Juyn                                * */
/* *             - added support to get totals after mng_read()             * */
/* *             1.0.5 - 24/02/2003 - G.Juyn                                * */
/* *             - B683152 - libjpeg suspension not always honored correctly* */
/* *                                                                        * */
/* *             1.0.6 - 04/11/2003 - G.Juyn                                * */
/* *             - B719420 - fixed several MNG_APP_CMS problems             * */
/* *             1.0.6 - 07/05/2003 - G. R-P                                * */
/* *             - optionally use zlib's crc32() function                   * */
/* *             1.0.6 - 07/29/2003 - G.R-P                                 * */
/* *             - added SKIPCHUNK conditionals around PAST chunk support   * */
/* *             1.0.6 - 08/17/2003 - G.R-P                                 * */
/* *             - added iPNGdepth member to pData structure                * */
/* *                                                                        * */
/* *             1.0.7 - 03/10/2004 - G.R-P                                 * */
/* *             - added conditionals around openstream/closestream         * */
/* *             1.0.7 - 03/24/2004 - G.R-P                                 * */
/* *             - added more SKIPCHUNK conditionals                        * */
/* *                                                                        * */
/* *             1.0.8 - 04/02/2004 - G.Juyn                                * */
/* *             - added CRC existence & checking flags                     * */
/* *             1.0.8 - 04/10/2004 - G.Juyn                                * */
/* *             - added data-push mechanisms for specialized decoders      * */
/* *                                                                        * */
/* *             1.0.9 - 12/11/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_DISPLAYCALLS              * */
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

#ifndef _libmng_data_h_
#define _libmng_data_h_

/* ************************************************************************** */

#define MNG_MAGIC 0x52530a0aL

/* ************************************************************************** */
/* *                                                                        * */
/* * Internal structures                                                    * */
/* *                                                                        * */
/* ************************************************************************** */

typedef mng_palette8 mng_rgbpaltab;

/* ************************************************************************** */
/* *                                                                        * */
/* * The saved_data structure                                               * */
/* *                                                                        * */
/* * This contains the saved data after a SAVE chunk has been processed.    * */
/* * The data is saved from the main data structure during SAVE processing, * */
/* * and restored to the main data structure during SEEK processing.        * */
/* *                                                                        * */
/* ************************************************************************** */

typedef struct mng_savedata_struct {

#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
           mng_bool          bHasglobalPLTE;     /* global PLTE chunk processed */
           mng_bool          bHasglobalTRNS;     /* global tRNS chunk processed */
           mng_bool          bHasglobalGAMA;     /* global gAMA chunk processed */
           mng_bool          bHasglobalCHRM;     /* global cHRM chunk processed */
           mng_bool          bHasglobalSRGB;     /* global sRGB chunk processed */
           mng_bool          bHasglobalICCP;     /* global iCCP chunk processed */
           mng_bool          bHasglobalBKGD;     /* global bKGD chunk processed */
#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

#ifdef MNG_SUPPORT_DISPLAY
           mng_uint16        iBACKred;           /* BACK fields */
           mng_uint16        iBACKgreen;
           mng_uint16        iBACKblue;
           mng_uint8         iBACKmandatory;
           mng_uint16        iBACKimageid;
           mng_uint8         iBACKtile;

           mng_uint8         iFRAMmode;          /* FRAM fields (global) */
           mng_uint32        iFRAMdelay;
           mng_uint32        iFRAMtimeout;
           mng_bool          bFRAMclipping;
           mng_int32         iFRAMclipl;
           mng_int32         iFRAMclipr;
           mng_int32         iFRAMclipt;
           mng_int32         iFRAMclipb;

           mng_uint32        iGlobalPLTEcount;   /* global PLTE fields */
           mng_rgbpaltab     aGlobalPLTEentries;

           mng_uint32        iGlobalTRNSrawlen;  /* global tRNS fields */
           mng_uint8arr      aGlobalTRNSrawdata;

           mng_uint32        iGlobalGamma;       /* global gAMA fields */

#ifndef MNG_SKIPCHUNK_cHRM
           mng_uint32        iGlobalWhitepointx; /* global cHRM fields */
           mng_uint32        iGlobalWhitepointy;
           mng_uint32        iGlobalPrimaryredx;
           mng_uint32        iGlobalPrimaryredy;
           mng_uint32        iGlobalPrimarygreenx;
           mng_uint32        iGlobalPrimarygreeny;
           mng_uint32        iGlobalPrimarybluex;
           mng_uint32        iGlobalPrimarybluey;
#endif

           mng_uint8         iGlobalRendintent;  /* global sRGB fields */

           mng_uint32        iGlobalProfilesize; /* global iCCP fields */
           mng_ptr           pGlobalProfile;

           mng_uint16        iGlobalBKGDred;     /* global bKGD fields */
           mng_uint16        iGlobalBKGDgreen;
           mng_uint16        iGlobalBKGDblue;
#endif /* MNG_SUPPORT_DISPLAY */

        } mng_savedata;

typedef mng_savedata * mng_savedatap;

/* ************************************************************************** */
/* *                                                                        * */
/* * Internal buffer structure for data push mechanisms                     * */
/* *                                                                        * */
/* ************************************************************************** */

typedef struct {
           mng_ptr           pNext;              /* for linked list */
           mng_ptr           pData;              /* used for chunks & data */
           mng_uint32        iLength;
           mng_bool          bOwned;
           mng_uint8p        pDatanext;          /* only used for data */
           mng_uint32        iRemaining;
        } mng_pushdata;
typedef mng_pushdata * mng_pushdatap;

/* ************************************************************************** */
/* *                                                                        * */
/* * The main libmng data structure                                         * */
/* *                                                                        * */
/* * The handle used in all functions points to this structure which        * */
/* * contains all volatile data necessary to process the network graphic.   * */
/* *                                                                        * */
/* ************************************************************************** */

typedef struct mng_data_struct {

           mng_uint32        iMagic;             /* magic number to validate
                                                    a given handle */     
           mng_ptr           pUserdata;          /* application workdata */

           mng_imgtype       eSigtype;           /* image information */
           mng_imgtype       eImagetype;         /* initially zeroed */
           mng_uint32        iWidth;             /* filled after header is processed */
           mng_uint32        iHeight;
           mng_uint32        iTicks;             /* these only after MHDR */
           mng_uint32        iLayercount;
           mng_uint32        iFramecount;
           mng_uint32        iPlaytime;
           mng_uint32        iSimplicity;
           mng_uint8         iAlphadepth;        /* indicates expected alpha-depth */

           mng_uint32        iImagelevel;        /* level of image inside a stream */

           mng_uint32        iCanvasstyle;       /* layout of the drawing-canvas */
           mng_uint32        iBkgdstyle;         /* layout of the background-canvas */

           mng_int8          iMagnify;           /* magnification factor (not used yet) */
           mng_uint32        iOffsetx;           /* x-offset for extremely large image */
           mng_uint32        iOffsety;           /* y-offset for extremely large image */
           mng_uint32        iCanvaswidth;       /* real canvas size */
           mng_uint32        iCanvasheight;      /* must be set by processheader callback */

           mng_uint16        iBGred;             /* default background color */
           mng_uint16        iBGgreen;           /* initially "black" */
           mng_uint16        iBGblue;
           mng_bool          bUseBKGD;           /* preferred use of bKGD for PNG */

           mng_bool          bIssRGB;            /* indicates sRGB system */

#ifdef MNG_FULL_CMS                              /* little CMS variables */
           mng_cmsprof       hProf1;             /* image input profile */
           mng_cmsprof       hProf2;             /* default output profile */
           mng_cmsprof       hProf3;             /* default sRGB profile */
           mng_cmstrans      hTrans;             /* current transformation handle */
#endif

           mng_float         dViewgamma;         /* gamma calculation variables */
           mng_float         dDisplaygamma;      /* initially set for sRGB conditions */
           mng_float         dDfltimggamma;

           mng_bool          bStorechunks;       /* switch for storing chunkdata */
           mng_bool          bSectionbreaks;     /* indicate NEEDSECTIONWAIT breaks */
           mng_bool          bCacheplayback;     /* switch to cache playback info */
           mng_bool          bDoProgressive;     /* progressive refresh for large images */
           mng_uint32        iCrcmode;           /* CRC existence & checking flags */

           mng_speedtype     iSpeed;             /* speed-modifier for animations */

           mng_uint32        iMaxwidth;          /* maximum canvas size */
           mng_uint32        iMaxheight;         /* initially set to 1024 x 1024 */

           mng_int32         iErrorcode;         /* error reporting fields */
           mng_int8          iSeverity;
           mng_int32         iErrorx1;
           mng_int32         iErrorx2;
           mng_pchar         zErrortext;

           mng_memalloc      fMemalloc;          /* callback pointers */
           mng_memfree       fMemfree;           /* initially nulled */
           mng_releasedata   fReleasedata;
#ifndef MNG_NO_OPEN_CLOSE_STREAM
           mng_openstream    fOpenstream;
           mng_closestream   fClosestream;
#endif
           mng_readdata      fReaddata;
           mng_writedata     fWritedata;
           mng_errorproc     fErrorproc;
           mng_traceproc     fTraceproc;
           mng_processheader fProcessheader;
           mng_processtext   fProcesstext;
           mng_processsave   fProcesssave;
           mng_processseek   fProcessseek;
           mng_processneed   fProcessneed;
           mng_processmend   fProcessmend;
           mng_processunknown fProcessunknown;
           mng_processterm   fProcessterm;
           mng_getcanvasline fGetcanvasline;
           mng_getbkgdline   fGetbkgdline;
           mng_getalphaline  fGetalphaline;
           mng_refresh       fRefresh;
           mng_gettickcount  fGettickcount;
           mng_settimer      fSettimer;
           mng_processgamma  fProcessgamma;
           mng_processchroma fProcesschroma;
           mng_processsrgb   fProcesssrgb;
           mng_processiccp   fProcessiccp;
           mng_processarow   fProcessarow;

#if defined(MNG_SUPPORT_READ) || defined(MNG_SUPPORT_WRITE)
#ifndef MNG_NO_OLD_VERSIONS
           mng_bool          bPreDraft48;        /* flags ancient style draft */
#endif

           mng_chunkid       iChunkname;         /* read/write-state variables */
           mng_uint32        iChunkseq;
           mng_chunkp        pFirstchunk;        /* double-linked list of */
           mng_chunkp        pLastchunk;         /* stored chunk-structures */

           mng_bool          bHasheader;         /* first header chunk processed */
           mng_bool          bHasMHDR;           /* inside a MHDR-MEND sequence */
           mng_bool          bHasIHDR;           /* inside a IHDR-IEND sequence */
           mng_bool          bHasBASI;           /* inside a BASI-IEND sequence */
           mng_bool          bHasDHDR;           /* inside a DHDR-IEND sequence */
#ifdef MNG_INCLUDE_JNG
           mng_bool          bHasJHDR;           /* inside a JHDR-IEND sequence */
           mng_bool          bHasJSEP;           /* passed the JSEP separator */
           mng_bool          bHasJDAA;           /* at least 1 JDAA processed */
           mng_bool          bHasJDAT;           /* at least 1 JDAT processed */
#endif
           mng_bool          bHasPLTE;           /* PLTE chunk processed */
           mng_bool          bHasTRNS;           /* tRNS chunk processed */
           mng_bool          bHasGAMA;           /* gAMA chunk processed */
           mng_bool          bHasCHRM;           /* cHRM chunk processed */
           mng_bool          bHasSRGB;           /* sRGB chunk processed */
           mng_bool          bHasICCP;           /* iCCP chunk processed */
           mng_bool          bHasBKGD;           /* bKGD chunk processed */
           mng_bool          bHasIDAT;           /* at least 1 IDAT processed */
           
           mng_bool          bHasSAVE;           /* SAVE chunk processed */
           mng_bool          bHasBACK;           /* BACK chunk processed */
           mng_bool          bHasFRAM;           /* FRAM chunk processed */
           mng_bool          bHasTERM;           /* TERM chunk processed */
           mng_bool          bHasLOOP;           /* at least 1 LOOP open */

           mng_bool          bHasglobalPLTE;     /* global PLTE chunk processed */
           mng_bool          bHasglobalTRNS;     /* global tRNS chunk processed */
           mng_bool          bHasglobalGAMA;     /* global gAMA chunk processed */
           mng_bool          bHasglobalCHRM;     /* global cHRM chunk processed */
           mng_bool          bHasglobalSRGB;     /* global sRGB chunk processed */
           mng_bool          bHasglobalICCP;     /* global iCCP chunk processed */
           mng_bool          bHasglobalBKGD;     /* global bKGD chunk processed */

           mng_uint32        iDatawidth;         /* IHDR/BASI/DHDR fields */
           mng_uint32        iDataheight;        /* valid if inside IHDR-IEND, */
           mng_uint8         iBitdepth;          /* BASI-IEND or DHDR-IEND */
           mng_uint8         iColortype;
           mng_uint8         iCompression;
           mng_uint8         iFilter;
           mng_uint8         iInterlace;

           mng_uint32        iPLTEcount;         /* PLTE fields */

#ifdef MNG_INCLUDE_JNG
           mng_uint8         iJHDRcolortype;     /* JHDR fields */
           mng_uint8         iJHDRimgbitdepth;   /* valid if inside JHDR-IEND */
           mng_uint8         iJHDRimgcompression;
           mng_uint8         iJHDRimginterlace;
           mng_uint8         iJHDRalphabitdepth;
           mng_uint8         iJHDRalphacompression;
           mng_uint8         iJHDRalphafilter;
           mng_uint8         iJHDRalphainterlace;
#endif

#endif /* MNG_SUPPORT_READ || MNG_SUPPORT_WRITE */

#ifdef MNG_SUPPORT_READ
           mng_bool          bReading;           /* read processing variables */
           mng_bool          bHavesig;
           mng_bool          bEOF;
           mng_uint32        iReadbufsize;
           mng_uint8p        pReadbuf;

           mng_uint32        iLargebufsize;      /* temp for very large chunks */
           mng_uint8p        pLargebuf;

           mng_uint32        iSuspendtime;       /* tickcount at last suspension */
           mng_bool          bSuspended;         /* input-reading has been suspended;
                                                    we're expecting a call to
                                                    mng_read_resume! */
           mng_uint8         iSuspendpoint;      /* indicates at which point the flow
                                                    was broken to suspend input-reading */
                                                    
           mng_bool          bSuspensionmode;    /* I/O-suspension variables */
           mng_uint32        iSuspendbufsize;
           mng_uint8p        pSuspendbuf;
           mng_uint8p        pSuspendbufnext;
           mng_uint32        iSuspendbufleft;
           mng_uint32        iChunklen;          /* chunk length */
           mng_uint8p        pReadbufnext;       /* 32K+ suspension-processing */
           mng_uint8p        pLargebufnext;

           mng_pushdatap     pFirstpushchunk;    /* variables for push mechanisms */
           mng_pushdatap     pLastpushchunk;
           mng_pushdatap     pFirstpushdata;
           mng_pushdatap     pLastpushdata;
#endif /* MNG_SUPPORT_READ */

#ifdef MNG_SUPPORT_WRITE
           mng_bool          bCreating;          /* create/write processing variables */
           mng_bool          bWriting;
           mng_chunkid       iFirstchunkadded;
           mng_uint32        iWritebufsize;
           mng_uint8p        pWritebuf;
#endif

#ifdef MNG_SUPPORT_DISPLAY
           mng_bool          bDisplaying;        /* display-state variables */
           mng_bool          bFramedone;
           mng_uint32        iFrameseq;
           mng_uint32        iLayerseq;
           mng_uint32        iFrametime;         /* millisecs */

           mng_uint32        iTotalframes;       /* end-totals after mng_read() */
           mng_uint32        iTotallayers;
           mng_uint32        iTotalplaytime;     /* millisecs */

           mng_bool          bSkipping;          /* LOOP iteration=0 */
           
#ifdef MNG_SUPPORT_DYNAMICMNG
           mng_bool          bDynamic;           /* MNG is dynamic (eg. has events) */
           mng_bool          bRunningevent;      /* currently processing an event */
           mng_bool          bStopafterseek;     /* stop after next SEEK */
           mng_int32         iEventx;            /* X/Y of current event */
           mng_int32         iEventy;
           mng_objectp       pLastmousemove;     /* last event triggered */
#endif

           mng_uint32        iRequestframe;      /* go_xxxx variables */
           mng_uint32        iRequestlayer;
           mng_uint32        iRequesttime;
           mng_bool          bSearching;

           mng_bool          bRestorebkgd;       /* flags restore required before IDAT/JDAT */

           mng_uint32        iRuntime;           /* millisecs since start */
           mng_uint32        iSynctime;          /* tickcount at last framesync */
           mng_uint32        iStarttime;         /* tickcount at start */
           mng_uint32        iEndtime;           /* tickcount at end */
           mng_bool          bRunning;           /* animation is active */
           mng_bool          bTimerset;          /* the timer has been set;
                                                    we're expecting a call to
                                                    mng_display_resume! */
           mng_uint8         iBreakpoint;        /* indicates at which point the
                                                    flow was broken to run the timer */
           mng_bool          bSectionwait;       /* indicates a section break */
           mng_bool          bFreezing;          /* indicates app requested a freeze */   
           mng_bool          bResetting;         /* indicates app requested a reset */   
           mng_bool          bNeedrefresh;       /* indicates screen-refresh is needed */
           mng_bool          bMisplacedTERM;     /* indicates TERM is out of place */
           mng_bool          bOnlyfirstframe;    /* show first frame after TERM and stop */
           mng_uint32        iFramesafterTERM;   /* determines frame-count after TERM */          
           mng_objectp       pCurrentobj;        /* current "object" */
           mng_objectp       pCurraniobj;        /* current animation object
                                                    "to be"/"being" processed */
           mng_objectp       pTermaniobj;        /* TERM animation object */
           mng_uint32        iIterations;        /* TERM/MEND iteration count */
           mng_objectp       pObjzero;           /* "on-the-fly" image (object = 0) */
           mng_objectp       pLastclone;         /* last clone */
           mng_objectp       pStoreobj;          /* current store object for row routines */
           mng_objectp       pStorebuf;          /* current store object-buffer for row routines */
           mng_objectp       pRetrieveobj;       /* current retrieve object for row routines */
           mng_savedatap     pSavedata;          /* pointer to saved data (after SAVE) */

           mng_uint32        iUpdateleft;        /* update region for refresh */
           mng_uint32        iUpdateright;
           mng_uint32        iUpdatetop;
           mng_uint32        iUpdatebottom;

           mng_int8          iPass;              /* current interlacing pass;
                                                    negative value means no interlace */
           mng_int32         iRow;               /* current row counter */
           mng_int32         iRowinc;            /* row increment for this pass */
           mng_int32         iCol;               /* current starting column */
           mng_int32         iColinc;            /* column increment for this pass */
           mng_int32         iRowsamples;        /* nr. of samples in current workrow */
           mng_int32         iSamplemul;         /* needed to calculate rowsize */
           mng_int32         iSampleofs;            /* from rowsamples */
           mng_int32         iSamplediv;
           mng_int32         iRowsize;           /* size of actual data in work row */
           mng_int32         iRowmax;            /* maximum size of data in work row */
           mng_int32         iFilterofs;         /* offset to filter-byte in work row */
           mng_int32         iPixelofs;          /* offset to pixel-bytes in work row */
           mng_uint32        iLevel0;            /* leveling variables */
           mng_uint32        iLevel1;
           mng_uint32        iLevel2;
           mng_uint32        iLevel3;
           mng_uint8p        pWorkrow;           /* working row of pixel-data */
           mng_uint8p        pPrevrow;           /* previous row of pixel-data */
           mng_uint8p        pRGBArow;           /* intermediate row of RGBA8 or RGBA16 data */
           mng_bool          bIsRGBA16;          /* indicates intermediate row is RGBA16 */
           mng_bool          bIsOpaque;          /* indicates intermediate row is fully opaque */
           mng_int32         iFilterbpp;         /* bpp index for filtering routines */

           mng_int32         iSourcel;           /* variables for showing objects */
           mng_int32         iSourcer;
           mng_int32         iSourcet;
           mng_int32         iSourceb;
           mng_int32         iDestl;
           mng_int32         iDestr;
           mng_int32         iDestt;
           mng_int32         iDestb;

           mng_objectp       pFirstimgobj;       /* double-linked list of */
           mng_objectp       pLastimgobj;        /* image-object structures */
           mng_objectp       pFirstaniobj;       /* double-linked list of */
           mng_objectp       pLastaniobj;        /* animation-object structures */
#ifdef MNG_SUPPORT_DYNAMICMNG
           mng_objectp       pFirstevent;        /* double-linked list of */
           mng_objectp       pLastevent;         /* event-object structures */
#endif

#if defined(MNG_GAMMA_ONLY) || defined(MNG_FULL_CMS) || defined(MNG_APP_CMS)
           mng_uint8         aGammatab[256];     /* precomputed gamma lookup table */
           mng_float         dLastgamma;         /* last gamma used to compute table */
#endif

           mng_fptr          fDisplayrow;        /* internal callback to display an
                                                    uncompressed/unfiltered/
                                                    color-corrected row */
           mng_fptr          fRestbkgdrow;       /* internal callback for restore-
                                                    background processing of a row */
           mng_fptr          fCorrectrow;        /* internal callback to color-correct an
                                                    uncompressed/unfiltered row */
           mng_fptr          fRetrieverow;       /* internal callback to retrieve an
                                                    uncompressed/unfiltered row of data */
           mng_fptr          fStorerow;          /* internal callback to store an
                                                    uncompressed/unfiltered row of data */
           mng_fptr          fProcessrow;        /* internal callback to process an
                                                    uncompressed row of data */
           mng_fptr          fDifferrow;         /* internal callback to perform
                                                    added filter leveling and
                                                    differing on an unfiltered row */
           mng_fptr          fScalerow;          /* internal callback to scale a
                                                    delta-row to the bitdepth of its target */
           mng_fptr          fDeltarow;          /* internal callback to execute a
                                                    delta-row onto a target */
#ifndef MNG_SKIPCHUNK_PAST
           mng_fptr          fFliprow;           /* internal callback to flip a row of pixels
                                                    left<->right for a PAST operation */
           mng_fptr          fTilerow;           /* internal callback to tile a row of pixels
                                                    during a PAST operation */
#endif
           mng_fptr          fInitrowproc;       /* internal callback to initialize
                                                    the row processing */

           mng_uint16        iDEFIobjectid;      /* DEFI fields */
           mng_bool          bDEFIhasdonotshow;
           mng_uint8         iDEFIdonotshow;
           mng_bool          bDEFIhasconcrete;
           mng_uint8         iDEFIconcrete;
           mng_bool          bDEFIhasloca;
           mng_int32         iDEFIlocax;
           mng_int32         iDEFIlocay;
           mng_bool          bDEFIhasclip;
           mng_int32         iDEFIclipl;
           mng_int32         iDEFIclipr;
           mng_int32         iDEFIclipt;
           mng_int32         iDEFIclipb;

           mng_uint16        iBACKred;           /* BACK fields */
           mng_uint16        iBACKgreen;
           mng_uint16        iBACKblue;
           mng_uint8         iBACKmandatory;
           mng_uint16        iBACKimageid;
           mng_uint8         iBACKtile;

           mng_int32         iBackimgoffsx;      /* temp variables for restore_bkgd */
           mng_int32         iBackimgoffsy;
           mng_uint32        iBackimgwidth;
           mng_uint32        iBackimgheight;

#ifndef MNG_SKIPCHUNK_FRAM
           mng_uint8         iFRAMmode;          /* FRAM fields (global) */
           mng_uint32        iFRAMdelay;
           mng_uint32        iFRAMtimeout;
           mng_bool          bFRAMclipping;
           mng_int32         iFRAMclipl;
           mng_int32         iFRAMclipr;
           mng_int32         iFRAMclipt;
           mng_int32         iFRAMclipb;

           mng_uint8         iFramemode;         /* current subframe variables */
           mng_uint32        iFramedelay;
           mng_uint32        iFrametimeout;
           mng_bool          bFrameclipping;
           mng_int32         iFrameclipl;
           mng_int32         iFrameclipr;
           mng_int32         iFrameclipt;
           mng_int32         iFrameclipb;

           mng_uint32        iNextdelay;         /* delay *after* next image */
#endif

#ifndef MNG_SKIPCHUNK_SHOW
           mng_uint8         iSHOWmode;          /* SHOW fields */
           mng_uint16        iSHOWfromid;
           mng_uint16        iSHOWtoid;
           mng_uint16        iSHOWnextid;
           mng_int16         iSHOWskip;
#endif

           mng_uint32        iGlobalPLTEcount;   /* global PLTE fields */
           mng_rgbpaltab     aGlobalPLTEentries;

           mng_uint32        iGlobalTRNSrawlen;  /* global tRNS fields */
           mng_uint8arr      aGlobalTRNSrawdata;

           mng_uint32        iGlobalGamma;       /* global gAMA fields */

#ifndef MNG_SKIPCHUNK_cHRM
           mng_uint32        iGlobalWhitepointx; /* global cHRM fields */
           mng_uint32        iGlobalWhitepointy;
           mng_uint32        iGlobalPrimaryredx;
           mng_uint32        iGlobalPrimaryredy;
           mng_uint32        iGlobalPrimarygreenx;
           mng_uint32        iGlobalPrimarygreeny;
           mng_uint32        iGlobalPrimarybluex;
           mng_uint32        iGlobalPrimarybluey;
#endif

           mng_uint8         iGlobalRendintent;  /* global sRGB fields */

#ifndef MNG_SKIPCHUNK_iCCP
           mng_uint32        iGlobalProfilesize; /* global iCCP fields */
           mng_ptr           pGlobalProfile;
#endif

           mng_uint16        iGlobalBKGDred;     /* global bKGD fields */
           mng_uint16        iGlobalBKGDgreen;
           mng_uint16        iGlobalBKGDblue;

           mng_ptr           pDeltaImage;        /* delta-image fields */
           mng_uint8         iDeltaImagetype;
#endif /* MNG_SUPPORT_DISPLAY */
           mng_uint8         iDeltatype;         /* need this one in read processing !! */
#ifdef MNG_SUPPORT_DISPLAY
           mng_uint32        iDeltaBlockwidth;
           mng_uint32        iDeltaBlockheight;
           mng_uint32        iDeltaBlockx;
           mng_uint32        iDeltaBlocky;
           mng_bool          bDeltaimmediate;

           mng_fptr          fDeltagetrow;       /* internal delta-proc callbacks */
           mng_fptr          fDeltaaddrow;
           mng_fptr          fDeltareplacerow;
           mng_fptr          fDeltaputrow;

#ifndef MNG_SKIPCHUNK_PROM
           mng_fptr          fPromoterow;        /* internal PROM fields */
           mng_fptr          fPromBitdepth;
           mng_ptr           pPromBuf;
           mng_uint8         iPromColortype;
           mng_uint8         iPromBitdepth;
           mng_uint8         iPromFilltype;
           mng_uint32        iPromWidth;
           mng_ptr           pPromSrc;
           mng_ptr           pPromDst;
#endif

#ifndef MNG_SKIPCHUNK_MAGN
           mng_uint16        iMAGNfromid;
           mng_uint16        iMAGNcurrentid;
           mng_uint16        iMAGNtoid;
#endif

#ifndef MNG_SKIPCHUNK_PAST
           mng_uint16        iPASTid;
           mng_int32         iPastx;             /* target x/y of last PAST */
           mng_int32         iPasty;
#endif

           mng_objectp       pLastseek;          /* last processed ani_seek object */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
           mng_objectp       pMPNG;              /* mpNG object if available */
#endif
#ifdef MNG_INCLUDE_ANG_PROPOSAL
           mng_objectp       pANG;               /* ANG object if available */
#endif

#endif /* MNG_SUPPORT_DISPLAY */

#ifdef MNG_INCLUDE_ZLIB
           z_stream          sZlib;              /* zlib (de)compression variables */

           mng_int32         iZlevel;            /* zlib compression parameters */
           mng_int32         iZmethod;
           mng_int32         iZwindowbits;
           mng_int32         iZmemlevel;
           mng_int32         iZstrategy;

           mng_uint32        iMaxIDAT;           /* maximum size of IDAT data */

           mng_bool          bInflating;         /* indicates "inflate" in progress */
           mng_bool          bDeflating;         /* indicates "deflate" in progress */
#endif /* MNG_INCLUDE_ZLIB */

#ifdef MNG_INCLUDE_JNG
           mngjpeg_dctmethod eJPEGdctmethod;     /* IJG compression variables */
           mng_int32         iJPEGquality;
           mng_int32         iJPEGsmoothing;
           mng_bool          bJPEGcompressprogr;
           mng_bool          bJPEGcompressopt;

           mng_uint32        iMaxJDAT;           /* maximum size of JDAT/JDAA data */

           mngjpeg_compp     pJPEGcinfo;         /* compression structure */
           mngjpeg_errorp    pJPEGcerr;          /* error-manager compress */

           mngjpeg_decompp   pJPEGdinfo;         /* decompression structure (JDAT) */
           mngjpeg_errorp    pJPEGderr;          /* error-manager decompress (JDAT) */
           mngjpeg_sourcep   pJPEGdsrc;          /* source-manager decompress (JDAT) */

           mngjpeg_decompp   pJPEGdinfo2;        /* decompression structure (JDAA) */
           mngjpeg_errorp    pJPEGderr2;         /* error-manager decompress (JDAA) */
           mngjpeg_sourcep   pJPEGdsrc2;         /* source-manager decompress (JDAA) */

           mng_uint8p        pJPEGbuf;           /* buffer for JPEG (de)compression (JDAT) */
           mng_uint32        iJPEGbufmax;        /* allocated space for buffer (JDAT) */
           mng_uint8p        pJPEGcurrent;       /* current pointer into buffer (JDAT) */
           mng_uint32        iJPEGbufremain;     /* remaining bytes in buffer (JDAT) */
           mng_uint32        iJPEGtoskip;        /* bytes to skip on next input-block (JDAT) */

           mng_uint8p        pJPEGbuf2;          /* buffer for JPEG (de)compression (JDAA) */
           mng_uint32        iJPEGbufmax2;       /* allocated space for buffer (JDAA) */
           mng_uint8p        pJPEGcurrent2;      /* current pointer into buffer (JDAA) */
           mng_uint32        iJPEGbufremain2;    /* remaining bytes in buffer (JDAA) */
           mng_uint32        iJPEGtoskip2;       /* bytes to skip on next input-block (JDAA) */

           mng_uint8p        pJPEGrow;           /* buffer for a JPEG row of samples (JDAT) */
           mng_uint32        iJPEGrowlen;

           mng_uint8p        pJPEGrow2;          /* buffer for a JPEG row of samples (JDAA) */
           mng_uint32        iJPEGrowlen2;

           mng_bool          bJPEGcompress;      /* indicates "compress" initialized */

           mng_bool          bJPEGdecompress;    /* indicates "decompress" initialized (JDAT) */
           mng_bool          bJPEGhasheader;     /* indicates "readheader" succeeded (JDAT) */
           mng_bool          bJPEGdecostarted;   /* indicates "decompress" started (JDAT) */
           mng_bool          bJPEGscanstarted;   /* indicates "first scan" started (JDAT) */
           mng_bool          bJPEGscanending;    /* indicates "finish_output" suspended (JDAT) */
           mng_bool          bJPEGprogressive;   /* indicates a progressive image (JDAT) */

           mng_bool          bJPEGdecompress2;   /* indicates "decompress" initialized (JDAA) */
           mng_bool          bJPEGhasheader2;    /* indicates "readheader" succeeded (JDAA) */
           mng_bool          bJPEGdecostarted2;  /* indicates "decompress" started (JDAA) */
           mng_bool          bJPEGscanstarted2;  /* indicates "first scan" started (JDAA) */
           mng_bool          bJPEGprogressive2;  /* indicates a progressive image (JDAA) */

           mng_fptr          fStorerow2;         /* internal callback to store an
                                                    uncompressed/unfiltered row of JPEG-data (JDAT) */

           mng_fptr          fStorerow3;         /* internal callback to store an
                                                    uncompressed/unfiltered row of JPEG-data (JDAA) */

           mng_uint32        iJPEGrow;           /* row-number for current JPEG row */
           mng_uint32        iJPEGalpharow;      /* nr. of rows filled with alpha */
           mng_uint32        iJPEGrgbrow;        /* nr. of rows filled with 'color'-info */
           mng_uint32        iJPEGdisprow;       /* nr. of rows already displayed "on-the-fly" */

#if defined(MNG_USE_SETJMP) && defined (MNG_INCLUDE_IJG6B)
           jmp_buf           sErrorbuf;          /* setjmp/longjmp buffer (error-recovery) */
#endif

#endif /* MNG_INCLUDE_JNG */

#ifndef MNG_USE_ZLIB_CRC
           mng_uint32        aCRCtable [256];    /* CRC prefab table */
           mng_bool          bCRCcomputed;       /* "has been built" indicator */
#endif

#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
           png_imgtype       ePng_imgtype;
#endif

#if defined(MNG_NO_1_2_4BIT_SUPPORT) || defined(MNG_NO_16BIT_SUPPORT)
           mng_uint8         iPNGdepth;          /* Real input depth */
           mng_uint8         iPNGmult;
#endif

#ifdef MNG_OPTIMIZE_DISPLAYCALLS
           mng_uint32        iRawlen;            /* temp vars for display processing */
           mng_uint8p        pRawdata;
#ifndef MNG_SKIPCHUNK_BASI
           mng_uint16        iBASIred;
           mng_uint16        iBASIgreen;
           mng_uint16        iBASIblue;
           mng_bool          bBASIhasalpha;
           mng_uint16        iBASIalpha;
           mng_uint8         iBASIviewable;
#endif
#ifndef MNG_SKIPCHUNK_CLON
           mng_uint16        iCLONsourceid;
           mng_uint16        iCLONcloneid;
           mng_uint8         iCLONclonetype;
           mng_bool          bCLONhasdonotshow;
           mng_uint8         iCLONdonotshow;
           mng_uint8         iCLONconcrete;
           mng_bool          bCLONhasloca;
           mng_uint8         iCLONlocationtype;
           mng_int32         iCLONlocationx;
           mng_int32         iCLONlocationy;
#endif
#ifndef MNG_SKIPCHUNK_DISC
           mng_uint32        iDISCcount;
           mng_uint16p       pDISCids;
#endif
#ifndef MNG_SKIPCHUNK_FRAM
           mng_uint8         iTempFramemode;
           mng_uint8         iTempChangedelay;
           mng_uint32        iTempDelay;
           mng_uint8         iTempChangetimeout;
           mng_uint32        iTempTimeout;
           mng_uint8         iTempChangeclipping;
           mng_uint8         iTempCliptype;
           mng_int32         iTempClipl;
           mng_int32         iTempClipr;
           mng_int32         iTempClipt;
           mng_int32         iTempClipb;
#endif
#ifndef MNG_SKIPCHUNK_MOVE
           mng_uint16        iMOVEfromid;
           mng_uint16        iMOVEtoid;
           mng_uint8         iMOVEmovetype;
           mng_int32         iMOVEmovex;
           mng_int32         iMOVEmovey;
#endif
#ifndef MNG_SKIPCHUNK_CLIP
           mng_uint16        iCLIPfromid;
           mng_uint16        iCLIPtoid;
           mng_uint8         iCLIPcliptype;
           mng_int32         iCLIPclipl;
           mng_int32         iCLIPclipr;
           mng_int32         iCLIPclipt;
           mng_int32         iCLIPclipb;
#endif
#ifndef MNG_NO_DELTA_PNG
           mng_uint16        iDHDRobjectid;
           mng_uint8         iDHDRimagetype;
           mng_uint8         iDHDRdeltatype;
           mng_uint32        iDHDRblockwidth;
           mng_uint32        iDHDRblockheight;
           mng_uint32        iDHDRblockx;
           mng_uint32        iDHDRblocky;
           mng_uint8         iPROMbitdepth;
           mng_uint8         iPROMcolortype;
           mng_uint8         iPROMfilltype;
           mng_uint8         iPPLTtype;
           mng_uint32        iPPLTcount;
           mng_palette8ep    paPPLTindexentries;
           mng_uint8p        paPPLTalphaentries;
           mng_uint8p        paPPLTusedentries;
#endif
#ifndef MNG_SKIPCHUNK_MAGN
           mng_uint16        iMAGNfirstid;
           mng_uint16        iMAGNlastid;
           mng_uint8         iMAGNmethodX;
           mng_uint16        iMAGNmX;
           mng_uint16        iMAGNmY;
           mng_uint16        iMAGNmL;
           mng_uint16        iMAGNmR;
           mng_uint16        iMAGNmT;
           mng_uint16        iMAGNmB;
           mng_uint8         iMAGNmethodY;
#endif
#ifndef MNG_SKIPCHUNK_PAST
           mng_uint16        iPASTtargetid;
           mng_uint8         iPASTtargettype;
           mng_int32         iPASTtargetx;
           mng_int32         iPASTtargety;
           mng_uint32        iPASTcount;
           mng_ptr           pPASTsources;
#endif
#endif /* MNG_OPTIMIZE_DISPLAYCALLS */

        } mng_data;

typedef mng_data * mng_datap;

/* ************************************************************************** */
/* *                                                                        * */
/* * Internal Callback-Function prototypes                                  * */
/* *                                                                        * */
/* ************************************************************************** */

typedef mng_retcode(*mng_displayrow)  (mng_datap  pData);
typedef mng_retcode(*mng_restbkgdrow) (mng_datap  pData);
typedef mng_retcode(*mng_correctrow)  (mng_datap  pData);
typedef mng_retcode(*mng_retrieverow) (mng_datap  pData);
typedef mng_retcode(*mng_storerow)    (mng_datap  pData);
typedef mng_retcode(*mng_processrow)  (mng_datap  pData);
typedef mng_retcode(*mng_initrowproc) (mng_datap  pData);
typedef mng_retcode(*mng_differrow)   (mng_datap  pData);
typedef mng_retcode(*mng_scalerow)    (mng_datap  pData);
typedef mng_retcode(*mng_deltarow)    (mng_datap  pData);
typedef mng_retcode(*mng_promoterow)  (mng_datap  pData);
typedef mng_retcode(*mng_fliprow)     (mng_datap  pData);
typedef mng_retcode(*mng_tilerow)     (mng_datap  pData);

typedef mng_uint8  (*mng_bitdepth_8)  (mng_uint8  iB);
typedef mng_uint16 (*mng_bitdepth_16) (mng_uint8  iB);

typedef mng_retcode(*mng_magnify_x)   (mng_datap  pData,
                                       mng_uint16 iMX,
                                       mng_uint16 iML,
                                       mng_uint16 iMR,
                                       mng_uint32 iWidth,
                                       mng_uint8p iSrcline,
                                       mng_uint8p iDstline);
typedef mng_retcode(*mng_magnify_y)   (mng_datap  pData,
                                       mng_int32  iM,
                                       mng_int32  iS,
                                       mng_uint32 iWidth,
                                       mng_uint8p iSrcline1,
                                       mng_uint8p iSrcline2,
                                       mng_uint8p iDstline);

/* ************************************************************************** */
/* *                                                                        * */
/* * Routines for swapping byte-order from and to graphic files             * */
/* * (This code is adapted from the libpng package)                         * */
/* *                                                                        * */
/* ************************************************************************** */

#ifndef MNG_BIGENDIAN_SUPPORTED
mng_uint32 mng_get_uint32 (mng_uint8p pBuf);
mng_int32  mng_get_int32  (mng_uint8p pBuf);
mng_uint16 mng_get_uint16 (mng_uint8p pBuf);
void       mng_put_uint32 (mng_uint8p pBuf,
                           mng_uint32 i);
void       mng_put_int32  (mng_uint8p pBuf,
                           mng_int32  i);
void       mng_put_uint16 (mng_uint8p pBuf,
                           mng_uint16 i);
#else /* MNG_BIGENDIAN_SUPPORTED */
#define mng_get_uint32(P)   *(mng_uint32p)(P)
#define mng_get_int32(P)    *(mng_int32p)(P)
#define mng_get_uint16(P)   *(mng_uint16p)(P)
#define mng_put_uint32(P,I) *(mng_uint32p)(P) = (I)
#define mng_put_int32(P,I)  *(mng_int32p)(P) = (I)
#define mng_put_uint16(P,I) *(mng_uint16p)(P) = (I)
#endif /* MNG_BIGENDIAN_SUPPORTED */

/* ************************************************************************** */
/* *                                                                        * */
/* * Some handy(?) macro definitions                                        * */
/* *                                                                        * */
/* ************************************************************************** */

#define MAX_COORD(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN_COORD(a, b)  (((a) < (b)) ? (a) : (b))

/* ************************************************************************** */

#endif /* _libmng_data_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
