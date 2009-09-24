// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngread.c - read a PNG file
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This file contains routines that an application calls directly to
 * read a PNG file or stream.
 */

#define PNG_INTERNAL
#include "png.h"

/* Create a PNG structure for reading, and allocate any memory needed. */
xpng_structp PNGAPI
xpng_create_read_struct(xpng_const_charp user_xpng_ver, xpng_voidp error_ptr,
   xpng_error_ptr error_fn, xpng_error_ptr warn_fn)
{

#ifdef PNG_USER_MEM_SUPPORTED
   return (xpng_create_read_struct_2(user_xpng_ver, error_ptr, error_fn,
      warn_fn, xpng_voidp_NULL, xpng_malloc_ptr_NULL, xpng_free_ptr_NULL));
}

/* Alternate create PNG structure for reading, and allocate any memory needed. */
xpng_structp PNGAPI
xpng_create_read_struct_2(xpng_const_charp user_xpng_ver, xpng_voidp error_ptr,
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

   xpng_debug(1, "in xpng_create_read_struct\n");
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_ptr = (xpng_structp)xpng_create_struct_2(PNG_STRUCT_PNG,
      (xpng_malloc_ptr)malloc_fn, (xpng_voidp)mem_ptr);
#else
   xpng_ptr = (xpng_structp)xpng_create_struct(PNG_STRUCT_PNG);
#endif
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
#ifdef PNG_USER_MEM_SUPPORTED
      xpng_destroy_struct_2((xpng_voidp)xpng_ptr, 
         (xpng_free_ptr)free_fn, (xpng_voidp)mem_ptr);
#else
      xpng_destroy_struct((xpng_voidp)xpng_ptr);
#endif
      return (NULL);
   }
#ifdef USE_FAR_KEYWORD
   xpng_memcpy(xpng_ptr->jmpbuf,jmpbuf,sizeof(jmp_buf));
#endif
#endif

#ifdef PNG_USER_MEM_SUPPORTED
   xpng_set_mem_fn(xpng_ptr, mem_ptr, malloc_fn, free_fn);
#endif

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
   xpng_ptr->zstream.zalloc = xpng_zalloc;
   xpng_ptr->zstream.zfree = xpng_zfree;
   xpng_ptr->zstream.opaque = (voidpf)xpng_ptr;

   switch (inflateInit(&xpng_ptr->zstream))
   {
     case Z_OK: /* Do nothing */ break;
     case Z_MEM_ERROR:
     case Z_STREAM_ERROR: xpng_error(xpng_ptr, "zlib memory error"); break;
     case Z_VERSION_ERROR: xpng_error(xpng_ptr, "zlib version error"); break;
     default: xpng_error(xpng_ptr, "Unknown zlib error");
   }

   xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
   xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;

   xpng_set_read_fn(xpng_ptr, xpng_voidp_NULL, xpng_rw_ptr_NULL);

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

/* Initialize PNG structure for reading, and allocate any memory needed.
   This interface is deprecated in favour of the xpng_create_read_struct(),
   and it will eventually disappear. */
#undef xpng_read_init
void PNGAPI
xpng_read_init(xpng_structp xpng_ptr)
{
   /* We only come here via pre-1.0.7-compiled applications */
   xpng_read_init_2(xpng_ptr, "1.0.6 or earlier", 0, 0);
}

void PNGAPI
xpng_read_init_2(xpng_structp xpng_ptr, xpng_const_charp user_xpng_ver,
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
       "The png struct allocated by the application for reading is too small.");
     }
   if(sizeof(xpng_info) > xpng_info_size)
     {
       xpng_ptr->error_fn=NULL;
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
       xpng_ptr->flags=0;
#endif
       xpng_error(xpng_ptr,
         "The info struct allocated by application for reading is too small.");
     }
   xpng_read_init_3(&xpng_ptr, user_xpng_ver, xpng_struct_size);
}

void PNGAPI
xpng_read_init_3(xpng_structpp ptr_ptr, xpng_const_charp user_xpng_ver,
   xpng_size_t xpng_struct_size)
{
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf tmp_jmp;  /* to save current jump buffer */
#endif

   int i=0;

   xpng_structp xpng_ptr=*ptr_ptr;

   do
   {
     if(user_xpng_ver[i] != xpng_libxpng_ver[i])
     {
#ifdef PNG_LEGACY_SUPPORTED
       xpng_ptr->flags |= PNG_FLAG_LIBRARY_MISMATCH;
#else
       xpng_ptr->warning_fn=NULL;
       xpng_warning(xpng_ptr,
        "Application uses deprecated xpng_read_init() and should be recompiled.");
       break;
#endif
     }
   } while (xpng_libxpng_ver[i++]);

   xpng_debug(1, "in xpng_read_init_3\n");

#ifdef PNG_SETJMP_SUPPORTED
   /* save jump buffer and error functions */
   xpng_memcpy(tmp_jmp, xpng_ptr->jmpbuf, sizeof (jmp_buf));
#endif

   if(sizeof(xpng_struct) > xpng_struct_size)
     {
       xpng_destroy_struct(xpng_ptr);
       *ptr_ptr = (xpng_structp)xpng_create_struct(PNG_STRUCT_PNG);
       xpng_ptr = *ptr_ptr;
     }

   /* reset all variables to 0 */
   xpng_memset(xpng_ptr, 0, sizeof (xpng_struct));

#ifdef PNG_SETJMP_SUPPORTED
   /* restore jump buffer */
   xpng_memcpy(xpng_ptr->jmpbuf, tmp_jmp, sizeof (jmp_buf));
#endif

   /* initialize zbuf - compression buffer */
   xpng_ptr->zbuf_size = PNG_ZBUF_SIZE;
   xpng_ptr->zbuf = (xpng_bytep)xpng_malloc(xpng_ptr,
     (xpng_uint_32)xpng_ptr->zbuf_size);
   xpng_ptr->zstream.zalloc = xpng_zalloc;
   xpng_ptr->zstream.zfree = xpng_zfree;
   xpng_ptr->zstream.opaque = (voidpf)xpng_ptr;

   switch (inflateInit(&xpng_ptr->zstream))
   {
     case Z_OK: /* Do nothing */ break;
     case Z_MEM_ERROR:
     case Z_STREAM_ERROR: xpng_error(xpng_ptr, "zlib memory"); break;
     case Z_VERSION_ERROR: xpng_error(xpng_ptr, "zlib version"); break;
     default: xpng_error(xpng_ptr, "Unknown zlib error");
   }

   xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
   xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;

   xpng_set_read_fn(xpng_ptr, xpng_voidp_NULL, xpng_rw_ptr_NULL);
}

/* Read the information before the actual image data.  This has been
 * changed in v0.90 to allow reading a file that already has the magic
 * bytes read from the stream.  You can tell libpng how many bytes have
 * been read from the beginning of the stream (up to the maximum of 8)
 * via xpng_set_sig_bytes(), and we will only check the remaining bytes
 * here.  The application can then have access to the signature bytes we
 * read if it is determined that this isn't a valid PNG file.
 */
void PNGAPI
xpng_read_info(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   xpng_debug(1, "in xpng_read_info\n");
   /* If we haven't checked all of the PNG signature bytes, do so now. */
   if (xpng_ptr->sig_bytes < 8)
   {
      xpng_size_t num_checked = xpng_ptr->sig_bytes,
                 num_to_check = 8 - num_checked;

      xpng_read_data(xpng_ptr, &(info_ptr->signature[num_checked]), num_to_check);
      xpng_ptr->sig_bytes = 8;

      if (xpng_sig_cmp(info_ptr->signature, num_checked, num_to_check))
      {
         if (num_checked < 4 &&
             xpng_sig_cmp(info_ptr->signature, num_checked, num_to_check - 4))
            xpng_error(xpng_ptr, "Not a PNG file");
         else
            xpng_error(xpng_ptr, "PNG file corrupted by ASCII conversion");
      }
      if (num_checked < 3)
         xpng_ptr->mode |= PNG_HAVE_PNG_SIGNATURE;
   }

   for(;;)
   {
#ifdef PNG_USE_LOCAL_ARRAYS
      PNG_IHDR;
      PNG_IDAT;
      PNG_IEND;
      PNG_PLTE;
#if defined(PNG_READ_bKGD_SUPPORTED)
      PNG_bKGD;
#endif
#if defined(PNG_READ_cHRM_SUPPORTED)
      PNG_cHRM;
#endif
#if defined(PNG_READ_gAMA_SUPPORTED)
      PNG_gAMA;
#endif
#if defined(PNG_READ_hIST_SUPPORTED)
      PNG_hIST;
#endif
#if defined(PNG_READ_iCCP_SUPPORTED)
      PNG_iCCP;
#endif
#if defined(PNG_READ_iTXt_SUPPORTED)
      PNG_iTXt;
#endif
#if defined(PNG_READ_oFFs_SUPPORTED)
      PNG_oFFs;
#endif
#if defined(PNG_READ_pCAL_SUPPORTED)
      PNG_pCAL;
#endif
#if defined(PNG_READ_pHYs_SUPPORTED)
      PNG_pHYs;
#endif
#if defined(PNG_READ_sBIT_SUPPORTED)
      PNG_sBIT;
#endif
#if defined(PNG_READ_sCAL_SUPPORTED)
      PNG_sCAL;
#endif
#if defined(PNG_READ_sPLT_SUPPORTED)
      PNG_sPLT;
#endif
#if defined(PNG_READ_sRGB_SUPPORTED)
      PNG_sRGB;
#endif
#if defined(PNG_READ_tEXt_SUPPORTED)
      PNG_tEXt;
#endif
#if defined(PNG_READ_tIME_SUPPORTED)
      PNG_tIME;
#endif
#if defined(PNG_READ_tRNS_SUPPORTED)
      PNG_tRNS;
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
      PNG_zTXt;
#endif
#endif /* PNG_GLOBAL_ARRAYS */
      xpng_byte chunk_length[4];
      xpng_uint_32 length;

      xpng_read_data(xpng_ptr, chunk_length, 4);
      length = xpng_get_uint_32(chunk_length);

      xpng_reset_crc(xpng_ptr);
      xpng_crc_read(xpng_ptr, xpng_ptr->chunk_name, 4);

      xpng_debug2(0, "Reading %s chunk, length=%lu.\n", xpng_ptr->chunk_name,
         length);

      if (length > PNG_MAX_UINT)
         xpng_error(xpng_ptr, "Invalid chunk length.");

      /* This should be a binary subdivision search or a hash for
       * matching the chunk name rather than a linear search.
       */
      if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IHDR, 4))
         xpng_handle_IHDR(xpng_ptr, info_ptr, length);
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IEND, 4))
         xpng_handle_IEND(xpng_ptr, info_ptr, length);
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      else if (xpng_handle_as_unknown(xpng_ptr, xpng_ptr->chunk_name))
      {
         if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IDAT, 4))
            xpng_ptr->mode |= PNG_HAVE_IDAT;
         xpng_handle_unknown(xpng_ptr, info_ptr, length);
         if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_PLTE, 4))
            xpng_ptr->mode |= PNG_HAVE_PLTE;
         else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IDAT, 4))
         {
            if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
               xpng_error(xpng_ptr, "Missing IHDR before IDAT");
            else if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
                     !(xpng_ptr->mode & PNG_HAVE_PLTE))
               xpng_error(xpng_ptr, "Missing PLTE before IDAT");
            break;
         }
      }
#endif
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_PLTE, 4))
         xpng_handle_PLTE(xpng_ptr, info_ptr, length);
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IDAT, 4))
      {
         if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
            xpng_error(xpng_ptr, "Missing IHDR before IDAT");
         else if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
                  !(xpng_ptr->mode & PNG_HAVE_PLTE))
            xpng_error(xpng_ptr, "Missing PLTE before IDAT");

         xpng_ptr->idat_size = length;
         xpng_ptr->mode |= PNG_HAVE_IDAT;
         break;
      }
#if defined(PNG_READ_bKGD_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_bKGD, 4))
         xpng_handle_bKGD(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_cHRM_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_cHRM, 4))
         xpng_handle_cHRM(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_gAMA_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_gAMA, 4))
         xpng_handle_gAMA(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_hIST_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_hIST, 4))
         xpng_handle_hIST(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_oFFs_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_oFFs, 4))
         xpng_handle_oFFs(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_pCAL_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_pCAL, 4))
         xpng_handle_pCAL(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_sCAL_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sCAL, 4))
         xpng_handle_sCAL(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_pHYs_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_pHYs, 4))
         xpng_handle_pHYs(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_sBIT_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sBIT, 4))
         xpng_handle_sBIT(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_sRGB_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sRGB, 4))
         xpng_handle_sRGB(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_iCCP_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_iCCP, 4))
         xpng_handle_iCCP(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_sPLT_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sPLT, 4))
         xpng_handle_sPLT(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_tEXt_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tEXt, 4))
         xpng_handle_tEXt(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_tIME_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tIME, 4))
         xpng_handle_tIME(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_tRNS_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tRNS, 4))
         xpng_handle_tRNS(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_zTXt, 4))
         xpng_handle_zTXt(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_iTXt_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_iTXt, 4))
         xpng_handle_iTXt(xpng_ptr, info_ptr, length);
#endif
      else
         xpng_handle_unknown(xpng_ptr, info_ptr, length);
   }
}

/* optional call to update the users info_ptr structure */
void PNGAPI
xpng_read_update_info(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   xpng_debug(1, "in xpng_read_update_info\n");
   if (!(xpng_ptr->flags & PNG_FLAG_ROW_INIT))
      xpng_read_start_row(xpng_ptr);
   else
      xpng_warning(xpng_ptr,
      "Ignoring extra xpng_read_update_info() call; row buffer not reallocated");
   xpng_read_transform_info(xpng_ptr, info_ptr);
}

/* Initialize palette, background, etc, after transformations
 * are set, but before any reading takes place.  This allows
 * the user to obtain a gamma-corrected palette, for example.
 * If the user doesn't call this, we will do it ourselves.
 */
void PNGAPI
xpng_start_read_image(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_start_read_image\n");
   if (!(xpng_ptr->flags & PNG_FLAG_ROW_INIT))
      xpng_read_start_row(xpng_ptr);
}

void PNGAPI
xpng_read_row(xpng_structp xpng_ptr, xpng_bytep row, xpng_bytep dsp_row)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_IDAT;
   const int xpng_pass_dsp_mask[7] = {0xff, 0x0f, 0xff, 0x33, 0xff, 0x55, 0xff};
   const int xpng_pass_mask[7] = {0x80, 0x08, 0x88, 0x22, 0xaa, 0x55, 0xff};
#endif
   int ret;
   xpng_debug2(1, "in xpng_read_row (row %lu, pass %d)\n",
      xpng_ptr->row_number, xpng_ptr->pass);
   if (!(xpng_ptr->flags & PNG_FLAG_ROW_INIT))
      xpng_read_start_row(xpng_ptr);
   if (xpng_ptr->row_number == 0 && xpng_ptr->pass == 0)
   {
   /* check for transforms that have been set but were defined out */
#if defined(PNG_WRITE_INVERT_SUPPORTED) && !defined(PNG_READ_INVERT_SUPPORTED)
   if (xpng_ptr->transformations & PNG_INVERT_MONO)
      xpng_warning(xpng_ptr, "PNG_READ_INVERT_SUPPORTED is not defined.");
#endif
#if defined(PNG_WRITE_FILLER_SUPPORTED) && !defined(PNG_READ_FILLER_SUPPORTED)
   if (xpng_ptr->transformations & PNG_FILLER)
      xpng_warning(xpng_ptr, "PNG_READ_FILLER_SUPPORTED is not defined.");
#endif
#if defined(PNG_WRITE_PACKSWAP_SUPPORTED) && !defined(PNG_READ_PACKSWAP_SUPPORTED)
   if (xpng_ptr->transformations & PNG_PACKSWAP)
      xpng_warning(xpng_ptr, "PNG_READ_PACKSWAP_SUPPORTED is not defined.");
#endif
#if defined(PNG_WRITE_PACK_SUPPORTED) && !defined(PNG_READ_PACK_SUPPORTED)
   if (xpng_ptr->transformations & PNG_PACK)
      xpng_warning(xpng_ptr, "PNG_READ_PACK_SUPPORTED is not defined.");
#endif
#if defined(PNG_WRITE_SHIFT_SUPPORTED) && !defined(PNG_READ_SHIFT_SUPPORTED)
   if (xpng_ptr->transformations & PNG_SHIFT)
      xpng_warning(xpng_ptr, "PNG_READ_SHIFT_SUPPORTED is not defined.");
#endif
#if defined(PNG_WRITE_BGR_SUPPORTED) && !defined(PNG_READ_BGR_SUPPORTED)
   if (xpng_ptr->transformations & PNG_BGR)
      xpng_warning(xpng_ptr, "PNG_READ_BGR_SUPPORTED is not defined.");
#endif
#if defined(PNG_WRITE_SWAP_SUPPORTED) && !defined(PNG_READ_SWAP_SUPPORTED)
   if (xpng_ptr->transformations & PNG_SWAP_BYTES)
      xpng_warning(xpng_ptr, "PNG_READ_SWAP_SUPPORTED is not defined.");
#endif
   }

#if defined(PNG_READ_INTERLACING_SUPPORTED)
   /* if interlaced and we do not need a new row, combine row and return */
   if (xpng_ptr->interlaced && (xpng_ptr->transformations & PNG_INTERLACE))
   {
      switch (xpng_ptr->pass)
      {
         case 0:
            if (xpng_ptr->row_number & 0x07)
            {
               if (dsp_row != NULL)
                  xpng_combine_row(xpng_ptr, dsp_row,
                     xpng_pass_dsp_mask[xpng_ptr->pass]);
               xpng_read_finish_row(xpng_ptr);
               return;
            }
            break;
         case 1:
            if ((xpng_ptr->row_number & 0x07) || xpng_ptr->width < 5)
            {
               if (dsp_row != NULL)
                  xpng_combine_row(xpng_ptr, dsp_row,
                     xpng_pass_dsp_mask[xpng_ptr->pass]);
               xpng_read_finish_row(xpng_ptr);
               return;
            }
            break;
         case 2:
            if ((xpng_ptr->row_number & 0x07) != 4)
            {
               if (dsp_row != NULL && (xpng_ptr->row_number & 4))
                  xpng_combine_row(xpng_ptr, dsp_row,
                     xpng_pass_dsp_mask[xpng_ptr->pass]);
               xpng_read_finish_row(xpng_ptr);
               return;
            }
            break;
         case 3:
            if ((xpng_ptr->row_number & 3) || xpng_ptr->width < 3)
            {
               if (dsp_row != NULL)
                  xpng_combine_row(xpng_ptr, dsp_row,
                     xpng_pass_dsp_mask[xpng_ptr->pass]);
               xpng_read_finish_row(xpng_ptr);
               return;
            }
            break;
         case 4:
            if ((xpng_ptr->row_number & 3) != 2)
            {
               if (dsp_row != NULL && (xpng_ptr->row_number & 2))
                  xpng_combine_row(xpng_ptr, dsp_row,
                     xpng_pass_dsp_mask[xpng_ptr->pass]);
               xpng_read_finish_row(xpng_ptr);
               return;
            }
            break;
         case 5:
            if ((xpng_ptr->row_number & 1) || xpng_ptr->width < 2)
            {
               if (dsp_row != NULL)
                  xpng_combine_row(xpng_ptr, dsp_row,
                     xpng_pass_dsp_mask[xpng_ptr->pass]);
               xpng_read_finish_row(xpng_ptr);
               return;
            }
            break;
         case 6:
            if (!(xpng_ptr->row_number & 1))
            {
               xpng_read_finish_row(xpng_ptr);
               return;
            }
            break;
      }
   }
#endif

   if (!(xpng_ptr->mode & PNG_HAVE_IDAT))
      xpng_error(xpng_ptr, "Invalid attempt to read row data");

   xpng_ptr->zstream.next_out = xpng_ptr->row_buf;
   xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->irowbytes;
   do
   {
      if (!(xpng_ptr->zstream.avail_in))
      {
         while (!xpng_ptr->idat_size)
         {
            xpng_byte chunk_length[4];

            xpng_crc_finish(xpng_ptr, 0);

            xpng_read_data(xpng_ptr, chunk_length, 4);
            xpng_ptr->idat_size = xpng_get_uint_32(chunk_length);

            if (xpng_ptr->idat_size > PNG_MAX_UINT)
              xpng_error(xpng_ptr, "Invalid chunk length.");

            xpng_reset_crc(xpng_ptr);
            xpng_crc_read(xpng_ptr, xpng_ptr->chunk_name, 4);
            if (xpng_memcmp(xpng_ptr->chunk_name, xpng_IDAT, 4))
               xpng_error(xpng_ptr, "Not enough image data");
         }
         xpng_ptr->zstream.avail_in = (uInt)xpng_ptr->zbuf_size;
         xpng_ptr->zstream.next_in = xpng_ptr->zbuf;
         if (xpng_ptr->zbuf_size > xpng_ptr->idat_size)
            xpng_ptr->zstream.avail_in = (uInt)xpng_ptr->idat_size;
         xpng_crc_read(xpng_ptr, xpng_ptr->zbuf,
            (xpng_size_t)xpng_ptr->zstream.avail_in);
         xpng_ptr->idat_size -= xpng_ptr->zstream.avail_in;
      }
      ret = inflate(&xpng_ptr->zstream, Z_PARTIAL_FLUSH);
      if (ret == Z_STREAM_END)
      {
         if (xpng_ptr->zstream.avail_out || xpng_ptr->zstream.avail_in ||
            xpng_ptr->idat_size)
            xpng_error(xpng_ptr, "Extra compressed data");
         xpng_ptr->mode |= PNG_AFTER_IDAT;
         xpng_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
         break;
      }
      if (ret != Z_OK)
         xpng_error(xpng_ptr, xpng_ptr->zstream.msg ? xpng_ptr->zstream.msg :
                   "Decompression error");

   } while (xpng_ptr->zstream.avail_out);

   xpng_ptr->row_info.color_type = xpng_ptr->color_type;
   xpng_ptr->row_info.width = xpng_ptr->iwidth;
   xpng_ptr->row_info.channels = xpng_ptr->channels;
   xpng_ptr->row_info.bit_depth = xpng_ptr->bit_depth;
   xpng_ptr->row_info.pixel_depth = xpng_ptr->pixel_depth;
   xpng_ptr->row_info.rowbytes = ((xpng_ptr->row_info.width *
      (xpng_uint_32)xpng_ptr->row_info.pixel_depth + 7) >> 3);

   if(xpng_ptr->row_buf[0])
   xpng_read_filter_row(xpng_ptr, &(xpng_ptr->row_info),
      xpng_ptr->row_buf + 1, xpng_ptr->prev_row + 1,
      (int)(xpng_ptr->row_buf[0]));

   xpng_memcpy_check(xpng_ptr, xpng_ptr->prev_row, xpng_ptr->row_buf,
      xpng_ptr->rowbytes + 1);
   
#if defined(PNG_MNG_FEATURES_SUPPORTED)
   if((xpng_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
      (xpng_ptr->filter_type == PNG_INTRAPIXEL_DIFFERENCING))
   {
      /* Intrapixel differencing */
      xpng_do_read_intrapixel(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
   }
#endif

   if (xpng_ptr->transformations)
      xpng_do_read_transformations(xpng_ptr);

#if defined(PNG_READ_INTERLACING_SUPPORTED)
   /* blow up interlaced rows to full size */
   if (xpng_ptr->interlaced &&
      (xpng_ptr->transformations & PNG_INTERLACE))
   {
      if (xpng_ptr->pass < 6)
/*       old interface (pre-1.0.9):
         xpng_do_read_interlace(&(xpng_ptr->row_info),
            xpng_ptr->row_buf + 1, xpng_ptr->pass, xpng_ptr->transformations);
 */
         xpng_do_read_interlace(xpng_ptr);

      if (dsp_row != NULL)
         xpng_combine_row(xpng_ptr, dsp_row,
            xpng_pass_dsp_mask[xpng_ptr->pass]);
      if (row != NULL)
         xpng_combine_row(xpng_ptr, row,
            xpng_pass_mask[xpng_ptr->pass]);
   }
   else
#endif
   {
      if (row != NULL)
         xpng_combine_row(xpng_ptr, row, 0xff);
      if (dsp_row != NULL)
         xpng_combine_row(xpng_ptr, dsp_row, 0xff);
   }
   xpng_read_finish_row(xpng_ptr);

   if (xpng_ptr->read_row_fn != NULL)
      (*(xpng_ptr->read_row_fn))(xpng_ptr, xpng_ptr->row_number, xpng_ptr->pass);
}

/* Read one or more rows of image data.  If the image is interlaced,
 * and xpng_set_interlace_handling() has been called, the rows need to
 * contain the contents of the rows from the previous pass.  If the
 * image has alpha or transparency, and xpng_handle_alpha()[*] has been
 * called, the rows contents must be initialized to the contents of the
 * screen.
 *
 * "row" holds the actual image, and pixels are placed in it
 * as they arrive.  If the image is displayed after each pass, it will
 * appear to "sparkle" in.  "display_row" can be used to display a
 * "chunky" progressive image, with finer detail added as it becomes
 * available.  If you do not want this "chunky" display, you may pass
 * NULL for display_row.  If you do not want the sparkle display, and
 * you have not called xpng_handle_alpha(), you may pass NULL for rows.
 * If you have called xpng_handle_alpha(), and the image has either an
 * alpha channel or a transparency chunk, you must provide a buffer for
 * rows.  In this case, you do not have to provide a display_row buffer
 * also, but you may.  If the image is not interlaced, or if you have
 * not called xpng_set_interlace_handling(), the display_row buffer will
 * be ignored, so pass NULL to it.
 *
 * [*] xpng_handle_alpha() does not exist yet, as of libpng version 1.2.5
 */

void PNGAPI
xpng_read_rows(xpng_structp xpng_ptr, xpng_bytepp row,
   xpng_bytepp display_row, xpng_uint_32 num_rows)
{
   xpng_uint_32 i;
   xpng_bytepp rp;
   xpng_bytepp dp;

   xpng_debug(1, "in xpng_read_rows\n");
   rp = row;
   dp = display_row;
   if (rp != NULL && dp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         xpng_bytep rptr = *rp++;
         xpng_bytep dptr = *dp++;

         xpng_read_row(xpng_ptr, rptr, dptr);
      }
   else if(rp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         xpng_bytep rptr = *rp;
         xpng_read_row(xpng_ptr, rptr, xpng_bytep_NULL);
         rp++;
      }
   else if(dp != NULL)
      for (i = 0; i < num_rows; i++)
      {
         xpng_bytep dptr = *dp;
         xpng_read_row(xpng_ptr, xpng_bytep_NULL, dptr);
         dp++;
      }
}

/* Read the entire image.  If the image has an alpha channel or a tRNS
 * chunk, and you have called xpng_handle_alpha()[*], you will need to
 * initialize the image to the current image that PNG will be overlaying.
 * We set the num_rows again here, in case it was incorrectly set in
 * xpng_read_start_row() by a call to xpng_read_update_info() or
 * xpng_start_read_image() if xpng_set_interlace_handling() wasn't called
 * prior to either of these functions like it should have been.  You can
 * only call this function once.  If you desire to have an image for
 * each pass of a interlaced image, use xpng_read_rows() instead.
 *
 * [*] xpng_handle_alpha() does not exist yet, as of libpng version 1.2.5
 */
void PNGAPI
xpng_read_image(xpng_structp xpng_ptr, xpng_bytepp image)
{
   xpng_uint_32 i,image_height;
   int pass, j;
   xpng_bytepp rp;

   xpng_debug(1, "in xpng_read_image\n");

#ifdef PNG_READ_INTERLACING_SUPPORTED
   pass = xpng_set_interlace_handling(xpng_ptr);
#else
   if (xpng_ptr->interlaced)
      xpng_error(xpng_ptr,
        "Cannot read interlaced image -- interlace handler disabled.");
   pass = 1;
#endif


   image_height=xpng_ptr->height;
   xpng_ptr->num_rows = image_height; /* Make sure this is set correctly */

   for (j = 0; j < pass; j++)
   {
      rp = image;
      for (i = 0; i < image_height; i++)
      {
         xpng_read_row(xpng_ptr, *rp, xpng_bytep_NULL);
         rp++;
      }
   }
}

/* Read the end of the PNG file.  Will not read past the end of the
 * file, will verify the end is accurate, and will read any comments
 * or time information at the end of the file, if info is not NULL.
 */
void PNGAPI
xpng_read_end(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   xpng_byte chunk_length[4];
   xpng_uint_32 length;

   xpng_debug(1, "in xpng_read_end\n");
   xpng_crc_finish(xpng_ptr, 0); /* Finish off CRC from last IDAT chunk */

   do
   {
#ifdef PNG_USE_LOCAL_ARRAYS
      PNG_IHDR;
      PNG_IDAT;
      PNG_IEND;
      PNG_PLTE;
#if defined(PNG_READ_bKGD_SUPPORTED)
      PNG_bKGD;
#endif
#if defined(PNG_READ_cHRM_SUPPORTED)
      PNG_cHRM;
#endif
#if defined(PNG_READ_gAMA_SUPPORTED)
      PNG_gAMA;
#endif
#if defined(PNG_READ_hIST_SUPPORTED)
      PNG_hIST;
#endif
#if defined(PNG_READ_iCCP_SUPPORTED)
      PNG_iCCP;
#endif
#if defined(PNG_READ_iTXt_SUPPORTED)
      PNG_iTXt;
#endif
#if defined(PNG_READ_oFFs_SUPPORTED)
      PNG_oFFs;
#endif
#if defined(PNG_READ_pCAL_SUPPORTED)
      PNG_pCAL;
#endif
#if defined(PNG_READ_pHYs_SUPPORTED)
      PNG_pHYs;
#endif
#if defined(PNG_READ_sBIT_SUPPORTED)
      PNG_sBIT;
#endif
#if defined(PNG_READ_sCAL_SUPPORTED)
      PNG_sCAL;
#endif
#if defined(PNG_READ_sPLT_SUPPORTED)
      PNG_sPLT;
#endif
#if defined(PNG_READ_sRGB_SUPPORTED)
      PNG_sRGB;
#endif
#if defined(PNG_READ_tEXt_SUPPORTED)
      PNG_tEXt;
#endif
#if defined(PNG_READ_tIME_SUPPORTED)
      PNG_tIME;
#endif
#if defined(PNG_READ_tRNS_SUPPORTED)
      PNG_tRNS;
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
      PNG_zTXt;
#endif
#endif /* PNG_GLOBAL_ARRAYS */

      xpng_read_data(xpng_ptr, chunk_length, 4);
      length = xpng_get_uint_32(chunk_length);

      xpng_reset_crc(xpng_ptr);
      xpng_crc_read(xpng_ptr, xpng_ptr->chunk_name, 4);

      xpng_debug1(0, "Reading %s chunk.\n", xpng_ptr->chunk_name);

      if (length > PNG_MAX_UINT)
         xpng_error(xpng_ptr, "Invalid chunk length.");

      if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IHDR, 4))
         xpng_handle_IHDR(xpng_ptr, info_ptr, length);
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IEND, 4))
         xpng_handle_IEND(xpng_ptr, info_ptr, length);
#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
      else if (xpng_handle_as_unknown(xpng_ptr, xpng_ptr->chunk_name))
      {
         if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IDAT, 4))
         {
            if (length > 0 || xpng_ptr->mode & PNG_AFTER_IDAT)
               xpng_error(xpng_ptr, "Too many IDAT's found");
         }
         else
            xpng_ptr->mode |= PNG_AFTER_IDAT;
         xpng_handle_unknown(xpng_ptr, info_ptr, length);
         if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_PLTE, 4))
            xpng_ptr->mode |= PNG_HAVE_PLTE;
      }
#endif
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IDAT, 4))
      {
         /* Zero length IDATs are legal after the last IDAT has been
          * read, but not after other chunks have been read.
          */
         if (length > 0 || xpng_ptr->mode & PNG_AFTER_IDAT)
            xpng_error(xpng_ptr, "Too many IDAT's found");
         xpng_crc_finish(xpng_ptr, length);
      }
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_PLTE, 4))
         xpng_handle_PLTE(xpng_ptr, info_ptr, length);
#if defined(PNG_READ_bKGD_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_bKGD, 4))
         xpng_handle_bKGD(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_cHRM_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_cHRM, 4))
         xpng_handle_cHRM(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_gAMA_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_gAMA, 4))
         xpng_handle_gAMA(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_hIST_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_hIST, 4))
         xpng_handle_hIST(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_oFFs_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_oFFs, 4))
         xpng_handle_oFFs(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_pCAL_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_pCAL, 4))
         xpng_handle_pCAL(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_sCAL_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sCAL, 4))
         xpng_handle_sCAL(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_pHYs_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_pHYs, 4))
         xpng_handle_pHYs(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_sBIT_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sBIT, 4))
         xpng_handle_sBIT(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_sRGB_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sRGB, 4))
         xpng_handle_sRGB(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_iCCP_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_iCCP, 4))
         xpng_handle_iCCP(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_sPLT_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sPLT, 4))
         xpng_handle_sPLT(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_tEXt_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tEXt, 4))
         xpng_handle_tEXt(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_tIME_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tIME, 4))
         xpng_handle_tIME(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_tRNS_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tRNS, 4))
         xpng_handle_tRNS(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_zTXt, 4))
         xpng_handle_zTXt(xpng_ptr, info_ptr, length);
#endif
#if defined(PNG_READ_iTXt_SUPPORTED)
      else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_iTXt, 4))
         xpng_handle_iTXt(xpng_ptr, info_ptr, length);
#endif
      else
         xpng_handle_unknown(xpng_ptr, info_ptr, length);
   } while (!(xpng_ptr->mode & PNG_HAVE_IEND));
}

/* free all memory used by the read */
void PNGAPI
xpng_destroy_read_struct(xpng_structpp xpng_ptr_ptr, xpng_infopp info_ptr_ptr,
   xpng_infopp end_info_ptr_ptr)
{
   xpng_structp xpng_ptr = NULL;
   xpng_infop info_ptr = NULL, end_info_ptr = NULL;
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_free_ptr free_fn = NULL;
   xpng_voidp mem_ptr = NULL;
#endif

   xpng_debug(1, "in xpng_destroy_read_struct\n");
   if (xpng_ptr_ptr != NULL)
      xpng_ptr = *xpng_ptr_ptr;

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (end_info_ptr_ptr != NULL)
      end_info_ptr = *end_info_ptr_ptr;

#ifdef PNG_USER_MEM_SUPPORTED
   free_fn = xpng_ptr->free_fn;
   mem_ptr = xpng_ptr->mem_ptr;
#endif

   xpng_read_destroy(xpng_ptr, info_ptr, end_info_ptr);

   if (info_ptr != NULL)
   {
#if defined(PNG_TEXT_SUPPORTED)
      xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_TEXT, -1);
#endif

#ifdef PNG_USER_MEM_SUPPORTED
      xpng_destroy_struct_2((xpng_voidp)info_ptr, (xpng_free_ptr)free_fn,
          (xpng_voidp)mem_ptr);
#else
      xpng_destroy_struct((xpng_voidp)info_ptr);
#endif
      *info_ptr_ptr = NULL;
   }

   if (end_info_ptr != NULL)
   {
#if defined(PNG_READ_TEXT_SUPPORTED)
      xpng_free_data(xpng_ptr, end_info_ptr, PNG_FREE_TEXT, -1);
#endif
#ifdef PNG_USER_MEM_SUPPORTED
      xpng_destroy_struct_2((xpng_voidp)end_info_ptr, (xpng_free_ptr)free_fn,
         (xpng_voidp)mem_ptr);
#else
      xpng_destroy_struct((xpng_voidp)end_info_ptr);
#endif
      *end_info_ptr_ptr = NULL;
   }

   if (xpng_ptr != NULL)
   {
#ifdef PNG_USER_MEM_SUPPORTED
      xpng_destroy_struct_2((xpng_voidp)xpng_ptr, (xpng_free_ptr)free_fn,
          (xpng_voidp)mem_ptr);
#else
      xpng_destroy_struct((xpng_voidp)xpng_ptr);
#endif
      *xpng_ptr_ptr = NULL;
   }
}

/* free all memory used by the read (old method) */
void /* PRIVATE */
xpng_read_destroy(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_infop end_info_ptr)
{
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf tmp_jmp;
#endif
   xpng_error_ptr error_fn;
   xpng_error_ptr warning_fn;
   xpng_voidp error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_free_ptr free_fn;
#endif

   xpng_debug(1, "in xpng_read_destroy\n");
   if (info_ptr != NULL)
      xpng_info_destroy(xpng_ptr, info_ptr);

   if (end_info_ptr != NULL)
      xpng_info_destroy(xpng_ptr, end_info_ptr);

   xpng_free(xpng_ptr, xpng_ptr->zbuf);
   xpng_free(xpng_ptr, xpng_ptr->big_row_buf);
   xpng_free(xpng_ptr, xpng_ptr->prev_row);
#if defined(PNG_READ_DITHER_SUPPORTED)
   xpng_free(xpng_ptr, xpng_ptr->palette_lookup);
   xpng_free(xpng_ptr, xpng_ptr->dither_index);
#endif
#if defined(PNG_READ_GAMMA_SUPPORTED)
   xpng_free(xpng_ptr, xpng_ptr->gamma_table);
#endif
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   xpng_free(xpng_ptr, xpng_ptr->gamma_from_1);
   xpng_free(xpng_ptr, xpng_ptr->gamma_to_1);
#endif
#ifdef PNG_FREE_ME_SUPPORTED
   if (xpng_ptr->free_me & PNG_FREE_PLTE)
      xpng_zfree(xpng_ptr, xpng_ptr->palette);
   xpng_ptr->free_me &= ~PNG_FREE_PLTE;
#else
   if (xpng_ptr->flags & PNG_FLAG_FREE_PLTE)
      xpng_zfree(xpng_ptr, xpng_ptr->palette);
   xpng_ptr->flags &= ~PNG_FLAG_FREE_PLTE;
#endif
#if defined(PNG_tRNS_SUPPORTED) || \
    defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
#ifdef PNG_FREE_ME_SUPPORTED
   if (xpng_ptr->free_me & PNG_FREE_TRNS)
      xpng_free(xpng_ptr, xpng_ptr->trans);
   xpng_ptr->free_me &= ~PNG_FREE_TRNS;
#else
   if (xpng_ptr->flags & PNG_FLAG_FREE_TRNS)
      xpng_free(xpng_ptr, xpng_ptr->trans);
   xpng_ptr->flags &= ~PNG_FLAG_FREE_TRNS;
#endif
#endif
#if defined(PNG_READ_hIST_SUPPORTED)
#ifdef PNG_FREE_ME_SUPPORTED
   if (xpng_ptr->free_me & PNG_FREE_HIST)
      xpng_free(xpng_ptr, xpng_ptr->hist);
   xpng_ptr->free_me &= ~PNG_FREE_HIST;
#else
   if (xpng_ptr->flags & PNG_FLAG_FREE_HIST)
      xpng_free(xpng_ptr, xpng_ptr->hist);
   xpng_ptr->flags &= ~PNG_FLAG_FREE_HIST;
#endif
#endif
#if defined(PNG_READ_GAMMA_SUPPORTED)
   if (xpng_ptr->gamma_16_table != NULL)
   {
      int i;
      int istop = (1 << (8 - xpng_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         xpng_free(xpng_ptr, xpng_ptr->gamma_16_table[i]);
      }
   xpng_free(xpng_ptr, xpng_ptr->gamma_16_table);
   }
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   if (xpng_ptr->gamma_16_from_1 != NULL)
   {
      int i;
      int istop = (1 << (8 - xpng_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         xpng_free(xpng_ptr, xpng_ptr->gamma_16_from_1[i]);
      }
   xpng_free(xpng_ptr, xpng_ptr->gamma_16_from_1);
   }
   if (xpng_ptr->gamma_16_to_1 != NULL)
   {
      int i;
      int istop = (1 << (8 - xpng_ptr->gamma_shift));
      for (i = 0; i < istop; i++)
      {
         xpng_free(xpng_ptr, xpng_ptr->gamma_16_to_1[i]);
      }
   xpng_free(xpng_ptr, xpng_ptr->gamma_16_to_1);
   }
#endif
#endif
#if defined(PNG_TIME_RFC1123_SUPPORTED)
   xpng_free(xpng_ptr, xpng_ptr->time_buffer);
#endif

   inflateEnd(&xpng_ptr->zstream);
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   xpng_free(xpng_ptr, xpng_ptr->save_buffer);
#endif

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
#ifdef PNG_TEXT_SUPPORTED
   xpng_free(xpng_ptr, xpng_ptr->current_text);
#endif /* PNG_TEXT_SUPPORTED */
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

   /* Save the important info out of the xpng_struct, in case it is
    * being used again.
    */
#ifdef PNG_SETJMP_SUPPORTED
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

void PNGAPI
xpng_set_read_status_fn(xpng_structp xpng_ptr, xpng_read_status_ptr read_row_fn)
{
   xpng_ptr->read_row_fn = read_row_fn;
}

#if defined(PNG_INFO_IMAGE_SUPPORTED)
void PNGAPI
xpng_read_png(xpng_structp xpng_ptr, xpng_infop info_ptr,
                           int transforms,
                           voidp params)
{
   int row;

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED)
   /* invert the alpha channel from opacity to transparency */
   if (transforms & PNG_TRANSFORM_INVERT_ALPHA)
       xpng_set_invert_alpha(xpng_ptr);
#endif

   /* The call to xpng_read_info() gives us all of the information from the
    * PNG file before the first IDAT (image data chunk).
    */
   xpng_read_info(xpng_ptr, info_ptr);

   /* -------------- image transformations start here ------------------- */

#if defined(PNG_READ_16_TO_8_SUPPORTED)
   /* tell libpng to strip 16 bit/color files down to 8 bits/color */
   if (transforms & PNG_TRANSFORM_STRIP_16)
       xpng_set_strip_16(xpng_ptr);
#endif

#if defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
   /* Strip alpha bytes from the input data without combining with the
    * background (not recommended).
    */
   if (transforms & PNG_TRANSFORM_STRIP_ALPHA)
       xpng_set_strip_alpha(xpng_ptr);
#endif

#if defined(PNG_READ_PACK_SUPPORTED) && !defined(PNG_READ_EXPAND_SUPPORTED)
   /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
   if (transforms & PNG_TRANSFORM_PACKING)
       xpng_set_packing(xpng_ptr);
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED)
   /* Change the order of packed pixels to least significant bit first
    * (not useful if you are using xpng_set_packing). */
   if (transforms & PNG_TRANSFORM_PACKSWAP)
       xpng_set_packswap(xpng_ptr);
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
   /* Expand paletted colors into true RGB triplets
    * Expand grayscale images to full 8 bits from 1, 2, or 4 bits/pixel
    * Expand paletted or RGB images with transparency to full alpha
    * channels so the data will be available as RGBA quartets.
    */
   if (transforms & PNG_TRANSFORM_EXPAND)
       if ((xpng_ptr->bit_depth < 8) ||
           (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE) ||
           (xpng_get_valid(xpng_ptr, info_ptr, PNG_INFO_tRNS)))
         xpng_set_expand(xpng_ptr);
#endif

   /* We don't handle background color or gamma transformation or dithering. */

#if defined(PNG_READ_INVERT_SUPPORTED)
   /* invert monochrome files to have 0 as white and 1 as black */
   if (transforms & PNG_TRANSFORM_INVERT_MONO)
       xpng_set_invert_mono(xpng_ptr);
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
   /* If you want to shift the pixel values from the range [0,255] or
    * [0,65535] to the original [0,7] or [0,31], or whatever range the
    * colors were originally in:
    */
   if ((transforms & PNG_TRANSFORM_SHIFT)
       && xpng_get_valid(xpng_ptr, info_ptr, PNG_INFO_sBIT))
   {
      xpng_color_8p sig_bit;

      xpng_get_sBIT(xpng_ptr, info_ptr, &sig_bit);
      xpng_set_shift(xpng_ptr, sig_bit);
   }
#endif

#if defined(PNG_READ_BGR_SUPPORTED)
   /* flip the RGB pixels to BGR (or RGBA to BGRA) */
   if (transforms & PNG_TRANSFORM_BGR)
       xpng_set_bgr(xpng_ptr);
#endif

#if defined(PNG_READ_SWAP_ALPHA_SUPPORTED)
   /* swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
   if (transforms & PNG_TRANSFORM_SWAP_ALPHA)
       xpng_set_swap_alpha(xpng_ptr);
#endif

#if defined(PNG_READ_SWAP_SUPPORTED)
   /* swap bytes of 16 bit files to least significant byte first */
   if (transforms & PNG_TRANSFORM_SWAP_ENDIAN)
       xpng_set_swap(xpng_ptr);
#endif

   /* We don't handle adding filler bytes */

   /* Optional call to gamma correct and add the background to the palette
    * and update info structure.  REQUIRED if you are expecting libpng to
    * update the palette for you (i.e., you selected such a transform above).
    */
   xpng_read_update_info(xpng_ptr, info_ptr);

   /* -------------- image transformations end here ------------------- */

#ifdef PNG_FREE_ME_SUPPORTED
   xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_ROWS, 0);
#endif
   if(info_ptr->row_pointers == NULL)
   {
      info_ptr->row_pointers = (xpng_bytepp)xpng_malloc(xpng_ptr,
         info_ptr->height * sizeof(xpng_bytep));
#ifdef PNG_FREE_ME_SUPPORTED
      info_ptr->free_me |= PNG_FREE_ROWS;
#endif
      for (row = 0; row < (int)info_ptr->height; row++)
      {
         info_ptr->row_pointers[row] = (xpng_bytep)xpng_malloc(xpng_ptr,
            xpng_get_rowbytes(xpng_ptr, info_ptr));
      }
   }

   xpng_read_image(xpng_ptr, info_ptr->row_pointers);
   info_ptr->valid |= PNG_INFO_IDAT;

   /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
   xpng_read_end(xpng_ptr, info_ptr);

   if(transforms == 0 || params == NULL)
      /* quiet compiler warnings */ return;

}
#endif
