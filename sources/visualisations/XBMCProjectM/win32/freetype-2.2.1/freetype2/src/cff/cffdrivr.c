/***************************************************************************/
/*                                                                         */
/*  cffdrivr.c                                                             */
/*                                                                         */
/*    OpenType font driver implementation (body).                          */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2005, 2006 by                   */
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
#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_TRUETYPE_IDS_H
#include FT_SERVICE_POSTSCRIPT_CMAPS_H
#include FT_SERVICE_POSTSCRIPT_INFO_H
#include FT_SERVICE_TT_CMAP_H

#include "cffdrivr.h"
#include "cffgload.h"
#include "cffload.h"
#include "cffcmap.h"

#include "cfferrs.h"

#include FT_SERVICE_XFREE86_NAME_H
#include FT_SERVICE_GLYPH_DICT_H

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffdriver


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                          F A C E S                              ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#undef  PAIR_TAG
#define PAIR_TAG( left, right )  ( ( (FT_ULong)left << 16 ) | \
                                     (FT_ULong)right        )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_get_kerning                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to return the kerning vector between two      */
  /*    glyphs of the same face.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to the source face object.                 */
  /*                                                                       */
  /*    left_glyph  :: The index of the left glyph in the kern pair.       */
  /*                                                                       */
  /*    right_glyph :: The index of the right glyph in the kern pair.      */
  /*                                                                       */
  /* <Output>                                                              */
  /*    kerning     :: The kerning vector.  This is in font units for      */
  /*                   scalable formats, and in pixels for fixed-sizes     */
  /*                   formats.                                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only horizontal layouts (left-to-right & right-to-left) are        */
  /*    supported by this function.  Other layouts, or more sophisticated  */
  /*    kernings, are out of scope of this method (the basic driver        */
  /*    interface is meant to be simple).                                  */
  /*                                                                       */
  /*    They can be implemented by format-specific interfaces.             */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_Error )
  cff_get_kerning( FT_Face     ttface,          /* TT_Face */
                   FT_UInt     left_glyph,
                   FT_UInt     right_glyph,
                   FT_Vector*  kerning )
  {
    TT_Face       face = (TT_Face)ttface;
    SFNT_Service  sfnt = (SFNT_Service)face->sfnt;


    kerning->x = 0;
    kerning->y = 0;

    if ( sfnt )
      kerning->x = sfnt->get_kerning( face, left_glyph, right_glyph );

    return CFF_Err_Ok;
  }


#undef PAIR_TAG


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Load_Glyph                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to load a glyph within a given glyph slot.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    slot        :: A handle to the target slot object where the glyph  */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    size        :: A handle to the source face size at which the glyph */
  /*                   must be scaled, loaded, etc.                        */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_??? constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_Error )
  Load_Glyph( FT_GlyphSlot  cffslot,        /* CFF_GlyphSlot */
              FT_Size       cffsize,        /* CFF_Size      */
              FT_UInt       glyph_index,
              FT_Int32      load_flags )
  {
    FT_Error  error;
    CFF_GlyphSlot  slot = (CFF_GlyphSlot)cffslot;
    CFF_Size       size = (CFF_Size)cffsize;


    if ( !slot )
      return CFF_Err_Invalid_Slot_Handle;

    /* check whether we want a scaled outline or bitmap */
    if ( !size )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    if ( load_flags & FT_LOAD_NO_SCALE )
      size = NULL;

    /* reset the size object if necessary */
    if ( size )
    {
      /* these two objects must have the same parent */
      if ( cffsize->face != cffslot->face )
        return CFF_Err_Invalid_Face_Handle;
    }

    /* now load the glyph outline if necessary */
    error = cff_slot_load( slot, size, glyph_index, load_flags );

    /* force drop-out mode to 2 - irrelevant now */
    /* slot->outline.dropout_mode = 2; */

    return error;
  }


 /*
  *  GLYPH DICT SERVICE
  *
  */

  static FT_Error
  cff_get_glyph_name( CFF_Face    face,
                      FT_UInt     glyph_index,
                      FT_Pointer  buffer,
                      FT_UInt     buffer_max )
  {
    CFF_Font            font   = (CFF_Font)face->extra.data;
    FT_Memory           memory = FT_FACE_MEMORY( face );
    FT_String*          gname;
    FT_UShort           sid;
    FT_Service_PsCMaps  psnames;
    FT_Error            error;


    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
    if ( !psnames )
    {
      FT_ERROR(( "cff_get_glyph_name:" ));
      FT_ERROR(( " cannot get glyph name from CFF & CEF fonts\n" ));
      FT_ERROR(( "                   " ));
      FT_ERROR(( " without the `PSNames' module\n" ));
      error = CFF_Err_Unknown_File_Format;
      goto Exit;
    }

    /* first, locate the sid in the charset table */
    sid = font->charset.sids[glyph_index];

    /* now, lookup the name itself */
    gname = cff_index_get_sid_string( &font->string_index, sid, psnames );

    if ( gname && buffer_max > 0 )
    {
      FT_UInt  len = (FT_UInt)ft_strlen( gname );


      if ( len >= buffer_max )
        len = buffer_max - 1;

      FT_MEM_COPY( buffer, gname, len );
      ((FT_Byte*)buffer)[len] = 0;
    }

    FT_FREE( gname );
    error = CFF_Err_Ok;

    Exit:
      return error;
  }


  static FT_UInt
  cff_get_name_index( CFF_Face    face,
                      FT_String*  glyph_name )
  {
    CFF_Font            cff;
    CFF_Charset         charset;
    FT_Service_PsCMaps  psnames;
    FT_Memory           memory = FT_FACE_MEMORY( face );
    FT_String*          name;
    FT_UShort           sid;
    FT_UInt             i;
    FT_Int              result;


    cff     = (CFF_FontRec *)face->extra.data;
    charset = &cff->charset;

    FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
    if ( !psnames )
      return 0;

    for ( i = 0; i < cff->num_glyphs; i++ )
    {
      sid = charset->sids[i];

      if ( sid > 390 )
        name = cff_index_get_name( &cff->string_index, sid - 391 );
      else
        name = (FT_String *)psnames->adobe_std_strings( sid );

      result = ft_strcmp( glyph_name, name );

      if ( sid > 390 )
        FT_FREE( name );

      if ( !result )
        return i;
    }

    return 0;
  }


  static const FT_Service_GlyphDictRec  cff_service_glyph_dict =
  {
    (FT_GlyphDict_GetNameFunc)  cff_get_glyph_name,
    (FT_GlyphDict_NameIndexFunc)cff_get_name_index,
  };


 /*
  *  POSTSCRIPT INFO SERVICE
  *
  */

  static FT_Int
  cff_ps_has_glyph_names( FT_Face  face )
  {
    return ( face->face_flags & FT_FACE_FLAG_GLYPH_NAMES ) > 0;
  }


  static const FT_Service_PsInfoRec  cff_service_ps_info =
  {
    (PS_GetFontInfoFunc)   NULL,        /* unsupported with CFF fonts */
    (PS_HasGlyphNamesFunc) cff_ps_has_glyph_names,
    (PS_GetFontPrivateFunc)NULL         /* unsupported with CFF fonts */
  };


  /*
   * TT CMAP INFO
   *
   * If the charmap is a synthetic Unicode encoding cmap or
   * a Type 1 standard (or expert) encoding cmap, hide TT CMAP INFO
   * service defined in SFNT module.
   *
   * Otherwise call the service function in the sfnt module.
   *
   */
  static FT_Error
  cff_get_cmap_info( FT_CharMap    charmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_CMap   cmap  = FT_CMAP( charmap );
    FT_Error  error = CFF_Err_Ok;


    cmap_info->language = 0;

    if ( cmap->clazz != &cff_cmap_encoding_class_rec &&
         cmap->clazz != &cff_cmap_unicode_class_rec  )
    {
      FT_Face             face    = FT_CMAP_FACE( cmap );
      FT_Library          library = FT_FACE_LIBRARY( face );
      FT_Module           sfnt    = FT_Get_Module( library, "sfnt" );
      FT_Service_TTCMaps  service =
        (FT_Service_TTCMaps)ft_module_get_service( sfnt,
                                                   FT_SERVICE_ID_TT_CMAP );


      if ( service && service->get_cmap_info )
        error = service->get_cmap_info( charmap, cmap_info );
    }

    return error;
  }


  static const FT_Service_TTCMapsRec  cff_service_get_cmap_info =
  {
    (TT_CMap_Info_GetFunc)cff_get_cmap_info
  };


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                D R I V E R  I N T E R F A C E                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  static const FT_ServiceDescRec  cff_services[] =
  {
    { FT_SERVICE_ID_XF86_NAME,       FT_XF86_FORMAT_CFF },
    { FT_SERVICE_ID_POSTSCRIPT_INFO, &cff_service_ps_info },
#ifndef FT_CONFIG_OPTION_NO_GLYPH_NAMES
    { FT_SERVICE_ID_GLYPH_DICT,      &cff_service_glyph_dict },
#endif
    { FT_SERVICE_ID_TT_CMAP,         &cff_service_get_cmap_info },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  cff_get_interface( FT_Module    driver,       /* CFF_Driver */
                     const char*  module_interface )
  {
    FT_Module            sfnt;
    FT_Module_Interface  result;


    result = ft_service_list_lookup( cff_services, module_interface );
    if ( result != NULL )
      return  result;

    /* we pass our request to the `sfnt' module */
    sfnt = FT_Get_Module( driver->library, "sfnt" );

    return sfnt ? sfnt->clazz->get_interface( sfnt, module_interface ) : 0;
  }


  /* The FT_DriverInterface structure is defined in ftdriver.h. */

  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  cff_driver_class =
  {
    /* begin with the FT_Module_Class fields */
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
      FT_MODULE_DRIVER_HAS_HINTER,

      sizeof( CFF_DriverRec ),
      "cff",
      0x10000L,
      0x20000L,

      0,   /* module-specific interface */

      cff_driver_init,
      cff_driver_done,
      cff_get_interface,
    },

    /* now the specific driver fields */
    sizeof( TT_FaceRec ),
    sizeof( CFF_SizeRec ),
    sizeof( CFF_GlyphSlotRec ),

    cff_face_init,
    cff_face_done,
    cff_size_init,
    cff_size_done,
    cff_slot_init,
    cff_slot_done,

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    ft_stub_set_char_sizes,
    ft_stub_set_pixel_sizes,
#endif

    Load_Glyph,

    cff_get_kerning,
    0,                      /* FT_Face_AttachFunc      */
    0,                      /* FT_Face_GetAdvancesFunc */

    cff_size_request,

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
    cff_size_select
#else
    0                       /* FT_Size_SelectFunc      */
#endif
  };


/* END */
