// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngrtran.c - transforms the data in a row for PNG readers
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This file contains functions optionally called by an application
 * in order to tell libpng how to handle data when reading a PNG.
 * Transformations that are used in both reading and writing are
 * in pngtrans.c.
 */

#define PNG_INTERNAL
#include "png.h"

/* Set the action on getting a CRC error for an ancillary or critical chunk. */
void PNGAPI
xpng_set_crc_action(xpng_structp xpng_ptr, int crit_action, int ancil_action)
{
   xpng_debug(1, "in xpng_set_crc_action\n");
   /* Tell libpng how we react to CRC errors in critical chunks */
   switch (crit_action)
   {
      case PNG_CRC_NO_CHANGE:                        /* leave setting as is */
         break;
      case PNG_CRC_WARN_USE:                               /* warn/use data */
         xpng_ptr->flags &= ~PNG_FLAG_CRC_CRITICAL_MASK;
         xpng_ptr->flags |= PNG_FLAG_CRC_CRITICAL_USE;
         break;
      case PNG_CRC_QUIET_USE:                             /* quiet/use data */
         xpng_ptr->flags &= ~PNG_FLAG_CRC_CRITICAL_MASK;
         xpng_ptr->flags |= PNG_FLAG_CRC_CRITICAL_USE |
                           PNG_FLAG_CRC_CRITICAL_IGNORE;
         break;
      case PNG_CRC_WARN_DISCARD:    /* not a valid action for critical data */
         xpng_warning(xpng_ptr, "Can't discard critical data on CRC error.");
      case PNG_CRC_ERROR_QUIT:                                /* error/quit */
      case PNG_CRC_DEFAULT:
      default:
         xpng_ptr->flags &= ~PNG_FLAG_CRC_CRITICAL_MASK;
         break;
   }

   switch (ancil_action)
   {
      case PNG_CRC_NO_CHANGE:                       /* leave setting as is */
         break;
      case PNG_CRC_WARN_USE:                              /* warn/use data */
         xpng_ptr->flags &= ~PNG_FLAG_CRC_ANCILLARY_MASK;
         xpng_ptr->flags |= PNG_FLAG_CRC_ANCILLARY_USE;
         break;
      case PNG_CRC_QUIET_USE:                            /* quiet/use data */
         xpng_ptr->flags &= ~PNG_FLAG_CRC_ANCILLARY_MASK;
         xpng_ptr->flags |= PNG_FLAG_CRC_ANCILLARY_USE |
                           PNG_FLAG_CRC_ANCILLARY_NOWARN;
         break;
      case PNG_CRC_ERROR_QUIT:                               /* error/quit */
         xpng_ptr->flags &= ~PNG_FLAG_CRC_ANCILLARY_MASK;
         xpng_ptr->flags |= PNG_FLAG_CRC_ANCILLARY_NOWARN;
         break;
      case PNG_CRC_WARN_DISCARD:                      /* warn/discard data */
      case PNG_CRC_DEFAULT:
      default:
         xpng_ptr->flags &= ~PNG_FLAG_CRC_ANCILLARY_MASK;
         break;
   }
}

#if defined(PNG_READ_BACKGROUND_SUPPORTED) && \
    defined(PNG_FLOATING_POINT_SUPPORTED)
/* handle alpha and tRNS via a background color */
void PNGAPI
xpng_set_background(xpng_structp xpng_ptr,
   xpng_color_16p background_color, int background_gamma_code,
   int need_expand, double background_gamma)
{
   xpng_debug(1, "in xpng_set_background\n");
   if (background_gamma_code == PNG_BACKGROUND_GAMMA_UNKNOWN)
   {
      xpng_warning(xpng_ptr, "Application must supply a known background gamma");
      return;
   }

   xpng_ptr->transformations |= PNG_BACKGROUND;
   xpng_memcpy(&(xpng_ptr->background), background_color, sizeof(xpng_color_16));
   xpng_ptr->background_gamma = (float)background_gamma;
   xpng_ptr->background_gamma_type = (xpng_byte)(background_gamma_code);
   xpng_ptr->transformations |= (need_expand ? PNG_BACKGROUND_EXPAND : 0);

   /* Note:  if need_expand is set and color_type is either RGB or RGB_ALPHA
    * (in which case need_expand is superfluous anyway), the background color
    * might actually be gray yet not be flagged as such. This is not a problem
    * for the current code, which uses PNG_BACKGROUND_IS_GRAY only to
    * decide when to do the xpng_do_gray_to_rgb() transformation.
    */
   if ((need_expand && !(xpng_ptr->color_type & PNG_COLOR_MASK_COLOR)) ||
       (!need_expand && background_color->red == background_color->green &&
        background_color->red == background_color->blue))
      xpng_ptr->mode |= PNG_BACKGROUND_IS_GRAY;
}
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
/* strip 16 bit depth files to 8 bit depth */
void PNGAPI
xpng_set_strip_16(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_set_strip_16\n");
   xpng_ptr->transformations |= PNG_16_TO_8;
}
#endif

#if defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
void PNGAPI
xpng_set_strip_alpha(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_set_strip_alpha\n");
   xpng_ptr->transformations |= PNG_STRIP_ALPHA;
}
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
/* Dither file to 8 bit.  Supply a palette, the current number
 * of elements in the palette, the maximum number of elements
 * allowed, and a histogram if possible.  If the current number
 * of colors is greater then the maximum number, the palette will be
 * modified to fit in the maximum number.  "full_dither" indicates
 * whether we need a dithering cube set up for RGB images, or if we
 * simply are reducing the number of colors in a paletted image.
 */

typedef struct xpng_dsort_struct
{
   struct xpng_dsort_struct FAR * next;
   xpng_byte left;
   xpng_byte right;
} xpng_dsort;
typedef xpng_dsort FAR *       xpng_dsortp;
typedef xpng_dsort FAR * FAR * xpng_dsortpp;

void PNGAPI
xpng_set_dither(xpng_structp xpng_ptr, xpng_colorp palette,
   int num_palette, int maximum_colors, xpng_uint_16p histogram,
   int full_dither)
{
   xpng_debug(1, "in xpng_set_dither\n");
   xpng_ptr->transformations |= PNG_DITHER;

   if (!full_dither)
   {
      int i;

      xpng_ptr->dither_index = (xpng_bytep)xpng_malloc(xpng_ptr,
         (xpng_uint_32)(num_palette * sizeof (xpng_byte)));
      for (i = 0; i < num_palette; i++)
         xpng_ptr->dither_index[i] = (xpng_byte)i;
   }

   if (num_palette > maximum_colors)
   {
      if (histogram != NULL)
      {
         /* This is easy enough, just throw out the least used colors.
            Perhaps not the best solution, but good enough. */

         int i;

         /* initialize an array to sort colors */
         xpng_ptr->dither_sort = (xpng_bytep)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(num_palette * sizeof (xpng_byte)));

         /* initialize the dither_sort array */
         for (i = 0; i < num_palette; i++)
            xpng_ptr->dither_sort[i] = (xpng_byte)i;

         /* Find the least used palette entries by starting a
            bubble sort, and running it until we have sorted
            out enough colors.  Note that we don't care about
            sorting all the colors, just finding which are
            least used. */

         for (i = num_palette - 1; i >= maximum_colors; i--)
         {
            int done; /* to stop early if the list is pre-sorted */
            int j;

            done = 1;
            for (j = 0; j < i; j++)
            {
               if (histogram[xpng_ptr->dither_sort[j]]
                   < histogram[xpng_ptr->dither_sort[j + 1]])
               {
                  xpng_byte t;

                  t = xpng_ptr->dither_sort[j];
                  xpng_ptr->dither_sort[j] = xpng_ptr->dither_sort[j + 1];
                  xpng_ptr->dither_sort[j + 1] = t;
                  done = 0;
               }
            }
            if (done)
               break;
         }

         /* swap the palette around, and set up a table, if necessary */
         if (full_dither)
         {
            int j = num_palette;

            /* put all the useful colors within the max, but don't
               move the others */
            for (i = 0; i < maximum_colors; i++)
            {
               if ((int)xpng_ptr->dither_sort[i] >= maximum_colors)
               {
                  do
                     j--;
                  while ((int)xpng_ptr->dither_sort[j] >= maximum_colors);
                  palette[i] = palette[j];
               }
            }
         }
         else
         {
            int j = num_palette;

            /* move all the used colors inside the max limit, and
               develop a translation table */
            for (i = 0; i < maximum_colors; i++)
            {
               /* only move the colors we need to */
               if ((int)xpng_ptr->dither_sort[i] >= maximum_colors)
               {
                  xpng_color tmp_color;

                  do
                     j--;
                  while ((int)xpng_ptr->dither_sort[j] >= maximum_colors);

                  tmp_color = palette[j];
                  palette[j] = palette[i];
                  palette[i] = tmp_color;
                  /* indicate where the color went */
                  xpng_ptr->dither_index[j] = (xpng_byte)i;
                  xpng_ptr->dither_index[i] = (xpng_byte)j;
               }
            }

            /* find closest color for those colors we are not using */
            for (i = 0; i < num_palette; i++)
            {
               if ((int)xpng_ptr->dither_index[i] >= maximum_colors)
               {
                  int min_d, k, min_k, d_index;

                  /* find the closest color to one we threw out */
                  d_index = xpng_ptr->dither_index[i];
                  min_d = PNG_COLOR_DIST(palette[d_index], palette[0]);
                  for (k = 1, min_k = 0; k < maximum_colors; k++)
                  {
                     int d;

                     d = PNG_COLOR_DIST(palette[d_index], palette[k]);

                     if (d < min_d)
                     {
                        min_d = d;
                        min_k = k;
                     }
                  }
                  /* point to closest color */
                  xpng_ptr->dither_index[i] = (xpng_byte)min_k;
               }
            }
         }
         xpng_free(xpng_ptr, xpng_ptr->dither_sort);
         xpng_ptr->dither_sort=NULL;
      }
      else
      {
         /* This is much harder to do simply (and quickly).  Perhaps
            we need to go through a median cut routine, but those
            don't always behave themselves with only a few colors
            as input.  So we will just find the closest two colors,
            and throw out one of them (chosen somewhat randomly).
            [We don't understand this at all, so if someone wants to
             work on improving it, be our guest - AED, GRP]
            */
         int i;
         int max_d;
         int num_new_palette;
         xpng_dsortp t;
         xpng_dsortpp hash;

         t=NULL;

         /* initialize palette index arrays */
         xpng_ptr->index_to_palette = (xpng_bytep)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(num_palette * sizeof (xpng_byte)));
         xpng_ptr->palette_to_index = (xpng_bytep)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(num_palette * sizeof (xpng_byte)));

         /* initialize the sort array */
         for (i = 0; i < num_palette; i++)
         {
            xpng_ptr->index_to_palette[i] = (xpng_byte)i;
            xpng_ptr->palette_to_index[i] = (xpng_byte)i;
         }

         hash = (xpng_dsortpp)xpng_malloc(xpng_ptr, (xpng_uint_32)(769 *
            sizeof (xpng_dsortp)));
         for (i = 0; i < 769; i++)
            hash[i] = NULL;
/*         xpng_memset(hash, 0, 769 * sizeof (xpng_dsortp)); */

         num_new_palette = num_palette;

         /* initial wild guess at how far apart the farthest pixel
            pair we will be eliminating will be.  Larger
            numbers mean more areas will be allocated, Smaller
            numbers run the risk of not saving enough data, and
            having to do this all over again.

            I have not done extensive checking on this number.
            */
         max_d = 96;

         while (num_new_palette > maximum_colors)
         {
            for (i = 0; i < num_new_palette - 1; i++)
            {
               int j;

               for (j = i + 1; j < num_new_palette; j++)
               {
                  int d;

                  d = PNG_COLOR_DIST(palette[i], palette[j]);

                  if (d <= max_d)
                  {

                     t = (xpng_dsortp)xpng_malloc_warn(xpng_ptr,
                         (xpng_uint_32)(sizeof(xpng_dsort)));
                     if (t == NULL)
                         break;
                     t->next = hash[d];
                     t->left = (xpng_byte)i;
                     t->right = (xpng_byte)j;
                     hash[d] = t;
                  }
               }
               if (t == NULL)
                  break;
            }

            if (t != NULL)
            for (i = 0; i <= max_d; i++)
            {
               if (hash[i] != NULL)
               {
                  xpng_dsortp p;

                  for (p = hash[i]; p; p = p->next)
                  {
                     if ((int)xpng_ptr->index_to_palette[p->left]
                        < num_new_palette &&
                        (int)xpng_ptr->index_to_palette[p->right]
                        < num_new_palette)
                     {
                        int j, next_j;

                        if (num_new_palette & 0x01)
                        {
                           j = p->left;
                           next_j = p->right;
                        }
                        else
                        {
                           j = p->right;
                           next_j = p->left;
                        }

                        num_new_palette--;
                        palette[xpng_ptr->index_to_palette[j]]
                          = palette[num_new_palette];
                        if (!full_dither)
                        {
                           int k;

                           for (k = 0; k < num_palette; k++)
                           {
                              if (xpng_ptr->dither_index[k] ==
                                 xpng_ptr->index_to_palette[j])
                                 xpng_ptr->dither_index[k] =
                                    xpng_ptr->index_to_palette[next_j];
                              if ((int)xpng_ptr->dither_index[k] ==
                                 num_new_palette)
                                 xpng_ptr->dither_index[k] =
                                    xpng_ptr->index_to_palette[j];
                           }
                        }

                        xpng_ptr->index_to_palette[xpng_ptr->palette_to_index
                           [num_new_palette]] = xpng_ptr->index_to_palette[j];
                        xpng_ptr->palette_to_index[xpng_ptr->index_to_palette[j]]
                           = xpng_ptr->palette_to_index[num_new_palette];

                        xpng_ptr->index_to_palette[j] = (xpng_byte)num_new_palette;
                        xpng_ptr->palette_to_index[num_new_palette] = (xpng_byte)j;
                     }
                     if (num_new_palette <= maximum_colors)
                        break;
                  }
                  if (num_new_palette <= maximum_colors)
                     break;
               }
            }

            for (i = 0; i < 769; i++)
            {
               if (hash[i] != NULL)
               {
                  xpng_dsortp p = hash[i];
                  while (p)
                  {
                     t = p->next;
                     xpng_free(xpng_ptr, p);
                     p = t;
                  }
               }
               hash[i] = 0;
            }
            max_d += 96;
         }
         xpng_free(xpng_ptr, hash);
         xpng_free(xpng_ptr, xpng_ptr->palette_to_index);
         xpng_free(xpng_ptr, xpng_ptr->index_to_palette);
         xpng_ptr->palette_to_index=NULL;
         xpng_ptr->index_to_palette=NULL;
      }
      num_palette = maximum_colors;
   }
   if (xpng_ptr->palette == NULL)
   {
      xpng_ptr->palette = palette;
   }
   xpng_ptr->num_palette = (xpng_uint_16)num_palette;

   if (full_dither)
   {
      int i;
      xpng_bytep distance;
      int total_bits = PNG_DITHER_RED_BITS + PNG_DITHER_GREEN_BITS +
         PNG_DITHER_BLUE_BITS;
      int num_red = (1 << PNG_DITHER_RED_BITS);
      int num_green = (1 << PNG_DITHER_GREEN_BITS);
      int num_blue = (1 << PNG_DITHER_BLUE_BITS);
      xpng_size_t num_entries = ((xpng_size_t)1 << total_bits);

      xpng_ptr->palette_lookup = (xpng_bytep )xpng_malloc(xpng_ptr,
         (xpng_uint_32)(num_entries * sizeof (xpng_byte)));

      xpng_memset(xpng_ptr->palette_lookup, 0, num_entries * sizeof (xpng_byte));

      distance = (xpng_bytep)xpng_malloc(xpng_ptr, (xpng_uint_32)(num_entries *
         sizeof(xpng_byte)));

      xpng_memset(distance, 0xff, num_entries * sizeof(xpng_byte));

      for (i = 0; i < num_palette; i++)
      {
         int ir, ig, ib;
         int r = (palette[i].red >> (8 - PNG_DITHER_RED_BITS));
         int g = (palette[i].green >> (8 - PNG_DITHER_GREEN_BITS));
         int b = (palette[i].blue >> (8 - PNG_DITHER_BLUE_BITS));

         for (ir = 0; ir < num_red; ir++)
         {
            int dr = abs(ir - r);
            int index_r = (ir << (PNG_DITHER_BLUE_BITS + PNG_DITHER_GREEN_BITS));

            for (ig = 0; ig < num_green; ig++)
            {
               int dg = abs(ig - g);
               int dt = dr + dg;
               int dm = ((dr > dg) ? dr : dg);
               int index_g = index_r | (ig << PNG_DITHER_BLUE_BITS);

               for (ib = 0; ib < num_blue; ib++)
               {
                  int d_index = index_g | ib;
                  int db = abs(ib - b);
                  int dmax = ((dm > db) ? dm : db);
                  int d = dmax + dt + db;

                  if (d < (int)distance[d_index])
                  {
                     distance[d_index] = (xpng_byte)d;
                     xpng_ptr->palette_lookup[d_index] = (xpng_byte)i;
                  }
               }
            }
         }
      }

      xpng_free(xpng_ptr, distance);
   }
}
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED) && defined(PNG_FLOATING_POINT_SUPPORTED)
/* Transform the image from the file_gamma to the screen_gamma.  We
 * only do transformations on images where the file_gamma and screen_gamma
 * are not close reciprocals, otherwise it slows things down slightly, and
 * also needlessly introduces small errors.
 *
 * We will turn off gamma transformation later if no semitransparent entries
 * are present in the tRNS array for palette images.  We can't do it here
 * because we don't necessarily have the tRNS chunk yet.
 */
void PNGAPI
xpng_set_gamma(xpng_structp xpng_ptr, double scrn_gamma, double file_gamma)
{
   xpng_debug(1, "in xpng_set_gamma\n");
   if ((fabs(scrn_gamma * file_gamma - 1.0) > PNG_GAMMA_THRESHOLD) ||
       (xpng_ptr->color_type & PNG_COLOR_MASK_ALPHA) ||
       (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE))
     xpng_ptr->transformations |= PNG_GAMMA;
   xpng_ptr->gamma = (float)file_gamma;
   xpng_ptr->screen_gamma = (float)scrn_gamma;
}
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
/* Expand paletted images to RGB, expand grayscale images of
 * less than 8-bit depth to 8-bit depth, and expand tRNS chunks
 * to alpha channels.
 */
void PNGAPI
xpng_set_expand(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_set_expand\n");
   xpng_ptr->transformations |= PNG_EXPAND;
}

/* GRR 19990627:  the following three functions currently are identical
 *  to xpng_set_expand().  However, it is entirely reasonable that someone
 *  might wish to expand an indexed image to RGB but *not* expand a single,
 *  fully transparent palette entry to a full alpha channel--perhaps instead
 *  convert tRNS to the grayscale/RGB format (16-bit RGB value), or replace
 *  the transparent color with a particular RGB value, or drop tRNS entirely.
 *  IOW, a future version of the library may make the transformations flag
 *  a bit more fine-grained, with separate bits for each of these three
 *  functions.
 *
 *  More to the point, these functions make it obvious what libpng will be
 *  doing, whereas "expand" can (and does) mean any number of things.
 */

/* Expand paletted images to RGB. */
void PNGAPI
xpng_set_palette_to_rgb(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_set_expand\n");
   xpng_ptr->transformations |= PNG_EXPAND;
}

/* Expand grayscale images of less than 8-bit depth to 8 bits. */
void PNGAPI
xpng_set_gray_1_2_4_to_8(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_set_expand\n");
   xpng_ptr->transformations |= PNG_EXPAND;
}

/* Expand tRNS chunks to alpha channels. */
void PNGAPI
xpng_set_tRNS_to_alpha(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_set_expand\n");
   xpng_ptr->transformations |= PNG_EXPAND;
}
#endif /* defined(PNG_READ_EXPAND_SUPPORTED) */

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
void PNGAPI
xpng_set_gray_to_rgb(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_set_gray_to_rgb\n");
   xpng_ptr->transformations |= PNG_GRAY_TO_RGB;
}
#endif

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
#if defined(PNG_FLOATING_POINT_SUPPORTED)
/* Convert a RGB image to a grayscale of the same width.  This allows us,
 * for example, to convert a 24 bpp RGB image into an 8 bpp grayscale image.
 */

void PNGAPI
xpng_set_rgb_to_gray(xpng_structp xpng_ptr, int error_action, double red,
   double green)
{
      int red_fixed = (int)((float)red*100000.0 + 0.5);
      int green_fixed = (int)((float)green*100000.0 + 0.5);
      xpng_set_rgb_to_gray_fixed(xpng_ptr, error_action, red_fixed, green_fixed);
}
#endif

void PNGAPI
xpng_set_rgb_to_gray_fixed(xpng_structp xpng_ptr, int error_action,
   xpng_fixed_point red, xpng_fixed_point green)
{
   xpng_debug(1, "in xpng_set_rgb_to_gray\n");
   switch(error_action)
   {
      case 1: xpng_ptr->transformations |= PNG_RGB_TO_GRAY;
              break;
      case 2: xpng_ptr->transformations |= PNG_RGB_TO_GRAY_WARN;
              break;
      case 3: xpng_ptr->transformations |= PNG_RGB_TO_GRAY_ERR;
   }
   if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
#if defined(PNG_READ_EXPAND_SUPPORTED)
      xpng_ptr->transformations |= PNG_EXPAND;
#else
   {
      xpng_warning(xpng_ptr, "Cannot do RGB_TO_GRAY without EXPAND_SUPPORTED.");
      xpng_ptr->transformations &= ~PNG_RGB_TO_GRAY;
   }
#endif
   {
      xpng_uint_16 red_int, green_int;
      if(red < 0 || green < 0)
      {
         red_int   =  6968; /* .212671 * 32768 + .5 */
         green_int = 23434; /* .715160 * 32768 + .5 */
      }
      else if(red + green < 100000L)
      {
        red_int = (xpng_uint_16)(((xpng_uint_32)red*32768L)/100000L);
        green_int = (xpng_uint_16)(((xpng_uint_32)green*32768L)/100000L);
      }
      else
      {
         xpng_warning(xpng_ptr, "ignoring out of range rgb_to_gray coefficients");
         red_int   =  6968;
         green_int = 23434;
      }
      xpng_ptr->rgb_to_gray_red_coeff   = red_int;
      xpng_ptr->rgb_to_gray_green_coeff = green_int;
      xpng_ptr->rgb_to_gray_blue_coeff  = (xpng_uint_16)(32768-red_int-green_int);
   }
}
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_LEGACY_SUPPORTED)
void PNGAPI
xpng_set_read_user_transform_fn(xpng_structp xpng_ptr, xpng_user_transform_ptr
   read_user_transform_fn)
{
   xpng_debug(1, "in xpng_set_read_user_transform_fn\n");
#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED)
   xpng_ptr->transformations |= PNG_USER_TRANSFORM;
   xpng_ptr->read_user_transform_fn = read_user_transform_fn;
#endif
#ifdef PNG_LEGACY_SUPPORTED
   if(read_user_transform_fn)
      xpng_warning(xpng_ptr,
        "This version of libpng does not support user transforms");
#endif
}
#endif

/* Initialize everything needed for the read.  This includes modifying
 * the palette.
 */
void /* PRIVATE */
xpng_init_read_transformations(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_init_read_transformations\n");
#if defined(PNG_USELESS_TESTS_SUPPORTED)
   if(xpng_ptr != NULL)
#endif
  {
#if defined(PNG_READ_BACKGROUND_SUPPORTED) || defined(PNG_READ_SHIFT_SUPPORTED) \
 || defined(PNG_READ_GAMMA_SUPPORTED)
   int color_type = xpng_ptr->color_type;
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED) && defined(PNG_READ_BACKGROUND_SUPPORTED)
   if ((xpng_ptr->transformations & PNG_BACKGROUND_EXPAND) &&
       (xpng_ptr->transformations & PNG_EXPAND))
   {
      if (!(color_type & PNG_COLOR_MASK_COLOR))  /* i.e., GRAY or GRAY_ALPHA */
      {
         /* expand background chunk. */
         switch (xpng_ptr->bit_depth)
         {
            case 1:
               xpng_ptr->background.gray *= (xpng_uint_16)0xff;
               xpng_ptr->background.red = xpng_ptr->background.green
                 =  xpng_ptr->background.blue = xpng_ptr->background.gray;
               break;
            case 2:
               xpng_ptr->background.gray *= (xpng_uint_16)0x55;
               xpng_ptr->background.red = xpng_ptr->background.green
                 = xpng_ptr->background.blue = xpng_ptr->background.gray;
               break;
            case 4:
               xpng_ptr->background.gray *= (xpng_uint_16)0x11;
               xpng_ptr->background.red = xpng_ptr->background.green
                 = xpng_ptr->background.blue = xpng_ptr->background.gray;
               break;
            case 8:
            case 16:
               xpng_ptr->background.red = xpng_ptr->background.green
                 = xpng_ptr->background.blue = xpng_ptr->background.gray;
               break;
         }
      }
      else if (color_type == PNG_COLOR_TYPE_PALETTE)
      {
         xpng_ptr->background.red   =
            xpng_ptr->palette[xpng_ptr->background.index].red;
         xpng_ptr->background.green =
            xpng_ptr->palette[xpng_ptr->background.index].green;
         xpng_ptr->background.blue  =
            xpng_ptr->palette[xpng_ptr->background.index].blue;

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED)
        if (xpng_ptr->transformations & PNG_INVERT_ALPHA)
        {
#if defined(PNG_READ_EXPAND_SUPPORTED)
           if (!(xpng_ptr->transformations & PNG_EXPAND))
#endif
           {
           /* invert the alpha channel (in tRNS) unless the pixels are
              going to be expanded, in which case leave it for later */
              int i,istop;
              istop=(int)xpng_ptr->num_trans;
              for (i=0; i<istop; i++)
                 xpng_ptr->trans[i] = (xpng_byte)(255 - xpng_ptr->trans[i]);
           }
        }
#endif

      }
   }
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED) && defined(PNG_READ_GAMMA_SUPPORTED)
   xpng_ptr->background_1 = xpng_ptr->background;
#endif
#if defined(PNG_READ_GAMMA_SUPPORTED) && defined(PNG_FLOATING_POINT_SUPPORTED)

   if ((color_type == PNG_COLOR_TYPE_PALETTE && xpng_ptr->num_trans != 0)
       && (fabs(xpng_ptr->screen_gamma * xpng_ptr->gamma - 1.0)
         < PNG_GAMMA_THRESHOLD))
   {
    int i,k;
    k=0;
    for (i=0; i<xpng_ptr->num_trans; i++)
    {
      if (xpng_ptr->trans[i] != 0 && xpng_ptr->trans[i] != 0xff)
        k=1; /* partial transparency is present */
    }
    if (k == 0)
      xpng_ptr->transformations &= (~PNG_GAMMA);
   }

   if (xpng_ptr->transformations & (PNG_GAMMA | PNG_RGB_TO_GRAY))
   {
      xpng_build_gamma_table(xpng_ptr);
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
      if (xpng_ptr->transformations & PNG_BACKGROUND)
      {
         if (color_type == PNG_COLOR_TYPE_PALETTE)
         {
           /* could skip if no transparency and 
           */
            xpng_color back, back_1;
            xpng_colorp palette = xpng_ptr->palette;
            int num_palette = xpng_ptr->num_palette;
            int i;
            if (xpng_ptr->background_gamma_type == PNG_BACKGROUND_GAMMA_FILE)
            {
               back.red = xpng_ptr->gamma_table[xpng_ptr->background.red];
               back.green = xpng_ptr->gamma_table[xpng_ptr->background.green];
               back.blue = xpng_ptr->gamma_table[xpng_ptr->background.blue];

               back_1.red = xpng_ptr->gamma_to_1[xpng_ptr->background.red];
               back_1.green = xpng_ptr->gamma_to_1[xpng_ptr->background.green];
               back_1.blue = xpng_ptr->gamma_to_1[xpng_ptr->background.blue];
            }
            else
            {
               double g, gs;

               switch (xpng_ptr->background_gamma_type)
               {
                  case PNG_BACKGROUND_GAMMA_SCREEN:
                     g = (xpng_ptr->screen_gamma);
                     gs = 1.0;
                     break;
                  case PNG_BACKGROUND_GAMMA_FILE:
                     g = 1.0 / (xpng_ptr->gamma);
                     gs = 1.0 / (xpng_ptr->gamma * xpng_ptr->screen_gamma);
                     break;
                  case PNG_BACKGROUND_GAMMA_UNIQUE:
                     g = 1.0 / (xpng_ptr->background_gamma);
                     gs = 1.0 / (xpng_ptr->background_gamma *
                                 xpng_ptr->screen_gamma);
                     break;
                  default:
                     g = 1.0;    /* back_1 */
                     gs = 1.0;   /* back */
               }

               if ( fabs(gs - 1.0) < PNG_GAMMA_THRESHOLD)
               {
                  back.red   = (xpng_byte)xpng_ptr->background.red;
                  back.green = (xpng_byte)xpng_ptr->background.green;
                  back.blue  = (xpng_byte)xpng_ptr->background.blue;
               }
               else
               {
                  back.red = (xpng_byte)(pow(
                     (double)xpng_ptr->background.red/255, gs) * 255.0 + .5);
                  back.green = (xpng_byte)(pow(
                     (double)xpng_ptr->background.green/255, gs) * 255.0 + .5);
                  back.blue = (xpng_byte)(pow(
                     (double)xpng_ptr->background.blue/255, gs) * 255.0 + .5);
               }

               back_1.red = (xpng_byte)(pow(
                  (double)xpng_ptr->background.red/255, g) * 255.0 + .5);
               back_1.green = (xpng_byte)(pow(
                  (double)xpng_ptr->background.green/255, g) * 255.0 + .5);
               back_1.blue = (xpng_byte)(pow(
                  (double)xpng_ptr->background.blue/255, g) * 255.0 + .5);
            }
            for (i = 0; i < num_palette; i++)
            {
               if (i < (int)xpng_ptr->num_trans && xpng_ptr->trans[i] != 0xff)
               {
                  if (xpng_ptr->trans[i] == 0)
                  {
                     palette[i] = back;
                  }
                  else /* if (xpng_ptr->trans[i] != 0xff) */
                  {
                     xpng_byte v, w;

                     v = xpng_ptr->gamma_to_1[palette[i].red];
                     xpng_composite(w, v, xpng_ptr->trans[i], back_1.red);
                     palette[i].red = xpng_ptr->gamma_from_1[w];

                     v = xpng_ptr->gamma_to_1[palette[i].green];
                     xpng_composite(w, v, xpng_ptr->trans[i], back_1.green);
                     palette[i].green = xpng_ptr->gamma_from_1[w];

                     v = xpng_ptr->gamma_to_1[palette[i].blue];
                     xpng_composite(w, v, xpng_ptr->trans[i], back_1.blue);
                     palette[i].blue = xpng_ptr->gamma_from_1[w];
                  }
               }
               else
               {
                  palette[i].red = xpng_ptr->gamma_table[palette[i].red];
                  palette[i].green = xpng_ptr->gamma_table[palette[i].green];
                  palette[i].blue = xpng_ptr->gamma_table[palette[i].blue];
               }
            }
         }
         /* if (xpng_ptr->background_gamma_type!=PNG_BACKGROUND_GAMMA_UNKNOWN) */
         else
         /* color_type != PNG_COLOR_TYPE_PALETTE */
         {
            double m = (double)(((xpng_uint_32)1 << xpng_ptr->bit_depth) - 1);
            double g = 1.0;
            double gs = 1.0;

            switch (xpng_ptr->background_gamma_type)
            {
               case PNG_BACKGROUND_GAMMA_SCREEN:
                  g = (xpng_ptr->screen_gamma);
                  gs = 1.0;
                  break;
               case PNG_BACKGROUND_GAMMA_FILE:
                  g = 1.0 / (xpng_ptr->gamma);
                  gs = 1.0 / (xpng_ptr->gamma * xpng_ptr->screen_gamma);
                  break;
               case PNG_BACKGROUND_GAMMA_UNIQUE:
                  g = 1.0 / (xpng_ptr->background_gamma);
                  gs = 1.0 / (xpng_ptr->background_gamma *
                     xpng_ptr->screen_gamma);
                  break;
            }

            xpng_ptr->background_1.gray = (xpng_uint_16)(pow(
               (double)xpng_ptr->background.gray / m, g) * m + .5);
            xpng_ptr->background.gray = (xpng_uint_16)(pow(
               (double)xpng_ptr->background.gray / m, gs) * m + .5);

            if ((xpng_ptr->background.red != xpng_ptr->background.green) ||
                (xpng_ptr->background.red != xpng_ptr->background.blue) ||
                (xpng_ptr->background.red != xpng_ptr->background.gray))
            {
               /* RGB or RGBA with color background */
               xpng_ptr->background_1.red = (xpng_uint_16)(pow(
                  (double)xpng_ptr->background.red / m, g) * m + .5);
               xpng_ptr->background_1.green = (xpng_uint_16)(pow(
                  (double)xpng_ptr->background.green / m, g) * m + .5);
               xpng_ptr->background_1.blue = (xpng_uint_16)(pow(
                  (double)xpng_ptr->background.blue / m, g) * m + .5);
               xpng_ptr->background.red = (xpng_uint_16)(pow(
                  (double)xpng_ptr->background.red / m, gs) * m + .5);
               xpng_ptr->background.green = (xpng_uint_16)(pow(
                  (double)xpng_ptr->background.green / m, gs) * m + .5);
               xpng_ptr->background.blue = (xpng_uint_16)(pow(
                  (double)xpng_ptr->background.blue / m, gs) * m + .5);
            }
            else
            {
               /* GRAY, GRAY ALPHA, RGB, or RGBA with gray background */
               xpng_ptr->background_1.red = xpng_ptr->background_1.green
                 = xpng_ptr->background_1.blue = xpng_ptr->background_1.gray;
               xpng_ptr->background.red = xpng_ptr->background.green
                 = xpng_ptr->background.blue = xpng_ptr->background.gray;
            }
         }
      }
      else
      /* transformation does not include PNG_BACKGROUND */
#endif /* PNG_READ_BACKGROUND_SUPPORTED */
      if (color_type == PNG_COLOR_TYPE_PALETTE)
      {
         xpng_colorp palette = xpng_ptr->palette;
         int num_palette = xpng_ptr->num_palette;
         int i;

         for (i = 0; i < num_palette; i++)
         {
            palette[i].red = xpng_ptr->gamma_table[palette[i].red];
            palette[i].green = xpng_ptr->gamma_table[palette[i].green];
            palette[i].blue = xpng_ptr->gamma_table[palette[i].blue];
         }
      }
   }
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   else
#endif
#endif /* PNG_READ_GAMMA_SUPPORTED && PNG_FLOATING_POINT_SUPPORTED */
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   /* No GAMMA transformation */
   if ((xpng_ptr->transformations & PNG_BACKGROUND) &&
       (color_type == PNG_COLOR_TYPE_PALETTE))
   {
      int i;
      int istop = (int)xpng_ptr->num_trans;
      xpng_color back;
      xpng_colorp palette = xpng_ptr->palette;

      back.red   = (xpng_byte)xpng_ptr->background.red;
      back.green = (xpng_byte)xpng_ptr->background.green;
      back.blue  = (xpng_byte)xpng_ptr->background.blue;

      for (i = 0; i < istop; i++)
      {
         if (xpng_ptr->trans[i] == 0)
         {
            palette[i] = back;
         }
         else if (xpng_ptr->trans[i] != 0xff)
         {
            /* The xpng_composite() macro is defined in png.h */
            xpng_composite(palette[i].red, palette[i].red,
               xpng_ptr->trans[i], back.red);
            xpng_composite(palette[i].green, palette[i].green,
               xpng_ptr->trans[i], back.green);
            xpng_composite(palette[i].blue, palette[i].blue,
               xpng_ptr->trans[i], back.blue);
         }
      }
   }
#endif /* PNG_READ_BACKGROUND_SUPPORTED */

#if defined(PNG_READ_SHIFT_SUPPORTED)
   if ((xpng_ptr->transformations & PNG_SHIFT) &&
      (color_type == PNG_COLOR_TYPE_PALETTE))
   {
      xpng_uint_16 i;
      xpng_uint_16 istop = xpng_ptr->num_palette;
      int sr = 8 - xpng_ptr->sig_bit.red;
      int sg = 8 - xpng_ptr->sig_bit.green;
      int sb = 8 - xpng_ptr->sig_bit.blue;

      if (sr < 0 || sr > 8)
         sr = 0;
      if (sg < 0 || sg > 8)
         sg = 0;
      if (sb < 0 || sb > 8)
         sb = 0;
      for (i = 0; i < istop; i++)
      {
         xpng_ptr->palette[i].red >>= sr;
         xpng_ptr->palette[i].green >>= sg;
         xpng_ptr->palette[i].blue >>= sb;
      }
   }
#endif  /* PNG_READ_SHIFT_SUPPORTED */
 }
#if !defined(PNG_READ_GAMMA_SUPPORTED) && !defined(PNG_READ_SHIFT_SUPPORTED) \
 && !defined(PNG_READ_BACKGROUND_SUPPORTED)
   if(xpng_ptr)
      return;
#endif
}

/* Modify the info structure to reflect the transformations.  The
 * info should be updated so a PNG file could be written with it,
 * assuming the transformations result in valid PNG data.
 */
void /* PRIVATE */
xpng_read_transform_info(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   xpng_debug(1, "in xpng_read_transform_info\n");
#if defined(PNG_READ_EXPAND_SUPPORTED)
   if (xpng_ptr->transformations & PNG_EXPAND)
   {
      if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         if (xpng_ptr->num_trans)
            info_ptr->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
         else
            info_ptr->color_type = PNG_COLOR_TYPE_RGB;
         info_ptr->bit_depth = 8;
         info_ptr->num_trans = 0;
      }
      else
      {
         if (xpng_ptr->num_trans)
            info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;
         if (info_ptr->bit_depth < 8)
            info_ptr->bit_depth = 8;
         info_ptr->num_trans = 0;
      }
   }
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   if (xpng_ptr->transformations & PNG_BACKGROUND)
   {
      info_ptr->color_type &= ~PNG_COLOR_MASK_ALPHA;
      info_ptr->num_trans = 0;
      info_ptr->background = xpng_ptr->background;
   }
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
   if (xpng_ptr->transformations & PNG_GAMMA)
   {
#ifdef PNG_FLOATING_POINT_SUPPORTED
      info_ptr->gamma = xpng_ptr->gamma;
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
      info_ptr->int_gamma = xpng_ptr->int_gamma;
#endif
   }
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
   if ((xpng_ptr->transformations & PNG_16_TO_8) && (info_ptr->bit_depth == 16))
      info_ptr->bit_depth = 8;
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
   if (xpng_ptr->transformations & PNG_DITHER)
   {
      if (((info_ptr->color_type == PNG_COLOR_TYPE_RGB) ||
         (info_ptr->color_type == PNG_COLOR_TYPE_RGB_ALPHA)) &&
         xpng_ptr->palette_lookup && info_ptr->bit_depth == 8)
      {
         info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
      }
   }
#endif

#if defined(PNG_READ_PACK_SUPPORTED)
   if ((xpng_ptr->transformations & PNG_PACK) && (info_ptr->bit_depth < 8))
      info_ptr->bit_depth = 8;
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
   if (xpng_ptr->transformations & PNG_GRAY_TO_RGB)
      info_ptr->color_type |= PNG_COLOR_MASK_COLOR;
#endif

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
   if (xpng_ptr->transformations & PNG_RGB_TO_GRAY)
      info_ptr->color_type &= ~PNG_COLOR_MASK_COLOR;
#endif

   if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      info_ptr->channels = 1;
   else if (info_ptr->color_type & PNG_COLOR_MASK_COLOR)
      info_ptr->channels = 3;
   else
      info_ptr->channels = 1;

#if defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
   if (xpng_ptr->transformations & PNG_STRIP_ALPHA)
      info_ptr->color_type &= ~PNG_COLOR_MASK_ALPHA;
#endif

   if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA)
      info_ptr->channels++;

#if defined(PNG_READ_FILLER_SUPPORTED)
   /* STRIP_ALPHA and FILLER allowed:  MASK_ALPHA bit stripped above */
   if ((xpng_ptr->transformations & PNG_FILLER) &&
       ((info_ptr->color_type == PNG_COLOR_TYPE_RGB) ||
       (info_ptr->color_type == PNG_COLOR_TYPE_GRAY)))
   {
      info_ptr->channels++;
#if 0 /* if adding a true alpha channel not just filler */
      info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;
#endif
   }
#endif

#if defined(PNG_USER_TRANSFORM_PTR_SUPPORTED) && \
defined(PNG_READ_USER_TRANSFORM_SUPPORTED)
   if(xpng_ptr->transformations & PNG_USER_TRANSFORM)
     {
       if(info_ptr->bit_depth < xpng_ptr->user_transform_depth)
         info_ptr->bit_depth = xpng_ptr->user_transform_depth;
       if(info_ptr->channels < xpng_ptr->user_transform_channels)
         info_ptr->channels = xpng_ptr->user_transform_channels;
     }
#endif

   info_ptr->pixel_depth = (xpng_byte)(info_ptr->channels *
      info_ptr->bit_depth);
   info_ptr->rowbytes = ((info_ptr->width * info_ptr->pixel_depth + 7) >> 3);

#if !defined(PNG_READ_EXPAND_SUPPORTED)
   if(xpng_ptr)
      return;
#endif
}

/* Transform the row.  The order of transformations is significant,
 * and is very touchy.  If you add a transformation, take care to
 * decide how it fits in with the other transformations here.
 */
void /* PRIVATE */
xpng_do_read_transformations(xpng_structp xpng_ptr)
{
   xpng_debug(1, "in xpng_do_read_transformations\n");
#if !defined(PNG_USELESS_TESTS_SUPPORTED)
   if (xpng_ptr->row_buf == NULL)
   {
#if !defined(PNG_NO_STDIO) && !defined(_WIN32_WCE)
      char msg[50];

      sprintf(msg, "NULL row buffer for row %ld, pass %d", xpng_ptr->row_number,
         xpng_ptr->pass);
      xpng_error(xpng_ptr, msg);
#else
      xpng_error(xpng_ptr, "NULL row buffer");
#endif
   }
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
   if (xpng_ptr->transformations & PNG_EXPAND)
   {
      if (xpng_ptr->row_info.color_type == PNG_COLOR_TYPE_PALETTE)
      {
         xpng_do_expand_palette(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
            xpng_ptr->palette, xpng_ptr->trans, xpng_ptr->num_trans);
      }
      else
      {
         if (xpng_ptr->num_trans)
            xpng_do_expand(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
               &(xpng_ptr->trans_values));
         else
            xpng_do_expand(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
               NULL);
      }
   }
#endif

#if defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
   if (xpng_ptr->transformations & PNG_STRIP_ALPHA)
      xpng_do_strip_filler(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
         PNG_FLAG_FILLER_AFTER);
#endif

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
   if (xpng_ptr->transformations & PNG_RGB_TO_GRAY)
   {
      int rgb_error =
         xpng_do_rgb_to_gray(xpng_ptr, &(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
      if(rgb_error)
      {
         xpng_ptr->rgb_to_gray_status=1;
         if(xpng_ptr->transformations == PNG_RGB_TO_GRAY_WARN)
            xpng_warning(xpng_ptr, "xpng_do_rgb_to_gray found nongray pixel");
         if(xpng_ptr->transformations == PNG_RGB_TO_GRAY_ERR)
            xpng_error(xpng_ptr, "xpng_do_rgb_to_gray found nongray pixel");
      }
   }
#endif

/*
From Andreas Dilger e-mail to png-implement, 26 March 1998:

  In most cases, the "simple transparency" should be done prior to doing
  gray-to-RGB, or you will have to test 3x as many bytes to check if a
  pixel is transparent.  You would also need to make sure that the
  transparency information is upgraded to RGB.

  To summarize, the current flow is:
  - Gray + simple transparency -> compare 1 or 2 gray bytes and composite
                                  with background "in place" if transparent,
                                  convert to RGB if necessary
  - Gray + alpha -> composite with gray background and remove alpha bytes,
                                  convert to RGB if necessary

  To support RGB backgrounds for gray images we need:
  - Gray + simple transparency -> convert to RGB + simple transparency, compare
                                  3 or 6 bytes and composite with background
                                  "in place" if transparent (3x compare/pixel
                                  compared to doing composite with gray bkgrnd)
  - Gray + alpha -> convert to RGB + alpha, composite with background and
                                  remove alpha bytes (3x float operations/pixel
                                  compared with composite on gray background)

  Greg's change will do this.  The reason it wasn't done before is for
  performance, as this increases the per-pixel operations.  If we would check
  in advance if the background was gray or RGB, and position the gray-to-RGB
  transform appropriately, then it would save a lot of work/time.
 */

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
   /* if gray -> RGB, do so now only if background is non-gray; else do later
    * for performance reasons */
   if ((xpng_ptr->transformations & PNG_GRAY_TO_RGB) &&
       !(xpng_ptr->mode & PNG_BACKGROUND_IS_GRAY))
      xpng_do_gray_to_rgb(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   if ((xpng_ptr->transformations & PNG_BACKGROUND) &&
      ((xpng_ptr->num_trans != 0 ) ||
      (xpng_ptr->color_type & PNG_COLOR_MASK_ALPHA)))
      xpng_do_background(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
         &(xpng_ptr->trans_values), &(xpng_ptr->background)
#if defined(PNG_READ_GAMMA_SUPPORTED)
         , &(xpng_ptr->background_1),
         xpng_ptr->gamma_table, xpng_ptr->gamma_from_1,
         xpng_ptr->gamma_to_1, xpng_ptr->gamma_16_table,
         xpng_ptr->gamma_16_from_1, xpng_ptr->gamma_16_to_1,
         xpng_ptr->gamma_shift
#endif
);
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
   if ((xpng_ptr->transformations & PNG_GAMMA) &&
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
      !((xpng_ptr->transformations & PNG_BACKGROUND) &&
      ((xpng_ptr->num_trans != 0) ||
      (xpng_ptr->color_type & PNG_COLOR_MASK_ALPHA))) &&
#endif
      (xpng_ptr->color_type != PNG_COLOR_TYPE_PALETTE))
      xpng_do_gamma(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
         xpng_ptr->gamma_table, xpng_ptr->gamma_16_table,
         xpng_ptr->gamma_shift);
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
   if (xpng_ptr->transformations & PNG_16_TO_8)
      xpng_do_chop(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
   if (xpng_ptr->transformations & PNG_DITHER)
   {
      xpng_do_dither((xpng_row_infop)&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
         xpng_ptr->palette_lookup, xpng_ptr->dither_index);
      if(xpng_ptr->row_info.rowbytes == (xpng_uint_32)0)
         xpng_error(xpng_ptr, "xpng_do_dither returned rowbytes=0");
   }
#endif

#if defined(PNG_READ_INVERT_SUPPORTED)
   if (xpng_ptr->transformations & PNG_INVERT_MONO)
      xpng_do_invert(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
   if (xpng_ptr->transformations & PNG_SHIFT)
      xpng_do_unshift(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
         &(xpng_ptr->shift));
#endif

#if defined(PNG_READ_PACK_SUPPORTED)
   if (xpng_ptr->transformations & PNG_PACK)
      xpng_do_unpack(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_BGR_SUPPORTED)
   if (xpng_ptr->transformations & PNG_BGR)
      xpng_do_bgr(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED)
   if (xpng_ptr->transformations & PNG_PACKSWAP)
      xpng_do_packswap(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
   /* if gray -> RGB, do so now only if we did not do so above */
   if ((xpng_ptr->transformations & PNG_GRAY_TO_RGB) &&
       (xpng_ptr->mode & PNG_BACKGROUND_IS_GRAY))
      xpng_do_gray_to_rgb(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_FILLER_SUPPORTED)
   if (xpng_ptr->transformations & PNG_FILLER)
      xpng_do_read_filler(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1,
         (xpng_uint_32)xpng_ptr->filler, xpng_ptr->flags);
#endif

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED)
   if (xpng_ptr->transformations & PNG_INVERT_ALPHA)
      xpng_do_read_invert_alpha(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_SWAP_ALPHA_SUPPORTED)
   if (xpng_ptr->transformations & PNG_SWAP_ALPHA)
      xpng_do_read_swap_alpha(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_SWAP_SUPPORTED)
   if (xpng_ptr->transformations & PNG_SWAP_BYTES)
      xpng_do_swap(&(xpng_ptr->row_info), xpng_ptr->row_buf + 1);
#endif

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED)
   if (xpng_ptr->transformations & PNG_USER_TRANSFORM)
    {
      if(xpng_ptr->read_user_transform_fn != NULL)
        (*(xpng_ptr->read_user_transform_fn)) /* user read transform function */
          (xpng_ptr,                    /* xpng_ptr */
           &(xpng_ptr->row_info),       /* row_info:     */
             /*  xpng_uint_32 width;          width of row */
             /*  xpng_uint_32 rowbytes;       number of bytes in row */
             /*  xpng_byte color_type;        color type of pixels */
             /*  xpng_byte bit_depth;         bit depth of samples */
             /*  xpng_byte channels;          number of channels (1-4) */
             /*  xpng_byte pixel_depth;       bits per pixel (depth*channels) */
           xpng_ptr->row_buf + 1);      /* start of pixel data for row */
#if defined(PNG_USER_TRANSFORM_PTR_SUPPORTED)
      if(xpng_ptr->user_transform_depth)
         xpng_ptr->row_info.bit_depth = xpng_ptr->user_transform_depth;
      if(xpng_ptr->user_transform_channels)
         xpng_ptr->row_info.channels = xpng_ptr->user_transform_channels;
#endif
      xpng_ptr->row_info.pixel_depth = (xpng_byte)(xpng_ptr->row_info.bit_depth *
         xpng_ptr->row_info.channels);
      xpng_ptr->row_info.rowbytes = (xpng_ptr->row_info.width *
         xpng_ptr->row_info.pixel_depth+7)>>3;
   }
#endif

}

#if defined(PNG_READ_PACK_SUPPORTED)
/* Unpack pixels of 1, 2, or 4 bits per pixel into 1 byte per pixel,
 * without changing the actual values.  Thus, if you had a row with
 * a bit depth of 1, you would end up with bytes that only contained
 * the numbers 0 or 1.  If you would rather they contain 0 and 255, use
 * xpng_do_shift() after this.
 */
void /* PRIVATE */
xpng_do_unpack(xpng_row_infop row_info, xpng_bytep row)
{
   xpng_debug(1, "in xpng_do_unpack\n");
#if defined(PNG_USELESS_TESTS_SUPPORTED)
   if (row != NULL && row_info != NULL && row_info->bit_depth < 8)
#else
   if (row_info->bit_depth < 8)
#endif
   {
      xpng_uint_32 i;
      xpng_uint_32 row_width=row_info->width;

      switch (row_info->bit_depth)
      {
         case 1:
         {
            xpng_bytep sp = row + (xpng_size_t)((row_width - 1) >> 3);
            xpng_bytep dp = row + (xpng_size_t)row_width - 1;
            xpng_uint_32 shift = 7 - (int)((row_width + 7) & 0x07);
            for (i = 0; i < row_width; i++)
            {
               *dp = (xpng_byte)((*sp >> shift) & 0x01);
               if (shift == 7)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift++;

               dp--;
            }
            break;
         }
         case 2:
         {

            xpng_bytep sp = row + (xpng_size_t)((row_width - 1) >> 2);
            xpng_bytep dp = row + (xpng_size_t)row_width - 1;
            xpng_uint_32 shift = (int)((3 - ((row_width + 3) & 0x03)) << 1);
            for (i = 0; i < row_width; i++)
            {
               *dp = (xpng_byte)((*sp >> shift) & 0x03);
               if (shift == 6)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift += 2;

               dp--;
            }
            break;
         }
         case 4:
         {
            xpng_bytep sp = row + (xpng_size_t)((row_width - 1) >> 1);
            xpng_bytep dp = row + (xpng_size_t)row_width - 1;
            xpng_uint_32 shift = (int)((1 - ((row_width + 1) & 0x01)) << 2);
            for (i = 0; i < row_width; i++)
            {
               *dp = (xpng_byte)((*sp >> shift) & 0x0f);
               if (shift == 4)
               {
                  shift = 0;
                  sp--;
               }
               else
                  shift = 4;

               dp--;
            }
            break;
         }
      }
      row_info->bit_depth = 8;
      row_info->pixel_depth = (xpng_byte)(8 * row_info->channels);
      row_info->rowbytes = row_width * row_info->channels;
   }
}
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
/* Reverse the effects of xpng_do_shift.  This routine merely shifts the
 * pixels back to their significant bits values.  Thus, if you have
 * a row of bit depth 8, but only 5 are significant, this will shift
 * the values back to 0 through 31.
 */
void /* PRIVATE */
xpng_do_unshift(xpng_row_infop row_info, xpng_bytep row, xpng_color_8p sig_bits)
{
   xpng_debug(1, "in xpng_do_unshift\n");
   if (
#if defined(PNG_USELESS_TESTS_SUPPORTED)
       row != NULL && row_info != NULL && sig_bits != NULL &&
#endif
       row_info->color_type != PNG_COLOR_TYPE_PALETTE)
   {
      int shift[4];
      int channels = 0;
      int c;
      xpng_uint_16 value = 0;
      xpng_uint_32 row_width = row_info->width;

      if (row_info->color_type & PNG_COLOR_MASK_COLOR)
      {
         shift[channels++] = row_info->bit_depth - sig_bits->red;
         shift[channels++] = row_info->bit_depth - sig_bits->green;
         shift[channels++] = row_info->bit_depth - sig_bits->blue;
      }
      else
      {
         shift[channels++] = row_info->bit_depth - sig_bits->gray;
      }
      if (row_info->color_type & PNG_COLOR_MASK_ALPHA)
      {
         shift[channels++] = row_info->bit_depth - sig_bits->alpha;
      }

      for (c = 0; c < channels; c++)
      {
         if (shift[c] <= 0)
            shift[c] = 0;
         else
            value = 1;
      }

      if (!value)
         return;

      switch (row_info->bit_depth)
      {
         case 2:
         {
            xpng_bytep bp;
            xpng_uint_32 i;
            xpng_uint_32 istop = row_info->rowbytes;

            for (bp = row, i = 0; i < istop; i++)
            {
               *bp >>= 1;
               *bp++ &= 0x55;
            }
            break;
         }
         case 4:
         {
            xpng_bytep bp = row;
            xpng_uint_32 i;
            xpng_uint_32 istop = row_info->rowbytes;
            xpng_byte mask = (xpng_byte)((((int)0xf0 >> shift[0]) & (int)0xf0) |
               (xpng_byte)((int)0xf >> shift[0]));

            for (i = 0; i < istop; i++)
            {
               *bp >>= shift[0];
               *bp++ &= mask;
            }
            break;
         }
         case 8:
         {
            xpng_bytep bp = row;
            xpng_uint_32 i;
            xpng_uint_32 istop = row_width * channels;

            for (i = 0; i < istop; i++)
            {
               *bp++ >>= shift[i%channels];
            }
            break;
         }
         case 16:
         {
            xpng_bytep bp = row;
            xpng_uint_32 i;
            xpng_uint_32 istop = channels * row_width;

            for (i = 0; i < istop; i++)
            {
               value = (xpng_uint_16)((*bp << 8) + *(bp + 1));
               value >>= shift[i%channels];
               *bp++ = (xpng_byte)(value >> 8);
               *bp++ = (xpng_byte)(value & 0xff);
            }
            break;
         }
      }
   }
}
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
/* chop rows of bit depth 16 down to 8 */
void /* PRIVATE */
xpng_do_chop(xpng_row_infop row_info, xpng_bytep row)
{
   xpng_debug(1, "in xpng_do_chop\n");
#if defined(PNG_USELESS_TESTS_SUPPORTED)
   if (row != NULL && row_info != NULL && row_info->bit_depth == 16)
#else
   if (row_info->bit_depth == 16)
#endif
   {
      xpng_bytep sp = row;
      xpng_bytep dp = row;
      xpng_uint_32 i;
      xpng_uint_32 istop = row_info->width * row_info->channels;

      for (i = 0; i<istop; i++, sp += 2, dp++)
      {
#if defined(PNG_READ_16_TO_8_ACCURATE_SCALE_SUPPORTED)
      /* This does a more accurate scaling of the 16-bit color
       * value, rather than a simple low-byte truncation.
       *
       * What the ideal calculation should be:
       *   *dp = (((((xpng_uint_32)(*sp) << 8) |
       *          (xpng_uint_32)(*(sp + 1))) * 255 + 127) / (xpng_uint_32)65535L;
       *
       * GRR: no, I think this is what it really should be:
       *   *dp = (((((xpng_uint_32)(*sp) << 8) |
       *           (xpng_uint_32)(*(sp + 1))) + 128L) / (xpng_uint_32)257L;
       *
       * GRR: here's the exact calculation with shifts:
       *   temp = (((xpng_uint_32)(*sp) << 8) | (xpng_uint_32)(*(sp + 1))) + 128L;
       *   *dp = (temp - (temp >> 8)) >> 8;
       *
       * Approximate calculation with shift/add instead of multiply/divide:
       *   *dp = ((((xpng_uint_32)(*sp) << 8) |
       *          (xpng_uint_32)((int)(*(sp + 1)) - *sp)) + 128) >> 8;
       *
       * What we actually do to avoid extra shifting and conversion:
       */

         *dp = *sp + ((((int)(*(sp + 1)) - *sp) > 128) ? 1 : 0);
#else
       /* Simply discard the low order byte */
         *dp = *sp;
#endif
      }
      row_info->bit_depth = 8;
      row_info->pixel_depth = (xpng_byte)(8 * row_info->channels);
      row_info->rowbytes = row_info->width * row_info->channels;
   }
}
#endif

#if defined(PNG_READ_SWAP_ALPHA_SUPPORTED)
void /* PRIVATE */
xpng_do_read_swap_alpha(xpng_row_infop row_info, xpng_bytep row)
{
   xpng_debug(1, "in xpng_do_read_swap_alpha\n");
#if defined(PNG_USELESS_TESTS_SUPPORTED)
   if (row != NULL && row_info != NULL)
#endif
   {
      xpng_uint_32 row_width = row_info->width;
      if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
      {
         /* This converts from RGBA to ARGB */
         if (row_info->bit_depth == 8)
         {
            xpng_bytep sp = row + row_info->rowbytes;
            xpng_bytep dp = sp;
            xpng_byte save;
            xpng_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               save = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = save;
            }
         }
         /* This converts from RRGGBBAA to AARRGGBB */
         else
         {
            xpng_bytep sp = row + row_info->rowbytes;
            xpng_bytep dp = sp;
            xpng_byte save[2];
            xpng_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               save[0] = *(--sp);
               save[1] = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = save[0];
               *(--dp) = save[1];
            }
         }
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         /* This converts from GA to AG */
         if (row_info->bit_depth == 8)
         {
            xpng_bytep sp = row + row_info->rowbytes;
            xpng_bytep dp = sp;
            xpng_byte save;
            xpng_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               save = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = save;
            }
         }
         /* This converts from GGAA to AAGG */
         else
         {
            xpng_bytep sp = row + row_info->rowbytes;
            xpng_bytep dp = sp;
            xpng_byte save[2];
            xpng_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               save[0] = *(--sp);
               save[1] = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = save[0];
               *(--dp) = save[1];
            }
         }
      }
   }
}
#endif

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED)
void /* PRIVATE */
xpng_do_read_invert_alpha(xpng_row_infop row_info, xpng_bytep row)
{
   xpng_debug(1, "in xpng_do_read_invert_alpha\n");
#if defined(PNG_USELESS_TESTS_SUPPORTED)
   if (row != NULL && row_info != NULL)
#endif
   {
      xpng_uint_32 row_width = row_info->width;
      if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
      {
         /* This inverts the alpha channel in RGBA */
         if (row_info->bit_depth == 8)
         {
            xpng_bytep sp = row + row_info->rowbytes;
            xpng_bytep dp = sp;
            xpng_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               *(--dp) = (xpng_byte)(255 - *(--sp));

/*             This does nothing:
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               We can replace it with:
*/
               sp-=3;
               dp=sp;
            }
         }
         /* This inverts the alpha channel in RRGGBBAA */
         else
         {
            xpng_bytep sp = row + row_info->rowbytes;
            xpng_bytep dp = sp;
            xpng_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               *(--dp) = (xpng_byte)(255 - *(--sp));
               *(--dp) = (xpng_byte)(255 - *(--sp));

/*             This does nothing:
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               We can replace it with:
*/
               sp-=6;
               dp=sp;
            }
         }
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         /* This inverts the alpha channel in GA */
         if (row_info->bit_depth == 8)
         {
            xpng_bytep sp = row + row_info->rowbytes;
            xpng_bytep dp = sp;
            xpng_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               *(--dp) = (xpng_byte)(255 - *(--sp));
               *(--dp) = *(--sp);
            }
         }
         /* This inverts the alpha channel in GGAA */
         else
         {
            xpng_bytep sp  = row + row_info->rowbytes;
            xpng_bytep dp = sp;
            xpng_uint_32 i;

            for (i = 0; i < row_width; i++)
            {
               *(--dp) = (xpng_byte)(255 - *(--sp));
               *(--dp) = (xpng_byte)(255 - *(--sp));
/*
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
*/
               sp-=2;
               dp=sp;
            }
         }
      }
   }
}
#endif

#if defined(PNG_READ_FILLER_SUPPORTED)
/* Add filler channel if we have RGB color */
void /* PRIVATE */
xpng_do_read_filler(xpng_row_infop row_info, xpng_bytep row,
   xpng_uint_32 filler, xpng_uint_32 flags)
{
   xpng_uint_32 i;
   xpng_uint_32 row_width = row_info->width;

   xpng_byte hi_filler = (xpng_byte)((filler>>8) & 0xff);
   xpng_byte lo_filler = (xpng_byte)(filler & 0xff);

   xpng_debug(1, "in xpng_do_read_filler\n");
   if (
#if defined(PNG_USELESS_TESTS_SUPPORTED)
       row != NULL  && row_info != NULL &&
#endif
       row_info->color_type == PNG_COLOR_TYPE_GRAY)
   {
      if(row_info->bit_depth == 8)
      {
         /* This changes the data from G to GX */
         if (flags & PNG_FLAG_FILLER_AFTER)
         {
            xpng_bytep sp = row + (xpng_size_t)row_width;
            xpng_bytep dp =  sp + (xpng_size_t)row_width;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            row_info->channels = 2;
            row_info->pixel_depth = 16;
            row_info->rowbytes = row_width * 2;
         }
      /* This changes the data from G to XG */
         else
         {
            xpng_bytep sp = row + (xpng_size_t)row_width;
            xpng_bytep dp = sp  + (xpng_size_t)row_width;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
            }
            row_info->channels = 2;
            row_info->pixel_depth = 16;
            row_info->rowbytes = row_width * 2;
         }
      }
      else if(row_info->bit_depth == 16)
      {
         /* This changes the data from GG to GGXX */
         if (flags & PNG_FLAG_FILLER_AFTER)
         {
            xpng_bytep sp = row + (xpng_size_t)row_width;
            xpng_bytep dp = sp  + (xpng_size_t)row_width;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = hi_filler;
               *(--dp) = lo_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = hi_filler;
            *(--dp) = lo_filler;
            row_info->channels = 2;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }
         /* This changes the data from GG to XXGG */
         else
         {
            xpng_bytep sp = row + (xpng_size_t)row_width;
            xpng_bytep dp = sp  + (xpng_size_t)row_width;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = hi_filler;
               *(--dp) = lo_filler;
            }
            row_info->channels = 2;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }
      }
   } /* COLOR_TYPE == GRAY */
   else if (row_info->color_type == PNG_COLOR_TYPE_RGB)
   {
      if(row_info->bit_depth == 8)
      {
         /* This changes the data from RGB to RGBX */
         if (flags & PNG_FLAG_FILLER_AFTER)
         {
            xpng_bytep sp = row + (xpng_size_t)row_width * 3;
            xpng_bytep dp = sp  + (xpng_size_t)row_width;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = lo_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = lo_filler;
            row_info->channels = 4;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }
      /* This changes the data from RGB to XRGB */
         else
         {
            xpng_bytep sp = row + (xpng_size_t)row_width * 3;
            xpng_bytep dp = sp + (xpng_size_t)row_width;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = lo_filler;
            }
            row_info->channels = 4;
            row_info->pixel_depth = 32;
            row_info->rowbytes = row_width * 4;
         }
      }
      else if(row_info->bit_depth == 16)
      {
         /* This changes the data from RRGGBB to RRGGBBXX */
         if (flags & PNG_FLAG_FILLER_AFTER)
         {
            xpng_bytep sp = row + (xpng_size_t)row_width * 3;
            xpng_bytep dp = sp  + (xpng_size_t)row_width;
            for (i = 1; i < row_width; i++)
            {
               *(--dp) = hi_filler;
               *(--dp) = lo_filler;
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
            }
            *(--dp) = hi_filler;
            *(--dp) = lo_filler;
            row_info->channels = 4;
            row_info->pixel_depth = 64;
            row_info->rowbytes = row_width * 8;
         }
         /* This changes the data from RRGGBB to XXRRGGBB */
         else
         {
            xpng_bytep sp = row + (xpng_size_t)row_width * 3;
            xpng_bytep dp = sp  + (xpng_size_t)row_width;
            for (i = 0; i < row_width; i++)
            {
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = *(--sp);
               *(--dp) = hi_filler;
               *(--dp) = lo_filler;
            }
            row_info->channels = 4;
            row_info->pixel_depth = 64;
            row_info->rowbytes = row_width * 8;
         }
      }
   } /* COLOR_TYPE == RGB */
}
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
/* expand grayscale files to RGB, with or without alpha */
void /* PRIVATE */
xpng_do_gray_to_rgb(xpng_row_infop row_info, xpng_bytep row)
{
   xpng_uint_32 i;
   xpng_uint_32 row_width = row_info->width;

   xpng_debug(1, "in xpng_do_gray_to_rgb\n");
   if (row_info->bit_depth >= 8 &&
#if defined(PNG_USELESS_TESTS_SUPPORTED)
       row != NULL && row_info != NULL &&
#endif
      !(row_info->color_type & PNG_COLOR_MASK_COLOR))
   {
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY)
      {
         if (row_info->bit_depth == 8)
         {
            xpng_bytep sp = row + (xpng_size_t)row_width - 1;
            xpng_bytep dp = sp  + (xpng_size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *(sp--);
            }
         }
         else
         {
            xpng_bytep sp = row + (xpng_size_t)row_width * 2 - 1;
            xpng_bytep dp = sp  + (xpng_size_t)row_width * 4;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
            }
         }
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      {
         if (row_info->bit_depth == 8)
         {
            xpng_bytep sp = row + (xpng_size_t)row_width * 2 - 1;
            xpng_bytep dp = sp  + (xpng_size_t)row_width * 2;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *sp;
               *(dp--) = *(sp--);
            }
         }
         else
         {
            xpng_bytep sp = row + (xpng_size_t)row_width * 4 - 1;
            xpng_bytep dp = sp  + (xpng_size_t)row_width * 4;
            for (i = 0; i < row_width; i++)
            {
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *sp;
               *(dp--) = *(sp - 1);
               *(dp--) = *(sp--);
               *(dp--) = *(sp--);
            }
         }
      }
      row_info->channels += (xpng_byte)2;
      row_info->color_type |= PNG_COLOR_MASK_COLOR;
      row_info->pixel_depth = (xpng_byte)(row_info->channels *
         row_info->bit_depth);
      row_info->rowbytes = ((row_width *
         row_info->pixel_depth + 7) >> 3);
   }
}
#endif

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
/* reduce RGB files to grayscale, with or without alpha
 * using the equation given in Poynton's ColorFAQ at
 * <http://www.inforamp.net/~poynton/>
 * Copyright (c) 1998-01-04 Charles Poynton poynton@inforamp.net
 *
 *     Y = 0.212671 * R + 0.715160 * G + 0.072169 * B
 *
 *  We approximate this with
 *
 *     Y = 0.21268 * R    + 0.7151 * G    + 0.07217 * B
 *
 *  which can be expressed with integers as
 *
 *     Y = (6969 * R + 23434 * G + 2365 * B)/32768
 *
 *  The calculation is to be done in a linear colorspace.
 *
 *  Other integer coefficents can be used via xpng_set_rgb_to_gray().
 */
int /* PRIVATE */
xpng_do_rgb_to_gray(xpng_structp xpng_ptr, xpng_row_infop row_info, xpng_bytep row)

{
   xpng_uint_32 i;

   xpng_uint_32 row_width = row_info->width;
   int rgb_error = 0;

   xpng_debug(1, "in xpng_do_rgb_to_gray\n");
   if (
#if defined(PNG_USELESS_TESTS_SUPPORTED)
       row != NULL && row_info != NULL &&
#endif
      (row_info->color_type & PNG_COLOR_MASK_COLOR))
   {
      xpng_uint_32 rc = xpng_ptr->rgb_to_gray_red_coeff;
      xpng_uint_32 gc = xpng_ptr->rgb_to_gray_green_coeff;
      xpng_uint_32 bc = xpng_ptr->rgb_to_gray_blue_coeff;

      if (row_info->color_type == PNG_COLOR_TYPE_RGB)
      {
         if (row_info->bit_depth == 8)
         {
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
            if (xpng_ptr->gamma_from_1 != NULL && xpng_ptr->gamma_to_1 != NULL)
            {
               xpng_bytep sp = row;
               xpng_bytep dp = row;

               for (i = 0; i < row_width; i++)
               {
                  xpng_byte red   = xpng_ptr->gamma_to_1[*(sp++)];
                  xpng_byte green = xpng_ptr->gamma_to_1[*(sp++)];
                  xpng_byte blue  = xpng_ptr->gamma_to_1[*(sp++)];
                  if(red != green || red != blue)
                  {
                     rgb_error |= 1;
                     *(dp++) = xpng_ptr->gamma_from_1[
                       (rc*red+gc*green+bc*blue)>>15];
                  }
                  else
                     *(dp++) = *(sp-1);
               }
            }
            else
#endif
            {
               xpng_bytep sp = row;
               xpng_bytep dp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_byte red   = *(sp++);
                  xpng_byte green = *(sp++);
                  xpng_byte blue  = *(sp++);
                  if(red != green || red != blue)
                  {
                     rgb_error |= 1;
                     *(dp++) = (xpng_byte)((rc*red+gc*green+bc*blue)>>15);
                  }
                  else
                     *(dp++) = *(sp-1);
               }
            }
         }

         else /* RGB bit_depth == 16 */
         {
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
            if (xpng_ptr->gamma_16_to_1 != NULL &&
                xpng_ptr->gamma_16_from_1 != NULL)
            {
               xpng_bytep sp = row;
               xpng_bytep dp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_uint_16 red, green, blue, w;

                  red   = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;
                  green = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;
                  blue  = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;

                  if(red == green && red == blue)
                     w = red;
                  else
                  {
                     xpng_uint_16 red_1   = xpng_ptr->gamma_16_to_1[(red&0xff) >>
                                  xpng_ptr->gamma_shift][red>>8];
                     xpng_uint_16 green_1 = xpng_ptr->gamma_16_to_1[(green&0xff) >>
                                  xpng_ptr->gamma_shift][green>>8];
                     xpng_uint_16 blue_1  = xpng_ptr->gamma_16_to_1[(blue&0xff) >>
                                  xpng_ptr->gamma_shift][blue>>8];
                     xpng_uint_16 gray16  = (xpng_uint_16)((rc*red_1 + gc*green_1
                                  + bc*blue_1)>>15);
                     w = xpng_ptr->gamma_16_from_1[(gray16&0xff) >>
                         xpng_ptr->gamma_shift][gray16 >> 8];
                     rgb_error |= 1;
                  }

                  *(dp++) = (xpng_byte)((w>>8) & 0xff);
                  *(dp++) = (xpng_byte)(w & 0xff);
               }
            }
            else
#endif
            {
               xpng_bytep sp = row;
               xpng_bytep dp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_uint_16 red, green, blue, gray16;

                  red   = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;
                  green = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;
                  blue  = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;

                  if(red != green || red != blue)
                     rgb_error |= 1;
                  gray16  = (xpng_uint_16)((rc*red + gc*green + bc*blue)>>15);
                  *(dp++) = (xpng_byte)((gray16>>8) & 0xff);
                  *(dp++) = (xpng_byte)(gray16 & 0xff);
               }
            }
         }
      }
      if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
      {
         if (row_info->bit_depth == 8)
         {
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
            if (xpng_ptr->gamma_from_1 != NULL && xpng_ptr->gamma_to_1 != NULL)
            {
               xpng_bytep sp = row;
               xpng_bytep dp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_byte red   = xpng_ptr->gamma_to_1[*(sp++)];
                  xpng_byte green = xpng_ptr->gamma_to_1[*(sp++)];
                  xpng_byte blue  = xpng_ptr->gamma_to_1[*(sp++)];
                  if(red != green || red != blue)
                     rgb_error |= 1;
                  *(dp++) =  xpng_ptr->gamma_from_1
                             [(rc*red + gc*green + bc*blue)>>15];
                  *(dp++) = *(sp++);  /* alpha */
               }
            }
            else
#endif
            {
               xpng_bytep sp = row;
               xpng_bytep dp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_byte red   = *(sp++);
                  xpng_byte green = *(sp++);
                  xpng_byte blue  = *(sp++);
                  if(red != green || red != blue)
                     rgb_error |= 1;
                  *(dp++) =  (xpng_byte)((gc*red + gc*green + bc*blue)>>8);
                  *(dp++) = *(sp++);  /* alpha */
               }
            }
         }
         else /* RGBA bit_depth == 16 */
         {
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
            if (xpng_ptr->gamma_16_to_1 != NULL &&
                xpng_ptr->gamma_16_from_1 != NULL)
            {
               xpng_bytep sp = row;
               xpng_bytep dp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_uint_16 red, green, blue, w;

                  red   = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;
                  green = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;
                  blue  = (xpng_uint_16)(((*(sp))<<8) | *(sp+1)); sp+=2;

                  if(red == green && red == blue)
                     w = red;
                  else
                  {
                     xpng_uint_16 red_1   = xpng_ptr->gamma_16_to_1[(red&0xff) >>
                                  xpng_ptr->gamma_shift][red>>8];
                     xpng_uint_16 green_1 = xpng_ptr->gamma_16_to_1[(green&0xff) >>
                                  xpng_ptr->gamma_shift][green>>8];
                     xpng_uint_16 blue_1  = xpng_ptr->gamma_16_to_1[(blue&0xff) >>
                                  xpng_ptr->gamma_shift][blue>>8];
                     xpng_uint_16 gray16  = (xpng_uint_16)((rc * red_1
                                  + gc * green_1 + bc * blue_1)>>15);
                     w = xpng_ptr->gamma_16_from_1[(gray16&0xff) >>
                         xpng_ptr->gamma_shift][gray16 >> 8];
                     rgb_error |= 1;
                  }

                  *(dp++) = (xpng_byte)((w>>8) & 0xff);
                  *(dp++) = (xpng_byte)(w & 0xff);
                  *(dp++) = *(sp++);  /* alpha */
                  *(dp++) = *(sp++);
               }
            }
            else
#endif
            {
               xpng_bytep sp = row;
               xpng_bytep dp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_uint_16 red, green, blue, gray16;
                  red   = (xpng_uint_16)((*(sp)<<8) | *(sp+1)); sp+=2;
                  green = (xpng_uint_16)((*(sp)<<8) | *(sp+1)); sp+=2;
                  blue  = (xpng_uint_16)((*(sp)<<8) | *(sp+1)); sp+=2;
                  if(red != green || red != blue)
                     rgb_error |= 1;
                  gray16  = (xpng_uint_16)((rc*red + gc*green + bc*blue)>>15);
                  *(dp++) = (xpng_byte)((gray16>>8) & 0xff);
                  *(dp++) = (xpng_byte)(gray16 & 0xff);
                  *(dp++) = *(sp++);  /* alpha */
                  *(dp++) = *(sp++);
               }
            }
         }
      }
   row_info->channels -= (xpng_byte)2;
      row_info->color_type &= ~PNG_COLOR_MASK_COLOR;
      row_info->pixel_depth = (xpng_byte)(row_info->channels *
         row_info->bit_depth);
      row_info->rowbytes = ((row_width *
         row_info->pixel_depth + 7) >> 3);
   }
   return rgb_error;
}
#endif

/* Build a grayscale palette.  Palette is assumed to be 1 << bit_depth
 * large of xpng_color.  This lets grayscale images be treated as
 * paletted.  Most useful for gamma correction and simplification
 * of code.
 */
void PNGAPI
xpng_build_grayscale_palette(int bit_depth, xpng_colorp palette)
{
   int num_palette;
   int color_inc;
   int i;
   int v;

   xpng_debug(1, "in xpng_do_build_grayscale_palette\n");
   if (palette == NULL)
      return;

   switch (bit_depth)
   {
      case 1:
         num_palette = 2;
         color_inc = 0xff;
         break;
      case 2:
         num_palette = 4;
         color_inc = 0x55;
         break;
      case 4:
         num_palette = 16;
         color_inc = 0x11;
         break;
      case 8:
         num_palette = 256;
         color_inc = 1;
         break;
      default:
         num_palette = 0;
         color_inc = 0;
         break;
   }

   for (i = 0, v = 0; i < num_palette; i++, v += color_inc)
   {
      palette[i].red = (xpng_byte)v;
      palette[i].green = (xpng_byte)v;
      palette[i].blue = (xpng_byte)v;
   }
}

/* This function is currently unused.  Do we really need it? */
#if defined(PNG_READ_DITHER_SUPPORTED) && defined(PNG_CORRECT_PALETTE_SUPPORTED)
void /* PRIVATE */
xpng_correct_palette(xpng_structp xpng_ptr, xpng_colorp palette,
   int num_palette)
{
   xpng_debug(1, "in xpng_correct_palette\n");
#if defined(PNG_READ_BACKGROUND_SUPPORTED) && \
    defined(PNG_READ_GAMMA_SUPPORTED) && defined(PNG_FLOATING_POINT_SUPPORTED)
   if (xpng_ptr->transformations & (PNG_GAMMA | PNG_BACKGROUND))
   {
      xpng_color back, back_1;

      if (xpng_ptr->background_gamma_type == PNG_BACKGROUND_GAMMA_FILE)
      {
         back.red = xpng_ptr->gamma_table[xpng_ptr->background.red];
         back.green = xpng_ptr->gamma_table[xpng_ptr->background.green];
         back.blue = xpng_ptr->gamma_table[xpng_ptr->background.blue];

         back_1.red = xpng_ptr->gamma_to_1[xpng_ptr->background.red];
         back_1.green = xpng_ptr->gamma_to_1[xpng_ptr->background.green];
         back_1.blue = xpng_ptr->gamma_to_1[xpng_ptr->background.blue];
      }
      else
      {
         double g;

         g = 1.0 / (xpng_ptr->background_gamma * xpng_ptr->screen_gamma);

         if (xpng_ptr->background_gamma_type == PNG_BACKGROUND_GAMMA_SCREEN ||
             fabs(g - 1.0) < PNG_GAMMA_THRESHOLD)
         {
            back.red = xpng_ptr->background.red;
            back.green = xpng_ptr->background.green;
            back.blue = xpng_ptr->background.blue;
         }
         else
         {
            back.red =
               (xpng_byte)(pow((double)xpng_ptr->background.red/255, g) *
                255.0 + 0.5);
            back.green =
               (xpng_byte)(pow((double)xpng_ptr->background.green/255, g) *
                255.0 + 0.5);
            back.blue =
               (xpng_byte)(pow((double)xpng_ptr->background.blue/255, g) *
                255.0 + 0.5);
         }

         g = 1.0 / xpng_ptr->background_gamma;

         back_1.red =
            (xpng_byte)(pow((double)xpng_ptr->background.red/255, g) *
             255.0 + 0.5);
         back_1.green =
            (xpng_byte)(pow((double)xpng_ptr->background.green/255, g) *
             255.0 + 0.5);
         back_1.blue =
            (xpng_byte)(pow((double)xpng_ptr->background.blue/255, g) *
             255.0 + 0.5);
      }

      if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         xpng_uint_32 i;

         for (i = 0; i < (xpng_uint_32)num_palette; i++)
         {
            if (i < xpng_ptr->num_trans && xpng_ptr->trans[i] == 0)
            {
               palette[i] = back;
            }
            else if (i < xpng_ptr->num_trans && xpng_ptr->trans[i] != 0xff)
            {
               xpng_byte v, w;

               v = xpng_ptr->gamma_to_1[xpng_ptr->palette[i].red];
               xpng_composite(w, v, xpng_ptr->trans[i], back_1.red);
               palette[i].red = xpng_ptr->gamma_from_1[w];

               v = xpng_ptr->gamma_to_1[xpng_ptr->palette[i].green];
               xpng_composite(w, v, xpng_ptr->trans[i], back_1.green);
               palette[i].green = xpng_ptr->gamma_from_1[w];

               v = xpng_ptr->gamma_to_1[xpng_ptr->palette[i].blue];
               xpng_composite(w, v, xpng_ptr->trans[i], back_1.blue);
               palette[i].blue = xpng_ptr->gamma_from_1[w];
            }
            else
            {
               palette[i].red = xpng_ptr->gamma_table[palette[i].red];
               palette[i].green = xpng_ptr->gamma_table[palette[i].green];
               palette[i].blue = xpng_ptr->gamma_table[palette[i].blue];
            }
         }
      }
      else
      {
         int i;

         for (i = 0; i < num_palette; i++)
         {
            if (palette[i].red == (xpng_byte)xpng_ptr->trans_values.gray)
            {
               palette[i] = back;
            }
            else
            {
               palette[i].red = xpng_ptr->gamma_table[palette[i].red];
               palette[i].green = xpng_ptr->gamma_table[palette[i].green];
               palette[i].blue = xpng_ptr->gamma_table[palette[i].blue];
            }
         }
      }
   }
   else
#endif
#if defined(PNG_READ_GAMMA_SUPPORTED)
   if (xpng_ptr->transformations & PNG_GAMMA)
   {
      int i;

      for (i = 0; i < num_palette; i++)
      {
         palette[i].red = xpng_ptr->gamma_table[palette[i].red];
         palette[i].green = xpng_ptr->gamma_table[palette[i].green];
         palette[i].blue = xpng_ptr->gamma_table[palette[i].blue];
      }
   }
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   else
#endif
#endif
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   if (xpng_ptr->transformations & PNG_BACKGROUND)
   {
      if (xpng_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         xpng_color back;

         back.red   = (xpng_byte)xpng_ptr->background.red;
         back.green = (xpng_byte)xpng_ptr->background.green;
         back.blue  = (xpng_byte)xpng_ptr->background.blue;

         for (i = 0; i < (int)xpng_ptr->num_trans; i++)
         {
            if (xpng_ptr->trans[i] == 0)
            {
               palette[i].red = back.red;
               palette[i].green = back.green;
               palette[i].blue = back.blue;
            }
            else if (xpng_ptr->trans[i] != 0xff)
            {
               xpng_composite(palette[i].red, xpng_ptr->palette[i].red,
                  xpng_ptr->trans[i], back.red);
               xpng_composite(palette[i].green, xpng_ptr->palette[i].green,
                  xpng_ptr->trans[i], back.green);
               xpng_composite(palette[i].blue, xpng_ptr->palette[i].blue,
                  xpng_ptr->trans[i], back.blue);
            }
         }
      }
      else /* assume grayscale palette (what else could it be?) */
      {
         int i;

         for (i = 0; i < num_palette; i++)
         {
            if (i == (xpng_byte)xpng_ptr->trans_values.gray)
            {
               palette[i].red = (xpng_byte)xpng_ptr->background.red;
               palette[i].green = (xpng_byte)xpng_ptr->background.green;
               palette[i].blue = (xpng_byte)xpng_ptr->background.blue;
            }
         }
      }
   }
#endif
}
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
/* Replace any alpha or transparency with the supplied background color.
 * "background" is already in the screen gamma, while "background_1" is
 * at a gamma of 1.0.  Paletted files have already been taken care of.
 */
void /* PRIVATE */
xpng_do_background(xpng_row_infop row_info, xpng_bytep row,
   xpng_color_16p trans_values, xpng_color_16p background
#if defined(PNG_READ_GAMMA_SUPPORTED)
   , xpng_color_16p background_1,
   xpng_bytep gamma_table, xpng_bytep gamma_from_1, xpng_bytep gamma_to_1,
   xpng_uint_16pp gamma_16, xpng_uint_16pp gamma_16_from_1,
   xpng_uint_16pp gamma_16_to_1, int gamma_shift
#endif
   )
{
   xpng_bytep sp, dp;
   xpng_uint_32 i;
   xpng_uint_32 row_width=row_info->width;
   int shift;

   xpng_debug(1, "in xpng_do_background\n");
   if (background != NULL &&
#if defined(PNG_USELESS_TESTS_SUPPORTED)
       row != NULL && row_info != NULL &&
#endif
      (!(row_info->color_type & PNG_COLOR_MASK_ALPHA) ||
      (row_info->color_type != PNG_COLOR_TYPE_PALETTE && trans_values)))
   {
      switch (row_info->color_type)
      {
         case PNG_COLOR_TYPE_GRAY:
         {
            switch (row_info->bit_depth)
            {
               case 1:
               {
                  sp = row;
                  shift = 7;
                  for (i = 0; i < row_width; i++)
                  {
                     if ((xpng_uint_16)((*sp >> shift) & 0x01)
                        == trans_values->gray)
                     {
                        *sp &= (xpng_byte)((0x7f7f >> (7 - shift)) & 0xff);
                        *sp |= (xpng_byte)(background->gray << shift);
                     }
                     if (!shift)
                     {
                        shift = 7;
                        sp++;
                     }
                     else
                        shift--;
                  }
                  break;
               }
               case 2:
               {
#if defined(PNG_READ_GAMMA_SUPPORTED)
                  if (gamma_table != NULL)
                  {
                     sp = row;
                     shift = 6;
                     for (i = 0; i < row_width; i++)
                     {
                        if ((xpng_uint_16)((*sp >> shift) & 0x03)
                            == trans_values->gray)
                        {
                           *sp &= (xpng_byte)((0x3f3f >> (6 - shift)) & 0xff);
                           *sp |= (xpng_byte)(background->gray << shift);
                        }
                        else
                        {
                           xpng_byte p = (xpng_byte)((*sp >> shift) & 0x03);
                           xpng_byte g = (xpng_byte)((gamma_table [p | (p << 2) |
                               (p << 4) | (p << 6)] >> 6) & 0x03);
                           *sp &= (xpng_byte)((0x3f3f >> (6 - shift)) & 0xff);
                           *sp |= (xpng_byte)(g << shift);
                        }
                        if (!shift)
                        {
                           shift = 6;
                           sp++;
                        }
                        else
                           shift -= 2;
                     }
                  }
                  else
#endif
                  {
                     sp = row;
                     shift = 6;
                     for (i = 0; i < row_width; i++)
                     {
                        if ((xpng_uint_16)((*sp >> shift) & 0x03)
                            == trans_values->gray)
                        {
                           *sp &= (xpng_byte)((0x3f3f >> (6 - shift)) & 0xff);
                           *sp |= (xpng_byte)(background->gray << shift);
                        }
                        if (!shift)
                        {
                           shift = 6;
                           sp++;
                        }
                        else
                           shift -= 2;
                     }
                  }
                  break;
               }
               case 4:
               {
#if defined(PNG_READ_GAMMA_SUPPORTED)
                  if (gamma_table != NULL)
                  {
                     sp = row;
                     shift = 4;
                     for (i = 0; i < row_width; i++)
                     {
                        if ((xpng_uint_16)((*sp >> shift) & 0x0f)
                            == trans_values->gray)
                        {
                           *sp &= (xpng_byte)((0xf0f >> (4 - shift)) & 0xff);
                           *sp |= (xpng_byte)(background->gray << shift);
                        }
                        else
                        {
                           xpng_byte p = (xpng_byte)((*sp >> shift) & 0x0f);
                           xpng_byte g = (xpng_byte)((gamma_table[p |
                             (p << 4)] >> 4) & 0x0f);
                           *sp &= (xpng_byte)((0xf0f >> (4 - shift)) & 0xff);
                           *sp |= (xpng_byte)(g << shift);
                        }
                        if (!shift)
                        {
                           shift = 4;
                           sp++;
                        }
                        else
                           shift -= 4;
                     }
                  }
                  else
#endif
                  {
                     sp = row;
                     shift = 4;
                     for (i = 0; i < row_width; i++)
                     {
                        if ((xpng_uint_16)((*sp >> shift) & 0x0f)
                            == trans_values->gray)
                        {
                           *sp &= (xpng_byte)((0xf0f >> (4 - shift)) & 0xff);
                           *sp |= (xpng_byte)(background->gray << shift);
                        }
                        if (!shift)
                        {
                           shift = 4;
                           sp++;
                        }
                        else
                           shift -= 4;
                     }
                  }
                  break;
               }
               case 8:
               {
#if defined(PNG_READ_GAMMA_SUPPORTED)
                  if (gamma_table != NULL)
                  {
                     sp = row;
                     for (i = 0; i < row_width; i++, sp++)
                     {
                        if (*sp == trans_values->gray)
                        {
                           *sp = (xpng_byte)background->gray;
                        }
                        else
                        {
                           *sp = gamma_table[*sp];
                        }
                     }
                  }
                  else
#endif
                  {
                     sp = row;
                     for (i = 0; i < row_width; i++, sp++)
                     {
                        if (*sp == trans_values->gray)
                        {
                           *sp = (xpng_byte)background->gray;
                        }
                     }
                  }
                  break;
               }
               case 16:
               {
#if defined(PNG_READ_GAMMA_SUPPORTED)
                  if (gamma_16 != NULL)
                  {
                     sp = row;
                     for (i = 0; i < row_width; i++, sp += 2)
                     {
                        xpng_uint_16 v;

                        v = (xpng_uint_16)(((*sp) << 8) + *(sp + 1));
                        if (v == trans_values->gray)
                        {
                           /* background is already in screen gamma */
                           *sp = (xpng_byte)((background->gray >> 8) & 0xff);
                           *(sp + 1) = (xpng_byte)(background->gray & 0xff);
                        }
                        else
                        {
                           v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                           *sp = (xpng_byte)((v >> 8) & 0xff);
                           *(sp + 1) = (xpng_byte)(v & 0xff);
                        }
                     }
                  }
                  else
#endif
                  {
                     sp = row;
                     for (i = 0; i < row_width; i++, sp += 2)
                     {
                        xpng_uint_16 v;

                        v = (xpng_uint_16)(((*sp) << 8) + *(sp + 1));
                        if (v == trans_values->gray)
                        {
                           *sp = (xpng_byte)((background->gray >> 8) & 0xff);
                           *(sp + 1) = (xpng_byte)(background->gray & 0xff);
                        }
                     }
                  }
                  break;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_table != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 3)
                  {
                     if (*sp == trans_values->red &&
                        *(sp + 1) == trans_values->green &&
                        *(sp + 2) == trans_values->blue)
                     {
                        *sp = (xpng_byte)background->red;
                        *(sp + 1) = (xpng_byte)background->green;
                        *(sp + 2) = (xpng_byte)background->blue;
                     }
                     else
                     {
                        *sp = gamma_table[*sp];
                        *(sp + 1) = gamma_table[*(sp + 1)];
                        *(sp + 2) = gamma_table[*(sp + 2)];
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 3)
                  {
                     if (*sp == trans_values->red &&
                        *(sp + 1) == trans_values->green &&
                        *(sp + 2) == trans_values->blue)
                     {
                        *sp = (xpng_byte)background->red;
                        *(sp + 1) = (xpng_byte)background->green;
                        *(sp + 2) = (xpng_byte)background->blue;
                     }
                  }
               }
            }
            else /* if (row_info->bit_depth == 16) */
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_16 != NULL)
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 6)
                  {
                     xpng_uint_16 r = (xpng_uint_16)(((*sp) << 8) + *(sp + 1));
                     xpng_uint_16 g = (xpng_uint_16)(((*(sp+2)) << 8) + *(sp+3));
                     xpng_uint_16 b = (xpng_uint_16)(((*(sp+4)) << 8) + *(sp+5));
                     if (r == trans_values->red && g == trans_values->green &&
                        b == trans_values->blue)
                     {
                        /* background is already in screen gamma */
                        *sp = (xpng_byte)((background->red >> 8) & 0xff);
                        *(sp + 1) = (xpng_byte)(background->red & 0xff);
                        *(sp + 2) = (xpng_byte)((background->green >> 8) & 0xff);
                        *(sp + 3) = (xpng_byte)(background->green & 0xff);
                        *(sp + 4) = (xpng_byte)((background->blue >> 8) & 0xff);
                        *(sp + 5) = (xpng_byte)(background->blue & 0xff);
                     }
                     else
                     {
                        xpng_uint_16 v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                        *sp = (xpng_byte)((v >> 8) & 0xff);
                        *(sp + 1) = (xpng_byte)(v & 0xff);
                        v = gamma_16[*(sp + 3) >> gamma_shift][*(sp + 2)];
                        *(sp + 2) = (xpng_byte)((v >> 8) & 0xff);
                        *(sp + 3) = (xpng_byte)(v & 0xff);
                        v = gamma_16[*(sp + 5) >> gamma_shift][*(sp + 4)];
                        *(sp + 4) = (xpng_byte)((v >> 8) & 0xff);
                        *(sp + 5) = (xpng_byte)(v & 0xff);
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  for (i = 0; i < row_width; i++, sp += 6)
                  {
                     xpng_uint_16 r = (xpng_uint_16)(((*sp) << 8) + *(sp+1));
                     xpng_uint_16 g = (xpng_uint_16)(((*(sp+2)) << 8) + *(sp+3));
                     xpng_uint_16 b = (xpng_uint_16)(((*(sp+4)) << 8) + *(sp+5));

                     if (r == trans_values->red && g == trans_values->green &&
                        b == trans_values->blue)
                     {
                        *sp = (xpng_byte)((background->red >> 8) & 0xff);
                        *(sp + 1) = (xpng_byte)(background->red & 0xff);
                        *(sp + 2) = (xpng_byte)((background->green >> 8) & 0xff);
                        *(sp + 3) = (xpng_byte)(background->green & 0xff);
                        *(sp + 4) = (xpng_byte)((background->blue >> 8) & 0xff);
                        *(sp + 5) = (xpng_byte)(background->blue & 0xff);
                     }
                  }
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_to_1 != NULL && gamma_from_1 != NULL &&
                   gamma_table != NULL)
               {
                  sp = row;
                  dp = row;
                  for (i = 0; i < row_width; i++, sp += 2, dp++)
                  {
                     xpng_uint_16 a = *(sp + 1);

                     if (a == 0xff)
                     {
                        *dp = gamma_table[*sp];
                     }
                     else if (a == 0)
                     {
                        /* background is already in screen gamma */
                        *dp = (xpng_byte)background->gray;
                     }
                     else
                     {
                        xpng_byte v, w;

                        v = gamma_to_1[*sp];
                        xpng_composite(w, v, a, background_1->gray);
                        *dp = gamma_from_1[w];
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  dp = row;
                  for (i = 0; i < row_width; i++, sp += 2, dp++)
                  {
                     xpng_byte a = *(sp + 1);

                     if (a == 0xff)
                     {
                        *dp = *sp;
                     }
#if defined(PNG_READ_GAMMA_SUPPORTED)
                     else if (a == 0)
                     {
                        *dp = (xpng_byte)background->gray;
                     }
                     else
                     {
                        xpng_composite(*dp, *sp, a, background_1->gray);
                     }
#else
                     *dp = (xpng_byte)background->gray;
#endif
                  }
               }
            }
            else /* if (xpng_ptr->bit_depth == 16) */
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_16 != NULL && gamma_16_from_1 != NULL &&
                   gamma_16_to_1 != NULL)
               {
                  sp = row;
                  dp = row;
                  for (i = 0; i < row_width; i++, sp += 4, dp += 2)
                  {
                     xpng_uint_16 a = (xpng_uint_16)(((*(sp+2)) << 8) + *(sp+3));

                     if (a == (xpng_uint_16)0xffff)
                     {
                        xpng_uint_16 v;

                        v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                        *dp = (xpng_byte)((v >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(v & 0xff);
                     }
#if defined(PNG_READ_GAMMA_SUPPORTED)
                     else if (a == 0)
#else
                     else
#endif
                     {
                        /* background is already in screen gamma */
                        *dp = (xpng_byte)((background->gray >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(background->gray & 0xff);
                     }
#if defined(PNG_READ_GAMMA_SUPPORTED)
                     else
                     {
                        xpng_uint_16 g, v, w;

                        g = gamma_16_to_1[*(sp + 1) >> gamma_shift][*sp];
                        xpng_composite_16(v, g, a, background_1->gray);
                        w = gamma_16_from_1[(v&0xff) >> gamma_shift][v >> 8];
                        *dp = (xpng_byte)((w >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(w & 0xff);
                     }
#endif
                  }
               }
               else
#endif
               {
                  sp = row;
                  dp = row;
                  for (i = 0; i < row_width; i++, sp += 4, dp += 2)
                  {
                     xpng_uint_16 a = (xpng_uint_16)(((*(sp+2)) << 8) + *(sp+3));
                     if (a == (xpng_uint_16)0xffff)
                     {
                        xpng_memcpy(dp, sp, 2);
                     }
#if defined(PNG_READ_GAMMA_SUPPORTED)
                     else if (a == 0)
#else
                     else
#endif
                     {
                        *dp = (xpng_byte)((background->gray >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(background->gray & 0xff);
                     }
#if defined(PNG_READ_GAMMA_SUPPORTED)
                     else
                     {
                        xpng_uint_16 g, v;

                        g = (xpng_uint_16)(((*sp) << 8) + *(sp + 1));
                        xpng_composite_16(v, g, a, background_1->gray);
                        *dp = (xpng_byte)((v >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(v & 0xff);
                     }
#endif
                  }
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_to_1 != NULL && gamma_from_1 != NULL &&
                   gamma_table != NULL)
               {
                  sp = row;
                  dp = row;
                  for (i = 0; i < row_width; i++, sp += 4, dp += 3)
                  {
                     xpng_byte a = *(sp + 3);

                     if (a == 0xff)
                     {
                        *dp = gamma_table[*sp];
                        *(dp + 1) = gamma_table[*(sp + 1)];
                        *(dp + 2) = gamma_table[*(sp + 2)];
                     }
                     else if (a == 0)
                     {
                        /* background is already in screen gamma */
                        *dp = (xpng_byte)background->red;
                        *(dp + 1) = (xpng_byte)background->green;
                        *(dp + 2) = (xpng_byte)background->blue;
                     }
                     else
                     {
                        xpng_byte v, w;

                        v = gamma_to_1[*sp];
                        xpng_composite(w, v, a, background_1->red);
                        *dp = gamma_from_1[w];
                        v = gamma_to_1[*(sp + 1)];
                        xpng_composite(w, v, a, background_1->green);
                        *(dp + 1) = gamma_from_1[w];
                        v = gamma_to_1[*(sp + 2)];
                        xpng_composite(w, v, a, background_1->blue);
                        *(dp + 2) = gamma_from_1[w];
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  dp = row;
                  for (i = 0; i < row_width; i++, sp += 4, dp += 3)
                  {
                     xpng_byte a = *(sp + 3);

                     if (a == 0xff)
                     {
                        *dp = *sp;
                        *(dp + 1) = *(sp + 1);
                        *(dp + 2) = *(sp + 2);
                     }
                     else if (a == 0)
                     {
                        *dp = (xpng_byte)background->red;
                        *(dp + 1) = (xpng_byte)background->green;
                        *(dp + 2) = (xpng_byte)background->blue;
                     }
                     else
                     {
                        xpng_composite(*dp, *sp, a, background->red);
                        xpng_composite(*(dp + 1), *(sp + 1), a,
                           background->green);
                        xpng_composite(*(dp + 2), *(sp + 2), a,
                           background->blue);
                     }
                  }
               }
            }
            else /* if (row_info->bit_depth == 16) */
            {
#if defined(PNG_READ_GAMMA_SUPPORTED)
               if (gamma_16 != NULL && gamma_16_from_1 != NULL &&
                   gamma_16_to_1 != NULL)
               {
                  sp = row;
                  dp = row;
                  for (i = 0; i < row_width; i++, sp += 8, dp += 6)
                  {
                     xpng_uint_16 a = (xpng_uint_16)(((xpng_uint_16)(*(sp + 6))
                         << 8) + (xpng_uint_16)(*(sp + 7)));
                     if (a == (xpng_uint_16)0xffff)
                     {
                        xpng_uint_16 v;

                        v = gamma_16[*(sp + 1) >> gamma_shift][*sp];
                        *dp = (xpng_byte)((v >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(v & 0xff);
                        v = gamma_16[*(sp + 3) >> gamma_shift][*(sp + 2)];
                        *(dp + 2) = (xpng_byte)((v >> 8) & 0xff);
                        *(dp + 3) = (xpng_byte)(v & 0xff);
                        v = gamma_16[*(sp + 5) >> gamma_shift][*(sp + 4)];
                        *(dp + 4) = (xpng_byte)((v >> 8) & 0xff);
                        *(dp + 5) = (xpng_byte)(v & 0xff);
                     }
                     else if (a == 0)
                     {
                        /* background is already in screen gamma */
                        *dp = (xpng_byte)((background->red >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(background->red & 0xff);
                        *(dp + 2) = (xpng_byte)((background->green >> 8) & 0xff);
                        *(dp + 3) = (xpng_byte)(background->green & 0xff);
                        *(dp + 4) = (xpng_byte)((background->blue >> 8) & 0xff);
                        *(dp + 5) = (xpng_byte)(background->blue & 0xff);
                     }
                     else
                     {
                        xpng_uint_16 v, w, x;

                        v = gamma_16_to_1[*(sp + 1) >> gamma_shift][*sp];
                        xpng_composite_16(w, v, a, background_1->red);
                        x = gamma_16_from_1[((w&0xff) >> gamma_shift)][w >> 8];
                        *dp = (xpng_byte)((x >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(x & 0xff);
                        v = gamma_16_to_1[*(sp + 3) >> gamma_shift][*(sp + 2)];
                        xpng_composite_16(w, v, a, background_1->green);
                        x = gamma_16_from_1[((w&0xff) >> gamma_shift)][w >> 8];
                        *(dp + 2) = (xpng_byte)((x >> 8) & 0xff);
                        *(dp + 3) = (xpng_byte)(x & 0xff);
                        v = gamma_16_to_1[*(sp + 5) >> gamma_shift][*(sp + 4)];
                        xpng_composite_16(w, v, a, background_1->blue);
                        x = gamma_16_from_1[(w & 0xff) >> gamma_shift][w >> 8];
                        *(dp + 4) = (xpng_byte)((x >> 8) & 0xff);
                        *(dp + 5) = (xpng_byte)(x & 0xff);
                     }
                  }
               }
               else
#endif
               {
                  sp = row;
                  dp = row;
                  for (i = 0; i < row_width; i++, sp += 8, dp += 6)
                  {
                     xpng_uint_16 a = (xpng_uint_16)(((xpng_uint_16)(*(sp + 6))
                        << 8) + (xpng_uint_16)(*(sp + 7)));
                     if (a == (xpng_uint_16)0xffff)
                     {
                        xpng_memcpy(dp, sp, 6);
                     }
                     else if (a == 0)
                     {
                        *dp = (xpng_byte)((background->red >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(background->red & 0xff);
                        *(dp + 2) = (xpng_byte)((background->green >> 8) & 0xff);
                        *(dp + 3) = (xpng_byte)(background->green & 0xff);
                        *(dp + 4) = (xpng_byte)((background->blue >> 8) & 0xff);
                        *(dp + 5) = (xpng_byte)(background->blue & 0xff);
                     }
                     else
                     {
                        xpng_uint_16 v;

                        xpng_uint_16 r = (xpng_uint_16)(((*sp) << 8) + *(sp + 1));
                        xpng_uint_16 g = (xpng_uint_16)(((*(sp + 2)) << 8)
                            + *(sp + 3));
                        xpng_uint_16 b = (xpng_uint_16)(((*(sp + 4)) << 8)
                            + *(sp + 5));

                        xpng_composite_16(v, r, a, background->red);
                        *dp = (xpng_byte)((v >> 8) & 0xff);
                        *(dp + 1) = (xpng_byte)(v & 0xff);
                        xpng_composite_16(v, g, a, background->green);
                        *(dp + 2) = (xpng_byte)((v >> 8) & 0xff);
                        *(dp + 3) = (xpng_byte)(v & 0xff);
                        xpng_composite_16(v, b, a, background->blue);
                        *(dp + 4) = (xpng_byte)((v >> 8) & 0xff);
                        *(dp + 5) = (xpng_byte)(v & 0xff);
                     }
                  }
               }
            }
            break;
         }
      }

      if (row_info->color_type & PNG_COLOR_MASK_ALPHA)
      {
         row_info->color_type &= ~PNG_COLOR_MASK_ALPHA;
         row_info->channels--;
         row_info->pixel_depth = (xpng_byte)(row_info->channels *
            row_info->bit_depth);
         row_info->rowbytes = ((row_width *
            row_info->pixel_depth + 7) >> 3);
      }
   }
}
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
/* Gamma correct the image, avoiding the alpha channel.  Make sure
 * you do this after you deal with the transparency issue on grayscale
 * or RGB images. If your bit depth is 8, use gamma_table, if it
 * is 16, use gamma_16_table and gamma_shift.  Build these with
 * build_gamma_table().
 */
void /* PRIVATE */
xpng_do_gamma(xpng_row_infop row_info, xpng_bytep row,
   xpng_bytep gamma_table, xpng_uint_16pp gamma_16_table,
   int gamma_shift)
{
   xpng_bytep sp;
   xpng_uint_32 i;
   xpng_uint_32 row_width=row_info->width;

   xpng_debug(1, "in xpng_do_gamma\n");
   if (
#if defined(PNG_USELESS_TESTS_SUPPORTED)
       row != NULL && row_info != NULL &&
#endif
       ((row_info->bit_depth <= 8 && gamma_table != NULL) ||
        (row_info->bit_depth == 16 && gamma_16_table != NULL)))
   {
      switch (row_info->color_type)
      {
         case PNG_COLOR_TYPE_RGB:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }
            else /* if (row_info->bit_depth == 16) */
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_uint_16 v;

                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (xpng_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (xpng_byte)(v & 0xff);
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (xpng_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (xpng_byte)(v & 0xff);
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (xpng_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (xpng_byte)(v & 0xff);
                  sp += 2;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_RGB_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  *sp = gamma_table[*sp];
                  sp++;
                  sp++;
               }
            }
            else /* if (row_info->bit_depth == 16) */
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (xpng_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (xpng_byte)(v & 0xff);
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (xpng_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (xpng_byte)(v & 0xff);
                  sp += 2;
                  v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (xpng_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (xpng_byte)(v & 0xff);
                  sp += 4;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY_ALPHA:
         {
            if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp += 2;
               }
            }
            else /* if (row_info->bit_depth == 16) */
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (xpng_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (xpng_byte)(v & 0xff);
                  sp += 4;
               }
            }
            break;
         }
         case PNG_COLOR_TYPE_GRAY:
         {
            if (row_info->bit_depth == 2)
            {
               sp = row;
               for (i = 0; i < row_width; i += 4)
               {
                  int a = *sp & 0xc0;
                  int b = *sp & 0x30;
                  int c = *sp & 0x0c;
                  int d = *sp & 0x03;

                  *sp = (xpng_byte)(
                        ((((int)gamma_table[a|(a>>2)|(a>>4)|(a>>6)])   ) & 0xc0)|
                        ((((int)gamma_table[(b<<2)|b|(b>>2)|(b>>4)])>>2) & 0x30)|
                        ((((int)gamma_table[(c<<4)|(c<<2)|c|(c>>2)])>>4) & 0x0c)|
                        ((((int)gamma_table[(d<<6)|(d<<4)|(d<<2)|d])>>6) ));
                  sp++;
               }
            }
            if (row_info->bit_depth == 4)
            {
               sp = row;
               for (i = 0; i < row_width; i += 2)
               {
                  int msb = *sp & 0xf0;
                  int lsb = *sp & 0x0f;

                  *sp = (xpng_byte)((((int)gamma_table[msb | (msb >> 4)]) & 0xf0)
                          | (((int)gamma_table[(lsb << 4) | lsb]) >> 4));
                  sp++;
               }
            }
            else if (row_info->bit_depth == 8)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  *sp = gamma_table[*sp];
                  sp++;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               sp = row;
               for (i = 0; i < row_width; i++)
               {
                  xpng_uint_16 v = gamma_16_table[*(sp + 1) >> gamma_shift][*sp];
                  *sp = (xpng_byte)((v >> 8) & 0xff);
                  *(sp + 1) = (xpng_byte)(v & 0xff);
                  sp += 2;
               }
            }
            break;
         }
      }
   }
}
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
/* Expands a palette row to an RGB or RGBA row depending
 * upon whether you supply trans and num_trans.
 */
void /* PRIVATE */
xpng_do_expand_palette(xpng_row_infop row_info, xpng_bytep row,
   xpng_colorp palette, xpng_bytep trans, int num_trans)
{
   int shift, value;
   xpng_bytep sp, dp;
   xpng_uint_32 i;
   xpng_uint_32 row_width=row_info->width;

   xpng_debug(1, "in xpng_do_expand_palette\n");
   if (
#if defined(PNG_USELESS_TESTS_SUPPORTED)
       row != NULL && row_info != NULL &&
#endif
       row_info->color_type == PNG_COLOR_TYPE_PALETTE)
   {
      if (row_info->bit_depth < 8)
      {
         switch (row_info->bit_depth)
         {
            case 1:
            {
               sp = row + (xpng_size_t)((row_width - 1) >> 3);
               dp = row + (xpng_size_t)row_width - 1;
               shift = 7 - (int)((row_width + 7) & 0x07);
               for (i = 0; i < row_width; i++)
               {
                  if ((*sp >> shift) & 0x01)
                     *dp = 1;
                  else
                     *dp = 0;
                  if (shift == 7)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift++;

                  dp--;
               }
               break;
            }
            case 2:
            {
               sp = row + (xpng_size_t)((row_width - 1) >> 2);
               dp = row + (xpng_size_t)row_width - 1;
               shift = (int)((3 - ((row_width + 3) & 0x03)) << 1);
               for (i = 0; i < row_width; i++)
               {
                  value = (*sp >> shift) & 0x03;
                  *dp = (xpng_byte)value;
                  if (shift == 6)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift += 2;

                  dp--;
               }
               break;
            }
            case 4:
            {
               sp = row + (xpng_size_t)((row_width - 1) >> 1);
               dp = row + (xpng_size_t)row_width - 1;
               shift = (int)((row_width & 0x01) << 2);
               for (i = 0; i < row_width; i++)
               {
                  value = (*sp >> shift) & 0x0f;
                  *dp = (xpng_byte)value;
                  if (shift == 4)
                  {
                     shift = 0;
                     sp--;
                  }
                  else
                     shift += 4;

                  dp--;
               }
               break;
            }
         }
         row_info->bit_depth = 8;
         row_info->pixel_depth = 8;
         row_info->rowbytes = row_width;
      }
      switch (row_info->bit_depth)
      {
         case 8:
         {
            if (trans != NULL)
            {
               sp = row + (xpng_size_t)row_width - 1;
               dp = row + (xpng_size_t)(row_width << 2) - 1;

               for (i = 0; i < row_width; i++)
               {
                  if ((int)(*sp) >= num_trans)
                     *dp-- = 0xff;
                  else
                     *dp-- = trans[*sp];
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }
               row_info->bit_depth = 8;
               row_info->pixel_depth = 32;
               row_info->rowbytes = row_width * 4;
               row_info->color_type = 6;
               row_info->channels = 4;
            }
            else
            {
               sp = row + (xpng_size_t)row_width - 1;
               dp = row + (xpng_size_t)(row_width * 3) - 1;

               for (i = 0; i < row_width; i++)
               {
                  *dp-- = palette[*sp].blue;
                  *dp-- = palette[*sp].green;
                  *dp-- = palette[*sp].red;
                  sp--;
               }
               row_info->bit_depth = 8;
               row_info->pixel_depth = 24;
               row_info->rowbytes = row_width * 3;
               row_info->color_type = 2;
               row_info->channels = 3;
            }
            break;
         }
      }
   }
}

/* If the bit depth < 8, it is expanded to 8.  Also, if the
 * transparency value is supplied, an alpha channel is built.
 */
void /* PRIVATE */
xpng_do_expand(xpng_row_infop row_info, xpng_bytep row,
   xpng_color_16p trans_value)
{
   int shift, value;
   xpng_bytep sp, dp;
   xpng_uint_32 i;
   xpng_uint_32 row_width=row_info->width;

   xpng_debug(1, "in xpng_do_expand\n");
#if defined(PNG_USELESS_TESTS_SUPPORTED)
   if (row != NULL && row_info != NULL)
#endif
   {
      if (row_info->color_type == PNG_COLOR_TYPE_GRAY)
      {
         xpng_uint_16 gray = (xpng_uint_16)(trans_value ? trans_value->gray : 0);

         if (row_info->bit_depth < 8)
         {
            switch (row_info->bit_depth)
            {
               case 1:
               {
                  gray = (xpng_uint_16)(gray*0xff);
                  sp = row + (xpng_size_t)((row_width - 1) >> 3);
                  dp = row + (xpng_size_t)row_width - 1;
                  shift = 7 - (int)((row_width + 7) & 0x07);
                  for (i = 0; i < row_width; i++)
                  {
                     if ((*sp >> shift) & 0x01)
                        *dp = 0xff;
                     else
                        *dp = 0;
                     if (shift == 7)
                     {
                        shift = 0;
                        sp--;
                     }
                     else
                        shift++;

                     dp--;
                  }
                  break;
               }
               case 2:
               {
                  gray = (xpng_uint_16)(gray*0x55);
                  sp = row + (xpng_size_t)((row_width - 1) >> 2);
                  dp = row + (xpng_size_t)row_width - 1;
                  shift = (int)((3 - ((row_width + 3) & 0x03)) << 1);
                  for (i = 0; i < row_width; i++)
                  {
                     value = (*sp >> shift) & 0x03;
                     *dp = (xpng_byte)(value | (value << 2) | (value << 4) |
                        (value << 6));
                     if (shift == 6)
                     {
                        shift = 0;
                        sp--;
                     }
                     else
                        shift += 2;

                     dp--;
                  }
                  break;
               }
               case 4:
               {
                  gray = (xpng_uint_16)(gray*0x11);
                  sp = row + (xpng_size_t)((row_width - 1) >> 1);
                  dp = row + (xpng_size_t)row_width - 1;
                  shift = (int)((1 - ((row_width + 1) & 0x01)) << 2);
                  for (i = 0; i < row_width; i++)
                  {
                     value = (*sp >> shift) & 0x0f;
                     *dp = (xpng_byte)(value | (value << 4));
                     if (shift == 4)
                     {
                        shift = 0;
                        sp--;
                     }
                     else
                        shift = 4;

                     dp--;
                  }
                  break;
               }
            }
            row_info->bit_depth = 8;
            row_info->pixel_depth = 8;
            row_info->rowbytes = row_width;
         }

         if (trans_value != NULL)
         {
            if (row_info->bit_depth == 8)
            {
               sp = row + (xpng_size_t)row_width - 1;
               dp = row + (xpng_size_t)(row_width << 1) - 1;
               for (i = 0; i < row_width; i++)
               {
                  if (*sp == gray)
                     *dp-- = 0;
                  else
                     *dp-- = 0xff;
                  *dp-- = *sp--;
               }
            }
            else if (row_info->bit_depth == 16)
            {
               sp = row + row_info->rowbytes - 1;
               dp = row + (row_info->rowbytes << 1) - 1;
               for (i = 0; i < row_width; i++)
               {
                  if (((xpng_uint_16)*(sp) |
                     ((xpng_uint_16)*(sp - 1) << 8)) == gray)
                  {
                     *dp-- = 0;
                     *dp-- = 0;
                  }
                  else
                  {
                     *dp-- = 0xff;
                     *dp-- = 0xff;
                  }
                  *dp-- = *sp--;
                  *dp-- = *sp--;
               }
            }
            row_info->color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
            row_info->channels = 2;
            row_info->pixel_depth = (xpng_byte)(row_info->bit_depth << 1);
            row_info->rowbytes =
               ((row_width * row_info->pixel_depth) >> 3);
         }
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_RGB && trans_value)
      {
         if (row_info->bit_depth == 8)
         {
            sp = row + (xpng_size_t)row_info->rowbytes - 1;
            dp = row + (xpng_size_t)(row_width << 2) - 1;
            for (i = 0; i < row_width; i++)
            {
               if (*(sp - 2) == trans_value->red &&
                  *(sp - 1) == trans_value->green &&
                  *(sp - 0) == trans_value->blue)
                  *dp-- = 0;
               else
                  *dp-- = 0xff;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         else if (row_info->bit_depth == 16)
         {
            sp = row + row_info->rowbytes - 1;
            dp = row + (xpng_size_t)(row_width << 3) - 1;
            for (i = 0; i < row_width; i++)
            {
               if ((((xpng_uint_16)*(sp - 4) |
                  ((xpng_uint_16)*(sp - 5) << 8)) == trans_value->red) &&
                  (((xpng_uint_16)*(sp - 2) |
                  ((xpng_uint_16)*(sp - 3) << 8)) == trans_value->green) &&
                  (((xpng_uint_16)*(sp - 0) |
                  ((xpng_uint_16)*(sp - 1) << 8)) == trans_value->blue))
               {
                  *dp-- = 0;
                  *dp-- = 0;
               }
               else
               {
                  *dp-- = 0xff;
                  *dp-- = 0xff;
               }
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
               *dp-- = *sp--;
            }
         }
         row_info->color_type = PNG_COLOR_TYPE_RGB_ALPHA;
         row_info->channels = 4;
         row_info->pixel_depth = (xpng_byte)(row_info->bit_depth << 2);
         row_info->rowbytes =
            ((row_width * row_info->pixel_depth) >> 3);
      }
   }
}
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
void /* PRIVATE */
xpng_do_dither(xpng_row_infop row_info, xpng_bytep row,
    xpng_bytep palette_lookup, xpng_bytep dither_lookup)
{
   xpng_bytep sp, dp;
   xpng_uint_32 i;
   xpng_uint_32 row_width=row_info->width;

   xpng_debug(1, "in xpng_do_dither\n");
#if defined(PNG_USELESS_TESTS_SUPPORTED)
   if (row != NULL && row_info != NULL)
#endif
   {
      if (row_info->color_type == PNG_COLOR_TYPE_RGB &&
         palette_lookup && row_info->bit_depth == 8)
      {
         int r, g, b, p;
         sp = row;
         dp = row;
         for (i = 0; i < row_width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;

            /* this looks real messy, but the compiler will reduce
               it down to a reasonable formula.  For example, with
               5 bits per color, we get:
               p = (((r >> 3) & 0x1f) << 10) |
                  (((g >> 3) & 0x1f) << 5) |
                  ((b >> 3) & 0x1f);
               */
            p = (((r >> (8 - PNG_DITHER_RED_BITS)) &
               ((1 << PNG_DITHER_RED_BITS) - 1)) <<
               (PNG_DITHER_GREEN_BITS + PNG_DITHER_BLUE_BITS)) |
               (((g >> (8 - PNG_DITHER_GREEN_BITS)) &
               ((1 << PNG_DITHER_GREEN_BITS) - 1)) <<
               (PNG_DITHER_BLUE_BITS)) |
               ((b >> (8 - PNG_DITHER_BLUE_BITS)) &
               ((1 << PNG_DITHER_BLUE_BITS) - 1));

            *dp++ = palette_lookup[p];
         }
         row_info->color_type = PNG_COLOR_TYPE_PALETTE;
         row_info->channels = 1;
         row_info->pixel_depth = row_info->bit_depth;
         row_info->rowbytes =
             ((row_width * row_info->pixel_depth + 7) >> 3);
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA &&
         palette_lookup != NULL && row_info->bit_depth == 8)
      {
         int r, g, b, p;
         sp = row;
         dp = row;
         for (i = 0; i < row_width; i++)
         {
            r = *sp++;
            g = *sp++;
            b = *sp++;
            sp++;

            p = (((r >> (8 - PNG_DITHER_RED_BITS)) &
               ((1 << PNG_DITHER_RED_BITS) - 1)) <<
               (PNG_DITHER_GREEN_BITS + PNG_DITHER_BLUE_BITS)) |
               (((g >> (8 - PNG_DITHER_GREEN_BITS)) &
               ((1 << PNG_DITHER_GREEN_BITS) - 1)) <<
               (PNG_DITHER_BLUE_BITS)) |
               ((b >> (8 - PNG_DITHER_BLUE_BITS)) &
               ((1 << PNG_DITHER_BLUE_BITS) - 1));

            *dp++ = palette_lookup[p];
         }
         row_info->color_type = PNG_COLOR_TYPE_PALETTE;
         row_info->channels = 1;
         row_info->pixel_depth = row_info->bit_depth;
         row_info->rowbytes =
            ((row_width * row_info->pixel_depth + 7) >> 3);
      }
      else if (row_info->color_type == PNG_COLOR_TYPE_PALETTE &&
         dither_lookup && row_info->bit_depth == 8)
      {
         sp = row;
         for (i = 0; i < row_width; i++, sp++)
         {
            *sp = dither_lookup[*sp];
         }
      }
   }
}
#endif

#ifdef PNG_FLOATING_POINT_SUPPORTED
#if defined(PNG_READ_GAMMA_SUPPORTED)
static int xpng_gamma_shift[] =
   {0x10, 0x21, 0x42, 0x84, 0x110, 0x248, 0x550, 0xff0};

/* We build the 8- or 16-bit gamma tables here.  Note that for 16-bit
 * tables, we don't make a full table if we are reducing to 8-bit in
 * the future.  Note also how the gamma_16 tables are segmented so that
 * we don't need to allocate > 64K chunks for a full 16-bit table.
 */
void /* PRIVATE */
xpng_build_gamma_table(xpng_structp xpng_ptr)
{
  xpng_debug(1, "in xpng_build_gamma_table\n");
  if(xpng_ptr->gamma != 0.0)
  {
   if (xpng_ptr->bit_depth <= 8)
   {
      int i;
      double g;

      if (xpng_ptr->screen_gamma > .000001)
         g = 1.0 / (xpng_ptr->gamma * xpng_ptr->screen_gamma);
      else
         g = 1.0;

      xpng_ptr->gamma_table = (xpng_bytep)xpng_malloc(xpng_ptr,
         (xpng_uint_32)256);

      for (i = 0; i < 256; i++)
      {
         xpng_ptr->gamma_table[i] = (xpng_byte)(pow((double)i / 255.0,
            g) * 255.0 + .5);
      }

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
    defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
      if (xpng_ptr->transformations & ((PNG_BACKGROUND) | PNG_RGB_TO_GRAY))
      {

         g = 1.0 / (xpng_ptr->gamma);

         xpng_ptr->gamma_to_1 = (xpng_bytep)xpng_malloc(xpng_ptr,
            (xpng_uint_32)256);

         for (i = 0; i < 256; i++)
         {
            xpng_ptr->gamma_to_1[i] = (xpng_byte)(pow((double)i / 255.0,
               g) * 255.0 + .5);
         }


         xpng_ptr->gamma_from_1 = (xpng_bytep)xpng_malloc(xpng_ptr,
            (xpng_uint_32)256);

         if(xpng_ptr->screen_gamma > 0.000001)
            g = 1.0 / xpng_ptr->screen_gamma;
         else
            g = xpng_ptr->gamma;   /* probably doing rgb_to_gray */

         for (i = 0; i < 256; i++)
         {
            xpng_ptr->gamma_from_1[i] = (xpng_byte)(pow((double)i / 255.0,
               g) * 255.0 + .5);

         }
      }
#endif /* PNG_READ_BACKGROUND_SUPPORTED || PNG_RGB_TO_GRAY_SUPPORTED */
   }
   else
   {
      double g;
      int i, j, shift, num;
      int sig_bit;
      xpng_uint_32 ig;

      if (xpng_ptr->color_type & PNG_COLOR_MASK_COLOR)
      {
         sig_bit = (int)xpng_ptr->sig_bit.red;
         if ((int)xpng_ptr->sig_bit.green > sig_bit)
            sig_bit = xpng_ptr->sig_bit.green;
         if ((int)xpng_ptr->sig_bit.blue > sig_bit)
            sig_bit = xpng_ptr->sig_bit.blue;
      }
      else
      {
         sig_bit = (int)xpng_ptr->sig_bit.gray;
      }

      if (sig_bit > 0)
         shift = 16 - sig_bit;
      else
         shift = 0;

      if (xpng_ptr->transformations & PNG_16_TO_8)
      {
         if (shift < (16 - PNG_MAX_GAMMA_8))
            shift = (16 - PNG_MAX_GAMMA_8);
      }

      if (shift > 8)
         shift = 8;
      if (shift < 0)
         shift = 0;

      xpng_ptr->gamma_shift = (xpng_byte)shift;

      num = (1 << (8 - shift));

      if (xpng_ptr->screen_gamma > .000001)
         g = 1.0 / (xpng_ptr->gamma * xpng_ptr->screen_gamma);
      else
         g = 1.0;

      xpng_ptr->gamma_16_table = (xpng_uint_16pp)xpng_malloc(xpng_ptr,
         (xpng_uint_32)(num * sizeof (xpng_uint_16p)));

      if (xpng_ptr->transformations & (PNG_16_TO_8 | PNG_BACKGROUND))
      {
         double fin, fout;
         xpng_uint_32 last, max;

         for (i = 0; i < num; i++)
         {
            xpng_ptr->gamma_16_table[i] = (xpng_uint_16p)xpng_malloc(xpng_ptr,
               (xpng_uint_32)(256 * sizeof (xpng_uint_16)));
         }

         g = 1.0 / g;
         last = 0;
         for (i = 0; i < 256; i++)
         {
            fout = ((double)i + 0.5) / 256.0;
            fin = pow(fout, g);
            max = (xpng_uint_32)(fin * (double)((xpng_uint_32)num << 8));
            while (last <= max)
            {
               xpng_ptr->gamma_16_table[(int)(last & (0xff >> shift))]
                  [(int)(last >> (8 - shift))] = (xpng_uint_16)(
                  (xpng_uint_16)i | ((xpng_uint_16)i << 8));
               last++;
            }
         }
         while (last < ((xpng_uint_32)num << 8))
         {
            xpng_ptr->gamma_16_table[(int)(last & (0xff >> shift))]
               [(int)(last >> (8 - shift))] = (xpng_uint_16)65535L;
            last++;
         }
      }
      else
      {
         for (i = 0; i < num; i++)
         {
            xpng_ptr->gamma_16_table[i] = (xpng_uint_16p)xpng_malloc(xpng_ptr,
               (xpng_uint_32)(256 * sizeof (xpng_uint_16)));

            ig = (((xpng_uint_32)i * (xpng_uint_32)xpng_gamma_shift[shift]) >> 4);
            for (j = 0; j < 256; j++)
            {
               xpng_ptr->gamma_16_table[i][j] =
                  (xpng_uint_16)(pow((double)(ig + ((xpng_uint_32)j << 8)) /
                     65535.0, g) * 65535.0 + .5);
            }
         }
      }

#if defined(PNG_READ_BACKGROUND_SUPPORTED) || \
    defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
      if (xpng_ptr->transformations & (PNG_BACKGROUND | PNG_RGB_TO_GRAY))
      {

         g = 1.0 / (xpng_ptr->gamma);

         xpng_ptr->gamma_16_to_1 = (xpng_uint_16pp)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(num * sizeof (xpng_uint_16p )));

         for (i = 0; i < num; i++)
         {
            xpng_ptr->gamma_16_to_1[i] = (xpng_uint_16p)xpng_malloc(xpng_ptr,
               (xpng_uint_32)(256 * sizeof (xpng_uint_16)));

            ig = (((xpng_uint_32)i *
               (xpng_uint_32)xpng_gamma_shift[shift]) >> 4);
            for (j = 0; j < 256; j++)
            {
               xpng_ptr->gamma_16_to_1[i][j] =
                  (xpng_uint_16)(pow((double)(ig + ((xpng_uint_32)j << 8)) /
                     65535.0, g) * 65535.0 + .5);
            }
         }

         if(xpng_ptr->screen_gamma > 0.000001)
            g = 1.0 / xpng_ptr->screen_gamma;
         else
            g = xpng_ptr->gamma;   /* probably doing rgb_to_gray */

         xpng_ptr->gamma_16_from_1 = (xpng_uint_16pp)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(num * sizeof (xpng_uint_16p)));

         for (i = 0; i < num; i++)
         {
            xpng_ptr->gamma_16_from_1[i] = (xpng_uint_16p)xpng_malloc(xpng_ptr,
               (xpng_uint_32)(256 * sizeof (xpng_uint_16)));

            ig = (((xpng_uint_32)i *
               (xpng_uint_32)xpng_gamma_shift[shift]) >> 4);
            for (j = 0; j < 256; j++)
            {
               xpng_ptr->gamma_16_from_1[i][j] =
                  (xpng_uint_16)(pow((double)(ig + ((xpng_uint_32)j << 8)) /
                     65535.0, g) * 65535.0 + .5);
            }
         }
      }
#endif /* PNG_READ_BACKGROUND_SUPPORTED || PNG_RGB_TO_GRAY_SUPPORTED */
   }
 }
}
#endif
/* To do: install integer version of xpng_build_gamma_table here */
#endif

#if defined(PNG_MNG_FEATURES_SUPPORTED)
/* undoes intrapixel differencing  */
void /* PRIVATE */
xpng_do_read_intrapixel(xpng_row_infop row_info, xpng_bytep row)
{
   xpng_debug(1, "in xpng_do_read_intrapixel\n");
   if (
#if defined(PNG_USELESS_TESTS_SUPPORTED)
       row != NULL && row_info != NULL &&
#endif
       (row_info->color_type & PNG_COLOR_MASK_COLOR))
   {
      int bytes_per_pixel;
      xpng_uint_32 row_width = row_info->width;
      if (row_info->bit_depth == 8)
      {
         xpng_bytep rp;
         xpng_uint_32 i;

         if (row_info->color_type == PNG_COLOR_TYPE_RGB)
            bytes_per_pixel = 3;
         else if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            bytes_per_pixel = 4;
         else
            return;

         for (i = 0, rp = row; i < row_width; i++, rp += bytes_per_pixel)
         {
            *(rp) = (xpng_byte)((256 + *rp + *(rp+1))&0xff);
            *(rp+2) = (xpng_byte)((256 + *(rp+2) + *(rp+1))&0xff);
         }
      }
      else if (row_info->bit_depth == 16)
      {
         xpng_bytep rp;
         xpng_uint_32 i;

         if (row_info->color_type == PNG_COLOR_TYPE_RGB)
            bytes_per_pixel = 6;
         else if (row_info->color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            bytes_per_pixel = 8;
         else
            return;

         for (i = 0, rp = row; i < row_width; i++, rp += bytes_per_pixel)
         {
            xpng_uint_32 s0=*(rp  )<<8 | *(rp+1);
            xpng_uint_32 s1=*(rp+2)<<8 | *(rp+3);
            xpng_uint_32 s2=*(rp+4)<<8 | *(rp+5);
            xpng_uint_32 red=(65536+s0+s1)&0xffff;
            xpng_uint_32 blue=(65536+s2+s1)&0xffff;
            *(rp  ) = (xpng_byte)((red>>8)&0xff);
            *(rp+1) = (xpng_byte)(red&0xff);
            *(rp+4) = (xpng_byte)((blue>>8)&0xff);
            *(rp+5) = (xpng_byte)(blue&0xff);
         }
      }
   }
}
#endif /* PNG_MNG_FEATURES_SUPPORTED */
