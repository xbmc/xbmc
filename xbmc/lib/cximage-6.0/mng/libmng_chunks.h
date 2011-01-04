/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_chunks.h           copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Chunk structures (definition)                              * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Definition of known chunk structures                       * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/04/2000 - G.Juyn                                * */
/* *             - put in some extra comments                               * */
/* *             0.5.1 - 05/06/2000 - G.Juyn                                * */
/* *             - fixed layout for sBIT, PPLT                              * */
/* *             0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed write callback definition                        * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/11/2000 - G.Juyn                                * */
/* *             - fixed layout for PPLT again (missed deltatype ?!?)       * */
/* *                                                                        * */
/* *             0.5.2 - 05/31/2000 - G.Juyn                                * */
/* *             - removed useless definition (contributed by Tim Rowley)   * */
/* *             0.5.2 - 06/03/2000 - G.Juyn                                * */
/* *             - fixed makeup for Linux gcc compile                       * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/26/2000 - G.Juyn                                * */
/* *             - added MAGN chunk                                         * */
/* *             0.9.3 - 09/10/2000 - G.Juyn                                * */
/* *             - fixed DEFI behavior                                      * */
/* *             0.9.3 - 10/16/2000 - G.Juyn                                * */
/* *             - added JDAA chunk                                         * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - added HLAPI function to copy chunks                      * */
/* *             1.0.5 - 09/14/2002 - G.Juyn                                * */
/* *             - added event handling for dynamic MNG                     * */
/* *             1.0.5 - 11/28/2002 - G.Juyn                                * */
/* *             - fixed definition of iMethodX/Y for MAGN chunk            * */
/* *                                                                        * */
/* *             1.0.6 - 05/25/2003 - G.R-P                                 * */
/* *               added MNG_SKIPCHUNK_cHNK footprint optimizations         * */
/* *             1.0.6 - 07/29/2003 - G.R-P                                 * */
/* *             - added conditionals around PAST chunk support             * */
/* *                                                                        * */
/* *             1.0.7 - 03/24/2004 - G.R-P                                 * */
/* *             - added conditional around MNG_NO_DELTA_PNG support        * */
/* *                                                                        * */
/* *             1.0.9 - 12/05/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_CHUNKINITFREE             * */
/* *             1.0.9 - 12/06/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_CHUNKREADER               * */
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

#ifndef _libmng_chunks_h_
#define _libmng_chunks_h_

/* ************************************************************************** */

#ifdef MNG_SWAP_ENDIAN
#define PNG_SIG 0x474e5089L
#define JNG_SIG 0x474e4a8bL
#define MNG_SIG 0x474e4d8aL
#define POST_SIG 0x0a1a0a0dL
#else
#define PNG_SIG 0x89504e47L
#define JNG_SIG 0x8b4a4e47L
#define MNG_SIG 0x8a4d4e47L
#define POST_SIG 0x0d0a1a0aL
#endif

/* ************************************************************************** */

#ifdef MNG_OPTIMIZE_CHUNKREADER

typedef mng_retcode (*mng_f_specialfunc)  (mng_datap   pData,
                                           mng_chunkp  pChunk,
                                           mng_uint32* piRawlen,
                                           mng_uint8p* ppRawdata);
                                           
typedef mng_retcode (*mng_c_specialfunc)  (mng_datap  pData,
                                           mng_chunkp pChunk);

#define MNG_FIELD_OPTIONAL    0x0001
#define MNG_FIELD_TERMINATOR  0x0002
#define MNG_FIELD_REPETITIVE  0x0004
#define MNG_FIELD_DEFLATED    0x0008
#define MNG_FIELD_IFIMGTYPES  0x01F0   /* image-type mask */
#define MNG_FIELD_IFIMGTYPE0  0x0010
#define MNG_FIELD_IFIMGTYPE2  0x0020
#define MNG_FIELD_IFIMGTYPE3  0x0040
#define MNG_FIELD_IFIMGTYPE4  0x0080
#define MNG_FIELD_IFIMGTYPE6  0x0100
#define MNG_FIELD_PUTIMGTYPE  0x0200
#define MNG_FIELD_NOHIGHBIT   0x0400
#define MNG_FIELD_GROUPMASK   0x7000
#define MNG_FIELD_GROUP1      0x1000
#define MNG_FIELD_GROUP2      0x2000
#define MNG_FIELD_GROUP3      0x3000
#define MNG_FIELD_GROUP4      0x4000
#define MNG_FIELD_GROUP5      0x5000
#define MNG_FIELD_GROUP6      0x6000
#define MNG_FIELD_GROUP7      0x7000
#define MNG_FIELD_INT         0x8000

typedef struct {                       /* chunk-field descriptor */
           mng_f_specialfunc pSpecialfunc;
           mng_uint16        iFlags;
           mng_uint16        iMinvalue;
           mng_uint16        iMaxvalue;
           mng_uint16        iLengthmin;
           mng_uint16        iLengthmax;
           mng_uint16        iOffsetchunk;
           mng_uint16        iOffsetchunkind;
           mng_uint16        iOffsetchunklen;
        } mng_field_descriptor;
typedef mng_field_descriptor * mng_field_descp;

#define MNG_DESCR_GLOBAL      0x0001
#define MNG_DESCR_EMPTY       0x0002
#define MNG_DESCR_EMPTYEMBED  0x0006
#define MNG_DESCR_EMPTYGLOBAL 0x000A

#define MNG_DESCR_GenHDR      0x0001   /* IHDR/JHDR/BASI/DHDR */
#define MNG_DESCR_JngHDR      0x0002   /* JHDR/DHDR */
#define MNG_DESCR_MHDR        0x0004
#define MNG_DESCR_IHDR        0x0008
#define MNG_DESCR_JHDR        0x0010
#define MNG_DESCR_DHDR        0x0020
#define MNG_DESCR_LOOP        0x0040
#define MNG_DESCR_PLTE        0x0080
#define MNG_DESCR_SAVE        0x0100

#define MNG_DESCR_NOIHDR      0x0001
#define MNG_DESCR_NOJHDR      0x0002
#define MNG_DESCR_NOBASI      0x0004
#define MNG_DESCR_NODHDR      0x0008
#define MNG_DESCR_NOIDAT      0x0010
#define MNG_DESCR_NOJDAT      0x0020
#define MNG_DESCR_NOJDAA      0x0040
#define MNG_DESCR_NOPLTE      0x0080
#define MNG_DESCR_NOJSEP      0x0100
#define MNG_DESCR_NOMHDR      0x0200
#define MNG_DESCR_NOTERM      0x0400
#define MNG_DESCR_NOLOOP      0x0800
#define MNG_DESCR_NOSAVE      0x1000

typedef struct {                       /* chunk descriptor */
           mng_imgtype       eImgtype;
           mng_createobjtype eCreateobject;
           mng_uint16        iObjsize;
           mng_uint16        iOffsetempty;
           mng_ptr           pObjcleanup;
           mng_ptr           pObjprocess;
           mng_c_specialfunc pSpecialfunc;
           mng_field_descp   pFielddesc;
           mng_uint16        iFielddesc;
           mng_uint16        iAllowed;
           mng_uint16        iMusthaves;
           mng_uint16        iMustNOThaves;
        } mng_chunk_descriptor;
typedef mng_chunk_descriptor * mng_chunk_descp;

#endif /* MNG_OPTIMIZE_CHUNKREADER */

/* ************************************************************************** */

typedef mng_retcode (*mng_createchunk)  (mng_datap   pData,
                                         mng_chunkp  pHeader,
                                         mng_chunkp* ppChunk);

typedef mng_retcode (*mng_cleanupchunk) (mng_datap   pData,
                                         mng_chunkp  pHeader);

typedef mng_retcode (*mng_readchunk)    (mng_datap   pData,
                                         mng_chunkp  pHeader,
                                         mng_uint32  iRawlen,
                                         mng_uint8p  pRawdata,
                                         mng_chunkp* pChunk);

typedef mng_retcode (*mng_writechunk)   (mng_datap   pData,
                                         mng_chunkp  pChunk);

typedef mng_retcode (*mng_assignchunk)  (mng_datap   pData,
                                         mng_chunkp  pChunkto,
                                         mng_chunkp  pChunkfrom);

/* ************************************************************************** */

typedef struct {                       /* generic header */
           mng_chunkid       iChunkname;
           mng_createchunk   fCreate;
           mng_cleanupchunk  fCleanup;
           mng_readchunk     fRead;
           mng_writechunk    fWrite;
           mng_assignchunk   fAssign;
           mng_chunkp        pNext;    /* for double-linked list */
           mng_chunkp        pPrev;
#ifdef MNG_OPTIMIZE_CHUNKINITFREE
           mng_size_t        iChunksize;
#endif
#ifdef MNG_OPTIMIZE_CHUNKREADER
           mng_chunk_descp   pChunkdescr;
#endif
        } mng_chunk_header;
typedef mng_chunk_header * mng_chunk_headerp;

/* ************************************************************************** */

typedef struct {                       /* IHDR */
           mng_chunk_header  sHeader;
           mng_uint32        iWidth;
           mng_uint32        iHeight;
           mng_uint8         iBitdepth;
           mng_uint8         iColortype;
           mng_uint8         iCompression;
           mng_uint8         iFilter;
           mng_uint8         iInterlace;
        } mng_ihdr;
typedef mng_ihdr * mng_ihdrp;

/* ************************************************************************** */

typedef struct {                       /* PLTE */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint32        iEntrycount;
           mng_rgbpaltab     aEntries;
        } mng_plte;
typedef mng_plte * mng_pltep;

/* ************************************************************************** */

typedef struct {                       /* IDAT */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint32        iDatasize;
           mng_ptr           pData;
        } mng_idat;
typedef mng_idat * mng_idatp;

/* ************************************************************************** */

typedef struct {                       /* IEND */
           mng_chunk_header  sHeader;
        } mng_iend;
typedef mng_iend * mng_iendp;

/* ************************************************************************** */

typedef struct {                       /* tRNS */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_bool          bGlobal;
           mng_uint8         iType;    /* colortype (0,2,3) */
           mng_uint32        iCount;
           mng_uint8arr      aEntries;
           mng_uint16        iGray;
           mng_uint16        iRed;
           mng_uint16        iGreen;
           mng_uint16        iBlue;
           mng_uint32        iRawlen;
           mng_uint8arr      aRawdata;
        } mng_trns;
typedef mng_trns * mng_trnsp;

/* ************************************************************************** */

typedef struct {                       /* gAMA */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint32        iGamma;
        } mng_gama;
typedef mng_gama * mng_gamap;

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_cHRM
typedef struct {                       /* cHRM */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint32        iWhitepointx;
           mng_uint32        iWhitepointy;
           mng_uint32        iRedx;
           mng_uint32        iRedy;
           mng_uint32        iGreenx;
           mng_uint32        iGreeny;
           mng_uint32        iBluex;
           mng_uint32        iBluey;
        } mng_chrm;
typedef mng_chrm * mng_chrmp;
#endif

/* ************************************************************************** */

typedef struct {                       /* sRGB */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint8         iRenderingintent;
        } mng_srgb;
typedef mng_srgb * mng_srgbp;

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iCCP
typedef struct {                       /* iCCP */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint32        iNamesize;
           mng_pchar         zName;
           mng_uint8         iCompression;
           mng_uint32        iProfilesize;
           mng_ptr           pProfile;
        } mng_iccp;
typedef mng_iccp * mng_iccpp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tEXt
typedef struct {                       /* tEXt */
           mng_chunk_header  sHeader;
           mng_uint32        iKeywordsize;
           mng_pchar         zKeyword;
           mng_uint32        iTextsize;
           mng_pchar         zText;
        } mng_text;
typedef mng_text * mng_textp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_zTXt
typedef struct {                       /* zTXt */
           mng_chunk_header  sHeader;
           mng_uint32        iKeywordsize;
           mng_pchar         zKeyword;
           mng_uint8         iCompression;
           mng_uint32        iTextsize;
           mng_pchar         zText;
        } mng_ztxt;
typedef mng_ztxt * mng_ztxtp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_iTXt
typedef struct {                       /* iTXt */
           mng_chunk_header  sHeader;
           mng_uint32        iKeywordsize;
           mng_pchar         zKeyword;
           mng_uint8         iCompressionflag;
           mng_uint8         iCompressionmethod;
           mng_uint32        iLanguagesize;
           mng_pchar         zLanguage;
           mng_uint32        iTranslationsize;
           mng_pchar         zTranslation;
           mng_uint32        iTextsize;
           mng_pchar         zText;
        } mng_itxt;
typedef mng_itxt * mng_itxtp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_bKGD
typedef struct {                       /* bKGD */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint8         iType;    /* 3=indexed, 0=gray, 2=rgb */
           mng_uint8         iIndex;
           mng_uint16        iGray;
           mng_uint16        iRed;
           mng_uint16        iGreen;
           mng_uint16        iBlue;
        } mng_bkgd;
typedef mng_bkgd * mng_bkgdp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYs
typedef struct {                       /* pHYs */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint32        iSizex;
           mng_uint32        iSizey;
           mng_uint8         iUnit;
        } mng_phys;
typedef mng_phys * mng_physp;
#endif

/* ************************************************************************** */
#ifndef MNG_SKIPCHUNK_sBIT

typedef struct {                       /* sBIT */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint8         iType;    /* colortype (0,2,3,4,6,10,12,14,16) */
           mng_uint8arr4     aBits;
        } mng_sbit;
typedef mng_sbit * mng_sbitp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_sPLT
typedef struct {                       /* sPLT */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint32        iNamesize;
           mng_pchar         zName;
           mng_uint8         iSampledepth;
           mng_uint32        iEntrycount;
           mng_ptr           pEntries;
        } mng_splt;
typedef mng_splt * mng_spltp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_hIST
typedef struct {                       /* hIST */
           mng_chunk_header  sHeader;
           mng_uint32        iEntrycount;
           mng_uint16arr     aEntries;
        } mng_hist;
typedef mng_hist * mng_histp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_tIME
typedef struct {                       /* tIME */
           mng_chunk_header  sHeader;
           mng_uint16        iYear;
           mng_uint8         iMonth;
           mng_uint8         iDay;
           mng_uint8         iHour;
           mng_uint8         iMinute;
           mng_uint8         iSecond;
        } mng_time;
typedef mng_time * mng_timep;
#endif

/* ************************************************************************** */

typedef struct {                       /* MHDR */
           mng_chunk_header  sHeader;
           mng_uint32        iWidth;
           mng_uint32        iHeight;
           mng_uint32        iTicks;
           mng_uint32        iLayercount;
           mng_uint32        iFramecount;
           mng_uint32        iPlaytime;
           mng_uint32        iSimplicity;
        } mng_mhdr;
typedef mng_mhdr * mng_mhdrp;

/* ************************************************************************** */

typedef struct {                       /* MEND */
           mng_chunk_header  sHeader;
        } mng_mend;
typedef mng_mend * mng_mendp;

/* ************************************************************************** */

typedef struct {                       /* LOOP */
           mng_chunk_header  sHeader;
           mng_uint8         iLevel;
           mng_uint32        iRepeat;
           mng_uint8         iTermination;
           mng_uint32        iItermin;
           mng_uint32        iItermax;
           mng_uint32        iCount;
           mng_uint32p       pSignals;
        } mng_loop;
typedef mng_loop * mng_loopp;

/* ************************************************************************** */

typedef struct {                       /* ENDL */
           mng_chunk_header  sHeader;
           mng_uint8         iLevel;
        } mng_endl;
typedef mng_endl * mng_endlp;

/* ************************************************************************** */

typedef struct {                       /* DEFI */
           mng_chunk_header  sHeader;
           mng_uint16        iObjectid;
           mng_bool          bHasdonotshow;
           mng_uint8         iDonotshow;
           mng_bool          bHasconcrete;
           mng_uint8         iConcrete;
           mng_bool          bHasloca;
           mng_int32         iXlocation;
           mng_int32         iYlocation;
           mng_bool          bHasclip;
           mng_int32         iLeftcb;
           mng_int32         iRightcb;
           mng_int32         iTopcb;
           mng_int32         iBottomcb;
        } mng_defi;
typedef mng_defi * mng_defip;

/* ************************************************************************** */

typedef struct {                       /* BASI */
           mng_chunk_header  sHeader;
           mng_uint32        iWidth;
           mng_uint32        iHeight;
           mng_uint8         iBitdepth;
           mng_uint8         iColortype;
           mng_uint8         iCompression;
           mng_uint8         iFilter;
           mng_uint8         iInterlace;
           mng_uint16        iRed;
           mng_uint16        iGreen;
           mng_uint16        iBlue;
#ifdef MNG_OPTIMIZE_CHUNKREADER
           mng_bool          bHasalpha;
#endif
           mng_uint16        iAlpha;
           mng_uint8         iViewable;
        } mng_basi;
typedef mng_basi * mng_basip;

/* ************************************************************************** */

typedef struct {                       /* CLON */
           mng_chunk_header  sHeader;
           mng_uint16        iSourceid;
           mng_uint16        iCloneid;
           mng_uint8         iClonetype;
#ifdef MNG_OPTIMIZE_CHUNKREADER
           mng_bool          bHasdonotshow;
#endif
           mng_uint8         iDonotshow;
           mng_uint8         iConcrete;
           mng_bool          bHasloca;
           mng_uint8         iLocationtype;
           mng_int32         iLocationx;
           mng_int32         iLocationy;
        } mng_clon;
typedef mng_clon * mng_clonp;

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_PAST
typedef struct {                       /* PAST source */
           mng_uint16        iSourceid;
           mng_uint8         iComposition;
           mng_uint8         iOrientation;
           mng_uint8         iOffsettype;
           mng_int32         iOffsetx;
           mng_int32         iOffsety;
           mng_uint8         iBoundarytype;
           mng_int32         iBoundaryl;
           mng_int32         iBoundaryr;
           mng_int32         iBoundaryt;
           mng_int32         iBoundaryb;
        } mng_past_source;
typedef mng_past_source * mng_past_sourcep;

typedef struct {                       /* PAST */
           mng_chunk_header  sHeader;
           mng_uint16        iDestid;
           mng_uint8         iTargettype;
           mng_int32         iTargetx;
           mng_int32         iTargety;
           mng_uint32        iCount;
           mng_past_sourcep  pSources;
        } mng_past;
typedef mng_past * mng_pastp;
#endif

/* ************************************************************************** */

typedef struct {                       /* DISC */
           mng_chunk_header  sHeader;
           mng_uint32        iCount;
           mng_uint16p       pObjectids;
        } mng_disc;
typedef mng_disc * mng_discp;

/* ************************************************************************** */

typedef struct {                       /* BACK */
           mng_chunk_header  sHeader;
           mng_uint16        iRed;
           mng_uint16        iGreen;
           mng_uint16        iBlue;
           mng_uint8         iMandatory;
           mng_uint16        iImageid;
           mng_uint8         iTile;
        } mng_back;
typedef mng_back * mng_backp;

/* ************************************************************************** */

typedef struct {                       /* FRAM */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint8         iMode;
           mng_uint32        iNamesize;
           mng_pchar         zName;
           mng_uint8         iChangedelay;
           mng_uint8         iChangetimeout;
           mng_uint8         iChangeclipping;
           mng_uint8         iChangesyncid;
           mng_uint32        iDelay;
           mng_uint32        iTimeout;
           mng_uint8         iBoundarytype;
           mng_int32         iBoundaryl;
           mng_int32         iBoundaryr;
           mng_int32         iBoundaryt;
           mng_int32         iBoundaryb;
           mng_uint32        iCount;
           mng_uint32p       pSyncids;
        } mng_fram;
typedef mng_fram * mng_framp;

/* ************************************************************************** */

typedef struct {                       /* MOVE */
           mng_chunk_header  sHeader;
           mng_uint16        iFirstid;
           mng_uint16        iLastid;
           mng_uint8         iMovetype;
           mng_int32         iMovex;
           mng_int32         iMovey;
        } mng_move;
typedef mng_move * mng_movep;

/* ************************************************************************** */

typedef struct {                       /* CLIP */
           mng_chunk_header  sHeader;
           mng_uint16        iFirstid;
           mng_uint16        iLastid;
           mng_uint8         iCliptype;
           mng_int32         iClipl;
           mng_int32         iClipr;
           mng_int32         iClipt;
           mng_int32         iClipb;
        } mng_clip;
typedef mng_clip * mng_clipp;

/* ************************************************************************** */

typedef struct {                       /* SHOW */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint16        iFirstid;
#ifdef MNG_OPTIMIZE_CHUNKREADER
           mng_bool          bHaslastid;
#endif
           mng_uint16        iLastid;
           mng_uint8         iMode;
        } mng_show;
typedef mng_show * mng_showp;

/* ************************************************************************** */

typedef struct {                       /* TERM */
           mng_chunk_header  sHeader;
           mng_uint8         iTermaction;
           mng_uint8         iIteraction;
           mng_uint32        iDelay;
           mng_uint32        iItermax;
        } mng_term;
typedef mng_term * mng_termp;

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SAVE
typedef struct {                       /* SAVE entry */
           mng_uint8         iEntrytype;
           mng_uint32arr2    iOffset;            /* 0=MSI, 1=LSI */
           mng_uint32arr2    iStarttime;         /* 0=MSI, 1=LSI */
           mng_uint32        iLayernr;
           mng_uint32        iFramenr;
           mng_uint32        iNamesize;
           mng_pchar         zName;
        } mng_save_entry;
typedef mng_save_entry * mng_save_entryp;

typedef struct {                       /* SAVE */
           mng_chunk_header  sHeader;
           mng_bool          bEmpty;
           mng_uint8         iOffsettype;
           mng_uint32        iCount;
           mng_save_entryp   pEntries;
        } mng_save;
typedef mng_save * mng_savep;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_SEEK
typedef struct {                       /* SEEK */
           mng_chunk_header  sHeader;
           mng_uint32        iNamesize;
           mng_pchar         zName;
        } mng_seek;
typedef mng_seek * mng_seekp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_eXPI
typedef struct {                       /* eXPI */
           mng_chunk_header  sHeader;
           mng_uint16        iSnapshotid;
           mng_uint32        iNamesize;
           mng_pchar         zName;
        } mng_expi;
typedef mng_expi * mng_expip;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_fPRI
typedef struct {                       /* fPRI */
           mng_chunk_header  sHeader;
           mng_uint8         iDeltatype;
           mng_uint8         iPriority;
        } mng_fpri;
typedef mng_fpri * mng_fprip;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_nEED
typedef struct {                       /* nEED */
           mng_chunk_header  sHeader;
           mng_uint32        iKeywordssize;
           mng_pchar         zKeywords;
        } mng_need;
typedef mng_need * mng_needp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_pHYg
typedef mng_phys mng_phyg;             /* pHYg */
typedef mng_phyg * mng_phygp;
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

typedef struct {                       /* JHDR */
           mng_chunk_header  sHeader;
           mng_uint32        iWidth;
           mng_uint32        iHeight;
           mng_uint8         iColortype;
           mng_uint8         iImagesampledepth;
           mng_uint8         iImagecompression;
           mng_uint8         iImageinterlace;
           mng_uint8         iAlphasampledepth;
           mng_uint8         iAlphacompression;
           mng_uint8         iAlphafilter;
           mng_uint8         iAlphainterlace;
        } mng_jhdr;
typedef mng_jhdr * mng_jhdrp;

/* ************************************************************************** */

typedef mng_idat mng_jdaa;             /* JDAA */
typedef mng_jdaa * mng_jdaap;

/* ************************************************************************** */

typedef mng_idat mng_jdat;             /* JDAT */
typedef mng_jdat * mng_jdatp;

/* ************************************************************************** */

typedef struct {                       /* JSEP */
           mng_chunk_header  sHeader;
        } mng_jsep;
typedef mng_jsep * mng_jsepp;

#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifndef MNG_NO_DELTA_PNG

typedef struct {                       /* DHDR */
           mng_chunk_header  sHeader;
           mng_uint16        iObjectid;
           mng_uint8         iImagetype;
           mng_uint8         iDeltatype;
#ifdef MNG_OPTIMIZE_CHUNKREADER
           mng_bool          bHasblocksize;
#endif
           mng_uint32        iBlockwidth;
           mng_uint32        iBlockheight;
#ifdef MNG_OPTIMIZE_CHUNKREADER
           mng_bool          bHasblockloc;
#endif
           mng_uint32        iBlockx;
           mng_uint32        iBlocky;
        } mng_dhdr;
typedef mng_dhdr * mng_dhdrp;

/* ************************************************************************** */

typedef struct {                       /* PROM */
           mng_chunk_header  sHeader;
           mng_uint8         iColortype;
           mng_uint8         iSampledepth;
           mng_uint8         iFilltype;
        } mng_prom;
typedef mng_prom * mng_promp;

/* ************************************************************************** */

typedef struct {                       /* IPNG */
           mng_chunk_header  sHeader;
        } mng_ipng;
typedef mng_ipng *mng_ipngp;

/* ************************************************************************** */

typedef struct {                       /* PPLT entry */
           mng_uint8         iRed;
           mng_uint8         iGreen;
           mng_uint8         iBlue;
           mng_uint8         iAlpha;
           mng_bool          bUsed;
        } mng_pplt_entry;
typedef mng_pplt_entry * mng_pplt_entryp;

typedef struct {                       /* PPLT */
           mng_chunk_header  sHeader;
           mng_uint8         iDeltatype;
           mng_uint32        iCount;
           mng_pplt_entry    aEntries [256];
        } mng_pplt;
typedef mng_pplt * mng_ppltp;

/* ************************************************************************** */

typedef struct {                       /* IJNG */
           mng_chunk_header  sHeader;
        } mng_ijng;
typedef mng_ijng *mng_ijngp;

/* ************************************************************************** */

typedef struct {                       /* DROP */
           mng_chunk_header  sHeader;
           mng_uint32        iCount;
           mng_chunkidp      pChunknames;
        } mng_drop;
typedef mng_drop * mng_dropp;

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_DBYK
typedef struct {                       /* DBYK */
           mng_chunk_header  sHeader;
           mng_chunkid       iChunkname;
           mng_uint8         iPolarity;
           mng_uint32        iKeywordssize;
           mng_pchar         zKeywords;
        } mng_dbyk;
typedef mng_dbyk * mng_dbykp;
#endif

/* ************************************************************************** */

#ifndef MNG_SKIPCHUNK_ORDR
typedef struct {                       /* ORDR entry */
           mng_chunkid       iChunkname;
           mng_uint8         iOrdertype;
        } mng_ordr_entry;
typedef mng_ordr_entry * mng_ordr_entryp;

typedef struct mng_ordr_struct {       /* ORDR */
           mng_chunk_header  sHeader;
           mng_uint32        iCount;
           mng_ordr_entryp   pEntries;
        } mng_ordr;
typedef mng_ordr * mng_ordrp;
#endif
#endif /* MNG_NO_DELTA_PNG */

/* ************************************************************************** */

typedef struct {                       /* MAGN */
           mng_chunk_header  sHeader;
           mng_uint16        iFirstid;
           mng_uint16        iLastid;
           mng_uint8         iMethodX;
           mng_uint16        iMX;
           mng_uint16        iMY;
           mng_uint16        iML;
           mng_uint16        iMR;
           mng_uint16        iMT;
           mng_uint16        iMB;
           mng_uint8         iMethodY;
        } mng_magn;
typedef mng_magn * mng_magnp;

/* ************************************************************************** */

typedef struct {                       /* evNT entry */
           mng_uint8         iEventtype;
           mng_uint8         iMasktype;
           mng_int32         iLeft;
           mng_int32         iRight;
           mng_int32         iTop;
           mng_int32         iBottom;
           mng_uint16        iObjectid;
           mng_uint8         iIndex;
           mng_uint32        iSegmentnamesize;
           mng_pchar         zSegmentname;
        } mng_evnt_entry;
typedef mng_evnt_entry * mng_evnt_entryp;

typedef struct {                       /* evNT */
           mng_chunk_header  sHeader;
           mng_uint32        iCount;
           mng_evnt_entryp   pEntries;
        } mng_evnt;
typedef mng_evnt * mng_evntp;

/* ************************************************************************** */

#ifdef MNG_INCLUDE_MPNG_PROPOSAL
typedef struct {                       /* mpNG frame */
           mng_uint32        iX;
           mng_uint32        iY;
           mng_uint32        iWidth;
           mng_uint32        iHeight;
           mng_int32         iXoffset;
           mng_int32         iYoffset;
           mng_uint16        iTicks;
        } mng_mpng_frame;
typedef mng_mpng_frame * mng_mpng_framep;

typedef struct {                       /* mpNG */
           mng_chunk_header  sHeader;
           mng_uint32        iFramewidth;
           mng_uint32        iFrameheight;
           mng_uint16        iNumplays;
           mng_uint16        iTickspersec;
           mng_uint8         iCompressionmethod;
           mng_uint32        iFramessize;
           mng_mpng_framep   pFrames;
        } mng_mpng;
typedef mng_mpng * mng_mpngp;
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ANG_PROPOSAL
typedef struct {                       /* ahDR */
           mng_chunk_header  sHeader;
           mng_uint32        iNumframes;
           mng_uint32        iTickspersec;
           mng_uint32        iNumplays;
           mng_uint32        iTilewidth;
           mng_uint32        iTileheight;
           mng_uint8         iInterlace;
           mng_uint8         iStillused;
        } mng_ahdr;
typedef mng_ahdr * mng_ahdrp;

typedef struct {                       /* adAT tile */
           mng_uint32        iTicks;
           mng_int32         iXoffset;
           mng_int32         iYoffset;
           mng_uint8         iTilesource;
        } mng_adat_tile;
typedef mng_adat_tile * mng_adat_tilep;

typedef struct {                       /* adAT */
           mng_chunk_header  sHeader;
           mng_uint32        iTilessize;
           mng_adat_tilep    pTiles;
        } mng_adat;
typedef mng_adat * mng_adatp;
#endif

/* ************************************************************************** */

typedef struct {                       /* unknown chunk */
           mng_chunk_header  sHeader;
           mng_uint32        iDatasize;
           mng_ptr           pData;
        } mng_unknown_chunk;
typedef mng_unknown_chunk * mng_unknown_chunkp;

/* ************************************************************************** */

#endif /* _libmng_chunks_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */
