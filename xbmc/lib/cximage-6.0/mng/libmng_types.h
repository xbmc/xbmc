/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_types.h            copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : type specifications                                        * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : Specification of the types used by the library             * */
/* *             Creates platform-independant structure                     * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/06/2000 - G.Juyn                                * */
/* *             - added iteratechunk callback definition                   * */
/* *             0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - improved definitions for DLL support                     * */
/* *             - added 8-bit palette definition                           * */
/* *             - added general array definitions                          * */
/* *             - added MNG_NULL definition                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/11/2000 - G.Juyn                                * */
/* *             - changed most callback prototypes to allow the app        * */
/* *               to report errors during callback processing              * */
/* *             0.5.1 - 05/16/2000 - G.Juyn                                * */
/* *             - moved standard header includes into this file            * */
/* *               (stdlib/mem for mem-mngmt & math for fp gamma-calc)      * */
/* *                                                                        * */
/* *             0.5.2 - 05/18/2000 - G.Juyn                                * */
/* *             - B003 - fixed problem with <mem.h> being proprietary      * */
/* *               to Borland platform                                      * */
/* *             - added helper definitions for JNG (IJG-based)             * */
/* *             - fixed support for IJGSRC6B                               * */
/* *             0.5.2 - 05/24/2000 - G.Juyn                                * */
/* *             - added default IJG compression parameters and such        * */
/* *             0.5.2 - 05/31/2000 - G.Juyn                                * */
/* *             - fixed inclusion for memcpy (contributed by Tim Rowley)   * */
/* *             - added mng_int32p (contributed by Tim Rowley)             * */
/* *             0.5.2 - 06/02/2000 - G.Juyn                                * */
/* *             - removed SWAP_ENDIAN reference (contributed by Tim Rowley)* */
/* *             - added getalphaline callback for RGB8_A8 canvasstyle      * */
/* *                                                                        * */
/* *             0.5.3 - 06/21/2000 - G.Juyn                                * */
/* *             - added speedtype to facilitate testing                    * */
/* *             0.5.3 - 06/27/2000 - G.Juyn                                * */
/* *             - added typedef for mng_size_t                             * */
/* *             - changed size parameter for memory callbacks to           * */
/* *               mng_size_t                                               * */
/* *             0.5.3 - 06/28/2000 - G.Juyn                                * */
/* *             - changed definition of 32-bit ints (64-bit platforms)     * */
/* *             - changed definition of mng_handle (64-bit platforms)      * */
/* *             0.5.3 - 06/29/2000 - G.Juyn                                * */
/* *             - changed definition of mng_handle (again)                 * */
/* *             - swapped refresh parameters                               * */
/* *             - added inclusion of stdlib.h for abs()                    * */
/* *                                                                        * */
/* *             0.9.0 - 06/30/2000 - G.Juyn                                * */
/* *             - changed refresh parameters to 'x,y,width,height'         * */
/* *             0.9.1 - 07/10/2000 - G.Juyn                                * */
/* *             - added suspendbuffer constants                            * */
/* *             0.9.1 - 07/15/2000 - G.Juyn                                * */
/* *             - added callbacks for SAVE/SEEK processing                 * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/07/2000 - G.Juyn                                * */
/* *             - B111300 - fixup for improved portability                 * */
/* *             0.9.3 - 08/12/2000 - G.Juyn                                * */
/* *             - added workaround for faulty PhotoShop iCCP chunk         * */
/* *             0.9.3 - 09/11/2000 - G.Juyn                                * */
/* *             - added export of zlib functions from windows dll          * */
/* *             - fixed inclusion parameters once again to make those      * */
/* *               external libs work together                              * */
/* *             - re-fixed fixed inclusion parameters                      * */
/* *               (these freeking libraries make me mad)                   * */
/* *             0.9.3 - 10/11/2000 - G.Juyn                                * */
/* *             - added support for nEED                                   * */
/* *             0.9.3 - 10/17/2000 - G.Juyn                                * */
/* *             - added callback to process non-critical unknown chunks    * */
/* *                                                                        * */
/* *             0.9.4 - 11/20/2000 - R.Giles                               * */
/* *             - fixed inclusion of lcms header for non-windows platforms * */
/* *             0.9.4 - 12/12/2000 - G.Juyn                                * */
/* *             - changed callback convention for MSVC (Thanks Chad)       * */
/* *             0.9.4 - 12/16/2000 - G.Juyn                                * */
/* *             - fixed mixup of data- & function-pointers (thanks Dimitri)* */
/* *                                                                        * */
/* *             1.0.1 - 02/08/2001 - G.Juyn                                * */
/* *             - added MEND processing callback                           * */
/* *                                                                        * */
/* *             1.0.2 - 06/23/2001 - G.Juyn                                * */
/* *             - added processterm callback                               * */
/* *                                                                        * */
/* *             1.0.3 - 08/06/2001 - G.Juyn                                * */
/* *             - changed inclusion of lcms.h for Linux platforms          * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *                                                                        * */
/* *             1.0.6 - 04/11/2003 - G.Juyn                                * */
/* *             - B719420 - fixed several MNG_APP_CMS problems             * */
/* *             1.0.6 - 06/15/2003 - R.Giles                               * */
/* *             - lcms.h inclusion is generally no longer prefixed         * */
/* *             1.0.6 - 07/07/2003 - G. R-P.                               * */
/* *             - added png_imgtypes enumeration                           * */
/* *                                                                        * */
/* *             1.0.7 - 03/10/2004 - G.R-P                                 * */
/* *             - added conditionals around openstream/closestream         * */
/* *                                                                        * */
/* *             1.0.8 - 04/11/2004 - G.Juyn                                * */
/* *             - added data-push mechanisms for specialized decoders      * */
/* *             1.0.8 - 08/01/2004 - G.Juyn                                * */
/* *             - added support for 3+byte pixelsize for JPEG's            * */
/* *                                                                        * */
/* *             1.0.9 - 12/05/2004 - G.Juyn                                * */
/* *             - inclusion of zlib/lcms/ijgsrc6b with <> instead of ""    * */
/* *             1.0.9 - 12/06/2004 - G.Juyn                                * */
/* *             - added conditional MNG_OPTIMIZE_CHUNKREADER               * */
/* *                                                                        * */
/* *             1.0.10 - 04/08/2007 - G.Juyn                               * */
/* *             - added support for mPNG proposal                          * */
/* *             1.0.10 - 04/12/2007 - G.Juyn                               * */
/* *             - added support for ANG proposal                           * */
/* *                                                                        * */
/* ************************************************************************** */

#ifndef _libmng_types_h_
#define _libmng_types_h_

/* ************************************************************************** */

#ifdef __BORLANDC__
#pragma option -AT                     /* turn off strict ANSI-C for the moment */
#endif

#ifndef WIN32
#if defined(_WIN32) || defined(__WIN32__) || defined(_Windows) || defined(_WINDOWS)
#define WIN32                          /* gather them into a single define */
#endif
#endif

/* ************************************************************************** */
/* *                                                                        * */
/* * Here's where the external & standard libs are embedded                 * */
/* *                                                                        * */
/* * (it can be a bit of a pain in the lower-back to get them to work       * */
/* *  together)                                                             * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef WIN32                           /* only include needed stuff */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#ifdef MNG_USE_DLL
#ifdef MNG_SKIP_ZLIB
#undef MNG_INCLUDE_ZLIB
#endif
#ifdef MNG_SKIP_LCMS
#undef MNG_INCLUDE_LCMS
#endif
#ifdef MNG_SKIP_IJG6B
#undef MNG_INCLUDE_IJG6B
#endif
#endif

#ifdef MNG_INCLUDE_ZLIB                /* zlib by Mark Adler & Jean-loup Gailly */
#include "../zlib/zlib.h"
#endif

#ifdef MNG_INCLUDE_LCMS                /* little cms by Marti Maria Saguer */
#ifndef ZLIB_DLL
#undef FAR
#endif
#include <lcms.h>
#endif /* MNG_INCLUDE_LCMS */

#ifdef MNG_INCLUDE_IJG6B               /* IJG's jpgsrc6b */
#include <stdio.h>
#ifdef MNG_USE_SETJMP
#include <setjmp.h>                    /* needed for error-recovery (blergh) */
#else
#ifdef WIN32
#define USE_WINDOWS_MESSAGEBOX         /* display a messagebox under Windoze */
#endif
#endif /* MNG_USE_SETJMP */
#ifdef FAR
#undef FAR                             /* possibly defined by zlib or lcms */
#endif
#define JPEG_INTERNAL_OPTIONS          /* for RGB_PIXELSIZE */
#include "../jpeg/jpeglib.h"                   /* all that for JPEG support  :-) */
#endif /* MNG_INCLUDE_IJG6B */

#if defined(MNG_INTERNAL_MEMMNGMT) || defined(MNG_INCLUDE_FILTERS)
#include <stdlib.h>                    /* "calloc" & "free" & "abs" */
#endif

#include <limits.h>                    /* get proper integer widths */

#ifdef WIN32
#if defined __BORLANDC__
#include <mem.h>                       /* defines "memcpy" for BCB */
#else
#include <memory.h>                    /* defines "memcpy" for other win32 platforms */
#endif
#include <string.h>                    /* "strncmp" + "strcmp" */
#else /* WIN32 */
#ifdef BSD
#include <strings.h>                   /* defines "memcpy", etc for BSD (?) */
#else
#include <string.h>                    /* defines "memcpy", etc for all others (???) */
#endif
#endif /* WIN32 */

#if defined(MNG_FULL_CMS) || defined(MNG_GAMMA_ONLY) || defined(MNG_APP_CMS)
#include <math.h>                      /* fp gamma-calculation */
#endif

/* ************************************************************************** */
/* *                                                                        * */
/* * Platform-dependant stuff                                               * */
/* *                                                                        * */
/* ************************************************************************** */

/* TODO: this may require some elaboration for other platforms;
   only works with BCB for now */

#ifndef MNG_DLL
#if defined(MNG_BUILD_DLL) || defined(MNG_USE_DLL)
#define MNG_DLL
#endif
#endif

#define MNG_LOCAL static

#if defined(MNG_DLL) && defined(WIN32) /* setup DLL calling conventions */ 
#define MNG_DECL __stdcall
#if defined(MNG_BUILD_DLL)
#define MNG_EXT __declspec(dllexport)
#elif defined(MNG_USE_DLL)
#define MNG_EXT __declspec(dllimport)
#else
#define MNG_EXT
#endif
#ifdef MNG_STRICT_ANSI
#undef MNG_STRICT_ANSI                 /* can't do strict-ANSI with this DLL-stuff */
#endif
#else
#define MNG_DECL                       /* dummies for non-DLL */
#define MNG_EXT
#endif /* MNG_DLL && WIN32 */

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* now force ANSI-C from here on */
#endif

/* ************************************************************************** */

#if USHRT_MAX == 0xffffffffU                     /* get the proper 32-bit width !!! */
typedef unsigned short   mng_uint32;
typedef signed   short   mng_int32;
#elif UINT_MAX == 0xffffffffU
typedef unsigned int     mng_uint32;
typedef signed   int     mng_int32;
#elif ULONG_MAX == 0xffffffffU
typedef unsigned long    mng_uint32;
typedef signed   long    mng_int32;
#else
#error "Sorry, I can't find any 32-bit integers on this platform."
#endif

typedef signed   short   mng_int16;              /* other basic integers */
typedef unsigned short   mng_uint16;
typedef signed   char    mng_int8;
typedef unsigned char    mng_uint8;

typedef double           mng_float;              /* basic float */

typedef size_t           mng_size_t;             /* size field for memory allocation */

typedef char *           mng_pchar;              /* string */
typedef void *           mng_ptr;                /* generic pointer */
typedef void             (*mng_fptr) (void);     /* generic function pointer */

/* ************************************************************************** */
/* *                                                                        * */
/* * Platform-independant from here                                         * */
/* *                                                                        * */
/* ************************************************************************** */

typedef mng_uint32 *     mng_uint32p;            /* pointer to unsigned longs */
typedef mng_int32 *      mng_int32p;             /* pointer to longs */
typedef mng_uint16 *     mng_uint16p;            /* pointer to unsigned words */
typedef mng_uint8 *      mng_uint8p;             /* pointer to unsigned bytes */

typedef mng_int8         mng_bool;               /* booleans */

struct mng_data_struct;
typedef struct mng_data_struct * mng_handle;     /* generic handle */

typedef mng_int32        mng_retcode;            /* generic return code */
typedef mng_int32        mng_chunkid;            /* 4-byte chunkname identifier */
typedef mng_ptr          mng_chunkp;             /* pointer to a chunk-structure */
typedef mng_ptr          mng_objectp;            /* pointer to an object-structure */

typedef mng_chunkid *    mng_chunkidp;           /* pointer to chunkid */

typedef struct {                                 /* 8-bit palette element */
          mng_uint8 iRed;
          mng_uint8 iGreen;
          mng_uint8 iBlue;
        } mng_palette8e;
typedef mng_palette8e   mng_palette8[256];       /* 8-bit palette */
typedef mng_palette8e * mng_palette8ep;

typedef mng_uint8       mng_uint8arr[256];       /* generic arrays */
typedef mng_uint8       mng_uint8arr4[4];
typedef mng_uint16      mng_uint16arr[256];
typedef mng_uint32      mng_uint32arr2[2];

/* ************************************************************************** */

#define MNG_FALSE 0
#define MNG_TRUE  1
#define MNG_NULL  0

#define MNG_SUSPENDBUFFERSIZE  32768
#define MNG_SUSPENDREQUESTSIZE  1024

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ZLIB

/* size of temporary zlib buffer for deflate processing */
#define MNG_ZLIB_MAXBUF     8192

/* default zlib compression parameters for deflateinit2 */
#define MNG_ZLIB_LEVEL      9                    /* level */
#define MNG_ZLIB_METHOD     Z_DEFLATED           /* method */
#define MNG_ZLIB_WINDOWBITS 15                   /* window size */
#define MNG_ZLIB_MEMLEVEL   9                    /* memory level */
#define MNG_ZLIB_STRATEGY   Z_DEFAULT_STRATEGY   /* strategy */

#define MNG_MAX_IDAT_SIZE   4096                 /* maximum size of IDAT data */

#endif /* MNG_INCLUDE_ZLIB */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG

#ifdef MNG_INCLUDE_IJG6B                         /* IJG helper defs */
typedef struct jpeg_compress_struct   mngjpeg_comp;
typedef struct jpeg_decompress_struct mngjpeg_decomp;
typedef struct jpeg_error_mgr         mngjpeg_error;
typedef struct jpeg_source_mgr        mngjpeg_source;

typedef mngjpeg_comp   * mngjpeg_compp;
typedef mngjpeg_decomp * mngjpeg_decompp;
typedef mngjpeg_error  * mngjpeg_errorp;
typedef mngjpeg_source * mngjpeg_sourcep;

typedef J_DCT_METHOD     mngjpeg_dctmethod;

/* default IJG parameters for compression */
#define MNG_JPEG_DCT         JDCT_DEFAULT        /* DCT algorithm (JDCT_ISLOW) */
#define MNG_JPEG_QUALITY     100                 /* quality 0..100; 100=best */
#define MNG_JPEG_SMOOTHING   0                   /* default no smoothing */
#define MNG_JPEG_PROGRESSIVE MNG_FALSE           /* default is just baseline */
#define MNG_JPEG_OPTIMIZED   MNG_FALSE           /* default is not optimized */
#endif /* MNG_INCLUDE_IJG6B */

#define MNG_JPEG_MAXBUF      65500               /* max size of temp JPEG buffer */
#define MNG_MAX_JDAT_SIZE    4096                /* maximum size of JDAT data */

#endif /* MNG_INCLUDE_JNG */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_LCMS
typedef cmsHPROFILE         mng_cmsprof;         /* little CMS helper defs */
typedef cmsHTRANSFORM       mng_cmstrans;
typedef cmsCIExyY           mng_CIExyY;
typedef cmsCIExyYTRIPLE     mng_CIExyYTRIPLE;
typedef LPGAMMATABLE        mng_gammatabp;
#endif /* MNG_INCLUDE_LCMS */

/* ************************************************************************** */

                                       /* enumeration of known graphics types */
enum mng_imgtypes {mng_it_unknown, mng_it_png, mng_it_mng, mng_it_jng
#ifdef MNG_INCLUDE_MPNG_PROPOSAL
     ,mng_it_mpng
#endif     
#ifdef MNG_INCLUDE_ANG_PROPOSAL
     ,mng_it_ang
#endif
     };
typedef enum mng_imgtypes mng_imgtype;

                                       /* enumeration of animation speed-types */
enum mng_speedtypes {mng_st_normal, mng_st_fast, mng_st_slow, mng_st_slowest};
typedef enum mng_speedtypes mng_speedtype;

#ifdef MNG_OPTIMIZE_CHUNKREADER
                                       /* enumeration object-creation indicators */
enum mng_createobjtypes {mng_create_none, mng_create_always, mng_create_ifglobal};
typedef enum mng_createobjtypes mng_createobjtype;
#endif

/* ************************************************************************** */

/* enumeration of PNG image types */
#ifdef MNG_OPTIMIZE_FOOTPRINT_INIT
enum png_imgtypes
  {
    png_g1,
    png_g2,
    png_g4,
    png_g8,
    png_rgb8,
    png_idx1,
    png_idx2,
    png_idx4,
    png_idx8,
    png_ga8,
    png_rgba8,
#ifdef MNG_INCLUDE_JNG
    png_jpeg_a1,
    png_jpeg_a2,
    png_jpeg_a4,
    png_jpeg_a8,
#endif
#ifndef MNG_NO_16BIT_SUPPORT
    png_g16,
    png_ga16,
    png_rgb16,
    png_rgba16,
#ifdef MNG_INCLUDE_JNG
    png_jpeg_a16,
#endif
#endif
    png_none
  };
    
typedef enum png_imgtypes png_imgtype;
#endif
/* ************************************************************************** */

                                       /* memory management callbacks */
typedef mng_ptr    (MNG_DECL *mng_memalloc)      (mng_size_t  iLen);
typedef void       (MNG_DECL *mng_memfree)       (mng_ptr     iPtr,
                                                  mng_size_t  iLen);

typedef void       (MNG_DECL *mng_releasedata)   (mng_ptr     pUserdata,
                                                  mng_ptr     pData,
                                                  mng_size_t  iLength);

                                       /* I/O management callbacks */
#ifndef MNG_NO_OPEN_CLOSE_STREAM
typedef mng_bool   (MNG_DECL *mng_openstream)    (mng_handle  hHandle);
typedef mng_bool   (MNG_DECL *mng_closestream)   (mng_handle  hHandle);
#endif
typedef mng_bool   (MNG_DECL *mng_readdata)      (mng_handle  hHandle,
                                                  mng_ptr     pBuf,
                                                  mng_uint32  iBuflen,
                                                  mng_uint32p pRead);
typedef mng_bool   (MNG_DECL *mng_writedata)     (mng_handle  hHandle,
                                                  mng_ptr     pBuf,
                                                  mng_uint32  iBuflen,
                                                  mng_uint32p pWritten);

                                       /* error & trace processing callbacks */
typedef mng_bool   (MNG_DECL *mng_errorproc)     (mng_handle  hHandle,
                                                  mng_int32   iErrorcode,
                                                  mng_int8    iSeverity,
                                                  mng_chunkid iChunkname,
                                                  mng_uint32  iChunkseq,
                                                  mng_int32   iExtra1,
                                                  mng_int32   iExtra2,
                                                  mng_pchar   zErrortext);
typedef mng_bool   (MNG_DECL *mng_traceproc)     (mng_handle  hHandle,
                                                  mng_int32   iFuncnr,
                                                  mng_int32   iFuncseq,
                                                  mng_pchar   zFuncname);

                                       /* read processing callbacks */
typedef mng_bool   (MNG_DECL *mng_processheader) (mng_handle  hHandle,
                                                  mng_uint32  iWidth,
                                                  mng_uint32  iHeight);
typedef mng_bool   (MNG_DECL *mng_processtext)   (mng_handle  hHandle,
                                                  mng_uint8   iType,
                                                  mng_pchar   zKeyword,
                                                  mng_pchar   zText,
                                                  mng_pchar   zLanguage,
                                                  mng_pchar   zTranslation);
typedef mng_bool   (MNG_DECL *mng_processsave)   (mng_handle  hHandle);
typedef mng_bool   (MNG_DECL *mng_processseek)   (mng_handle  hHandle,
                                                  mng_pchar   zName);
typedef mng_bool   (MNG_DECL *mng_processneed)   (mng_handle  hHandle,
                                                  mng_pchar   zKeyword);
typedef mng_bool   (MNG_DECL *mng_processmend)   (mng_handle  hHandle,
                                                  mng_uint32  iIterationsdone,
                                                  mng_uint32  iIterationsleft);
typedef mng_bool   (MNG_DECL *mng_processunknown) (mng_handle  hHandle,
                                                   mng_chunkid iChunkid,
                                                   mng_uint32  iRawlen,
                                                   mng_ptr     pRawdata);
typedef mng_bool   (MNG_DECL *mng_processterm)   (mng_handle  hHandle,
                                                  mng_uint8   iTermaction,
                                                  mng_uint8   iIteraction,
                                                  mng_uint32  iDelay,
                                                  mng_uint32  iItermax);

                                       /* display processing callbacks */
typedef mng_ptr    (MNG_DECL *mng_getcanvasline) (mng_handle  hHandle,
                                                  mng_uint32  iLinenr);
typedef mng_ptr    (MNG_DECL *mng_getbkgdline)   (mng_handle  hHandle,
                                                  mng_uint32  iLinenr);
typedef mng_ptr    (MNG_DECL *mng_getalphaline)  (mng_handle  hHandle,
                                                  mng_uint32  iLinenr);
typedef mng_bool   (MNG_DECL *mng_refresh)       (mng_handle  hHandle,
                                                  mng_uint32  iX,
                                                  mng_uint32  iY,
                                                  mng_uint32  iWidth,
                                                  mng_uint32  iHeight);

                                       /* timer management callbacks */
typedef mng_uint32 (MNG_DECL *mng_gettickcount)  (mng_handle  hHandle);
typedef mng_bool   (MNG_DECL *mng_settimer)      (mng_handle  hHandle,
                                                  mng_uint32  iMsecs);

                                       /* color management callbacks */
typedef mng_bool   (MNG_DECL *mng_processgamma)  (mng_handle  hHandle,
                                                  mng_uint32  iGamma);
typedef mng_bool   (MNG_DECL *mng_processchroma) (mng_handle  hHandle,
                                                  mng_uint32  iWhitepointx,
                                                  mng_uint32  iWhitepointy,
                                                  mng_uint32  iRedx,
                                                  mng_uint32  iRedy,
                                                  mng_uint32  iGreenx,
                                                  mng_uint32  iGreeny,
                                                  mng_uint32  iBluex,
                                                  mng_uint32  iBluey);
typedef mng_bool   (MNG_DECL *mng_processsrgb)   (mng_handle  hHandle,
                                                  mng_uint8   iRenderingintent);
typedef mng_bool   (MNG_DECL *mng_processiccp)   (mng_handle  hHandle,
                                                  mng_uint32  iProfilesize,
                                                  mng_ptr     pProfile);
typedef mng_bool   (MNG_DECL *mng_processarow)   (mng_handle  hHandle,
                                                  mng_uint32  iRowsamples,
                                                  mng_bool    bIsRGBA16,
                                                  mng_ptr     pRow);

                                       /* chunk access callback(s) */
typedef mng_bool   (MNG_DECL *mng_iteratechunk)  (mng_handle  hHandle,
                                                  mng_handle  hChunk,
                                                  mng_chunkid iChunkid,
                                                  mng_uint32  iChunkseq);

/* ************************************************************************** */

#endif /* _libmng_types_h_ */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

