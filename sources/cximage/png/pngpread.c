// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngpread.c - read a png file in push mode
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 */

#define PNG_INTERNAL
#include "png.h"

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED

/* push model modes */
#define PNG_READ_SIG_MODE   0
#define PNG_READ_CHUNK_MODE 1
#define PNG_READ_IDAT_MODE  2
#define PNG_SKIP_MODE       3
#define PNG_READ_tEXt_MODE  4
#define PNG_READ_zTXt_MODE  5
#define PNG_READ_DONE_MODE  6
#define PNG_READ_iTXt_MODE  7
#define PNG_ERROR_MODE      8

void PNGAPI
xpng_process_data(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_bytep buffer, xpng_size_t buffer_size)
{
   xpng_push_restore_buffer(xpng_ptr, buffer, buffer_size);

   while (xpng_ptr->buffer_size)
   {
      xpng_process_some_data(xpng_ptr, info_ptr);
   }
}

/* What we do with the incoming data depends on what we were previously
 * doing before we ran out of data...
 */
void /* PRIVATE */
xpng_process_some_data(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   switch (xpng_ptr->process_mode)
   {
      case PNG_READ_SIG_MODE:
      {
         xpng_push_read_sig(xpng_ptr, info_ptr);
         break;
      }
      case PNG_READ_CHUNK_MODE:
      {
         xpng_push_read_chunk(xpng_ptr, info_ptr);
         break;
      }
      case PNG_READ_IDAT_MODE:
      {
         xpng_push_read_IDAT(xpng_ptr);
         break;
      }
#if defined(PNG_READ_tEXt_SUPPORTED)
      case PNG_READ_tEXt_MODE:
      {
         xpng_push_read_tEXt(xpng_ptr, info_ptr);
         break;
      }
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
      case PNG_READ_zTXt_MODE:
      {
         xpng_push_read_zTXt(xpng_ptr, info_ptr);
         break;
      }
#endif
#if defined(PNG_READ_iTXt_SUPPORTED)
      case PNG_READ_iTXt_MODE:
      {
         xpng_push_read_iTXt(xpng_ptr, info_ptr);
         break;
      }
#endif
      case PNG_SKIP_MODE:
      {
         xpng_push_crc_finish(xpng_ptr);
         break;
      }
      default:
      {
         xpng_ptr->buffer_size = 0;
         break;
      }
   }
}

/* Read any remaining signature bytes from the stream and compare them with
 * the correct PNG signature.  It is possible that this routine is called
 * with bytes already read from the signature, either because they have been
 * checked by the calling application, or because of multiple calls to this
 * routine.
 */
void /* PRIVATE */
xpng_push_read_sig(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   xpng_size_t num_checked = xpng_ptr->sig_bytes,
             num_to_check = 8 - num_checked;

   if (xpng_ptr->buffer_size < num_to_check)
   {
      num_to_check = xpng_ptr->buffer_size;
   }

   xpng_push_fill_buffer(xpng_ptr, &(info_ptr->signature[num_checked]),
      num_to_check);
   xpng_ptr->sig_bytes = (xpng_byte)(xpng_ptr->sig_bytes+num_to_check);

   if (xpng_sig_cmp(info_ptr->signature, num_checked, num_to_check))
   {
      if (num_checked < 4 &&
          xpng_sig_cmp(info_ptr->signature, num_checked, num_to_check - 4))
         xpng_error(xpng_ptr, "Not a PNG file");
      else
         xpng_error(xpng_ptr, "PNG file corrupted by ASCII conversion");
   }
   else
   {
      if (xpng_ptr->sig_bytes >= 8)
      {
         xpng_ptr->process_mode = PNG_READ_CHUNK_MODE;
      }
   }
}

void /* PRIVATE */
xpng_push_read_chunk(xpng_structp xpng_ptr, xpng_infop info_ptr)
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
#if defined(PNG_READ_sRGB_SUPPORTED)
      PNG_sRGB;
#endif
#if defined(PNG_READ_sPLT_SUPPORTED)
      PNG_sPLT;
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
#endif /* PNG_USE_LOCAL_ARRAYS */
   /* First we make sure we have enough data for the 4 byte chunk name
    * and the 4 byte chunk length before proceeding with decoding the
    * chunk data.  To fully decode each of these chunks, we also make
    * sure we have enough data in the buffer for the 4 byte CRC at the
    * end of every chunk (except IDAT, which is handled separately).
    */
   if (!(xpng_ptr->mode & PNG_HAVE_CHUNK_HEADER))
   {
      xpng_byte chunk_length[4];

      if (xpng_ptr->buffer_size < 8)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }

      xpng_push_fill_buffer(xpng_ptr, chunk_length, 4);
      xpng_ptr->push_length = xpng_get_uint_32(chunk_length);
      xpng_reset_crc(xpng_ptr);
      xpng_crc_read(xpng_ptr, xpng_ptr->chunk_name, 4);
      xpng_ptr->mode |= PNG_HAVE_CHUNK_HEADER;
   }

   if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IHDR, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_IHDR(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_PLTE, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_PLTE(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
   else if (!xpng_memcmp(xpng_ptr->chunk_name, (xpng_bytep)xpng_IDAT, 4))
   {
      /* If we reach an IDAT chunk, this means we have read all of the
       * header chunks, and we can start reading the image (or if this
       * is called after the image has been read - we have an error).
       */
     if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
       xpng_error(xpng_ptr, "Missing IHDR before IDAT");
     else if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
         !(xpng_ptr->mode & PNG_HAVE_PLTE))
       xpng_error(xpng_ptr, "Missing PLTE before IDAT");

      if (xpng_ptr->mode & PNG_HAVE_IDAT)
      {
         if (xpng_ptr->push_length == 0)
            return;

         if (xpng_ptr->mode & PNG_AFTER_IDAT)
            xpng_error(xpng_ptr, "Too many IDAT's found");
      }

      xpng_ptr->idat_size = xpng_ptr->push_length;
      xpng_ptr->mode |= PNG_HAVE_IDAT;
      xpng_ptr->process_mode = PNG_READ_IDAT_MODE;
      xpng_push_have_info(xpng_ptr, info_ptr);
      xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->irowbytes;
      xpng_ptr->zstream.next_out = xpng_ptr->row_buf;
      return;
   }
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_IEND, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_IEND(xpng_ptr, info_ptr, xpng_ptr->push_length);

      xpng_ptr->process_mode = PNG_READ_DONE_MODE;
      xpng_push_have_end(xpng_ptr, info_ptr);
   }
#if defined(PNG_READ_gAMA_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_gAMA, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_gAMA(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_sBIT_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sBIT, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_sBIT(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_cHRM_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_cHRM, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_cHRM(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_sRGB_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sRGB, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_sRGB(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_iCCP_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_iCCP, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_iCCP(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_sPLT_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sPLT, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_sPLT(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_tRNS_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tRNS, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_tRNS(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_bKGD_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_bKGD, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_bKGD(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_hIST_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_hIST, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_hIST(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_pHYs_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_pHYs, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_pHYs(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_oFFs_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_oFFs, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_oFFs(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_pCAL_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_pCAL, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_pCAL(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_sCAL_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_sCAL, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_sCAL(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_tIME_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tIME, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_handle_tIME(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_tEXt_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_tEXt, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_push_handle_tEXt(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_zTXt, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_push_handle_zTXt(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
#if defined(PNG_READ_iTXt_SUPPORTED)
   else if (!xpng_memcmp(xpng_ptr->chunk_name, xpng_iTXt, 4))
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_push_handle_iTXt(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }
#endif
   else
   {
      if (xpng_ptr->push_length + 4 > xpng_ptr->buffer_size)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }
      xpng_push_handle_unknown(xpng_ptr, info_ptr, xpng_ptr->push_length);
   }

   xpng_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;
}

void /* PRIVATE */
xpng_push_crc_skip(xpng_structp xpng_ptr, xpng_uint_32 skip)
{
   xpng_ptr->process_mode = PNG_SKIP_MODE;
   xpng_ptr->skip_length = skip;
}

void /* PRIVATE */
xpng_push_crc_finish(xpng_structp xpng_ptr)
{
   if (xpng_ptr->skip_length && xpng_ptr->save_buffer_size)
   {
      xpng_size_t save_size;

      if (xpng_ptr->skip_length < (xpng_uint_32)xpng_ptr->save_buffer_size)
         save_size = (xpng_size_t)xpng_ptr->skip_length;
      else
         save_size = xpng_ptr->save_buffer_size;

      xpng_calculate_crc(xpng_ptr, xpng_ptr->save_buffer_ptr, save_size);

      xpng_ptr->skip_length -= save_size;
      xpng_ptr->buffer_size -= save_size;
      xpng_ptr->save_buffer_size -= save_size;
      xpng_ptr->save_buffer_ptr += save_size;
   }
   if (xpng_ptr->skip_length && xpng_ptr->current_buffer_size)
   {
      xpng_size_t save_size;

      if (xpng_ptr->skip_length < (xpng_uint_32)xpng_ptr->current_buffer_size)
         save_size = (xpng_size_t)xpng_ptr->skip_length;
      else
         save_size = xpng_ptr->current_buffer_size;

      xpng_calculate_crc(xpng_ptr, xpng_ptr->current_buffer_ptr, save_size);

      xpng_ptr->skip_length -= save_size;
      xpng_ptr->buffer_size -= save_size;
      xpng_ptr->current_buffer_size -= save_size;
      xpng_ptr->current_buffer_ptr += save_size;
   }
   if (!xpng_ptr->skip_length)
   {
      if (xpng_ptr->buffer_size < 4)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }

      xpng_crc_finish(xpng_ptr, 0);
      xpng_ptr->process_mode = PNG_READ_CHUNK_MODE;
   }
}

void PNGAPI
xpng_push_fill_buffer(xpng_structp xpng_ptr, xpng_bytep buffer, xpng_size_t length)
{
   xpng_bytep ptr;

   ptr = buffer;
   if (xpng_ptr->save_buffer_size)
   {
      xpng_size_t save_size;

      if (length < xpng_ptr->save_buffer_size)
         save_size = length;
      else
         save_size = xpng_ptr->save_buffer_size;

      xpng_memcpy(ptr, xpng_ptr->save_buffer_ptr, save_size);
      length -= save_size;
      ptr += save_size;
      xpng_ptr->buffer_size -= save_size;
      xpng_ptr->save_buffer_size -= save_size;
      xpng_ptr->save_buffer_ptr += save_size;
   }
   if (length && xpng_ptr->current_buffer_size)
   {
      xpng_size_t save_size;

      if (length < xpng_ptr->current_buffer_size)
         save_size = length;
      else
         save_size = xpng_ptr->current_buffer_size;

      xpng_memcpy(ptr, xpng_ptr->current_buffer_ptr, save_size);
      xpng_ptr->buffer_size -= save_size;
      xpng_ptr->current_buffer_size -= save_size;
      xpng_ptr->current_buffer_ptr += save_size;
   }
}

void /* PRIVATE */
xpng_push_save_buffer(xpng_structp xpng_ptr)
{
   if (xpng_ptr->save_buffer_size)
   {
      if (xpng_ptr->save_buffer_ptr != xpng_ptr->save_buffer)
      {
         xpng_size_t i,istop;
         xpng_bytep sp;
         xpng_bytep dp;

         istop = xpng_ptr->save_buffer_size;
         for (i = 0, sp = xpng_ptr->save_buffer_ptr, dp = xpng_ptr->save_buffer;
            i < istop; i++, sp++, dp++)
         {
            *dp = *sp;
         }
      }
   }
   if (xpng_ptr->save_buffer_size + xpng_ptr->current_buffer_size >
      xpng_ptr->save_buffer_max)
   {
      xpng_size_t new_max;
      xpng_bytep old_buffer;

      new_max = xpng_ptr->save_buffer_size + xpng_ptr->current_buffer_size + 256;
      old_buffer = xpng_ptr->save_buffer;
      xpng_ptr->save_buffer = (xpng_bytep)xpng_malloc(xpng_ptr,
         (xpng_uint_32)new_max);
      xpng_memcpy(xpng_ptr->save_buffer, old_buffer, xpng_ptr->save_buffer_size);
      xpng_free(xpng_ptr, old_buffer);
      xpng_ptr->save_buffer_max = new_max;
   }
   if (xpng_ptr->current_buffer_size)
   {
      xpng_memcpy(xpng_ptr->save_buffer + xpng_ptr->save_buffer_size,
         xpng_ptr->current_buffer_ptr, xpng_ptr->current_buffer_size);
      xpng_ptr->save_buffer_size += xpng_ptr->current_buffer_size;
      xpng_ptr->current_buffer_size = 0;
   }
   xpng_ptr->save_buffer_ptr = xpng_ptr->save_buffer;
   xpng_ptr->buffer_size = 0;
}

void /* PRIVATE */
xpng_push_restore_buffer(xpng_structp xpng_ptr, xpng_bytep buffer,
   xpng_size_t buffer_length)
{
   xpng_ptr->current_buffer = buffer;
   xpng_ptr->current_buffer_size = buffer_length;
   xpng_ptr->buffer_size = buffer_length + xpng_ptr->save_buffer_size;
   xpng_ptr->current_buffer_ptr = xpng_ptr->current_buffer;
}

void /* PRIVATE */
xpng_push_read_IDAT(xpng_structp xpng_ptr)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   PNG_IDAT;
#endif
   if (!(xpng_ptr->mode & PNG_HAVE_CHUNK_HEADER))
   {
      xpng_byte chunk_length[4];

      if (xpng_ptr->buffer_size < 8)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }

      xpng_push_fill_buffer(xpng_ptr, chunk_length, 4);
      xpng_ptr->push_length = xpng_get_uint_32(chunk_length);

      xpng_reset_crc(xpng_ptr);
      xpng_crc_read(xpng_ptr, xpng_ptr->chunk_name, 4);
      xpng_ptr->mode |= PNG_HAVE_CHUNK_HEADER;

      if (xpng_memcmp(xpng_ptr->chunk_name, (xpng_bytep)xpng_IDAT, 4))
      {
         xpng_ptr->process_mode = PNG_READ_CHUNK_MODE;
         if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_FINISHED))
            xpng_error(xpng_ptr, "Not enough compressed data");
         return;
      }

      xpng_ptr->idat_size = xpng_ptr->push_length;
   }
   if (xpng_ptr->idat_size && xpng_ptr->save_buffer_size)
   {
      xpng_size_t save_size;

      if (xpng_ptr->idat_size < (xpng_uint_32)xpng_ptr->save_buffer_size)
      {
         save_size = (xpng_size_t)xpng_ptr->idat_size;
         /* check for overflow */
         if((xpng_uint_32)save_size != xpng_ptr->idat_size)
            xpng_error(xpng_ptr, "save_size overflowed in pngpread");
      }
      else
         save_size = xpng_ptr->save_buffer_size;

      xpng_calculate_crc(xpng_ptr, xpng_ptr->save_buffer_ptr, save_size);
      if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_FINISHED))
         xpng_process_IDAT_data(xpng_ptr, xpng_ptr->save_buffer_ptr, save_size);
      xpng_ptr->idat_size -= save_size;
      xpng_ptr->buffer_size -= save_size;
      xpng_ptr->save_buffer_size -= save_size;
      xpng_ptr->save_buffer_ptr += save_size;
   }
   if (xpng_ptr->idat_size && xpng_ptr->current_buffer_size)
   {
      xpng_size_t save_size;

      if (xpng_ptr->idat_size < (xpng_uint_32)xpng_ptr->current_buffer_size)
      {
         save_size = (xpng_size_t)xpng_ptr->idat_size;
         /* check for overflow */
         if((xpng_uint_32)save_size != xpng_ptr->idat_size)
            xpng_error(xpng_ptr, "save_size overflowed in pngpread");
      }
      else
         save_size = xpng_ptr->current_buffer_size;

      xpng_calculate_crc(xpng_ptr, xpng_ptr->current_buffer_ptr, save_size);
      if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_FINISHED))
        xpng_process_IDAT_data(xpng_ptr, xpng_ptr->current_buffer_ptr, save_size);

      xpng_ptr->idat_size -= save_size;
      xpng_ptr->buffer_size -= save_size;
      xpng_ptr->current_buffer_size -= save_size;
      xpng_ptr->current_buffer_ptr += save_size;
   }
   if (!xpng_ptr->idat_size)
   {
      if (xpng_ptr->buffer_size < 4)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }

      xpng_crc_finish(xpng_ptr, 0);
      xpng_ptr->mode &= ~PNG_HAVE_CHUNK_HEADER;
      xpng_ptr->mode |= PNG_AFTER_IDAT;
   }
}

void /* PRIVATE */
xpng_process_IDAT_data(xpng_structp xpng_ptr, xpng_bytep buffer,
   xpng_size_t buffer_length)
{
   int ret;

   if ((xpng_ptr->flags & PNG_FLAG_ZLIB_FINISHED) && buffer_length)
      xpng_error(xpng_ptr, "Extra compression data");

   xpng_ptr->zstream.next_in = buffer;
   xpng_ptr->zstream.avail_in = (uInt)buffer_length;
   for(;;)
   {
      ret = inflate(&xpng_ptr->zstream, Z_PARTIAL_FLUSH);
      if (ret != Z_OK)
      {
         if (ret == Z_STREAM_END)
         {
            if (xpng_ptr->zstream.avail_in)
               xpng_error(xpng_ptr, "Extra compressed data");
            if (!(xpng_ptr->zstream.avail_out))
            {
               xpng_push_process_row(xpng_ptr);
            }

            xpng_ptr->mode |= PNG_AFTER_IDAT;
            xpng_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
            break;
         }
         else if (ret == Z_BUF_ERROR)
            break;
         else
            xpng_error(xpng_ptr, "Decompression Error");
      }
      if (!(xpng_ptr->zstream.avail_out))
      {
         if ((
#if defined(PNG_READ_INTERLACING_SUPPORTED)
             xpng_ptr->interlaced && xpng_ptr->pass > 6) ||
             (!xpng_ptr->interlaced &&
#endif
             xpng_ptr->row_number == xpng_ptr->num_rows-1))
         {
           if (xpng_ptr->zstream.avail_in)
             xpng_warning(xpng_ptr, "Too much data in IDAT chunks");
           xpng_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
           break;
         }
         xpng_push_process_row(xpng_ptr);
         xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->irowbytes;
         xpng_ptr->zstream.next_out = xpng_ptr->row_buf;
      }
      else
         break;
   }
}

void /* PRIVATE */
xpng_push_process_row(xpng_structp xpng_ptr)
{
   xpng_ptr->row_info.color_type = xpng_ptr->color_type;
   xpng_ptr->row_info.width = xpng_ptr->iwidth;
   xpng_ptr->row_info.channels = xpng_ptr->channels;
   xpng_ptr->row_info.bit_depth = xpng_ptr->bit_depth;
   xpng_ptr->row_info.pixel_depth = xpng_ptr->pixel_depth;

   xpng_ptr->row_info.rowbytes = ((xpng_ptr->row_info.width *
      (xpng_uint_32)xpng_ptr->row_info.pixel_depth + 7) >> 3);

   xpng_read_filter_row(xpng_ptr, &(xpng_ptr->row_info),
      xpng_ptr->row_buf + 1, xpng_ptr->prev_row + 1,
      (int)(xpng_ptr->row_buf[0]));

   xpng_memcpy_check(xpng_ptr, xpng_ptr->prev_row, xpng_ptr->row_buf,
      xpng_ptr->rowbytes + 1);

   if (xpng_ptr->transformations)
      xpng_do_read_transformations(xpng_ptr);

#if defined(PNG_READ_INTERLACING_SUPPORTED)
   /* blow up interlaced rows to full size */
   if (xpng_ptr->interlaced && (xpng_ptr->transformations & PNG_INTERLACE))
   {
      if (xpng_ptr->pass < 6)
/*       old interface (pre-1.0.9):
         xpng_do_read_interlace(&(xpng_ptr->row_info),
            xpng_ptr->row_buf + 1, xpng_ptr->pass, xpng_ptr->transformations);
 */
         xpng_do_read_interlace(xpng_ptr);

    switch (xpng_ptr->pass)
    {
         case 0:
         {
            int i;
            for (i = 0; i < 8 && xpng_ptr->pass == 0; i++)
            {
               xpng_push_have_row(xpng_ptr, xpng_ptr->row_buf + 1);
               xpng_read_push_finish_row(xpng_ptr); /* updates xpng_ptr->pass */
            }
            if (xpng_ptr->pass == 2) /* pass 1 might be empty */
            {
               for (i = 0; i < 4 && xpng_ptr->pass == 2; i++)
               {
                  xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
                  xpng_read_push_finish_row(xpng_ptr);
               }
            }
            if (xpng_ptr->pass == 4 && xpng_ptr->height <= 4)
            {
               for (i = 0; i < 2 && xpng_ptr->pass == 4; i++)
               {
                  xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
                  xpng_read_push_finish_row(xpng_ptr);
               }
            }
            if (xpng_ptr->pass == 6 && xpng_ptr->height <= 4)
            {
                xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
                xpng_read_push_finish_row(xpng_ptr);
            }
            break;
         }
         case 1:
         {
            int i;
            for (i = 0; i < 8 && xpng_ptr->pass == 1; i++)
            {
               xpng_push_have_row(xpng_ptr, xpng_ptr->row_buf + 1);
               xpng_read_push_finish_row(xpng_ptr);
            }
            if (xpng_ptr->pass == 2) /* skip top 4 generated rows */
            {
               for (i = 0; i < 4 && xpng_ptr->pass == 2; i++)
               {
                  xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
                  xpng_read_push_finish_row(xpng_ptr);
               }
            }
            break;
         }
         case 2:
         {
            int i;
            for (i = 0; i < 4 && xpng_ptr->pass == 2; i++)
            {
               xpng_push_have_row(xpng_ptr, xpng_ptr->row_buf + 1);
               xpng_read_push_finish_row(xpng_ptr);
            }
            for (i = 0; i < 4 && xpng_ptr->pass == 2; i++)
            {
               xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
               xpng_read_push_finish_row(xpng_ptr);
            }
            if (xpng_ptr->pass == 4) /* pass 3 might be empty */
            {
               for (i = 0; i < 2 && xpng_ptr->pass == 4; i++)
               {
                  xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
                  xpng_read_push_finish_row(xpng_ptr);
               }
            }
            break;
         }
         case 3:
         {
            int i;
            for (i = 0; i < 4 && xpng_ptr->pass == 3; i++)
            {
               xpng_push_have_row(xpng_ptr, xpng_ptr->row_buf + 1);
               xpng_read_push_finish_row(xpng_ptr);
            }
            if (xpng_ptr->pass == 4) /* skip top two generated rows */
            {
               for (i = 0; i < 2 && xpng_ptr->pass == 4; i++)
               {
                  xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
                  xpng_read_push_finish_row(xpng_ptr);
               }
            }
            break;
         }
         case 4:
         {
            int i;
            for (i = 0; i < 2 && xpng_ptr->pass == 4; i++)
            {
               xpng_push_have_row(xpng_ptr, xpng_ptr->row_buf + 1);
               xpng_read_push_finish_row(xpng_ptr);
            }
            for (i = 0; i < 2 && xpng_ptr->pass == 4; i++)
            {
               xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
               xpng_read_push_finish_row(xpng_ptr);
            }
            if (xpng_ptr->pass == 6) /* pass 5 might be empty */
            {
               xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
               xpng_read_push_finish_row(xpng_ptr);
            }
            break;
         }
         case 5:
         {
            int i;
            for (i = 0; i < 2 && xpng_ptr->pass == 5; i++)
            {
               xpng_push_have_row(xpng_ptr, xpng_ptr->row_buf + 1);
               xpng_read_push_finish_row(xpng_ptr);
            }
            if (xpng_ptr->pass == 6) /* skip top generated row */
            {
               xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
               xpng_read_push_finish_row(xpng_ptr);
            }
            break;
         }
         case 6:
         {
            xpng_push_have_row(xpng_ptr, xpng_ptr->row_buf + 1);
            xpng_read_push_finish_row(xpng_ptr);
            if (xpng_ptr->pass != 6)
               break;
            xpng_push_have_row(xpng_ptr, xpng_bytep_NULL);
            xpng_read_push_finish_row(xpng_ptr);
         }
      }
   }
   else
#endif
   {
      xpng_push_have_row(xpng_ptr, xpng_ptr->row_buf + 1);
      xpng_read_push_finish_row(xpng_ptr);
   }
}

void /* PRIVATE */
xpng_read_push_finish_row(xpng_structp xpng_ptr)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   /* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

   /* start of interlace block */
   const int FARDATA xpng_pass_start[] = {0, 4, 0, 2, 0, 1, 0};

   /* offset to next interlace block */
   const int FARDATA xpng_pass_inc[] = {8, 8, 4, 4, 2, 2, 1};

   /* start of interlace block in the y direction */
   const int FARDATA xpng_pass_ystart[] = {0, 0, 4, 0, 2, 0, 1};

   /* offset to next interlace block in the y direction */
   const int FARDATA xpng_pass_yinc[] = {8, 8, 8, 4, 4, 2, 2};

   /* Width of interlace block.  This is not currently used - if you need
    * it, uncomment it here and in png.h
   const int FARDATA xpng_pass_width[] = {8, 4, 4, 2, 2, 1, 1};
   */

   /* Height of interlace block.  This is not currently used - if you need
    * it, uncomment it here and in png.h
   const int FARDATA xpng_pass_height[] = {8, 8, 4, 4, 2, 2, 1};
   */
#endif

   xpng_ptr->row_number++;
   if (xpng_ptr->row_number < xpng_ptr->num_rows)
      return;

   if (xpng_ptr->interlaced)
   {
      xpng_ptr->row_number = 0;
      xpng_memset_check(xpng_ptr, xpng_ptr->prev_row, 0,
         xpng_ptr->rowbytes + 1);
      do
      {
         xpng_ptr->pass++;
         if ((xpng_ptr->pass == 1 && xpng_ptr->width < 5) ||
             (xpng_ptr->pass == 3 && xpng_ptr->width < 3) ||
             (xpng_ptr->pass == 5 && xpng_ptr->width < 2))
           xpng_ptr->pass++;

         if (xpng_ptr->pass > 7)
            xpng_ptr->pass--;
         if (xpng_ptr->pass >= 7)
            break;

         xpng_ptr->iwidth = (xpng_ptr->width +
            xpng_pass_inc[xpng_ptr->pass] - 1 -
            xpng_pass_start[xpng_ptr->pass]) /
            xpng_pass_inc[xpng_ptr->pass];

         xpng_ptr->irowbytes = ((xpng_ptr->iwidth *
            xpng_ptr->pixel_depth + 7) >> 3) + 1;

         if (xpng_ptr->transformations & PNG_INTERLACE)
            break;

         xpng_ptr->num_rows = (xpng_ptr->height +
            xpng_pass_yinc[xpng_ptr->pass] - 1 -
            xpng_pass_ystart[xpng_ptr->pass]) /
            xpng_pass_yinc[xpng_ptr->pass];

      } while (xpng_ptr->iwidth == 0 || xpng_ptr->num_rows == 0);
   }
}

#if defined(PNG_READ_tEXt_SUPPORTED)
void /* PRIVATE */
xpng_push_handle_tEXt(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32
   length)
{
   if (!(xpng_ptr->mode & PNG_HAVE_IHDR) || (xpng_ptr->mode & PNG_HAVE_IEND))
      {
         xpng_error(xpng_ptr, "Out of place tEXt");
         /* to quiet some compiler warnings */
         if(info_ptr == NULL) return;
      }

#ifdef PNG_MAX_MALLOC_64K
   xpng_ptr->skip_length = 0;  /* This may not be necessary */

   if (length > (xpng_uint_32)65535L) /* Can't hold entire string in memory */
   {
      xpng_warning(xpng_ptr, "tEXt chunk too large to fit in memory");
      xpng_ptr->skip_length = length - (xpng_uint_32)65535L;
      length = (xpng_uint_32)65535L;
   }
#endif

   xpng_ptr->current_text = (xpng_charp)xpng_malloc(xpng_ptr,
         (xpng_uint_32)(length+1));
   xpng_ptr->current_text[length] = '\0';
   xpng_ptr->current_text_ptr = xpng_ptr->current_text;
   xpng_ptr->current_text_size = (xpng_size_t)length;
   xpng_ptr->current_text_left = (xpng_size_t)length;
   xpng_ptr->process_mode = PNG_READ_tEXt_MODE;
}

void /* PRIVATE */
xpng_push_read_tEXt(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   if (xpng_ptr->buffer_size && xpng_ptr->current_text_left)
   {
      xpng_size_t text_size;

      if (xpng_ptr->buffer_size < xpng_ptr->current_text_left)
         text_size = xpng_ptr->buffer_size;
      else
         text_size = xpng_ptr->current_text_left;
      xpng_crc_read(xpng_ptr, (xpng_bytep)xpng_ptr->current_text_ptr, text_size);
      xpng_ptr->current_text_left -= text_size;
      xpng_ptr->current_text_ptr += text_size;
   }
   if (!(xpng_ptr->current_text_left))
   {
      xpng_textp text_ptr;
      xpng_charp text;
      xpng_charp key;
      int ret;

      if (xpng_ptr->buffer_size < 4)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }

      xpng_push_crc_finish(xpng_ptr);

#if defined(PNG_MAX_MALLOC_64K)
      if (xpng_ptr->skip_length)
         return;
#endif

      key = xpng_ptr->current_text;

      for (text = key; *text; text++)
         /* empty loop */ ;

      if (text != key + xpng_ptr->current_text_size)
         text++;

      text_ptr = (xpng_textp)xpng_malloc(xpng_ptr, (xpng_uint_32)sizeof(xpng_text));
      text_ptr->compression = PNG_TEXT_COMPRESSION_NONE;
      text_ptr->key = key;
#ifdef PNG_iTXt_SUPPORTED
      text_ptr->lang = NULL;
      text_ptr->lang_key = NULL;
#endif
      text_ptr->text = text;

      ret = xpng_set_text_2(xpng_ptr, info_ptr, text_ptr, 1);

      xpng_free(xpng_ptr, key);
      xpng_free(xpng_ptr, text_ptr);
      xpng_ptr->current_text = NULL;

      if (ret)
        xpng_warning(xpng_ptr, "Insufficient memory to store text chunk.");
   }
}
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
void /* PRIVATE */
xpng_push_handle_zTXt(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32
   length)
{
   if (!(xpng_ptr->mode & PNG_HAVE_IHDR) || (xpng_ptr->mode & PNG_HAVE_IEND))
      {
         xpng_error(xpng_ptr, "Out of place zTXt");
         /* to quiet some compiler warnings */
         if(info_ptr == NULL) return;
      }

#ifdef PNG_MAX_MALLOC_64K
   /* We can't handle zTXt chunks > 64K, since we don't have enough space
    * to be able to store the uncompressed data.  Actually, the threshold
    * is probably around 32K, but it isn't as definite as 64K is.
    */
   if (length > (xpng_uint_32)65535L)
   {
      xpng_warning(xpng_ptr, "zTXt chunk too large to fit in memory");
      xpng_push_crc_skip(xpng_ptr, length);
      return;
   }
#endif

   xpng_ptr->current_text = (xpng_charp)xpng_malloc(xpng_ptr,
       (xpng_uint_32)(length+1));
   xpng_ptr->current_text[length] = '\0';
   xpng_ptr->current_text_ptr = xpng_ptr->current_text;
   xpng_ptr->current_text_size = (xpng_size_t)length;
   xpng_ptr->current_text_left = (xpng_size_t)length;
   xpng_ptr->process_mode = PNG_READ_zTXt_MODE;
}

void /* PRIVATE */
xpng_push_read_zTXt(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   if (xpng_ptr->buffer_size && xpng_ptr->current_text_left)
   {
      xpng_size_t text_size;

      if (xpng_ptr->buffer_size < (xpng_uint_32)xpng_ptr->current_text_left)
         text_size = xpng_ptr->buffer_size;
      else
         text_size = xpng_ptr->current_text_left;
      xpng_crc_read(xpng_ptr, (xpng_bytep)xpng_ptr->current_text_ptr, text_size);
      xpng_ptr->current_text_left -= text_size;
      xpng_ptr->current_text_ptr += text_size;
   }
   if (!(xpng_ptr->current_text_left))
   {
      xpng_textp text_ptr;
      xpng_charp text;
      xpng_charp key;
      int ret;
      xpng_size_t text_size, key_size;

      if (xpng_ptr->buffer_size < 4)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }

      xpng_push_crc_finish(xpng_ptr);

      key = xpng_ptr->current_text;

      for (text = key; *text; text++)
         /* empty loop */ ;

      /* zTXt can't have zero text */
      if (text == key + xpng_ptr->current_text_size)
      {
         xpng_ptr->current_text = NULL;
         xpng_free(xpng_ptr, key);
         return;
      }

      text++;

      if (*text != PNG_TEXT_COMPRESSION_zTXt) /* check compression byte */
      {
         xpng_ptr->current_text = NULL;
         xpng_free(xpng_ptr, key);
         return;
      }

      text++;

      xpng_ptr->zstream.next_in = (xpng_bytep )text;
      xpng_ptr->zstream.avail_in = (uInt)(xpng_ptr->current_text_size -
         (text - key));
      xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
      xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;

      key_size = text - key;
      text_size = 0;
      text = NULL;
      ret = Z_STREAM_END;

      while (xpng_ptr->zstream.avail_in)
      {
         ret = inflate(&xpng_ptr->zstream, Z_PARTIAL_FLUSH);
         if (ret != Z_OK && ret != Z_STREAM_END)
         {
            inflateReset(&xpng_ptr->zstream);
            xpng_ptr->zstream.avail_in = 0;
            xpng_ptr->current_text = NULL;
            xpng_free(xpng_ptr, key);
            xpng_free(xpng_ptr, text);
            return;
         }
         if (!(xpng_ptr->zstream.avail_out) || ret == Z_STREAM_END)
         {
            if (text == NULL)
            {
               text = (xpng_charp)xpng_malloc(xpng_ptr,
                  (xpng_uint_32)(xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out
                     + key_size + 1));
               xpng_memcpy(text + key_size, xpng_ptr->zbuf,
                  xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out);
               xpng_memcpy(text, key, key_size);
               text_size = key_size + xpng_ptr->zbuf_size -
                  xpng_ptr->zstream.avail_out;
               *(text + text_size) = '\0';
            }
            else
            {
               xpng_charp tmp;

               tmp = text;
               text = (xpng_charp)xpng_malloc(xpng_ptr, text_size +
                  (xpng_uint_32)(xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out
                   + 1));
               xpng_memcpy(text, tmp, text_size);
               xpng_free(xpng_ptr, tmp);
               xpng_memcpy(text + text_size, xpng_ptr->zbuf,
                  xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out);
               text_size += xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out;
               *(text + text_size) = '\0';
            }
            if (ret != Z_STREAM_END)
            {
               xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
               xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
            }
         }
         else
         {
            break;
         }

         if (ret == Z_STREAM_END)
            break;
      }

      inflateReset(&xpng_ptr->zstream);
      xpng_ptr->zstream.avail_in = 0;

      if (ret != Z_STREAM_END)
      {
         xpng_ptr->current_text = NULL;
         xpng_free(xpng_ptr, key);
         xpng_free(xpng_ptr, text);
         return;
      }

      xpng_ptr->current_text = NULL;
      xpng_free(xpng_ptr, key);
      key = text;
      text += key_size;

      text_ptr = (xpng_textp)xpng_malloc(xpng_ptr, (xpng_uint_32)sizeof(xpng_text));
      text_ptr->compression = PNG_TEXT_COMPRESSION_zTXt;
      text_ptr->key = key;
#ifdef PNG_iTXt_SUPPORTED
      text_ptr->lang = NULL;
      text_ptr->lang_key = NULL;
#endif
      text_ptr->text = text;

      ret = xpng_set_text_2(xpng_ptr, info_ptr, text_ptr, 1);

      xpng_free(xpng_ptr, key);
      xpng_free(xpng_ptr, text_ptr);

      if (ret)
        xpng_warning(xpng_ptr, "Insufficient memory to store text chunk.");
   }
}
#endif

#if defined(PNG_READ_iTXt_SUPPORTED)
void /* PRIVATE */
xpng_push_handle_iTXt(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32
   length)
{
   if (!(xpng_ptr->mode & PNG_HAVE_IHDR) || (xpng_ptr->mode & PNG_HAVE_IEND))
      {
         xpng_error(xpng_ptr, "Out of place iTXt");
         /* to quiet some compiler warnings */
         if(info_ptr == NULL) return;
      }

#ifdef PNG_MAX_MALLOC_64K
   xpng_ptr->skip_length = 0;  /* This may not be necessary */

   if (length > (xpng_uint_32)65535L) /* Can't hold entire string in memory */
   {
      xpng_warning(xpng_ptr, "iTXt chunk too large to fit in memory");
      xpng_ptr->skip_length = length - (xpng_uint_32)65535L;
      length = (xpng_uint_32)65535L;
   }
#endif

   xpng_ptr->current_text = (xpng_charp)xpng_malloc(xpng_ptr,
         (xpng_uint_32)(length+1));
   xpng_ptr->current_text[length] = '\0';
   xpng_ptr->current_text_ptr = xpng_ptr->current_text;
   xpng_ptr->current_text_size = (xpng_size_t)length;
   xpng_ptr->current_text_left = (xpng_size_t)length;
   xpng_ptr->process_mode = PNG_READ_iTXt_MODE;
}

void /* PRIVATE */
xpng_push_read_iTXt(xpng_structp xpng_ptr, xpng_infop info_ptr)
{

   if (xpng_ptr->buffer_size && xpng_ptr->current_text_left)
   {
      xpng_size_t text_size;

      if (xpng_ptr->buffer_size < xpng_ptr->current_text_left)
         text_size = xpng_ptr->buffer_size;
      else
         text_size = xpng_ptr->current_text_left;
      xpng_crc_read(xpng_ptr, (xpng_bytep)xpng_ptr->current_text_ptr, text_size);
      xpng_ptr->current_text_left -= text_size;
      xpng_ptr->current_text_ptr += text_size;
   }
   if (!(xpng_ptr->current_text_left))
   {
      xpng_textp text_ptr;
      xpng_charp key;
      int comp_flag;
      xpng_charp lang;
      xpng_charp lang_key;
      xpng_charp text;
      int ret;

      if (xpng_ptr->buffer_size < 4)
      {
         xpng_push_save_buffer(xpng_ptr);
         return;
      }

      xpng_push_crc_finish(xpng_ptr);

#if defined(PNG_MAX_MALLOC_64K)
      if (xpng_ptr->skip_length)
         return;
#endif

      key = xpng_ptr->current_text;

      for (lang = key; *lang; lang++)
         /* empty loop */ ;

      if (lang != key + xpng_ptr->current_text_size)
         lang++;

      comp_flag = *lang++;
      lang++;     /* skip comp_type, always zero */

      for (lang_key = lang; *lang_key; lang_key++)
         /* empty loop */ ;
      lang_key++;        /* skip NUL separator */

      for (text = lang_key; *text; text++)
         /* empty loop */ ;

      if (text != key + xpng_ptr->current_text_size)
         text++;

      text_ptr = (xpng_textp)xpng_malloc(xpng_ptr, (xpng_uint_32)sizeof(xpng_text));
      text_ptr->compression = comp_flag + 2;
      text_ptr->key = key;
      text_ptr->lang = lang;
      text_ptr->lang_key = lang_key;
      text_ptr->text = text;
      text_ptr->text_length = 0;
      text_ptr->itxt_length = xpng_strlen(text);

      ret = xpng_set_text_2(xpng_ptr, info_ptr, text_ptr, 1);

      xpng_ptr->current_text = NULL;

      xpng_free(xpng_ptr, text_ptr);
      if (ret)
        xpng_warning(xpng_ptr, "Insufficient memory to store iTXt chunk.");
   }
}
#endif

/* This function is called when we haven't found a handler for this
 * chunk.  If there isn't a problem with the chunk itself (ie a bad chunk
 * name or a critical chunk), the chunk is (currently) silently ignored.
 */
void /* PRIVATE */
xpng_push_handle_unknown(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32
   length)
{
   xpng_uint_32 skip=0;
   xpng_check_chunk_name(xpng_ptr, xpng_ptr->chunk_name);

   if (!(xpng_ptr->chunk_name[0] & 0x20))
   {
#if defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED)
      if(xpng_handle_as_unknown(xpng_ptr, xpng_ptr->chunk_name) !=
           HANDLE_CHUNK_ALWAYS
#if defined(PNG_READ_USER_CHUNKS_SUPPORTED)
           && xpng_ptr->read_user_chunk_fn == NULL
#endif
         )
#endif
         xpng_chunk_error(xpng_ptr, "unknown critical chunk");

      /* to quiet compiler warnings about unused info_ptr */
      if (info_ptr == NULL)
         return;
   }

#if defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED)
   if (xpng_ptr->flags & PNG_FLAG_KEEP_UNKNOWN_CHUNKS)
   {
       xpng_unknown_chunk chunk;

#ifdef PNG_MAX_MALLOC_64K
       if (length > (xpng_uint_32)65535L)
       {
           xpng_warning(xpng_ptr, "unknown chunk too large to fit in memory");
           skip = length - (xpng_uint_32)65535L;
           length = (xpng_uint_32)65535L;
       }
#endif

       xpng_strcpy((xpng_charp)chunk.name, (xpng_charp)xpng_ptr->chunk_name);
       chunk.data = (xpng_bytep)xpng_malloc(xpng_ptr, length);
       xpng_crc_read(xpng_ptr, chunk.data, length);
       chunk.size = length;
#if defined(PNG_READ_USER_CHUNKS_SUPPORTED)
       if(xpng_ptr->read_user_chunk_fn != NULL)
       {
          /* callback to user unknown chunk handler */
          if ((*(xpng_ptr->read_user_chunk_fn)) (xpng_ptr, &chunk) <= 0)
          {
             if (!(xpng_ptr->chunk_name[0] & 0x20))
                if(xpng_handle_as_unknown(xpng_ptr, xpng_ptr->chunk_name) !=
                     HANDLE_CHUNK_ALWAYS)
                   xpng_chunk_error(xpng_ptr, "unknown critical chunk");
          }
             xpng_set_unknown_chunks(xpng_ptr, info_ptr, &chunk, 1);
       }
       else
#endif
          xpng_set_unknown_chunks(xpng_ptr, info_ptr, &chunk, 1);
       xpng_free(xpng_ptr, chunk.data);
   }
   else
#endif
      skip=length;
   xpng_push_crc_skip(xpng_ptr, skip);
}

void /* PRIVATE */
xpng_push_have_info(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   if (xpng_ptr->info_fn != NULL)
      (*(xpng_ptr->info_fn))(xpng_ptr, info_ptr);
}

void /* PRIVATE */
xpng_push_have_end(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   if (xpng_ptr->end_fn != NULL)
      (*(xpng_ptr->end_fn))(xpng_ptr, info_ptr);
}

void /* PRIVATE */
xpng_push_have_row(xpng_structp xpng_ptr, xpng_bytep row)
{
   if (xpng_ptr->row_fn != NULL)
      (*(xpng_ptr->row_fn))(xpng_ptr, row, xpng_ptr->row_number,
         (int)xpng_ptr->pass);
}

void PNGAPI
xpng_progressive_combine_row (xpng_structp xpng_ptr,
   xpng_bytep old_row, xpng_bytep new_row)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   const int FARDATA xpng_pass_dsp_mask[7] =
      {0xff, 0x0f, 0xff, 0x33, 0xff, 0x55, 0xff};
#endif
   if (new_row != NULL)    /* new_row must == xpng_ptr->row_buf here. */
      xpng_combine_row(xpng_ptr, old_row, xpng_pass_dsp_mask[xpng_ptr->pass]);
}

void PNGAPI
xpng_set_progressive_read_fn(xpng_structp xpng_ptr, xpng_voidp progressive_ptr,
   xpng_progressive_info_ptr info_fn, xpng_progressive_row_ptr row_fn,
   xpng_progressive_end_ptr end_fn)
{
   xpng_ptr->info_fn = info_fn;
   xpng_ptr->row_fn = row_fn;
   xpng_ptr->end_fn = end_fn;

   xpng_set_read_fn(xpng_ptr, progressive_ptr, xpng_push_fill_buffer);
}

xpng_voidp PNGAPI
xpng_get_progressive_ptr(xpng_structp xpng_ptr)
{
   return xpng_ptr->io_ptr;
}
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */
