/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_jpeg.c             copyright (c) 2000-2004 G.Juyn   * */
/* * version   : 1.0.9                                                      * */
/* *                                                                        * */
/* * purpose   : JPEG library interface (implementation)                    * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the JPEG library interface               * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *                                                                        * */
/* *             0.5.2 - 05/22/2000 - G.Juyn                                * */
/* *             - implemented all the JNG routines                         * */
/* *                                                                        * */
/* *             0.5.3 - 06/17/2000 - G.Juyn                                * */
/* *             - added tracing of JPEG calls                              * */
/* *             0.5.3 - 06/24/2000 - G.Juyn                                * */
/* *             - fixed inclusion of IJG read/write code                   * */
/* *             0.5.3 - 06/29/2000 - G.Juyn                                * */
/* *             - fixed some 64-bit warnings                               * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 10/16/2000 - G.Juyn                                * */
/* *             - added support for JDAA                                   * */
/* *                                                                        * */
/* *             1.0.1 - 04/19/2001 - G.Juyn                                * */
/* *             - added export of JPEG functions for DLL                   * */
/* *             1.0.1 - 04/22/2001 - G.Juyn                                * */
/* *             - fixed memory-leaks (Thanks Gregg!)                       * */
/* *                                                                        * */
/* *             1.0.4 - 06/22/2002 - G.Juyn                                * */
/* *             - B526138 - returned IJGSRC6B calling convention to        * */
/* *               default for MSVC                                         * */
/* *                                                                        * */
/* *             1.0.5 - 24/02/2003 - G.Juyn                                * */
/* *             - B683152 - libjpeg suspension not always honored correctly* */
/* *                                                                        * */
/* *             1.0.6 - 03/04/2003 - G.Juyn                                * */
/* *             - fixed some compiler-warnings                             * */
/* *                                                                        * */
/* *             1.0.8 - 08/01/2004 - G.Juyn                                * */
/* *             - added support for 3+byte pixelsize for JPEG's            * */
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
#include "libmng_memory.h"
#include "libmng_pixels.h"
#include "libmng_jpeg.h"

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */

#if defined(MNG_INCLUDE_JNG) && defined(MNG_INCLUDE_DISPLAY_PROCS)

/* ************************************************************************** */
/* *                                                                        * */
/* * Local IJG callback routines (source-manager, error-manager and such)   * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_IJG6B

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
#ifdef MNG_DEFINE_JPEG_STDCALL
void MNG_DECL mng_init_source (j_decompress_ptr cinfo)
#else
void mng_init_source (j_decompress_ptr cinfo)
#endif
{
  return;                              /* nothing needed */
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
#ifdef MNG_DEFINE_JPEG_STDCALL
boolean MNG_DECL mng_fill_input_buffer (j_decompress_ptr cinfo)
#else
boolean mng_fill_input_buffer (j_decompress_ptr cinfo)
#endif
{
  return FALSE;                        /* force IJG routine to return to caller */
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
#ifdef MNG_DEFINE_JPEG_STDCALL
void MNG_DECL mng_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
#else
void mng_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
#endif
{
  if (num_bytes > 0)                   /* ignore fony calls */
  {                                    /* address my generic structure */
    mng_datap pData = (mng_datap)cinfo->client_data;
                                       /* address source manager */
    mngjpeg_sourcep pSrc = pData->pJPEGdinfo->src;
                                       /* problem scenario ? */
    if (pSrc->bytes_in_buffer < (size_t)num_bytes)
    {                                  /* tell the boss we need to skip some data! */
      pData->iJPEGtoskip = (mng_uint32)((size_t)num_bytes - pSrc->bytes_in_buffer);

      pSrc->bytes_in_buffer = 0;       /* let the JPEG lib suspend */
      pSrc->next_input_byte = MNG_NULL;
    }
    else
    {                                  /* simply advance in the buffer */
      pSrc->bytes_in_buffer -= num_bytes;
      pSrc->next_input_byte += num_bytes;
    }
  }

  return;
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
#ifdef MNG_DEFINE_JPEG_STDCALL
void MNG_DECL mng_skip_input_data2 (j_decompress_ptr cinfo, long num_bytes)
#else
void mng_skip_input_data2 (j_decompress_ptr cinfo, long num_bytes)
#endif
{
  if (num_bytes > 0)                   /* ignore fony calls */
  {                                    /* address my generic structure */
    mng_datap pData = (mng_datap)cinfo->client_data;
                                       /* address source manager */
    mngjpeg_sourcep pSrc = pData->pJPEGdinfo2->src;
                                       /* problem scenario ? */
    if (pSrc->bytes_in_buffer < (size_t)num_bytes)
    {                                  /* tell the boss we need to skip some data! */
      pData->iJPEGtoskip2 = (mng_uint32)((size_t)num_bytes - pSrc->bytes_in_buffer);

      pSrc->bytes_in_buffer = 0;       /* let the JPEG lib suspend */
      pSrc->next_input_byte = MNG_NULL;
    }
    else
    {                                  /* simply advance in the buffer */
      pSrc->bytes_in_buffer -= num_bytes;
      pSrc->next_input_byte += num_bytes;
    }
  }

  return;
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
#ifdef MNG_DEFINE_JPEG_STDCALL
void MNG_DECL mng_term_source (j_decompress_ptr cinfo)
#else
void mng_term_source (j_decompress_ptr cinfo)
#endif
{
  return;                              /* nothing needed */
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_USE_SETJMP
#ifdef MNG_DEFINE_JPEG_STDCALL
void MNG_DECL mng_error_exit (j_common_ptr cinfo)
#else
void mng_error_exit (j_common_ptr cinfo)
#endif
{                                      /* address my generic structure */
  mng_datap pData = (mng_datap)cinfo->client_data;

#ifdef MNG_ERROR_TELLTALE              /* fill the message text ??? */
  (*cinfo->err->output_message) (cinfo);
#endif
                                       /* return to the point of no return... */
  longjmp (pData->sErrorbuf, cinfo->err->msg_code);
}
#endif /* MNG_USE_SETJMP */

/* ************************************************************************** */

#ifdef MNG_USE_SETJMP
#ifdef MNG_DEFINE_JPEG_STDCALL
void MNG_DECL mng_output_message (j_common_ptr cinfo)
#else
void mng_output_message (j_common_ptr cinfo)
#endif
{
  return;                              /* just do nothing ! */
}
#endif /* MNG_USE_SETJMP */

/* ************************************************************************** */

#endif /* MNG_INCLUDE_IJG6B */

/* ************************************************************************** */
/* *                                                                        * */
/* * Global JPEG routines                                                   * */
/* *                                                                        * */
/* ************************************************************************** */

mng_retcode mngjpeg_initialize (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_INITIALIZE, MNG_LC_START);
#endif
                                       /* allocate space for JPEG structures if necessary */
#ifdef MNG_INCLUDE_JNG_READ
  if (pData->pJPEGderr   == MNG_NULL)
    MNG_ALLOC (pData, pData->pJPEGderr,   sizeof (mngjpeg_error ));
  if (pData->pJPEGdsrc   == MNG_NULL)
    MNG_ALLOC (pData, pData->pJPEGdsrc,   sizeof (mngjpeg_source));
  if (pData->pJPEGdinfo  == MNG_NULL)
    MNG_ALLOC (pData, pData->pJPEGdinfo,  sizeof (mngjpeg_decomp));
                                       /* enable reverse addressing */
  pData->pJPEGdinfo->client_data  = pData;

  if (pData->pJPEGderr2  == MNG_NULL)
    MNG_ALLOC (pData, pData->pJPEGderr2,  sizeof (mngjpeg_error ));
  if (pData->pJPEGdsrc2  == MNG_NULL)
    MNG_ALLOC (pData, pData->pJPEGdsrc2,  sizeof (mngjpeg_source));
  if (pData->pJPEGdinfo2 == MNG_NULL)
    MNG_ALLOC (pData, pData->pJPEGdinfo2, sizeof (mngjpeg_decomp));
                                       /* enable reverse addressing */
  pData->pJPEGdinfo2->client_data = pData;
#endif

#ifdef MNG_INCLUDE_JNG_WRITE
  if (pData->pJPEGcerr  == MNG_NULL)
    MNG_ALLOC (pData, pData->pJPEGcerr,  sizeof (mngjpeg_error ));
  if (pData->pJPEGcinfo == MNG_NULL)
    MNG_ALLOC (pData, pData->pJPEGcinfo, sizeof (mngjpeg_comp  ));
                                       /* enable reverse addressing */
  pData->pJPEGcinfo->client_data = pData;
#endif

  if (pData->pJPEGbuf   == MNG_NULL)   /* initialize temporary buffers */
  {
    pData->iJPEGbufmax     = MNG_JPEG_MAXBUF;
    MNG_ALLOC (pData, pData->pJPEGbuf, pData->iJPEGbufmax);
  }

  if (pData->pJPEGbuf2  == MNG_NULL) 
  {
    pData->iJPEGbufmax2    = MNG_JPEG_MAXBUF;
    MNG_ALLOC (pData, pData->pJPEGbuf2, pData->iJPEGbufmax2);
  }

  pData->pJPEGcurrent      = pData->pJPEGbuf;
  pData->iJPEGbufremain    = 0;
  pData->pJPEGrow          = MNG_NULL;
  pData->iJPEGrowlen       = 0;
  pData->iJPEGtoskip       = 0;

  pData->pJPEGcurrent2     = pData->pJPEGbuf2;
  pData->iJPEGbufremain2   = 0;
  pData->pJPEGrow2         = MNG_NULL;
  pData->iJPEGrowlen2      = 0;
  pData->iJPEGtoskip2      = 0;
                                      /* not doing anything yet ! */
  pData->bJPEGcompress     = MNG_FALSE;
  
  pData->bJPEGdecompress   = MNG_FALSE;
  pData->bJPEGhasheader    = MNG_FALSE;
  pData->bJPEGdecostarted  = MNG_FALSE;
  pData->bJPEGscanstarted  = MNG_FALSE;
  pData->bJPEGscanending   = MNG_FALSE;

  pData->bJPEGdecompress2  = MNG_FALSE;
  pData->bJPEGhasheader2   = MNG_FALSE;
  pData->bJPEGdecostarted2 = MNG_FALSE;
  pData->bJPEGscanstarted2 = MNG_FALSE;

  pData->iJPEGrow          = 0;        /* zero input/output lines */
  pData->iJPEGalpharow     = 0;
  pData->iJPEGrgbrow       = 0;
  pData->iJPEGdisprow      = 0;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_INITIALIZE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

mng_retcode mngjpeg_cleanup (mng_datap pData)
{
#if defined(MNG_INCLUDE_IJG6B) && defined(MNG_USE_SETJMP)
  mng_retcode iRetcode;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_CLEANUP, MNG_LC_START);
#endif

#ifdef MNG_INCLUDE_IJG6B
#ifdef MNG_USE_SETJMP
  iRetcode = setjmp (pData->sErrorbuf);/* setup local JPEG error-recovery */
  if (iRetcode != 0)                   /* got here from longjmp ? */
    MNG_ERRORJ (pData, iRetcode);      /* then IJG-lib issued an error */
#endif

#ifdef MNG_INCLUDE_JNG_READ            /* still decompressing something ? */
  if (pData->bJPEGdecompress)
    jpeg_destroy_decompress (pData->pJPEGdinfo);
  if (pData->bJPEGdecompress2)
    jpeg_destroy_decompress (pData->pJPEGdinfo2);
#endif

#ifdef MNG_INCLUDE_JNG_WRITE
  if (pData->bJPEGcompress)            /* still compressing something ? */
    jpeg_destroy_compress (pData->pJPEGcinfo);
#endif

#endif /* MNG_INCLUDE_IJG6B */
                                       /* cleanup temporary buffers */
  MNG_FREE (pData, pData->pJPEGbuf2, pData->iJPEGbufmax2);
  MNG_FREE (pData, pData->pJPEGbuf,  pData->iJPEGbufmax);
                                       /* cleanup space for JPEG structures */
#ifdef MNG_INCLUDE_JNG_WRITE
  MNG_FREE (pData, pData->pJPEGcinfo,  sizeof (mngjpeg_comp  ));
  MNG_FREE (pData, pData->pJPEGcerr,   sizeof (mngjpeg_error ));
#endif

#ifdef MNG_INCLUDE_JNG_READ
  MNG_FREE (pData, pData->pJPEGdinfo,  sizeof (mngjpeg_decomp));
  MNG_FREE (pData, pData->pJPEGdsrc,   sizeof (mngjpeg_source));
  MNG_FREE (pData, pData->pJPEGderr,   sizeof (mngjpeg_error ));
  MNG_FREE (pData, pData->pJPEGdinfo2, sizeof (mngjpeg_decomp));
  MNG_FREE (pData, pData->pJPEGdsrc2,  sizeof (mngjpeg_source));
  MNG_FREE (pData, pData->pJPEGderr2,  sizeof (mngjpeg_error ));
#endif

  MNG_FREE (pData, pData->pJPEGrow2, pData->iJPEGrowlen2);
  MNG_FREE (pData, pData->pJPEGrow,  pData->iJPEGrowlen);
                                       /* whatever we were doing ... */
                                       /* we don't anymore ... */
  pData->bJPEGcompress     = MNG_FALSE;

  pData->bJPEGdecompress   = MNG_FALSE;
  pData->bJPEGhasheader    = MNG_FALSE;
  pData->bJPEGdecostarted  = MNG_FALSE;
  pData->bJPEGscanstarted  = MNG_FALSE;
  pData->bJPEGscanending   = MNG_FALSE;

  pData->bJPEGdecompress2  = MNG_FALSE;
  pData->bJPEGhasheader2   = MNG_FALSE;
  pData->bJPEGdecostarted2 = MNG_FALSE;
  pData->bJPEGscanstarted2 = MNG_FALSE;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_CLEANUP, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */
/* *                                                                        * */
/* * JPEG decompression routines (JDAT)                                     * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
mng_retcode mngjpeg_decompressinit (mng_datap pData)
{
#if defined(MNG_INCLUDE_IJG6B) && defined(MNG_USE_SETJMP)
  mng_retcode iRetcode;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSINIT, MNG_LC_START);
#endif

#ifdef MNG_INCLUDE_IJG6B
  /* allocate and initialize a JPEG decompression object */
  pData->pJPEGdinfo->err = jpeg_std_error (pData->pJPEGderr);

#ifdef MNG_USE_SETJMP                  /* setup local JPEG error-routines */
  pData->pJPEGderr->error_exit     = mng_error_exit;
  pData->pJPEGderr->output_message = mng_output_message;

  iRetcode = setjmp (pData->sErrorbuf);/* setup local JPEG error-recovery */
  if (iRetcode != 0)                   /* got here from longjmp ? */
    MNG_ERRORJ (pData, iRetcode);      /* then IJG-lib issued an error */
#endif /* MNG_USE_SETJMP */

  /* allocate and initialize a JPEG decompression object (continued) */
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSINIT, MNG_LC_JPEG_CREATE_DECOMPRESS)
#endif
  jpeg_create_decompress (pData->pJPEGdinfo);

  pData->bJPEGdecompress = MNG_TRUE;   /* indicate it's initialized */

  /* specify the source of the compressed data (eg, a file) */
                                       /* no, not a file; we have buffered input */
  pData->pJPEGdinfo->src = pData->pJPEGdsrc;
                                       /* use the default handler */
  pData->pJPEGdinfo->src->resync_to_restart = jpeg_resync_to_restart;
                                       /* setup local source routine & parms */
  pData->pJPEGdinfo->src->init_source       = mng_init_source;
  pData->pJPEGdinfo->src->fill_input_buffer = mng_fill_input_buffer;
  pData->pJPEGdinfo->src->skip_input_data   = mng_skip_input_data;
  pData->pJPEGdinfo->src->term_source       = mng_term_source;
  pData->pJPEGdinfo->src->next_input_byte   = pData->pJPEGcurrent;
  pData->pJPEGdinfo->src->bytes_in_buffer   = pData->iJPEGbufremain;

#endif /* MNG_INCLUDE_IJG6B */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSINIT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
mng_retcode mngjpeg_decompressdata (mng_datap  pData,
                                    mng_uint32 iRawsize,
                                    mng_uint8p pRawdata)
{
  mng_retcode iRetcode;
  mng_uint32  iRemain;
  mng_uint8p  pWork;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_START);
#endif

#if defined (MNG_INCLUDE_IJG6B) && defined(MNG_USE_SETJMP)
  iRetcode = setjmp (pData->sErrorbuf);/* initialize local JPEG error-recovery */
  if (iRetcode != 0)                   /* got here from longjmp ? */
    MNG_ERRORJ (pData, iRetcode);      /* then IJG-lib issued an error */
#endif

  pWork   = pRawdata;
  iRemain = iRawsize;

  if (pData->iJPEGtoskip)              /* JPEG-lib told us to skip some more data ? */
  {
    if (iRemain > pData->iJPEGtoskip)  /* enough data in this buffer ? */
    {
      iRemain -= pData->iJPEGtoskip;   /* skip enough to access the next byte */
      pWork   += pData->iJPEGtoskip;

      pData->iJPEGtoskip = 0;          /* no more to skip then */
    }
    else
    {
      pData->iJPEGtoskip -= iRemain;   /* skip all data in the buffer */
      iRemain = 0;                     /* and indicate this accordingly */
    }
                                       /* the skip set current-pointer to NULL ! */
    pData->pJPEGcurrent = pData->pJPEGbuf;
  }

  while (iRemain)                      /* repeat until no more input-bytes */
  {                                    /* need to shift anything ? */
    if ((pData->pJPEGcurrent > pData->pJPEGbuf) &&
        (pData->pJPEGcurrent - pData->pJPEGbuf + pData->iJPEGbufremain + iRemain > pData->iJPEGbufmax))
    {
      if (pData->iJPEGbufremain > 0)   /* then do so */
        MNG_COPY (pData->pJPEGbuf, pData->pJPEGcurrent, pData->iJPEGbufremain);

      pData->pJPEGcurrent = pData->pJPEGbuf;
    }
                                       /* does the remaining input fit into the buffer ? */
    if (pData->iJPEGbufremain + iRemain <= pData->iJPEGbufmax)
    {                                  /* move the lot */
      MNG_COPY ((pData->pJPEGcurrent + pData->iJPEGbufremain), pWork, iRemain);

      pData->iJPEGbufremain += iRemain;/* adjust remaining_bytes counter */
      iRemain = 0;                     /* and indicate there's no input left */
    }
    else
    {                                  /* calculate what does fit */
      mng_uint32 iFits = pData->iJPEGbufmax - pData->iJPEGbufremain;

      if (iFits <= 0)                  /* no space is just bugger 'm all */
        MNG_ERROR (pData, MNG_JPEGBUFTOOSMALL);
                                       /* move that */
      MNG_COPY ((pData->pJPEGcurrent + pData->iJPEGbufremain), pWork, iFits);

      pData->iJPEGbufremain += iFits;  /* adjust remain_bytes counter */
      iRemain -= iFits;                /* and the input-parms */
      pWork   += iFits;
    }

#ifdef MNG_INCLUDE_IJG6B
    pData->pJPEGdinfo->src->next_input_byte = pData->pJPEGcurrent;
    pData->pJPEGdinfo->src->bytes_in_buffer = pData->iJPEGbufremain;

    if (!pData->bJPEGhasheader)        /* haven't got the header yet ? */
    {
      /* call jpeg_read_header() to obtain image info */
#ifdef MNG_SUPPORT_TRACE
      MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_READ_HEADER)
#endif
      if (jpeg_read_header (pData->pJPEGdinfo, TRUE) != JPEG_SUSPENDED)
      {                                /* indicate the header's oke */
        pData->bJPEGhasheader = MNG_TRUE;
                                       /* let's do some sanity checks ! */
        if ((pData->pJPEGdinfo->image_width  != pData->iDatawidth ) ||
            (pData->pJPEGdinfo->image_height != pData->iDataheight)    )
          MNG_ERROR (pData, MNG_JPEGPARMSERR);

        if ( ((pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAY ) ||
              (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAYA)    ) &&
             (pData->pJPEGdinfo->jpeg_color_space != JCS_GRAYSCALE  )    )
          MNG_ERROR (pData, MNG_JPEGPARMSERR);

        if ( ((pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLOR ) ||
              (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLORA)    ) &&
             (pData->pJPEGdinfo->jpeg_color_space != JCS_YCbCr       )    )
          MNG_ERROR (pData, MNG_JPEGPARMSERR);
                                       /* indicate whether or not it's progressive */
        pData->bJPEGprogressive = (mng_bool)jpeg_has_multiple_scans (pData->pJPEGdinfo);
                                       /* progressive+alpha can't display "on-the-fly"!! */
        if ((pData->bJPEGprogressive) &&
            ((pData->iJHDRcolortype == MNG_COLORTYPE_JPEGGRAYA ) ||
             (pData->iJHDRcolortype == MNG_COLORTYPE_JPEGCOLORA)    ))
          pData->fDisplayrow = MNG_NULL;
                                       /* allocate a row of JPEG-samples */
        if (pData->pJPEGdinfo->jpeg_color_space == JCS_YCbCr)
          pData->iJPEGrowlen = pData->pJPEGdinfo->image_width * RGB_PIXELSIZE;
        else
          pData->iJPEGrowlen = pData->pJPEGdinfo->image_width;

        MNG_ALLOC (pData, pData->pJPEGrow, pData->iJPEGrowlen);

        pData->iJPEGrgbrow = 0;        /* quite empty up to now */
      }

      pData->pJPEGcurrent   = (mng_uint8p)pData->pJPEGdinfo->src->next_input_byte;
      pData->iJPEGbufremain = (mng_uint32)pData->pJPEGdinfo->src->bytes_in_buffer;
    }
                                       /* decompress not started ? */
    if ((pData->bJPEGhasheader) && (!pData->bJPEGdecostarted))
    {
      /* set parameters for decompression */

      if (pData->bJPEGprogressive)     /* progressive display ? */
        pData->pJPEGdinfo->buffered_image = TRUE;

      /* jpeg_start_decompress(...); */
#ifdef MNG_SUPPORT_TRACE
      MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_START_DECOMPRESS)
#endif
      if (jpeg_start_decompress (pData->pJPEGdinfo) == TRUE)
                                       /* indicate it started */
        pData->bJPEGdecostarted = MNG_TRUE;

      pData->pJPEGcurrent   = (mng_uint8p)pData->pJPEGdinfo->src->next_input_byte;
      pData->iJPEGbufremain = (mng_uint32)pData->pJPEGdinfo->src->bytes_in_buffer;
    }
                                       /* process some scanlines ? */
    if ((pData->bJPEGhasheader) && (pData->bJPEGdecostarted) &&
	    ((!jpeg_input_complete (pData->pJPEGdinfo)) ||
         (pData->pJPEGdinfo->output_scanline < pData->pJPEGdinfo->output_height) ||
         ((pData->bJPEGprogressive) && (pData->bJPEGscanending))))
    {
      mng_int32 iLines = 0;

      /* for (each output pass) */
      do
      {                                /* address the row output buffer */
        JSAMPROW pRow = (JSAMPROW)pData->pJPEGrow;

                                       /* init new pass ? */
        if ((pData->bJPEGprogressive) && (!pData->bJPEGscanstarted))
        {
          pData->bJPEGscanstarted = MNG_TRUE;

          /* adjust output decompression parameters if required */
          /* nop */

          /* start a new output pass */
#ifdef MNG_SUPPORT_TRACE
          MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_START_OUTPUT)
#endif
          jpeg_start_output (pData->pJPEGdinfo, pData->pJPEGdinfo->input_scan_number);

          pData->iJPEGrow = 0;         /* start at row 0 in the image again */
        }

        /* while (scan lines remain to be read) */
        if ((!pData->bJPEGprogressive) || (!pData->bJPEGscanending))
        {
          do
          {
          /*   jpeg_read_scanlines(...); */
#ifdef MNG_SUPPORT_TRACE
            MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_READ_SCANLINES)
#endif
            iLines = jpeg_read_scanlines (pData->pJPEGdinfo, (JSAMPARRAY)&pRow, 1);

            pData->pJPEGcurrent   = (mng_uint8p)pData->pJPEGdinfo->src->next_input_byte;
            pData->iJPEGbufremain = (mng_uint32)pData->pJPEGdinfo->src->bytes_in_buffer;

            if (iLines > 0)            /* got something ? */
            {
              if (pData->fStorerow2)   /* store in object ? */
              {
                iRetcode = ((mng_storerow)pData->fStorerow2) (pData);

                if (iRetcode)          /* on error bail out */
                return iRetcode;

              }
            }
          }
          while ((pData->pJPEGdinfo->output_scanline < pData->pJPEGdinfo->output_height) &&
                 (iLines > 0));        /* until end-of-image or not enough input-data */
        }

        /* terminate output pass */
        if ((pData->bJPEGprogressive) &&
            (pData->pJPEGdinfo->output_scanline >= pData->pJPEGdinfo->output_height))
        {
#ifdef MNG_SUPPORT_TRACE
          MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_FINISH_OUTPUT)
#endif
          if (jpeg_finish_output (pData->pJPEGdinfo) != JPEG_SUSPENDED)
          {                            /* this scan has ended */
            pData->bJPEGscanstarted = MNG_FALSE;
            pData->bJPEGscanending  = MNG_FALSE;
          }
          else
          {
            pData->bJPEGscanending  = MNG_TRUE;
          }
        }
      }
      while ((!jpeg_input_complete (pData->pJPEGdinfo)) &&
             (iLines > 0) && (!pData->bJPEGscanending));
    }
                                       /* end of image ? */
    if ((pData->bJPEGhasheader) && (pData->bJPEGdecostarted) &&
        (!pData->bJPEGscanending) && (jpeg_input_complete (pData->pJPEGdinfo)) &&
        (pData->pJPEGdinfo->input_scan_number == pData->pJPEGdinfo->output_scan_number))
    {
      /* jpeg_finish_decompress(...); */
#ifdef MNG_SUPPORT_TRACE
      MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_FINISH_DECOMPRESS)
#endif
      if (jpeg_finish_decompress (pData->pJPEGdinfo) == TRUE)
      {                                /* indicate it's done */
        pData->bJPEGhasheader   = MNG_FALSE;
        pData->bJPEGdecostarted = MNG_FALSE;
        pData->pJPEGcurrent     = (mng_uint8p)pData->pJPEGdinfo->src->next_input_byte;
        pData->iJPEGbufremain   = (mng_uint32)pData->pJPEGdinfo->src->bytes_in_buffer;
                                       /* remaining fluff is an error ! */
        if ((pData->iJPEGbufremain > 0) || (iRemain > 0))
          MNG_ERROR (pData, MNG_TOOMUCHJDAT);
      }
    }
#endif /* MNG_INCLUDE_IJG6B */
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
mng_retcode mngjpeg_decompressfree (mng_datap pData)
{
#if defined(MNG_INCLUDE_IJG6B) && defined(MNG_USE_SETJMP)
  mng_retcode iRetcode;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSFREE, MNG_LC_START);
#endif

#ifdef MNG_INCLUDE_IJG6B
#ifdef MNG_USE_SETJMP
  iRetcode = setjmp (pData->sErrorbuf);/* setup local JPEG error-recovery */
  if (iRetcode != 0)                   /* got here from longjmp ? */
    MNG_ERRORJ (pData, iRetcode);      /* then IJG-lib issued an error */
#endif
                                       /* free the row of JPEG-samples*/
  MNG_FREE (pData, pData->pJPEGrow, pData->iJPEGrowlen);

  /* release the JPEG decompression object */
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSFREE, MNG_LC_JPEG_DESTROY_DECOMPRESS)
#endif
  jpeg_destroy_decompress (pData->pJPEGdinfo);

  pData->bJPEGdecompress = MNG_FALSE;  /* indicate it's done */

#endif /* MNG_INCLUDE_IJG6B */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSFREE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */
/* *                                                                        * */
/* * JPEG decompression routines (JDAA)                                     * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
mng_retcode mngjpeg_decompressinit2 (mng_datap pData)
{
#if defined(MNG_INCLUDE_IJG6B) && defined(MNG_USE_SETJMP)
  mng_retcode iRetcode;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSINIT, MNG_LC_START);
#endif

#ifdef MNG_INCLUDE_IJG6B
  /* allocate and initialize a JPEG decompression object */
  pData->pJPEGdinfo2->err = jpeg_std_error (pData->pJPEGderr2);

#ifdef MNG_USE_SETJMP                  /* setup local JPEG error-routines */
  pData->pJPEGderr2->error_exit     = mng_error_exit;
  pData->pJPEGderr2->output_message = mng_output_message;

  iRetcode = setjmp (pData->sErrorbuf);/* setup local JPEG error-recovery */
  if (iRetcode != 0)                   /* got here from longjmp ? */
    MNG_ERRORJ (pData, iRetcode);      /* then IJG-lib issued an error */
#endif /* MNG_USE_SETJMP */

  /* allocate and initialize a JPEG decompression object (continued) */
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSINIT, MNG_LC_JPEG_CREATE_DECOMPRESS)
#endif
  jpeg_create_decompress (pData->pJPEGdinfo2);

  pData->bJPEGdecompress2 = MNG_TRUE;  /* indicate it's initialized */

  /* specify the source of the compressed data (eg, a file) */
                                       /* no, not a file; we have buffered input */
  pData->pJPEGdinfo2->src = pData->pJPEGdsrc2;
                                       /* use the default handler */
  pData->pJPEGdinfo2->src->resync_to_restart = jpeg_resync_to_restart;
                                       /* setup local source routine & parms */
  pData->pJPEGdinfo2->src->init_source       = mng_init_source;
  pData->pJPEGdinfo2->src->fill_input_buffer = mng_fill_input_buffer;
  pData->pJPEGdinfo2->src->skip_input_data   = mng_skip_input_data2;
  pData->pJPEGdinfo2->src->term_source       = mng_term_source;
  pData->pJPEGdinfo2->src->next_input_byte   = pData->pJPEGcurrent2;
  pData->pJPEGdinfo2->src->bytes_in_buffer   = pData->iJPEGbufremain2;

#endif /* MNG_INCLUDE_IJG6B */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSINIT, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
mng_retcode mngjpeg_decompressdata2 (mng_datap  pData,
                                     mng_uint32 iRawsize,
                                     mng_uint8p pRawdata)
{
  mng_retcode iRetcode;
  mng_uint32  iRemain;
  mng_uint8p  pWork;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_START);
#endif

#if defined (MNG_INCLUDE_IJG6B) && defined(MNG_USE_SETJMP)
  iRetcode = setjmp (pData->sErrorbuf);/* initialize local JPEG error-recovery */
  if (iRetcode != 0)                   /* got here from longjmp ? */
    MNG_ERRORJ (pData, iRetcode);      /* then IJG-lib issued an error */
#endif

  pWork   = pRawdata;
  iRemain = iRawsize;

  if (pData->iJPEGtoskip2)             /* JPEG-lib told us to skip some more data ? */
  {
    if (iRemain > pData->iJPEGtoskip2) /* enough data in this buffer ? */
    {
      iRemain -= pData->iJPEGtoskip2;  /* skip enough to access the next byte */
      pWork   += pData->iJPEGtoskip2;

      pData->iJPEGtoskip2 = 0;         /* no more to skip then */
    }
    else
    {
      pData->iJPEGtoskip2 -= iRemain;  /* skip all data in the buffer */
      iRemain = 0;                     /* and indicate this accordingly */
    }
                                       /* the skip set current-pointer to NULL ! */
    pData->pJPEGcurrent2 = pData->pJPEGbuf2;
  }

  while (iRemain)                      /* repeat until no more input-bytes */
  {                                    /* need to shift anything ? */
    if ((pData->pJPEGcurrent2 > pData->pJPEGbuf2) &&
        (pData->pJPEGcurrent2 - pData->pJPEGbuf2 + pData->iJPEGbufremain2 + iRemain > pData->iJPEGbufmax2))
    {
      if (pData->iJPEGbufremain2 > 0)  /* then do so */
        MNG_COPY (pData->pJPEGbuf2, pData->pJPEGcurrent2, pData->iJPEGbufremain2);

      pData->pJPEGcurrent2 = pData->pJPEGbuf2;
    }
                                       /* does the remaining input fit into the buffer ? */
    if (pData->iJPEGbufremain2 + iRemain <= pData->iJPEGbufmax2)
    {                                  /* move the lot */
      MNG_COPY ((pData->pJPEGcurrent2 + pData->iJPEGbufremain2), pWork, iRemain);
                                       /* adjust remaining_bytes counter */
      pData->iJPEGbufremain2 += iRemain;
      iRemain = 0;                     /* and indicate there's no input left */
    }
    else
    {                                  /* calculate what does fit */
      mng_uint32 iFits = pData->iJPEGbufmax2 - pData->iJPEGbufremain2;

      if (iFits <= 0)                  /* no space is just bugger 'm all */
        MNG_ERROR (pData, MNG_JPEGBUFTOOSMALL);
                                       /* move that */
      MNG_COPY ((pData->pJPEGcurrent2 + pData->iJPEGbufremain2), pWork, iFits);

      pData->iJPEGbufremain2 += iFits; /* adjust remain_bytes counter */
      iRemain -= iFits;                /* and the input-parms */
      pWork   += iFits;
    }

#ifdef MNG_INCLUDE_IJG6B
    pData->pJPEGdinfo2->src->next_input_byte = pData->pJPEGcurrent2;
    pData->pJPEGdinfo2->src->bytes_in_buffer = pData->iJPEGbufremain2;

    if (!pData->bJPEGhasheader2)       /* haven't got the header yet ? */
    {
      /* call jpeg_read_header() to obtain image info */
#ifdef MNG_SUPPORT_TRACE
      MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_READ_HEADER)
#endif
      if (jpeg_read_header (pData->pJPEGdinfo2, TRUE) != JPEG_SUSPENDED)
      {                                /* indicate the header's oke */
        pData->bJPEGhasheader2 = MNG_TRUE;
                                       /* let's do some sanity checks ! */
        if ((pData->pJPEGdinfo2->image_width  != pData->iDatawidth ) ||
            (pData->pJPEGdinfo2->image_height != pData->iDataheight)    )
          MNG_ERROR (pData, MNG_JPEGPARMSERR);

        if (pData->pJPEGdinfo2->jpeg_color_space != JCS_GRAYSCALE)
          MNG_ERROR (pData, MNG_JPEGPARMSERR);
                                       /* indicate whether or not it's progressive */
        pData->bJPEGprogressive2 = (mng_bool)jpeg_has_multiple_scans (pData->pJPEGdinfo2);

        if (pData->bJPEGprogressive2)  /* progressive alphachannel not allowed !!! */
          MNG_ERROR (pData, MNG_JPEGPARMSERR);
                                       /* allocate a row of JPEG-samples */
        if (pData->pJPEGdinfo2->jpeg_color_space == JCS_YCbCr)
          pData->iJPEGrowlen2 = pData->pJPEGdinfo2->image_width * RGB_PIXELSIZE;
        else
          pData->iJPEGrowlen2 = pData->pJPEGdinfo2->image_width;

        MNG_ALLOC (pData, pData->pJPEGrow2, pData->iJPEGrowlen2);

        pData->iJPEGalpharow = 0;      /* quite empty up to now */
      }

      pData->pJPEGcurrent2   = (mng_uint8p)pData->pJPEGdinfo2->src->next_input_byte;
      pData->iJPEGbufremain2 = (mng_uint32)pData->pJPEGdinfo2->src->bytes_in_buffer;
    }
                                       /* decompress not started ? */
    if ((pData->bJPEGhasheader2) && (!pData->bJPEGdecostarted2))
    {
      /* set parameters for decompression */

      if (pData->bJPEGprogressive2)    /* progressive display ? */
        pData->pJPEGdinfo2->buffered_image = TRUE;

      /* jpeg_start_decompress(...); */
#ifdef MNG_SUPPORT_TRACE
      MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_START_DECOMPRESS)
#endif
      if (jpeg_start_decompress (pData->pJPEGdinfo2) == TRUE)
                                       /* indicate it started */
        pData->bJPEGdecostarted2 = MNG_TRUE;

      pData->pJPEGcurrent2   = (mng_uint8p)pData->pJPEGdinfo2->src->next_input_byte;
      pData->iJPEGbufremain2 = (mng_uint32)pData->pJPEGdinfo2->src->bytes_in_buffer;
    }
                                       /* process some scanlines ? */
    if ((pData->bJPEGhasheader2) && (pData->bJPEGdecostarted2) &&
	    ((!jpeg_input_complete (pData->pJPEGdinfo2)) ||
         (pData->pJPEGdinfo2->output_scanline < pData->pJPEGdinfo2->output_height)))
    {
      mng_int32 iLines;

      /* for (each output pass) */
      do
      {                                /* address the row output buffer */
        JSAMPROW pRow = (JSAMPROW)pData->pJPEGrow2;

                                       /* init new pass ? */
        if ((pData->bJPEGprogressive2) &&
            ((!pData->bJPEGscanstarted2) ||
             (pData->pJPEGdinfo2->output_scanline >= pData->pJPEGdinfo2->output_height)))
        {
          pData->bJPEGscanstarted2 = MNG_TRUE;

          /* adjust output decompression parameters if required */
          /* nop */

          /* start a new output pass */
#ifdef MNG_SUPPORT_TRACE
          MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_START_OUTPUT)
#endif
          jpeg_start_output (pData->pJPEGdinfo2, pData->pJPEGdinfo2->input_scan_number);

          pData->iJPEGrow = 0;         /* start at row 0 in the image again */
        }

        /* while (scan lines remain to be read) */
        do
        {
          /*   jpeg_read_scanlines(...); */
#ifdef MNG_SUPPORT_TRACE
          MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_READ_SCANLINES)
#endif
          iLines = jpeg_read_scanlines (pData->pJPEGdinfo2, (JSAMPARRAY)&pRow, 1);

          pData->pJPEGcurrent2   = (mng_uint8p)pData->pJPEGdinfo2->src->next_input_byte;
          pData->iJPEGbufremain2 = (mng_uint32)pData->pJPEGdinfo2->src->bytes_in_buffer;

          if (iLines > 0)              /* got something ? */
          {
            if (pData->fStorerow3)     /* store in object ? */
            {
              iRetcode = ((mng_storerow)pData->fStorerow3) (pData);

              if (iRetcode)            /* on error bail out */
                return iRetcode;

            }
          }
        }
        while ((pData->pJPEGdinfo2->output_scanline < pData->pJPEGdinfo2->output_height) &&
               (iLines > 0));          /* until end-of-image or not enough input-data */

        /* terminate output pass */
        if ((pData->bJPEGprogressive2) &&
            (pData->pJPEGdinfo2->output_scanline >= pData->pJPEGdinfo2->output_height))
        {
#ifdef MNG_SUPPORT_TRACE
          MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_FINISH_OUTPUT)
#endif
          if (jpeg_finish_output (pData->pJPEGdinfo2) == JPEG_SUSPENDED)
            jpeg_finish_output (pData->pJPEGdinfo2);
                                       /* this scan has ended */
          pData->bJPEGscanstarted2 = MNG_FALSE;
        }
      }
      while ((!jpeg_input_complete (pData->pJPEGdinfo2)) && (iLines > 0));
    }
                                       /* end of image ? */
    if ((pData->bJPEGhasheader2) && (pData->bJPEGdecostarted2) &&
        (jpeg_input_complete (pData->pJPEGdinfo2)) &&
        (pData->pJPEGdinfo2->input_scan_number == pData->pJPEGdinfo2->output_scan_number))
    {
      /* jpeg_finish_decompress(...); */
#ifdef MNG_SUPPORT_TRACE
      MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_JPEG_FINISH_DECOMPRESS)
#endif
      if (jpeg_finish_decompress (pData->pJPEGdinfo2) == TRUE)
      {                                /* indicate it's done */
        pData->bJPEGhasheader2   = MNG_FALSE;
        pData->bJPEGdecostarted2 = MNG_FALSE;
        pData->pJPEGcurrent2     = (mng_uint8p)pData->pJPEGdinfo2->src->next_input_byte;
        pData->iJPEGbufremain2   = (mng_uint32)pData->pJPEGdinfo2->src->bytes_in_buffer;
                                       /* remaining fluff is an error ! */
        if ((pData->iJPEGbufremain2 > 0) || (iRemain > 0))
          MNG_ERROR (pData, MNG_TOOMUCHJDAT);
      }
    }
#endif /* MNG_INCLUDE_IJG6B */
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSDATA, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_JNG_READ
mng_retcode mngjpeg_decompressfree2 (mng_datap pData)
{
#if defined(MNG_INCLUDE_IJG6B) && defined(MNG_USE_SETJMP)
  mng_retcode iRetcode;
#endif

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSFREE, MNG_LC_START);
#endif

#ifdef MNG_INCLUDE_IJG6B
#ifdef MNG_USE_SETJMP
  iRetcode = setjmp (pData->sErrorbuf);/* setup local JPEG error-recovery */
  if (iRetcode != 0)                   /* got here from longjmp ? */
    MNG_ERRORJ (pData, iRetcode);      /* then IJG-lib issued an error */
#endif
                                       /* free the row of JPEG-samples*/
  MNG_FREE (pData, pData->pJPEGrow2, pData->iJPEGrowlen2);

  /* release the JPEG decompression object */
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSFREE, MNG_LC_JPEG_DESTROY_DECOMPRESS)
#endif
  jpeg_destroy_decompress (pData->pJPEGdinfo2);

  pData->bJPEGdecompress2 = MNG_FALSE; /* indicate it's done */

#endif /* MNG_INCLUDE_IJG6B */

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_JPEG_DECOMPRESSFREE, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_JNG_READ */

/* ************************************************************************** */

#endif /* MNG_INCLUDE_JNG && MNG_INCLUDE_DISPLAY_PROCS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */

