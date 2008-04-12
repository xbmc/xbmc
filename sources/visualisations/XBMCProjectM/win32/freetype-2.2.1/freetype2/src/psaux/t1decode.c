/***************************************************************************/
/*                                                                         */
/*  t1decode.c                                                             */
/*                                                                         */
/*    PostScript Type 1 decoding routines (body).                          */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include FT_OUTLINE_H

#include "t1decode.h"
#include "psobjs.h"

#include "psauxerr.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1decode


  typedef enum  T1_Operator_
  {
    op_none = 0,
    op_endchar,
    op_hsbw,
    op_seac,
    op_sbw,
    op_closepath,
    op_hlineto,
    op_hmoveto,
    op_hvcurveto,
    op_rlineto,
    op_rmoveto,
    op_rrcurveto,
    op_vhcurveto,
    op_vlineto,
    op_vmoveto,
    op_dotsection,
    op_hstem,
    op_hstem3,
    op_vstem,
    op_vstem3,
    op_div,
    op_callothersubr,
    op_callsubr,
    op_pop,
    op_return,
    op_setcurrentpoint,

    op_max    /* never remove this one */

  } T1_Operator;


  static
  const FT_Int  t1_args_count[op_max] =
  {
    0, /* none */
    0, /* endchar */
    2, /* hsbw */
    5, /* seac */
    4, /* sbw */
    0, /* closepath */
    1, /* hlineto */
    1, /* hmoveto */
    4, /* hvcurveto */
    2, /* rlineto */
    2, /* rmoveto */
    6, /* rrcurveto */
    4, /* vhcurveto */
    1, /* vlineto */
    1, /* vmoveto */
    0, /* dotsection */
    2, /* hstem */
    6, /* hstem3 */
    2, /* vstem */
    6, /* vstem3 */
    2, /* div */
   -1, /* callothersubr */
    1, /* callsubr */
    0, /* pop */
    0, /* return */
    2  /* setcurrentpoint */
  };


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_lookup_glyph_by_stdcharcode                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Looks up a given glyph by its StandardEncoding charcode.  Used to  */
  /*    implement the SEAC Type 1 operator.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: The current face object.                               */
  /*                                                                       */
  /*    charcode :: The character code to look for.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A glyph index in the font face.  Returns -1 if the corresponding   */
  /*    glyph wasn't found.                                                */
  /*                                                                       */
  static FT_Int
  t1_lookup_glyph_by_stdcharcode( T1_Decoder  decoder,
                                  FT_Int      charcode )
  {
    FT_UInt             n;
    const FT_String*    glyph_name;
    FT_Service_PsCMaps  psnames = decoder->psnames;


    /* check range of standard char code */
    if ( charcode < 0 || charcode > 255 )
      return -1;

    glyph_name = psnames->adobe_std_strings(
                   psnames->adobe_std_encoding[charcode]);

    for ( n = 0; n < decoder->num_glyphs; n++ )
    {
      FT_String*  name = (FT_String*)decoder->glyph_names[n];


      if ( name && name[0] == glyph_name[0]  &&
           ft_strcmp( name, glyph_name ) == 0 )
        return n;
    }

    return -1;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1operator_seac                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Implements the `seac' Type 1 operator for a Type 1 decoder.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    decoder :: The current CID decoder.                                */
  /*                                                                       */
  /*    asb     :: The accent's side bearing.                              */
  /*                                                                       */
  /*    adx     :: The horizontal offset of the accent.                    */
  /*                                                                       */
  /*    ady     :: The vertical offset of the accent.                      */
  /*                                                                       */
  /*    bchar   :: The base character's StandardEncoding charcode.         */
  /*                                                                       */
  /*    achar   :: The accent character's StandardEncoding charcode.       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static FT_Error
  t1operator_seac( T1_Decoder  decoder,
                   FT_Pos      asb,
                   FT_Pos      adx,
                   FT_Pos      ady,
                   FT_Int      bchar,
                   FT_Int      achar )
  {
    FT_Error     error;
    FT_Int       bchar_index, achar_index;
#if 0
    FT_Int       n_base_points;
    FT_Outline*  base = decoder->builder.base;
#endif
    FT_Vector    left_bearing, advance;


    /* seac weirdness */
    adx += decoder->builder.left_bearing.x;

    /* `glyph_names' is set to 0 for CID fonts which do not */
    /* include an encoding.  How can we deal with these?    */
    if ( decoder->glyph_names == 0 )
    {
      FT_ERROR(( "t1operator_seac:" ));
      FT_ERROR(( " glyph names table not available in this font!\n" ));
      return PSaux_Err_Syntax_Error;
    }

    bchar_index = t1_lookup_glyph_by_stdcharcode( decoder, bchar );
    achar_index = t1_lookup_glyph_by_stdcharcode( decoder, achar );

    if ( bchar_index < 0 || achar_index < 0 )
    {
      FT_ERROR(( "t1operator_seac:" ));
      FT_ERROR(( " invalid seac character code arguments\n" ));
      return PSaux_Err_Syntax_Error;
    }

    /* if we are trying to load a composite glyph, do not load the */
    /* accent character and return the array of subglyphs.         */
    if ( decoder->builder.no_recurse )
    {
      FT_GlyphSlot    glyph  = (FT_GlyphSlot)decoder->builder.glyph;
      FT_GlyphLoader  loader = glyph->internal->loader;
      FT_SubGlyph     subg;


      /* reallocate subglyph array if necessary */
      error = FT_GlyphLoader_CheckSubGlyphs( loader, 2 );
      if ( error )
        goto Exit;

      subg = loader->current.subglyphs;

      /* subglyph 0 = base character */
      subg->index = bchar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES |
                    FT_SUBGLYPH_FLAG_USE_MY_METRICS;
      subg->arg1  = 0;
      subg->arg2  = 0;
      subg++;

      /* subglyph 1 = accent character */
      subg->index = achar_index;
      subg->flags = FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES;
      subg->arg1  = (FT_Int)( adx - asb );
      subg->arg2  = (FT_Int)ady;

      /* set up remaining glyph fields */
      glyph->num_subglyphs = 2;
      glyph->subglyphs     = loader->base.subglyphs;
      glyph->format        = FT_GLYPH_FORMAT_COMPOSITE;

      loader->current.num_subglyphs = 2;
      goto Exit;
    }

    /* First load `bchar' in builder */
    /* now load the unscaled outline */

    FT_GlyphLoader_Prepare( decoder->builder.loader );  /* prepare loader */

    error = t1_decoder_parse_glyph( decoder, bchar_index );
    if ( error )
      goto Exit;

    /* save the left bearing and width of the base character */
    /* as they will be erased by the next load.              */

    left_bearing = decoder->builder.left_bearing;
    advance      = decoder->builder.advance;

    decoder->builder.left_bearing.x = 0;
    decoder->builder.left_bearing.y = 0;

    decoder->builder.pos_x = adx - asb;
    decoder->builder.pos_y = ady;

    /* Now load `achar' on top of */
    /* the base outline           */
    error = t1_decoder_parse_glyph( decoder, achar_index );
    if ( error )
      goto Exit;

    /* restore the left side bearing and   */
    /* advance width of the base character */

    decoder->builder.left_bearing = left_bearing;
    decoder->builder.advance      = advance;

    decoder->builder.pos_x = 0;
    decoder->builder.pos_y = 0;

  Exit:
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    t1_decoder_parse_charstrings                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a given Type 1 charstrings program.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    decoder         :: The current Type 1 decoder.                     */
  /*                                                                       */
  /*    charstring_base :: The base address of the charstring stream.      */
  /*                                                                       */
  /*    charstring_len  :: The length in bytes of the charstring stream.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_parse_charstrings( T1_Decoder  decoder,
                                FT_Byte*    charstring_base,
                                FT_UInt     charstring_len )
  {
    FT_Error         error;
    T1_Decoder_Zone  zone;
    FT_Byte*         ip;
    FT_Byte*         limit;
    T1_Builder       builder = &decoder->builder;
    FT_Pos           x, y, orig_x, orig_y;

    T1_Hints_Funcs   hinter;


    /* we don't want to touch the source code -- use macro trick */
#define start_point    t1_builder_start_point
#define check_points   t1_builder_check_points
#define add_point      t1_builder_add_point
#define add_point1     t1_builder_add_point1
#define add_contour    t1_builder_add_contour
#define close_contour  t1_builder_close_contour

    /* First of all, initialize the decoder */
    decoder->top  = decoder->stack;
    decoder->zone = decoder->zones;
    zone          = decoder->zones;

    builder->parse_state = T1_Parse_Start;

    hinter = (T1_Hints_Funcs)builder->hints_funcs;

    zone->base           = charstring_base;
    limit = zone->limit  = charstring_base + charstring_len;
    ip    = zone->cursor = zone->base;

    error = PSaux_Err_Ok;

    x = orig_x = builder->pos_x;
    y = orig_y = builder->pos_y;

    /* begin hints recording session, if any */
    if ( hinter )
      hinter->open( hinter->hints );

    /* now, execute loop */
    while ( ip < limit )
    {
      FT_Long*     top   = decoder->top;
      T1_Operator  op    = op_none;
      FT_Long      value = 0;


      /*********************************************************************/
      /*                                                                   */
      /* Decode operator or operand                                        */
      /*                                                                   */
      /*                                                                   */

      /* first of all, decompress operator or value */
      switch ( *ip++ )
      {
      case 1:
        op = op_hstem;
        break;

      case 3:
        op = op_vstem;
        break;
      case 4:
        op = op_vmoveto;
        break;
      case 5:
        op = op_rlineto;
        break;
      case 6:
        op = op_hlineto;
        break;
      case 7:
        op = op_vlineto;
        break;
      case 8:
        op = op_rrcurveto;
        break;
      case 9:
        op = op_closepath;
        break;
      case 10:
        op = op_callsubr;
        break;
      case 11:
        op = op_return;
        break;

      case 13:
        op = op_hsbw;
        break;
      case 14:
        op = op_endchar;
        break;

      case 15:          /* undocumented, obsolete operator */
        op = op_none;
        break;

      case 21:
        op = op_rmoveto;
        break;
      case 22:
        op = op_hmoveto;
        break;

      case 30:
        op = op_vhcurveto;
        break;
      case 31:
        op = op_hvcurveto;
        break;

      case 12:
        if ( ip > limit )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid escape (12+EOF)\n" ));
          goto Syntax_Error;
        }

        switch ( *ip++ )
        {
        case 0:
          op = op_dotsection;
          break;
        case 1:
          op = op_vstem3;
          break;
        case 2:
          op = op_hstem3;
          break;
        case 6:
          op = op_seac;
          break;
        case 7:
          op = op_sbw;
          break;
        case 12:
          op = op_div;
          break;
        case 16:
          op = op_callothersubr;
          break;
        case 17:
          op = op_pop;
          break;
        case 33:
          op = op_setcurrentpoint;
          break;

        default:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid escape (12+%d)\n",
                     ip[-1] ));
          goto Syntax_Error;
        }
        break;

      case 255:    /* four bytes integer */
        if ( ip + 4 > limit )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "unexpected EOF in integer\n" ));
          goto Syntax_Error;
        }

        value = (FT_Int32)( ((FT_Long)ip[0] << 24) |
                            ((FT_Long)ip[1] << 16) |
                            ((FT_Long)ip[2] << 8 ) |
                                      ip[3] );
        ip += 4;
        break;

      default:
        if ( ip[-1] >= 32 )
        {
          if ( ip[-1] < 247 )
            value = (FT_Long)ip[-1] - 139;
          else
          {
            if ( ++ip > limit )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "unexpected EOF in integer\n" ));
              goto Syntax_Error;
            }

            if ( ip[-2] < 251 )
              value =  ( ( (FT_Long)ip[-2] - 247 ) << 8 ) + ip[-1] + 108;
            else
              value = -( ( ( (FT_Long)ip[-2] - 251 ) << 8 ) + ip[-1] + 108 );
          }
        }
        else
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid byte (%d)\n", ip[-1] ));
          goto Syntax_Error;
        }
      }

      /*********************************************************************/
      /*                                                                   */
      /*  Push value on stack, or process operator                         */
      /*                                                                   */
      /*                                                                   */
      if ( op == op_none )
      {
        if ( top - decoder->stack >= T1_MAX_CHARSTRINGS_OPERANDS )
        {
          FT_ERROR(( "t1_decoder_parse_charstrings: stack overflow!\n" ));
          goto Syntax_Error;
        }

        FT_TRACE4(( " %ld", value ));

        *top++       = value;
        decoder->top = top;
      }
      else if ( op == op_callothersubr )  /* callothersubr */
      {
        FT_TRACE4(( " callothersubr" ));

        if ( top - decoder->stack < 2 )
          goto Stack_Underflow;

        top -= 2;
        switch ( (FT_Int)top[1] )
        {
        case 1:                     /* start flex feature */
          if ( top[0] != 0 )
            goto Unexpected_OtherSubr;

          decoder->flex_state        = 1;
          decoder->num_flex_vectors  = 0;
          if ( start_point( builder, x, y ) ||
               check_points( builder, 6 )   )
            goto Fail;
          break;

        case 2:                     /* add flex vectors */
          {
            FT_Int  idx;


            if ( top[0] != 0 )
              goto Unexpected_OtherSubr;

            /* note that we should not add a point for index 0; */
            /* this will move our current position to the flex  */
            /* point without adding any point to the outline    */
            idx = decoder->num_flex_vectors++;
            if ( idx > 0 && idx < 7 )
              add_point( builder,
                         x,
                         y,
                         (FT_Byte)( idx == 3 || idx == 6 ) );
          }
          break;

        case 0:                     /* end flex feature */
          if ( top[0] != 3 )
            goto Unexpected_OtherSubr;

          if ( decoder->flex_state       == 0 ||
               decoder->num_flex_vectors != 7 )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: "
                       "unexpected flex end\n" ));
            goto Syntax_Error;
          }

          /* now consume the remaining `pop pop setcurpoint' */
          if ( ip + 6 > limit ||
               ip[0] != 12 || ip[1] != 17 || /* pop */
               ip[2] != 12 || ip[3] != 17 || /* pop */
               ip[4] != 12 || ip[5] != 33 )  /* setcurpoint */
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: "
                       "invalid flex charstring\n" ));
            goto Syntax_Error;
          }

          ip += 6;
          decoder->flex_state = 0;
          break;

        case 3:                     /* change hints */
          if ( top[0] != 1 )
            goto Unexpected_OtherSubr;

          /* eat the following `pop' */
          if ( ip + 2 > limit )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: "
                       "invalid escape (12+%d)\n", ip[-1] ));
            goto Syntax_Error;
          }

          if ( ip[0] != 12 || ip[1] != 17 )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: " ));
            FT_ERROR(( "`pop' expected, found (%d %d)\n", ip[0], ip[1] ));
            goto Syntax_Error;
          }
          ip += 2;

          if ( hinter )
            hinter->reset( hinter->hints, builder->current->n_points );

          break;

        case 12:
        case 13:
          /* counter control hints, clear stack */
          top = decoder->stack;
          break;

        case 14:
        case 15:
        case 16:
        case 17:
        case 18:                    /* multiple masters */
          {
            PS_Blend  blend = decoder->blend;
            FT_UInt   num_points, nn, mm;
            FT_Long*  delta;
            FT_Long*  values;


            if ( !blend )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "unexpected multiple masters operator!\n" ));
              goto Syntax_Error;
            }

            num_points = (FT_UInt)top[1] - 13 + ( top[1] == 18 );
            if ( top[0] != (FT_Int)( num_points * blend->num_designs ) )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: " ));
              FT_ERROR(( "incorrect number of mm arguments\n" ));
              goto Syntax_Error;
            }

            top -= blend->num_designs * num_points;
            if ( top < decoder->stack )
              goto Stack_Underflow;

            /* we want to compute:                                   */
            /*                                                       */
            /*  a0*w0 + a1*w1 + ... + ak*wk                          */
            /*                                                       */
            /* but we only have the a0, a1-a0, a2-a0, .. ak-a0       */
            /* however, given that w0 + w1 + ... + wk == 1, we can   */
            /* rewrite it easily as:                                 */
            /*                                                       */
            /*  a0 + (a1-a0)*w1 + (a2-a0)*w2 + .. + (ak-a0)*wk       */
            /*                                                       */
            /* where k == num_designs-1                              */
            /*                                                       */
            /* I guess that's why it's written in this `compact'     */
            /* form.                                                 */
            /*                                                       */
            delta  = top + num_points;
            values = top;
            for ( nn = 0; nn < num_points; nn++ )
            {
              FT_Long  tmp = values[0];


              for ( mm = 1; mm < blend->num_designs; mm++ )
                tmp += FT_MulFix( *delta++, blend->weight_vector[mm] );

              *values++ = tmp;
            }
            /* note that `top' will be incremented later by calls to `pop' */
            break;
          }

        default:
        Unexpected_OtherSubr:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "invalid othersubr [%d %d]!\n", top[0], top[1] ));
          goto Syntax_Error;
        }
        decoder->top = top;
      }
      else  /* general operator */
      {
        FT_Int  num_args = t1_args_count[op];


        if ( top - decoder->stack < num_args )
          goto Stack_Underflow;

        top -= num_args;

        switch ( op )
        {
        case op_endchar:
          FT_TRACE4(( " endchar" ));

          close_contour( builder );

          /* close hints recording session */
          if ( hinter )
          {
            if (hinter->close( hinter->hints, builder->current->n_points ))
              goto Syntax_Error;

            /* apply hints to the loaded glyph outline now */
            hinter->apply( hinter->hints,
                           builder->current,
                           (PSH_Globals) builder->hints_globals,
                           decoder->hint_mode );
          }

          /* add current outline to the glyph slot */
          FT_GlyphLoader_Add( builder->loader );

          /* return now! */
          FT_TRACE4(( "\n\n" ));
          return PSaux_Err_Ok;

        case op_hsbw:
          FT_TRACE4(( " hsbw" ));

          builder->parse_state = T1_Parse_Have_Width;

          builder->left_bearing.x += top[0];
          builder->advance.x       = top[1];
          builder->advance.y       = 0;

          orig_x = builder->last.x = x = builder->pos_x + top[0];
          orig_y = builder->last.y = y = builder->pos_y;

          FT_UNUSED( orig_y );

          /* the `metrics_only' indicates that we only want to compute */
          /* the glyph's metrics (lsb + advance width), not load the   */
          /* rest of it; so exit immediately                           */
          if ( builder->metrics_only )
            return PSaux_Err_Ok;

          break;

        case op_seac:
          /* return immediately after the processing */
          return t1operator_seac( decoder, top[0], top[1], top[2],
                                           (FT_Int)top[3], (FT_Int)top[4] );

        case op_sbw:
          FT_TRACE4(( " sbw" ));

          builder->parse_state = T1_Parse_Have_Width;

          builder->left_bearing.x += top[0];
          builder->left_bearing.y += top[1];
          builder->advance.x       = top[2];
          builder->advance.y       = top[3];

          builder->last.x = x = builder->pos_x + top[0];
          builder->last.y = y = builder->pos_y + top[1];

          /* the `metrics_only' indicates that we only want to compute */
          /* the glyph's metrics (lsb + advance width), not load the   */
          /* rest of it; so exit immediately                           */
          if ( builder->metrics_only )
            return PSaux_Err_Ok;

          break;

        case op_closepath:
          FT_TRACE4(( " closepath" ));

          close_contour( builder );
          if ( !( builder->parse_state == T1_Parse_Have_Path   ||
                  builder->parse_state == T1_Parse_Have_Moveto ) )
            goto Syntax_Error;
          builder->parse_state = T1_Parse_Have_Width;
          break;

        case op_hlineto:
          FT_TRACE4(( " hlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          x += top[0];
          goto Add_Line;

        case op_hmoveto:
          FT_TRACE4(( " hmoveto" ));

          x += top[0];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_hvcurveto:
          FT_TRACE4(( " hvcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          x += top[0];
          add_point( builder, x, y, 0 );
          x += top[1];
          y += top[2];
          add_point( builder, x, y, 0 );
          y += top[3];
          add_point( builder, x, y, 1 );
          break;

        case op_rlineto:
          FT_TRACE4(( " rlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          x += top[0];
          y += top[1];

        Add_Line:
          if ( add_point1( builder, x, y ) )
            goto Fail;
          break;

        case op_rmoveto:
          FT_TRACE4(( " rmoveto" ));

          x += top[0];
          y += top[1];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_rrcurveto:
          FT_TRACE4(( " rcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          x += top[0];
          y += top[1];
          add_point( builder, x, y, 0 );

          x += top[2];
          y += top[3];
          add_point( builder, x, y, 0 );

          x += top[4];
          y += top[5];
          add_point( builder, x, y, 1 );
          break;

        case op_vhcurveto:
          FT_TRACE4(( " vhcurveto" ));

          if ( start_point( builder, x, y ) ||
               check_points( builder, 3 )   )
            goto Fail;

          y += top[0];
          add_point( builder, x, y, 0 );
          x += top[1];
          y += top[2];
          add_point( builder, x, y, 0 );
          x += top[3];
          add_point( builder, x, y, 1 );
          break;

        case op_vlineto:
          FT_TRACE4(( " vlineto" ));

          if ( start_point( builder, x, y ) )
            goto Fail;

          y += top[0];
          goto Add_Line;

        case op_vmoveto:
          FT_TRACE4(( " vmoveto" ));

          y += top[0];
          if ( !decoder->flex_state )
          {
            if ( builder->parse_state == T1_Parse_Start )
              goto Syntax_Error;
            builder->parse_state = T1_Parse_Have_Moveto;
          }
          break;

        case op_div:
          FT_TRACE4(( " div" ));

          if ( top[1] )
          {
            *top = top[0] / top[1];
            ++top;
          }
          else
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: division by 0\n" ));
            goto Syntax_Error;
          }
          break;

        case op_callsubr:
          {
            FT_Int  idx;


            FT_TRACE4(( " callsubr" ));

            idx = (FT_Int)top[0];
            if ( idx < 0 || idx >= (FT_Int)decoder->num_subrs )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "invalid subrs index\n" ));
              goto Syntax_Error;
            }

            if ( zone - decoder->zones >= T1_MAX_SUBRS_CALLS )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "too many nested subrs\n" ));
              goto Syntax_Error;
            }

            zone->cursor = ip;  /* save current instruction pointer */

            zone++;

            /* The Type 1 driver stores subroutines without the seed bytes. */
            /* The CID driver stores subroutines with seed bytes.  This     */
            /* case is taken care of when decoder->subrs_len == 0.          */
            zone->base = decoder->subrs[idx];

            if ( decoder->subrs_len )
              zone->limit = zone->base + decoder->subrs_len[idx];
            else
            {
              /* We are using subroutines from a CID font.  We must adjust */
              /* for the seed bytes.                                       */
              zone->base  += ( decoder->lenIV >= 0 ? decoder->lenIV : 0 );
              zone->limit  = decoder->subrs[idx + 1];
            }

            zone->cursor = zone->base;

            if ( !zone->base )
            {
              FT_ERROR(( "t1_decoder_parse_charstrings: "
                         "invoking empty subrs!\n" ));
              goto Syntax_Error;
            }

            decoder->zone = zone;
            ip            = zone->base;
            limit         = zone->limit;
            break;
          }

        case op_pop:
          FT_TRACE4(( " pop" ));

          /* theoretically, the arguments are already on the stack */
          top++;
          break;

        case op_return:
          FT_TRACE4(( " return" ));

          if ( zone <= decoder->zones )
          {
            FT_ERROR(( "t1_decoder_parse_charstrings: unexpected return\n" ));
            goto Syntax_Error;
          }

          zone--;
          ip            = zone->cursor;
          limit         = zone->limit;
          decoder->zone = zone;
          break;

        case op_dotsection:
          FT_TRACE4(( " dotsection" ));

          break;

        case op_hstem:
          FT_TRACE4(( " hstem" ));

          /* record horizontal hint */
          if ( hinter )
          {
            /* top[0] += builder->left_bearing.y; */
            hinter->stem( hinter->hints, 1, top );
          }

          break;

        case op_hstem3:
          FT_TRACE4(( " hstem3" ));

          /* record horizontal counter-controlled hints */
          if ( hinter )
            hinter->stem3( hinter->hints, 1, top );

          break;

        case op_vstem:
          FT_TRACE4(( " vstem" ));

          /* record vertical  hint */
          if ( hinter )
          {
            top[0] += orig_x;
            hinter->stem( hinter->hints, 0, top );
          }

          break;

        case op_vstem3:
          FT_TRACE4(( " vstem3" ));

          /* record vertical counter-controlled hints */
          if ( hinter )
          {
            FT_Pos  dx = orig_x;


            top[0] += dx;
            top[2] += dx;
            top[4] += dx;
            hinter->stem3( hinter->hints, 0, top );
          }
          break;

        case op_setcurrentpoint:
          FT_TRACE4(( " setcurrentpoint" ));

          FT_ERROR(( "t1_decoder_parse_charstrings: " ));
          FT_ERROR(( "unexpected `setcurrentpoint'\n" ));
          goto Syntax_Error;

        default:
          FT_ERROR(( "t1_decoder_parse_charstrings: "
                     "unhandled opcode %d\n", op ));
          goto Syntax_Error;
        }

        decoder->top = top;

      } /* general operator processing */

    } /* while ip < limit */

    FT_TRACE4(( "..end..\n\n" ));

  Fail:
    return error;

  Syntax_Error:
    return PSaux_Err_Syntax_Error;

  Stack_Underflow:
    return PSaux_Err_Stack_Underflow;
  }


  /* parse a single Type 1 glyph */
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_parse_glyph( T1_Decoder  decoder,
                          FT_UInt     glyph )
  {
    return decoder->parse_callback( decoder, glyph );
  }


  /* initialize T1 decoder */
  FT_LOCAL_DEF( FT_Error )
  t1_decoder_init( T1_Decoder           decoder,
                   FT_Face              face,
                   FT_Size              size,
                   FT_GlyphSlot         slot,
                   FT_Byte**            glyph_names,
                   PS_Blend             blend,
                   FT_Bool              hinting,
                   FT_Render_Mode       hint_mode,
                   T1_Decoder_Callback  parse_callback )
  {
    FT_MEM_ZERO( decoder, sizeof ( *decoder ) );

    /* retrieve PSNames interface from list of current modules */
    {
      FT_Service_PsCMaps  psnames = 0;


      FT_FACE_FIND_GLOBAL_SERVICE( face, psnames, POSTSCRIPT_CMAPS );
      if ( !psnames )
      {
        FT_ERROR(( "t1_decoder_init: " ));
        FT_ERROR(( "the `psnames' module is not available\n" ));
        return PSaux_Err_Unimplemented_Feature;
      }

      decoder->psnames = psnames;
    }

    t1_builder_init( &decoder->builder, face, size, slot, hinting );

    decoder->num_glyphs     = (FT_UInt)face->num_glyphs;
    decoder->glyph_names    = glyph_names;
    decoder->hint_mode      = hint_mode;
    decoder->blend          = blend;
    decoder->parse_callback = parse_callback;

    decoder->funcs          = t1_decoder_funcs;

    return 0;
  }


  /* finalize T1 decoder */
  FT_LOCAL_DEF( void )
  t1_decoder_done( T1_Decoder  decoder )
  {
    t1_builder_done( &decoder->builder );
  }


/* END */
