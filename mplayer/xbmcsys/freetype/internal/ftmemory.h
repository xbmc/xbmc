/***************************************************************************/
/*                                                                         */
/*  ftmemory.h                                                             */
/*                                                                         */
/*    The FreeType memory management macros (specification).               */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2004 by                                     */
/*  David Turner, Robert Wilhelm, and Werner Lemberg                       */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTMEMORY_H__
#define __FTMEMORY_H__


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_TYPES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Macro>                                                               */
  /*    FT_SET_ERROR                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This macro is used to set an implicit `error' variable to a given  */
  /*    expression's value (usually a function call), and convert it to a  */
  /*    boolean which is set whenever the value is != 0.                   */
  /*                                                                       */
#undef  FT_SET_ERROR
#define FT_SET_ERROR( expression ) \
          ( ( error = (expression) ) != 0 )


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           M E M O R Y                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

#ifdef FT_DEBUG_MEMORY

  FT_BASE( FT_Error )
  FT_Alloc_Debug( FT_Memory    memory,
                  FT_Long      size,
                  void*       *P,
                  const char*  file_name,
                  FT_Long      line_no );

  FT_BASE( FT_Error )
  FT_QAlloc_Debug( FT_Memory    memory,
                   FT_Long      size,
                   void*       *P,
                   const char*  file_name,
                   FT_Long      line_no );

  FT_BASE( FT_Error )
  FT_Realloc_Debug( FT_Memory    memory,
                    FT_Long      current,
                    FT_Long      size,
                    void*       *P,
                    const char*  file_name,
                    FT_Long      line_no );

  FT_BASE( FT_Error )
  FT_QRealloc_Debug( FT_Memory    memory,
                     FT_Long      current,
                     FT_Long      size,
                     void*       *P,
                     const char*  file_name,
                     FT_Long      line_no );

  FT_BASE( void )
  FT_Free_Debug( FT_Memory    memory,
                 FT_Pointer   block,
                 const char*  file_name,
                 FT_Long      line_no );

#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Alloc                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Allocates a new block of memory.  The returned area is always      */
  /*    zero-filled; this is a strong convention in many FreeType parts.   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' which handles        */
  /*              allocation.                                              */
  /*                                                                       */
  /*    size   :: The size in bytes of the block to allocate.              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    P      :: A pointer to the fresh new block.  It should be set to   */
  /*              NULL if `size' is 0, or in case of error.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_BASE( FT_Error )
  FT_Alloc( FT_Memory  memory,
            FT_Long    size,
            void*     *P );


  FT_BASE( FT_Error )
  FT_QAlloc( FT_Memory  memory,
             FT_Long    size,
             void*     *p );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Realloc                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Reallocates a block of memory pointed to by `*P' to `Size' bytes   */
  /*    from the heap, possibly changing `*P'.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory  :: A handle to a given `memory object' which handles       */
  /*               reallocation.                                           */
  /*                                                                       */
  /*    current :: The current block size in bytes.                        */
  /*                                                                       */
  /*    size    :: The new block size in bytes.                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    P       :: A pointer to the fresh new block.  It should be set to  */
  /*               NULL if `size' is 0, or in case of error.               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    All callers of FT_Realloc() _must_ provide the current block size  */
  /*    as well as the new one.                                            */
  /*                                                                       */
  FT_BASE( FT_Error )
  FT_Realloc( FT_Memory  memory,
              FT_Long    current,
              FT_Long    size,
              void*     *P );


  FT_BASE( FT_Error )
  FT_QRealloc( FT_Memory  memory,
               FT_Long    current,
               FT_Long    size,
               void*     *p );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Free                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Releases a given block of memory allocated through FT_Alloc().     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' which handles        */
  /*              memory deallocation                                      */
  /*                                                                       */
  /*    P      :: This is the _address_ of a _pointer_ which points to the */
  /*              allocated block.  It is always set to NULL on exit.      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If P or *P is NULL, this function should return successfully.      */
  /*    This is a strong convention within all of FreeType and its         */
  /*    drivers.                                                           */
  /*                                                                       */
  FT_BASE( void )
  FT_Free( FT_Memory  memory,
           void*     *P );


#define FT_MEM_SET( dest, byte, count )     ft_memset( dest, byte, count )

#define FT_MEM_COPY( dest, source, count )  ft_memcpy( dest, source, count )

#define FT_MEM_MOVE( dest, source, count )  ft_memmove( dest, source, count )


#define FT_MEM_ZERO( dest, count )  FT_MEM_SET( dest, 0, count )

#define FT_ZERO( p )                FT_MEM_ZERO( p, sizeof ( *(p) ) )

#define FT_ARRAY_COPY( dest, source, count )                       \
          FT_MEM_COPY( dest, source, (count) * sizeof( *(dest) ) )

#define FT_ARRAY_MOVE( dest, source, count )                       \
          FT_MEM_MOVE( dest, source, (count) * sizeof( *(dest) ) )


  /*************************************************************************/
  /*                                                                       */
  /* We first define FT_MEM_ALLOC, FT_MEM_REALLOC, and FT_MEM_FREE.  All   */
  /* macros use an _implicit_ `memory' parameter to access the current     */
  /* memory allocator.                                                     */
  /*                                                                       */

#ifdef FT_DEBUG_MEMORY

#define FT_MEM_ALLOC( _pointer_, _size_ )                            \
          FT_Alloc_Debug( memory, _size_,                            \
                          (void**)&(_pointer_), __FILE__, __LINE__ )

#define FT_MEM_REALLOC( _pointer_, _current_, _size_ )                 \
          FT_Realloc_Debug( memory, _current_, _size_,                 \
                            (void**)&(_pointer_), __FILE__, __LINE__ )

#define FT_MEM_QALLOC( _pointer_, _size_ )                            \
          FT_QAlloc_Debug( memory, _size_,                            \
                           (void**)&(_pointer_), __FILE__, __LINE__ )

#define FT_MEM_QREALLOC( _pointer_, _current_, _size_ )                 \
          FT_QRealloc_Debug( memory, _current_, _size_,                 \
                             (void**)&(_pointer_), __FILE__, __LINE__ )

#define FT_MEM_FREE( _pointer_ )                                            \
          FT_Free_Debug( memory, (void**)&(_pointer_), __FILE__, __LINE__ )


#else  /* !FT_DEBUG_MEMORY */


#define FT_MEM_ALLOC( _pointer_, _size_ )                  \
          FT_Alloc( memory, _size_, (void**)&(_pointer_) )

#define FT_MEM_FREE( _pointer_ )                  \
          FT_Free( memory, (void**)&(_pointer_) )

#define FT_MEM_REALLOC( _pointer_, _current_, _size_ )                  \
          FT_Realloc( memory, _current_, _size_, (void**)&(_pointer_) )

#define FT_MEM_QALLOC( _pointer_, _size_ )                  \
          FT_QAlloc( memory, _size_, (void**)&(_pointer_) )

#define FT_MEM_QREALLOC( _pointer_, _current_, _size_ )                  \
          FT_QRealloc( memory, _current_, _size_, (void**)&(_pointer_) )

#endif /* !FT_DEBUG_MEMORY */


  /*************************************************************************/
  /*                                                                       */
  /* The following functions macros expect that their pointer argument is  */
  /* _typed_ in order to automatically compute array element sizes.        */
  /*                                                                       */

#define FT_MEM_NEW( _pointer_ )                               \
          FT_MEM_ALLOC( _pointer_, sizeof ( *(_pointer_) ) )

#define FT_MEM_NEW_ARRAY( _pointer_, _count_ )                           \
          FT_MEM_ALLOC( _pointer_, (_count_) * sizeof ( *(_pointer_) ) )

#define FT_MEM_RENEW_ARRAY( _pointer_, _old_, _new_ )                    \
          FT_MEM_REALLOC( _pointer_, (_old_) * sizeof ( *(_pointer_) ),  \
                                     (_new_) * sizeof ( *(_pointer_) ) )

#define FT_MEM_QNEW( _pointer_ )                               \
          FT_MEM_QALLOC( _pointer_, sizeof ( *(_pointer_) ) )

#define FT_MEM_QNEW_ARRAY( _pointer_, _count_ )                           \
          FT_MEM_QALLOC( _pointer_, (_count_) * sizeof ( *(_pointer_) ) )

#define FT_MEM_QRENEW_ARRAY( _pointer_, _old_, _new_ )                    \
          FT_MEM_QREALLOC( _pointer_, (_old_) * sizeof ( *(_pointer_) ),  \
                                      (_new_) * sizeof ( *(_pointer_) ) )


  /*************************************************************************/
  /*                                                                       */
  /* the following macros are obsolete but kept for compatibility reasons  */
  /*                                                                       */

#define FT_MEM_ALLOC_ARRAY( _pointer_, _count_, _type_ )           \
          FT_MEM_ALLOC( _pointer_, (_count_) * sizeof ( _type_ ) )

#define FT_MEM_REALLOC_ARRAY( _pointer_, _old_, _new_, _type_ )    \
          FT_MEM_REALLOC( _pointer_, (_old_) * sizeof ( _type ),   \
                                     (_new_) * sizeof ( _type_ ) )


  /*************************************************************************/
  /*                                                                       */
  /* The following macros are variants of their FT_MEM_XXXX equivalents;   */
  /* they are used to set an _implicit_ `error' variable and return TRUE   */
  /* if an error occured (i.e. if 'error != 0').                           */
  /*                                                                       */

#define FT_ALLOC( _pointer_, _size_ )                       \
          FT_SET_ERROR( FT_MEM_ALLOC( _pointer_, _size_ ) )

#define FT_REALLOC( _pointer_, _current_, _size_ )                       \
          FT_SET_ERROR( FT_MEM_REALLOC( _pointer_, _current_, _size_ ) )

#define FT_FREE( _pointer_ )       \
          FT_MEM_FREE( _pointer_ )

#define FT_QALLOC( _pointer_, _size_ )                       \
          FT_SET_ERROR( FT_MEM_QALLOC( _pointer_, _size_ ) )

#define FT_QREALLOC( _pointer_, _current_, _size_ )                       \
          FT_SET_ERROR( FT_MEM_QREALLOC( _pointer_, _current_, _size_ ) )


#define FT_NEW( _pointer_ )  \
          FT_SET_ERROR( FT_MEM_NEW( _pointer_ ) )

#define FT_NEW_ARRAY( _pointer_, _count_ )  \
          FT_SET_ERROR( FT_MEM_NEW_ARRAY( _pointer_, _count_ ) )

#define FT_RENEW_ARRAY( _pointer_, _old_, _new_ )   \
          FT_SET_ERROR( FT_MEM_RENEW_ARRAY( _pointer_, _old_, _new_ ) )

#define FT_QNEW( _pointer_ )  \
          FT_SET_ERROR( FT_MEM_QNEW( _pointer_ ) )

#define FT_QNEW_ARRAY( _pointer_, _count_ )  \
          FT_SET_ERROR( FT_MEM_QNEW_ARRAY( _pointer_, _count_ ) )

#define FT_QRENEW_ARRAY( _pointer_, _old_, _new_ )   \
          FT_SET_ERROR( FT_MEM_QRENEW_ARRAY( _pointer_, _old_, _new_ ) )


#define FT_ALLOC_ARRAY( _pointer_, _count_, _type_ )                    \
          FT_SET_ERROR( FT_MEM_ALLOC( _pointer_,                        \
                                      (_count_) * sizeof ( _type_ ) ) )

#define FT_REALLOC_ARRAY( _pointer_, _old_, _new_, _type_ )             \
          FT_SET_ERROR( FT_MEM_REALLOC( _pointer_,                      \
                                        (_old_) * sizeof ( _type_ ),    \
                                        (_new_) * sizeof ( _type_ ) ) )

 /* */


FT_END_HEADER

#endif /* __FTMEMORY_H__ */


/* END */
