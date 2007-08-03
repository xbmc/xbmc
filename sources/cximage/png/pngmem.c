// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngmem.c - stub functions for memory allocation
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This file provides a location for all memory allocation.  Users who
 * need special memory handling are expected to supply replacement
 * functions for xpng_malloc() and xpng_free(), and to use
 * xpng_create_read_struct_2() and xpng_create_write_struct_2() to
 * identify the replacement functions.
 */

#define PNG_INTERNAL
#include "png.h"

/* Borland DOS special memory handler */
#if defined(__TURBOC__) && !defined(_Windows) && !defined(__FLAT__)
/* if you change this, be sure to change the one in png.h also */

/* Allocate memory for a xpng_struct.  The malloc and memset can be replaced
   by a single call to calloc() if this is thought to improve performance. */
xpng_voidp /* PRIVATE */
xpng_create_struct(int type)
{
#ifdef PNG_USER_MEM_SUPPORTED
   return (xpng_create_struct_2(type, xpng_malloc_ptr_NULL, xpng_voidp_NULL));
}

/* Alternate version of xpng_create_struct, for use with user-defined malloc. */
xpng_voidp /* PRIVATE */
xpng_create_struct_2(int type, xpng_malloc_ptr malloc_fn, xpng_voidp mem_ptr)
{
#endif /* PNG_USER_MEM_SUPPORTED */
   xpng_size_t size;
   xpng_voidp struct_ptr;

   if (type == PNG_STRUCT_INFO)
     size = sizeof(xpng_info);
   else if (type == PNG_STRUCT_PNG)
     size = sizeof(xpng_struct);
   else
     return (xpng_get_copyright());

#ifdef PNG_USER_MEM_SUPPORTED
   if(malloc_fn != NULL)
   {
      xpng_struct dummy_struct;
      xpng_structp xpng_ptr = &dummy_struct;
      xpng_ptr->mem_ptr=mem_ptr;
      struct_ptr = (*(malloc_fn))(xpng_ptr, (xpng_uint_32)size);
   }
   else
#endif /* PNG_USER_MEM_SUPPORTED */
      struct_ptr = (xpng_voidp)farmalloc(size));
   if (struct_ptr != NULL)
      xpng_memset(struct_ptr, 0, size);
   return (struct_ptr);
}

/* Free memory allocated by a xpng_create_struct() call */
void /* PRIVATE */
xpng_destroy_struct(xpng_voidp struct_ptr)
{
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_destroy_struct_2(struct_ptr, xpng_free_ptr_NULL, xpng_voidp_NULL);
}

/* Free memory allocated by a xpng_create_struct() call */
void /* PRIVATE */
xpng_destroy_struct_2(xpng_voidp struct_ptr, xpng_free_ptr free_fn,
    xpng_voidp mem_ptr)
{
#endif
   if (struct_ptr != NULL)
   {
#ifdef PNG_USER_MEM_SUPPORTED
      if(free_fn != NULL)
      {
         xpng_struct dummy_struct;
         xpng_structp xpng_ptr = &dummy_struct;
         xpng_ptr->mem_ptr=mem_ptr;
         (*(free_fn))(xpng_ptr, struct_ptr);
         return;
      }
#endif /* PNG_USER_MEM_SUPPORTED */
      farfree (struct_ptr);
   }
}

/* Allocate memory.  For reasonable files, size should never exceed
 * 64K.  However, zlib may allocate more then 64K if you don't tell
 * it not to.  See zconf.h and png.h for more information. zlib does
 * need to allocate exactly 64K, so whatever you call here must
 * have the ability to do that.
 *
 * Borland seems to have a problem in DOS mode for exactly 64K.
 * It gives you a segment with an offset of 8 (perhaps to store its
 * memory stuff).  zlib doesn't like this at all, so we have to
 * detect and deal with it.  This code should not be needed in
 * Windows or OS/2 modes, and only in 16 bit mode.  This code has
 * been updated by Alexander Lehmann for version 0.89 to waste less
 * memory.
 *
 * Note that we can't use xpng_size_t for the "size" declaration,
 * since on some systems a xpng_size_t is a 16-bit quantity, and as a
 * result, we would be truncating potentially larger memory requests
 * (which should cause a fatal error) and introducing major problems.
 */

xpng_voidp PNGAPI
xpng_malloc(xpng_structp xpng_ptr, xpng_uint_32 size)
{
   xpng_voidp ret;

   if (xpng_ptr == NULL || size == 0)
      return (NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   if(xpng_ptr->malloc_fn != NULL)
   {
       ret = ((xpng_voidp)(*(xpng_ptr->malloc_fn))(xpng_ptr, (xpng_size_t)size));
       if (ret == NULL && (xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
          xpng_error(xpng_ptr, "Out of memory!");
       return (ret);
   }
   else
       return xpng_malloc_default(xpng_ptr, size);
}

xpng_voidp PNGAPI
xpng_malloc_default(xpng_structp xpng_ptr, xpng_uint_32 size)
{
   xpng_voidp ret;
#endif /* PNG_USER_MEM_SUPPORTED */

#ifdef PNG_MAX_MALLOC_64K
   if (size > (xpng_uint_32)65536L)
      xpng_error(xpng_ptr, "Cannot Allocate > 64K");
#endif

   if (size == (xpng_uint_32)65536L)
   {
      if (xpng_ptr->offset_table == NULL)
      {
         /* try to see if we need to do any of this fancy stuff */
         ret = farmalloc(size);
         if (ret == NULL || ((xpng_size_t)ret & 0xffff))
         {
            int num_blocks;
            xpng_uint_32 total_size;
            xpng_bytep table;
            int i;
            xpng_byte huge * hptr;

            if (ret != NULL)
            {
               farfree(ret);
               ret = NULL;
            }

            if(xpng_ptr->zlib_window_bits > 14)
               num_blocks = (int)(1 << (xpng_ptr->zlib_window_bits - 14));
            else
               num_blocks = 1;
            if (xpng_ptr->zlib_mem_level >= 7)
               num_blocks += (int)(1 << (xpng_ptr->zlib_mem_level - 7));
            else
               num_blocks++;

            total_size = ((xpng_uint_32)65536L) * (xpng_uint_32)num_blocks+16;

            table = farmalloc(total_size);

            if (table == NULL)
            {
               if (xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
                  xpng_error(xpng_ptr, "Out Of Memory."); /* Note "O" and "M" */
               else
                  xpng_warning(xpng_ptr, "Out Of Memory.");
               return (NULL);
            }

            if ((xpng_size_t)table & 0xfff0)
            {
               if (xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
                  xpng_error(xpng_ptr,
                    "Farmalloc didn't return normalized pointer");
               else
                  xpng_warning(xpng_ptr,
                    "Farmalloc didn't return normalized pointer");
               return (NULL);
            }

            xpng_ptr->offset_table = table;
            xpng_ptr->offset_table_ptr = farmalloc(num_blocks *
               sizeof (xpng_bytep));

            if (xpng_ptr->offset_table_ptr == NULL)
            {
               if (xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
                  xpng_error(xpng_ptr, "Out Of memory."); /* Note "O" and "M" */
               else
                  xpng_warning(xpng_ptr, "Out Of memory.");
               return (NULL);
            }

            hptr = (xpng_byte huge *)table;
            if ((xpng_size_t)hptr & 0xf)
            {
               hptr = (xpng_byte huge *)((long)(hptr) & 0xfffffff0L);
               hptr = hptr + 16L;  /* "hptr += 16L" fails on Turbo C++ 3.0 */
            }
            for (i = 0; i < num_blocks; i++)
            {
               xpng_ptr->offset_table_ptr[i] = (xpng_bytep)hptr;
               hptr = hptr + (xpng_uint_32)65536L;  /* "+=" fails on TC++3.0 */
            }

            xpng_ptr->offset_table_number = num_blocks;
            xpng_ptr->offset_table_count = 0;
            xpng_ptr->offset_table_count_free = 0;
         }
      }

      if (xpng_ptr->offset_table_count >= xpng_ptr->offset_table_number)
      {
         if (xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
            xpng_error(xpng_ptr, "Out of Memory."); /* Note "o" and "M" */
         else
            xpng_warning(xpng_ptr, "Out of Memory.");
         return (NULL);
      }

      ret = xpng_ptr->offset_table_ptr[xpng_ptr->offset_table_count++];
   }
   else
      ret = farmalloc(size);

   if (ret == NULL)
   {
      if (xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
         xpng_error(xpng_ptr, "Out of memory."); /* Note "o" and "m" */
      else
         xpng_warning(xpng_ptr, "Out of memory."); /* Note "o" and "m" */
   }

   return (ret);
}

/* free a pointer allocated by xpng_malloc().  In the default
   configuration, xpng_ptr is not used, but is passed in case it
   is needed.  If ptr is NULL, return without taking any action. */
void PNGAPI
xpng_free(xpng_structp xpng_ptr, xpng_voidp ptr)
{
   if (xpng_ptr == NULL || ptr == NULL)
      return;

#ifdef PNG_USER_MEM_SUPPORTED
   if (xpng_ptr->free_fn != NULL)
   {
      (*(xpng_ptr->free_fn))(xpng_ptr, ptr);
      return;
   }
   else xpng_free_default(xpng_ptr, ptr);
}

void PNGAPI
xpng_free_default(xpng_structp xpng_ptr, xpng_voidp ptr)
{
#endif /* PNG_USER_MEM_SUPPORTED */

   if (xpng_ptr->offset_table != NULL)
   {
      int i;

      for (i = 0; i < xpng_ptr->offset_table_count; i++)
      {
         if (ptr == xpng_ptr->offset_table_ptr[i])
         {
            ptr = NULL;
            xpng_ptr->offset_table_count_free++;
            break;
         }
      }
      if (xpng_ptr->offset_table_count_free == xpng_ptr->offset_table_count)
      {
         farfree(xpng_ptr->offset_table);
         farfree(xpng_ptr->offset_table_ptr);
         xpng_ptr->offset_table = NULL;
         xpng_ptr->offset_table_ptr = NULL;
      }
   }

   if (ptr != NULL)
   {
      farfree(ptr);
   }
}

#else /* Not the Borland DOS special memory handler */

/* Allocate memory for a xpng_struct or a xpng_info.  The malloc and
   memset can be replaced by a single call to calloc() if this is thought
   to improve performance noticably. */
xpng_voidp /* PRIVATE */
xpng_create_struct(int type)
{
#ifdef PNG_USER_MEM_SUPPORTED
   return (xpng_create_struct_2(type, xpng_malloc_ptr_NULL, xpng_voidp_NULL));
}

/* Allocate memory for a xpng_struct or a xpng_info.  The malloc and
   memset can be replaced by a single call to calloc() if this is thought
   to improve performance noticably. */
xpng_voidp /* PRIVATE */
xpng_create_struct_2(int type, xpng_malloc_ptr malloc_fn, xpng_voidp mem_ptr)
{
#endif /* PNG_USER_MEM_SUPPORTED */
   xpng_size_t size;
   xpng_voidp struct_ptr;

   if (type == PNG_STRUCT_INFO)
      size = sizeof(xpng_info);
   else if (type == PNG_STRUCT_PNG)
      size = sizeof(xpng_struct);
   else
      return (NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   if(malloc_fn != NULL)
   {
      xpng_struct dummy_struct;
      xpng_structp xpng_ptr = &dummy_struct;
      xpng_ptr->mem_ptr=mem_ptr;
      struct_ptr = (*(malloc_fn))(xpng_ptr, size);
      if (struct_ptr != NULL)
         xpng_memset(struct_ptr, 0, size);
      return (struct_ptr);
   }
#endif /* PNG_USER_MEM_SUPPORTED */

#if defined(__TURBOC__) && !defined(__FLAT__)
   if ((struct_ptr = (xpng_voidp)farmalloc(size)) != NULL)
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
   if ((struct_ptr = (xpng_voidp)halloc(size,1)) != NULL)
# else
   if ((struct_ptr = (xpng_voidp)malloc(size)) != NULL)
# endif
#endif
   {
      xpng_memset(struct_ptr, 0, size);
   }

   return (struct_ptr);
}


/* Free memory allocated by a xpng_create_struct() call */
void /* PRIVATE */
xpng_destroy_struct(xpng_voidp struct_ptr)
{
#ifdef PNG_USER_MEM_SUPPORTED
   xpng_destroy_struct_2(struct_ptr, xpng_free_ptr_NULL, xpng_voidp_NULL);
}

/* Free memory allocated by a xpng_create_struct() call */
void /* PRIVATE */
xpng_destroy_struct_2(xpng_voidp struct_ptr, xpng_free_ptr free_fn,
    xpng_voidp mem_ptr)
{
#endif /* PNG_USER_MEM_SUPPORTED */
   if (struct_ptr != NULL)
   {
#ifdef PNG_USER_MEM_SUPPORTED
      if(free_fn != NULL)
      {
         xpng_struct dummy_struct;
         xpng_structp xpng_ptr = &dummy_struct;
         xpng_ptr->mem_ptr=mem_ptr;
         (*(free_fn))(xpng_ptr, struct_ptr);
         return;
      }
#endif /* PNG_USER_MEM_SUPPORTED */
#if defined(__TURBOC__) && !defined(__FLAT__)
      farfree(struct_ptr);
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
      hfree(struct_ptr);
# else
      free(struct_ptr);
# endif
#endif
   }
}

/* Allocate memory.  For reasonable files, size should never exceed
   64K.  However, zlib may allocate more then 64K if you don't tell
   it not to.  See zconf.h and png.h for more information.  zlib does
   need to allocate exactly 64K, so whatever you call here must
   have the ability to do that. */

xpng_voidp PNGAPI
xpng_malloc(xpng_structp xpng_ptr, xpng_uint_32 size)
{
   xpng_voidp ret;

   if (xpng_ptr == NULL || size == 0)
      return (NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   if(xpng_ptr->malloc_fn != NULL)
   {
       ret = ((xpng_voidp)(*(xpng_ptr->malloc_fn))(xpng_ptr, (xpng_size_t)size));
       if (ret == NULL && (xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
          xpng_error(xpng_ptr, "Out of Memory!");
       return (ret);
   }
   else
       return (xpng_malloc_default(xpng_ptr, size));
}

xpng_voidp PNGAPI
xpng_malloc_default(xpng_structp xpng_ptr, xpng_uint_32 size)
{
   xpng_voidp ret;
#endif /* PNG_USER_MEM_SUPPORTED */

#ifdef PNG_MAX_MALLOC_64K
   if (size > (xpng_uint_32)65536L)
   {
      if(xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
         xpng_error(xpng_ptr, "Cannot Allocate > 64K");
      else
         return NULL;
   }
#endif

#if defined(__TURBOC__) && !defined(__FLAT__)
   ret = farmalloc(size);
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
   ret = halloc(size, 1);
# else
   ret = malloc((size_t)size);
# endif
#endif

   if (ret == NULL && (xpng_ptr->flags&PNG_FLAG_MALLOC_NULL_MEM_OK) == 0)
      xpng_error(xpng_ptr, "Out of Memory");

   return (ret);
}

/* Free a pointer allocated by xpng_malloc().  If ptr is NULL, return
   without taking any action. */
void PNGAPI
xpng_free(xpng_structp xpng_ptr, xpng_voidp ptr)
{
   if (xpng_ptr == NULL || ptr == NULL)
      return;

#ifdef PNG_USER_MEM_SUPPORTED
   if (xpng_ptr->free_fn != NULL)
   {
      (*(xpng_ptr->free_fn))(xpng_ptr, ptr);
      return;
   }
   else xpng_free_default(xpng_ptr, ptr);
}
void PNGAPI
xpng_free_default(xpng_structp xpng_ptr, xpng_voidp ptr)
{
   if (xpng_ptr == NULL || ptr == NULL)
      return;

#endif /* PNG_USER_MEM_SUPPORTED */

#if defined(__TURBOC__) && !defined(__FLAT__)
   farfree(ptr);
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
   hfree(ptr);
# else
   free(ptr);
# endif
#endif
}

#endif /* Not Borland DOS special memory handler */

#if defined(PNG_1_0_X)
#  define xpng_malloc_warn xpng_malloc
#else
/* This function was added at libpng version 1.2.3.  The xpng_malloc_warn()
 * function will issue a xpng_warning and return NULL instead of issuing a
 * xpng_error, if it fails to allocate the requested memory.
 */
xpng_voidp PNGAPI
xpng_malloc_warn(xpng_structp xpng_ptr, xpng_uint_32 size)
{
   xpng_voidp ptr;
   xpng_uint_32 save_flags=xpng_ptr->flags;

   xpng_ptr->flags|=PNG_FLAG_MALLOC_NULL_MEM_OK;
   ptr = (xpng_voidp)xpng_malloc((xpng_structp)xpng_ptr, size);
   xpng_ptr->flags=save_flags;
   return(ptr);
}
#endif

xpng_voidp PNGAPI
xpng_memcpy_check (xpng_structp xpng_ptr, xpng_voidp s1, xpng_voidp s2,
   xpng_uint_32 length)
{
   xpng_size_t size;

   size = (xpng_size_t)length;
   if ((xpng_uint_32)size != length)
      xpng_error(xpng_ptr,"Overflow in xpng_memcpy_check.");

   return(xpng_memcpy (s1, s2, size));
}

xpng_voidp PNGAPI
xpng_memset_check (xpng_structp xpng_ptr, xpng_voidp s1, int value,
   xpng_uint_32 length)
{
   xpng_size_t size;

   size = (xpng_size_t)length;
   if ((xpng_uint_32)size != length)
      xpng_error(xpng_ptr,"Overflow in xpng_memset_check.");

   return (xpng_memset (s1, value, size));

}

#ifdef PNG_USER_MEM_SUPPORTED
/* This function is called when the application wants to use another method
 * of allocating and freeing memory.
 */
void PNGAPI
xpng_set_mem_fn(xpng_structp xpng_ptr, xpng_voidp mem_ptr, xpng_malloc_ptr
  malloc_fn, xpng_free_ptr free_fn)
{
   xpng_ptr->mem_ptr = mem_ptr;
   xpng_ptr->malloc_fn = malloc_fn;
   xpng_ptr->free_fn = free_fn;
}

/* This function returns a pointer to the mem_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before xpng_write_destroy and xpng_read_destroy are called.
 */
xpng_voidp PNGAPI
xpng_get_mem_ptr(xpng_structp xpng_ptr)
{
   return ((xpng_voidp)xpng_ptr->mem_ptr);
}
#endif /* PNG_USER_MEM_SUPPORTED */
