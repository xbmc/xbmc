/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_error.c            copyright (c) 2000-2007 G.Juyn   * */
/* * version   : 1.0.10                                                     * */
/* *                                                                        * */
/* * purpose   : Error routines (implementation)                            * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the general error handling routines      * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *                                                                        * */
/* *             0.5.2 - 05/23/2000 - G.Juyn                                * */
/* *             - added error telltaling                                   * */
/* *             0.5.2 - 05/30/2000 - G.Juyn                                * */
/* *             - added errorstrings for delta-image processing            * */
/* *             0.5.2 - 05/31/2000 - G.Juyn                                * */
/* *             - fixed up punctuation (contributed by Tim Rowley)         * */
/* *             0.5.2 - 06/06/2000 - G.Juyn                                * */
/* *             - added errorstring for delayed buffer-processing          * */
/* *                                                                        * */
/* *             0.9.1 - 07/06/2000 - G.Juyn                                * */
/* *             - added MNG_NEEDTIMERWAIT errorstring                      * */
/* *             0.9.1 - 07/15/2000 - G.Juyn                                * */
/* *             - added NEEDSECTIONWAIT errorstring                        * */
/* *             - added macro + routine to set returncode without          * */
/* *               calling error callback                                   * */
/* *             0.9.1 - 07/19/2000 - G.Juyn                                * */
/* *             - added errorstring for updatemngheader if not a MNG       * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/09/2000 - G.Juyn                                * */
/* *             - added check for simplicity-bits in MHDR                  * */
/* *             0.9.3 - 10/11/2000 - G.Juyn                                * */
/* *             - fixed processing of unknown critical chunks              * */
/* *             - added support for nEED                                   * */
/* *             0.9.3 - 10/20/2000 - G.Juyn                                * */
/* *             - added errorcode for delayed delta-processing             * */
/* *                                                                        * */
/* *             0.9.4 - 01/18/2001 - G.Juyn                                * */
/* *             - added errorcode for MAGN methods                         * */
/* *                                                                        * */
/* *             1.0.2 - 06/23/2001 - G.Juyn                                * */
/* *             - added optimization option for MNG-video playback         * */
/* *                                                                        * */
/* *             1.0.5 - 07/04/2002 - G.Juyn                                * */
/* *             - added errorcode for extreme chunk-sizes                  * */
/* *             1.0.5 - 08/15/2002 - G.Juyn                                * */
/* *             - completed delta-image support                            * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             1.0.5 - 09/14/2002 - G.Juyn                                * */
/* *             - added event handling for dynamic MNG                     * */
/* *             1.0.5 - 09/15/2002 - G.Juyn                                * */
/* *             - fixed LOOP iteration=0 special case                      * */
/* *             1.0.5 - 09/19/2002 - G.Juyn                                * */
/* *             - warnings are ignored by default now!                     * */
/* *             1.0.5 - 09/20/2002 - G.Juyn                                * */
/* *             - added support for PAST                                   * */
/* *             1.0.5 - 10/07/2002 - G.Juyn                                * */
/* *             - added check for TERM placement during create/write       * */
/* *                                                                        * */
/* *             1.0.6 - 07/07/2003 - G. R-P                                * */
/* *             - added MNG_SKIPCHUNK_CHNK, MNG_NO_DELTA_PNG reductions.   * */
/* *             - skipped more code when MNG_INCLUDE_JNG is not enabled.   * */
/* *             1.0.6 - 07/29/2003 - G.R-P                                 * */
/* *             - added conditional around evNT chunk support              * */
/* *                                                                        * */
/* *             1.0.7 - 03/24/2004 - G.R-P                                 * */
/* *             - fixed typo on SKIPCHUNK_evNT (->PAST)                    * */
/* *                                                                        * */
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

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_ERROR_STRINGS
MNG_LOCAL mng_error_entry const error_table [] =
  {
    {MNG_NOERROR,          "No error"},
    {MNG_OUTOFMEMORY,      "Out of memory"},
    {MNG_INVALIDHANDLE,    "The handle is invalid"},
    {MNG_NOCALLBACK,       "A required callback is not defined"},
    {MNG_UNEXPECTEDEOF,    "Encountered unexpected end-of-file"},
    {MNG_ZLIBERROR,        "zlib encountered an error"},
#ifdef MNG_INCLUDE_JNG
    {MNG_JPEGERROR,        "ijgsrc6b encountered an error"},
#endif
    {MNG_LCMSERROR,        "lcms encountered an error"},
    {MNG_NOOUTPUTPROFILE,  "No output-profile defined for CMS"},
    {MNG_NOSRGBPROFILE,    "No sRGB-profile defined for CMS"},
    {MNG_BUFOVERFLOW,      "Internal buffer-overflow"},
    {MNG_FUNCTIONINVALID,  "Function is invalid at this point"},
    {MNG_OUTPUTERROR,      "Writing was unsuccessful; disk full?"},
    {MNG_JPEGBUFTOOSMALL,  "Internal buffer for JPEG processing too small"},
    {MNG_NEEDMOREDATA,     "Reading suspended; waiting for I/O to catch up"},
    {MNG_NEEDTIMERWAIT,    "Timer suspension; normal animation delay"},
    {MNG_NEEDSECTIONWAIT,  "SEEK suspension; application decides"},
    {MNG_LOOPWITHCACHEOFF, "LOOP encountered when playback cache is turned off"},

    {MNG_APPIOERROR,       "Application signalled I/O error"},
    {MNG_APPTIMERERROR,    "Application signalled timing error"},
    {MNG_APPCMSERROR,      "Application signalled CMS error"},
    {MNG_APPMISCERROR,     "Application signalled an error"},
    {MNG_APPTRACEABORT,    "Application signalled error during trace-callback"},

    {MNG_INTERNALERROR,    "Internal error in libmng"},

    {MNG_INVALIDSIG,       "The signature is invalid"},
    {MNG_INVALIDCRC,       "The CRC for this chunk is invalid"},
    {MNG_INVALIDLENGTH,    "Chunk-length is invalid"},
    {MNG_SEQUENCEERROR,    "Chunk out of sequence"},
    {MNG_CHUNKNOTALLOWED,  "Chunk not allowed at this point"},
    {MNG_MULTIPLEERROR,    "Chunk cannot occur multiple times"},
    {MNG_PLTEMISSING,      "Missing PLTE chunk"},
    {MNG_IDATMISSING,      "Missing IDAT chunk(s)"},
    {MNG_CANNOTBEEMPTY,    "Chunk cannot be empty"},
    {MNG_GLOBALLENGTHERR,  "Global data length invalid"},
    {MNG_INVALIDBITDEPTH,  "The bit_depth is invalid"},
    {MNG_INVALIDCOLORTYPE, "The color_type is invalid"},
    {MNG_INVALIDCOMPRESS,  "The compression_method is invalid"},
    {MNG_INVALIDFILTER,    "The filter_method or filter_type is invalid"},
    {MNG_INVALIDINTERLACE, "The interlace_method is invalid"},
    {MNG_NOTENOUGHIDAT,    "There is not enough data in the IDAT chunk(s)"},
    {MNG_PLTEINDEXERROR,   "Palette-index out of bounds"},
    {MNG_NULLNOTFOUND,     "NULL separator not found"},
    {MNG_KEYWORDNULL,      "Keyword cannot be zero-length"},
    {MNG_OBJECTUNKNOWN,    "Object does not exist"},
    {MNG_OBJECTEXISTS,     "Object already exists"},
    {MNG_TOOMUCHIDAT,      "Too much data in IDAT chunk(s)"},
    {MNG_INVSAMPLEDEPTH,   "The sample_depth is invalid"},
    {MNG_INVOFFSETSIZE,    "The offset_type is invalid"},
    {MNG_INVENTRYTYPE,     "The entry_type is invalid"},
    {MNG_ENDWITHNULL,      "Chunk must not end with NULL byte"},
    {MNG_INVIMAGETYPE,     "The image_type is invalid"},
#ifndef MNG_NO_DELTA_PNG
    {MNG_INVDELTATYPE,     "The delta_type is invalid"},
#endif
    {MNG_INVALIDINDEX,     "Index-value out of bounds"},
#ifdef MNG_INCLUDE_JNG
    {MNG_TOOMUCHJDAT,      "Too much data in JDAT chunk(s)"},
    {MNG_JPEGPARMSERR,     "JHDR parameters & JFIF-data do not match"},
#endif
    {MNG_INVFILLMETHOD,    "The fill_method is invalid"},
#ifndef MNG_NO_DELTA_PNG
    {MNG_OBJNOTCONCRETE,   "Target object for DHDR must be concrete"},
#endif
    {MNG_TARGETNOALPHA,    "Target object must have alpha-channel"},
    {MNG_MNGTOOCOMPLEX,    "MHDR simplicity indicates unsupported feature(s)"},
    {MNG_UNKNOWNCRITICAL,  "Unknown critical chunk encountered"},
#ifndef MNG_SKIPCHUNK_nEED
    {MNG_UNSUPPORTEDNEED,  "Requested nEED resources are not supported"},
#endif
    {MNG_INVALIDDELTA,     "The delta operation is invalid (mismatched color_types?)"},
    {MNG_INVALIDMETHOD,    "Method is invalid"},
    {MNG_IMPROBABLELENGTH, "Chunklength is incredibly large"},
    {MNG_INVALIDBLOCK,     "Delta block width and or height invalid"},
    {MNG_INVALIDEVENT,     "Event type is invalid"},
    {MNG_INVALIDMASK,      "Mask type is invalid"},
    {MNG_NOMATCHINGLOOP,   "ENDL without matching LOOP"},
#ifndef MNG_SKIPCHUNK_evNT
    {MNG_SEEKNOTFOUND,     "evNT points to unknown SEEK"},
#endif
#ifndef MNG_SKIPCHUNK_PAST
    {MNG_OBJNOTABSTRACT,   "Destination object for PAST must be abstract"},
#endif
    {MNG_TERMSEQERROR,     "TERM misplaced during creation of MNG stream"},
    {MNG_INVALIDFIELDVAL,  "invalid fieldvalue (generic)"},
    {MNG_INVALIDWIDTH,     "invalid frame/image width"},
    {MNG_INVALIDHEIGHT,    "invalid frame/image height"},

    {MNG_INVALIDCNVSTYLE,  "Canvas_style is invalid"},
    {MNG_WRONGCHUNK,       "Attempt to access the wrong chunk"},
    {MNG_INVALIDENTRYIX,   "Attempt to access an non-existing entry"},
    {MNG_NOHEADER,         "No valid header-chunk"},
    {MNG_NOCORRCHUNK,      "Parent chunk not found"},
    {MNG_NOMHDR,           "No MNG header (MHDR) found"},

    {MNG_IMAGETOOLARGE,    "Image is larger than defined maximum"},
    {MNG_NOTANANIMATION,   "Image is not an animation"},
    {MNG_FRAMENRTOOHIGH,   "Framenr out of bounds"},
    {MNG_LAYERNRTOOHIGH,   "Layernr out of bounds"},
    {MNG_PLAYTIMETOOHIGH,  "Playtime out of bounds"},
    {MNG_FNNOTIMPLEMENTED, "Function not yet implemented"},
    {MNG_IMAGEFROZEN,      "Image is frozen"},

    {MNG_LCMS_NOHANDLE,    "Handle could not be initialized"},
    {MNG_LCMS_NOMEM,       "No memory for gamma-table(s)"},
    {MNG_LCMS_NOTRANS,     "Transformation could not be initialized"}
  };
#endif /* MNG_INCLUDE_ERROR_STRINGS */

/* ************************************************************************** */

mng_bool mng_store_error (mng_datap   pData,
                          mng_retcode iError,
                          mng_retcode iExtra1,
                          mng_retcode iExtra2)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEB (pData, MNG_FN_STORE_ERROR, MNG_LC_START);
#endif

  if (pData != 0)
  {
    pData->iErrorcode = iError;        /* save also for getlasterror */
    pData->iErrorx1   = iExtra1;
    pData->iErrorx2   = iExtra2;

#ifdef MNG_INCLUDE_ERROR_STRINGS
    {                                  /* binary search variables */
      mng_int32        iTop, iLower, iUpper, iMiddle;
      mng_error_entryp pEntry;         /* pointer to found entry */
                                       /* determine max index of table */
      iTop = (sizeof (error_table) / sizeof (error_table [0])) - 1;

      iLower  = 0;                     /* initialize binary search */
      iMiddle = iTop >> 1;             /* start in the middle */
      iUpper  = iTop;
      pEntry  = 0;                     /* no goods yet! */

      do                               /* the binary search itself */
        {
          if (error_table [iMiddle].iError < iError)
            iLower = iMiddle + 1;
          else if (error_table [iMiddle].iError > iError)
            iUpper = iMiddle - 1;
          else
          {
            pEntry = &error_table [iMiddle];
            break;
          }

          iMiddle = (iLower + iUpper) >> 1;
        }
      while (iLower <= iUpper);

      if (pEntry)                      /* found it ? */
        pData->zErrortext = pEntry->zErrortext;
      else
        pData->zErrortext = "Unknown error";
      }
#else /* MNG_INCLUDE_ERROR_STRINGS */
    pData->zErrortext = 0;
#endif /* MNG_INCLUDE_ERROR_STRINGS */

    if (iError == 0)                   /* no error is not severe ! */
    {
      pData->iSeverity = 0;
    }
    else
    {
      switch (iError&0x3C00)           /* determine the severity */
      {
        case 0x0800 : { pData->iSeverity = 5; break; }
        case 0x1000 : { pData->iSeverity = 2; break; }
        case 0x2000 : { pData->iSeverity = 1; break; }      
        default     : { pData->iSeverity = 9; }
      }
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEB (pData, MNG_FN_STORE_ERROR, MNG_LC_END);
#endif

  return MNG_TRUE;
}

/* ************************************************************************** */

mng_bool mng_process_error (mng_datap   pData,
                            mng_retcode iError,
                            mng_retcode iExtra1,
                            mng_retcode iExtra2)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEB (pData, MNG_FN_PROCESS_ERROR, MNG_LC_START);
#endif

  mng_store_error (pData, iError, iExtra1, iExtra2);

  if ((pData != MNG_NULL) && (pData->iMagic == MNG_MAGIC))
  {
    if (pData->fErrorproc)             /* callback defined ? */
      return pData->fErrorproc (((mng_handle)pData), iError, pData->iSeverity,
                                pData->iChunkname, pData->iChunkseq,
                                pData->iErrorx1, pData->iErrorx2, pData->zErrortext);
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACEB (pData, MNG_FN_PROCESS_ERROR, MNG_LC_END);
#endif

  return MNG_TRUE;                     /* warnings are ignored by default ! */
}

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

