// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* png.c - location for general purpose libpng functions
 *
 * libpng version 1.2.5 - October 3, 2002
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 */

#define PNG_INTERNAL
#define PNG_NO_EXTERN
#include "png.h"

/* Generate a compiler error if there is an old png.h in the search path. */
typedef version_1_2_5 Your_xpng_h_is_not_version_1_2_5;

/* Version information for C files.  This had better match the version
 * string defined in png.h.  */

#ifdef PNG_USE_GLOBAL_ARRAYS
/* xpng_libxpng_ver was changed to a function in version 1.0.5c */
const char xpng_libxpng_ver[18] = "1.2.5";

/* xpng_sig was changed to a function in version 1.0.5c */
/* Place to hold the signature string for a PNG file. */
const xpng_byte FARDATA xpng_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

/* Invoke global declarations for constant strings for known chunk types */
PNG_IHDR;
PNG_IDAT;
PNG_IEND;
PNG_PLTE;
PNG_bKGD;
PNG_cHRM;
PNG_gAMA;
PNG_hIST;
PNG_iCCP;
PNG_iTXt;
PNG_oFFs;
PNG_pCAL;
PNG_sCAL;
PNG_pHYs;
PNG_sBIT;
PNG_sPLT;
PNG_sRGB;
PNG_tEXt;
PNG_tIME;
PNG_tRNS;
PNG_zTXt;

/* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

/* start of interlace block */
const int FARDATA xpng_pass_start[] = {0, 4, 0, 2, 0, 1, 0};

/* offset to next interlace block */
const int FARDATA xpng_pass_inc[] = {8, 8, 4, 4, 2, 2, 1};

/* start of interlace block in the y direction */
const int FARDATA xpng_pass_ystart[] = {0, 0, 4, 0, 2, 0, 1};

/* offset to next interlace block in the y direction */
const int FARDATA xpng_pass_yinc[] = {8, 8, 8, 4, 4, 2, 2};

/* width of interlace block (used in assembler routines only) */
#ifdef PNG_HAVE_ASSEMBLER_COMBINE_ROW
const int FARDATA xpng_pass_width[] = {8, 4, 4, 2, 2, 1, 1};
#endif

/* Height of interlace block.  This is not currently used - if you need
 * it, uncomment it here and in png.h
const int FARDATA xpng_pass_height[] = {8, 8, 4, 4, 2, 2, 1};
*/

/* Mask to determine which pixels are valid in a pass */
const int FARDATA xpng_pass_mask[] = {0x80, 0x08, 0x88, 0x22, 0xaa, 0x55, 0xff};

/* Mask to determine which pixels to overwrite while displaying */
const int FARDATA xpng_pass_dsp_mask[]
   = {0xff, 0x0f, 0xff, 0x33, 0xff, 0x55, 0xff};

#endif

/* Tells libpng that we have already handled the first "num_bytes" bytes
 * of the PNG file signature.  If the PNG data is embedded into another
 * stream we can set num_bytes = 8 so that libpng will not attempt to read
 * or write any of the magic bytes before it starts on the IHDR.
 */

void PNGAPI
xpng_set_sig_bytes(xpng_structp xpng_ptr, int num_bytes)
{
   xpng_debug(1, "in xpng_set_sig_bytes\n");
   if (num_bytes > 8)
      xpng_error(xpng_ptr, "Too many bytes for PNG signature.");

   xpng_ptr->sig_bytes = (xpng_byte)(num_bytes < 0 ? 0 : num_bytes);
}

/* Checks whether the supplied bytes match the PNG signature.  We allow
 * checking less than the full 8-byte signature so that those apps that
 * already read the first few bytes of a file to determine the file type
 * can simply check the remaining bytes for extra assurance.  Returns
 * an integer less than, equal to, or greater than zero if sig is found,
 * respectively, to be less than, to match, or be greater than the correct
 * PNG signature (this is the same behaviour as strcmp, memcmp, etc).
 */
int PNGAPI
xpng_sig_cmp(xpng_bytep sig, xpng_size_t start, xpng_size_t num_to_check)
{
   xpng_byte xpng_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
   if (num_to_check > 8)
      num_to_check = 8;
   else if (num_to_check < 1)
      return (0);

   if (start > 7)
      return (0);

   if (start + num_to_check > 8)
      num_to_check = 8 - start;

   return ((int)(xpng_memcmp(&sig[start], &xpng_signature[start], num_to_check)));
}

/* (Obsolete) function to check signature bytes.  It does not allow one
 * to check a partial signature.  This function might be removed in the
 * future - use xpng_sig_cmp().  Returns true (nonzero) if the file is a PNG.
 */
int PNGAPI
xpng_check_sig(xpng_bytep sig, int num)
{
  return ((int)!xpng_sig_cmp(sig, (xpng_size_t)0, (xpng_size_t)num));
}

/* Function to allocate memory for zlib and clear it to 0. */
#ifdef PNG_1_0_X
voidpf PNGAPI
#else
voidpf /* private */
#endif
xpng_zalloc(voidpf xpng_ptr, uInt items, uInt size)
{
   xpng_uint_32 num_bytes = (xpng_uint_32)items * size;
   xpng_voidp ptr;
   xpng_structp p=xpng_ptr;
   xpng_uint_32 save_flags=p->flags;

   p->flags|=PNG_FLAG_MALLOC_NULL_MEM_OK;
   ptr = (xpng_voidp)xpng_malloc((xpng_structp)xpng_ptr, num_bytes);
   p->flags=save_flags;

#ifndef PNG_NO_ZALLOC_ZERO
   if (ptr == NULL)
       return ((voidpf)ptr);

   if (num_bytes > (xpng_uint_32)0x8000L)
   {
      xpng_memset(ptr, 0, (xpng_size_t)0x8000L);
      xpng_memset((xpng_bytep)ptr + (xpng_size_t)0x8000L, 0,
         (xpng_size_t)(num_bytes - (xpng_uint_32)0x8000L));
   }
   else
   {
      xpng_memset(ptr, 0, (xpng_size_t)num_bytes);
   }
#endif
   return ((voidpf)ptr);
}

/* function to free memory for zlib */
#ifdef PNG_1_0_X
void PNGAPI
#else
void /* private */
#endif
xpng_zfree(voidpf xpng_ptr, voidpf ptr)
{
   xpng_free((xpng_structp)xpng_ptr, (xpng_voidp)ptr);
}

/* Reset the CRC variable to 32 bits of 1's.  Care must be taken
 * in case CRC is > 32 bits to leave the top bits 0.
 */
void /* PRIVATE */
xpng_reset_crc(xpng_structp xpng_ptr)
{
   xpng_ptr->crc = crc32(0, Z_NULL, 0);
}

/* Calculate the CRC over a section of data.  We can only pass as
 * much data to this routine as the largest single buffer size.  We
 * also check that this data will actually be used before going to the
 * trouble of calculating it.
 */
void /* PRIVATE */
xpng_calculate_crc(xpng_structp xpng_ptr, xpng_bytep ptr, xpng_size_t length)
{
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

   if (need_crc)
      xpng_ptr->crc = crc32(xpng_ptr->crc, ptr, (uInt)length);
}

/* Allocate the memory for an info_struct for the application.  We don't
 * really need the xpng_ptr, but it could potentially be useful in the
 * future.  This should be used in favour of malloc(sizeof(xpng_info))
 * and xpng_info_init() so that applications that want to use a shared
 * libpng don't have to be recompiled if xpng_info changes size.
 */
xpng_infop PNGAPI
xpng_create_info_struct(xpng_structp xpng_ptr)
{
   xpng_infop info_ptr;

   xpng_debug(1, "in xpng_create_info_struct\n");
   if(xpng_ptr == NULL) return (NULL);
#ifdef PNG_USER_MEM_SUPPORTED
   info_ptr = (xpng_infop)xpng_create_struct_2(PNG_STRUCT_INFO,
      xpng_ptr->malloc_fn, xpng_ptr->mem_ptr);
#else
   info_ptr = (xpng_infop)xpng_create_struct(PNG_STRUCT_INFO);
#endif
   if (info_ptr != NULL)
      xpng_info_init_3(&info_ptr, sizeof(xpng_info));

   return (info_ptr);
}

/* This function frees the memory associated with a single info struct.
 * Normally, one would use either xpng_destroy_read_struct() or
 * xpng_destroy_write_struct() to free an info struct, but this may be
 * useful for some applications.
 */
void PNGAPI
xpng_destroy_info_struct(xpng_structp xpng_ptr, xpng_infopp info_ptr_ptr)
{
   xpng_infop info_ptr = NULL;

   xpng_debug(1, "in xpng_destroy_info_struct\n");
   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      xpng_info_destroy(xpng_ptr, info_ptr);

#ifdef PNG_USER_MEM_SUPPORTED
      xpng_destroy_struct_2((xpng_voidp)info_ptr, xpng_ptr->free_fn,
          xpng_ptr->mem_ptr);
#else
      xpng_destroy_struct((xpng_voidp)info_ptr);
#endif
      *info_ptr_ptr = NULL;
   }
}

/* Initialize the info structure.  This is now an internal function (0.89)
 * and applications using it are urged to use xpng_create_info_struct()
 * instead.
 */
#undef xpng_info_init
void PNGAPI
xpng_info_init(xpng_infop info_ptr)
{
   /* We only come here via pre-1.0.12-compiled applications */
   xpng_info_init_3(&info_ptr, 0);
}

void PNGAPI
xpng_info_init_3(xpng_infopp ptr_ptr, xpng_size_t xpng_info_struct_size)
{
   xpng_infop info_ptr = *ptr_ptr;

   xpng_debug(1, "in xpng_info_init_3\n");

   if(sizeof(xpng_info) > xpng_info_struct_size)
     {
       xpng_destroy_struct(info_ptr);
       info_ptr = (xpng_infop)xpng_create_struct(PNG_STRUCT_INFO);
       *ptr_ptr = info_ptr;
     }

   /* set everything to 0 */
   xpng_memset(info_ptr, 0, sizeof (xpng_info));
}

#ifdef PNG_FREE_ME_SUPPORTED
void PNGAPI
xpng_data_freer(xpng_structp xpng_ptr, xpng_infop info_ptr,
   int freer, xpng_uint_32 mask)
{
   xpng_debug(1, "in xpng_data_freer\n");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;
   if(freer == PNG_DESTROY_WILL_FREE_DATA)
      info_ptr->free_me |= mask;
   else if(freer == PNG_USER_WILL_FREE_DATA)
      info_ptr->free_me &= ~mask;
   else
      xpng_warning(xpng_ptr,
         "Unknown freer parameter in xpng_data_freer.");
}
#endif

void PNGAPI
xpng_free_data(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_32 mask,
   int num)
{
   xpng_debug(1, "in xpng_free_data\n");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

#if defined(PNG_TEXT_SUPPORTED)
/* free text item num or (if num == -1) all text items */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_TEXT) & info_ptr->free_me)
#else
if (mask & PNG_FREE_TEXT)
#endif
{
   if (num != -1)
   {
     if (info_ptr->text && info_ptr->text[num].key)
     {
         xpng_free(xpng_ptr, info_ptr->text[num].key);
         info_ptr->text[num].key = NULL;
     }
   }
   else
   {
       int i;
       for (i = 0; i < info_ptr->num_text; i++)
           xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_TEXT, i);
       xpng_free(xpng_ptr, info_ptr->text);
       info_ptr->text = NULL;
       info_ptr->num_text=0;
   }
}
#endif

#if defined(PNG_tRNS_SUPPORTED)
/* free any tRNS entry */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_TRNS) & info_ptr->free_me)
#else
if ((mask & PNG_FREE_TRNS) && (xpng_ptr->flags & PNG_FLAG_FREE_TRNS))
#endif
{
    xpng_free(xpng_ptr, info_ptr->trans);
    info_ptr->valid &= ~PNG_INFO_tRNS;
#ifndef PNG_FREE_ME_SUPPORTED
    xpng_ptr->flags &= ~PNG_FLAG_FREE_TRNS;
#endif
    info_ptr->trans = NULL;
}
#endif

#if defined(PNG_sCAL_SUPPORTED)
/* free any sCAL entry */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_SCAL) & info_ptr->free_me)
#else
if (mask & PNG_FREE_SCAL)
#endif
{
#if defined(PNG_FIXED_POINT_SUPPORTED) && !defined(PNG_FLOATING_POINT_SUPPORTED)
    xpng_free(xpng_ptr, info_ptr->scal_s_width);
    xpng_free(xpng_ptr, info_ptr->scal_s_height);
    info_ptr->scal_s_width = NULL;
    info_ptr->scal_s_height = NULL;
#endif
    info_ptr->valid &= ~PNG_INFO_sCAL;
}
#endif

#if defined(PNG_pCAL_SUPPORTED)
/* free any pCAL entry */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_PCAL) & info_ptr->free_me)
#else
if (mask & PNG_FREE_PCAL)
#endif
{
    xpng_free(xpng_ptr, info_ptr->pcal_purpose);
    xpng_free(xpng_ptr, info_ptr->pcal_units);
    info_ptr->pcal_purpose = NULL;
    info_ptr->pcal_units = NULL;
    if (info_ptr->pcal_params != NULL)
    {
        int i;
        for (i = 0; i < (int)info_ptr->pcal_nparams; i++)
        {
          xpng_free(xpng_ptr, info_ptr->pcal_params[i]);
          info_ptr->pcal_params[i]=NULL;
        }
        xpng_free(xpng_ptr, info_ptr->pcal_params);
        info_ptr->pcal_params = NULL;
    }
    info_ptr->valid &= ~PNG_INFO_pCAL;
}
#endif

#if defined(PNG_iCCP_SUPPORTED)
/* free any iCCP entry */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_ICCP) & info_ptr->free_me)
#else
if (mask & PNG_FREE_ICCP)
#endif
{
    xpng_free(xpng_ptr, info_ptr->iccp_name);
    xpng_free(xpng_ptr, info_ptr->iccp_profile);
    info_ptr->iccp_name = NULL;
    info_ptr->iccp_profile = NULL;
    info_ptr->valid &= ~PNG_INFO_iCCP;
}
#endif

#if defined(PNG_sPLT_SUPPORTED)
/* free a given sPLT entry, or (if num == -1) all sPLT entries */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_SPLT) & info_ptr->free_me)
#else
if (mask & PNG_FREE_SPLT)
#endif
{
   if (num != -1)
   {
      if(info_ptr->splt_palettes)
      {
          xpng_free(xpng_ptr, info_ptr->splt_palettes[num].name);
          xpng_free(xpng_ptr, info_ptr->splt_palettes[num].entries);
          info_ptr->splt_palettes[num].name = NULL;
          info_ptr->splt_palettes[num].entries = NULL;
      }
   }
   else
   {
       if(info_ptr->splt_palettes_num)
       {
         int i;
         for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
            xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_SPLT, i);

         xpng_free(xpng_ptr, info_ptr->splt_palettes);
         info_ptr->splt_palettes = NULL;
         info_ptr->splt_palettes_num = 0;
       }
       info_ptr->valid &= ~PNG_INFO_sPLT;
   }
}
#endif

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_UNKN) & info_ptr->free_me)
#else
if (mask & PNG_FREE_UNKN)
#endif
{
   if (num != -1)
   {
       if(info_ptr->unknown_chunks)
       {
          xpng_free(xpng_ptr, info_ptr->unknown_chunks[num].data);
          info_ptr->unknown_chunks[num].data = NULL;
       }
   }
   else
   {
       int i;

       if(info_ptr->unknown_chunks_num)
       {
         for (i = 0; i < (int)info_ptr->unknown_chunks_num; i++)
            xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_UNKN, i);

         xpng_free(xpng_ptr, info_ptr->unknown_chunks);
         info_ptr->unknown_chunks = NULL;
         info_ptr->unknown_chunks_num = 0;
       }
   }
}
#endif

#if defined(PNG_hIST_SUPPORTED)
/* free any hIST entry */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_HIST)  & info_ptr->free_me)
#else
if ((mask & PNG_FREE_HIST) && (xpng_ptr->flags & PNG_FLAG_FREE_HIST))
#endif
{
    xpng_free(xpng_ptr, info_ptr->hist);
    info_ptr->hist = NULL;
    info_ptr->valid &= ~PNG_INFO_hIST;
#ifndef PNG_FREE_ME_SUPPORTED
    xpng_ptr->flags &= ~PNG_FLAG_FREE_HIST;
#endif
}
#endif

/* free any PLTE entry that was internally allocated */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_PLTE) & info_ptr->free_me)
#else
if ((mask & PNG_FREE_PLTE) && (xpng_ptr->flags & PNG_FLAG_FREE_PLTE))
#endif
{
    xpng_zfree(xpng_ptr, info_ptr->palette);
    info_ptr->palette = NULL;
    info_ptr->valid &= ~PNG_INFO_PLTE;
#ifndef PNG_FREE_ME_SUPPORTED
    xpng_ptr->flags &= ~PNG_FLAG_FREE_PLTE;
#endif
    info_ptr->num_palette = 0;
}

#if defined(PNG_INFO_IMAGE_SUPPORTED)
/* free any image bits attached to the info structure */
#ifdef PNG_FREE_ME_SUPPORTED
if ((mask & PNG_FREE_ROWS) & info_ptr->free_me)
#else
if (mask & PNG_FREE_ROWS)
#endif
{
    if(info_ptr->row_pointers)
    {
       int row;
       for (row = 0; row < (int)info_ptr->height; row++)
       {
          xpng_free(xpng_ptr, info_ptr->row_pointers[row]);
          info_ptr->row_pointers[row]=NULL;
       }
       xpng_free(xpng_ptr, info_ptr->row_pointers);
       info_ptr->row_pointers=NULL;
    }
    info_ptr->valid &= ~PNG_INFO_IDAT;
}
#endif

#ifdef PNG_FREE_ME_SUPPORTED
   if(num == -1)
     info_ptr->free_me &= ~mask;
   else
     info_ptr->free_me &= ~(mask & ~PNG_FREE_MUL);
#endif
}

/* This is an internal routine to free any memory that the info struct is
 * pointing to before re-using it or freeing the struct itself.  Recall
 * that xpng_free() checks for NULL pointers for us.
 */
void /* PRIVATE */
xpng_info_destroy(xpng_structp xpng_ptr, xpng_infop info_ptr)
{
   xpng_debug(1, "in xpng_info_destroy\n");

   xpng_free_data(xpng_ptr, info_ptr, PNG_FREE_ALL, -1);

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
   if (xpng_ptr->num_chunk_list)
   {
       xpng_free(xpng_ptr, xpng_ptr->chunk_list);
       xpng_ptr->chunk_list=NULL;
       xpng_ptr->num_chunk_list=0;
   }
#endif

   xpng_info_init_3(&info_ptr, sizeof(xpng_info));
}

/* This function returns a pointer to the io_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before xpng_write_destroy() or xpng_read_destroy() are called.
 */
xpng_voidp PNGAPI
xpng_get_io_ptr(xpng_structp xpng_ptr)
{
   return (xpng_ptr->io_ptr);
}

#if !defined(PNG_NO_STDIO)
/* Initialize the default input/output functions for the PNG file.  If you
 * use your own read or write routines, you can call either xpng_set_read_fn()
 * or xpng_set_write_fn() instead of xpng_init_io().  If you have defined
 * PNG_NO_STDIO, you must use a function of your own because "FILE *" isn't
 * necessarily available.
 */
void PNGAPI
xpng_init_io(xpng_structp xpng_ptr, xpng_FILE_p fp)
{
   xpng_debug(1, "in xpng_init_io\n");
   xpng_ptr->io_ptr = (xpng_voidp)fp;
}
#endif

#if defined(PNG_TIME_RFC1123_SUPPORTED)
/* Convert the supplied time into an RFC 1123 string suitable for use in
 * a "Creation Time" or other text-based time string.
 */
xpng_charp PNGAPI
xpng_convert_to_rfc1123(xpng_structp xpng_ptr, xpng_timep ptime)
{
   static PNG_CONST char short_months[12][4] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

   if (xpng_ptr->time_buffer == NULL)
   {
      xpng_ptr->time_buffer = (xpng_charp)xpng_malloc(xpng_ptr, (xpng_uint_32)(29*
         sizeof(char)));
   }

#if defined(_WIN32_WCE)
   {
      wchar_t time_buf[29];
      wsprintf(time_buf, TEXT("%d %S %d %02d:%02d:%02d +0000"),
          ptime->day % 32, short_months[(ptime->month - 1) % 12],
        ptime->year, ptime->hour % 24, ptime->minute % 60,
          ptime->second % 61);
      WideCharToMultiByte(CP_ACP, 0, time_buf, -1, xpng_ptr->time_buffer, 29,
          NULL, NULL);
   }
#else
#ifdef USE_FAR_KEYWORD
   {
      char near_time_buf[29];
      sprintf(near_time_buf, "%d %s %d %02d:%02d:%02d +0000",
          ptime->day % 32, short_months[(ptime->month - 1) % 12],
          ptime->year, ptime->hour % 24, ptime->minute % 60,
          ptime->second % 61);
      xpng_memcpy(xpng_ptr->time_buffer, near_time_buf,
          29*sizeof(char));
   }
#else
   sprintf(xpng_ptr->time_buffer, "%d %s %d %02d:%02d:%02d +0000",
       ptime->day % 32, short_months[(ptime->month - 1) % 12],
       ptime->year, ptime->hour % 24, ptime->minute % 60,
       ptime->second % 61);
#endif
#endif /* _WIN32_WCE */
   return ((xpng_charp)xpng_ptr->time_buffer);
}
#endif /* PNG_TIME_RFC1123_SUPPORTED */

#if 0
/* Signature string for a PNG file. */
xpng_bytep PNGAPI
xpng_sig_bytes(void)
{
   return ((xpng_bytep)"\211\120\116\107\015\012\032\012");
}
#endif

xpng_charp PNGAPI
xpng_get_copyright(xpng_structp xpng_ptr)
{
   if (xpng_ptr != NULL || xpng_ptr == NULL)  /* silence compiler warning */
   return ((xpng_charp) "\n libpng version 1.2.5 - October 3, 2002\n\
   Copyright (c) 1998-2002 Glenn Randers-Pehrson\n\
   Copyright (c) 1996-1997 Andreas Dilger\n\
   Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.\n");
   return ((xpng_charp) "");
}

/* The following return the library version as a short string in the
 * format 1.0.0 through 99.99.99zz.  To get the version of *.h files used
 * with your application, print out PNG_LIBPNG_VER_STRING, which is defined
 * in png.h.
 */

xpng_charp PNGAPI
xpng_get_libxpng_ver(xpng_structp xpng_ptr)
{
   /* Version of *.c files used when building libpng */
   if(xpng_ptr != NULL) /* silence compiler warning about unused xpng_ptr */
      return((xpng_charp) "1.2.5");
   return((xpng_charp) "1.2.5");
}

xpng_charp PNGAPI
xpng_get_header_ver(xpng_structp xpng_ptr)
{
   /* Version of *.h files used when building libpng */
   if(xpng_ptr != NULL) /* silence compiler warning about unused xpng_ptr */
      return((xpng_charp) PNG_LIBPNG_VER_STRING);
   return((xpng_charp) PNG_LIBPNG_VER_STRING);
}

xpng_charp PNGAPI
xpng_get_header_version(xpng_structp xpng_ptr)
{
   /* Returns longer string containing both version and date */
   if(xpng_ptr != NULL) /* silence compiler warning about unused xpng_ptr */
      return((xpng_charp) PNG_HEADER_VERSION_STRING);
   return((xpng_charp) PNG_HEADER_VERSION_STRING);
}

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
int PNGAPI
xpng_handle_as_unknown(xpng_structp xpng_ptr, xpng_bytep chunk_name)
{
   /* check chunk_name and return "keep" value if it's on the list, else 0 */
   int i;
   xpng_bytep p;
   if((xpng_ptr == NULL && chunk_name == NULL) || xpng_ptr->num_chunk_list<=0)
      return 0;
   p=xpng_ptr->chunk_list+xpng_ptr->num_chunk_list*5-5;
   for (i = xpng_ptr->num_chunk_list; i; i--, p-=5)
      if (!xpng_memcmp(chunk_name, p, 4))
        return ((int)*(p+4));
   return 0;
}
#endif

/* This function, added to libpng-1.0.6g, is untested. */
int PNGAPI
xpng_reset_zstream(xpng_structp xpng_ptr)
{
   return (inflateReset(&xpng_ptr->zstream));
}

/* This function was added to libpng-1.0.7 */
xpng_uint_32 PNGAPI
xpng_access_version_number(void)
{
   /* Version of *.c files used when building libpng */
   return((xpng_uint_32) 10205L);
}


#if !defined(PNG_1_0_X)
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
    /* GRR:  could add this:   && defined(PNG_MMX_CODE_SUPPORTED) */
/* this INTERNAL function was added to libpng 1.2.0 */
void /* PRIVATE */
xpng_init_mmx_flags (xpng_structp xpng_ptr)
{
    xpng_ptr->mmx_rowbytes_threshold = 0;
    xpng_ptr->mmx_bitdepth_threshold = 0;

#  if (defined(PNG_USE_PNGVCRD) || defined(PNG_USE_PNGGCCRD))

    xpng_ptr->asm_flags |= PNG_ASM_FLAG_MMX_SUPPORT_COMPILED;

    if (xpng_mmx_support() > 0) {
        xpng_ptr->asm_flags |= PNG_ASM_FLAG_MMX_SUPPORT_IN_CPU
#    ifdef PNG_HAVE_ASSEMBLER_COMBINE_ROW
                              | PNG_ASM_FLAG_MMX_READ_COMBINE_ROW
#    endif
#    ifdef PNG_HAVE_ASSEMBLER_READ_INTERLACE
                              | PNG_ASM_FLAG_MMX_READ_INTERLACE
#    endif
#    ifndef PNG_HAVE_ASSEMBLER_READ_FILTER_ROW
                              ;
#    else
                              | PNG_ASM_FLAG_MMX_READ_FILTER_SUB
                              | PNG_ASM_FLAG_MMX_READ_FILTER_UP
                              | PNG_ASM_FLAG_MMX_READ_FILTER_AVG
                              | PNG_ASM_FLAG_MMX_READ_FILTER_PAETH ;

        xpng_ptr->mmx_rowbytes_threshold = PNG_MMX_ROWBYTES_THRESHOLD_DEFAULT;
        xpng_ptr->mmx_bitdepth_threshold = PNG_MMX_BITDEPTH_THRESHOLD_DEFAULT;
#    endif
    } else {
        xpng_ptr->asm_flags &= ~( PNG_ASM_FLAG_MMX_SUPPORT_IN_CPU
                               | PNG_MMX_READ_FLAGS
                               | PNG_MMX_WRITE_FLAGS );
    }

#  else /* !((PNGVCRD || PNGGCCRD) && PNG_ASSEMBLER_CODE_SUPPORTED)) */

    /* clear all MMX flags; no support is compiled in */
    xpng_ptr->asm_flags &= ~( PNG_MMX_FLAGS );

#  endif /* ?(PNGVCRD || PNGGCCRD) */
}

#endif /* !(PNG_ASSEMBLER_CODE_SUPPORTED) */

/* this function was added to libpng 1.2.0 */
#if !defined(PNG_USE_PNGGCCRD) && \
    !(defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_USE_PNGVCRD))
int PNGAPI
xpng_mmx_support(void)
{
    return -1;
}
#endif
#endif /* PNG_1_0_X */
