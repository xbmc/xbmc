// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngwio.c - functions for data output
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This file provides a location for all output.  Users who need
 * special handling are expected to write functions that have the same
 * arguments as these and perform similar functions, but that possibly
 * use different output methods.  Note that you shouldn't change these
 * functions, but rather write replacement functions and then change
 * them at run time with xpng_set_write_fn(...).
 */

#define PNG_INTERNAL
#include "png.h"
#ifdef PNG_WRITE_SUPPORTED

/* Write the data to whatever output you are using.  The default routine
   writes to a file pointer.  Note that this routine sometimes gets called
   with very small lengths, so you should implement some kind of simple
   buffering if you are using unbuffered writes.  This should never be asked
   to write more than 64K on a 16 bit machine.  */

void /* PRIVATE */
xpng_write_data(xpng_structp xpng_ptr, xpng_bytep data, xpng_size_t length)
{
   if (xpng_ptr->write_data_fn != NULL )
      (*(xpng_ptr->write_data_fn))(xpng_ptr, data, length);
   else
      xpng_error(xpng_ptr, "Call to NULL write function");
}

#if !defined(PNG_NO_STDIO)
/* This is the function that does the actual writing of data.  If you are
   not writing to a standard C stream, you should create a replacement
   write_data function and use it at run time with xpng_set_write_fn(), rather
   than changing the library. */
#ifndef USE_FAR_KEYWORD
void PNGAPI
xpng_default_write_data(xpng_structp xpng_ptr, xpng_bytep data, xpng_size_t length)
{
   xpng_uint_32 check;

#if defined(_WIN32_WCE)
   if ( !WriteFile((HANDLE)(xpng_ptr->io_ptr), data, length, &check, NULL) )
      check = 0;
#else
   check = fwrite(data, 1, length, (xpng_FILE_p)(xpng_ptr->io_ptr));
#endif
   if (check != length)
      xpng_error(xpng_ptr, "Write Error");
}
#else
/* this is the model-independent version. Since the standard I/O library
   can't handle far buffers in the medium and small models, we have to copy
   the data.
*/

#define NEAR_BUF_SIZE 1024
#define MIN(a,b) (a <= b ? a : b)

void PNGAPI
xpng_default_write_data(xpng_structp xpng_ptr, xpng_bytep data, xpng_size_t length)
{
   xpng_uint_32 check;
   xpng_byte *near_data;  /* Needs to be "xpng_byte *" instead of "xpng_bytep" */
   xpng_FILE_p io_ptr;

   /* Check if data really is near. If so, use usual code. */
   near_data = (xpng_byte *)CVT_PTR_NOCHECK(data);
   io_ptr = (xpng_FILE_p)CVT_PTR(xpng_ptr->io_ptr);
   if ((xpng_bytep)near_data == data)
   {
#if defined(_WIN32_WCE)
      if ( !WriteFile(io_ptr, near_data, length, &check, NULL) )
         check = 0;
#else
      check = fwrite(near_data, 1, length, io_ptr);
#endif
   }
   else
   {
      xpng_byte buf[NEAR_BUF_SIZE];
      xpng_size_t written, remaining, err;
      check = 0;
      remaining = length;
      do
      {
         written = MIN(NEAR_BUF_SIZE, remaining);
         xpng_memcpy(buf, data, written); /* copy far buffer to near buffer */
#if defined(_WIN32_WCE)
         if ( !WriteFile(io_ptr, buf, written, &err, NULL) )
            err = 0;
#else
         err = fwrite(buf, 1, written, io_ptr);
#endif
         if (err != written)
            break;
         else
            check += err;
         data += written;
         remaining -= written;
      }
      while (remaining != 0);
   }
   if (check != length)
      xpng_error(xpng_ptr, "Write Error");
}

#endif
#endif

/* This function is called to output any data pending writing (normally
   to disk).  After xpng_flush is called, there should be no data pending
   writing in any buffers. */
#if defined(PNG_WRITE_FLUSH_SUPPORTED)
void /* PRIVATE */
xpng_flush(xpng_structp xpng_ptr)
{
   if (xpng_ptr->output_flush_fn != NULL)
      (*(xpng_ptr->output_flush_fn))(xpng_ptr);
}

#if !defined(PNG_NO_STDIO)
void PNGAPI
xpng_default_flush(xpng_structp xpng_ptr)
{
#if !defined(_WIN32_WCE)
   xpng_FILE_p io_ptr;
   io_ptr = (xpng_FILE_p)CVT_PTR((xpng_ptr->io_ptr));
   if (io_ptr != NULL)
      fflush(io_ptr);
#endif
}
#endif
#endif

/* This function allows the application to supply new output functions for
   libpng if standard C streams aren't being used.

   This function takes as its arguments:
   xpng_ptr       - pointer to a png output data structure
   io_ptr        - pointer to user supplied structure containing info about
                   the output functions.  May be NULL.
   write_data_fn - pointer to a new output function that takes as its
                   arguments a pointer to a xpng_struct, a pointer to
                   data to be written, and a 32-bit unsigned int that is
                   the number of bytes to be written.  The new write
                   function should call xpng_error(xpng_ptr, "Error msg")
                   to exit and output any fatal error messages.
   flush_data_fn - pointer to a new flush function that takes as its
                   arguments a pointer to a xpng_struct.  After a call to
                   the flush function, there should be no data in any buffers
                   or pending transmission.  If the output method doesn't do
                   any buffering of ouput, a function prototype must still be
                   supplied although it doesn't have to do anything.  If
                   PNG_WRITE_FLUSH_SUPPORTED is not defined at libpng compile
                   time, output_flush_fn will be ignored, although it must be
                   supplied for compatibility. */
void PNGAPI
xpng_set_write_fn(xpng_structp xpng_ptr, xpng_voidp io_ptr,
   xpng_rw_ptr write_data_fn, xpng_flush_ptr output_flush_fn)
{
   xpng_ptr->io_ptr = io_ptr;

#if !defined(PNG_NO_STDIO)
   if (write_data_fn != NULL)
      xpng_ptr->write_data_fn = write_data_fn;
   else
      xpng_ptr->write_data_fn = xpng_default_write_data;
#else
   xpng_ptr->write_data_fn = write_data_fn;
#endif

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
#if !defined(PNG_NO_STDIO)
   if (output_flush_fn != NULL)
      xpng_ptr->output_flush_fn = output_flush_fn;
   else
      xpng_ptr->output_flush_fn = xpng_default_flush;
#else
   xpng_ptr->output_flush_fn = output_flush_fn;
#endif
#endif /* PNG_WRITE_FLUSH_SUPPORTED */

   /* It is an error to read while writing a png file */
   if (xpng_ptr->read_data_fn != NULL)
   {
      xpng_ptr->read_data_fn = NULL;
      xpng_warning(xpng_ptr,
         "Attempted to set both read_data_fn and write_data_fn in");
      xpng_warning(xpng_ptr,
         "the same structure.  Resetting read_data_fn to NULL.");
   }
}

#if defined(USE_FAR_KEYWORD)
#if defined(_MSC_VER)
void *xpng_far_to_near(xpng_structp xpng_ptr,xpng_voidp ptr, int check)
{
   void *near_ptr;
   void FAR *far_ptr;
   FP_OFF(near_ptr) = FP_OFF(ptr);
   far_ptr = (void FAR *)near_ptr;
   if(check != 0)
      if(FP_SEG(ptr) != FP_SEG(far_ptr))
         xpng_error(xpng_ptr,"segment lost in conversion");
   return(near_ptr);
}
#  else
void *xpng_far_to_near(xpng_structp xpng_ptr,xpng_voidp ptr, int check)
{
   void *near_ptr;
   void FAR *far_ptr;
   near_ptr = (void FAR *)ptr;
   far_ptr = (void FAR *)near_ptr;
   if(check != 0)
      if(far_ptr != ptr)
         xpng_error(xpng_ptr,"segment lost in conversion");
   return(near_ptr);
}
#   endif
#   endif
#endif /* PNG_WRITE_SUPPORTED */
