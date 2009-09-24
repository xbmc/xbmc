// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngset.c - storage of image information into info struct
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * The functions here are used during reads to store data from the file
 * into the info struct, and during writes to store application data
 * into the info struct for writing into the file.  This abstracts the
 * info struct and allows us to change the structure in the future.
 */

#define PNG_INTERNAL
#include "png.h"

#if defined(PNG_bKGD_SUPPORTED)
void PNGAPI
xpng_set_bKGD(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_color_16p background)
{
   xpng_debug1(1, "in %s storage function\n", "bKGD");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   xpng_memcpy(&(info_ptr->background), background, sizeof(xpng_color_16));
   info_ptr->valid |= PNG_INFO_bKGD;
}
#endif

#if defined(PNG_cHRM_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
xpng_set_cHRM(xpng_structp xpng_ptr, xpng_infop info_ptr,
   double white_x, double white_y, double red_x, double red_y,
   double green_x, double green_y, double blue_x, double blue_y)
{
   xpng_debug1(1, "in %s storage function\n", "cHRM");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   if (white_x < 0.0 || white_y < 0.0 ||
         red_x < 0.0 ||   red_y < 0.0 ||
       green_x < 0.0 || green_y < 0.0 ||
        blue_x < 0.0 ||  blue_y < 0.0)
   {
      xpng_warning(xpng_ptr,
        "Ignoring attempt to set negative chromaticity value");
      return;
   }
   if (white_x > 21474.83 || white_y > 21474.83 ||
         red_x > 21474.83 ||   red_y > 21474.83 ||
       green_x > 21474.83 || green_y > 21474.83 ||
        blue_x > 21474.83 ||  blue_y > 21474.83)
   {
      xpng_warning(xpng_ptr,
        "Ignoring attempt to set chromaticity value exceeding 21474.83");
      return;
   }

   info_ptr->x_white = (float)white_x;
   info_ptr->y_white = (float)white_y;
   info_ptr->x_red   = (float)red_x;
   info_ptr->y_red   = (float)red_y;
   info_ptr->x_green = (float)green_x;
   info_ptr->y_green = (float)green_y;
   info_ptr->x_blue  = (float)blue_x;
   info_ptr->y_blue  = (float)blue_y;
#ifdef PNG_FIXED_POINT_SUPPORTED
   info_ptr->int_x_white = (xpng_fixed_point)(white_x*100000.+0.5);
   info_ptr->int_y_white = (xpng_fixed_point)(white_y*100000.+0.5);
   info_ptr->int_x_red   = (xpng_fixed_point)(  red_x*100000.+0.5);
   info_ptr->int_y_red   = (xpng_fixed_point)(  red_y*100000.+0.5);
   info_ptr->int_x_green = (xpng_fixed_point)(green_x*100000.+0.5);
   info_ptr->int_y_green = (xpng_fixed_point)(green_y*100000.+0.5);
   info_ptr->int_x_blue  = (xpng_fixed_point)( blue_x*100000.+0.5);
   info_ptr->int_y_blue  = (xpng_fixed_point)( blue_y*100000.+0.5);
#endif
   info_ptr->valid |= PNG_INFO_cHRM;
}
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
void PNGAPI
xpng_set_cHRM_fixed(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_fixed_point white_x, xpng_fixed_point white_y, xpng_fixed_point red_x,
   xpng_fixed_point red_y, xpng_fixed_point green_x, xpng_fixed_point green_y,
   xpng_fixed_point blue_x, xpng_fixed_point blue_y)
{
   xpng_debug1(1, "in %s storage function\n", "cHRM");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   if (white_x < 0 || white_y < 0 ||
         red_x < 0 ||   red_y < 0 ||
       green_x < 0 || green_y < 0 ||
        blue_x < 0 ||  blue_y < 0)
   {
      xpng_warning(xpng_ptr,
        "Ignoring attempt to set negative chromaticity value");
      return;
   }
   if (white_x > (double) PNG_MAX_UINT || white_y > (double) PNG_MAX_UINT ||
         red_x > (double) PNG_MAX_UINT ||   red_y > (double) PNG_MAX_UINT ||
       green_x > (double) PNG_MAX_UINT || green_y > (double) PNG_MAX_UINT ||
        blue_x > (double) PNG_MAX_UINT ||  blue_y > (double) PNG_MAX_UINT)
   {
      xpng_warning(xpng_ptr,
        "Ignoring attempt to set chromaticity value exceeding 21474.83");
      return;
   }
   info_ptr->int_x_white = white_x;
   info_ptr->int_y_white = white_y;
   info_ptr->int_x_red   = red_x;
   info_ptr->int_y_red   = red_y;
   info_ptr->int_x_green = green_x;
   info_ptr->int_y_green = green_y;
   info_ptr->int_x_blue  = blue_x;
   info_ptr->int_y_blue  = blue_y;
#ifdef PNG_FLOATING_POINT_SUPPORTED
   info_ptr->x_white = (float)(white_x/100000.);
   info_ptr->y_white = (float)(white_y/100000.);
   info_ptr->x_red   = (float)(  red_x/100000.);
   info_ptr->y_red   = (float)(  red_y/100000.);
   info_ptr->x_green = (float)(green_x/100000.);
   info_ptr->y_green = (float)(green_y/100000.);
   info_ptr->x_blue  = (float)( blue_x/100000.);
   info_ptr->y_blue  = (float)( blue_y/100000.);
#endif
   info_ptr->valid |= PNG_INFO_cHRM;
}
#endif
#endif

#if defined(PNG_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
xpng_set_gAMA(xpng_structp xpng_ptr, xpng_infop info_ptr, double file_gamma)
{
   double gamma;
   xpng_debug1(1, "in %s storage function\n", "gAMA");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   /* Check for overflow */
   if (file_gamma > 21474.83)
   {
      xpng_warning(xpng_ptr, "Limiting gamma to 21474.83");
      gamma=21474.83;
   }
   else
      gamma=file_gamma;
   info_ptr->gamma = (float)gamma;
#ifdef PNG_FIXED_POINT_SUPPORTED
   info_ptr->int_gamma = (int)(gamma*100000.+.5);
#endif
   info_ptr->valid |= PNG_INFO_gAMA;
   if(gamma == 0.0)
      xpng_warning(xpng_ptr, "Setting gamma=0");
}
#endif
void PNGAPI
xpng_set_gAMA_fixed(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_fixed_point
   int_gamma)
{
   xpng_fixed_point gamma;

   xpng_debug1(1, "in %s storage function\n", "gAMA");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   if (int_gamma > (xpng_fixed_point) PNG_MAX_UINT)
   {
     xpng_warning(xpng_ptr, "Limiting gamma to 21474.83");
     gamma=PNG_MAX_UINT;
   }
   else
   {
     if (int_gamma < 0)
     {
       xpng_warning(xpng_ptr, "Setting negative gamma to zero");
       gamma=0;
     }
     else
       gamma=int_gamma;
   }
#ifdef PNG_FLOATING_POINT_SUPPORTED
   info_ptr->gamma = (float)(gamma/100000.);
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   info_ptr->int_gamma = gamma;
#endif
   info_ptr->valid |= PNG_INFO_gAMA;
   if(gamma == 0)
      xpng_warning(xpng_ptr, "Setting gamma=0");
}
#endif

#if defined(PNG_hIST_SUPPORTED)
void PNGAPI
xpng_set_hIST(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_16p hist)
{
   int i;

   xpng_debug1(1, "in %s storage function\n", "hIST");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;
   if (info_ptr->num_palette == 0)
   {
       xpng_warning(xpng_ptr,
          "Palette size 0, hIST allocation skipped.");
       return;
   }

#ifdef PNG_FREE_ME_SUPPORTED
   xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_HIST, 0);
#endif
   /* Changed from info->num_palette to 256 in version 1.2.1 */
   xpng_ptr->hist = (xpng_uint_16p)xpng_malloc_warn(xpng_ptr,
      (xpng_uint_32)(256 * sizeof (xpng_uint_16)));
   if (xpng_ptr->hist == NULL)
     {
       xpng_warning(xpng_ptr, "Insufficient memory for hIST chunk data.");
       return;
     }

   for (i = 0; i < info_ptr->num_palette; i++)
       xpng_ptr->hist[i] = hist[i];
   info_ptr->hist = xpng_ptr->hist;
   info_ptr->valid |= PNG_INFO_hIST;

#ifdef PNG_FREE_ME_SUPPORTED
   info_ptr->free_me |= PNG_FREE_HIST;
#else
   xpng_ptr->flags |= PNG_FLAG_FREE_HIST;
#endif
}
#endif

void PNGAPI
xpng_set_IHDR(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_uint_32 width, xpng_uint_32 height, int bit_depth,
   int color_type, int interlace_type, int compression_type,
   int filter_type)
{
   int rowbytes_per_pixel;
   xpng_debug1(1, "in %s storage function\n", "IHDR");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   /* check for width and height valid values */
   if (width == 0 || height == 0)
      xpng_error(xpng_ptr, "Image width or height is zero in IHDR");
   if (width > PNG_MAX_UINT || height > PNG_MAX_UINT)
      xpng_error(xpng_ptr, "Invalid image size in IHDR");

   /* check other values */
   if (bit_depth != 1 && bit_depth != 2 && bit_depth != 4 &&
      bit_depth != 8 && bit_depth != 16)
      xpng_error(xpng_ptr, "Invalid bit depth in IHDR");

   if (color_type < 0 || color_type == 1 ||
      color_type == 5 || color_type > 6)
      xpng_error(xpng_ptr, "Invalid color type in IHDR");

   if (((color_type == PNG_COLOR_TYPE_PALETTE) && bit_depth > 8) ||
       ((color_type == PNG_COLOR_TYPE_RGB ||
         color_type == PNG_COLOR_TYPE_GRAY_ALPHA ||
         color_type == PNG_COLOR_TYPE_RGB_ALPHA) && bit_depth < 8))
      xpng_error(xpng_ptr, "Invalid color type/bit depth combination in IHDR");

   if (interlace_type >= PNG_INTERLACE_LAST)
      xpng_error(xpng_ptr, "Unknown interlace method in IHDR");

   if (compression_type != PNG_COMPRESSION_TYPE_BASE)
      xpng_error(xpng_ptr, "Unknown compression method in IHDR");

#if defined(PNG_MNG_FEATURES_SUPPORTED)
   /* Accept filter_method 64 (intrapixel differencing) only if
    * 1. Libpng was compiled with PNG_MNG_FEATURES_SUPPORTED and
    * 2. Libpng did not read a PNG signature (this filter_method is only
    *    used in PNG datastreams that are embedded in MNG datastreams) and
    * 3. The application called xpng_permit_mng_features with a mask that
    *    included PNG_FLAG_MNG_FILTER_64 and
    * 4. The filter_method is 64 and
    * 5. The color_type is RGB or RGBA
    */
   if((xpng_ptr->mode&PNG_HAVE_PNG_SIGNATURE)&&xpng_ptr->mng_features_permitted)
      xpng_warning(xpng_ptr,"MNG features are not allowed in a PNG datastream\n");
   if(filter_type != PNG_FILTER_TYPE_BASE)
   {
     if(!((xpng_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
        (filter_type == PNG_INTRAPIXEL_DIFFERENCING) &&
        ((xpng_ptr->mode&PNG_HAVE_PNG_SIGNATURE) == 0) &&
        (color_type == PNG_COLOR_TYPE_RGB || 
         color_type == PNG_COLOR_TYPE_RGB_ALPHA)))
        xpng_error(xpng_ptr, "Unknown filter method in IHDR");
     if(xpng_ptr->mode&PNG_HAVE_PNG_SIGNATURE)
        xpng_warning(xpng_ptr, "Invalid filter method in IHDR");
   }
#else
   if(filter_type != PNG_FILTER_TYPE_BASE)
      xpng_error(xpng_ptr, "Unknown filter method in IHDR");
#endif

   info_ptr->width = width;
   info_ptr->height = height;
   info_ptr->bit_depth = (xpng_byte)bit_depth;
   info_ptr->color_type =(xpng_byte) color_type;
   info_ptr->compression_type = (xpng_byte)compression_type;
   info_ptr->filter_type = (xpng_byte)filter_type;
   info_ptr->interlace_type = (xpng_byte)interlace_type;
   if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      info_ptr->channels = 1;
   else if (info_ptr->color_type & PNG_COLOR_MASK_COLOR)
      info_ptr->channels = 3;
   else
      info_ptr->channels = 1;
   if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA)
      info_ptr->channels++;
   info_ptr->pixel_depth = (xpng_byte)(info_ptr->channels * info_ptr->bit_depth);

   /* check for overflow */
   rowbytes_per_pixel = (info_ptr->pixel_depth + 7) >> 3;
   if ( width > PNG_MAX_UINT/rowbytes_per_pixel - 64)
   {
      xpng_warning(xpng_ptr,
         "Width too large to process image data; rowbytes will overflow.");
      info_ptr->rowbytes = (xpng_size_t)0;
   }
   else
      info_ptr->rowbytes = (info_ptr->width * info_ptr->pixel_depth + 7) >> 3;
}

#if defined(PNG_oFFs_SUPPORTED)
void PNGAPI
xpng_set_oFFs(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_int_32 offset_x, xpng_int_32 offset_y, int unit_type)
{
   xpng_debug1(1, "in %s storage function\n", "oFFs");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_offset = offset_x;
   info_ptr->y_offset = offset_y;
   info_ptr->offset_unit_type = (xpng_byte)unit_type;
   info_ptr->valid |= PNG_INFO_oFFs;
}
#endif

#if defined(PNG_pCAL_SUPPORTED)
void PNGAPI
xpng_set_pCAL(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_charp purpose, xpng_int_32 X0, xpng_int_32 X1, int type, int nparams,
   xpng_charp units, xpng_charpp params)
{
   xpng_uint_32 length;
   int i;

   xpng_debug1(1, "in %s storage function\n", "pCAL");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   length = xpng_strlen(purpose) + 1;
   xpng_debug1(3, "allocating purpose for info (%lu bytes)\n", length);
   info_ptr->pcal_purpose = (xpng_charp)xpng_malloc_warn(xpng_ptr, length);
   if (info_ptr->pcal_purpose == NULL)
     {
       xpng_warning(xpng_ptr, "Insufficient memory for pCAL purpose.");
       return;
     }
   xpng_memcpy(info_ptr->pcal_purpose, purpose, (xpng_size_t)length);

   xpng_debug(3, "storing X0, X1, type, and nparams in info\n");
   info_ptr->pcal_X0 = X0;
   info_ptr->pcal_X1 = X1;
   info_ptr->pcal_type = (xpng_byte)type;
   info_ptr->pcal_nparams = (xpng_byte)nparams;

   length = xpng_strlen(units) + 1;
   xpng_debug1(3, "allocating units for info (%lu bytes)\n", length);
   info_ptr->pcal_units = (xpng_charp)xpng_malloc_warn(xpng_ptr, length);
   if (info_ptr->pcal_units == NULL)
     {
       xpng_warning(xpng_ptr, "Insufficient memory for pCAL units.");
       return;
     }
   xpng_memcpy(info_ptr->pcal_units, units, (xpng_size_t)length);

   info_ptr->pcal_params = (xpng_charpp)xpng_malloc_warn(xpng_ptr,
      (xpng_uint_32)((nparams + 1) * sizeof(xpng_charp)));
   if (info_ptr->pcal_params == NULL)
     {
       xpng_warning(xpng_ptr, "Insufficient memory for pCAL params.");
       return;
     }

   info_ptr->pcal_params[nparams] = NULL;

   for (i = 0; i < nparams; i++)
   {
      length = xpng_strlen(params[i]) + 1;
      xpng_debug2(3, "allocating parameter %d for info (%lu bytes)\n", i, length);
      info_ptr->pcal_params[i] = (xpng_charp)xpng_malloc_warn(xpng_ptr, length);
      if (info_ptr->pcal_params[i] == NULL)
        {
          xpng_warning(xpng_ptr, "Insufficient memory for pCAL parameter.");
          return;
        }
      xpng_memcpy(info_ptr->pcal_params[i], params[i], (xpng_size_t)length);
   }

   info_ptr->valid |= PNG_INFO_pCAL;
#ifdef PNG_FREE_ME_SUPPORTED
   info_ptr->free_me |= PNG_FREE_PCAL;
#endif
}
#endif

#if defined(PNG_READ_sCAL_SUPPORTED) || defined(PNG_WRITE_sCAL_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
xpng_set_sCAL(xpng_structp xpng_ptr, xpng_infop info_ptr,
             int unit, double width, double height)
{
   xpng_debug1(1, "in %s storage function\n", "sCAL");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->scal_unit = (xpng_byte)unit;
   info_ptr->scal_pixel_width = width;
   info_ptr->scal_pixel_height = height;

   info_ptr->valid |= PNG_INFO_sCAL;
}
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
void PNGAPI
xpng_set_sCAL_s(xpng_structp xpng_ptr, xpng_infop info_ptr,
             int unit, xpng_charp swidth, xpng_charp sheight)
{
   xpng_uint_32 length;

   xpng_debug1(1, "in %s storage function\n", "sCAL");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->scal_unit = (xpng_byte)unit;

   length = xpng_strlen(swidth) + 1;
   xpng_debug1(3, "allocating unit for info (%d bytes)\n", length);
   info_ptr->scal_s_width = (xpng_charp)xpng_malloc(xpng_ptr, length);
   xpng_memcpy(info_ptr->scal_s_width, swidth, (xpng_size_t)length);

   length = xpng_strlen(sheight) + 1;
   xpng_debug1(3, "allocating unit for info (%d bytes)\n", length);
   info_ptr->scal_s_height = (xpng_charp)xpng_malloc(xpng_ptr, length);
   xpng_memcpy(info_ptr->scal_s_height, sheight, (xpng_size_t)length);

   info_ptr->valid |= PNG_INFO_sCAL;
#ifdef PNG_FREE_ME_SUPPORTED
   info_ptr->free_me |= PNG_FREE_SCAL;
#endif
}
#endif
#endif
#endif

#if defined(PNG_pHYs_SUPPORTED)
void PNGAPI
xpng_set_pHYs(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_uint_32 res_x, xpng_uint_32 res_y, int unit_type)
{
   xpng_debug1(1, "in %s storage function\n", "pHYs");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_pixels_per_unit = res_x;
   info_ptr->y_pixels_per_unit = res_y;
   info_ptr->phys_unit_type = (xpng_byte)unit_type;
   info_ptr->valid |= PNG_INFO_pHYs;
}
#endif

void PNGAPI
xpng_set_PLTE(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_colorp palette, int num_palette)
{

   xpng_debug1(1, "in %s storage function\n", "PLTE");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   /*
    * It may not actually be necessary to set xpng_ptr->palette here;
    * we do it for backward compatibility with the way the xpng_handle_tRNS
    * function used to do the allocation.
    */
#ifdef PNG_FREE_ME_SUPPORTED
   xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_PLTE, 0);
#endif
   /* Changed in libpng-1.2.1 to allocate 256 instead of num_palette entries,
      in case of an invalid PNG file that has too-large sample values. */
   xpng_ptr->palette = (xpng_colorp)xpng_zalloc(xpng_ptr, (uInt)256,
      sizeof (xpng_color));
   if (xpng_ptr->palette == NULL)
      xpng_error(xpng_ptr, "Unable to malloc palette");
   xpng_memcpy(xpng_ptr->palette, palette, num_palette * sizeof (xpng_color));
   info_ptr->palette = xpng_ptr->palette;
   info_ptr->num_palette = xpng_ptr->num_palette = (xpng_uint_16)num_palette;

#ifdef PNG_FREE_ME_SUPPORTED
   info_ptr->free_me |= PNG_FREE_PLTE;
#else
   xpng_ptr->flags |= PNG_FLAG_FREE_PLTE;
#endif

   info_ptr->valid |= PNG_INFO_PLTE;
}

#if defined(PNG_sBIT_SUPPORTED)
void PNGAPI
xpng_set_sBIT(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_color_8p sig_bit)
{
   xpng_debug1(1, "in %s storage function\n", "sBIT");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   xpng_memcpy(&(info_ptr->sig_bit), sig_bit, sizeof (xpng_color_8));
   info_ptr->valid |= PNG_INFO_sBIT;
}
#endif

#if defined(PNG_sRGB_SUPPORTED)
void PNGAPI
xpng_set_sRGB(xpng_structp xpng_ptr, xpng_infop info_ptr, int intent)
{
   xpng_debug1(1, "in %s storage function\n", "sRGB");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->srgb_intent = (xpng_byte)intent;
   info_ptr->valid |= PNG_INFO_sRGB;
}

void PNGAPI
xpng_set_sRGB_gAMA_and_cHRM(xpng_structp xpng_ptr, xpng_infop info_ptr,
   int intent)
{
#if defined(PNG_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
   float file_gamma;
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   xpng_fixed_point int_file_gamma;
#endif
#endif
#if defined(PNG_cHRM_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
   float white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y;
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   xpng_fixed_point int_white_x, int_white_y, int_red_x, int_red_y, int_green_x,
      int_green_y, int_blue_x, int_blue_y;
#endif
#endif
   xpng_debug1(1, "in %s storage function\n", "sRGB_gAMA_and_cHRM");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   xpng_set_sRGB(xpng_ptr, info_ptr, intent);

#if defined(PNG_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
   file_gamma = (float).45455;
   xpng_set_gAMA(xpng_ptr, info_ptr, file_gamma);
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   int_file_gamma = 45455L;
   xpng_set_gAMA_fixed(xpng_ptr, info_ptr, int_file_gamma);
#endif
#endif

#if defined(PNG_cHRM_SUPPORTED)
#ifdef PNG_FIXED_POINT_SUPPORTED
   int_white_x = 31270L;
   int_white_y = 32900L;
   int_red_x   = 64000L;
   int_red_y   = 33000L;
   int_green_x = 30000L;
   int_green_y = 60000L;
   int_blue_x  = 15000L;
   int_blue_y  =  6000L;

   xpng_set_cHRM_fixed(xpng_ptr, info_ptr,
      int_white_x, int_white_y, int_red_x, int_red_y, int_green_x, int_green_y,
      int_blue_x, int_blue_y);
#endif
#ifdef PNG_FLOATING_POINT_SUPPORTED
   white_x = (float).3127;
   white_y = (float).3290;
   red_x   = (float).64;
   red_y   = (float).33;
   green_x = (float).30;
   green_y = (float).60;
   blue_x  = (float).15;
   blue_y  = (float).06;

   xpng_set_cHRM(xpng_ptr, info_ptr,
      white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y);
#endif
#endif
}
#endif


#if defined(PNG_iCCP_SUPPORTED)
void PNGAPI
xpng_set_iCCP(xpng_structp xpng_ptr, xpng_infop info_ptr,
             xpng_charp name, int compression_type,
             xpng_charp profile, xpng_uint_32 proflen)
{
   xpng_charp new_iccp_name;
   xpng_charp new_iccp_profile;

   xpng_debug1(1, "in %s storage function\n", "iCCP");
   if (xpng_ptr == NULL || info_ptr == NULL || name == NULL || profile == NULL)
      return;

   new_iccp_name = (xpng_charp)xpng_malloc(xpng_ptr, xpng_strlen(name)+1);
   xpng_strcpy(new_iccp_name, name);
   new_iccp_profile = (xpng_charp)xpng_malloc(xpng_ptr, proflen);
   xpng_memcpy(new_iccp_profile, profile, (xpng_size_t)proflen);

   xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_ICCP, 0);

   info_ptr->iccp_proflen = proflen;
   info_ptr->iccp_name = new_iccp_name;
   info_ptr->iccp_profile = new_iccp_profile;
   /* Compression is always zero but is here so the API and info structure
    * does not have to change if we introduce multiple compression types */
   info_ptr->iccp_compression = (xpng_byte)compression_type;
#ifdef PNG_FREE_ME_SUPPORTED
   info_ptr->free_me |= PNG_FREE_ICCP;
#endif
   info_ptr->valid |= PNG_INFO_iCCP;
}
#endif

#if defined(PNG_TEXT_SUPPORTED)
void PNGAPI
xpng_set_text(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_textp text_ptr,
   int num_text)
{
   int ret;
   ret=xpng_set_text_2(xpng_ptr, info_ptr, text_ptr, num_text);
   if (ret)
     xpng_error(xpng_ptr, "Insufficient memory to store text");
}

int /* PRIVATE */
xpng_set_text_2(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_textp text_ptr,
   int num_text)
{
   int i;

   xpng_debug1(1, "in %s storage function\n", (xpng_ptr->chunk_name[0] == '\0' ?
      "text" : (xpng_const_charp)xpng_ptr->chunk_name));

   if (xpng_ptr == NULL || info_ptr == NULL || num_text == 0)
      return(0);

   /* Make sure we have enough space in the "text" array in info_struct
    * to hold all of the incoming text_ptr objects.
    */
   if (info_ptr->num_text + num_text > info_ptr->max_text)
   {
      if (info_ptr->text != NULL)
      {
         xpng_textp old_text;
         int old_max;

         old_max = info_ptr->max_text;
         info_ptr->max_text = info_ptr->num_text + num_text + 8;
         old_text = info_ptr->text;
         info_ptr->text = (xpng_textp)xpng_malloc_warn(xpng_ptr,
            (xpng_uint_32)(info_ptr->max_text * sizeof (xpng_text)));
         if (info_ptr->text == NULL)
           {
             xpng_free(xpng_ptr, old_text);
             return(1);
           }
         xpng_memcpy(info_ptr->text, old_text, (xpng_size_t)(old_max *
            sizeof(xpng_text)));
         xpng_free(xpng_ptr, old_text);
      }
      else
      {
         info_ptr->max_text = num_text + 8;
         info_ptr->num_text = 0;
         info_ptr->text = (xpng_textp)xpng_malloc_warn(xpng_ptr,
            (xpng_uint_32)(info_ptr->max_text * sizeof (xpng_text)));
         if (info_ptr->text == NULL)
           return(1);
#ifdef PNG_FREE_ME_SUPPORTED
         info_ptr->free_me |= PNG_FREE_TEXT;
#endif
      }
      xpng_debug1(3, "allocated %d entries for info_ptr->text\n",
         info_ptr->max_text);
   }
   for (i = 0; i < num_text; i++)
   {
      xpng_size_t text_length,key_len;
      xpng_size_t lang_len,lang_key_len;
      xpng_textp textp = &(info_ptr->text[info_ptr->num_text]);

      if (text_ptr[i].key == NULL)
          continue;

      key_len = xpng_strlen(text_ptr[i].key);

      if(text_ptr[i].compression <= 0)
      {
        lang_len = 0;
        lang_key_len = 0;
      }
      else
#ifdef PNG_iTXt_SUPPORTED
      {
        /* set iTXt data */
        if (text_ptr[i].lang != NULL)
          lang_len = xpng_strlen(text_ptr[i].lang);
        else
          lang_len = 0;
        if (text_ptr[i].lang_key != NULL)
          lang_key_len = xpng_strlen(text_ptr[i].lang_key);
        else
          lang_key_len = 0;
      }
#else
      {
        xpng_warning(xpng_ptr, "iTXt chunk not supported.");
        continue;
      }
#endif

      if (text_ptr[i].text == NULL || text_ptr[i].text[0] == '\0')
      {
         text_length = 0;
#ifdef PNG_iTXt_SUPPORTED
         if(text_ptr[i].compression > 0)
            textp->compression = PNG_ITXT_COMPRESSION_NONE;
         else
#endif
            textp->compression = PNG_TEXT_COMPRESSION_NONE;
      }
      else
      {
         text_length = xpng_strlen(text_ptr[i].text);
         textp->compression = text_ptr[i].compression;
      }

      textp->key = (xpng_charp)xpng_malloc_warn(xpng_ptr,
         (xpng_uint_32)(key_len + text_length + lang_len + lang_key_len + 4));
      if (textp->key == NULL)
        return(1);
      xpng_debug2(2, "Allocated %lu bytes at %x in xpng_set_text\n",
         (xpng_uint_32)(key_len + lang_len + lang_key_len + text_length + 4),
         (int)textp->key);

      xpng_memcpy(textp->key, text_ptr[i].key,
         (xpng_size_t)(key_len));
      *(textp->key+key_len) = '\0';
#ifdef PNG_iTXt_SUPPORTED
      if (text_ptr[i].compression > 0)
      {
         textp->lang=textp->key + key_len + 1;
         xpng_memcpy(textp->lang, text_ptr[i].lang, lang_len);
         *(textp->lang+lang_len) = '\0';
         textp->lang_key=textp->lang + lang_len + 1;
         xpng_memcpy(textp->lang_key, text_ptr[i].lang_key, lang_key_len);
         *(textp->lang_key+lang_key_len) = '\0';
         textp->text=textp->lang_key + lang_key_len + 1;
      }
      else
#endif
      {
#ifdef PNG_iTXt_SUPPORTED
         textp->lang=NULL;
         textp->lang_key=NULL;
#endif
         textp->text=textp->key + key_len + 1;
      }
      if(text_length)
         xpng_memcpy(textp->text, text_ptr[i].text,
            (xpng_size_t)(text_length));
      *(textp->text+text_length) = '\0';

#ifdef PNG_iTXt_SUPPORTED
      if(textp->compression > 0)
      {
         textp->text_length = 0;
         textp->itxt_length = text_length;
      }
      else
#endif
      {
         textp->text_length = text_length;
#ifdef PNG_iTXt_SUPPORTED
         textp->itxt_length = 0;
#endif
      }
      info_ptr->text[info_ptr->num_text]= *textp;
      info_ptr->num_text++;
      xpng_debug1(3, "transferred text chunk %d\n", info_ptr->num_text);
   }
   return(0);
}
#endif

#if defined(PNG_tIME_SUPPORTED)
void PNGAPI
xpng_set_tIME(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_timep mod_time)
{
   xpng_debug1(1, "in %s storage function\n", "tIME");
   if (xpng_ptr == NULL || info_ptr == NULL ||
       (xpng_ptr->mode & PNG_WROTE_tIME))
      return;

   xpng_memcpy(&(info_ptr->mod_time), mod_time, sizeof (xpng_time));
   info_ptr->valid |= PNG_INFO_tIME;
}
#endif

#if defined(PNG_tRNS_SUPPORTED)
void PNGAPI
xpng_set_tRNS(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_bytep trans, int num_trans, xpng_color_16p trans_values)
{
   xpng_debug1(1, "in %s storage function\n", "tRNS");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   if (trans != NULL)
   {
       /*
        * It may not actually be necessary to set xpng_ptr->trans here;
        * we do it for backward compatibility with the way the xpng_handle_tRNS
        * function used to do the allocation.
        */
#ifdef PNG_FREE_ME_SUPPORTED
       xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_TRNS, 0);
#endif
       /* Changed from num_trans to 256 in version 1.2.1 */
       xpng_ptr->trans = info_ptr->trans = (xpng_bytep)xpng_malloc(xpng_ptr,
           (xpng_uint_32)256);
       xpng_memcpy(info_ptr->trans, trans, (xpng_size_t)num_trans);
#ifdef PNG_FREE_ME_SUPPORTED
       info_ptr->free_me |= PNG_FREE_TRNS;
#else
       xpng_ptr->flags |= PNG_FLAG_FREE_TRNS;
#endif
   }

   if (trans_values != NULL)
   {
      xpng_memcpy(&(info_ptr->trans_values), trans_values,
         sizeof(xpng_color_16));
      if (num_trans == 0)
        num_trans = 1;
   }
   info_ptr->num_trans = (xpng_uint_16)num_trans;
   info_ptr->valid |= PNG_INFO_tRNS;
}
#endif

#if defined(PNG_sPLT_SUPPORTED)
void PNGAPI
xpng_set_sPLT(xpng_structp xpng_ptr,
             xpng_infop info_ptr, xpng_sPLT_tp entries, int nentries)
{
    xpng_sPLT_tp np;
    int i;

    np = (xpng_sPLT_tp)xpng_malloc_warn(xpng_ptr,
        (info_ptr->splt_palettes_num + nentries) * sizeof(xpng_sPLT_t));
    if (np == NULL)
    {
      xpng_warning(xpng_ptr, "No memory for sPLT palettes.");
      return;
    }

    xpng_memcpy(np, info_ptr->splt_palettes,
           info_ptr->splt_palettes_num * sizeof(xpng_sPLT_t));
    xpng_free(xpng_ptr, info_ptr->splt_palettes);
    info_ptr->splt_palettes=NULL;

    for (i = 0; i < nentries; i++)
    {
        xpng_sPLT_tp to = np + info_ptr->splt_palettes_num + i;
        xpng_sPLT_tp from = entries + i;

        to->name = (xpng_charp)xpng_malloc(xpng_ptr,
            xpng_strlen(from->name) + 1);
        xpng_strcpy(to->name, from->name);
        to->entries = (xpng_sPLT_entryp)xpng_malloc(xpng_ptr,
            from->nentries * sizeof(xpng_sPLT_t));
        xpng_memcpy(to->entries, from->entries,
            from->nentries * sizeof(xpng_sPLT_t));
        to->nentries = from->nentries;
        to->depth = from->depth;
    }

    info_ptr->splt_palettes = np;
    info_ptr->splt_palettes_num += nentries;
    info_ptr->valid |= PNG_INFO_sPLT;
#ifdef PNG_FREE_ME_SUPPORTED
    info_ptr->free_me |= PNG_FREE_SPLT;
#endif
}
#endif /* PNG_sPLT_SUPPORTED */

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
void PNGAPI
xpng_set_unknown_chunks(xpng_structp xpng_ptr,
   xpng_infop info_ptr, xpng_unknown_chunkp unknowns, int num_unknowns)
{
    xpng_unknown_chunkp np;
    int i;

    if (xpng_ptr == NULL || info_ptr == NULL || num_unknowns == 0)
        return;

    np = (xpng_unknown_chunkp)xpng_malloc_warn(xpng_ptr,
        (info_ptr->unknown_chunks_num + num_unknowns) *
        sizeof(xpng_unknown_chunk));
    if (np == NULL)
    {
       xpng_warning(xpng_ptr, "Out of memory while processing unknown chunk.");
       return;
    }

    xpng_memcpy(np, info_ptr->unknown_chunks,
           info_ptr->unknown_chunks_num * sizeof(xpng_unknown_chunk));
    xpng_free(xpng_ptr, info_ptr->unknown_chunks);
    info_ptr->unknown_chunks=NULL;

    for (i = 0; i < num_unknowns; i++)
    {
        xpng_unknown_chunkp to = np + info_ptr->unknown_chunks_num + i;
        xpng_unknown_chunkp from = unknowns + i;

        xpng_strcpy((xpng_charp)to->name, (xpng_charp)from->name);
        to->data = (xpng_bytep)xpng_malloc(xpng_ptr, from->size);
        if (to->data == NULL)
           xpng_warning(xpng_ptr, "Out of memory while processing unknown chunk.");
        else
        {
          xpng_memcpy(to->data, from->data, from->size);
          to->size = from->size;

          /* note our location in the read or write sequence */
          to->location = (xpng_byte)(xpng_ptr->mode & 0xff);
        }
    }

    info_ptr->unknown_chunks = np;
    info_ptr->unknown_chunks_num += num_unknowns;
#ifdef PNG_FREE_ME_SUPPORTED
    info_ptr->free_me |= PNG_FREE_UNKN;
#endif
}
void PNGAPI
xpng_set_unknown_chunk_location(xpng_structp xpng_ptr, xpng_infop info_ptr,
   int chunk, int location)
{
   if(xpng_ptr != NULL && info_ptr != NULL && chunk >= 0 && chunk <
         (int)info_ptr->unknown_chunks_num)
      info_ptr->unknown_chunks[chunk].location = (xpng_byte)location;
}
#endif

#if defined(PNG_READ_EMPTY_PLTE_SUPPORTED) || \
    defined(PNG_WRITE_EMPTY_PLTE_SUPPORTED)
void PNGAPI
xpng_permit_empty_plte (xpng_structp xpng_ptr, int empty_plte_permitted)
{
   /* This function is deprecated in favor of xpng_permit_mng_features()
      and will be removed from libpng-2.0.0 */
   xpng_debug(1, "in xpng_permit_empty_plte, DEPRECATED.\n");
   if (xpng_ptr == NULL)
      return;
   xpng_ptr->mng_features_permitted = (xpng_byte)
     ((xpng_ptr->mng_features_permitted & (~(PNG_FLAG_MNG_EMPTY_PLTE))) |
     ((empty_plte_permitted & PNG_FLAG_MNG_EMPTY_PLTE)));
}
#endif

#if defined(PNG_MNG_FEATURES_SUPPORTED)
xpng_uint_32 PNGAPI
xpng_permit_mng_features (xpng_structp xpng_ptr, xpng_uint_32 mng_features)
{
   xpng_debug(1, "in xpng_permit_mng_features\n");
   if (xpng_ptr == NULL)
      return (xpng_uint_32)0;
   xpng_ptr->mng_features_permitted =
     (xpng_byte)(mng_features & PNG_ALL_MNG_FEATURES);
   return (xpng_uint_32)xpng_ptr->mng_features_permitted;
}
#endif

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
void PNGAPI
xpng_set_keep_unknown_chunks(xpng_structp xpng_ptr, int keep, xpng_bytep
   chunk_list, int num_chunks)
{
    xpng_bytep new_list, p;
    int i, old_num_chunks;
    if (num_chunks == 0)
    {
      if(keep == HANDLE_CHUNK_ALWAYS || keep == HANDLE_CHUNK_IF_SAFE)
        xpng_ptr->flags |= PNG_FLAG_KEEP_UNKNOWN_CHUNKS;
      else
        xpng_ptr->flags &= ~PNG_FLAG_KEEP_UNKNOWN_CHUNKS;

      if(keep == HANDLE_CHUNK_ALWAYS)
        xpng_ptr->flags |= PNG_FLAG_KEEP_UNSAFE_CHUNKS;
      else
        xpng_ptr->flags &= ~PNG_FLAG_KEEP_UNSAFE_CHUNKS;
      return;
    }
    if (chunk_list == NULL)
      return;
    old_num_chunks=xpng_ptr->num_chunk_list;
    new_list=(xpng_bytep)xpng_malloc(xpng_ptr,
       (xpng_uint_32)(5*(num_chunks+old_num_chunks)));
    if(xpng_ptr->chunk_list != NULL)
    {
       xpng_memcpy(new_list, xpng_ptr->chunk_list,
          (xpng_size_t)(5*old_num_chunks));
       xpng_free(xpng_ptr, xpng_ptr->chunk_list);
       xpng_ptr->chunk_list=NULL;
    }
    xpng_memcpy(new_list+5*old_num_chunks, chunk_list,
       (xpng_size_t)(5*num_chunks));
    for (p=new_list+5*old_num_chunks+4, i=0; i<num_chunks; i++, p+=5)
       *p=(xpng_byte)keep;
    xpng_ptr->num_chunk_list=old_num_chunks+num_chunks;
    xpng_ptr->chunk_list=new_list;
#ifdef PNG_FREE_ME_SUPPORTED
    xpng_ptr->free_me |= PNG_FREE_LIST;
#endif
}
#endif

#if defined(PNG_READ_USER_CHUNKS_SUPPORTED)
void PNGAPI
xpng_set_read_user_chunk_fn(xpng_structp xpng_ptr, xpng_voidp user_chunk_ptr,
   xpng_user_chunk_ptr read_user_chunk_fn)
{
   xpng_debug(1, "in xpng_set_read_user_chunk_fn\n");
   xpng_ptr->read_user_chunk_fn = read_user_chunk_fn;
   xpng_ptr->user_chunk_ptr = user_chunk_ptr;
}
#endif

#if defined(PNG_INFO_IMAGE_SUPPORTED)
void PNGAPI
xpng_set_rows(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_bytepp row_pointers)
{
   xpng_debug1(1, "in %s storage function\n", "rows");

   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   if(info_ptr->row_pointers && (info_ptr->row_pointers != row_pointers))
      xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_ROWS, 0);
   info_ptr->row_pointers = row_pointers;
   if(row_pointers)
      info_ptr->valid |= PNG_INFO_IDAT;
}
#endif

void PNGAPI
xpng_set_compression_buffer_size(xpng_structp xpng_ptr, xpng_uint_32 size)
{
    if(xpng_ptr->zbuf)
       xpng_free(xpng_ptr, xpng_ptr->zbuf);
    xpng_ptr->zbuf_size = (xpng_size_t)size;
    xpng_ptr->zbuf = (xpng_bytep)xpng_malloc(xpng_ptr, size);
    xpng_ptr->zstream.next_out = xpng_ptr->zbuf;
    xpng_ptr->zstream.avail_out = (uInt)xpng_ptr->zbuf_size;
}

void PNGAPI
xpng_set_invalid(xpng_structp xpng_ptr, xpng_infop info_ptr, int mask)
{
   if (xpng_ptr && info_ptr)
      info_ptr->valid &= ~(mask);
}


#ifndef PNG_1_0_X
#ifdef PNG_ASSEMBLER_CODE_SUPPORTED
/* this function was added to libpng 1.2.0 and should always exist by default */
void PNGAPI
xpng_set_asm_flags (xpng_structp xpng_ptr, xpng_uint_32 asm_flags)
{
    xpng_uint_32 settable_asm_flags;
    xpng_uint_32 settable_mmx_flags;

    settable_mmx_flags =
#ifdef PNG_HAVE_ASSEMBLER_COMBINE_ROW
                         PNG_ASM_FLAG_MMX_READ_COMBINE_ROW  |
#endif
#ifdef PNG_HAVE_ASSEMBLER_READ_INTERLACE
                         PNG_ASM_FLAG_MMX_READ_INTERLACE    |
#endif
#ifdef PNG_HAVE_ASSEMBLER_READ_FILTER_ROW
                         PNG_ASM_FLAG_MMX_READ_FILTER_SUB   |
                         PNG_ASM_FLAG_MMX_READ_FILTER_UP    |
                         PNG_ASM_FLAG_MMX_READ_FILTER_AVG   |
                         PNG_ASM_FLAG_MMX_READ_FILTER_PAETH |
#endif
                         0;

    /* could be some non-MMX ones in the future, but not currently: */
    settable_asm_flags = settable_mmx_flags;

    if (!(xpng_ptr->asm_flags & PNG_ASM_FLAG_MMX_SUPPORT_COMPILED) ||
        !(xpng_ptr->asm_flags & PNG_ASM_FLAG_MMX_SUPPORT_IN_CPU))
    {
        /* clear all MMX flags if MMX isn't supported */
        settable_asm_flags &= ~settable_mmx_flags;
        xpng_ptr->asm_flags &= ~settable_mmx_flags;
    }

    /* we're replacing the settable bits with those passed in by the user,
     * so first zero them out of the master copy, then logical-OR in the
     * allowed subset that was requested */

    xpng_ptr->asm_flags &= ~settable_asm_flags;               /* zero them */
    xpng_ptr->asm_flags |= (asm_flags & settable_asm_flags);  /* set them */
}
#endif /* ?PNG_ASSEMBLER_CODE_SUPPORTED */

#ifdef PNG_ASSEMBLER_CODE_SUPPORTED
/* this function was added to libpng 1.2.0 */
void PNGAPI
xpng_set_mmx_thresholds (xpng_structp xpng_ptr,
                        xpng_byte mmx_bitdepth_threshold,
                        xpng_uint_32 mmx_rowbytes_threshold)
{
    xpng_ptr->mmx_bitdepth_threshold = mmx_bitdepth_threshold;
    xpng_ptr->mmx_rowbytes_threshold = mmx_rowbytes_threshold;
}
#endif /* ?PNG_ASSEMBLER_CODE_SUPPORTED */
#endif /* ?PNG_1_0_X */
