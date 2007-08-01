// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngrio.c - functions for data input
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This file provides a location for all input.  Users who need
 * special handling are expected to write a function that has the same
 * arguments as this and performs a similar function, but that possibly
 * has a different input method.  Note that you shouldn't change this
 * function, but rather write a replacement function and then make
 * libpng use it at run time with xpng_set_read_fn(...).
 */

#define PNG_INTERNAL
#include "png.h"

/* Read the data from whatever input you are using.  The default routine
   reads from a file pointer.  Note that this routine sometimes gets called
   with very small lengths, so you should implement some kind of simple
   buffering if you are using unbuffered reads.  This should never be asked
   to read more then 64K on a 16 bit machine. */
void /* PRIVATE */
xpng_read_data(xpng_structp xpng_ptr, xpng_bytep data, xpng_size_t length)
{
   xpng_debug1(4,"reading %d bytes\n", (int)length);
   if (xpng_ptr->read_data_fn != NULL)
      (*(xpng_ptr->read_data_fn))(xpng_ptr, data, length);
   else
      xpng_error(xpng_ptr, "Call to NULL read function");
}

#if !defined(PNG_NO_STDIO)
/* This is the function that does the actual reading of data.  If you are
   not reading from a standard C stream, you should create a replacement
   read_data function and use it at run time with xpng_set_read_fn(), rather
   than changing the library. */
#ifndef USE_FAR_KEYWORD
void PNGAPI
xpng_default_read_data(xpng_structp xpng_ptr, xpng_bytep data, xpng_size_t length)
{
   xpng_size_t check;

   /* fread() returns 0 on error, so it is OK to store this in a xpng_size_t
    * instead of an int, which is what fread() actually returns.
    */
#if defined(_WIN32_WCE)
   if ( !ReadFile((HANDLE)(xpng_ptr->io_ptr), data, length, &check, NULL) )
      check = 0;
#else
   check = (xpng_size_t)fread(data, (xpng_size_t)1, length,
      (xpng_FILE_p)xpng_ptr->io_ptr);
#endif

   if (check != length)
      xpng_error(xpng_ptr, "Read Error");
}
#else
/* this is the model-independent version. Since the standard I/O library
   can't handle far buffers in the medium and small models, we have to copy
   the data.
*/

#define NEAR_BUF_SIZE 1024
#define MIN(a,b) (a <= b ? a : b)

static void /* PRIVATE */
xpng_default_read_data(xpng_structp xpng_ptr, xpng_bytep data, xpng_size_t length)
{
   int check;
   xpng_byte *n_data;
   xpng_FILE_p io_ptr;

   /* Check if data really is near. If so, use usual code. */
   n_data = (xpng_byte *)CVT_PTR_NOCHECK(data);
   io_ptr = (xpng_FILE_p)CVT_PTR(xpng_ptr->io_ptr);
   if ((xpng_bytep)n_data == data)
   {
#if defined(_WIN32_WCE)
      if ( !ReadFile((HANDLE)(xpng_ptr->io_ptr), data, length, &check, NULL) )
         check = 0;
#else
      check = fread(n_data, 1, length, io_ptr);
#endif
   }
   else
   {
      xpng_byte buf[NEAR_BUF_SIZE];
      xpng_size_t read, remaining, err;
      check = 0;
      remaining = length;
      do
      {
         read = MIN(NEAR_BUF_SIZE, remaining);
#if defined(_WIN32_WCE)
         if ( !ReadFile((HANDLE)(io_ptr), buf, read, &err, NULL) )
            err = 0;
#else
         err = fread(buf, (xpng_size_t)1, read, io_ptr);
#endif
         xpng_memcpy(data, buf, read); /* copy far buffer to near buffer */
         if(err != read)
            break;
         else
            check += err;
         data += read;
         remaining -= read;
      }
      while (remaining != 0);
   }
   if ((xpng_uint_32)check != (xpng_uint_32)length)
      xpng_error(xpng_ptr, "read Error");
}
#endif
#endif

/* This function allows the application to supply a new input function
   for libpng if standard C streams aren't being used.

   This function takes as its arguments:
   xpng_ptr      - pointer to a png input data structure
   io_ptr       - pointer to user supplied structure containing info about
                  the input functions.  May be NULL.
   read_data_fn - pointer to a new input function that takes as its
                  arguments a pointer to a xpng_struct, a pointer to
                  a location where input data can be stored, and a 32-bit
                  unsigned int that is the number of bytes to be read.
                  To exit and output any fatal error messages the new write
                  function should call xpng_error(xpng_ptr, "Error msg"). */
void PNGAPI
xpng_set_read_fn(xpng_structp xpng_ptr, xpng_voidp io_ptr,
   xpng_rw_ptr read_data_fn)
{
   xpng_ptr->io_ptr = io_ptr;

#if !defined(PNG_NO_STDIO)
   if (read_data_fn != NULL)
      xpng_ptr->read_data_fn = read_data_fn;
   else
      xpng_ptr->read_data_fn = xpng_default_read_data;
#else
   xpng_ptr->read_data_fn = read_data_fn;
#endif

   /* It is an error to write to a read device */
   if (xpng_ptr->write_data_fn != NULL)
   {
      xpng_ptr->write_data_fn = NULL;
      xpng_warning(xpng_ptr,
         "It's an error to set both read_data_fn and write_data_fn in the ");
      xpng_warning(xpng_ptr,
         "same structure.  Resetting write_data_fn to NULL.");
   }

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   xpng_ptr->output_flush_fn = NULL;
#endif
}
