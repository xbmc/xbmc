// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif

/* pngerror.c - stub functions for i/o and memory allocation
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This file provides a location for all error handling.  Users who
 * need special error handling are expected to write replacement functions
 * and use xpng_set_error_fn() to use those functions.  See the instructions
 * at each function.
 */

#define PNG_INTERNAL
#include "png.h"

static void /* PRIVATE */
xpng_default_error PNGARG((xpng_structp xpng_ptr,
  xpng_const_charp error_message));
static void /* PRIVATE */
xpng_default_warning PNGARG((xpng_structp xpng_ptr,
  xpng_const_charp warning_message));

/* This function is called whenever there is a fatal error.  This function
 * should not be changed.  If there is a need to handle errors differently,
 * you should supply a replacement error function and use xpng_set_error_fn()
 * to replace the error function at run-time.
 */
void PNGAPI
xpng_error(xpng_structp xpng_ptr, xpng_const_charp error_message)
{
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
   char msg[16];
   if (xpng_ptr->flags&(PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))
   {
     int offset = 0;
     if (*error_message == '#')
     {
         for (offset=1; offset<15; offset++)
            if (*(error_message+offset) == ' ')
                break;
         if (xpng_ptr->flags&PNG_FLAG_STRIP_ERROR_TEXT)
         {
            int i;
            for (i=0; i<offset-1; i++)
               msg[i]=error_message[i+1];
            msg[i]='\0';
            error_message=msg;
         }
         else
            error_message+=offset;
     }
     else
     {
         if (xpng_ptr->flags&PNG_FLAG_STRIP_ERROR_TEXT)
         {
            msg[0]='0';        
            msg[1]='\0';
            error_message=msg;
         }
     }
   }
#endif
   if (xpng_ptr->error_fn != NULL)
      (*(xpng_ptr->error_fn))(xpng_ptr, error_message);

   /* if the following returns or doesn't exist, use the default function,
      which will not return */
   xpng_default_error(xpng_ptr, error_message);
}

/* This function is called whenever there is a non-fatal error.  This function
 * should not be changed.  If there is a need to handle warnings differently,
 * you should supply a replacement warning function and use
 * xpng_set_error_fn() to replace the warning function at run-time.
 */
void PNGAPI
xpng_warning(xpng_structp xpng_ptr, xpng_const_charp warning_message)
{
     int offset = 0;
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
   if (xpng_ptr->flags&(PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))
#endif
   {
     if (*warning_message == '#')
     {
         for (offset=1; offset<15; offset++)
            if (*(warning_message+offset) == ' ')
                break;
     }
   }
   if (xpng_ptr->warning_fn != NULL)
      (*(xpng_ptr->warning_fn))(xpng_ptr,
         (xpng_const_charp)(warning_message+offset));
   else
      xpng_default_warning(xpng_ptr, (xpng_const_charp)(warning_message+offset));
}

/* These utilities are used internally to build an error message that relates
 * to the current chunk.  The chunk name comes from xpng_ptr->chunk_name,
 * this is used to prefix the message.  The message is limited in length
 * to 63 bytes, the name characters are output as hex digits wrapped in []
 * if the character is invalid.
 */
#define isnonalpha(c) ((c) < 41 || (c) > 122 || ((c) > 90 && (c) < 97))
static PNG_CONST char xpng_digit[16] = {
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
   'F' };

static void /* PRIVATE */
xpng_format_buffer(xpng_structp xpng_ptr, xpng_charp buffer, xpng_const_charp
   error_message)
{
   int iout = 0, iin = 0;

   while (iin < 4)
   {
      int c = xpng_ptr->chunk_name[iin++];
      if (isnonalpha(c))
      {
         buffer[iout++] = '[';
         buffer[iout++] = xpng_digit[(c & 0xf0) >> 4];
         buffer[iout++] = xpng_digit[c & 0x0f];
         buffer[iout++] = ']';
      }
      else
      {
         buffer[iout++] = (xpng_byte)c;
      }
   }

   if (error_message == NULL)
      buffer[iout] = 0;
   else
   {
      buffer[iout++] = ':';
      buffer[iout++] = ' ';
      xpng_memcpy(buffer+iout, error_message, 64);
      buffer[iout+63] = 0;
   }
}

void PNGAPI
xpng_chunk_error(xpng_structp xpng_ptr, xpng_const_charp error_message)
{
   char msg[18+64];
   xpng_format_buffer(xpng_ptr, msg, error_message);
   xpng_error(xpng_ptr, msg);
}

void PNGAPI
xpng_chunk_warning(xpng_structp xpng_ptr, xpng_const_charp warning_message)
{
   char msg[18+64];
   xpng_format_buffer(xpng_ptr, msg, warning_message);
   xpng_warning(xpng_ptr, msg);
}

/* This is the default error handling function.  Note that replacements for
 * this function MUST NOT RETURN, or the program will likely crash.  This
 * function is used by default, or if the program supplies NULL for the
 * error function pointer in xpng_set_error_fn().
 */
static void /* PRIVATE */
xpng_default_error(xpng_structp xpng_ptr, xpng_const_charp error_message)
{
#ifndef PNG_NO_CONSOLE_IO
#ifdef PNG_ERROR_NUMBERS_SUPPORTED
   if (*error_message == '#')
   {
     int offset;
     char error_number[16];
     for (offset=0; offset<15; offset++)
     {
         error_number[offset] = *(error_message+offset+1);
         if (*(error_message+offset) == ' ')
             break;
     }
     if((offset > 1) && (offset < 15))
     {
       error_number[offset-1]='\0';
       fprintf(stderr, "libpng error no. %s: %s\n", error_number,
          error_message+offset);
     }
     else
       fprintf(stderr, "libpng error: %s, offset=%d\n", error_message,offset);
   }
   else
#endif
   fprintf(stderr, "libpng error: %s\n", error_message);
#else
   if (error_message)
     /* make compiler happy */ ;
#endif

#ifdef PNG_SETJMP_SUPPORTED
#  ifdef USE_FAR_KEYWORD
   {
      jmp_buf jmpbuf;
      xpng_memcpy(jmpbuf,xpng_ptr->jmpbuf,sizeof(jmp_buf));
      longjmp(jmpbuf, 1);
   }
#  else
   longjmp(xpng_ptr->jmpbuf, 1);
# endif
#else
   if (xpng_ptr)
     /* make compiler happy */ ;
   PNG_ABORT();
#endif
}

/* This function is called when there is a warning, but the library thinks
 * it can continue anyway.  Replacement functions don't have to do anything
 * here if you don't want them to.  In the default configuration, xpng_ptr is
 * not used, but it is passed in case it may be useful.
 */
static void /* PRIVATE */
xpng_default_warning(xpng_structp xpng_ptr, xpng_const_charp warning_message)
{
#ifndef PNG_NO_CONSOLE_IO
#  ifdef PNG_ERROR_NUMBERS_SUPPORTED
   if (*warning_message == '#')
   {
     int offset;
     char warning_number[16];
     for (offset=0; offset<15; offset++)
     {
        warning_number[offset]=*(warning_message+offset+1);
        if (*(warning_message+offset) == ' ')
            break;
     }
     if((offset > 1) && (offset < 15))
     {
       warning_number[offset-1]='\0';
       fprintf(stderr, "libpng warning no. %s: %s\n", warning_number,
          warning_message+offset);
     }
     else
       fprintf(stderr, "libpng warning: %s\n", warning_message);
   }
   else
#  endif
     fprintf(stderr, "libpng warning: %s\n", warning_message);
#else
   if (warning_message)
     /* appease compiler */ ;
#endif
   if (xpng_ptr)
      return;
}

/* This function is called when the application wants to use another method
 * of handling errors and warnings.  Note that the error function MUST NOT
 * return to the calling routine or serious problems will occur.  The return
 * method used in the default routine calls longjmp(xpng_ptr->jmpbuf, 1)
 */
void PNGAPI
xpng_set_error_fn(xpng_structp xpng_ptr, xpng_voidp error_ptr,
   xpng_error_ptr error_fn, xpng_error_ptr warning_fn)
{
   xpng_ptr->error_ptr = error_ptr;
   xpng_ptr->error_fn = error_fn;
   xpng_ptr->warning_fn = warning_fn;
}


/* This function returns a pointer to the error_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before xpng_write_destroy and xpng_read_destroy are called.
 */
xpng_voidp PNGAPI
xpng_get_error_ptr(xpng_structp xpng_ptr)
{
   return ((xpng_voidp)xpng_ptr->error_ptr);
}


#ifdef PNG_ERROR_NUMBERS_SUPPORTED
void PNGAPI
xpng_set_strip_error_numbers(xpng_structp xpng_ptr, xpng_uint_32 strip_mode)
{
   if(xpng_ptr != NULL)
   {
     xpng_ptr->flags &=
       ((~(PNG_FLAG_STRIP_ERROR_NUMBERS|PNG_FLAG_STRIP_ERROR_TEXT))&strip_mode);
   }
}
#endif
