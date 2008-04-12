/***************************************************************************/
/*                                                                         */
/*  t1gload.c                                                              */
/*                                                                         */
/*    Type 1 Glyph Loader (body).                                          */
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
#include "t1gload.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_OUTLINE_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H

#include "t1errors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1gload


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /**********                                                      *********/
  /**********            COMPUTE THE MAXIMUM ADVANCE WIDTH         *********/
  /**********                                                      *********/
  /**********    The following code is in charge of computing      *********/
  /**********    the maximum advance width of the font.  It        *********/
  /**********    quickly processes each glyph charstring to        *********/
  /**********    extract the value from either a `sbw' or `seac'   *********/
  /**********    operator.                                         *********/
  /**********                                                      *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( FT_Error )
  T1_Parse_Glyph_And_Get_Char_String( T1_Decoder  decoder,
                                      FT_UInt     glyph_index,
                                      FT_Data*    char_string )
  {
    T1_Face   face  = (T1_Face)decoder->builder.face;
    T1_Font   type1 = &face->type1;
    FT_Error  error = T1_Err_Ok;


    decoder->font_matrix = type1->font_matrix;
    decoder->font_offset = type1->font_offset;

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* For incremental fonts get the character data using the */
    /* callback function.                                     */
    if ( face->root.internal->incremental_interface )
      error = face->root.internal->incremental_interface->funcs->get_glyph_data(
                face->root.internal->incremental_interface->object,
                glyph_index, char_string );
    else

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

    /* For ordinary fonts get the character data stored in the face record. */
    {
      char_string->pointer = type1->charstrings[glyph_index];
      char_string->length  = (FT_Int)type1->charstrings_len[glyph_index];
    }

    if ( !error )
      error = decoder->funcs.parse_charstrings(
                decoder, (FT_Byte*)char_string->pointer,
                char_string->length );

#ifdef FT_CONFIG_OPTION_INCREMENTAL

    /* Incremental fonts can optionally override the metrics. */
    if ( !error && face->root.internal->incremental_interface                 &&
         face->root.internal->incremental_interface->funcs->get_glyph_metrics )
    {
      FT_Incremental_MetricsRec  metrics;


      metrics.bearing_x = decoder->builder.left_bearing.x;
      metrics.bearing_y = decoder->builder.left_bearing.y;
      metrics.advance   = decoder->builder.advance.x;
      error = face->root.internal->incremental_interface->funcs->get_glyph_metrics(
                face->root.internal->incremental_interface->object,
                glyph_index, FALSE, &metrics );
      decoder->builder.left_bearing.x = metrics.bearing_x;
      decoder->builder.left_bearing.y = metrics.bearing_y;
      decoder->builder.advance.x      = metrics.advance;
      decoder->builder.advance.y      = 0;
    }

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  T1_Parse_Glyph( T1_Decoder  decoder,
                  FT_UInt     glyph_index )
  {
    FT_Data   glyph_data;
    FT_Error  error = T1_Parse_Glyph_And_Get_Char_String(
                        decoder, glyph_index, &glyph_data );


#ifdef FT_CONFIG_OPTION_INCREMENTAL

    if ( !error )
    {
      T1_Face  face = (T1_Face)decoder->builder.face;


      if ( face->root.internal->incremental_interface )
        face->root.internal->incremental_interface->funcs->free_glyph_data(
          face->root.internal->incremental_interface->object,
          &glyph_data );
    }

#endif /* FT_CONFIG_OPTION_INCREMENTAL */

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Compute_Max_Advance( T1_Face  face,
                          FT_Pos*  max_advance )
  {
    FT_Error       error;
    T1_DecoderRec  decoder;
    FT_Int         glyph_index;
    T1_Font        type1 = &face->type1;
    PSAux_Service  psaux = (PSAux_Service)face->psaux;


    *max_advance = 0;

    /* initialize load decoder */
    error = psaux->t1_decoder_funcs->init( &decoder,
                                           (FT_Face)face,
                                           0, /* size       */
                                           0, /* glyph slot */
                                           (FT_Byte**)type1->glyph_names,
                                           face->blend,
                                           0,
                                           FT_RENDER_MODE_NORMAL,
                                           T1_Parse_Glyph );
    if ( error )
      return error;

    decoder.builder.metrics_only = 1;
    decoder.builder.load_points  = 0;

    decoder.num_subrs = type1->num_subrs;
    decoder.subrs     = type1->subrs;
    decoder.subrs_len = type1->subrs_len;

    *max_advance = 0;

    /* for each glyph, parse the glyph charstring and extract */
    /* the advance width                                      */
    for ( glyph_index = 0; glyph_index < type1->num_glyphs; glyph_index++ )
    {
      /* now get load the unscaled outline */
      error = T1_Parse_Glyph( &decoder, glyph_index );
      if ( glyph_index == 0 || decoder.builder.advance.x > *max_advance )
        *max_advance = decoder.builder.advance.x;

      /* ignore the error if one occurred - skip to next glyph */
    }

    return T1_Err_Ok;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Load_Glyph( T1_GlyphSlot  glyph,
                 T1_Size       size,
                 FT_UInt       glyph_index,
                 FT_Int32      load_flags )
  {
    FT_Error                error;
    T1_DecoderRec           decoder;
    T1_Face                 face = (T1_Face)glyph->root.face;
    FT_Bool                 hinting;
    T1_Font                 type1         = &face->type1;
    PSAux_Service           psaux         = (PSAux_Service)face->psaux;
    const T1_Decoder_Funcs  decoder_funcs = psaux->t1_decoder_funcs;

    FT_Matrix               font_matrix;
    FT_Vector               font_offset;
    FT_Data                 glyph_data;
#ifdef FT_CONFIG_OPTION_INCREMENTAL
    FT_Bool                 glyph_data_loaded = 0;
#endif


    if ( load_flags & FT_LOAD_NO_RECURSE )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    glyph->x_scale = size->root.metrics.x_scale;
    glyph->y_scale = size->root.metrics.y_scale;

    glyph->root.outline.n_points   = 0;
    glyph->root.outline.n_contours = 0;

    hinting = FT_BOOL( ( load_flags & FT_LOAD_NO_SCALE   ) == 0 &&
                       ( load_flags & FT_LOAD_NO_HINTING ) == 0 );

    glyph->root.format = FT_GLYPH_FORMAT_OUTLINE;

    error = decoder_funcs->init( &decoder,
                                 (FT_Face)face,
                                 (FT_Size)size,
                                 (FT_GlyphSlot)glyph,
                                 (FT_Byte**)type1->glyph_names,
                                 face->blend,
                                 FT_BOOL( hinting ),
                                 FT_LOAD_TARGET_MODE( load_flags ),
                                 T1_Parse_Glyph );
    if ( error )
      goto Exit;

    decoder.builder.no_recurse = FT_BOOL(
                                   ( load_flags & FT_LOAD_NO_RECURSE ) != 0 );

    decoder.num_subrs = type1->num_subrs;
    decoder.subrs     = type1->subrs;
    decoder.subrs_len = type1->subrs_len;

    /* now load the unscaled outline */
    error = T1_Parse_Glyph_And_Get_Char_String( &decoder, glyph_index,
                                                &glyph_data );
    if ( error )
      goto Exit;
#ifdef FT_CONFIG_OPTION_INCREMENTAL
    glyph_data_loaded = 1;
#endif

    font_matrix = decoder.font_matrix;
    font_offset = decoder.font_offset;

    /* save new glyph tables */
    decoder_funcs->done( &decoder );

    /* now, set the metrics -- this is rather simple, as   */
    /* the left side bearing is the xMin, and the top side */
    /* bearing the yMax                                    */
    if ( !error )
    {
      glyph->root.outline.flags &= FT_OUTLINE_OWNER;
      glyph->root.outline.flags |= FT_OUTLINE_REVERSE_FILL;

      /* for composite glyphs, return only left side bearing and */
      /* advance width                                           */
      if ( load_flags & FT_LOAD_NO_RECURSE )
      {
        FT_Slot_Internal  internal = glyph->root.internal;


        glyph->root.metrics.horiBearingX = decoder.builder.left_bearing.x;
        glyph->root.metrics.horiAdvance  = decoder.builder.advance.x;
        internal->glyph_matrix           = font_matrix;
        internal->glyph_delta            = font_offset;
        internal->glyph_transformed      = 1;
      }
      else
      {
        FT_BBox            cbox;
        FT_Glyph_Metrics*  metrics = &glyph->root.metrics;
        FT_Vector          advance;


        /* copy the _unscaled_ advance width */
        metrics->horiAdvance                    = decoder.builder.advance.x;
        glyph->root.linearHoriAdvance           = decoder.builder.advance.x;
        glyph->root.internal->glyph_transformed = 0;

        /* make up vertical ones */
        metrics->vertAdvance = ( face->type1.font_bbox.yMax -
                                 face->type1.font_bbox.yMin ) >> 16;
        glyph->root.linearVertAdvance = metrics->vertAdvance;

        glyph->root.format = FT_GLYPH_FORMAT_OUTLINE;

        if ( size && size->root.metrics.y_ppem < 24 )
          glyph->root.outline.flags |= FT_OUTLINE_HIGH_PRECISION;

#if 1
        /* apply the font matrix, if any */
        FT_Outline_Transform( &glyph->root.outline, &font_matrix );

        FT_Outline_Translate( &glyph->root.outline,
                              font_offset.x,
                              font_offset.y );

        advance.x = metrics->horiAdvance;
        advance.y = 0;
        FT_Vector_Transform( &advance, &font_matrix );
        metrics->horiAdvance = advance.x + font_offset.x;
        advance.x = 0;
        advance.y = metrics->vertAdvance;
        FT_Vector_Transform( &advance, &font_matrix );
        metrics->vertAdvance = advance.y + font_offset.y;
#endif

        if ( ( load_flags & FT_LOAD_NO_SCALE ) == 0 )
        {
          /* scale the outline and the metrics */
          FT_Int       n;
          FT_Outline*  cur = decoder.builder.base;
          FT_Vector*   vec = cur->points;
          FT_Fixed     x_scale = glyph->x_scale;
          FT_Fixed     y_scale = glyph->y_scale;


          /* First of all, scale the points, if we are not hinting */
          if ( !hinting || ! decoder.builder.hints_funcs )
            for ( n = cur->n_points; n > 0; n--, vec++ )
            {
              vec->x = FT_MulFix( vec->x, x_scale );
              vec->y = FT_MulFix( vec->y, y_scale );
            }

          /* Then scale the metrics */
          metrics->horiAdvance  = FT_MulFix( metrics->horiAdvance,  x_scale );
          metrics->vertAdvance  = FT_MulFix( metrics->vertAdvance,  y_scale );
        }

        /* compute the other metrics */
        FT_Outline_Get_CBox( &glyph->root.outline, &cbox );

        metrics->width  = cbox.xMax - cbox.xMin;
        metrics->height = cbox.yMax - cbox.yMin;

        metrics->horiBearingX = cbox.xMin;
        metrics->horiBearingY = cbox.yMax;

        /* make up vertical ones */
        ft_synthesize_vertical_metrics( metrics,
                                        metrics->vertAdvance );
      }

      /* Set control data to the glyph charstrings.  Note that this is */
      /* _not_ zero-terminated.                                        */
      glyph->root.control_data = (FT_Byte*)glyph_data.pointer;
      glyph->root.control_len  = glyph_data.length;
    }


  Exit:

#ifdef FT_CONFIG_OPTION_INCREMENTAL
    if ( glyph_data_loaded && face->root.internal->incremental_interface )
    {
      face->root.internal->incremental_interface->funcs->free_glyph_data(
        face->root.internal->incremental_interface->object,
        &glyph_data );

      /* Set the control data to null - it is no longer available if   */
      /* loaded incrementally.                                         */
      glyph->root.control_data = 0;
      glyph->root.control_len  = 0;
    }
#endif

    return error;
  }


/* END */
