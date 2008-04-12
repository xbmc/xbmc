/***************************************************************************/
/*                                                                         */
/*  ftsmooth.c                                                             */
/*                                                                         */
/*    Anti-aliasing renderer interface (body).                             */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003, 2004, 2005 by                         */
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
#include FT_INTERNAL_OBJECTS_H
#include FT_OUTLINE_H
#include "ftsmooth.h"
#include "ftgrays.h"

#include "ftsmerrs.h"


  /* initialize renderer -- init its raster */
  static FT_Error
  ft_smooth_init( FT_Renderer  render )
  {
    FT_Library  library = FT_MODULE_LIBRARY( render );


    render->clazz->raster_class->raster_reset( render->raster,
                                               library->raster_pool,
                                               library->raster_pool_size );

    return 0;
  }


  /* sets render-specific mode */
  static FT_Error
  ft_smooth_set_mode( FT_Renderer  render,
                      FT_ULong     mode_tag,
                      FT_Pointer   data )
  {
    /* we simply pass it to the raster */
    return render->clazz->raster_class->raster_set_mode( render->raster,
                                                         mode_tag,
                                                         data );
  }

  /* transform a given glyph image */
  static FT_Error
  ft_smooth_transform( FT_Renderer       render,
                       FT_GlyphSlot      slot,
                       const FT_Matrix*  matrix,
                       const FT_Vector*  delta )
  {
    FT_Error  error = Smooth_Err_Ok;


    if ( slot->format != render->glyph_format )
    {
      error = Smooth_Err_Invalid_Argument;
      goto Exit;
    }

    if ( matrix )
      FT_Outline_Transform( &slot->outline, matrix );

    if ( delta )
      FT_Outline_Translate( &slot->outline, delta->x, delta->y );

  Exit:
    return error;
  }


  /* return the glyph's control box */
  static void
  ft_smooth_get_cbox( FT_Renderer   render,
                      FT_GlyphSlot  slot,
                      FT_BBox*      cbox )
  {
    FT_MEM_ZERO( cbox, sizeof ( *cbox ) );

    if ( slot->format == render->glyph_format )
      FT_Outline_Get_CBox( &slot->outline, cbox );
  }


  /* convert a slot's glyph image into a bitmap */
  static FT_Error
  ft_smooth_render_generic( FT_Renderer       render,
                            FT_GlyphSlot      slot,
                            FT_Render_Mode    mode,
                            const FT_Vector*  origin,
                            FT_Render_Mode    required_mode,
                            FT_Int            hmul,
                            FT_Int            vmul )
  {
    FT_Error     error;
    FT_Outline*  outline = NULL;
    FT_BBox      cbox;
    FT_UInt      width, height, pitch;
    FT_Bitmap*   bitmap;
    FT_Memory    memory;

    FT_Raster_Params  params;


    /* check glyph image format */
    if ( slot->format != render->glyph_format )
    {
      error = Smooth_Err_Invalid_Argument;
      goto Exit;
    }

    /* check mode */
    if ( mode != required_mode )
      return Smooth_Err_Cannot_Render_Glyph;

    outline = &slot->outline;

    /* translate the outline to the new origin if needed */
    if ( origin )
      FT_Outline_Translate( outline, origin->x, origin->y );

    /* compute the control box, and grid fit it */
    FT_Outline_Get_CBox( outline, &cbox );

    cbox.xMin = FT_PIX_FLOOR( cbox.xMin );
    cbox.yMin = FT_PIX_FLOOR( cbox.yMin );
    cbox.xMax = FT_PIX_CEIL( cbox.xMax );
    cbox.yMax = FT_PIX_CEIL( cbox.yMax );

    width  = (FT_UInt)( ( cbox.xMax - cbox.xMin ) >> 6 );
    height = (FT_UInt)( ( cbox.yMax - cbox.yMin ) >> 6 );
    bitmap = &slot->bitmap;
    memory = render->root.memory;

    /* release old bitmap buffer */
    if ( slot->internal->flags & FT_GLYPH_OWN_BITMAP )
    {
      FT_FREE( bitmap->buffer );
      slot->internal->flags &= ~FT_GLYPH_OWN_BITMAP;
    }

    /* allocate new one, depends on pixel format */
    pitch = width;
    if ( hmul )
    {
      width = width * hmul;
      pitch = FT_PAD_CEIL( width, 4 );
    }

    if ( vmul )
      height *= vmul;

    bitmap->pixel_mode = FT_PIXEL_MODE_GRAY;
    bitmap->num_grays  = 256;
    bitmap->width      = width;
    bitmap->rows       = height;
    bitmap->pitch      = pitch;

    if ( FT_ALLOC( bitmap->buffer, (FT_ULong)pitch * height ) )
      goto Exit;

    slot->internal->flags |= FT_GLYPH_OWN_BITMAP;

    /* translate outline to render it into the bitmap */
    FT_Outline_Translate( outline, -cbox.xMin, -cbox.yMin );

    /* set up parameters */
    params.target = bitmap;
    params.source = outline;
    params.flags  = FT_RASTER_FLAG_AA;

    /* implode outline if needed */
    {
      FT_Int      n;
      FT_Vector*  vec;


      if ( hmul )
        for ( vec = outline->points, n = 0; n < outline->n_points; n++, vec++ )
          vec->x *= hmul;

      if ( vmul )
        for ( vec = outline->points, n = 0; n < outline->n_points; n++, vec++ )
          vec->y *= vmul;
    }

    /* render outline into the bitmap */
    error = render->raster_render( render->raster, &params );

    /* deflate outline if needed */
    {
      FT_Int      n;
      FT_Vector*  vec;


      if ( hmul )
        for ( vec = outline->points, n = 0; n < outline->n_points; n++, vec++ )
          vec->x /= hmul;

      if ( vmul )
        for ( vec = outline->points, n = 0; n < outline->n_points; n++, vec++ )
          vec->y /= vmul;
    }

    FT_Outline_Translate( outline, cbox.xMin, cbox.yMin );

    if ( error )
      goto Exit;

    slot->format      = FT_GLYPH_FORMAT_BITMAP;
    slot->bitmap_left = (FT_Int)( cbox.xMin >> 6 );
    slot->bitmap_top  = (FT_Int)( cbox.yMax >> 6 );

  Exit:
    if ( outline && origin )
      FT_Outline_Translate( outline, -origin->x, -origin->y );

    return error;
  }


  /* convert a slot's glyph image into a bitmap */
  static FT_Error
  ft_smooth_render( FT_Renderer       render,
                    FT_GlyphSlot      slot,
                    FT_Render_Mode    mode,
                    const FT_Vector*  origin )
  {
    if ( mode == FT_RENDER_MODE_LIGHT )
      mode = FT_RENDER_MODE_NORMAL;

    return ft_smooth_render_generic( render, slot, mode, origin,
                                     FT_RENDER_MODE_NORMAL,
                                     0, 0 );
  }


  /* convert a slot's glyph image into a horizontal LCD bitmap */
  static FT_Error
  ft_smooth_render_lcd( FT_Renderer       render,
                        FT_GlyphSlot      slot,
                        FT_Render_Mode    mode,
                        const FT_Vector*  origin )
  {
    FT_Error  error;

    error = ft_smooth_render_generic( render, slot, mode, origin,
                                      FT_RENDER_MODE_LCD,
                                      3, 0 );
    if ( !error )
      slot->bitmap.pixel_mode = FT_PIXEL_MODE_LCD;

    return error;
  }


  /* convert a slot's glyph image into a vertical LCD bitmap */
  static FT_Error
  ft_smooth_render_lcd_v( FT_Renderer       render,
                          FT_GlyphSlot      slot,
                          FT_Render_Mode    mode,
                          const FT_Vector*  origin )
  {
    FT_Error  error;

    error = ft_smooth_render_generic( render, slot, mode, origin,
                                      FT_RENDER_MODE_LCD_V,
                                      0, 3 );
    if ( !error )
      slot->bitmap.pixel_mode = FT_PIXEL_MODE_LCD_V;

    return error;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_Renderer_Class  ft_smooth_renderer_class =
  {
    {
      FT_MODULE_RENDERER,
      sizeof( FT_RendererRec ),

      "smooth",
      0x10000L,
      0x20000L,

      0,    /* module specific interface */

      (FT_Module_Constructor)ft_smooth_init,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    FT_GLYPH_FORMAT_OUTLINE,

    (FT_Renderer_RenderFunc)   ft_smooth_render,
    (FT_Renderer_TransformFunc)ft_smooth_transform,
    (FT_Renderer_GetCBoxFunc)  ft_smooth_get_cbox,
    (FT_Renderer_SetModeFunc)  ft_smooth_set_mode,

    (FT_Raster_Funcs*)    &ft_grays_raster
  };


  FT_CALLBACK_TABLE_DEF
  const FT_Renderer_Class  ft_smooth_lcd_renderer_class =
  {
    {
      FT_MODULE_RENDERER,
      sizeof( FT_RendererRec ),

      "smooth-lcd",
      0x10000L,
      0x20000L,

      0,    /* module specific interface */

      (FT_Module_Constructor)ft_smooth_init,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    FT_GLYPH_FORMAT_OUTLINE,

    (FT_Renderer_RenderFunc)   ft_smooth_render_lcd,
    (FT_Renderer_TransformFunc)ft_smooth_transform,
    (FT_Renderer_GetCBoxFunc)  ft_smooth_get_cbox,
    (FT_Renderer_SetModeFunc)  ft_smooth_set_mode,

    (FT_Raster_Funcs*)    &ft_grays_raster
  };



  FT_CALLBACK_TABLE_DEF
  const FT_Renderer_Class  ft_smooth_lcdv_renderer_class =
  {
    {
      FT_MODULE_RENDERER,
      sizeof( FT_RendererRec ),

      "smooth-lcdv",
      0x10000L,
      0x20000L,

      0,    /* module specific interface */

      (FT_Module_Constructor)ft_smooth_init,
      (FT_Module_Destructor) 0,
      (FT_Module_Requester)  0
    },

    FT_GLYPH_FORMAT_OUTLINE,

    (FT_Renderer_RenderFunc)   ft_smooth_render_lcd_v,
    (FT_Renderer_TransformFunc)ft_smooth_transform,
    (FT_Renderer_GetCBoxFunc)  ft_smooth_get_cbox,
    (FT_Renderer_SetModeFunc)  ft_smooth_set_mode,

    (FT_Raster_Funcs*)    &ft_grays_raster
  };


/* END */
