// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngwrite.c - general routines to write a PNG file
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 */

/* get internal access to png.h */
#define PNG_INTERNAL
#include "png.h"
#ifdef PNG_WRITE_SUPPORTED

/* Writes all the PNG information.  This is the suggested way to use the
 * library.  If you have a new chunk to add, make a function to write it,
 * and put it in the correct location here.  If you want the chunk written
 * after the image data, put it in xpng_write_end().  I strongly encourage
 * you to supply a PNG_INFO_ flag, and check info_ptr->valid before writing
 * the chunk, as that will keep the code from breaking if you want to just
 * write a plain PNG file.  If you have long comments, I suggest writing
 * them in xpng_write_end(), and compressing them.
 */
void PNGAPI
xpng_write_info_before_PLTE(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   xpng_debug(1, "in xpng_write_info_before_PLTE\n");
   if (!(xpng_ptr->mode & PNG_WROTE_INFO_BEFORE_PLTE))
   {
   xpng_write_sig(xpng_ptr); /* write PNG signature */
#if defined(PNG_MNG_FEATURES_SUPPORTED)
   if((xpng_ptr->mode&PNG_HAVE_PNG_SIGNATURE)&&(xpng_ptr->mng_features_permitted))
   {
      xpng_warning(xpng_ptr,"MNG features are not allowed in a PNG datastream\n");
      xpng_ptr->mng_features_permitted=0;
   }
#endif
   /* write IHDR information. */
   xpng_write_IHDR(xpng_ptr, info_ptr->width, info_ptr->height,
      info_ptr->bit_depth, info_ptr->color_type, info_ptr->compression_type,
      info_ptr->filter_type,
#if defined(PNG_WRITE_INTERLACING_SUPPORTED)
      info_ptr->interlace_type);
#else
      0);
#endif
   /* the rest of these check to see if the valid field has the appropriate
      flag set, and if it does, writes the chunk. */
#if defined(PNG_WRITE_gAMA_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_gAMA)
   {
#  ifdef PNG_FLOATING_POINT_SUPPORTED
      xpng_write_gAMA(xpng_ptr, info_ptr->gamma);
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
      xpng_write_gAMA_fixed(xpng_ptr, info_ptr->int_gamma);
#  endif
#endif
   }
#endif
#if defined(PNG_WRITE_sRGB_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_sRGB)
      xpng_write_sRGB(xpng_ptr, (int)info_ptr->srgb_intent);
#endif
#if defined(PNG_WRITE_iCCP_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_iCCP)
      xpng_write_iCCP(xpng_ptr, info_ptr->iccp_name, PNG_COMPRESSION_TYPE_BASE,
                     info_ptr->iccp_profile, (int)info_ptr->iccp_proflen);
#endif
#if defined(PNG_WRITE_sBIT_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_sBIT)
      xpng_write_sBIT(xpng_ptr, &(info_ptr->sig_bit), info_ptr->color_type);
#endif
#if defined(PNG_WRITE_cHRM_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_cHRM)
   {
#ifdef PNG_FLOATING_POINT_SUPPORTED
      xpng_write_cHRM(xpng_ptr,
         info_ptr->x_white, info_ptr->y_white,
         info_ptr->x_red, info_ptr->y_red,
         info_ptr->x_green, info_ptr->y_green,
         info_ptr->x_blue, info_ptr->y_blue);
#else
#  ifdef PNG_FIXED_POINT_SUPPORTED
      xpng_write_cHRM_fixed(xpng_ptr,
         info_ptr->int_x_white, info_ptr->int_y_white,
         info_ptr->int_x_red, info_ptr->int_y_red,
         info_ptr->int_x_green, info_ptr->int_y_green,
         info_ptr->int_x_blue, info_ptr->int_y_blue);
#  endif
#endif
   }
#endif
#if defined(PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED)
   if (info_ptr->unknown_chunks_num)
   {
       xpng_unknown_chunk *up;

       xpng_debug(5, "writing extra chunks\n");

       for (up = info_ptr->unknown_chunks;
            up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
            up++)
       {
         int keep=xpng_handle_as_unknown(xpng_ptr, up->name);
         if (keep != HANDLE_CHUNK_NEVER &&
            up->location && (!(up->location & PNG_HAVE_PLTE)) &&
            ((up->name[3] & 0x20) || keep == HANDLE_CHUNK_ALWAYS ||
            (xpng_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            xpng_write_chunk(xpng_ptr, up->name, up->data, up->size);
         }
       }
   }
#endif
      xpng_ptr->mode |= PNG_WROTE_INFO_BEFORE_PLTE;
   }
}

void PNGAPI
xpng_write_info(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)
   int i;
#endif

   xpng_debug(1, "in xpng_write_info\n");

   xpng_write_info_before_PLTE(xpng_ptr, info_ptr);

   if (info_ptr->valid & PNG_INFO_PLTE)
      xpng_write_PLTE(xpng_ptr, info_ptr->palette,
         (xpng_uint_32)info_ptr->num_palette);
   else if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      xpng_error(xpng_ptr, "Valid palette required for paletted images\n");

#if defined(PNG_WRITE_tRNS_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_tRNS)
      {
#if defined(PNG_WRITE_INVERT_ALPHA_SUPPORTED)
         /* invert the alpha channel (in tRNS) */
         if ((xpng_ptr->transformations & PNG_INVERT_ALPHA) &&
            info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
         {
            int j;
            for (j=0; j<(int)info_ptr->num_trans; j++)
               info_ptr->trans[j] = (xpng_byte)(255 - info_ptr->trans[j]);
         }
#endif
      xpng_write_tRNS(xpng_ptr, info_ptr->trans, &(info_ptr->trans_values),
         info_ptr->num_trans, info_ptr->color_type);
      }
#endif
#if defined(PNG_WRITE_bKGD_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_bKGD)
      xpng_write_bKGD(xpng_ptr, &(info_ptr->background), info_ptr->color_type);
#endif
#if defined(PNG_WRITE_hIST_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_hIST)
      xpng_write_hIST(xpng_ptr, info_ptr->hist, info_ptr->num_palette);
#endif
#if defined(PNG_WRITE_oFFs_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_oFFs)
      xpng_write_oFFs(xpng_ptr, info_ptr->x_offset, info_ptr->y_offset,
         info_ptr->offset_unit_type);
#endif
#if defined(PNG_WRITE_pCAL_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_pCAL)
      xpng_write_pCAL(xpng_ptr, info_ptr->pcal_purpose, info_ptr->pcal_X0,
         info_ptr->pcal_X1, info_ptr->pcal_type, info_ptr->pcal_nparams,
         info_ptr->pcal_units, info_ptr->pcal_params);
#endif
#if defined(PNG_WRITE_sCAL_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_sCAL)
#if defined(PNG_FLOATING_POINT_SUPPORTED) && !defined(PNG_NO_STDIO)
      xpng_write_sCAL(xpng_ptr, (int)info_ptr->scal_unit,
          info_ptr->scal_pixel_width, info_ptr->scal_pixel_height);
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
      xpng_write_sCAL_s(xpng_ptr, (int)info_ptr->scal_unit,
          info_ptr->scal_s_width, info_ptr->scal_s_height);
#else
      xpng_warning(xpng_ptr,
          "xpng_write_sCAL not supported; sCAL chunk not written.\n");
#endif
#endif
#endif
#if defined(PNG_WRITE_pHYs_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_pHYs)
      xpng_write_pHYs(xpng_ptr, info_ptr->x_pixels_per_unit,
         info_ptr->y_pixels_per_unit, info_ptr->phys_unit_type);
#endif
#if defined(PNG_WRITE_tIME_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_tIME)
   {
      xpng_write_tIME(xpng_ptr, &(info_ptr->mod_time));
      xpng_ptr->mode |= PNG_WROTE_tIME;
   }
#endif
#if defined(PNG_WRITE_sPLT_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_sPLT)
     for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
       xpng_write_sPLT(xpng_ptr, info_ptr->splt_palettes + i);
#endif
#if defined(PNG_WRITE_TEXT_SUPPORTED)
   /* Check to see if we need to write text chunks */
   for (i = 0; i < info_ptr->num_text; i++)
   {
      xpng_debug2(2, "Writing header text chunk %d, type %d\n", i,
         info_ptr->text[i].compression);
      /* an internationalized chunk? */
      if (info_ptr->text[i].compression > 0)
      {
#if defined(PNG_WRITE_iTXt_SUPPORTED)
          /* write international chunk */
          xpng_write_iTXt(xpng_ptr,
                         info_ptr->text[i].compression,
                         info_ptr->text[i].key,
                         info_ptr->text[i].lang,
                         info_ptr->text[i].lang_key,
                         info_ptr->text[i].text);
#else
          xpng_warning(xpng_ptr, "Unable to write international text\n");
#endif
          /* Mark this chunk as written */
          info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
      }
      /* If we want a compressed text chunk */
      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_zTXt)
      {
#if defined(PNG_WRITE_zTXt_SUPPORTED)
         /* write compressed chunk */
         xpng_write_zTXt(xpng_ptr, info_ptr->text[i].key,
            info_ptr->text[i].text, 0,
            info_ptr->text[i].compression);
#else
         xpng_warning(xpng_ptr, "Unable to write compressed text\n");
#endif
         /* Mark this chunk as written */
         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
      }
      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
      {
#if defined(PNG_WRITE_tEXt_SUPPORTED)
         /* write uncompressed chunk */
         xpng_write_tEXt(xpng_ptr, info_ptr->text[i].key,
                         info_ptr->text[i].text,
                         0);
#else
         xpng_warning(xpng_ptr, "Unable to write uncompressed text\n");
#endif
         /* Mark this chunk as written */
         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
      }
   }
#endif
#if defined(PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED)
   if (info_ptr->unknown_chunks_num)
   {
       xpng_unknown_chunk *up;

       xpng_debug(5, "writing extra chunks\n");

       for (up = info_ptr->unknown_chunks;
            up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
            up++)
       {
         int keep=xpng_handle_as_unknown(xpng_ptr, up->name);
         if (keep != HANDLE_CHUNK_NEVER &&
            up->location && (up->location & PNG_HAVE_PLTE) &&
            !(up->location & PNG_HAVE_IDAT) &&
            ((up->name[3] & 0x20) || keep == HANDLE_CHUNK_ALWAYS ||
            (xpng_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            xpng_write_chunk(xpng_ptr, up->name, up->data, up->size);
         }
       }
   }
#endif
}

/* Writes the end of the PNG file.  If you don't want to write comments or
 * time information, you can pass NULL for info.  If you already wrote these
 * in xpng_write_info(), do not write them again here.  If you have long
 * comments, I suggest writing them here, and compressing them.
 */
void PNGAPI
xpng_write_end(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   xpng_debug(1, "in xpng_write_end\n");
   if (!(xpng_ptr->mode & PNG_HAVE_IDAT))
      xpng_error(xpng_ptr, "No IDATs written into file");

   /* see if user wants us to write information chunks */
   if (info_ptr != NULL)
   {
#if defined(PNG_WRITE_TEXT_SUPPORTED)
      int i; /* local index variable */
#endif
#if defined(PNG_WRITE_tIME_SUPPORTED)
      /* check to see if user has supplied a time chunk */
      if ((info_ptr->valid & PNG_INFO_tIME) &&
         !(xpng_ptr->mode & PNG_WROTE_tIME))
         xpng_write_tIME(xpng_ptr, &(info_ptr->mod_time));
#endif
#if defined(PNG_WRITE_TEXT_SUPPORTED)
      /* loop through comment chunks */
      for (i = 0; i < info_ptr->num_text; i++)
      {
         xpng_debug2(2, "Writing trailer text chunk %d, type %d\n", i,
            info_ptr->text[i].compression);
         /* an internationalized chunk? */
         if (info_ptr->text[i].compression > 0)
         {
#if defined(PNG_WRITE_iTXt_SUPPORTED)
             /* write international chunk */
             xpng_write_iTXt(xpng_ptr,
                         info_ptr->text[i].compression,
                         info_ptr->text[i].key,
                         info_ptr->text[i].lang,
                         info_ptr->text[i].lang_key,
                         info_ptr->text[i].text);
#else
             xpng_warning(xpng_ptr, "Unable to write international text\n");
#endif
             /* Mark this chunk as written */
             info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
         }
         else if (info_ptr->text[i].compression >= PNG_TEXT_COMPRESSION_zTXt)
         {
#if defined(PNG_WRITE_zTXt_SUPPORTED)
            /* write compressed chunk */
            xpng_write_zTXt(xpng_ptr, info_ptr->text[i].key,
               info_ptr->text[i].text, 0,
               info_ptr->text[i].compression);
#else
            xpng_warning(xpng_ptr, "Unable to write compressed text\n");
#endif
            /* Mark this chunk as written */
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
         }
         else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
         {
#if defined(PNG_WRITE_tEXt_SUPPORTED)
            /* write uncompressed chunk */
            xpng_write_tEXt(xpng_ptr, info_ptr->text[i].key,
               info_ptr->text[i].text, 0);
#else
            xpng_warning(xpng_ptr, "Unable to write uncompressed text\n");
#endif

            /* Mark this chunk as written */
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
         }
      }
#endif
#if defined(PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED)
   if (info_ptr->unknown_chunks_num)
   {
       xpng_unknown_chunk *up;

       xpng_debug(5, "writing extra chunks\n");

       for (up = info_ptr->unknown_chunks;
            up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
            up++)
       {
         int keep=xpng_handle_as_unknown(xpng_ptr, up->name);
         if (keep != HANDLE_CHUNK_NEVER &&
            up->location && (up->location & PNG_AFTER_IDAT) &&
            ((up->name[3] & 0x20) || keep == HANDLE_CHUNK_ALWAYS ||
            (xpng_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            xpng_write_chunk(xpng_ptr, up->name, up->data, up->size);
         }
       }
   }
#endif
   }

   xpng_ptr->mode |= PNG_AFTER_IDAT;

   /* write end of PNG file */
   xpng_write_IEND(xpng_ptr);
#if 0
/* This flush, added in libpng-1.0.8,  causes some applications to crash
   because they do not set xpng_ptr->output_flush_fn */
   xpng_flush(xpng_ptr);
#endif
}

#if defined(PNG_WRITE_tIME_SUPPORTED)
#if !defined(_WIN32_WCE)
/* "time.h" functions are not supported on WindowsCE */
void PNGAPI
xpng_convert_from_struct_tm(xpng_timep ptime, struct tm FAR * ttime)
{
   xpng_debug(1, "in xpng_convert_from_struct_tm\n");
   ptime->year = (xpng_uint_16)(1900 + ttime->tm_year);
   ptime->month = (xpng_byte)(ttime->tm_mon + 1);
   ptime->day = (xpng_byte)ttime->tm_mday;
   ptime->hour = (xpng_byte)ttime->tm_hour;
   ptime->minute = (xpng_byte)ttime->tm_min;
   ptime->second = (xpng_byte)ttime->tm_sec;
}

void PNGAPI
xpng_convert_from_time_t(xpng_timep ptime, time_t ttime)
{
   struct tm *tbuf;

   xpng_debug(1, "in xpng_convert_from_time_t\n");
   tbuf = gmtime(&ttime);
   xpng_convert_from_struct_tm(ptime, tbuf);
}
#endif
#endif

/* Initialize xpng_ptr structure, and allocate any memory needed */
xpng_structp PNGAPI
xpng_create_write_struct(xpng_const_charp user_xpng_ver, xpng_voidp error_ptr,
   xpng_error_ptr error_fn, xpng_error_ptr warn_fn)
{
#ifdef PNG_USER_MEM_SUPPORTED
   return (xpng_create_write_struct_2(user_xpng_ver, error_ptr, error_fn,
      warn_fn, xpng_voidp_NULL, xpng_malloc_ptr_NULL, xpng_free_ptr_NULL));
}

/* Alternate initialize xpng_ptr structure, and allocate any memory needed */
xpng_structp PNGAPI
xpng_create_write_struct_2(xpng_const_charp user_xpng_ver, xpng_voidp error_ptr,
   xpng_error_ptr error_fn, xpng_error_ptr warn_fn, xpng_voidp mem_ptr,
   xpng_malloc_ptr malloc_fn, xpng_free_ptr free_fn)
{
#endif /* PNG_USER_MEM_SUPPORTED */
   xpng_structp xpng_ptr;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
   jmp_buf jmpbuf;
#endif
#endif
   int i;
   xpng_debug(1, "in xpng_create_write_struct\n");
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_ptr = (xpng_structp)xpng_create_struct_2(PNG_STRUCT_PNG,
      (xpng_malloc_ptr)malloc_fn, (xpng_voidp)mem_ptr);
#else
   xpng_ptr = (xpng_structp)xpng_create_struct(PNG_STRUCT_PNG);
#endif /* PNG_USER_MEM_SUPPORTED */
   if (xpng_ptr == NULL)
      return (NULL);

#if !defined(PNG_1_0_X)
#ifdef PNG_ASSEMBLER_CODE_SUPPORTED
   xpng_init_mmx_flags(xpng_ptr);   /* 1.2.0 addition */
#endif
#endif /* PNG_1_0_X */

#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
   if (setjmp(jmpbuf))
#else
   if (setjmp(xpng_ptr->jmpbuf))
#endif
   {
      xpng_free(xpng_ptr, xpng_ptr->zbuf);
      xpng_ptr->zbuf=NULL;
      xpng_destroy_struct(xpng_ptr);
      return (NULL);
   }
#ifdef USE_FAR_KEYWORD
   xpng_memcpy(xpng_ptr->jmpbuf,jmpbuf,sizeof(jmp_buf));
#endif
#endif

#ifdef PNG_USER_MEM_SUPPORTED
   xpng_set_mem_fn(xpng_ptr, mem_ptr, malloc_fn, free_fn);
#endif /* PNG_USER_MEM_SUPPORTED */
   xpng_set_error_fn(xpng_ptr, error_ptr, error_fn, warn_fn);

   i=0;
   do
   {
     if(user_xpng_ver[i] != xpng_libxpng_ver[i])
        xpng_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;
   } while (xpng_libxpng_ver[i++]);

   if (xpng_ptr->flags & PNG_FLAG_LIBRARY_MISMATCH)
   {
     /* Libpng 0.90 and later are binary incompatible with libpng 0.89, so
      * we must recompile any applications that use any older library version.
      * For versions after libpng 1.0, we will be compatible, so we need
      * only check the first digit.
      */
     if (user_xpng_ver == NULL || user_xpng_ver[0] != xpng_libxpng_ver[0] ||
         (user_xpng_ver[0] == '1' && user_xpng_ver[2] != xpng_libxpng_ver[2]) ||
         (user_xpng_ver[0] == '0' && user_xpng_ver[2] < '9'))
     {
#if !defined(PNG_NO_STDIO) && !defined(_WIN32_WCE)
        char msg[80];
        if (user_xpng_ver)
        {
          sprintf(msg, "Application was compiled with png.h from libpng-%.20s",
             user_xpng_ver);
          xpng_warning(xpng_ptr, msg);
        }
        sprintf(msg, "Application  is  running with png.c from libpng-%.20s",
           xpng_libxpng_ver);
        xpng_warning(xpng_ptr, msg);
#endif
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
        xpng_ptr->flags=0;
#endif
        xpng_error(xpng_ptr,
           "Incompatible libpng version in application and library");
     }
   }

   /* initialize zbuf - compression buffer */
   xpng_ptr->zbuf_size = PNG_ZBUF_SIZE;
   xpng_ptr->zbuf = (xpng_bytep)xpng_malloc(xpng_ptr,
      (xpng_uint_32)xpng_ptr->zbuf_size);

   xpng_set_write_fn(xpng_ptr, xpng_voidp_NULL, xpng_rw_ptr_NULL,
      xpng_flush_ptr_NULL);

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
   xpng_set_filter_heuristics(xpng_ptr, PNG_FILTER_HEURISTIC_DEFAULT,
      1, xpng_doublep_NULL, xpng_doublep_NULL);
#endif

#ifdef PNG_SETJMP_SUPPORTED
/* Applications that neglect to set up their own setjmp() and then encounter
   a xpng_error() will longjmp here.  Since the jmpbuf is then meaningless we
   abort instead of returning. */
#ifdef USE_FAR_KEYWORD
   if (setjmp(jmpbuf))
      PNG_ABORT();
   xpng_memcpy(xpng_ptr->jmpbuf,jmpbuf,sizeof(jmp_buf));
#else
   if (setjmp(xpng_ptr->jmpbuf))
      PNG_ABORT();
#endif
#endif
   return (xpng_ptr);
}

/* Initialize xpng_ptr structure, and allocate any memory needed */
#undef xpng_write_init
void PNGAPI
xpng_write_init(xpng_structp xpng_ptr)
{
   /* We only come here via pre-1.0.7-compiled applications */
   xpng_write_init_2(xpng_ptr, "1.0.6 or earlier", 0, 0);
}

void PNGAPI
xpng_write_init_2(xpng_structp xpng_ptr, xpng_const_charp user_xpng_ver,
   xpng_size_t xpng_struct_size, xpng_size_t xpng_info_size)
{
   /* We only come here via pre-1.0.12-compiled applications */
#if !defined(PNG_NO_STDIO) && !defined(_WIN32_WCE)
   if(sizeof(xpng_struct) > xpng_struct_size || sizeof(xpng_info) > xpng_info_size)
   {
      char msg[80];
      xpng_ptr->warning_fn=NULL;
      if (user_xpng_ver)
      {
        sprintf(msg, "Application was compiled with png.h from libpng-%.20s",
           user_xpng_ver);
        xpng_warning(xpng_ptr, msg);
      }
      sprintf(msg, "Application  is  running with png.c from libpng-%.20s",
         xpng_libxpng_ver);
      xpng_warning(xpng_ptr, msg);
   }
#endif
   if(sizeof(xpng_struct) > xpng_struct_size)
     {
       xpng_ptr->error_fn=NULL;
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
       xpng_ptr->flags=0;
#endif
       xpng_error(xpng_ptr,
       "The png struct allocated by the application for writing is too small.");
     }
   if(sizeof(xpng_info) > xpng_info_size)
     {
       xpng_ptr->error_fn=NULL;
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
       xpng_ptr->flags=0;
#endif
       xpng_error(xpng_ptr,
       "The info struct allocated by the application for writing is too small.");
     }
   xpng_write_init_3(&xpng_ptr, user_xpng_ver, xpng_struct_size);
}


void PNGAPI
xpng_write_init_3(xpng_structpp ptr_ptr, xpng_const_charp user_xpng_ver,
   xpng_size_t xpng_struct_size)
{
   xpng_structp xpng_ptr=*ptr_ptr;
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf tmp_jmp; /* to save current jump buffer */
#endif
   int i = 0;
   do
   {
     if (user_xpng_ver[i] != xpng_libxpng_ver[i])
     {
#ifdef PNG_LEGACY_SUPPORTED
       xpng_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;
#else
       xpng_ptr->warning_fn=NULL;
       xpng_warning(xpng_ptr,
     "Application uses deprecated xpng_write_init() and should be recompiled.");
       break;
#endif
     }
   } while (xpng_libxpng_ver[i++]);

   xpng_debug(1, "in xpng_write_init_3\n");

#ifdef PNG_SETJMP_SUPPORTED
   /* save jump buffer and error functions */
   xpng_memcpy(tmp_jmp, xpng_ptr->jmpbuf, sizeof (jmp_buf));
#endif

   if (sizeof(xpng_struct) > xpng_struct_size)
     {
       xpng_destroy_struct(xpng_ptr);
       xpng_ptr = (xpng_structp)xpng_create_struct(PNG_STRUCT_PNG);
       *ptr_ptr = xpng_ptr;
     }

   /* reset all variables to 0 */
   xpng_memset(xpng_ptr, 0, sizeof (xpng_struct));

#if !defined(PNG_1_0_X)
#ifdef PNG_ASSEMBLER_CODE_SUPPORTED
   xpng_init_mmx_flags(xpng_ptr);   /* 1.2.0 addition */
#endif
#endif /* PNG_1_0_X */

#ifdef PNG_SETJMP_SUPPORTED
   /* restore jump buffer */
   xpng_memcpy(xpng_ptr->jmpbuf, tmp_jmp, sizeof (jmp_buf));
#endif

   xpng_set_write_fn(xpng_ptr, xpng_voidp_NULL, xpng_rw_ptr_NULL,
      xpng_flush_ptr_NULL);

   /* initialize zbuf - compression buffer */
   xpng_ptr->zbuf_size = PNG_ZBUF_SIZE;
   xpng_ptr->zbuf = (xpng_bytep)xpng_malloc(xpng_ptr,
      (xpng_uint_32)xpng_ptr->zbuf_size);

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
   xpng_set_filter_heuristics(xpng_ptr, PNG_FILTER_HEURISTIC_DEFAULT,
      1, xpng_doublep_NULL, xpng_doublep_NULL);
#endif
}

/* Write a few rows of image data.  If the image is interlaced,
 * either you will have to write the 7 sub images, or, if you
 * have called xpng_set_interlace_handling(), you will have to
 * "write" the image seven times.
 */
void PNGAPI
xpng_write_rows(xpng_structp xpng_ptr, xpng_bytepp row,
   xpng_uint_32 num_rows)
{
   xpng_uint_32 i; /* row counter */
   xpng_bytepp rp; /* row pointer */

   xpng_debug(1, "in xpng_write_rows\n");
   /* loop through the rows */
   for (i = 0, rp = row; i < num_rows; i++, rp++)
   {
      xpng_write_row(xpng_ptr, *rp);
   }
}

/* Write the image.  You only need to call this function once, even
 * if you are writing an interlaced image.
 */
void PNGAPI
xpng_write_image(xpng_structp xpng_ptr, xpng_bytepp image)
{
   xpng_uint_32 i; /* row index */
   int pass, num_pass; /* pass variables */
   xpng_bytepp rp; /* points to current row */

   xpng_debug(1, "in xpng_write_image\n");
#if defined(PNG_WRITE_INTERLACING_SUPPORTED)
   /* intialize interlace handling.  If image is not interlaced,
      this will set pass to 1 */
   num_pass = xpng_set_interlace_handling(xpng_ptr);
#else
   num_pass = 1;
#endif
   /* loop through passes */
   for (pass = 0; pass < num_pass; pass++)
   {
      /* loop through image */
      for (i = 0, rp = image; i < xpng_ptr->height; i++, rp++)
      {
         xpng_write_row(xpng_ptr, *rp);
      }
   }
}

/* called by user to write a row of image data */
void PNGAPI
xpng_write_row(xpng_structp xpng_ptr, xpng_bytep row)
{
   xpng_debug2(1, "in xpng_write_row (row %ld, pass %d)\n",
      xpng_ptr->row_number, xpng_ptr->pass);
   /* initialize transformations and other stuff if first time */
   if (xpng_ptr->row_number == 0 && xpng_ptr->pass == 0)
   {
   /* make sure we wrote the header info */
   if (!(xpng_ptr->mode & PNG_WROTE_INFO_BEFORE_PLTE))
      xpng_error(xpng_ptr,
         "xpng_write_info was never called before xpng_write_row.");

   /* check for transforms that have been set but were defined out */
#if !defined(PNG_WRITE_INVERT_SUPPORTED) && defined(PNG_READ_INVERT_SUPPORTED)
   if (xpng_ptr->transformations & PNG_INVERT_MONO)
      xpng_warning(xpng_ptr, "PNG_WRITE_INVERT_SUPPORTED is not defined.");
#endif
#if !defined(PNG_WRITE_FILLER_SUPPORTED) && defined(PNG_READ_FILLER_SUPPORTED)
   if (xpng_ptr->transformations & PNG_FILLER)
      xpng_warning(xpng_ptr, "PNG_WRITE_FILLER_SUPPORTED is not defined.");
#endif
#if !defined(PNG_WRITE_PACKSWAP_SUPPORTED) && defined(PNG_READ_PACKSWAP_SUPPORTED)
   if (xpng_ptr->transformations & PNG_PACKSWAP)
      xpng_warning(xpng_ptr, "PNG_WRITE_PACKSWAP_SUPPORTED is not defined.");
#endif
#if !defined(PNG_WRITE_PACK_SUPPORTED) && defined(PNG_READ_PACK_SUPPORTED)
   if (xpng_ptr->transformations & PNG_PACK)
      xpng_warning(xpng_ptr, "PNG_WRITE_PACK_SUPPORTED is not defined.");
#endif
#if !defined(PNG_WRITE_SHIFT_SUPPORTED) && defined(PNG_READ_SHIFT_SUPPORTED)
   if (xpng_ptr->transformations & PNG_SHIFT)
      xpng_warning(xpng_ptr, "PNG_WRITE_SHIFT_SUPPORTED is not defined.");
#endif
#if !defined(PNG_WRITE_BGR_SUPPORTED) && defined(PNG_READ_BGR_SUPPORTED)
   if (xpng_ptr->transformations & PNG_BGR)
      xpng_warning(xpng_ptr, "PNG_WRITE_BGR_SUPPORTED is not defined.");
#endif
#if !defined(PNG_WRITE_SWAP_SUPPORTED) && defined(PNG_READ_SWAP_SUPPORTED)
   if (xpng_ptr->transformations & PNG_SWAP_BYTES)
      xpng_warning(xpng_ptr, "PNG_WRITE_SWAP_SUPPORTED is not defined.");
#endif

      xpng_write_start_row(xpng_ptr);
   }

#if defined(PNG_WRITE_INTERLACING_SUPPORTED)
   /* if interlaced and not interested in row, return */
   if (xpng_ptr->interlaced && (xpng_ptr->transformations & PNG_INTERLACE))
   {
      switch (xpng_ptr->pass)
      {
         case 0:
            if (xpng_ptr->row_number & 0x07)
            {
               xpng_write_finish_row(xpng_ptr);
               return;
            }
            break;
         case 1:
            if ((xpng_ptr->row_number & 0x07) || xpng_ptr->width < 5)
            {
               xpng_write_finish_row(xpng_ptr);
               return;
            }
            break;
         case 2:
            if ((xpng_ptr->row_number & 0x07) != 4)
            {
               xpng_write_finish_row(xpng_ptr);
               return;
            }
            break;
         case 3:
            if ((xpng_ptr->row_number & 0x03) || xpng_ptr->width < 3)
            {
               xpng_write_finish_row(xpng_ptr);
               return;
            }
            break;
         case 4:
            if ((xpng_ptr->row_number & 0x03) != 2)
            {
               xpng_write_finish_row(xpng_ptr);
               return;
            }
            break;
         case 5:
            if ((xpng_ptr->row_number & 0x01) || xpng_ptr->width < 2)
            {
               xpng_write_finish_row(xpng_ptr);
               return;
            }
            break;
         case 6:
            if (!(xpng_ptr->row_number & 0x01))
            {
               xpng_write_finish_row(xpng_ptr);
               return;
            }
            break;
      }
   }
#endif

   /* set up row info for transformations */
   xpng_ptr->row_info.color_type = xpng_ptr->color_type;
   xpng_ptr->row_info.width = xpng_ptr->usr_width;
   xpng_ptr->row_info.channels = xpng_ptr->usr_channels;
   xpng_ptr->row_info.bit_depth = xpng_ptr->usr_bit_depth;
   xpng_ptr->row_info.pixel_depth = (xpng_byte)(xpng_ptr->row_info.bit_depth *
      xpng_ptr->row_info.channels);

   xpng_ptr->row_info.rowbytes = ((xpng_ptr->row_info.width *
      (xpng_uint_32)xpng_ptr->row_info.pixel_depth + 7) >> 3);

   xpng_debug1(3, "row_info->color_type = %d\n", xpng_ptr->row_info.color_type);
   xpng_debug1(3, "row_info->width = %lu\n", xpng_ptr->row_info.width);
   xpng_debug1(3, "row_info->channels = %d\n", xpng_ptr->row_info.channels);
   xpng_debug1(3, "row_info->bit_depth = %d\n", xpng_ptr->row_info.bit_depth);
   xpng_debug1(3, "row_info->pixel_depth = %d\n", xpng_ptr->row_info.pixel_depth);
   xpng_debug1(3, "row_info->rowbytes = %lu\n", xpng_ptr->row_info.rowbytes);

   /* Copy user's row into buffer, leaving room for filter byte. */
   xpng_memcpy_check(xpng_ptr, xpng_ptr->row_buf + 1, row,
      xpng_ptr->row_info.rowbytes);

#if defined(PNG_WRITE_INTERLACING_SUPPORTED)
   /* handle interlacing */
   if (xpng_ptr->interlaced && xpng_ptr->pass < 6 &&
      (xpng_ptr->transformations & PNG_INTERLACE))
   {
      xpng_do_write_interlace(&(xpng_ptr->row_info),
         xpng_ptr->row_buf + 1, xpng_ptr->pass);
      /* this should always get caught above, but still ... */
      if (!(xpng_ptr->row_info.width))
      {
         xpng_write_finish_row(xpng_ptr);
         return;
      }
   }
#endif

   /* handle other transformations */
   if (xpng_ptr->transformations)
      xpng_do_write_transformations(xpng_ptr);

#if defined(PNG_MNG_FEATURES_SUPPORTED)
   /* Write filter_method 64 (intrapixel differencing) only if
    * 1. Libpng was compiled with PNG_MNG_FEATURES_SUPPORTED and
    * 2. Libpng did not write a PNG signature (this filter_method is only
    *    used in PNG datastreams that are embedded in MNG datastreams) and
    * 3. The application called xpng_permit_mng_features with a mask that
    *    included PNG_FLAG_MNG_FILTER_64 and
    * 4. The filter_method is 64 and
    * 5. The color_type is RGB or RGBA
    */
   if((xpng_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
      (xpng_ptr->filter_type == PNG_INTRAPIXEL_DIFFERENCING))
   {
      /* Intrapixel differencing */
      xpng_do_write_intrapixel(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
   }
#endif

   /* Find a filter if necessary, filter the row and write it out. */
   xpng_write_find_filter(xpng_ptr, &(xpng_ptr->row_info));

   if (xpng_ptr->write_row_fn != NULL)
      (*(xpng_ptr->write_row_fn))(xpng_ptr, xpng_ptr->row_number, xpng_ptr->pass);
}

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
/* Set the automatic flush interval or 0 to turn flushing off */
void PNGAPI
xpng_set_flush(xpng_structp xpng_ptr, int nrows)
{
   xpng_debug(1, "in xpng_set_flush\n");
   xpng_ptr->flush_dist = (nrows < 0 ? 0 : nrows);
}

/* flush the current output buffers now */
void PNGAPI
xpng_write_flush(xpng_structp xpng_ptr)
{
   int wrote_IDAT;

   xpng_debug(1, "in xpng_write_flush\n");
   /* We have already written out all of the data */
   if (xpng_ptr->row_number >= xpng_ptr->num_rows)
     return;

   do
   {
      int ret;

      /* compress the data */
      ret = deflate(&xpng_ptr->zstream, Z_SYNC_FLUSH);
      wrote_IDAT = 0;

      /* check for compression errors */
      if (ret != Z_OK)
      {
         if (xpng_ptr->zstream.msg != NULL)
            xpng_error(xpng_ptr, xpng_ptr->zstream.msg);
         else
            xpng_error(xpng_ptr, "zlib error");
      }

      if (!(xpng_ptr->zstream.avail_out))
      {
         /* write the IDAT and reset the zlib output buffer */
         xpng_write_IDAT(xpng_ptr, xpng_ptr->zbuf,
                        xpng_ptr->zbuf_size);
         xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
         xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
         wrote_IDAT = 1;
      }
   } while(wrote_IDAT == 1);

   /* If there is any data left to be output, write it into a new IDAT */
   if (xpng_ptr->zbuf_size != xpng_ptr->zstream.avail_out)
   {
      /* write the IDAT and reset the zlib output buffer */
      xpng_write_IDAT(xpng_ptr, xpng_ptr->zbuf,
                     xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out);
      xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
      xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
   }
   xpng_ptr->flush_rows = 0;
   xpng_flush(xpng_ptr);
}
#endif /* PNG_WRITE_FLUSH_SUPPORTED */

/* free all memory used by the write */
void PNGAPI
xpng_destroy_write_struct(xpng_structpp xpng_ptr_ptr, xpng_infopp info_ptr_ptr)
{
   xpng_structp xpng_ptr = NULL;
   xpng_infop info_ptr = NULL;
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_free_ptr free_fn = NULL;
   xpng_voidp mem_ptr = NULL;
#endif

   xpng_debug(1, "in xpng_destroy_write_struct\n");
   if (xpng_ptr_ptr != NULL)
   {
      xpng_ptr = *xpng_ptr_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
      free_fn = xpng_ptr->free_fn;
      mem_ptr = xpng_ptr->mem_ptr;
#endif
   }

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_ALL, -1);

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
      if (xpng_ptr->num_chunk_list)
      {
         xpng_free(xpng_ptr, xpng_ptr->chunk_list);
         xpng_ptr->chunk_list=NULL;
         xpng_ptr->num_chunk_list=0;
      }
#endif

#ifdef PNG_USER_MEM_SUPPORTED
      xpng_destroy_struct_2((xpng_voidp)info_ptr, (xpng_free_ptr)free_fn,
         (xpng_voidp)mem_ptr);
#else
      xpng_destroy_struct((xpng_voidp)info_ptr);
#endif
      *info_ptr_ptr = NULL;
   }

   if (xpng_ptr != NULL)
   {
      xpng_write_destroy(xpng_ptr);
#ifdef PNG_USER_MEM_SUPPORTED
      xpng_destroy_struct_2((xpng_voidp)xpng_ptr, (xpng_free_ptr)free_fn,
         (xpng_voidp)mem_ptr);
#else
      xpng_destroy_struct((xpng_voidp)xpng_ptr);
#endif
      *xpng_ptr_ptr = NULL;
   }
}


/* Free any memory used in xpng_ptr struct (old method) */
void /* PRIVATE */
xpng_write_destroy(xpng_structp xpng_ptr)
{
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf tmp_jmp; /* save jump buffer */
#endif
   xpng_error_ptr error_fn;
   xpng_error_ptr warning_fn;
   xpng_voidp error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_free_ptr free_fn;
#endif

   xpng_debug(1, "in xpng_write_destroy\n");
   /* free any memory zlib uses */
   deflateEnd(&xpng_ptr->zstream);

   /* free our memory.  xpng_free checks NULL for us. */
   xpng_free(xpng_ptr, xpng_ptr->zbuf);
   xpng_free(xpng_ptr, xpng_ptr->row_buf);
   xpng_free(xpng_ptr, xpng_ptr->prev_row);
   xpng_free(xpng_ptr, xpng_ptr->sub_row);
   xpng_free(xpng_ptr, xpng_ptr->up_row);
   xpng_free(xpng_ptr, xpng_ptr->avg_row);
   xpng_free(xpng_ptr, xpng_ptr->paeth_row);

#if defined(PNG_TIME_RFC1123_SUPPORTED)
   xpng_free(xpng_ptr, xpng_ptr->time_buffer);
#endif

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
   xpng_free(xpng_ptr, xpng_ptr->prev_filters);
   xpng_free(xpng_ptr, xpng_ptr->filter_weights);
   xpng_free(xpng_ptr, xpng_ptr->inv_filter_weights);
   xpng_free(xpng_ptr, xpng_ptr->filter_costs);
   xpng_free(xpng_ptr, xpng_ptr->inv_filter_costs);
#endif

#ifdef PNG_SETJMP_SUPPORTED
   /* reset structure */
   xpng_memcpy(tmp_jmp, xpng_ptr->jmpbuf, sizeof (jmp_buf));
#endif

   error_fn = xpng_ptr->error_fn;
   warning_fn = xpng_ptr->warning_fn;
   error_ptr = xpng_ptr->error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   free_fn = xpng_ptr->free_fn;
#endif

   xpng_memset(xpng_ptr, 0, sizeof (xpng_struct));

   xpng_ptr->error_fn = error_fn;
   xpng_ptr->warning_fn = warning_fn;
   xpng_ptr->error_ptr = error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_ptr->free_fn = free_fn;
#endif

#ifdef PNG_SETJMP_SUPPORTED
   xpng_memcpy(xpng_ptr->jmpbuf, tmp_jmp, sizeof (jmp_buf));
#endif
}

/* Allow the application to select one or more row filters to use. */
void PNGAPI
xpng_set_filter(xpng_structp xpng_ptr, int method, int filters)
{
   xpng_debug(1, "in xpng_set_filter\n");
#if defined(PNG_MNG_FEATURES_SUPPORTED)
   if((xpng_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
      (method == PNG_INTRAPIXEL_DIFFERENCING))
         method = PNG_FILTER_TYPE_BASE;
#endif
   if (method == PNG_FILTER_TYPE_BASE)
   {
      switch (filters & (PNG_ALL_FILTERS | 0x07))
      {
         case 5:
         case 6:
         case 7: xpng_warning(xpng_ptr, "Unknown row filter for method 0");
         case PNG_FILTER_VALUE_NONE:  xpng_ptr->do_filter=PNG_FILTER_NONE; break;
         case PNG_FILTER_VALUE_SUB:   xpng_ptr->do_filter=PNG_FILTER_SUB;  break;
         case PNG_FILTER_VALUE_UP:    xpng_ptr->do_filter=PNG_FILTER_UP;   break;
         case PNG_FILTER_VALUE_AVG:   xpng_ptr->do_filter=PNG_FILTER_AVG;  break;
         case PNG_FILTER_VALUE_PAETH: xpng_ptr->do_filter=PNG_FILTER_PAETH;break;
         default: xpng_ptr->do_filter = (xpng_byte)filters; break;
      }

      /* If we have allocated the row_buf, this means we have already started
       * with the image and we should have allocated all of the filter buffers
       * that have been selected.  If prev_row isn't already allocated, then
       * it is too late to start using the filters that need it, since we
       * will be missing the data in the previous row.  If an application
       * wants to start and stop using particular filters during compression,
       * it should start out with all of the filters, and then add and
       * remove them after the start of compression.
       */
      if (xpng_ptr->row_buf != NULL)
      {
         if ((xpng_ptr->do_filter & PNG_FILTER_SUB) && xpng_ptr->sub_row == NULL)
         {
            xpng_ptr->sub_row = (xpng_bytep)xpng_malloc(xpng_ptr,
              (xpng_ptr->rowbytes + 1));
            xpng_ptr->sub_row[0] = PNG_FILTER_VALUE_SUB;
         }

         if ((xpng_ptr->do_filter & PNG_FILTER_UP) && xpng_ptr->up_row == NULL)
         {
            if (xpng_ptr->prev_row == NULL)
            {
               xpng_warning(xpng_ptr, "Can't add Up filter after starting");
               xpng_ptr->do_filter &= ~PNG_FILTER_UP;
            }
            else
            {
               xpng_ptr->up_row = (xpng_bytep)xpng_malloc(xpng_ptr,
                  (xpng_ptr->rowbytes + 1));
               xpng_ptr->up_row[0] = PNG_FILTER_VALUE_UP;
            }
         }

         if ((xpng_ptr->do_filter & PNG_FILTER_AVG) && xpng_ptr->avg_row == NULL)
         {
            if (xpng_ptr->prev_row == NULL)
            {
               xpng_warning(xpng_ptr, "Can't add Average filter after starting");
               xpng_ptr->do_filter &= ~PNG_FILTER_AVG;
            }
            else
            {
               xpng_ptr->avg_row = (xpng_bytep)xpng_malloc(xpng_ptr,
                  (xpng_ptr->rowbytes + 1));
               xpng_ptr->avg_row[0] = PNG_FILTER_VALUE_AVG;
            }
         }

         if ((xpng_ptr->do_filter & PNG_FILTER_PAETH) &&
             xpng_ptr->paeth_row == NULL)
         {
            if (xpng_ptr->prev_row == NULL)
            {
               xpng_warning(xpng_ptr, "Can't add Paeth filter after starting");
               xpng_ptr->do_filter &= (xpng_byte)(~PNG_FILTER_PAETH);
            }
            else
            {
               xpng_ptr->paeth_row = (xpng_bytep)xpng_malloc(xpng_ptr,
                  (xpng_ptr->rowbytes + 1));
               xpng_ptr->paeth_row[0] = PNG_FILTER_VALUE_PAETH;
            }
         }

         if (xpng_ptr->do_filter == PNG_NO_FILTERS)
            xpng_ptr->do_filter = PNG_FILTER_NONE;
      }
   }
   else
      xpng_error(xpng_ptr, "Unknown custom filter method");
}

/* This allows us to influence the way in which libpng chooses the "best"
 * filter for the current scanline.  While the "minimum-sum-of-absolute-
 * differences metric is relatively fast and effective, there is some
 * question as to whether it can be improved upon by trying to keep the
 * filtered data going to zlib more consistent, hopefully resulting in
 * better compression.
 */
#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)      /* GRR 970116 */
void PNGAPI
xpng_set_filter_heuristics(xpng_structp xpng_ptr, int heuristic_method,
   int num_weights, xpng_doublep filter_weights,
   xpng_doublep filter_costs)
{
   int i;

   xpng_debug(1, "in xpng_set_filter_heuristics\n");
   if (heuristic_method >= PNG_FILTER_HEURISTIC_LAST)
   {
      xpng_warning(xpng_ptr, "Unknown filter heuristic method");
      return;
   }

   if (heuristic_method == PNG_FILTER_HEURISTIC_DEFAULT)
   {
      heuristic_method = PNG_FILTER_HEURISTIC_UNWEIGHTED;
   }

   if (num_weights < 0 || filter_weights == NULL ||
      heuristic_method == PNG_FILTER_HEURISTIC_UNWEIGHTED)
   {
      num_weights = 0;
   }

   xpng_ptr->num_prev_filters = (xpng_byte)num_weights;
   xpng_ptr->heuristic_method = (xpng_byte)heuristic_method;

   if (num_weights > 0)
   {
      if (xpng_ptr->prev_filters == NULL)
      {
         xpng_ptr->prev_filters = (xpng_bytep)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(sizeof(xpng_byte) * num_weights));

         /* To make sure that the weighting starts out fairly */
         for (i = 0; i < num_weights; i++)
         {
            xpng_ptr->prev_filters[i] = 255;
         }
      }

      if (xpng_ptr->filter_weights == NULL)
      {
         xpng_ptr->filter_weights = (xpng_uint_16p)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(sizeof(xpng_uint_16) * num_weights));

         xpng_ptr->inv_filter_weights = (xpng_uint_16p)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(sizeof(xpng_uint_16) * num_weights));
         for (i = 0; i < num_weights; i++)
         {
            xpng_ptr->inv_filter_weights[i] =
            xpng_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }
      }

      for (i = 0; i < num_weights; i++)
      {
         if (filter_weights[i] < 0.0)
         {
            xpng_ptr->inv_filter_weights[i] =
            xpng_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }
         else
         {
            xpng_ptr->inv_filter_weights[i] =
               (xpng_uint_16)((double)PNG_WEIGHT_FACTOR*filter_weights[i]+0.5);
            xpng_ptr->filter_weights[i] =
               (xpng_uint_16)((double)PNG_WEIGHT_FACTOR/filter_weights[i]+0.5);
         }
      }
   }

   /* If, in the future, there are other filter methods, this would
    * need to be based on xpng_ptr->filter.
    */
   if (xpng_ptr->filter_costs == NULL)
   {
      xpng_ptr->filter_costs = (xpng_uint_16p)xpng_malloc(xpng_ptr,
         (xpng_uint_32)(sizeof(xpng_uint_16) * PNG_FILTER_VALUE_LAST));

      xpng_ptr->inv_filter_costs = (xpng_uint_16p)xpng_malloc(xpng_ptr,
         (xpng_uint_32)(sizeof(xpng_uint_16) * PNG_FILTER_VALUE_LAST));

      for (i = 0; i < PNG_FILTER_VALUE_LAST; i++)
      {
         xpng_ptr->inv_filter_costs[i] =
         xpng_ptr->filter_costs[i] = PNG_COST_FACTOR;
      }
   }

   /* Here is where we set the relative costs of the different filters.  We
    * should take the desired compression level into account when setting
    * the costs, so that Paeth, for instance, has a high relative cost at low
    * compression levels, while it has a lower relative cost at higher
    * compression settings.  The filter types are in order of increasing
    * relative cost, so it would be possible to do this with an algorithm.
    */
   for (i = 0; i < PNG_FILTER_VALUE_LAST; i++)
   {
      if (filter_costs == NULL || filter_costs[i] < 0.0)
      {
         xpng_ptr->inv_filter_costs[i] =
         xpng_ptr->filter_costs[i] = PNG_COST_FACTOR;
      }
      else if (filter_costs[i] >= 1.0)
      {
         xpng_ptr->inv_filter_costs[i] =
            (xpng_uint_16)((double)PNG_COST_FACTOR / filter_costs[i] + 0.5);
         xpng_ptr->filter_costs[i] =
            (xpng_uint_16)((double)PNG_COST_FACTOR * filter_costs[i] + 0.5);
      }
   }
}
#endif /* PNG_WRITE_WEIGHTED_FILTER_SUPPORTED */

void PNGAPI
xpng_set_compression_level(xpng_structp xpng_ptr, int level)
{
   xpng_debug(1, "in xpng_set_compression_level\n");
   xpng_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_LEVEL;
   xpng_ptr->zlib_level = level;
}

void PNGAPI
xpng_set_compression_mem_level(xpng_structp xpng_ptr, int mem_level)
{
   xpng_debug(1, "in xpng_set_compression_mem_level\n");
   xpng_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL;
   xpng_ptr->zlib_mem_level = mem_level;
}

void PNGAPI
xpng_set_compression_strategy(xpng_structp xpng_ptr, int strategy)
{
   xpng_debug(1, "in xpng_set_compression_strategy\n");
   xpng_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_STRATEGY;
   xpng_ptr->zlib_strategy = strategy;
}

void PNGAPI
xpng_set_compression_window_bits(xpng_structp xpng_ptr, int window_bits)
{
   if (window_bits > 15)
      xpng_warning(xpng_ptr, "Only compression windows <= 32k supported by PNG");
   else if (window_bits < 8)
      xpng_warning(xpng_ptr, "Only compression windows >= 256 supported by PNG");
#ifndef WBITS_8_OK
   /* avoid libpng bug with 256-byte windows */
   if (window_bits == 8)
     {
       xpng_warning(xpng_ptr, "Compression window is being reset to 512");
       window_bits=9;
     }
#endif
   xpng_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS;
   xpng_ptr->zlib_window_bits = window_bits;
}

void PNGAPI
xpng_set_compression_method(xpng_structp xpng_ptr, int method)
{
   xpng_debug(1, "in xpng_set_compression_method\n");
   if (method != 8)
      xpng_warning(xpng_ptr, "Only compression method 8 is supported by PNG");
   xpng_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_METHOD;
   xpng_ptr->zlib_method = method;
}

void PNGAPI
xpng_set_write_status_fn(xpng_structp xpng_ptr, xpng_write_status_ptr write_row_fn)
{
   xpng_ptr->write_row_fn = write_row_fn;
}

#if defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
void PNGAPI
xpng_set_write_user_transform_fn(xpng_structp xpng_ptr, xpng_user_transform_ptr
   write_user_transform_fn)
{
   xpng_debug(1, "in xpng_set_write_user_transform_fn\n");
   xpng_ptr->transformations |= PNG_USER_TRANSFORM;
   xpng_ptr->write_user_transform_fn = write_user_transform_fn;
}
#endif


#if defined(PNG_INFO_IMAGE_SUPPORTED)
void PNGAPI
xpng_write_png(xpng_structp xpng_ptr, xpng_infop info_ptr,
              int transforms, voidp params)
{
#if defined(PNG_WRITE_INVERT_ALPHA_SUPPORTED)
   /* invert the alpha channel from opacity to transparency */
   if (transforms & PNG_TRANSFORM_INVERT_ALPHA)
       xpng_set_invert_alpha(xpng_ptr);
#endif

   /* Write the file header information. */
   xpng_write_info(xpng_ptr, info_ptr);

   /* ------ these transformations don't touch the info structure ------- */

#if defined(PNG_WRITE_INVERT_SUPPORTED)
   /* invert monochrome pixels */
   if (transforms & PNG_TRANSFORM_INVERT_MONO)
       xpng_set_invert_mono(xpng_ptr);
#endif

#if defined(PNG_WRITE_SHIFT_SUPPORTED)
   /* Shift the pixels up to a legal bit depth and fill in
    * as appropriate to correctly scale the image.
    */
   if ((transforms & PNG_TRANSFORM_SHIFT)
               && (info_ptr->valid & PNG_INFO_sBIT))
       xpng_set_shift(xpng_ptr, &info_ptr->sig_bit);
#endif

#if defined(PNG_WRITE_PACK_SUPPORTED)
   /* pack pixels into bytes */
   if (transforms & PNG_TRANSFORM_PACKING)
       xpng_set_packing(xpng_ptr);
#endif

#if defined(PNG_WRITE_SWAP_ALPHA_SUPPORTED)
   /* swap location of alpha bytes from ARGB to RGBA */
   if (transforms & PNG_TRANSFORM_SWAP_ALPHA)
       xpng_set_swap_alpha(xpng_ptr);
#endif

#if defined(PNG_WRITE_FILLER_SUPPORTED)
   /* Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
    * RGB (4 channels -> 3 channels). The second parameter is not used.
    */
   if (transforms & PNG_TRANSFORM_STRIP_FILLER)
       xpng_set_filler(xpng_ptr, 0, PNG_FILLER_BEFORE);
#endif

#if defined(PNG_WRITE_BGR_SUPPORTED)
   /* flip BGR pixels to RGB */
   if (transforms & PNG_TRANSFORM_BGR)
       xpng_set_bgr(xpng_ptr);
#endif

#if defined(PNG_WRITE_SWAP_SUPPORTED)
   /* swap bytes of 16-bit files to most significant byte first */
   if (transforms & PNG_TRANSFORM_SWAP_ENDIAN)
       xpng_set_swap(xpng_ptr);
#endif

#if defined(PNG_WRITE_PACKSWAP_SUPPORTED)
   /* swap bits of 1, 2, 4 bit packed pixel formats */
   if (transforms & PNG_TRANSFORM_PACKSWAP)
       xpng_set_packswap(xpng_ptr);
#endif

   /* ----------------------- end of transformations ------------------- */

   /* write the bits */
   if (info_ptr->valid & PNG_INFO_IDAT)
       xpng_write_image(xpng_ptr, info_ptr->row_pointers);

   /* It is REQUIRED to call this to finish writing the rest of the file */
   xpng_write_end(xpng_ptr, info_ptr);

   if(transforms == 0 || params == NULL)
      /* quiet compiler warnings */ return;
}
#endif
#endif /* PNG_WRITE_SUPPORTED */
