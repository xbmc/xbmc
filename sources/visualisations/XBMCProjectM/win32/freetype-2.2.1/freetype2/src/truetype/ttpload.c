/***************************************************************************/
/*                                                                         */
/*  ttpload.c                                                              */
/*                                                                         */
/*    TrueType-specific tables loader (body).                              */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2004, 2005, 2006 by                         */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H

#include "ttpload.h"

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include "ttgxvar.h"
#endif

#include "tterrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttpload


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_loca                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the locations table.                                          */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
#ifdef FT_OPTIMIZE_MEMORY

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_loca( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error  error;
    FT_ULong  table_len;


    /* we need the size of the `glyf' table for malformed `loca' tables */
    error = face->goto_table( face, TTAG_glyf, stream, &face->glyf_len );
    if ( error )
      goto Exit;

    FT_TRACE2(( "Locations " ));
    error = face->goto_table( face, TTAG_loca, stream, &table_len );
    if ( error )
    {
      error = TT_Err_Locations_Missing;
      goto Exit;
    }

    if ( face->header.Index_To_Loc_Format != 0 )
    {
      if ( table_len >= 0x40000L )
      {
        FT_TRACE2(( "table too large!\n" ));
        error = TT_Err_Invalid_Table;
        goto Exit;
      }
      face->num_locations = (FT_UInt)( table_len >> 2 );
    }
    else
    {
      if ( table_len >= 0x20000L )
      {
        FT_TRACE2(( "table too large!\n" ));
        error = TT_Err_Invalid_Table;
        goto Exit;
      }
      face->num_locations = (FT_UInt)( table_len >> 1 );
    }

    /*
     * Extract the frame.  We don't need to decompress it since
     * we are able to parse it directly.
     */
    if ( FT_FRAME_EXTRACT( table_len, face->glyph_locations ) )
      goto Exit;

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_ULong )
  tt_face_get_location( TT_Face   face,
                        FT_UInt   gindex,
                        FT_UInt  *asize )
  {
    FT_ULong  pos1, pos2;
    FT_Byte*  p;
    FT_Byte*  p_limit;


    pos1 = pos2 = 0;

    if ( gindex < face->num_locations )
    {
      if ( face->header.Index_To_Loc_Format != 0 )
      {
        p       = face->glyph_locations + gindex * 4;
        p_limit = face->glyph_locations + face->num_locations * 4;

        pos1 = FT_NEXT_ULONG( p );
        pos2 = pos1;

        if ( p + 4 <= p_limit )
          pos2 = FT_NEXT_ULONG( p );
      }
      else
      {
        p       = face->glyph_locations + gindex * 2;
        p_limit = face->glyph_locations + face->num_locations * 2;

        pos1 = FT_NEXT_USHORT( p );
        pos2 = pos1;

        if ( p + 2 <= p_limit )
          pos2 = FT_NEXT_USHORT( p );

        pos1 <<= 1;
        pos2 <<= 1;
      }
    }

    /* It isn't mentioned explicitly that the `loca' table must be  */
    /* ordered, but implicitly it refers to the length of an entry  */
    /* as the difference between the current and the next position. */
    /* Anyway, there do exist (malformed) fonts which don't obey    */
    /* this rule, so we are only able to provide an upper bound for */
    /* the size.                                                    */
    if ( pos2 >= pos1 )
      *asize = (FT_UInt)( pos2 - pos1 );
    else
      *asize = (FT_UInt)( face->glyf_len - pos1 );

    return pos1;
  }


  FT_LOCAL_DEF( void )
  tt_face_done_loca( TT_Face  face )
  {
    FT_Stream  stream = face->root.stream;


    FT_FRAME_RELEASE( face->glyph_locations );
    face->num_locations = 0;
  }


#else /* !FT_OPTIMIZE_MEMORY */


  FT_LOCAL_DEF( FT_Error )
  tt_face_load_loca( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_Short   LongOffsets;
    FT_ULong   table_len;


    /* we need the size of the `glyf' table for malformed `loca' tables */
    error = face->goto_table( face, TTAG_glyf, stream, &face->glyf_len );
    if ( error )
      goto Exit;

    FT_TRACE2(( "Locations " ));
    LongOffsets = face->header.Index_To_Loc_Format;

    error = face->goto_table( face, TTAG_loca, stream, &table_len );
    if ( error )
    {
      error = TT_Err_Locations_Missing;
      goto Exit;
    }

    if ( LongOffsets != 0 )
    {
      face->num_locations = (FT_UShort)( table_len >> 2 );

      FT_TRACE2(( "(32bit offsets): %12d ", face->num_locations ));

      if ( FT_NEW_ARRAY( face->glyph_locations, face->num_locations ) )
        goto Exit;

      if ( FT_FRAME_ENTER( face->num_locations * 4L ) )
        goto Exit;

      {
        FT_Long*  loc   = face->glyph_locations;
        FT_Long*  limit = loc + face->num_locations;


        for ( ; loc < limit; loc++ )
          *loc = FT_GET_LONG();
      }

      FT_FRAME_EXIT();
    }
    else
    {
      face->num_locations = (FT_UShort)( table_len >> 1 );

      FT_TRACE2(( "(16bit offsets): %12d ", face->num_locations ));

      if ( FT_NEW_ARRAY( face->glyph_locations, face->num_locations ) )
        goto Exit;

      if ( FT_FRAME_ENTER( face->num_locations * 2L ) )
        goto Exit;

      {
        FT_Long*  loc   = face->glyph_locations;
        FT_Long*  limit = loc + face->num_locations;


        for ( ; loc < limit; loc++ )
          *loc = (FT_Long)( (FT_ULong)FT_GET_USHORT() * 2 );
      }

      FT_FRAME_EXIT();
    }

    FT_TRACE2(( "loaded\n" ));

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_ULong )
  tt_face_get_location( TT_Face   face,
                        FT_UInt   gindex,
                        FT_UInt  *asize )
  {
    FT_ULong  offset;
    FT_UInt   count;


    offset = face->glyph_locations[gindex];
    count  = 0;

    if ( gindex < (FT_UInt)face->num_locations - 1 )
    {
      FT_ULong  offset1 = face->glyph_locations[gindex + 1];


      /* It isn't mentioned explicitly that the `loca' table must be  */
      /* ordered, but implicitly it refers to the length of an entry  */
      /* as the difference between the current and the next position. */
      /* Anyway, there do exist (malformed) fonts which don't obey    */
      /* this rule, so we are only able to provide an upper bound for */
      /* the size.                                                    */
      if ( offset1 >= offset )
        count = (FT_UInt)( offset1 - offset );
      else
        count = (FT_UInt)( face->glyf_len - offset );
    }

    *asize = count;
    return offset;
  }


  FT_LOCAL_DEF( void )
  tt_face_done_loca( TT_Face  face )
  {
    FT_Memory  memory = face->root.memory;


    FT_FREE( face->glyph_locations );
    face->num_locations = 0;
  }


#endif /* !FT_OPTIMIZE_MEMORY */


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_cvt                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the control value table into a face object.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_cvt( TT_Face    face,
                    FT_Stream  stream )
  {
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_ULong   table_len;


    FT_TRACE2(( "CVT " ));

    error = face->goto_table( face, TTAG_cvt, stream, &table_len );
    if ( error )
    {
      FT_TRACE2(( "is missing!\n" ));

      face->cvt_size = 0;
      face->cvt      = NULL;
      error          = TT_Err_Ok;

      goto Exit;
    }

    face->cvt_size = table_len / 2;

    if ( FT_NEW_ARRAY( face->cvt, face->cvt_size ) )
      goto Exit;

    if ( FT_FRAME_ENTER( face->cvt_size * 2L ) )
      goto Exit;

    {
      FT_Short*  cur   = face->cvt;
      FT_Short*  limit = cur + face->cvt_size;


      for ( ; cur <  limit; cur++ )
        *cur = FT_GET_SHORT();
    }

    FT_FRAME_EXIT();
    FT_TRACE2(( "loaded\n" ));

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
    if ( face->doblend )
      error = tt_face_vary_cvt( face, stream );
#endif

  Exit:
    return error;

#else /* !TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    FT_UNUSED( face   );
    FT_UNUSED( stream );

    return TT_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_fpgm                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the font program.                                             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_fpgm( TT_Face    face,
                     FT_Stream  stream )
  {
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    FT_Error  error;
    FT_ULong  table_len;


    FT_TRACE2(( "Font program " ));

    /* The font program is optional */
    error = face->goto_table( face, TTAG_fpgm, stream, &table_len );
    if ( error )
    {
      face->font_program      = NULL;
      face->font_program_size = 0;
      error                   = TT_Err_Ok;

      FT_TRACE2(( "is missing!\n" ));
    }
    else
    {
      face->font_program_size = table_len;
      if ( FT_FRAME_EXTRACT( table_len, face->font_program ) )
        goto Exit;

      FT_TRACE2(( "loaded, %12d bytes\n", face->font_program_size ));
    }

  Exit:
    return error;

#else /* !TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    FT_UNUSED( face   );
    FT_UNUSED( stream );

    return TT_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_prep                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the cvt program.                                              */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  tt_face_load_prep( TT_Face    face,
                     FT_Stream  stream )
  {
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

    FT_Error  error;
    FT_ULong  table_len;


    FT_TRACE2(( "Prep program " ));

    error = face->goto_table( face, TTAG_prep, stream, &table_len );
    if ( error )
    {
      face->cvt_program      = NULL;
      face->cvt_program_size = 0;
      error                  = TT_Err_Ok;

      FT_TRACE2(( "is missing!\n" ));
    }
    else
    {
      face->cvt_program_size = table_len;
      if ( FT_FRAME_EXTRACT( table_len, face->cvt_program ) )
        goto Exit;

      FT_TRACE2(( "loaded, %12d bytes\n", face->cvt_program_size ));
    }

  Exit:
    return error;

#else /* !TT_CONFIG_OPTION_BYTECODE_INTERPRETER */

    FT_UNUSED( face   );
    FT_UNUSED( stream );

    return TT_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    tt_face_load_hdmx                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the `hdmx' table into the face object.                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the input stream.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
#ifdef FT_OPTIMIZE_MEMORY

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hdmx( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;
    FT_UInt    version, nn, num_records;
    FT_ULong   table_size, record_size;
    FT_Byte*   p;
    FT_Byte*   limit;


    /* this table is optional */
    error = face->goto_table( face, TTAG_hdmx, stream, &table_size );
    if ( error || table_size < 8 )
      return TT_Err_Ok;

    if ( FT_FRAME_EXTRACT( table_size, face->hdmx_table ) )
      goto Exit;

    p     = face->hdmx_table;
    limit = p + table_size;

    version     = FT_NEXT_USHORT( p );
    num_records = FT_NEXT_USHORT( p );
    record_size = FT_NEXT_ULONG( p );

    if ( version != 0 || num_records > 255 || record_size > 0x40000 )
    {
      error = TT_Err_Invalid_File_Format;
      goto Fail;
    }

    if ( FT_NEW_ARRAY( face->hdmx_record_sizes, num_records ) )
      goto Fail;

    for ( nn = 0; nn < num_records; nn++ )
    {
      if ( p + record_size > limit )
        break;

      face->hdmx_record_sizes[nn] = p[0];
      p                          += record_size;
    }

    face->hdmx_record_count = nn;
    face->hdmx_table_size   = table_size;
    face->hdmx_record_size  = record_size;

  Exit:
    return error;

  Fail:
    FT_FRAME_RELEASE( face->hdmx_table );
    face->hdmx_table_size = 0;
    goto Exit;
  }


  FT_LOCAL_DEF( void )
  tt_face_free_hdmx( TT_Face  face )
  {
    FT_Stream  stream = face->root.stream;
    FT_Memory  memory = stream->memory;


    FT_FREE( face->hdmx_record_sizes );
    FT_FRAME_RELEASE( face->hdmx_table );
  }

#else /* !FT_OPTIMIZE_MEMORY */

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_hdmx( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    TT_Hdmx    hdmx = &face->hdmx;
    FT_Short   num_records;
    FT_Long    num_glyphs;
    FT_Long    record_size;


    hdmx->version     = 0;
    hdmx->num_records = 0;
    hdmx->records     = 0;

    /* this table is optional */
    error = face->goto_table( face, TTAG_hdmx, stream, 0 );
    if ( error )
      return TT_Err_Ok;

    if ( FT_FRAME_ENTER( 8L ) )
      goto Exit;

    hdmx->version = FT_GET_USHORT();
    num_records   = FT_GET_SHORT();
    record_size   = FT_GET_LONG();

    FT_FRAME_EXIT();

    if ( record_size < 0 || num_records < 0 )
      return TT_Err_Invalid_File_Format;

    /* Only recognize format 0 */
    if ( hdmx->version != 0 )
      goto Exit;

    /* we can't use FT_QNEW_ARRAY here; otherwise tt_face_free_hdmx */
    /* could fail during deallocation                               */
    if ( FT_NEW_ARRAY( hdmx->records, num_records ) )
      goto Exit;

    hdmx->num_records = num_records;
    num_glyphs        = face->root.num_glyphs;
    record_size      -= num_glyphs + 2;

    {
      TT_HdmxEntry  cur   = hdmx->records;
      TT_HdmxEntry  limit = cur + hdmx->num_records;


      for ( ; cur < limit; cur++ )
      {
        /* read record */
        if ( FT_READ_BYTE( cur->ppem      ) ||
             FT_READ_BYTE( cur->max_width ) )
          goto Exit;

        if ( FT_QALLOC( cur->widths, num_glyphs )      ||
             FT_STREAM_READ( cur->widths, num_glyphs ) )
          goto Exit;

        /* skip padding bytes */
        if ( record_size > 0 && FT_STREAM_SKIP( record_size ) )
          goto Exit;
      }
    }

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  tt_face_free_hdmx( TT_Face  face )
  {
    if ( face )
    {
      FT_Int     n;
      FT_Memory  memory = face->root.driver->root.memory;


      for ( n = 0; n < face->hdmx.num_records; n++ )
        FT_FREE( face->hdmx.records[n].widths );

      FT_FREE( face->hdmx.records );
      face->hdmx.num_records = 0;
    }
  }

#endif /* !OPTIMIZE_MEMORY */


  /*************************************************************************/
  /*                                                                       */
  /* Return the advance width table for a given pixel size if it is found  */
  /* in the font's `hdmx' table (if any).                                  */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Byte* )
  tt_face_get_device_metrics( TT_Face  face,
                              FT_UInt  ppem,
                              FT_UInt  gindex )
  {
#ifdef FT_OPTIMIZE_MEMORY

    FT_UInt   nn;
    FT_Byte*  result      = NULL;
    FT_ULong  record_size = face->hdmx_record_size;
    FT_Byte*  record      = face->hdmx_table + 8;


    for ( nn = 0; nn < face->hdmx_record_count; nn++ )
      if ( face->hdmx_record_sizes[nn] == ppem )
      {
        gindex += 2;
        if ( gindex < record_size )
          result = record + nn * record_size + gindex;
        break;
      }

    return result;

#else

    FT_UShort  n;


    for ( n = 0; n < face->hdmx.num_records; n++ )
      if ( face->hdmx.records[n].ppem == ppem )
        return &face->hdmx.records[n].widths[gindex];

    return NULL;

#endif
  }


/* END */
