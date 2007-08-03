// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngrutil.c - utilities to read a PNG file
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This file contains routines that are only called from within
 * libpng itself during the course of reading an image.
 */

#define PNG_INTERNAL
#include "png.h"

#if defined(_WIN32_WCE)
/* strtod() function is not supported on WindowsCE */
#  ifdef PNG_FLOATING_POINT_SUPPORTED
__inline double strtod(const char *nptr, char **endptr)
{
   double result = 0;
   int len;
   wchar_t *str, *end;

   len = MultiByteToWideChar(CP_ACP, 0, nptr, -1, NULL, 0);
   str = (wchar_t *)malloc(len * sizeof(wchar_t));
   if ( NULL != str )
   {
      MultiByteToWideChar(CP_ACP, 0, nptr, -1, str, len);
      result = wcstod(str, &end);
      len = WideCharToMultiByte(CP_ACP, 0, end, -1, NULL, 0, NULL, NULL);
      *endptr = (char *)nptr + (xpng_strlen(nptr) - len + 1);
      free(str);
   }
   return result;
}
#  endif
#endif

#ifndef PNG_READ_BIG_ENDIAN_SUPPORTED
/* Grab an unsigned 32-bit integer from a buffer in big-endian format. */
xpng_uint_32 /* PRIVATE */
xpng_get_uint_32(xpng_bytep buf)
{
   xpng_uint_32 i = ((xpng_uint_32)(*buf) << 24) +
      ((xpng_uint_32)(*(buf + 1)) << 16) +
      ((xpng_uint_32)(*(buf + 2)) << 8) +
      (xpng_uint_32)(*(buf + 3));

   return (i);
}

#if defined(PNG_READ_pCAL_SUPPORTED) || defined(PNG_READ_oFFs_SUPPORTED)
/* Grab a signed 32-bit integer from a buffer in big-endian format.  The
 * data is stored in the PNG file in two's complement format, and it is
 * assumed that the machine format for signed integers is the same. */
xpng_int_32 /* PRIVATE */
xpng_get_int_32(xpng_bytep buf)
{
   xpng_int_32 i = ((xpng_int_32)(*buf) << 24) +
      ((xpng_int_32)(*(buf + 1)) << 16) +
      ((xpng_int_32)(*(buf + 2)) << 8) +
      (xpng_int_32)(*(buf + 3));

   return (i);
}
#endif /* PNG_READ_pCAL_SUPPORTED */

/* Grab an unsigned 16-bit integer from a buffer in big-endian format. */
xpng_uint_16 /* PRIVATE */
xpng_get_uint_16(xpng_bytep buf)
{
   xpng_uint_16 i = (xpng_uint_16)(((xpng_uint_16)(*buf) << 8) +
      (xpng_uint_16)(*(buf + 1)));

   return (i);
}
#endif /* PNG_READ_BIG_ENDIAN_SUPPORTED */

/* Read data, and (optionally) run it through the CRC. */
void /* PRIVATE */
xpng_crc_read(xpng_structp xpng_ptr, xpng_bytep buf, xpng_size_t length)
{
   xpng_read_data(xpng_ptr, buf, length);
   xpng_calculate_crc(xpng_ptr, buf, length);
}

/* Optionally skip data and then check the CRC.  Depending on whether we
   are reading a ancillary or critical chunk, and how the program has set
   things up, we may calculate the CRC on the data and print a message.
   Returns '1' if there was a CRC error, '0' otherwise. */
int /* PRIVATE */
xpng_crc_finish(xpng_structp xpng_ptr, xpng_uint_32 skip)
{
   xpng_size_t i;
   xpng_size_t istop = xpng_ptr->zbuf_size;

   for (i = (xpng_size_t)skip; i > istop; i -= istop)
   {
      xpng_crc_read(xpng_ptr, xpng_ptr->zbuf, xpng_ptr->zbuf_size);
   }
   if (i)
   {
      xpng_crc_read(xpng_ptr, xpng_ptr->zbuf, i);
   }

   if (xpng_crc_error(xpng_ptr))
   {
      if (((xpng_ptr->chunk_name[0] & 0x20) &&                /* Ancillary */
           !(xpng_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN)) ||
          (!(xpng_ptr->chunk_name[0] & 0x20) &&             /* Critical  */
          (xpng_ptr->flags & PNG_FLAG_CRC_CRITICAL_USE)))
      {
         xpng_chunk_warning(xpng_ptr, "CRC error");
      }
      else
      {
         xpng_chunk_error(xpng_ptr, "CRC error");
      }
      return (1);
   }

   return (0);
}

/* Compare the CRC stored in the PNG file with that calculated by libpng from
   the data it has read thus far. */
int /* PRIVATE */
xpng_crc_error(xpng_structp xpng_ptr)
{
   xpng_byte crc_bytes[4];
   xpng_uint_32 crc;
   int need_crc = 1;

   if (xpng_ptr->chunk_name[0] & 0x20)                     /* ancillary */
   {
      if ((xpng_ptr->flags & PNG_FLAG_CRC_ANCILLARY_MASK) ==
          (PNG_FLAG_CRC_ANCILLARY_USE | PNG_FLAG_CRC_ANCILLARY_NOWARN))
         need_crc = 0;
   }
   else                                                    /* critical */
   {
      if (xpng_ptr->flags & PNG_FLAG_CRC_CRITICAL_IGNORE)
         need_crc = 0;
   }

   xpng_read_data(xpng_ptr, crc_bytes, 4);

   if (need_crc)
   {
      crc = xpng_get_uint_32(crc_bytes);
      return ((int)(crc != xpng_ptr->crc));
   }
   else
      return (0);
}

#if defined(PNG_READ_zTXt_SUPPORTED) || defined(PNG_READ_iTXt_SUPPORTED) || \
    defined(PNG_READ_iCCP_SUPPORTED)
/*
 * Decompress trailing data in a chunk.  The assumption is that chunkdata
 * points at an allocated area holding the contents of a chunk with a
 * trailing compressed part.  What we get back is an allocated area
 * holding the original prefix part and an uncompressed version of the
 * trailing part (the malloc area passed in is freed).
 */
xpng_charp /* PRIVATE */
xpng_decompress_chunk(xpng_structp xpng_ptr, int comp_type,
                              xpng_charp chunkdata, xpng_size_t chunklength,
                              xpng_size_t prefix_size, xpng_size_t *newlength)
{
   static char msg[] = "Error decoding compressed text";
   xpng_charp text = NULL;
   xpng_size_t text_size;

   if (comp_type == PNG_COMPRESSION_TYPE_BASE)
   {
      int ret = Z_OK;
      xpng_ptr->zstream.next_in = (xpng_bytep)(chunkdata + prefix_size);
      xpng_ptr->zstream.avail_in = (uInt)(chunklength - prefix_size);
      xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
      xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;

      text_size = 0;
      text = NULL;

      while (xpng_ptr->zstream.avail_in)
      {
         ret = inflate(&xpng_ptr->zstream, Z_PARTIAL_FLUSH);
         if (ret != Z_OK && ret != Z_STREAM_END)
         {
            if (xpng_ptr->zstream.msg != NULL)
               xpng_warning(xpng_ptr, xpng_ptr->zstream.msg);
            else
               xpng_warning(xpng_ptr, msg);
            inflateReset(&xpng_ptr->zstream);
            xpng_ptr->zstream.avail_in = 0;

            if (text ==  NULL)
            {
               text_size = prefix_size + sizeof(msg) + 1;
               text = (xpng_charp)xpng_malloc_warn(xpng_ptr, text_size);
               if (text ==  NULL)
                 {
                    xpng_free(xpng_ptr,chunkdata);
                    xpng_error(xpng_ptr,"Not enough memory to decompress chunk");
                 }
               xpng_memcpy(text, chunkdata, prefix_size);
            }

            text[text_size - 1] = 0x00;

            /* Copy what we can of the error message into the text chunk */
            text_size = (xpng_size_t)(chunklength - (text - chunkdata) - 1);
            text_size = sizeof(msg) > text_size ? text_size : sizeof(msg);
            xpng_memcpy(text + prefix_size, msg, text_size + 1);
            break;
         }
         if (!xpng_ptr->zstream.avail_out || ret == Z_STREAM_END)
         {
            if (text == NULL)
            {
               text_size = prefix_size +
                   xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out;
               text = (xpng_charp)xpng_malloc_warn(xpng_ptr, text_size + 1);
               if (text ==  NULL)
                 {
                    xpng_free(xpng_ptr,chunkdata);
                    xpng_error(xpng_ptr,"Not enough memory to decompress chunk.");
                 }
               xpng_memcpy(text + prefix_size, xpng_ptr->zbuf,
                    text_size - prefix_size);
               xpng_memcpy(text, chunkdata, prefix_size);
               *(text + text_size) = 0x00;
            }
            else
            {
               xpng_charp tmp;

               tmp = text;
               text = (xpng_charp)xpng_malloc_warn(xpng_ptr,
                  (xpng_uint_32)(text_size +
                  xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out + 1));
               if (text == NULL)
               {
                  xpng_free(xpng_ptr, tmp);
                  xpng_free(xpng_ptr, chunkdata);
                  xpng_error(xpng_ptr,"Not enough memory to decompress chunk..");
               }
               xpng_memcpy(text, tmp, text_size);
               xpng_free(xpng_ptr, tmp);
               xpng_memcpy(text + text_size, xpng_ptr->zbuf,
                  (xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out));
               text_size += xpng_ptr->zbuf_size - xpng_ptr->zstream.avail_out;
               *(text + text_size) = 0x00;
            }
            if (ret == Z_STREAM_END)
               break;
            else
            {
               xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
               xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
            }
         }
      }
      if (ret != Z_STREAM_END)
      {
#if !defined(PNG_NO_STDIO) && !defined(_WIN32_WCE)
         char umsg[50];

         if (ret == Z_BUF_ERROR)
            sprintf(umsg,"Buffer error in compressed datastream in %s chunk",
                xpng_ptr->chunk_name);
         else if (ret == Z_DATA_ERROR)
            sprintf(umsg,"Data error in compressed datastream in %s chunk",
                xpng_ptr->chunk_name);
         else
            sprintf(umsg,"Incomplete compressed datastream in %s chunk",
                xpng_ptr->chunk_name);
         xpng_warning(xpng_ptr, umsg);
#else
         xpng_warning(xpng_ptr,
            "Incomplete compressed datastream in chunk other than IDAT");
#endif
         text_size=prefix_size;
         if (text ==  NULL)
         {
            text = (xpng_charp)xpng_malloc_warn(xpng_ptr, text_size+1);
            if (text == NULL)
              {
                xpng_free(xpng_ptr, chunkdata);
                xpng_error(xpng_ptr,"Not enough memory for text.");
              }
            xpng_memcpy(text, chunkdata, prefix_size);
         }
         *(text + text_size) = 0x00;
      }

      inflateReset(&xpng_ptr->zstream);
      xpng_ptr->zstream.avail_in = 0;

      xpng_free(xpng_ptr, chunkdata);
      chunkdata = text;
      *newlength=text_size;
   }
   else /* if (comp_type != PNG_COMPRESSION_TYPE_BASE) */
   {
#if !defined(PNG_NO_STDIO) && !defined(_WIN32_WCE)
      char umsg[50];

      sprintf(umsg, "Unknown zTXt compression type %d", comp_type);
      xpng_warning(xpng_ptr, umsg);
#else
      xpng_warning(xpng_ptr, "Unknown zTXt compression type");
#endif

      *(chunkdata + prefix_size) = 0x00;
      *newlength=prefix_size;
   }

   return chunkdata;
}
#endif

/* read and check the IDHR chunk */
void /* PRIVATE */
xpng_handle_IHDR(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_byte buf[13];
   xpng_uint_32 width, height;
   int bit_depth, color_type, compression_type, filter_type;
   int interlace_type;

   xpng_debug(1, "in xpng_handle_IHDR\n");

   if (xpng_ptr->mode & PNG_HAVE_IHDR)
      xpng_error(xpng_ptr, "Out of place IHDR");

   /* check the length */
   if (length != 13)
      xpng_error(xpng_ptr, "Invalid IHDR chunk");

   xpng_ptr->mode |= PNG_HAVE_IHDR;

   xpng_crc_read(xpng_ptr, buf, 13);
   xpng_crc_finish(xpng_ptr, 0);

   width = xpng_get_uint_32(buf);
   height = xpng_get_uint_32(buf + 4);
   bit_depth = buf[8];
   color_type = buf[9];
   compression_type = buf[10];
   filter_type = buf[11];
   interlace_type = buf[12];


   /* set internal variables */
   xpng_ptr->width = width;
   xpng_ptr->height = height;
   xpng_ptr->bit_depth = (xpng_byte)bit_depth;
   xpng_ptr->interlaced = (xpng_byte)interlace_type;
   xpng_ptr->color_type = (xpng_byte)color_type;
#if defined(PNG_MNG_FEATURES_SUPPORTED)
   xpng_ptr->filter_type = (xpng_byte)filter_type;
#endif

   /* find number of channels */
   switch (xpng_ptr->color_type)
   {
      case PNG_COLOR_TYPE_GRAY:
      case PNG_COLOR_TYPE_PALETTE:
         xpng_ptr->channels = 1;
         break;
      case PNG_COLOR_TYPE_RGB:
         xpng_ptr->channels = 3;
         break;
      case PNG_COLOR_TYPE_GRAY_ALPHA:
         xpng_ptr->channels = 2;
         break;
      case PNG_COLOR_TYPE_RGB_ALPHA:
         xpng_ptr->channels = 4;
         break;
   }

   /* set up other useful info */
   xpng_ptr->pixel_depth = (xpng_byte)(xpng_ptr->bit_depth *
   xpng_ptr->channels);
   xpng_ptr->rowbytes = ((xpng_ptr->width *
      (xpng_uint_32)xpng_ptr->pixel_depth + 7) >> 3);
   xpng_debug1(3,"bit_depth = %d\n", xpng_ptr->bit_depth);
   xpng_debug1(3,"channels = %d\n", xpng_ptr->channels);
   xpng_debug1(3,"rowbytes = %lu\n", xpng_ptr->rowbytes);
   xpng_set_IHDR(xpng_ptr, info_ptr, width, height, bit_depth,
      color_type, interlace_type, compression_type, filter_type);
}

/* read and check the palette */
void /* PRIVATE */
xpng_handle_PLTE(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_color palette[PNG_MAX_PALETTE_LENGTH];
   int num, i;
#ifndef PNG_NO_POINTER_INDEXING
   xpng_colorp pal_ptr;
#endif

   xpng_debug(1, "in xpng_handle_PLTE\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before PLTE");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid PLTE after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (xpng_ptr->mode & PNG_HAVE_PLTE)
      xpng_error(xpng_ptr, "Duplicate PLTE chunk");

   xpng_ptr->mode |= PNG_HAVE_PLTE;

   if (!(xpng_ptr->color_type&PNG_COLOR_MASK_COLOR))
   {
      xpng_warning(xpng_ptr,
        "Ignoring PLTE chunk in grayscale PNG");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
#if !defined(PNG_READ_OPT_PLTE_SUPPORTED)
   if (xpng_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
   {
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
#endif

   if (length > 3*PNG_MAX_PALETTE_LENGTH || length % 3)
   {
      if (xpng_ptr->color_type != PNG_COLOR_TYPE_PALETTE)
      {
         xpng_warning(xpng_ptr, "Invalid palette chunk");
         xpng_crc_finish(xpng_ptr, length);
         return;
      }
      else
      {
         xpng_error(xpng_ptr, "Invalid palette chunk");
      }
   }

   num = (int)length / 3;

#ifndef PNG_NO_POINTER_INDEXING
   for (i = 0, pal_ptr = palette; i < num; i++, pal_ptr++)
   {
      xpng_byte buf[3];

      xpng_crc_read(xpng_ptr, buf, 3);
      pal_ptr->red = buf[0];
      pal_ptr->green = buf[1];
      pal_ptr->blue = buf[2];
   }
#else
   for (i = 0; i < num; i++)
   {
      xpng_byte buf[3];

      xpng_crc_read(xpng_ptr, buf, 3);
      /* don't depend upon xpng_color being any order */
      palette[i].red = buf[0];
      palette[i].green = buf[1];
      palette[i].blue = buf[2];
   }
#endif

   /* If we actually NEED the PLTE chunk (ie for a paletted image), we do
      whatever the normal CRC configuration tells us.  However, if we
      have an RGB image, the PLTE can be considered ancillary, so
      we will act as though it is. */
#if !defined(PNG_READ_OPT_PLTE_SUPPORTED)
   if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
#endif
   {
      xpng_crc_finish(xpng_ptr, 0);
   }
#if !defined(PNG_READ_OPT_PLTE_SUPPORTED)
   else if (xpng_crc_error(xpng_ptr))  /* Only if we have a CRC error */
   {
      /* If we don't want to use the data from an ancillary chunk,
         we have two options: an error abort, or a warning and we
         ignore the data in this chunk (which should be OK, since
         it's considered ancillary for a RGB or RGBA image). */
      if (!(xpng_ptr->flags & PNG_FLAG_CRC_ANCILLARY_USE))
      {
         if (xpng_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN)
         {
            xpng_chunk_error(xpng_ptr, "CRC error");
         }
         else
         {
            xpng_chunk_warning(xpng_ptr, "CRC error");
            return;
         }
      }
      /* Otherwise, we (optionally) emit a warning and use the chunk. */
      else if (!(xpng_ptr->flags & PNG_FLAG_CRC_ANCILLARY_NOWARN))
      {
         xpng_chunk_warning(xpng_ptr, "CRC error");
      }
   }
#endif

   xpng_set_PLTE(xpng_ptr, info_ptr, palette, num);

#if defined(PNG_READ_tRNS_SUPPORTED)
   if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tRNS))
      {
         if (xpng_ptr->num_trans > (xpng_uint_16)num)
         {
            xpng_warning(xpng_ptr, "Truncating incorrect tRNS chunk length");
            xpng_ptr->num_trans = (xpng_uint_16)num;
         }
         if (info_ptr->num_trans > (xpng_uint_16)num)
         {
            xpng_warning(xpng_ptr, "Truncating incorrect info tRNS chunk length");
            info_ptr->num_trans = (xpng_uint_16)num;
         }
      }
   }
#endif

}

void /* PRIVATE */
xpng_handle_IEND(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_debug(1, "in xpng_handle_IEND\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR) || !(xpng_ptr->mode & PNG_HAVE_IDAT))
   {
      xpng_error(xpng_ptr, "No image in file");

      info_ptr = info_ptr; /* quiet compiler warnings about unused info_ptr */
   }

   xpng_ptr->mode |= (PNG_AFTER_IDAT | PNG_HAVE_IEND);

   if (length != 0)
   {
      xpng_warning(xpng_ptr, "Incorrect IEND chunk length");
   }
   xpng_crc_finish(xpng_ptr, length);
}

#if defined(PNG_READ_gAMA_SUPPORTED)
void /* PRIVATE */
xpng_handle_gAMA(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_fixed_point igamma;
#ifdef PNG_FLOATING_POINT_SUPPORTED
   float file_gamma;
#endif
   xpng_byte buf[4];

   xpng_debug(1, "in xpng_handle_gAMA\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before gAMA");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid gAMA after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (xpng_ptr->mode & PNG_HAVE_PLTE)
      /* Should be an error, but we can cope with it */
      xpng_warning(xpng_ptr, "Out of place gAMA chunk");

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_gAMA)
#if defined(PNG_READ_sRGB_SUPPORTED)
      && !(info_ptr->valid & PNG_INFO_sRGB)
#endif
      )
   {
      xpng_warning(xpng_ptr, "Duplicate gAMA chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (length != 4)
   {
      xpng_warning(xpng_ptr, "Incorrect gAMA chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_crc_read(xpng_ptr, buf, 4);
   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   igamma = (xpng_fixed_point)xpng_get_uint_32(buf);
   /* check for zero gamma */
   if (igamma == 0)
      {
         xpng_warning(xpng_ptr,
           "Ignoring gAMA chunk with gamma=0");
         return;
      }

#if defined(PNG_READ_sRGB_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_sRGB)
      if(igamma < 45000L || igamma > 46000L)
      {
         xpng_warning(xpng_ptr,
           "Ignoring incorrect gAMA value when sRGB is also present");
#ifndef PNG_NO_CONSOLE_IO
         fprintf(stderr, "gamma = (%d/100000)\n", (int)igamma);
#endif
         return;
      }
#endif /* PNG_READ_sRGB_SUPPORTED */

#ifdef PNG_FLOATING_POINT_SUPPORTED
   file_gamma = (float)igamma / (float)100000.0;
#  ifdef PNG_READ_GAMMA_SUPPORTED
     xpng_ptr->gamma = file_gamma;
#  endif
     xpng_set_gAMA(xpng_ptr, info_ptr, file_gamma);
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   xpng_set_gAMA_fixed(xpng_ptr, info_ptr, igamma);
#endif
}
#endif

#if defined(PNG_READ_sBIT_SUPPORTED)
void /* PRIVATE */
xpng_handle_sBIT(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_size_t truelen;
   xpng_byte buf[4];

   xpng_debug(1, "in xpng_handle_sBIT\n");

   buf[0] = buf[1] = buf[2] = buf[3] = 0;

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before sBIT");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid sBIT after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (xpng_ptr->mode & PNG_HAVE_PLTE)
   {
      /* Should be an error, but we can cope with it */
      xpng_warning(xpng_ptr, "Out of place sBIT chunk");
   }
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sBIT))
   {
      xpng_warning(xpng_ptr, "Duplicate sBIT chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      truelen = 3;
   else
      truelen = (xpng_size_t)xpng_ptr->channels;

   if (length != truelen)
   {
      xpng_warning(xpng_ptr, "Incorrect sBIT chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_crc_read(xpng_ptr, buf, truelen);
   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   if (xpng_ptr->color_type & PNG_COLOR_MASK_COLOR)
   {
      xpng_ptr->sig_bit.red = buf[0];
      xpng_ptr->sig_bit.green = buf[1];
      xpng_ptr->sig_bit.blue = buf[2];
      xpng_ptr->sig_bit.alpha = buf[3];
   }
   else
   {
      xpng_ptr->sig_bit.gray = buf[0];
      xpng_ptr->sig_bit.red = buf[0];
      xpng_ptr->sig_bit.green = buf[0];
      xpng_ptr->sig_bit.blue = buf[0];
      xpng_ptr->sig_bit.alpha = buf[1];
   }
   xpng_set_sBIT(xpng_ptr, info_ptr, &(xpng_ptr->sig_bit));
}
#endif

#if defined(PNG_READ_cHRM_SUPPORTED)
void /* PRIVATE */
xpng_handle_cHRM(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_byte buf[4];
#ifdef PNG_FLOATING_POINT_SUPPORTED
   float white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y;
#endif
   xpng_fixed_point int_x_white, int_y_white, int_x_red, int_y_red, int_x_green,
      int_y_green, int_x_blue, int_y_blue;

   xpng_uint_32 uint_x, uint_y;

   xpng_debug(1, "in xpng_handle_cHRM\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before cHRM");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (xpng_ptr->mode & PNG_HAVE_PLTE)
      /* Should be an error, but we can cope with it */
      xpng_warning(xpng_ptr, "Missing PLTE before cHRM");

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_cHRM)
#if defined(PNG_READ_sRGB_SUPPORTED)
      && !(info_ptr->valid & PNG_INFO_sRGB)
#endif
      )
   {
      xpng_warning(xpng_ptr, "Duplicate cHRM chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (length != 32)
   {
      xpng_warning(xpng_ptr, "Incorrect cHRM chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_crc_read(xpng_ptr, buf, 4);
   uint_x = xpng_get_uint_32(buf);

   xpng_crc_read(xpng_ptr, buf, 4);
   uint_y = xpng_get_uint_32(buf);

   if (uint_x > 80000L || uint_y > 80000L ||
      uint_x + uint_y > 100000L)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM white point");
      xpng_crc_finish(xpng_ptr, 24);
      return;
   }
   int_x_white = (xpng_fixed_point)uint_x;
   int_y_white = (xpng_fixed_point)uint_y;

   xpng_crc_read(xpng_ptr, buf, 4);
   uint_x = xpng_get_uint_32(buf);

   xpng_crc_read(xpng_ptr, buf, 4);
   uint_y = xpng_get_uint_32(buf);

   if (uint_x > 80000L || uint_y > 80000L ||
      uint_x + uint_y > 100000L)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM red point");
      xpng_crc_finish(xpng_ptr, 16);
      return;
   }
   int_x_red = (xpng_fixed_point)uint_x;
   int_y_red = (xpng_fixed_point)uint_y;

   xpng_crc_read(xpng_ptr, buf, 4);
   uint_x = xpng_get_uint_32(buf);

   xpng_crc_read(xpng_ptr, buf, 4);
   uint_y = xpng_get_uint_32(buf);

   if (uint_x > 80000L || uint_y > 80000L ||
      uint_x + uint_y > 100000L)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM green point");
      xpng_crc_finish(xpng_ptr, 8);
      return;
   }
   int_x_green = (xpng_fixed_point)uint_x;
   int_y_green = (xpng_fixed_point)uint_y;

   xpng_crc_read(xpng_ptr, buf, 4);
   uint_x = xpng_get_uint_32(buf);

   xpng_crc_read(xpng_ptr, buf, 4);
   uint_y = xpng_get_uint_32(buf);

   if (uint_x > 80000L || uint_y > 80000L ||
      uint_x + uint_y > 100000L)
   {
      xpng_warning(xpng_ptr, "Invalid cHRM blue point");
      xpng_crc_finish(xpng_ptr, 0);
      return;
   }
   int_x_blue = (xpng_fixed_point)uint_x;
   int_y_blue = (xpng_fixed_point)uint_y;

#ifdef PNG_FLOATING_POINT_SUPPORTED
   white_x = (float)int_x_white / (float)100000.0;
   white_y = (float)int_y_white / (float)100000.0;
   red_x   = (float)int_x_red   / (float)100000.0;
   red_y   = (float)int_y_red   / (float)100000.0;
   green_x = (float)int_x_green / (float)100000.0;
   green_y = (float)int_y_green / (float)100000.0;
   blue_x  = (float)int_x_blue  / (float)100000.0;
   blue_y  = (float)int_y_blue  / (float)100000.0;
#endif

#if defined(PNG_READ_sRGB_SUPPORTED)
   if (info_ptr->valid & PNG_INFO_sRGB)
      {
      if (abs(int_x_white - 31270L) > 1000 ||
          abs(int_y_white - 32900L) > 1000 ||
          abs(int_x_red   - 64000L) > 1000 ||
          abs(int_y_red   - 33000L) > 1000 ||
          abs(int_x_green - 30000L) > 1000 ||
          abs(int_y_green - 60000L) > 1000 ||
          abs(int_x_blue  - 15000L) > 1000 ||
          abs(int_y_blue  -  6000L) > 1000)
         {

            xpng_warning(xpng_ptr,
              "Ignoring incorrect cHRM value when sRGB is also present");
#ifndef PNG_NO_CONSOLE_IO
#ifdef PNG_FLOATING_POINT_SUPPORTED
            fprintf(stderr,"wx=%f, wy=%f, rx=%f, ry=%f\n",
               white_x, white_y, red_x, red_y);
            fprintf(stderr,"gx=%f, gy=%f, bx=%f, by=%f\n",
               green_x, green_y, blue_x, blue_y);
#else
            fprintf(stderr,"wx=%ld, wy=%ld, rx=%ld, ry=%ld\n",
               int_x_white, int_y_white, int_x_red, int_y_red);
            fprintf(stderr,"gx=%ld, gy=%ld, bx=%ld, by=%ld\n",
               int_x_green, int_y_green, int_x_blue, int_y_blue);
#endif
#endif /* PNG_NO_CONSOLE_IO */
         }
         xpng_crc_finish(xpng_ptr, 0);
         return;
      }
#endif /* PNG_READ_sRGB_SUPPORTED */

#ifdef PNG_FLOATING_POINT_SUPPORTED
   xpng_set_cHRM(xpng_ptr, info_ptr,
      white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y);
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   xpng_set_cHRM_fixed(xpng_ptr, info_ptr,
      int_x_white, int_y_white, int_x_red, int_y_red, int_x_green,
      int_y_green, int_x_blue, int_y_blue);
#endif
   if (xpng_crc_finish(xpng_ptr, 0))
      return;
}
#endif

#if defined(PNG_READ_sRGB_SUPPORTED)
void /* PRIVATE */
xpng_handle_sRGB(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   int intent;
   xpng_byte buf[1];

   xpng_debug(1, "in xpng_handle_sRGB\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before sRGB");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid sRGB after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (xpng_ptr->mode & PNG_HAVE_PLTE)
      /* Should be an error, but we can cope with it */
      xpng_warning(xpng_ptr, "Out of place sRGB chunk");

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sRGB))
   {
      xpng_warning(xpng_ptr, "Duplicate sRGB chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (length != 1)
   {
      xpng_warning(xpng_ptr, "Incorrect sRGB chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_crc_read(xpng_ptr, buf, 1);
   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   intent = buf[0];
   /* check for bad intent */
   if (intent >= PNG_sRGB_INTENT_LAST)
   {
      xpng_warning(xpng_ptr, "Unknown sRGB intent");
      return;
   }

#if defined(PNG_READ_gAMA_SUPPORTED) && defined(PNG_READ_GAMMA_SUPPORTED)
   if ((info_ptr->valid & PNG_INFO_gAMA))
   {
   int igamma;
#ifdef PNG_FIXED_POINT_SUPPORTED
      igamma=(int)info_ptr->int_gamma;
#else
#  ifdef PNG_FLOATING_POINT_SUPPORTED
      igamma=(int)(info_ptr->gamma * 100000.);
#  endif
#endif
      if(igamma < 45000L || igamma > 46000L)
      {
         xpng_warning(xpng_ptr,
           "Ignoring incorrect gAMA value when sRGB is also present");
#ifndef PNG_NO_CONSOLE_IO
#  ifdef PNG_FIXED_POINT_SUPPORTED
         fprintf(stderr,"incorrect gamma=(%d/100000)\n",(int)xpng_ptr->int_gamma);
#  else
#    ifdef PNG_FLOATING_POINT_SUPPORTED
         fprintf(stderr,"incorrect gamma=%f\n",xpng_ptr->gamma);
#    endif
#  endif
#endif
      }
   }
#endif /* PNG_READ_gAMA_SUPPORTED */

#ifdef PNG_READ_cHRM_SUPPORTED
#ifdef PNG_FIXED_POINT_SUPPORTED
   if (info_ptr->valid & PNG_INFO_cHRM)
      if (abs(info_ptr->int_x_white - 31270L) > 1000 ||
          abs(info_ptr->int_y_white - 32900L) > 1000 ||
          abs(info_ptr->int_x_red   - 64000L) > 1000 ||
          abs(info_ptr->int_y_red   - 33000L) > 1000 ||
          abs(info_ptr->int_x_green - 30000L) > 1000 ||
          abs(info_ptr->int_y_green - 60000L) > 1000 ||
          abs(info_ptr->int_x_blue  - 15000L) > 1000 ||
          abs(info_ptr->int_y_blue  -  6000L) > 1000)
         {
            xpng_warning(xpng_ptr,
              "Ignoring incorrect cHRM value when sRGB is also present");
         }
#endif /* PNG_FIXED_POINT_SUPPORTED */
#endif /* PNG_READ_cHRM_SUPPORTED */

   xpng_set_sRGB_gAMA_and_cHRM(xpng_ptr, info_ptr, intent);
}
#endif /* PNG_READ_sRGB_SUPPORTED */

#if defined(PNG_READ_iCCP_SUPPORTED)
void /* PRIVATE */
xpng_handle_iCCP(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
/* Note: this does not properly handle chunks that are > 64K under DOS */
{
   xpng_charp chunkdata;
   xpng_byte compression_type;
   xpng_bytep pC;
   xpng_charp profile;
   xpng_uint_32 skip = 0;
   xpng_uint_32 profile_size = 0;
   xpng_uint_32 profile_length = 0;
   xpng_size_t slength, prefix_length, data_length;

   xpng_debug(1, "in xpng_handle_iCCP\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before iCCP");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid iCCP after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (xpng_ptr->mode & PNG_HAVE_PLTE)
      /* Should be an error, but we can cope with it */
      xpng_warning(xpng_ptr, "Out of place iCCP chunk");

   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_iCCP))
   {
      xpng_warning(xpng_ptr, "Duplicate iCCP chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

#ifdef PNG_MAX_MALLOC_64K
   if (length > (xpng_uint_32)65535L)
   {
      xpng_warning(xpng_ptr, "iCCP chunk too large to fit in memory");
      skip = length - (xpng_uint_32)65535L;
      length = (xpng_uint_32)65535L;
   }
#endif

   chunkdata = (xpng_charp)xpng_malloc(xpng_ptr, length + 1);
   slength = (xpng_size_t)length;
   xpng_crc_read(xpng_ptr, (xpng_bytep)chunkdata, slength);

   if (xpng_crc_finish(xpng_ptr, skip))
   {
      xpng_free(xpng_ptr, chunkdata);
      return;
   }

   chunkdata[slength] = 0x00;

   for (profile = chunkdata; *profile; profile++)
      /* empty loop to find end of name */ ;

   ++profile;

   /* there should be at least one zero (the compression type byte)
      following the separator, and we should be on it  */
   if ( profile >= chunkdata + slength)
   {
      xpng_free(xpng_ptr, chunkdata);
      xpng_warning(xpng_ptr, "Malformed iCCP chunk");
      return;
   }

   /* compression_type should always be zero */
   compression_type = *profile++;
   if (compression_type)
   {
      xpng_warning(xpng_ptr, "Ignoring nonzero compression type in iCCP chunk");
      compression_type=0x00;  /* Reset it to zero (libpng-1.0.6 through 1.0.8
                                 wrote nonzero) */
   }

   prefix_length = profile - chunkdata;
   chunkdata = xpng_decompress_chunk(xpng_ptr, compression_type, chunkdata,
                                    slength, prefix_length, &data_length);

   profile_length = data_length - prefix_length;

   if ( prefix_length > data_length || profile_length < 4)
   {
      xpng_free(xpng_ptr, chunkdata);
      xpng_warning(xpng_ptr, "Profile size field missing from iCCP chunk");
      return;
   }

   /* Check the profile_size recorded in the first 32 bits of the ICC profile */
   pC = (xpng_bytep)(chunkdata+prefix_length);
   profile_size = ((*(pC  ))<<24) |
                  ((*(pC+1))<<16) |
                  ((*(pC+2))<< 8) |
                  ((*(pC+3))    );

   if(profile_size < profile_length)
      profile_length = profile_size;

   if(profile_size > profile_length)
   {
      xpng_free(xpng_ptr, chunkdata);
      xpng_warning(xpng_ptr, "Ignoring truncated iCCP profile.\n");
      return;
   }

   xpng_set_iCCP(xpng_ptr, info_ptr, chunkdata, compression_type,
                chunkdata + prefix_length, profile_length);
   xpng_free(xpng_ptr, chunkdata);
}
#endif /* PNG_READ_iCCP_SUPPORTED */

#if defined(PNG_READ_sPLT_SUPPORTED)
void /* PRIVATE */
xpng_handle_sPLT(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
/* Note: this does not properly handle chunks that are > 64K under DOS */
{
   xpng_bytep chunkdata;
   xpng_bytep entry_start;
   xpng_sPLT_t new_palette;
#ifdef PNG_NO_POINTER_INDEXING
   xpng_sPLT_entryp pp;
#endif
   int data_length, entry_size, i;
   xpng_uint_32 skip = 0;
   xpng_size_t slength;

   xpng_debug(1, "in xpng_handle_sPLT\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before sPLT");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid sPLT after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

#ifdef PNG_MAX_MALLOC_64K
   if (length > (xpng_uint_32)65535L)
   {
      xpng_warning(xpng_ptr, "sPLT chunk too large to fit in memory");
      skip = length - (xpng_uint_32)65535L;
      length = (xpng_uint_32)65535L;
   }
#endif

   chunkdata = (xpng_bytep)xpng_malloc(xpng_ptr, length + 1);
   slength = (xpng_size_t)length;
   xpng_crc_read(xpng_ptr, (xpng_bytep)chunkdata, slength);

   if (xpng_crc_finish(xpng_ptr, skip))
   {
      xpng_free(xpng_ptr, chunkdata);
      return;
   }

   chunkdata[slength] = 0x00;

   for (entry_start = chunkdata; *entry_start; entry_start++)
      /* empty loop to find end of name */ ;
   ++entry_start;

   /* a sample depth should follow the separator, and we should be on it  */
   if (entry_start > chunkdata + slength)
   {
      xpng_free(xpng_ptr, chunkdata);
      xpng_warning(xpng_ptr, "malformed sPLT chunk");
      return;
   }

   new_palette.depth = *entry_start++;
   entry_size = (new_palette.depth == 8 ? 6 : 10);
   data_length = (slength - (entry_start - chunkdata));

   /* integrity-check the data length */
   if (data_length % entry_size)
   {
      xpng_free(xpng_ptr, chunkdata);
      xpng_warning(xpng_ptr, "sPLT chunk has bad length");
      return;
   }

   new_palette.nentries = data_length / entry_size;
   new_palette.entries = (xpng_sPLT_entryp)xpng_malloc(
       xpng_ptr, new_palette.nentries * sizeof(xpng_sPLT_entry));

#ifndef PNG_NO_POINTER_INDEXING
   for (i = 0; i < new_palette.nentries; i++)
   {
      xpng_sPLT_entryp pp = new_palette.entries + i;

      if (new_palette.depth == 8)
      {
          pp->red = *entry_start++;
          pp->green = *entry_start++;
          pp->blue = *entry_start++;
          pp->alpha = *entry_start++;
      }
      else
      {
          pp->red   = xpng_get_uint_16(entry_start); entry_start += 2;
          pp->green = xpng_get_uint_16(entry_start); entry_start += 2;
          pp->blue  = xpng_get_uint_16(entry_start); entry_start += 2;
          pp->alpha = xpng_get_uint_16(entry_start); entry_start += 2;
      }
      pp->frequency = xpng_get_uint_16(entry_start); entry_start += 2;
   }
#else
   pp = new_palette.entries;
   for (i = 0; i < new_palette.nentries; i++)
   {

      if (new_palette.depth == 8)
      {
          pp[i].red   = *entry_start++;
          pp[i].green = *entry_start++;
          pp[i].blue  = *entry_start++;
          pp[i].alpha = *entry_start++;
      }
      else
      {
          pp[i].red   = xpng_get_uint_16(entry_start); entry_start += 2;
          pp[i].green = xpng_get_uint_16(entry_start); entry_start += 2;
          pp[i].blue  = xpng_get_uint_16(entry_start); entry_start += 2;
          pp[i].alpha = xpng_get_uint_16(entry_start); entry_start += 2;
      }
      pp->frequency = xpng_get_uint_16(entry_start); entry_start += 2;
   }
#endif

   /* discard all chunk data except the name and stash that */
   new_palette.name = (xpng_charp)chunkdata;

   xpng_set_sPLT(xpng_ptr, info_ptr, &new_palette, 1);

   xpng_free(xpng_ptr, chunkdata);
   xpng_free(xpng_ptr, new_palette.entries);
}
#endif /* PNG_READ_sPLT_SUPPORTED */

#if defined(PNG_READ_tRNS_SUPPORTED)
void /* PRIVATE */
xpng_handle_tRNS(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_byte readbuf[PNG_MAX_PALETTE_LENGTH];

   xpng_debug(1, "in xpng_handle_tRNS\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before tRNS");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid tRNS after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tRNS))
   {
      xpng_warning(xpng_ptr, "Duplicate tRNS chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (!(xpng_ptr->mode & PNG_HAVE_PLTE))
      {
         /* Should be an error, but we can cope with it */
         xpng_warning(xpng_ptr, "Missing PLTE before tRNS");
      }
      else if (length > (xpng_uint_32)xpng_ptr->num_palette)
      {
         xpng_warning(xpng_ptr, "Incorrect tRNS chunk length");
         xpng_crc_finish(xpng_ptr, length);
         return;
      }
      if (length == 0)
      {
         xpng_warning(xpng_ptr, "Zero length tRNS chunk");
         xpng_crc_finish(xpng_ptr, length);
         return;
      }

      xpng_crc_read(xpng_ptr, readbuf, (xpng_size_t)length);
      xpng_ptr->num_trans = (xpng_uint_16)length;
   }
   else if (xpng_ptr->color_type == PNG_COLOR_TYPE_RGB)
   {
      xpng_byte buf[6];

      if (length != 6)
      {
         xpng_warning(xpng_ptr, "Incorrect tRNS chunk length");
         xpng_crc_finish(xpng_ptr, length);
         return;
      }

      xpng_crc_read(xpng_ptr, buf, (xpng_size_t)length);
      xpng_ptr->num_trans = 1;
      xpng_ptr->trans_values.red = xpng_get_uint_16(buf);
      xpng_ptr->trans_values.green = xpng_get_uint_16(buf + 2);
      xpng_ptr->trans_values.blue = xpng_get_uint_16(buf + 4);
   }
   else if (xpng_ptr->color_type == PNG_COLOR_TYPE_GRAY)
   {
      xpng_byte buf[6];

      if (length != 2)
      {
         xpng_warning(xpng_ptr, "Incorrect tRNS chunk length");
         xpng_crc_finish(xpng_ptr, length);
         return;
      }

      xpng_crc_read(xpng_ptr, buf, 2);
      xpng_ptr->num_trans = 1;
      xpng_ptr->trans_values.gray = xpng_get_uint_16(buf);
   }
   else
   {
      xpng_warning(xpng_ptr, "tRNS chunk not allowed with alpha channel");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   xpng_set_tRNS(xpng_ptr, info_ptr, readbuf, xpng_ptr->num_trans,
      &(xpng_ptr->trans_values));
}
#endif

#if defined(PNG_READ_bKGD_SUPPORTED)
void /* PRIVATE */
xpng_handle_bKGD(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_size_t truelen;
   xpng_byte buf[6];

   xpng_debug(1, "in xpng_handle_bKGD\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before bKGD");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid bKGD after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE &&
            !(xpng_ptr->mode & PNG_HAVE_PLTE))
   {
      xpng_warning(xpng_ptr, "Missing PLTE before bKGD");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_bKGD))
   {
      xpng_warning(xpng_ptr, "Duplicate bKGD chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      truelen = 1;
   else if (xpng_ptr->color_type & PNG_COLOR_MASK_COLOR)
      truelen = 6;
   else
      truelen = 2;

   if (length != truelen)
   {
      xpng_warning(xpng_ptr, "Incorrect bKGD chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_crc_read(xpng_ptr, buf, truelen);
   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   /* We convert the index value into RGB components so that we can allow
    * arbitrary RGB values for background when we have transparency, and
    * so it is easy to determine the RGB values of the background color
    * from the info_ptr struct. */
   if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      xpng_ptr->background.index = buf[0];
      if(info_ptr->num_palette)
      {
          if(buf[0] > info_ptr->num_palette)
          {
             xpng_warning(xpng_ptr, "Incorrect bKGD chunk index value");
             return;
          }
          xpng_ptr->background.red =
             (xpng_uint_16)xpng_ptr->palette[buf[0]].red;
          xpng_ptr->background.green =
             (xpng_uint_16)xpng_ptr->palette[buf[0]].green;
          xpng_ptr->background.blue =
             (xpng_uint_16)xpng_ptr->palette[buf[0]].blue;
      }
   }
   else if (!(xpng_ptr->color_type & PNG_COLOR_MASK_COLOR)) /* GRAY */
   {
      xpng_ptr->background.red =
      xpng_ptr->background.green =
      xpng_ptr->background.blue =
      xpng_ptr->background.gray = xpng_get_uint_16(buf);
   }
   else
   {
      xpng_ptr->background.red = xpng_get_uint_16(buf);
      xpng_ptr->background.green = xpng_get_uint_16(buf + 2);
      xpng_ptr->background.blue = xpng_get_uint_16(buf + 4);
   }

   xpng_set_bKGD(xpng_ptr, info_ptr, &(xpng_ptr->background));
}
#endif

#if defined(PNG_READ_hIST_SUPPORTED)
void /* PRIVATE */
xpng_handle_hIST(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   int num, i;
   xpng_uint_16 readbuf[PNG_MAX_PALETTE_LENGTH];

   xpng_debug(1, "in xpng_handle_hIST\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before hIST");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid hIST after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (!(xpng_ptr->mode & PNG_HAVE_PLTE))
   {
      xpng_warning(xpng_ptr, "Missing PLTE before hIST");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_hIST))
   {
      xpng_warning(xpng_ptr, "Duplicate hIST chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   num = (int)length / 2 ;
   if (num != xpng_ptr->num_palette)
   {
      xpng_warning(xpng_ptr, "Incorrect hIST chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   for (i = 0; i < num; i++)
   {
      xpng_byte buf[2];

      xpng_crc_read(xpng_ptr, buf, 2);
      readbuf[i] = xpng_get_uint_16(buf);
   }

   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   xpng_set_hIST(xpng_ptr, info_ptr, readbuf);
}
#endif

#if defined(PNG_READ_pHYs_SUPPORTED)
void /* PRIVATE */
xpng_handle_pHYs(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_byte buf[9];
   xpng_uint_32 res_x, res_y;
   int unit_type;

   xpng_debug(1, "in xpng_handle_pHYs\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before pHYs");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid pHYs after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_pHYs))
   {
      xpng_warning(xpng_ptr, "Duplicate pHYs chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (length != 9)
   {
      xpng_warning(xpng_ptr, "Incorrect pHYs chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_crc_read(xpng_ptr, buf, 9);
   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   res_x = xpng_get_uint_32(buf);
   res_y = xpng_get_uint_32(buf + 4);
   unit_type = buf[8];
   xpng_set_pHYs(xpng_ptr, info_ptr, res_x, res_y, unit_type);
}
#endif

#if defined(PNG_READ_oFFs_SUPPORTED)
void /* PRIVATE */
xpng_handle_oFFs(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_byte buf[9];
   xpng_int_32 offset_x, offset_y;
   int unit_type;

   xpng_debug(1, "in xpng_handle_oFFs\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before oFFs");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid oFFs after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_oFFs))
   {
      xpng_warning(xpng_ptr, "Duplicate oFFs chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (length != 9)
   {
      xpng_warning(xpng_ptr, "Incorrect oFFs chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_crc_read(xpng_ptr, buf, 9);
   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   offset_x = xpng_get_int_32(buf);
   offset_y = xpng_get_int_32(buf + 4);
   unit_type = buf[8];
   xpng_set_oFFs(xpng_ptr, info_ptr, offset_x, offset_y, unit_type);
}
#endif

#if defined(PNG_READ_pCAL_SUPPORTED)
/* read the pCAL chunk (described in the PNG Extensions document) */
void /* PRIVATE */
xpng_handle_pCAL(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_charp purpose;
   xpng_int_32 X0, X1;
   xpng_byte type, nparams;
   xpng_charp buf, units, endptr;
   xpng_charpp params;
   xpng_size_t slength;
   int i;

   xpng_debug(1, "in xpng_handle_pCAL\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before pCAL");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid pCAL after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_pCAL))
   {
      xpng_warning(xpng_ptr, "Duplicate pCAL chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_debug1(2, "Allocating and reading pCAL chunk data (%lu bytes)\n",
      length + 1);
   purpose = (xpng_charp)xpng_malloc_warn(xpng_ptr, length + 1);
   if (purpose == NULL)
     {
       xpng_warning(xpng_ptr, "No memory for pCAL purpose.");
       return;
     }
   slength = (xpng_size_t)length;
   xpng_crc_read(xpng_ptr, (xpng_bytep)purpose, slength);

   if (xpng_crc_finish(xpng_ptr, 0))
   {
      xpng_free(xpng_ptr, purpose);
      return;
   }

   purpose[slength] = 0x00; /* null terminate the last string */

   xpng_debug(3, "Finding end of pCAL purpose string\n");
   for (buf = purpose; *buf; buf++)
      /* empty loop */ ;

   endptr = purpose + slength;

   /* We need to have at least 12 bytes after the purpose string
      in order to get the parameter information. */
   if (endptr <= buf + 12)
   {
      xpng_warning(xpng_ptr, "Invalid pCAL data");
      xpng_free(xpng_ptr, purpose);
      return;
   }

   xpng_debug(3, "Reading pCAL X0, X1, type, nparams, and units\n");
   X0 = xpng_get_int_32((xpng_bytep)buf+1);
   X1 = xpng_get_int_32((xpng_bytep)buf+5);
   type = buf[9];
   nparams = buf[10];
   units = buf + 11;

   xpng_debug(3, "Checking pCAL equation type and number of parameters\n");
   /* Check that we have the right number of parameters for known
      equation types. */
   if ((type == PNG_EQUATION_LINEAR && nparams != 2) ||
       (type == PNG_EQUATION_BASE_E && nparams != 3) ||
       (type == PNG_EQUATION_ARBITRARY && nparams != 3) ||
       (type == PNG_EQUATION_HYPERBOLIC && nparams != 4))
   {
      xpng_warning(xpng_ptr, "Invalid pCAL parameters for equation type");
      xpng_free(xpng_ptr, purpose);
      return;
   }
   else if (type >= PNG_EQUATION_LAST)
   {
      xpng_warning(xpng_ptr, "Unrecognized equation type for pCAL chunk");
   }

   for (buf = units; *buf; buf++)
      /* Empty loop to move past the units string. */ ;

   xpng_debug(3, "Allocating pCAL parameters array\n");
   params = (xpng_charpp)xpng_malloc_warn(xpng_ptr, (xpng_uint_32)(nparams
      *sizeof(xpng_charp))) ;
   if (params == NULL)
     {
       xpng_free(xpng_ptr, purpose);
       xpng_warning(xpng_ptr, "No memory for pCAL params.");
       return;
     }

   /* Get pointers to the start of each parameter string. */
   for (i = 0; i < (int)nparams; i++)
   {
      buf++; /* Skip the null string terminator from previous parameter. */

      xpng_debug1(3, "Reading pCAL parameter %d\n", i);
      for (params[i] = buf; *buf != 0x00 && buf <= endptr; buf++)
         /* Empty loop to move past each parameter string */ ;

      /* Make sure we haven't run out of data yet */
      if (buf > endptr)
      {
         xpng_warning(xpng_ptr, "Invalid pCAL data");
         xpng_free(xpng_ptr, purpose);
         xpng_free(xpng_ptr, params);
         return;
      }
   }

   xpng_set_pCAL(xpng_ptr, info_ptr, purpose, X0, X1, type, nparams,
      units, params);

   xpng_free(xpng_ptr, purpose);
   xpng_free(xpng_ptr, params);
}
#endif

#if defined(PNG_READ_sCAL_SUPPORTED)
/* read the sCAL chunk */
void /* PRIVATE */
xpng_handle_sCAL(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_charp buffer, ep;
#ifdef PNG_FLOATING_POINT_SUPPORTED
   double width, height;
   xpng_charp vp;
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
   xpng_charp swidth, sheight;
#endif
#endif
   xpng_size_t slength;

   xpng_debug(1, "in xpng_handle_sCAL\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before sCAL");
   else if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
      xpng_warning(xpng_ptr, "Invalid sCAL after IDAT");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_sCAL))
   {
      xpng_warning(xpng_ptr, "Duplicate sCAL chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_debug1(2, "Allocating and reading sCAL chunk data (%lu bytes)\n",
      length + 1);
   buffer = (xpng_charp)xpng_malloc_warn(xpng_ptr, length + 1);
   if (buffer == NULL)
     {
       xpng_warning(xpng_ptr, "Out of memory while processing sCAL chunk");
       return;
     }
   slength = (xpng_size_t)length;
   xpng_crc_read(xpng_ptr, (xpng_bytep)buffer, slength);

   if (xpng_crc_finish(xpng_ptr, 0))
   {
      xpng_free(xpng_ptr, buffer);
      return;
   }

   buffer[slength] = 0x00; /* null terminate the last string */

   ep = buffer + 1;        /* skip unit byte */

#ifdef PNG_FLOATING_POINT_SUPPORTED
   width = strtod(ep, &vp);
   if (*vp)
   {
       xpng_warning(xpng_ptr, "malformed width string in sCAL chunk");
       return;
   }
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
   swidth = (xpng_charp)xpng_malloc_warn(xpng_ptr, xpng_strlen(ep) + 1);
   if (swidth == NULL)
     {
       xpng_warning(xpng_ptr, "Out of memory while processing sCAL chunk width");
       return;
     }
   xpng_memcpy(swidth, ep, (xpng_size_t)xpng_strlen(ep));
#endif
#endif

   for (ep = buffer; *ep; ep++)
      /* empty loop */ ;
   ep++;

#ifdef PNG_FLOATING_POINT_SUPPORTED
   height = strtod(ep, &vp);
   if (*vp)
   {
       xpng_warning(xpng_ptr, "malformed height string in sCAL chunk");
       return;
   }
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
   sheight = (xpng_charp)xpng_malloc_warn(xpng_ptr, xpng_strlen(ep) + 1);
   if (swidth == NULL)
     {
       xpng_warning(xpng_ptr, "Out of memory while processing sCAL chunk height");
       return;
     }
   xpng_memcpy(sheight, ep, (xpng_size_t)xpng_strlen(ep));
#endif
#endif

   if (buffer + slength < ep
#ifdef PNG_FLOATING_POINT_SUPPORTED
      || width <= 0. || height <= 0.
#endif
      )
   {
      xpng_warning(xpng_ptr, "Invalid sCAL data");
      xpng_free(xpng_ptr, buffer);
#if defined(PNG_FIXED_POINT_SUPPORTED) && !defined(PNG_FLOATING_POINT_SUPPORTED)
      xpng_free(xpng_ptr, swidth);
      xpng_free(xpng_ptr, sheight);
#endif
      return;
   }


#ifdef PNG_FLOATING_POINT_SUPPORTED
   xpng_set_sCAL(xpng_ptr, info_ptr, buffer[0], width, height);
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
   xpng_set_sCAL_s(xpng_ptr, info_ptr, buffer[0], swidth, sheight);
#endif
#endif

   xpng_free(xpng_ptr, buffer);
#if defined(PNG_FIXED_POINT_SUPPORTED) && !defined(PNG_FLOATING_POINT_SUPPORTED)
   xpng_free(xpng_ptr, swidth);
   xpng_free(xpng_ptr, sheight);
#endif
}
#endif

#if defined(PNG_READ_tIME_SUPPORTED)
void /* PRIVATE */
xpng_handle_tIME(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_byte buf[7];
   xpng_time mod_time;

   xpng_debug(1, "in xpng_handle_tIME\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Out of place tIME chunk");
   else if (info_ptr != NULL && (info_ptr->valid & PNG_INFO_tIME))
   {
      xpng_warning(xpng_ptr, "Duplicate tIME chunk");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   if (xpng_ptr->mode & PNG_HAVE_IDAT)
      xpng_ptr->mode |= PNG_AFTER_IDAT;

   if (length != 7)
   {
      xpng_warning(xpng_ptr, "Incorrect tIME chunk length");
      xpng_crc_finish(xpng_ptr, length);
      return;
   }

   xpng_crc_read(xpng_ptr, buf, 7);
   if (xpng_crc_finish(xpng_ptr, 0))
      return;

   mod_time.second = buf[6];
   mod_time.minute = buf[5];
   mod_time.hour = buf[4];
   mod_time.day = buf[3];
   mod_time.month = buf[2];
   mod_time.year = xpng_get_uint_16(buf);

   xpng_set_tIME(xpng_ptr, info_ptr, &mod_time);
}
#endif

#if defined(PNG_READ_tEXt_SUPPORTED)
/* Note: this does not properly handle chunks that are > 64K under DOS */
void /* PRIVATE */
xpng_handle_tEXt(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_textp text_ptr;
   xpng_charp key;
   xpng_charp text;
   xpng_uint_32 skip = 0;
   xpng_size_t slength;
   int ret;

   xpng_debug(1, "in xpng_handle_tEXt\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before tEXt");

   if (xpng_ptr->mode & PNG_HAVE_IDAT)
      xpng_ptr->mode |= PNG_AFTER_IDAT;

#ifdef PNG_MAX_MALLOC_64K
   if (length > (xpng_uint_32)65535L)
   {
      xpng_warning(xpng_ptr, "tEXt chunk too large to fit in memory");
      skip = length - (xpng_uint_32)65535L;
      length = (xpng_uint_32)65535L;
   }
#endif

   key = (xpng_charp)xpng_malloc_warn(xpng_ptr, length + 1);
   if (key == NULL)
   {
     xpng_warning(xpng_ptr, "No memory to process text chunk.");
     return;
   }
   slength = (xpng_size_t)length;
   xpng_crc_read(xpng_ptr, (xpng_bytep)key, slength);

   if (xpng_crc_finish(xpng_ptr, skip))
   {
      xpng_free(xpng_ptr, key);
      return;
   }

   key[slength] = 0x00;

   for (text = key; *text; text++)
      /* empty loop to find end of key */ ;

   if (text != key + slength)
      text++;

   text_ptr = (xpng_textp)xpng_malloc_warn(xpng_ptr, (xpng_uint_32)sizeof(xpng_text));
   if (text_ptr == NULL)
   {
     xpng_warning(xpng_ptr, "Not enough memory to process text chunk.");
     xpng_free(xpng_ptr, key);
     return;
   }
   text_ptr->compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr->key = key;
#ifdef PNG_iTXt_SUPPORTED
   text_ptr->lang = NULL;
   text_ptr->lang_key = NULL;
   text_ptr->itxt_length = 0;
#endif
   text_ptr->text = text;
   text_ptr->text_length = xpng_strlen(text);

   ret=xpng_set_text_2(xpng_ptr, info_ptr, text_ptr, 1);

   xpng_free(xpng_ptr, key);
   xpng_free(xpng_ptr, text_ptr);
   if (ret)
     xpng_warning(xpng_ptr, "Insufficient memory to process text chunk.");
}
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
/* note: this does not correctly handle chunks that are > 64K under DOS */
void /* PRIVATE */
xpng_handle_zTXt(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_textp text_ptr;
   xpng_charp chunkdata;
   xpng_charp text;
   int comp_type;
   int ret;
   xpng_size_t slength, prefix_len, data_len;

   xpng_debug(1, "in xpng_handle_zTXt\n");
   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before zTXt");

   if (xpng_ptr->mode & PNG_HAVE_IDAT)
      xpng_ptr->mode |= PNG_AFTER_IDAT;

#ifdef PNG_MAX_MALLOC_64K
   /* We will no doubt have problems with chunks even half this size, but
      there is no hard and fast rule to tell us where to stop. */
   if (length > (xpng_uint_32)65535L)
   {
     xpng_warning(xpng_ptr,"zTXt chunk too large to fit in memory");
     xpng_crc_finish(xpng_ptr, length);
     return;
   }
#endif

   chunkdata = (xpng_charp)xpng_malloc_warn(xpng_ptr, length + 1);
   if (chunkdata == NULL)
   {
     xpng_warning(xpng_ptr,"Out of memory processing zTXt chunk.");
     return;
   }
   slength = (xpng_size_t)length;
   xpng_crc_read(xpng_ptr, (xpng_bytep)chunkdata, slength);
   if (xpng_crc_finish(xpng_ptr, 0))
   {
      xpng_free(xpng_ptr, chunkdata);
      return;
   }

   chunkdata[slength] = 0x00;

   for (text = chunkdata; *text; text++)
      /* empty loop */ ;

   /* zTXt must have some text after the chunkdataword */
   if (text == chunkdata + slength)
   {
      comp_type = PNG_TEXT_COMPRESSION_NONE;
      xpng_warning(xpng_ptr, "Zero length zTXt chunk");
   }
   else
   {
       comp_type = *(++text);
       if (comp_type != PNG_TEXT_COMPRESSION_zTXt)
       {
          xpng_warning(xpng_ptr, "Unknown compression type in zTXt chunk");
          comp_type = PNG_TEXT_COMPRESSION_zTXt;
       }
       text++;        /* skip the compression_method byte */
   }
   prefix_len = text - chunkdata;

   chunkdata = (xpng_charp)xpng_decompress_chunk(xpng_ptr, comp_type, chunkdata,
                                    (xpng_size_t)length, prefix_len, &data_len);

   text_ptr = (xpng_textp)xpng_malloc_warn(xpng_ptr, (xpng_uint_32)sizeof(xpng_text));
   if (text_ptr == NULL)
   {
     xpng_warning(xpng_ptr,"Not enough memory to process zTXt chunk.");
     xpng_free(xpng_ptr, chunkdata);
     return;
   }
   text_ptr->compression = comp_type;
   text_ptr->key = chunkdata;
#ifdef PNG_iTXt_SUPPORTED
   text_ptr->lang = NULL;
   text_ptr->lang_key = NULL;
   text_ptr->itxt_length = 0;
#endif
   text_ptr->text = chunkdata + prefix_len;
   text_ptr->text_length = data_len;

   ret=xpng_set_text_2(xpng_ptr, info_ptr, text_ptr, 1);

   xpng_free(xpng_ptr, text_ptr);
   xpng_free(xpng_ptr, chunkdata);
   if (ret)
     xpng_error(xpng_ptr, "Insufficient memory to store zTXt chunk.");
}
#endif

#if defined(PNG_READ_iTXt_SUPPORTED)
/* note: this does not correctly handle chunks that are > 64K under DOS */
void /* PRIVATE */
xpng_handle_iTXt(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_textp text_ptr;
   xpng_charp chunkdata;
   xpng_charp key, lang, text, lang_key;
   int comp_flag;
   int comp_type = 0;
   int ret;
   xpng_size_t slength, prefix_len, data_len;

   xpng_debug(1, "in xpng_handle_iTXt\n");

   if (!(xpng_ptr->mode & PNG_HAVE_IHDR))
      xpng_error(xpng_ptr, "Missing IHDR before iTXt");

   if (xpng_ptr->mode & PNG_HAVE_IDAT)
      xpng_ptr->mode |= PNG_AFTER_IDAT;

#ifdef PNG_MAX_MALLOC_64K
   /* We will no doubt have problems with chunks even half this size, but
      there is no hard and fast rule to tell us where to stop. */
   if (length > (xpng_uint_32)65535L)
   {
     xpng_warning(xpng_ptr,"iTXt chunk too large to fit in memory");
     xpng_crc_finish(xpng_ptr, length);
     return;
   }
#endif

   chunkdata = (xpng_charp)xpng_malloc_warn(xpng_ptr, length + 1);
   if (chunkdata == NULL)
   {
     xpng_warning(xpng_ptr, "No memory to process iTXt chunk.");
     return;
   }
   slength = (xpng_size_t)length;
   xpng_crc_read(xpng_ptr, (xpng_bytep)chunkdata, slength);
   if (xpng_crc_finish(xpng_ptr, 0))
   {
      xpng_free(xpng_ptr, chunkdata);
      return;
   }

   chunkdata[slength] = 0x00;

   for (lang = chunkdata; *lang; lang++)
      /* empty loop */ ;
   lang++;        /* skip NUL separator */

   /* iTXt must have a language tag (possibly empty), two compression bytes,
      translated keyword (possibly empty), and possibly some text after the
      keyword */

   if (lang >= chunkdata + slength)
   {
      comp_flag = PNG_TEXT_COMPRESSION_NONE;
      xpng_warning(xpng_ptr, "Zero length iTXt chunk");
   }
   else
   {
       comp_flag = *lang++;
       comp_type = *lang++;
   }

   for (lang_key = lang; *lang_key; lang_key++)
      /* empty loop */ ;
   lang_key++;        /* skip NUL separator */

   for (text = lang_key; *text; text++)
      /* empty loop */ ;
   text++;        /* skip NUL separator */

   prefix_len = text - chunkdata;

   key=chunkdata;
   if (comp_flag)
       chunkdata = xpng_decompress_chunk(xpng_ptr, comp_type, chunkdata,
          (size_t)length, prefix_len, &data_len);
   else
       data_len=xpng_strlen(chunkdata + prefix_len);
   text_ptr = (xpng_textp)xpng_malloc_warn(xpng_ptr, (xpng_uint_32)sizeof(xpng_text));
   if (text_ptr == NULL)
   {
     xpng_warning(xpng_ptr,"Not enough memory to process iTXt chunk.");
     xpng_free(xpng_ptr, chunkdata);
     return;
   }
   text_ptr->compression = (int)comp_flag + 1;
   text_ptr->lang_key = chunkdata+(lang_key-key);
   text_ptr->lang = chunkdata+(lang-key);
   text_ptr->itxt_length = data_len;
   text_ptr->text_length = 0;
   text_ptr->key = chunkdata;
   text_ptr->text = chunkdata + prefix_len;

   ret=xpng_set_text_2(xpng_ptr, info_ptr, text_ptr, 1);

   xpng_free(xpng_ptr, text_ptr);
   xpng_free(xpng_ptr, chunkdata);
   if (ret)
     xpng_error(xpng_ptr, "Insufficient memory to store iTXt chunk.");
}
#endif

/* This function is called when we haven't found a handler for a
   chunk.  If there isn't a problem with the chunk itself (ie bad
   chunk name, CRC, or a critical chunk), the chunk is silently ignored
   -- unless the PNG_FLAG_UNKNOWN_CHUNKS_SUPPORTED flag is on in which
   case it will be saved away to be written out later. */
void /* PRIVATE */
xpng_handle_unknown(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 length)
{
   xpng_uint_32 skip = 0;

   xpng_debug(1, "in xpng_handle_unknown\n");

   if (xpng_ptr->mode & PNG_HAVE_IDAT)
   {
#ifdef PNG_USE_LOCAL_ARRAYS
      PNG_IDAT;
#endif
      if (xpng_memcmp(xpng_ptr->chunk_name, xpng_IDAT, 4))  /* not an IDAT */
         xpng_ptr->mode |= PNG_AFTER_IDAT;
   }

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
       chunk.size = (xpng_size_t)length;
       xpng_crc_read(xpng_ptr, (xpng_bytep)chunk.data, length);
#if defined(PNG_READ_USER_CHUNKS_SUPPORTED)
       if(xpng_ptr->read_user_chunk_fn != NULL)
       {
          /* callback to user unknown chunk handler */
          if ((*(xpng_ptr->read_user_chunk_fn)) (xpng_ptr, &chunk) <= 0)
          {
             if (!(xpng_ptr->chunk_name[0] & 0x20))
                if(xpng_handle_as_unknown(xpng_ptr, xpng_ptr->chunk_name) !=
                     HANDLE_CHUNK_ALWAYS)
                 {
                   xpng_free(xpng_ptr, chunk.data);
                   xpng_chunk_error(xpng_ptr, "unknown critical chunk");
                 }
             xpng_set_unknown_chunks(xpng_ptr, info_ptr, &chunk, 1);
          }
       }
       else
#endif
          xpng_set_unknown_chunks(xpng_ptr, info_ptr, &chunk, 1);
       xpng_free(xpng_ptr, chunk.data);
   }
   else
#endif
      skip = length;

   xpng_crc_finish(xpng_ptr, skip);

#if !defined(PNG_READ_USER_CHUNKS_SUPPORTED)
   info_ptr = info_ptr; /* quiet compiler warnings about unused info_ptr */
#endif
}

/* This function is called to verify that a chunk name is valid.
   This function can't have the "critical chunk check" incorporated
   into it, since in the future we will need to be able to call user
   functions to handle unknown critical chunks after we check that
   the chunk name itself is valid. */

#define isnonalpha(c) ((c) < 41 || (c) > 122 || ((c) > 90 && (c) < 97))

void /* PRIVATE */
xpng_check_chunk_name(xpng_structp xpng_ptr, xpng_bytep chunk_name)
{
   xpng_debug(1, "in xpng_check_chunk_name\n");
   if (isnonalpha(chunk_name[0]) || isnonalpha(chunk_name[1]) ||
       isnonalpha(chunk_name[2]) || isnonalpha(chunk_name[3]))
   {
      xpng_chunk_error(xpng_ptr, "invalid chunk type");
   }
}

/* Combines the row recently read in with the existing pixels in the
   row.  This routine takes care of alpha and transparency if requested.
   This routine also handles the two methods of progressive display
   of interlaced images, depending on the mask value.
   The mask value describes which pixels are to be combined with
   the row.  The pattern always repeats every 8 pixels, so just 8
   bits are needed.  A one indicates the pixel is to be combined,
   a zero indicates the pixel is to be skipped.  This is in addition
   to any alpha or transparency value associated with the pixel.  If
   you want all pixels to be combined, pass 0xff (255) in mask.  */
#ifndef PNG_HAVE_ASSEMBLER_COMBINE_ROW
void /* PRIVATE */
xpng_combine_row(xpng_structp xpng_ptr, xpng_bytep row, int mask)
{
   xpng_debug(1,"in xpng_combine_row\n");
   if (mask == 0xff)
   {
      xpng_memcpy(row, xpng_ptr->row_buf + 1,
         (xpng_size_t)((xpng_ptr->width *
         xpng_ptr->row_info.pixel_depth + 7) >> 3));
   }
   else
   {
      switch (xpng_ptr->row_info.pixel_depth)
      {
         case 1:
         {
            xpng_bytep sp = xpng_ptr->row_buf + 1;
            xpng_bytep dp = row;
            int s_inc, s_start, s_end;
            int m = 0x80;
            int shift;
            xpng_uint_32 i;
            xpng_uint_32 row_width = xpng_ptr->width;

#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (xpng_ptr->transformations & PNG_PACKSWAP)
            {
                s_start = 0;
                s_end = 7;
                s_inc = 1;
            }
            else
#endif
            {
                s_start = 7;
                s_end = 0;
                s_inc = -1;
            }

            shift = s_start;

            for (i = 0; i < row_width; i++)
            {
               if (m & mask)
               {
                  int value;

                  value = (*sp >> shift) & 0x01;
                  *dp &= (xpng_byte)((0x7f7f >> (7 - shift)) & 0xff);
                  *dp |= (xpng_byte)(value << shift);
               }

               if (shift == s_end)
               {
                  shift = s_start;
                  sp++;
                  dp++;
               }
               else
                  shift += s_inc;

               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }
         case 2:
         {
            xpng_bytep sp = xpng_ptr->row_buf + 1;
            xpng_bytep dp = row;
            int s_start, s_end, s_inc;
            int m = 0x80;
            int shift;
            xpng_uint_32 i;
            xpng_uint_32 row_width = xpng_ptr->width;
            int value;

#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (xpng_ptr->transformations & PNG_PACKSWAP)
            {
               s_start = 0;
               s_end = 6;
               s_inc = 2;
            }
            else
#endif
            {
               s_start = 6;
               s_end = 0;
               s_inc = -2;
            }

            shift = s_start;

            for (i = 0; i < row_width; i++)
            {
               if (m & mask)
               {
                  value = (*sp >> shift) & 0x03;
                  *dp &= (xpng_byte)((0x3f3f >> (6 - shift)) & 0xff);
                  *dp |= (xpng_byte)(value << shift);
               }

               if (shift == s_end)
               {
                  shift = s_start;
                  sp++;
                  dp++;
               }
               else
                  shift += s_inc;
               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }
         case 4:
         {
            xpng_bytep sp = xpng_ptr->row_buf + 1;
            xpng_bytep dp = row;
            int s_start, s_end, s_inc;
            int m = 0x80;
            int shift;
            xpng_uint_32 i;
            xpng_uint_32 row_width = xpng_ptr->width;
            int value;

#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (xpng_ptr->transformations & PNG_PACKSWAP)
            {
               s_start = 0;
               s_end = 4;
               s_inc = 4;
            }
            else
#endif
            {
               s_start = 4;
               s_end = 0;
               s_inc = -4;
            }
            shift = s_start;

            for (i = 0; i < row_width; i++)
            {
               if (m & mask)
               {
                  value = (*sp >> shift) & 0xf;
                  *dp &= (xpng_byte)((0xf0f >> (4 - shift)) & 0xff);
                  *dp |= (xpng_byte)(value << shift);
               }

               if (shift == s_end)
               {
                  shift = s_start;
                  sp++;
                  dp++;
               }
               else
                  shift += s_inc;
               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }
         default:
         {
            xpng_bytep sp = xpng_ptr->row_buf + 1;
            xpng_bytep dp = row;
            xpng_size_t pixel_bytes = (xpng_ptr->row_info.pixel_depth >> 3);
            xpng_uint_32 i;
            xpng_uint_32 row_width = xpng_ptr->width;
            xpng_byte m = 0x80;


            for (i = 0; i < row_width; i++)
            {
               if (m & mask)
               {
                  xpng_memcpy(dp, sp, pixel_bytes);
               }

               sp += pixel_bytes;
               dp += pixel_bytes;

               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }
      }
   }
}
#endif /* !PNG_HAVE_ASSEMBLER_COMBINE_ROW */

#ifdef PNG_READ_INTERLACING_SUPPORTED
#ifndef PNG_HAVE_ASSEMBLER_READ_INTERLACE   /* else in pngvcrd.c, pnggccrd.c */
/* OLD pre-1.0.9 interface:
void xpng_do_read_interlace(xpng_row_infop row_info, xpng_bytep row, int pass,
   xpng_uint_32 transformations)
 */
void /* PRIVATE */
xpng_do_read_interlace(xpng_structp xpng_ptr)
{
   xpng_row_infop row_info = &(xpng_ptr->row_info);
   xpng_bytep row = xpng_ptr->row_buf + 1;
   int pass = xpng_ptr->pass;
   xpng_uint_32 transformations = xpng_ptr->transformations;
#ifdef PNG_USE_LOCAL_ARRAYS
   /* arrays to facilitate easy interlacing - use pass (0 - 6) as index */
   /* offset to next interlace block */
   const int xpng_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};
#endif

   xpng_debug(1,"in xpng_do_read_interlace (stock C version)\n");
   if (row != NULL && row_info != NULL)
   {
      xpng_uint_32 final_width;

      final_width = row_info->width * xpng_pass_inc[pass];

      switch (row_info->pixel_depth)
      {
         case 1:
         {
            xpng_bytep sp = row + (xpng_size_t)((row_info->width - 1) >> 3);
            xpng_bytep dp = row + (xpng_size_t)((final_width - 1) >> 3);
            int sshift, dshift;
            int s_start, s_end, s_inc;
            int jstop = xpng_pass_inc[pass];
            xpng_byte v;
            xpng_uint_32 i;
            int j;

#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (transformations & PNG_PACKSWAP)
            {
                sshift = (int)((row_info->width + 7) & 0x07);
                dshift = (int)((final_width + 7) & 0x07);
                s_start = 7;
                s_end = 0;
                s_inc = -1;
            }
            else
#endif
            {
                sshift = 7 - (int)((row_info->width + 7) & 0x07);
                dshift = 7 - (int)((final_width + 7) & 0x07);
                s_start = 0;
                s_end = 7;
                s_inc = 1;
            }

            for (i = 0; i < row_info->width; i++)
            {
               v = (xpng_byte)((*sp >> sshift) & 0x01);
               for (j = 0; j < jstop; j++)
               {
                  *dp &= (xpng_byte)((0x7f7f >> (7 - dshift)) & 0xff);
                  *dp |= (xpng_byte)(v << dshift);
                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }
                  else
                     dshift += s_inc;
               }
               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }
               else
                  sshift += s_inc;
            }
            break;
         }
         case 2:
         {
            xpng_bytep sp = row + (xpng_uint_32)((row_info->width - 1) >> 2);
            xpng_bytep dp = row + (xpng_uint_32)((final_width - 1) >> 2);
            int sshift, dshift;
            int s_start, s_end, s_inc;
            int jstop = xpng_pass_inc[pass];
            xpng_uint_32 i;

#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (transformations & PNG_PACKSWAP)
            {
               sshift = (int)(((row_info->width + 3) & 0x03) << 1);
               dshift = (int)(((final_width + 3) & 0x03) << 1);
               s_start = 6;
               s_end = 0;
               s_inc = -2;
            }
            else
#endif
            {
               sshift = (int)((3 - ((row_info->width + 3) & 0x03)) << 1);
               dshift = (int)((3 - ((final_width + 3) & 0x03)) << 1);
               s_start = 0;
               s_end = 6;
               s_inc = 2;
            }

            for (i = 0; i < row_info->width; i++)
            {
               xpng_byte v;
               int j;

               v = (xpng_byte)((*sp >> sshift) & 0x03);
               for (j = 0; j < jstop; j++)
               {
                  *dp &= (xpng_byte)((0x3f3f >> (6 - dshift)) & 0xff);
                  *dp |= (xpng_byte)(v << dshift);
                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }
                  else
                     dshift += s_inc;
               }
               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }
               else
                  sshift += s_inc;
            }
            break;
         }
         case 4:
         {
            xpng_bytep sp = row + (xpng_size_t)((row_info->width - 1) >> 1);
            xpng_bytep dp = row + (xpng_size_t)((final_width - 1) >> 1);
            int sshift, dshift;
            int s_start, s_end, s_inc;
            xpng_uint_32 i;
            int jstop = xpng_pass_inc[pass];

#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (transformations & PNG_PACKSWAP)
            {
               sshift = (int)(((row_info->width + 1) & 0x01) << 2);
               dshift = (int)(((final_width + 1) & 0x01) << 2);
               s_start = 4;
               s_end = 0;
               s_inc = -4;
            }
            else
#endif
            {
               sshift = (int)((1 - ((row_info->width + 1) & 0x01)) << 2);
               dshift = (int)((1 - ((final_width + 1) & 0x01)) << 2);
               s_start = 0;
               s_end = 4;
               s_inc = 4;
            }

            for (i = 0; i < row_info->width; i++)
            {
               xpng_byte v = (xpng_byte)((*sp >> sshift) & 0xf);
               int j;

               for (j = 0; j < jstop; j++)
               {
                  *dp &= (xpng_byte)((0xf0f >> (4 - dshift)) & 0xff);
                  *dp |= (xpng_byte)(v << dshift);
                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }
                  else
                     dshift += s_inc;
               }
               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }
               else
                  sshift += s_inc;
            }
            break;
         }
         default:
         {
            xpng_size_t pixel_bytes = (row_info->pixel_depth >> 3);
            xpng_bytep sp = row + (xpng_size_t)(row_info->width - 1) * pixel_bytes;
            xpng_bytep dp = row + (xpng_size_t)(final_width - 1) * pixel_bytes;

            int jstop = xpng_pass_inc[pass];
            xpng_uint_32 i;

            for (i = 0; i < row_info->width; i++)
            {
               xpng_byte v[8];
               int j;

               xpng_memcpy(v, sp, pixel_bytes);
               for (j = 0; j < jstop; j++)
               {
                  xpng_memcpy(dp, v, pixel_bytes);
                  dp -= pixel_bytes;
               }
               sp -= pixel_bytes;
            }
            break;
         }
      }
      row_info->width = final_width;
      row_info->rowbytes = ((final_width *
         (xpng_uint_32)row_info->pixel_depth + 7) >> 3);
   }
#if !defined(PNG_READ_PACKSWAP_SUPPORTED)
   transformations = transformations; /* silence compiler warning */
#endif
}
#endif /* !PNG_HAVE_ASSEMBLER_READ_INTERLACE */
#endif /* PNG_READ_INTERLACING_SUPPORTED */

#ifndef PNG_HAVE_ASSEMBLER_READ_FILTER_ROW
void /* PRIVATE */
xpng_read_filter_row(xpng_structp xpng_ptr, xpng_row_infop row_info, xpng_bytep row,
   xpng_bytep prev_row, int filter)
{
   xpng_debug(1, "in xpng_read_filter_row\n");
   xpng_debug2(2,"row = %lu, filter = %d\n", xpng_ptr->row_number, filter);
   switch (filter)
   {
      case PNG_FILTER_VALUE_NONE:
         break;
      case PNG_FILTER_VALUE_SUB:
      {
         xpng_uint_32 i;
         xpng_uint_32 istop = row_info->rowbytes;
         xpng_uint_32 bpp = (row_info->pixel_depth + 7) >> 3;
         xpng_bytep rp = row + bpp;
         xpng_bytep lp = row;

         for (i = bpp; i < istop; i++)
         {
            *rp = (xpng_byte)(((int)(*rp) + (int)(*lp++)) & 0xff);
            rp++;
         }
         break;
      }
      case PNG_FILTER_VALUE_UP:
      {
         xpng_uint_32 i;
         xpng_uint_32 istop = row_info->rowbytes;
         xpng_bytep rp = row;
         xpng_bytep pp = prev_row;

         for (i = 0; i < istop; i++)
         {
            *rp = (xpng_byte)(((int)(*rp) + (int)(*pp++)) & 0xff);
            rp++;
         }
         break;
      }
      case PNG_FILTER_VALUE_AVG:
      {
         xpng_uint_32 i;
         xpng_bytep rp = row;
         xpng_bytep pp = prev_row;
         xpng_bytep lp = row;
         xpng_uint_32 bpp = (row_info->pixel_depth + 7) >> 3;
         xpng_uint_32 istop = row_info->rowbytes - bpp;

         for (i = 0; i < bpp; i++)
         {
            *rp = (xpng_byte)(((int)(*rp) +
               ((int)(*pp++) / 2 )) & 0xff);
            rp++;
         }

         for (i = 0; i < istop; i++)
         {
            *rp = (xpng_byte)(((int)(*rp) +
               (int)(*pp++ + *lp++) / 2 ) & 0xff);
            rp++;
         }
         break;
      }
      case PNG_FILTER_VALUE_PAETH:
      {
         xpng_uint_32 i;
         xpng_bytep rp = row;
         xpng_bytep pp = prev_row;
         xpng_bytep lp = row;
         xpng_bytep cp = prev_row;
         xpng_uint_32 bpp = (row_info->pixel_depth + 7) >> 3;
         xpng_uint_32 istop=row_info->rowbytes - bpp;

         for (i = 0; i < bpp; i++)
         {
            *rp = (xpng_byte)(((int)(*rp) + (int)(*pp++)) & 0xff);
            rp++;
         }

         for (i = 0; i < istop; i++)   /* use leftover rp,pp */
         {
            int a, b, c, pa, pb, pc, p;

            a = *lp++;
            b = *pp++;
            c = *cp++;

            p = b - c;
            pc = a - c;

#ifdef PNG_USE_ABS
            pa = abs(p);
            pb = abs(pc);
            pc = abs(p + pc);
#else
            pa = p < 0 ? -p : p;
            pb = pc < 0 ? -pc : pc;
            pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif

            /*
               if (pa <= pb && pa <= pc)
                  p = a;
               else if (pb <= pc)
                  p = b;
               else
                  p = c;
             */

            p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;

            *rp = (xpng_byte)(((int)(*rp) + p) & 0xff);
            rp++;
         }
         break;
      }
      default:
         xpng_warning(xpng_ptr, "Ignoring bad adaptive filter type");
         *row=0;
         break;
   }
}
#endif /* !PNG_HAVE_ASSEMBLER_READ_FILTER_ROW */

void /* PRIVATE */
xpng_read_finish_row(xpng_structp xpng_ptr)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   /* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

   /* start of interlace block */
   const int xpng_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   /* offset to next interlace block */
   const int xpng_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   /* start of interlace block in the y direction */
   const int xpng_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   /* offset to next interlace block in the y direction */
   const int xpng_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};
#endif

   xpng_debug(1, "in xpng_read_finish_row\n");
   xpng_ptr->row_number++;
   if (xpng_ptr->row_number < xpng_ptr->num_rows)
      return;

   if (xpng_ptr->interlaced)
   {
      xpng_ptr->row_number = 0;
      xpng_memset_check(xpng_ptr, xpng_ptr->prev_row, 0, xpng_ptr->rowbytes + 1);
      do
      {
         xpng_ptr->pass++;
         if (xpng_ptr->pass >= 7)
            break;
         xpng_ptr->iwidth = (xpng_ptr->width +
            xpng_pass_inc[xpng_ptr->pass] - 1 -
            xpng_pass_start[xpng_ptr->pass]) /
            xpng_pass_inc[xpng_ptr->pass];
            xpng_ptr->irowbytes = ((xpng_ptr->iwidth *
               (xpng_uint_32)xpng_ptr->pixel_depth + 7) >> 3) +1;

         if (!(xpng_ptr->transformations & PNG_INTERLACE))
         {
            xpng_ptr->num_rows = (xpng_ptr->height +
               xpng_pass_yinc[xpng_ptr->pass] - 1 -
               xpng_pass_ystart[xpng_ptr->pass]) /
               xpng_pass_yinc[xpng_ptr->pass];
            if (!(xpng_ptr->num_rows))
               continue;
         }
         else  /* if (xpng_ptr->transformations & PNG_INTERLACE) */
            break;
      } while (xpng_ptr->iwidth == 0);

      if (xpng_ptr->pass < 7)
         return;
   }

   if (!(xpng_ptr->flags & PNG_FLAG_ZLIB_FINISHED))
   {
#ifdef PNG_USE_LOCAL_ARRAYS
      PNG_IDAT;
#endif
      char extra;
      int ret;

      xpng_ptr->zstream.next_out = (Byte *)&extra;
      xpng_ptr->zstream.avail_out = (uInt)1;
      for(;;)
      {
         if (!(xpng_ptr->zstream.avail_in))
         {
            while (!xpng_ptr->idat_size)
            {
               xpng_byte chunk_length[4];

               xpng_crc_finish(xpng_ptr, 0);

               xpng_read_data(xpng_ptr, chunk_length, 4);
               xpng_ptr->idat_size = xpng_get_uint_32(chunk_length);

               xpng_reset_crc(xpng_ptr);
               xpng_crc_read(xpng_ptr, xpng_ptr->chunk_name, 4);
               if (xpng_memcmp(xpng_ptr->chunk_name, (xpng_bytep)xpng_IDAT, 4))
                  xpng_error(xpng_ptr, "Not enough image data");

            }
            xpng_ptr->zstream.avail_in = (uInt)xpng_ptr->zbuf_size;
            xpng_ptr->zstream.next_in = xpng_ptr->zbuf;
            if (xpng_ptr->zbuf_size > xpng_ptr->idat_size)
               xpng_ptr->zstream.avail_in = (uInt)xpng_ptr->idat_size;
            xpng_crc_read(xpng_ptr, xpng_ptr->zbuf, xpng_ptr->zstream.avail_in);
            xpng_ptr->idat_size -= xpng_ptr->zstream.avail_in;
         }
         ret = inflate(&xpng_ptr->zstream, Z_PARTIAL_FLUSH);
         if (ret == Z_STREAM_END)
         {
            if (!(xpng_ptr->zstream.avail_out) || xpng_ptr->zstream.avail_in ||
               xpng_ptr->idat_size)
               xpng_warning(xpng_ptr, "Extra compressed data");
            xpng_ptr->mode |= PNG_AFTER_IDAT;
            xpng_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
            break;
         }
         if (ret != Z_OK)
            xpng_error(xpng_ptr, xpng_ptr->zstream.msg ? xpng_ptr->zstream.msg :
                      "Decompression Error");

         if (!(xpng_ptr->zstream.avail_out))
         {
            xpng_warning(xpng_ptr, "Extra compressed data.");
            xpng_ptr->mode |= PNG_AFTER_IDAT;
            xpng_ptr->flags |= PNG_FLAG_ZLIB_FINISHED;
            break;
         }

      }
      xpng_ptr->zstream.avail_out = 0;
   }

   if (xpng_ptr->idat_size || xpng_ptr->zstream.avail_in)
      xpng_warning(xpng_ptr, "Extra compression data");

   inflateReset(&xpng_ptr->zstream);

   xpng_ptr->mode |= PNG_AFTER_IDAT;
}

void /* PRIVATE */
xpng_read_start_row(xpng_structp xpng_ptr)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   /* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

   /* start of interlace block */
   const int xpng_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};

   /* offset to next interlace block */
   const int xpng_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};

   /* start of interlace block in the y direction */
   const int xpng_pass_ystart[7] = {0, 0, 4, 0, 2, 0, 1};

   /* offset to next interlace block in the y direction */
   const int xpng_pass_yinc[7] = {8, 8, 8, 4, 4, 2, 2};
#endif

   int max_pixel_depth;
   xpng_uint_32 row_bytes;

   xpng_debug(1, "in xpng_read_start_row\n");
   xpng_ptr->zstream.avail_in = 0;
   xpng_init_read_transformations(xpng_ptr);
   if (xpng_ptr->interlaced)
   {
      if (!(xpng_ptr->transformations & PNG_INTERLACE))
         xpng_ptr->num_rows = (xpng_ptr->height + xpng_pass_yinc[0] - 1 -
            xpng_pass_ystart[0]) / xpng_pass_yinc[0];
      else
         xpng_ptr->num_rows = xpng_ptr->height;

      xpng_ptr->iwidth = (xpng_ptr->width +
         xpng_pass_inc[xpng_ptr->pass] - 1 -
         xpng_pass_start[xpng_ptr->pass]) /
         xpng_pass_inc[xpng_ptr->pass];

         row_bytes = ((xpng_ptr->iwidth *
            (xpng_uint_32)xpng_ptr->pixel_depth + 7) >> 3) +1;
         xpng_ptr->irowbytes = (xpng_size_t)row_bytes;
         if((xpng_uint_32)xpng_ptr->irowbytes != row_bytes)
            xpng_error(xpng_ptr, "Rowbytes overflow in xpng_read_start_row");
   }
   else
   {
      xpng_ptr->num_rows = xpng_ptr->height;
      xpng_ptr->iwidth = xpng_ptr->width;
      xpng_ptr->irowbytes = xpng_ptr->rowbytes + 1;
   }
   max_pixel_depth = xpng_ptr->pixel_depth;

#if defined(PNG_READ_PACK_SUPPORTED)
   if ((xpng_ptr->transformations & PNG_PACK) && xpng_ptr->bit_depth < 8)
      max_pixel_depth = 8;
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
   if (xpng_ptr->transformations & PNG_EXPAND)
   {
      if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         if (xpng_ptr->num_trans)
            max_pixel_depth = 32;
         else
            max_pixel_depth = 24;
      }
      else if (xpng_ptr->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (max_pixel_depth < 8)
            max_pixel_depth = 8;
         if (xpng_ptr->num_trans)
            max_pixel_depth *= 2;
      }
      else if (xpng_ptr->color_type == PNG_COLOR_TYPE_RGB)
      {
         if (xpng_ptr->num_trans)
         {
            max_pixel_depth *= 4;
            max_pixel_depth /= 3;
         }
      }
   }
#endif

#if defined(PNG_READ_FILLER_SUPPORTED)
   if (xpng_ptr->transformations & (PNG_FILLER))
   {
      if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
         max_pixel_depth = 32;
      else if (xpng_ptr->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (max_pixel_depth <= 8)
            max_pixel_depth = 16;
         else
            max_pixel_depth = 32;
      }
      else if (xpng_ptr->color_type == PNG_COLOR_TYPE_RGB)
      {
         if (max_pixel_depth <= 32)
            max_pixel_depth = 32;
         else
            max_pixel_depth = 64;
      }
   }
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
   if (xpng_ptr->transformations & PNG_GRAY_TO_RGB)
   {
      if (
#if defined(PNG_READ_EXPAND_SUPPORTED)
        (xpng_ptr->num_trans && (xpng_ptr->transformations & PNG_EXPAND)) ||
#endif
#if defined(PNG_READ_FILLER_SUPPORTED)
        (xpng_ptr->transformations & (PNG_FILLER)) ||
#endif
        xpng_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         if (max_pixel_depth <= 16)
            max_pixel_depth = 32;
         else
            max_pixel_depth = 64;
      }
      else
      {
         if (max_pixel_depth <= 8)
           {
             if (xpng_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
               max_pixel_depth = 32;
             else
               max_pixel_depth = 24;
           }
         else if (xpng_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            max_pixel_depth = 64;
         else
            max_pixel_depth = 48;
      }
   }
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) && \
defined(PNG_USER_TRANSFORM_PTR_SUPPORTED)
   if(xpng_ptr->transformations & PNG_USER_TRANSFORM)
     {
       int user_pixel_depth=xpng_ptr->user_transform_depth*
         xpng_ptr->user_transform_channels;
       if(user_pixel_depth > max_pixel_depth)
         max_pixel_depth=user_pixel_depth;
     }
#endif

   /* align the width on the next larger 8 pixels.  Mainly used
      for interlacing */
   row_bytes = ((xpng_ptr->width + 7) & ~((xpng_uint_32)7));
   /* calculate the maximum bytes needed, adding a byte and a pixel
      for safety's sake */
   row_bytes = ((row_bytes * (xpng_uint_32)max_pixel_depth + 7) >> 3) +
      1 + ((max_pixel_depth + 7) >> 3);
#ifdef PNG_MAX_MALLOC_64K
   if (row_bytes > (xpng_uint_32)65536L)
      xpng_error(xpng_ptr, "This image requires a row greater than 64KB");
#endif
   xpng_ptr->big_row_buf = (xpng_bytep)xpng_malloc(xpng_ptr, row_bytes+64);
   xpng_ptr->row_buf = xpng_ptr->big_row_buf+32;
#if defined(PNG_DEBUG) && defined(PNG_USE_PNGGCCRD)
   xpng_ptr->row_buf_size = row_bytes;
#endif

#ifdef PNG_MAX_MALLOC_64K
   if ((xpng_uint_32)xpng_ptr->rowbytes + 1 > (xpng_uint_32)65536L)
      xpng_error(xpng_ptr, "This image requires a row greater than 64KB");
#endif
   xpng_ptr->prev_row = (xpng_bytep)xpng_malloc(xpng_ptr, (xpng_uint_32)(
      xpng_ptr->rowbytes + 1));

   xpng_memset_check(xpng_ptr, xpng_ptr->prev_row, 0, xpng_ptr->rowbytes + 1);

   xpng_debug1(3, "width = %lu,\n", xpng_ptr->width);
   xpng_debug1(3, "height = %lu,\n", xpng_ptr->height);
   xpng_debug1(3, "iwidth = %lu,\n", xpng_ptr->iwidth);
   xpng_debug1(3, "num_rows = %lu\n", xpng_ptr->num_rows);
   xpng_debug1(3, "rowbytes = %lu,\n", xpng_ptr->rowbytes);
   xpng_debug1(3, "irowbytes = %lu,\n", xpng_ptr->irowbytes);

   xpng_ptr->flags |= PNG_FLAG_ROW_INIT;
}
