#ifndef __FT_SYSTEM_IO_H__
#define __FT_SYSTEM_IO_H__

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****    NOTE: THE CONTENT OF THIS FILE IS NOT CURRENTLY USED      *****/
 /*****          IN NORMAL BUILDS.  CONSIDER IT EXPERIMENTAL.        *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/


 /********************************************************************
  *
  *  designing custom streams is a bit different now
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  *
  */

#include <ft2build.h>
#include FT_INTERNAL_OBJECT_H

FT_BEGIN_HEADER

 /*@*******************************************************************
  *
  * @type: FT_Stream
  *
  * @description:
  *   handle to an input stream object. These are also @FT_Object handles
  */
  typedef struct FT_StreamRec_*    FT_Stream;


 /*@*******************************************************************
  *
  * @type: FT_Stream_Class
  *
  * @description:
  *   opaque handle to a @FT_Stream_ClassRec class structure describing
  *   the methods of input streams
  */
  typedef const struct FT_Stream_ClassRec_*   FT_Stream_Class;


 /*@*******************************************************************
  *
  * @functype: FT_Stream_ReadFunc
  *
  * @description:
  *   a method used to read bytes from an input stream into memory
  *
  * @input:
  *   stream  :: target stream handle
  *   buffer  :: target buffer address
  *   size    :: number of bytes to read
  *
  * @return:
  *   number of bytes effectively read. Must be <= 'size'.
  */
  typedef FT_ULong  (*FT_Stream_ReadFunc)( FT_Stream   stream,
                                           FT_Byte*    buffer,
                                           FT_ULong    size );


 /*@*******************************************************************
  *
  * @functype: FT_Stream_SeekFunc
  *
  * @description:
  *   a method used to seek to a new position within a stream
  *
  * @input:
  *   stream  :: target stream handle
  *   pos     :: new read position, from start of stream
  *
  * @return:
  *   error code. 0 means success
  */
  typedef FT_Error  (*FT_Stream_SeekFunc)( FT_Stream   stream,
                                           FT_ULong    pos );


 /*@*******************************************************************
  *
  * @struct: FT_Stream_ClassRec
  *
  * @description:
  *   a structure used to describe an input stream class
  *
  * @input:
  *   clazz       :: root @FT_ClassRec fields
  *   stream_read :: stream byte read method
  *   stream_seek :: stream seek method
  */
  typedef struct FT_Stream_ClassRec_
  {
    FT_ClassRec          clazz;
    FT_Stream_ReadFunc   stream_read;
    FT_Stream_SeekFunc   stream_seek;

  } FT_Stream_ClassRec;

 /* */

#define  FT_STREAM_CLASS(x)        ((FT_Stream_Class)(x))
#define  FT_STREAM_CLASS__READ(x)  FT_STREAM_CLASS(x)->stream_read
#define  FT_STREAM_CLASS__SEEK(x)  FT_STREAM_CLASS(x)->stream_seek;

 /*@*******************************************************************
  *
  * @struct: FT_StreamRec
  *
  * @description:
  *   the input stream object structure. See @FT_Stream_ClassRec for
  *   its class descriptor
  *
  * @fields:
  *   object      :: root @FT_ObjectRec fields
  *   size        :: size of stream in bytes (0 if unknown)
  *   pos         :: current position within stream
  *   base        :: for memory-based streams, the address of the stream's
  *                  first data byte in memory. NULL otherwise
  *
  *   cursor      :: the current cursor position within an input stream
  *                  frame. Only valid within a FT_FRAME_ENTER .. FT_FRAME_EXIT
  *                  block; NULL otherwise
  *
  *   limit       :: the current frame limit within a FT_FRAME_ENTER ..
  *                  FT_FRAME_EXIT block. NULL otherwise
  */
  typedef struct FT_StreamRec_
  {
    FT_ObjectRec        object;
    FT_ULong            size;
    FT_ULong            pos;
    const FT_Byte*      base;
    const FT_Byte*      cursor;
    const FT_Byte*      limit;

  } FT_StreamRec;

 /* some useful macros */
#define  FT_STREAM(x)    ((FT_Stream)(x))
#define  FT_STREAM_P(x)  ((FT_Stream*)(x))

#define  FT_STREAM__READ(x)  FT_STREAM_CLASS__READ(FT_OBJECT__CLASS(x))
#define  FT_STREAM__SEEK(x)  FT_STREAM_CLASS__SEEK(FT_OBJECT__CLASS(x))

#define  FT_STREAM_IS_BASED(x)  ( FT_STREAM(x)->base != NULL )

 /* */

 /* create new memory-based stream */
  FT_BASE( FT_Error )   ft_stream_new_memory( const FT_Byte*  stream_base,
                                              FT_ULong        stream_size,
                                              FT_Memory       memory,
                                              FT_Stream      *astream );

  FT_BASE( FT_Error )   ft_stream_new_iso( const char*  pathanme,
                                           FT_Memory    memory,
                                           FT_Stream   *astream );


 /* handle to default stream class implementation for a given build */
 /* this is used by "FT_New_Face"                                   */
 /*                                                                 */
  FT_APIVAR( FT_Type )   ft_stream_default_type;

FT_END_HEADER

#endif /* __FT_SYSTEM_STREAM_H__ */
