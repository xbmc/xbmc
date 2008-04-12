/***************************************************************************/
/*                                                                         */
/*  winfnt.c                                                               */
/*                                                                         */
/*    FreeType font driver for Windows FNT/FON files                       */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2006 by                         */
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
#include FT_WINFONTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H

#include "winfnt.h"
#include "fnterrs.h"
#include FT_SERVICE_WINFNT_H
#include FT_SERVICE_XFREE86_NAME_H

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_winfnt


  static const FT_Frame_Field  winmz_header_fields[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  WinMZ_HeaderRec

    FT_FRAME_START( 64 ),
      FT_FRAME_USHORT_LE ( magic ),
      FT_FRAME_SKIP_BYTES( 29 * 2 ),
      FT_FRAME_ULONG_LE  ( lfanew ),
    FT_FRAME_END
  };

  static const FT_Frame_Field  winne_header_fields[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  WinNE_HeaderRec

    FT_FRAME_START( 40 ),
      FT_FRAME_USHORT_LE ( magic ),
      FT_FRAME_SKIP_BYTES( 34 ),
      FT_FRAME_USHORT_LE ( resource_tab_offset ),
      FT_FRAME_USHORT_LE ( rname_tab_offset ),
    FT_FRAME_END
  };

  static const FT_Frame_Field  winfnt_header_fields[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  FT_WinFNT_HeaderRec

    FT_FRAME_START( 148 ),
      FT_FRAME_USHORT_LE( version ),
      FT_FRAME_ULONG_LE ( file_size ),
      FT_FRAME_BYTES    ( copyright, 60 ),
      FT_FRAME_USHORT_LE( file_type ),
      FT_FRAME_USHORT_LE( nominal_point_size ),
      FT_FRAME_USHORT_LE( vertical_resolution ),
      FT_FRAME_USHORT_LE( horizontal_resolution ),
      FT_FRAME_USHORT_LE( ascent ),
      FT_FRAME_USHORT_LE( internal_leading ),
      FT_FRAME_USHORT_LE( external_leading ),
      FT_FRAME_BYTE     ( italic ),
      FT_FRAME_BYTE     ( underline ),
      FT_FRAME_BYTE     ( strike_out ),
      FT_FRAME_USHORT_LE( weight ),
      FT_FRAME_BYTE     ( charset ),
      FT_FRAME_USHORT_LE( pixel_width ),
      FT_FRAME_USHORT_LE( pixel_height ),
      FT_FRAME_BYTE     ( pitch_and_family ),
      FT_FRAME_USHORT_LE( avg_width ),
      FT_FRAME_USHORT_LE( max_width ),
      FT_FRAME_BYTE     ( first_char ),
      FT_FRAME_BYTE     ( last_char ),
      FT_FRAME_BYTE     ( default_char ),
      FT_FRAME_BYTE     ( break_char ),
      FT_FRAME_USHORT_LE( bytes_per_row ),
      FT_FRAME_ULONG_LE ( device_offset ),
      FT_FRAME_ULONG_LE ( face_name_offset ),
      FT_FRAME_ULONG_LE ( bits_pointer ),
      FT_FRAME_ULONG_LE ( bits_offset ),
      FT_FRAME_BYTE     ( reserved ),
      FT_FRAME_ULONG_LE ( flags ),
      FT_FRAME_USHORT_LE( A_space ),
      FT_FRAME_USHORT_LE( B_space ),
      FT_FRAME_USHORT_LE( C_space ),
      FT_FRAME_ULONG_LE ( color_table_offset ),
      FT_FRAME_BYTES    ( reserved1, 16 ),
    FT_FRAME_END
  };


  static void
  fnt_font_done( FNT_Face face )
  {
    FT_Memory  memory = FT_FACE( face )->memory;
    FT_Stream  stream = FT_FACE( face )->stream;
    FNT_Font   font   = face->font;


    if ( !font )
      return;

    if ( font->fnt_frame )
      FT_FRAME_RELEASE( font->fnt_frame );
    FT_FREE( font->family_name );

    FT_FREE( font );
    face->font = 0;
  }


  static FT_Error
  fnt_font_load( FNT_Font   font,
                 FT_Stream  stream )
  {
    FT_Error          error;
    FT_WinFNT_Header  header = &font->header;
    FT_Bool           new_format;
    FT_UInt           size;


    /* first of all, read the FNT header */
    if ( FT_STREAM_SEEK( font->offset )                        ||
         FT_STREAM_READ_FIELDS( winfnt_header_fields, header ) )
      goto Exit;

    /* check header */
    if ( header->version != 0x200 &&
         header->version != 0x300 )
    {
      FT_TRACE2(( "[not a valid FNT file]\n" ));
      error = FNT_Err_Unknown_File_Format;
      goto Exit;
    }

    new_format = FT_BOOL( font->header.version == 0x300 );
    size       = new_format ? 148 : 118;

    if ( header->file_size < size )
    {
      FT_TRACE2(( "[not a valid FNT file]\n" ));
      error = FNT_Err_Unknown_File_Format;
      goto Exit;
    }

    /* Version 2 doesn't have these fields */
    if ( header->version == 0x200 )
    {
      header->flags   = 0;
      header->A_space = 0;
      header->B_space = 0;
      header->C_space = 0;

      header->color_table_offset = 0;
    }

    if ( header->file_type & 1 )
    {
      FT_TRACE2(( "[can't handle vector FNT fonts]\n" ));
      error = FNT_Err_Unknown_File_Format;
      goto Exit;
    }

    /* this is a FNT file/table; extract its frame */
    if ( FT_STREAM_SEEK( font->offset )                         ||
         FT_FRAME_EXTRACT( header->file_size, font->fnt_frame ) )
      goto Exit;

  Exit:
    return error;
  }


  static FT_Error
  fnt_face_get_dll_font( FNT_Face  face,
                         FT_Int    face_index )
  {
    FT_Error         error;
    FT_Stream        stream = FT_FACE( face )->stream;
    FT_Memory        memory = FT_FACE( face )->memory;
    WinMZ_HeaderRec  mz_header;


    face->font = 0;

    /* does it begin with an MZ header? */
    if ( FT_STREAM_SEEK( 0 )                                      ||
         FT_STREAM_READ_FIELDS( winmz_header_fields, &mz_header ) )
      goto Exit;

    error = FNT_Err_Unknown_File_Format;
    if ( mz_header.magic == WINFNT_MZ_MAGIC )
    {
      /* yes, now look for an NE header in the file */
      WinNE_HeaderRec  ne_header;


      if ( FT_STREAM_SEEK( mz_header.lfanew )                       ||
           FT_STREAM_READ_FIELDS( winne_header_fields, &ne_header ) )
        goto Exit;

      error = FNT_Err_Unknown_File_Format;
      if ( ne_header.magic == WINFNT_NE_MAGIC )
      {
        /* good, now look into the resource table for each FNT resource */
        FT_ULong   res_offset  = mz_header.lfanew +
                                   ne_header.resource_tab_offset;
        FT_UShort  size_shift;
        FT_UShort  font_count  = 0;
        FT_ULong   font_offset = 0;


        if ( FT_STREAM_SEEK( res_offset )                    ||
             FT_FRAME_ENTER( ne_header.rname_tab_offset -
                             ne_header.resource_tab_offset ) )
          goto Exit;

        size_shift = FT_GET_USHORT_LE();

        for (;;)
        {
          FT_UShort  type_id, count;


          type_id = FT_GET_USHORT_LE();
          if ( !type_id )
            break;

          count = FT_GET_USHORT_LE();

          if ( type_id == 0x8008U )
          {
            font_count  = count;
            font_offset = (FT_ULong)( FT_STREAM_POS() + 4 +
                                      ( stream->cursor - stream->limit ) );
            break;
          }

          stream->cursor += 4 + count * 12;
        }

        FT_FRAME_EXIT();

        if ( !font_count || !font_offset )
        {
          FT_TRACE2(( "this file doesn't contain any FNT resources!\n" ));
          error = FNT_Err_Unknown_File_Format;
          goto Exit;
        }

        face->root.num_faces = font_count;

        if ( face_index >= font_count )
        {
          error = FNT_Err_Bad_Argument;
          goto Exit;
        }

        if ( FT_NEW( face->font ) )
          goto Exit;

        if ( FT_STREAM_SEEK( font_offset + face_index * 12 ) ||
             FT_FRAME_ENTER( 12 )                            )
          goto Fail;

        face->font->offset     = (FT_ULong)FT_GET_USHORT_LE() << size_shift;
        face->font->fnt_size   = (FT_ULong)FT_GET_USHORT_LE() << size_shift;
        face->font->size_shift = size_shift;

        stream->cursor += 8;

        FT_FRAME_EXIT();

        error = fnt_font_load( face->font, stream );
      }
    }

  Fail:
    if ( error )
      fnt_font_done( face );

  Exit:
    return error;
  }


  typedef struct  FNT_CMapRec_
  {
    FT_CMapRec  cmap;
    FT_UInt32   first;
    FT_UInt32   count;

  } FNT_CMapRec, *FNT_CMap;


  static FT_Error
  fnt_cmap_init( FNT_CMap  cmap )
  {
    FNT_Face  face = (FNT_Face)FT_CMAP_FACE( cmap );
    FNT_Font  font = face->font;


    cmap->first = (FT_UInt32)  font->header.first_char;
    cmap->count = (FT_UInt32)( font->header.last_char - cmap->first + 1 );

    return 0;
  }


  static FT_UInt
  fnt_cmap_char_index( FNT_CMap   cmap,
                       FT_UInt32  char_code )
  {
    FT_UInt  gindex = 0;


    char_code -= cmap->first;
    if ( char_code < cmap->count )
      gindex = char_code + 1; /* we artificially increase the glyph index; */
                              /* FNT_Load_Glyph reverts to the right one   */
    return gindex;
  }


  static FT_UInt
  fnt_cmap_char_next( FNT_CMap    cmap,
                      FT_UInt32  *pchar_code )
  {
    FT_UInt    gindex = 0;
    FT_UInt32  result = 0;
    FT_UInt32  char_code = *pchar_code + 1;


    if ( char_code <= cmap->first )
    {
      result = cmap->first;
      gindex = 1;
    }
    else
    {
      char_code -= cmap->first;
      if ( char_code < cmap->count )
      {
        result = cmap->first + char_code;
        gindex = char_code + 1;
      }
    }

    *pchar_code = result;
    return gindex;
  }


  static const FT_CMap_ClassRec  fnt_cmap_class_rec =
  {
    sizeof ( FNT_CMapRec ),

    (FT_CMap_InitFunc)     fnt_cmap_init,
    (FT_CMap_DoneFunc)     NULL,
    (FT_CMap_CharIndexFunc)fnt_cmap_char_index,
    (FT_CMap_CharNextFunc) fnt_cmap_char_next
  };

  static FT_CMap_Class const  fnt_cmap_class = &fnt_cmap_class_rec;


  static void
  FNT_Face_Done( FNT_Face  face )
  {
    FT_Memory  memory = FT_FACE_MEMORY( face );


    fnt_font_done( face );

    FT_FREE( face->root.available_sizes );
    face->root.num_fixed_sizes = 0;
  }


  static FT_Error
  FNT_Face_Init( FT_Stream      stream,
                 FNT_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error   error;
    FT_Memory  memory = FT_FACE_MEMORY( face );

    FT_UNUSED( num_params );
    FT_UNUSED( params );


    /* try to load font from a DLL */
    error = fnt_face_get_dll_font( face, face_index );
    if ( error )
    {
      /* this didn't work; try to load a single FNT font */
      FNT_Font  font;


      if ( FT_NEW( face->font ) )
        goto Exit;

      face->root.num_faces = 1;

      font           = face->font;
      font->offset   = 0;
      font->fnt_size = stream->size;

      error = fnt_font_load( font, stream );
      if ( error )
        goto Fail;
    }

    /* we now need to fill the root FT_Face fields */
    /* with relevant information                   */
    {
      FT_Face     root = FT_FACE( face );
      FNT_Font    font = face->font;
      FT_PtrDist  family_size;


      root->face_flags = FT_FACE_FLAG_FIXED_SIZES |
                         FT_FACE_FLAG_HORIZONTAL;

      if ( font->header.avg_width == font->header.max_width )
        root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      if ( font->header.italic )
        root->style_flags |= FT_STYLE_FLAG_ITALIC;

      if ( font->header.weight >= 800 )
        root->style_flags |= FT_STYLE_FLAG_BOLD;

      /* set up the `fixed_sizes' array */
      if ( FT_NEW_ARRAY( root->available_sizes, 1 ) )
        goto Fail;

      root->num_fixed_sizes = 1;

      {
        FT_Bitmap_Size*  bsize = root->available_sizes;
        FT_UShort        x_res, y_res;


        bsize->width  = font->header.avg_width;
        bsize->height = (FT_Short)(
          font->header.pixel_height + font->header.external_leading );
        bsize->size   = font->header.nominal_point_size << 6;

        x_res = font->header.horizontal_resolution;
        if ( !x_res )
          x_res = 72;

        y_res = font->header.vertical_resolution;
        if ( !y_res )
          y_res = 72;

        bsize->y_ppem = FT_MulDiv( bsize->size, y_res, 72 );
        bsize->y_ppem = FT_PIX_ROUND( bsize->y_ppem );

        /*
         * this reads:
         *
         * the nominal height is larger than the bbox's height
         *
         * => nominal_point_size contains incorrect value;
         *    use pixel_height as the nominal height
         */
        if ( bsize->y_ppem > font->header.pixel_height << 6 )
        {
          FT_TRACE2(( "use pixel_height as the nominal height\n" ));

          bsize->y_ppem = font->header.pixel_height << 6;
          bsize->size   = FT_MulDiv( bsize->y_ppem, 72, y_res );
        }

        bsize->x_ppem = FT_MulDiv( bsize->size, x_res, 72 );
        bsize->x_ppem = FT_PIX_ROUND( bsize->x_ppem );
      }

      {
        FT_CharMapRec  charmap;


        charmap.encoding    = FT_ENCODING_NONE;
        charmap.platform_id = 0;
        charmap.encoding_id = 0;
        charmap.face        = root;

        if ( font->header.charset == FT_WinFNT_ID_MAC )
        {
          charmap.encoding    = FT_ENCODING_APPLE_ROMAN;
          charmap.platform_id = 1;
/*        charmap.encoding_id = 0; */
        }

        error = FT_CMap_New( fnt_cmap_class,
                             NULL,
                             &charmap,
                             NULL );
        if ( error )
          goto Fail;

        /* Select default charmap */
        if ( root->num_charmaps )
          root->charmap = root->charmaps[0];
      }

      /* setup remaining flags */

      /* reserve one slot for the .notdef glyph at index 0 */
      root->num_glyphs = font->header.last_char -
                         font->header.first_char + 1 + 1;

      /* Some broken fonts don't delimit the face name with a final */
      /* NULL byte -- the frame is erroneously one byte too small.  */
      /* We thus allocate one more byte, setting it explicitly to   */
      /* zero.                                                      */
      family_size = font->header.file_size - font->header.face_name_offset;
      if ( FT_ALLOC( font->family_name, family_size + 1 ) )
        goto Fail;

      FT_MEM_COPY( font->family_name,
                   font->fnt_frame + font->header.face_name_offset,
                   family_size );

      font->family_name[family_size] = '\0';

      if ( FT_REALLOC( font->family_name,
                       family_size,
                       ft_strlen( font->family_name ) + 1 ) )
        goto Fail;

      root->family_name = font->family_name;
      root->style_name  = (char *)"Regular";

      if ( root->style_flags & FT_STYLE_FLAG_BOLD )
      {
        if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
          root->style_name = (char *)"Bold Italic";
        else
          root->style_name = (char *)"Bold";
      }
      else if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
        root->style_name = (char *)"Italic";
    }
    goto Exit;

  Fail:
    FNT_Face_Done( face );

  Exit:
    return error;
  }


  static FT_Error
  FNT_Size_Select( FT_Size  size )
  {
    FNT_Face          face   = (FNT_Face)size->face;
    FT_WinFNT_Header  header = &face->font->header;


    FT_Select_Metrics( size->face, 0 );

    size->metrics.ascender    = header->ascent * 64;
    size->metrics.descender   = -( header->pixel_height -
                                   header->ascent ) * 64;
    size->metrics.max_advance = header->max_width * 64;

    return FNT_Err_Ok;
  }


  static FT_Error
  FNT_Size_Request( FT_Size          size,
                    FT_Size_Request  req )
  {
    FNT_Face          face    = (FNT_Face)size->face;
    FT_WinFNT_Header  header  = &face->font->header;
    FT_Bitmap_Size*   bsize   = size->face->available_sizes;
    FT_Error          error   = FNT_Err_Invalid_Pixel_Size;
    FT_Long           height;


    height = FT_REQUEST_HEIGHT( req );
    height = ( height + 32 ) >> 6;

    switch ( req->type )
    {
    case FT_SIZE_REQUEST_TYPE_NOMINAL:
      if ( height == ( bsize->y_ppem + 32 ) >> 6 )
        error = FNT_Err_Ok;
      break;

    case FT_SIZE_REQUEST_TYPE_REAL_DIM:
      if ( height == header->pixel_height )
        error = FNT_Err_Ok;
      break;

    default:
      error = FNT_Err_Unimplemented_Feature;
      break;
    }

    if ( error )
      return error;
    else
      return FNT_Size_Select( size );
  }


  static FT_Error
  FNT_Load_Glyph( FT_GlyphSlot  slot,
                  FT_Size       size,
                  FT_UInt       glyph_index,
                  FT_Int32      load_flags )
  {
    FNT_Face    face   = (FNT_Face)FT_SIZE_FACE( size );
    FNT_Font    font   = face->font;
    FT_Error    error  = FNT_Err_Ok;
    FT_Byte*    p;
    FT_Int      len;
    FT_Bitmap*  bitmap = &slot->bitmap;
    FT_ULong    offset;
    FT_Bool     new_format;

    FT_UNUSED( load_flags );


    if ( !face || !font )
    {
      error = FNT_Err_Invalid_Argument;
      goto Exit;
    }

    if ( glyph_index > 0 )
      glyph_index--;                           /* revert to real index */
    else
      glyph_index = font->header.default_char; /* the .notdef glyph */

    new_format = FT_BOOL( font->header.version == 0x300 );
    len        = new_format ? 6 : 4;

    /* jump to glyph entry */
    p = font->fnt_frame + ( new_format ? 148 : 118 ) + len * glyph_index;

    bitmap->width = FT_NEXT_SHORT_LE( p );

    if ( new_format )
      offset = FT_NEXT_ULONG_LE( p );
    else
      offset = FT_NEXT_USHORT_LE( p );

    if ( offset >= font->header.file_size )
    {
      FT_TRACE2(( "invalid FNT offset!\n" ));
      error = FNT_Err_Invalid_File_Format;
      goto Exit;
    }

    /* jump to glyph data */
    p = font->fnt_frame + /* font->header.bits_offset */ + offset;

    /* allocate and build bitmap */
    {
      FT_Memory  memory = FT_FACE_MEMORY( slot->face );
      FT_Int     pitch  = ( bitmap->width + 7 ) >> 3;
      FT_Byte*   column;
      FT_Byte*   write;


      bitmap->pitch      = pitch;
      bitmap->rows       = font->header.pixel_height;
      bitmap->pixel_mode = FT_PIXEL_MODE_MONO;

      /* note: since glyphs are stored in columns and not in rows we */
      /*       can't use ft_glyphslot_set_bitmap                     */
      if ( FT_ALLOC_MULT( bitmap->buffer, pitch, bitmap->rows ) )
        goto Exit;

      column = (FT_Byte*)bitmap->buffer;

      for ( ; pitch > 0; pitch--, column++ )
      {
        FT_Byte*  limit = p + bitmap->rows;


        for ( write = column; p < limit; p++, write += bitmap->pitch )
          *write = *p;
      }
    }

    slot->internal->flags = FT_GLYPH_OWN_BITMAP;
    slot->bitmap_left     = 0;
    slot->bitmap_top      = font->header.ascent;
    slot->format          = FT_GLYPH_FORMAT_BITMAP;

    /* now set up metrics */
    slot->metrics.width        = bitmap->width << 6;
    slot->metrics.height       = bitmap->rows << 6;
    slot->metrics.horiAdvance  = bitmap->width << 6;
    slot->metrics.horiBearingX = 0;
    slot->metrics.horiBearingY = slot->bitmap_top << 6;

    ft_synthesize_vertical_metrics( &slot->metrics,
                                    bitmap->rows << 6 );

  Exit:
    return error;
  }


  static FT_Error
  winfnt_get_header( FT_Face               face,
                     FT_WinFNT_HeaderRec  *aheader )
  {
    FNT_Font  font = ((FNT_Face)face)->font;


    *aheader = font->header;

    return 0;
  }


  static const FT_Service_WinFntRec  winfnt_service_rec =
  {
    winfnt_get_header
  };

 /*
  *  SERVICE LIST
  *
  */

  static const FT_ServiceDescRec  winfnt_services[] =
  {
    { FT_SERVICE_ID_XF86_NAME, FT_XF86_FORMAT_WINFNT },
    { FT_SERVICE_ID_WINFNT,    &winfnt_service_rec },
    { NULL, NULL }
  };


  static FT_Module_Interface
  winfnt_get_service( FT_Driver         driver,
                      const FT_String*  service_id )
  {
    FT_UNUSED( driver );

    return ft_service_list_lookup( winfnt_services, service_id );
  }




  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  winfnt_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER        |
      FT_MODULE_DRIVER_NO_OUTLINES,
      sizeof ( FT_DriverRec ),

      "winfonts",
      0x10000L,
      0x20000L,

      0,

      (FT_Module_Constructor)0,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  winfnt_get_service
    },

    sizeof( FNT_FaceRec ),
    sizeof( FT_SizeRec ),
    sizeof( FT_GlyphSlotRec ),

    (FT_Face_InitFunc)        FNT_Face_Init,
    (FT_Face_DoneFunc)        FNT_Face_Done,
    (FT_Size_InitFunc)        0,
    (FT_Size_DoneFunc)        0,
    (FT_Slot_InitFunc)        0,
    (FT_Slot_DoneFunc)        0,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif
    (FT_Slot_LoadFunc)        FNT_Load_Glyph,

    (FT_Face_GetKerningFunc)  0,
    (FT_Face_AttachFunc)      0,
    (FT_Face_GetAdvancesFunc) 0,

    (FT_Size_RequestFunc)     FNT_Size_Request,
    (FT_Size_SelectFunc)      FNT_Size_Select
  };


/* END */
