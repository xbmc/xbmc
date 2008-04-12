/***************************************************************************/
/*                                                                         */
/*  pshalgo.c                                                              */
/*                                                                         */
/*    PostScript hinting algorithm (body).                                 */
/*                                                                         */
/*  Copyright 2001, 2002, 2003, 2004, 2005 by                              */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include "pshalgo.h"

#include "pshnterr.h"


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pshalgo2


#ifdef DEBUG_HINTER
  PSH_Hint_Table  ps_debug_hint_table = 0;
  PSH_HintFunc    ps_debug_hint_func  = 0;
  PSH_Glyph       ps_debug_glyph      = 0;
#endif


#define  COMPUTE_INFLEXS  /* compute inflection points to optimize `S' */
                          /* and similar glyphs                        */
#define  STRONGER         /* slightly increase the contrast of smooth  */
                          /* hinting                                   */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                  BASIC HINTS RECORDINGS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* return true if two stem hints overlap */
  static FT_Int
  psh_hint_overlap( PSH_Hint  hint1,
                    PSH_Hint  hint2 )
  {
    return hint1->org_pos + hint1->org_len >= hint2->org_pos &&
           hint2->org_pos + hint2->org_len >= hint1->org_pos;
  }


  /* destroy hints table */
  static void
  psh_hint_table_done( PSH_Hint_Table  table,
                       FT_Memory       memory )
  {
    FT_FREE( table->zones );
    table->num_zones = 0;
    table->zone      = 0;

    FT_FREE( table->sort );
    FT_FREE( table->hints );
    table->num_hints   = 0;
    table->max_hints   = 0;
    table->sort_global = 0;
  }


  /* deactivate all hints in a table */
  static void
  psh_hint_table_deactivate( PSH_Hint_Table  table )
  {
    FT_UInt   count = table->max_hints;
    PSH_Hint  hint  = table->hints;


    for ( ; count > 0; count--, hint++ )
    {
      psh_hint_deactivate( hint );
      hint->order = -1;
    }
  }


  /* internal function to record a new hint */
  static void
  psh_hint_table_record( PSH_Hint_Table  table,
                         FT_UInt         idx )
  {
    PSH_Hint  hint = table->hints + idx;


    if ( idx >= table->max_hints )
    {
      FT_ERROR(( "psh_hint_table_record: invalid hint index %d\n", idx ));
      return;
    }

    /* ignore active hints */
    if ( psh_hint_is_active( hint ) )
      return;

    psh_hint_activate( hint );

    /* now scan the current active hint set to check */
    /* whether `hint' overlaps with another hint     */
    {
      PSH_Hint*  sorted = table->sort_global;
      FT_UInt    count  = table->num_hints;
      PSH_Hint   hint2;


      hint->parent = 0;
      for ( ; count > 0; count--, sorted++ )
      {
        hint2 = sorted[0];

        if ( psh_hint_overlap( hint, hint2 ) )
        {
          hint->parent = hint2;
          break;
        }
      }
    }

    if ( table->num_hints < table->max_hints )
      table->sort_global[table->num_hints++] = hint;
    else
      FT_ERROR(( "psh_hint_table_record: too many sorted hints!  BUG!\n" ));
  }


  static void
  psh_hint_table_record_mask( PSH_Hint_Table  table,
                              PS_Mask         hint_mask )
  {
    FT_Int    mask = 0, val = 0;
    FT_Byte*  cursor = hint_mask->bytes;
    FT_UInt   idx, limit;


    limit = hint_mask->num_bits;

    for ( idx = 0; idx < limit; idx++ )
    {
      if ( mask == 0 )
      {
        val  = *cursor++;
        mask = 0x80;
      }

      if ( val & mask )
        psh_hint_table_record( table, idx );

      mask >>= 1;
    }
  }


  /* create hints table */
  static FT_Error
  psh_hint_table_init( PSH_Hint_Table  table,
                       PS_Hint_Table   hints,
                       PS_Mask_Table   hint_masks,
                       PS_Mask_Table   counter_masks,
                       FT_Memory       memory )
  {
    FT_UInt   count;
    FT_Error  error;

    FT_UNUSED( counter_masks );


    count = hints->num_hints;

    /* allocate our tables */
    if ( FT_NEW_ARRAY( table->sort,  2 * count     ) ||
         FT_NEW_ARRAY( table->hints,     count     ) ||
         FT_NEW_ARRAY( table->zones, 2 * count + 1 ) )
      goto Exit;

    table->max_hints   = count;
    table->sort_global = table->sort + count;
    table->num_hints   = 0;
    table->num_zones   = 0;
    table->zone        = 0;

    /* initialize the `table->hints' array */
    {
      PSH_Hint  write = table->hints;
      PS_Hint   read  = hints->hints;


      for ( ; count > 0; count--, write++, read++ )
      {
        write->org_pos = read->pos;
        write->org_len = read->len;
        write->flags   = read->flags;
      }
    }

    /* we now need to determine the initial `parent' stems; first  */
    /* activate the hints that are given by the initial hint masks */
    if ( hint_masks )
    {
      PS_Mask  mask = hint_masks->masks;


      count             = hint_masks->num_masks;
      table->hint_masks = hint_masks;

      for ( ; count > 0; count--, mask++ )
        psh_hint_table_record_mask( table, mask );
    }

    /* finally, do a linear parse in case some hints were left alone */
    if ( table->num_hints != table->max_hints )
    {
      FT_UInt  idx;


      FT_ERROR(( "psh_hint_table_init: missing/incorrect hint masks!\n" ));

      count = table->max_hints;
      for ( idx = 0; idx < count; idx++ )
        psh_hint_table_record( table, idx );
    }

  Exit:
    return error;
  }


  static void
  psh_hint_table_activate_mask( PSH_Hint_Table  table,
                                PS_Mask         hint_mask )
  {
    FT_Int    mask = 0, val = 0;
    FT_Byte*  cursor = hint_mask->bytes;
    FT_UInt   idx, limit, count;


    limit = hint_mask->num_bits;
    count = 0;

    psh_hint_table_deactivate( table );

    for ( idx = 0; idx < limit; idx++ )
    {
      if ( mask == 0 )
      {
        val  = *cursor++;
        mask = 0x80;
      }

      if ( val & mask )
      {
        PSH_Hint  hint = &table->hints[idx];


        if ( !psh_hint_is_active( hint ) )
        {
          FT_UInt     count2;

#if 0
          PSH_Hint*  sort = table->sort;
          PSH_Hint   hint2;


          for ( count2 = count; count2 > 0; count2--, sort++ )
          {
            hint2 = sort[0];
            if ( psh_hint_overlap( hint, hint2 ) )
              FT_ERROR(( "psh_hint_table_activate_mask:"
                         " found overlapping hints\n" ))
          }
#else
          count2 = 0;
#endif

          if ( count2 == 0 )
          {
            psh_hint_activate( hint );
            if ( count < table->max_hints )
              table->sort[count++] = hint;
            else
              FT_ERROR(( "psh_hint_tableactivate_mask:"
                         " too many active hints\n" ));
          }
        }
      }

      mask >>= 1;
    }
    table->num_hints = count;

    /* now, sort the hints; they are guaranteed to not overlap */
    /* so we can compare their "org_pos" field directly        */
    {
      FT_Int     i1, i2;
      PSH_Hint   hint1, hint2;
      PSH_Hint*  sort = table->sort;


      /* a simple bubble sort will do, since in 99% of cases, the hints */
      /* will be already sorted -- and the sort will be linear          */
      for ( i1 = 1; i1 < (FT_Int)count; i1++ )
      {
        hint1 = sort[i1];
        for ( i2 = i1 - 1; i2 >= 0; i2-- )
        {
          hint2 = sort[i2];

          if ( hint2->org_pos < hint1->org_pos )
            break;

          sort[i2 + 1] = hint2;
          sort[i2]     = hint1;
        }
      }
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****               HINTS GRID-FITTING AND OPTIMIZATION             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#if 1
  static FT_Pos
  psh_dimension_quantize_len( PSH_Dimension  dim,
                              FT_Pos         len,
                              FT_Bool        do_snapping )
  {
    if ( len <= 64 )
      len = 64;
    else
    {
      FT_Pos  delta = len - dim->stdw.widths[0].cur;


      if ( delta < 0 )
        delta = -delta;

      if ( delta < 40 )
      {
        len = dim->stdw.widths[0].cur;
        if ( len < 48 )
          len = 48;
      }

      if ( len < 3 * 64 )
      {
        delta = ( len & 63 );
        len  &= -64;

        if ( delta < 10 )
          len += delta;

        else if ( delta < 32 )
          len += 10;

        else if ( delta < 54 )
          len += 54;

        else
          len += delta;
      }
      else
        len = FT_PIX_ROUND( len );
    }

    if ( do_snapping )
      len = FT_PIX_ROUND( len );

    return  len;
  }
#endif /* 0 */


#ifdef DEBUG_HINTER

  static void
  ps_simple_scale( PSH_Hint_Table  table,
                   FT_Fixed        scale,
                   FT_Fixed        delta,
                   FT_Int          dimension )
  {
    PSH_Hint  hint;
    FT_UInt   count;


    for ( count = 0; count < table->max_hints; count++ )
    {
      hint = table->hints + count;

      hint->cur_pos = FT_MulFix( hint->org_pos, scale ) + delta;
      hint->cur_len = FT_MulFix( hint->org_len, scale );

      if ( ps_debug_hint_func )
        ps_debug_hint_func( hint, dimension );
    }
  }

#endif /* DEBUG_HINTER */


  static FT_Fixed
  psh_hint_snap_stem_side_delta( FT_Fixed  pos,
                                 FT_Fixed  len )
  {
    FT_Fixed  delta1 = FT_PIX_ROUND( pos ) - pos;
    FT_Fixed  delta2 = FT_PIX_ROUND( pos + len ) - pos - len;


    if ( FT_ABS( delta1 ) <= FT_ABS( delta2 ) )
      return delta1;
    else
      return delta2;
  }


  static void
  psh_hint_align( PSH_Hint     hint,
                  PSH_Globals  globals,
                  FT_Int       dimension,
                  PSH_Glyph    glyph )
  {
    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( !psh_hint_is_fitted( hint ) )
    {
      FT_Pos  pos = FT_MulFix( hint->org_pos, scale ) + delta;
      FT_Pos  len = FT_MulFix( hint->org_len, scale );

      FT_Int            do_snapping;
      FT_Pos            fit_len;
      PSH_AlignmentRec  align;


      /* ignore stem alignments when requested through the hint flags */
      if ( ( dimension == 0 && !glyph->do_horz_hints ) ||
           ( dimension == 1 && !glyph->do_vert_hints ) )
      {
        hint->cur_pos = pos;
        hint->cur_len = len;

        psh_hint_set_fitted( hint );
        return;
      }

      /* perform stem snapping when requested - this is necessary
       * for monochrome and LCD hinting modes only
       */
      do_snapping = ( dimension == 0 && glyph->do_horz_snapping ) ||
                    ( dimension == 1 && glyph->do_vert_snapping );

      hint->cur_len = fit_len = len;

      /* check blue zones for horizontal stems */
      align.align     = PSH_BLUE_ALIGN_NONE;
      align.align_bot = align.align_top = 0;

      if ( dimension == 1 )
        psh_blues_snap_stem( &globals->blues,
                             hint->org_pos + hint->org_len,
                             hint->org_pos,
                             &align );

      switch ( align.align )
      {
      case PSH_BLUE_ALIGN_TOP:
        /* the top of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_top - fit_len;
        break;

      case PSH_BLUE_ALIGN_BOT:
        /* the bottom of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_bot;
        break;

      case PSH_BLUE_ALIGN_TOP | PSH_BLUE_ALIGN_BOT:
        /* both edges of the stem are aligned against blue zones */
        hint->cur_pos = align.align_bot;
        hint->cur_len = align.align_top - align.align_bot;
        break;

      default:
        {
          PSH_Hint  parent = hint->parent;


          if ( parent )
          {
            FT_Pos  par_org_center, par_cur_center;
            FT_Pos  cur_org_center, cur_delta;


            /* ensure that parent is already fitted */
            if ( !psh_hint_is_fitted( parent ) )
              psh_hint_align( parent, globals, dimension, glyph );

            /* keep original relation between hints, this is, use the */
            /* scaled distance between the centers of the hints to    */
            /* compute the new position                               */
            par_org_center = parent->org_pos + ( parent->org_len >> 1 );
            par_cur_center = parent->cur_pos + ( parent->cur_len >> 1 );
            cur_org_center = hint->org_pos   + ( hint->org_len   >> 1 );

            cur_delta = FT_MulFix( cur_org_center - par_org_center, scale );
            pos       = par_cur_center + cur_delta - ( len >> 1 );
          }

          hint->cur_pos = pos;
          hint->cur_len = fit_len;

          /* Stem adjustment tries to snap stem widths to standard
           * ones.  This is important to prevent unpleasant rounding
           * artefacts.
           */
          if ( glyph->do_stem_adjust )
          {
            if ( len <= 64 )
            {
              /* the stem is less than one pixel; we will center it
               * around the nearest pixel center
               */
#if 1
              pos = FT_PIX_FLOOR( pos + ( len >> 1 ) );
#else
             /* this seems to be a bug! */
              pos = pos + FT_PIX_FLOOR( len >> 1 );
#endif
              len = 64;
            }
            else
            {
              len = psh_dimension_quantize_len( dim, len, 0 );
            }
          }

          /* now that we have a good hinted stem width, try to position */
          /* the stem along a pixel grid integer coordinate             */
          hint->cur_pos = pos + psh_hint_snap_stem_side_delta( pos, len );
          hint->cur_len = len;
        }
      }

      if ( do_snapping )
      {
        pos = hint->cur_pos;
        len = hint->cur_len;

        if ( len < 64 )
          len = 64;
        else
          len = FT_PIX_ROUND( len );

        switch ( align.align )
        {
          case PSH_BLUE_ALIGN_TOP:
            hint->cur_pos = align.align_top - len;
            hint->cur_len = len;
            break;

          case PSH_BLUE_ALIGN_BOT:
            hint->cur_len = len;
            break;

          case PSH_BLUE_ALIGN_BOT | PSH_BLUE_ALIGN_TOP:
            /* don't touch */
            break;


          default:
            hint->cur_len = len;
            if ( len & 64 )
              pos = FT_PIX_FLOOR( pos + ( len >> 1 ) ) + 32;
            else
              pos = FT_PIX_ROUND( pos + ( len >> 1 ) );

            hint->cur_pos = pos - ( len >> 1 );
            hint->cur_len = len;
        }
      }

      psh_hint_set_fitted( hint );

#ifdef DEBUG_HINTER
      if ( ps_debug_hint_func )
        ps_debug_hint_func( hint, dimension );
#endif
    }
  }


#if 0  /* not used for now, experimental */

 /*
  *  A variant to perform "light" hinting (i.e. FT_RENDER_MODE_LIGHT)
  *  of stems
  */
  static void
  psh_hint_align_light( PSH_Hint     hint,
                        PSH_Globals  globals,
                        FT_Int       dimension,
                        PSH_Glyph    glyph )
  {
    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( !psh_hint_is_fitted( hint ) )
    {
      FT_Pos  pos = FT_MulFix( hint->org_pos, scale ) + delta;
      FT_Pos  len = FT_MulFix( hint->org_len, scale );

      FT_Pos  fit_len;

      PSH_AlignmentRec  align;


      /* ignore stem alignments when requested through the hint flags */
      if ( ( dimension == 0 && !glyph->do_horz_hints ) ||
           ( dimension == 1 && !glyph->do_vert_hints ) )
      {
        hint->cur_pos = pos;
        hint->cur_len = len;

        psh_hint_set_fitted( hint );
        return;
      }

      fit_len = len;

      hint->cur_len = fit_len;

      /* check blue zones for horizontal stems */
      align.align = PSH_BLUE_ALIGN_NONE;
      align.align_bot = align.align_top = 0;

      if ( dimension == 1 )
        psh_blues_snap_stem( &globals->blues,
                             hint->org_pos + hint->org_len,
                             hint->org_pos,
                             &align );

      switch ( align.align )
      {
      case PSH_BLUE_ALIGN_TOP:
        /* the top of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_top - fit_len;
        break;

      case PSH_BLUE_ALIGN_BOT:
        /* the bottom of the stem is aligned against a blue zone */
        hint->cur_pos = align.align_bot;
        break;

      case PSH_BLUE_ALIGN_TOP | PSH_BLUE_ALIGN_BOT:
        /* both edges of the stem are aligned against blue zones */
        hint->cur_pos = align.align_bot;
        hint->cur_len = align.align_top - align.align_bot;
        break;

      default:
        {
          PSH_Hint  parent = hint->parent;


          if ( parent )
          {
            FT_Pos  par_org_center, par_cur_center;
            FT_Pos  cur_org_center, cur_delta;


            /* ensure that parent is already fitted */
            if ( !psh_hint_is_fitted( parent ) )
              psh_hint_align_light( parent, globals, dimension, glyph );

            par_org_center = parent->org_pos + ( parent->org_len / 2 );
            par_cur_center = parent->cur_pos + ( parent->cur_len / 2 );
            cur_org_center = hint->org_pos   + ( hint->org_len   / 2 );

            cur_delta = FT_MulFix( cur_org_center - par_org_center, scale );
            pos       = par_cur_center + cur_delta - ( len >> 1 );
          }

          /* Stems less than one pixel wide are easy -- we want to
           * make them as dark as possible, so they must fall within
           * one pixel.  If the stem is split between two pixels
           * then snap the edge that is nearer to the pixel boundary
           * to the pixel boundary.
           */
          if ( len <= 64 )
          {
            if ( ( pos + len + 63 ) / 64  != pos / 64 + 1 )
              pos += psh_hint_snap_stem_side_delta ( pos, len );
          }

          /* Position stems other to minimize the amount of mid-grays.
           * There are, in general, two positions that do this,
           * illustrated as A) and B) below.
           *
           *   +                   +                   +                   +
           *
           * A)             |--------------------------------|
           * B)   |--------------------------------|
           * C)       |--------------------------------|
           *
           * Position A) (split the excess stem equally) should be better
           * for stems of width N + f where f < 0.5.
           *
           * Position B) (split the deficiency equally) should be better
           * for stems of width N + f where f > 0.5.
           *
           * It turns out though that minimizing the total number of lit
           * pixels is also important, so position C), with one edge
           * aligned with a pixel boundary is actually preferable
           * to A).  There are also more possibile positions for C) than
           * for A) or B), so it involves less distortion of the overall
           * character shape.
           */
          else /* len > 64 */
          {
            FT_Fixed  frac_len = len & 63;
            FT_Fixed  center = pos + ( len >> 1 );
            FT_Fixed  delta_a, delta_b;


            if ( ( len / 64 ) & 1 )
            {
              delta_a = FT_PIX_FLOOR( center ) + 32 - center;
              delta_b = FT_PIX_ROUND( center ) - center;
            }
            else
            {
              delta_a = FT_PIX_ROUND( center ) - center;
              delta_b = FT_PIX_FLOOR( center ) + 32 - center;
            }

            /* We choose between B) and C) above based on the amount
             * of fractinal stem width; for small amounts, choose
             * C) always, for large amounts, B) always, and inbetween,
             * pick whichever one involves less stem movement.
             */
            if ( frac_len < 32 )
            {
              pos += psh_hint_snap_stem_side_delta ( pos, len );
            }
            else if ( frac_len < 48 )
            {
              FT_Fixed  side_delta = psh_hint_snap_stem_side_delta ( pos,
                                                                     len );

              if ( FT_ABS( side_delta ) < FT_ABS( delta_b ) )
                pos += side_delta;
              else
                pos += delta_b;
            }
            else
            {
              pos += delta_b;
            }
          }

          hint->cur_pos = pos;
        }
      }  /* switch */

      psh_hint_set_fitted( hint );

#ifdef DEBUG_HINTER
      if ( ps_debug_hint_func )
        ps_debug_hint_func( hint, dimension );
#endif
    }
  }

#endif /* 0 */


  static void
  psh_hint_table_align_hints( PSH_Hint_Table  table,
                              PSH_Globals     globals,
                              FT_Int          dimension,
                              PSH_Glyph       glyph )
  {
    PSH_Hint       hint;
    FT_UInt        count;

#ifdef DEBUG_HINTER

    PSH_Dimension  dim   = &globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;
    FT_Fixed       delta = dim->scale_delta;


    if ( ps_debug_no_vert_hints && dimension == 0 )
    {
      ps_simple_scale( table, scale, delta, dimension );
      return;
    }

    if ( ps_debug_no_horz_hints && dimension == 1 )
    {
      ps_simple_scale( table, scale, delta, dimension );
      return;
    }

#endif /* DEBUG_HINTER*/

    hint  = table->hints;
    count = table->max_hints;

    for ( ; count > 0; count--, hint++ )
      psh_hint_align( hint, globals, dimension, glyph );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                POINTS INTERPOLATION ROUTINES                  *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#define PSH_ZONE_MIN  -3200000L
#define PSH_ZONE_MAX  +3200000L

#define xxDEBUG_ZONES


#ifdef DEBUG_ZONES

#include <stdio.h>

  static void
  psh_print_zone( PSH_Zone  zone )
  {
    printf( "zone [scale,delta,min,max] = [%.3f,%.3f,%d,%d]\n",
             zone->scale / 65536.0,
             zone->delta / 64.0,
             zone->min,
             zone->max );
  }

#else

#define psh_print_zone( x )  do { } while ( 0 )

#endif /* DEBUG_ZONES */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    HINTER GLYPH MANAGEMENT                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#ifdef COMPUTE_INFLEXS

  /* compute all inflex points in a given glyph */
  static void
  psh_glyph_compute_inflections( PSH_Glyph  glyph )
  {
    FT_UInt  n;


    for ( n = 0; n < glyph->num_contours; n++ )
    {
      PSH_Point  first, start, end, before, after;
      FT_Angle   angle_in, angle_seg, angle_out;
      FT_Angle   diff_in, diff_out;
      FT_Int     finished = 0;


      /* we need at least 4 points to create an inflection point */
      if ( glyph->contours[n].count < 4 )
        continue;

      /* compute first segment in contour */
      first = glyph->contours[n].start;

      start = end = first;
      do
      {
        end = end->next;
        if ( end == first )
          goto Skip;

      } while ( PSH_POINT_EQUAL_ORG( end, first ) );

      angle_seg = PSH_POINT_ANGLE( start, end );

      /* extend the segment start whenever possible */
      before = start;
      do
      {
        do
        {
          start  = before;
          before = before->prev;
          if ( before == first )
            goto Skip;

        } while ( PSH_POINT_EQUAL_ORG( before, start ) );

        angle_in = PSH_POINT_ANGLE( before, start );

      } while ( angle_in == angle_seg );

      first   = start;
      diff_in = FT_Angle_Diff( angle_in, angle_seg );

      /* now, process all segments in the contour */
      do
      {
        /* first, extend current segment's end whenever possible */
        after = end;
        do
        {
          do
          {
            end   = after;
            after = after->next;
            if ( after == first )
              finished = 1;

          } while ( PSH_POINT_EQUAL_ORG( end, after ) );

          angle_out = PSH_POINT_ANGLE( end, after );

        } while ( angle_out == angle_seg );

        diff_out = FT_Angle_Diff( angle_seg, angle_out );

        if ( ( diff_in ^ diff_out ) < 0 )
        {
          /* diff_in and diff_out have different signs, we have */
          /* inflection points here...                          */

          do
          {
            psh_point_set_inflex( start );
            start = start->next;
          }
          while ( start != end );

          psh_point_set_inflex( start );
        }

        start     = end;
        end       = after;
        angle_seg = angle_out;
        diff_in   = diff_out;

      } while ( !finished );

    Skip:
      ;
    }
  }

#endif /* COMPUTE_INFLEXS */


  static void
  psh_glyph_done( PSH_Glyph  glyph )
  {
    FT_Memory  memory = glyph->memory;


    psh_hint_table_done( &glyph->hint_tables[1], memory );
    psh_hint_table_done( &glyph->hint_tables[0], memory );

    FT_FREE( glyph->points );
    FT_FREE( glyph->contours );

    glyph->num_points   = 0;
    glyph->num_contours = 0;

    glyph->memory = 0;
  }


  static int
  psh_compute_dir( FT_Pos  dx,
                   FT_Pos  dy )
  {
    FT_Pos  ax, ay;
    int     result = PSH_DIR_NONE;


    ax = ( dx >= 0 ) ? dx : -dx;
    ay = ( dy >= 0 ) ? dy : -dy;

    if ( ay * 12 < ax )
    {
      /* |dy| <<< |dx|  means a near-horizontal segment */
      result = ( dx >= 0 ) ? PSH_DIR_RIGHT : PSH_DIR_LEFT;
    }
    else if ( ax * 12 < ay )
    {
      /* |dx| <<< |dy|  means a near-vertical segment */
      result = ( dy >= 0 ) ? PSH_DIR_UP : PSH_DIR_DOWN;
    }

    return result;
  }


  /* load outline point coordinates into hinter glyph */
  static void
  psh_glyph_load_points( PSH_Glyph  glyph,
                         FT_Int     dimension )
  {
    FT_Vector*  vec   = glyph->outline->points;
    PSH_Point   point = glyph->points;
    FT_UInt     count = glyph->num_points;


    for ( ; count > 0; count--, point++, vec++ )
    {
      point->flags2 = 0;
      point->hint   = NULL;
      if ( dimension == 0 )
      {
        point->org_u = vec->x;
        point->org_v = vec->y;
      }
      else
      {
        point->org_u = vec->y;
        point->org_v = vec->x;
      }

#ifdef DEBUG_HINTER
      point->org_x = vec->x;
      point->org_y = vec->y;
#endif

    }
  }


  /* save hinted point coordinates back to outline */
  static void
  psh_glyph_save_points( PSH_Glyph  glyph,
                         FT_Int     dimension )
  {
    FT_UInt     n;
    PSH_Point   point = glyph->points;
    FT_Vector*  vec   = glyph->outline->points;
    char*       tags  = glyph->outline->tags;


    for ( n = 0; n < glyph->num_points; n++ )
    {
      if ( dimension == 0 )
        vec[n].x = point->cur_u;
      else
        vec[n].y = point->cur_u;

      if ( psh_point_is_strong( point ) )
        tags[n] |= (char)( ( dimension == 0 ) ? 32 : 64 );

#ifdef DEBUG_HINTER

      if ( dimension == 0 )
      {
        point->cur_x   = point->cur_u;
        point->flags_x = point->flags2 | point->flags;
      }
      else
      {
        point->cur_y   = point->cur_u;
        point->flags_y = point->flags2 | point->flags;
      }

#endif

      point++;
    }
  }


  static FT_Error
  psh_glyph_init( PSH_Glyph    glyph,
                  FT_Outline*  outline,
                  PS_Hints     ps_hints,
                  PSH_Globals  globals )
  {
    FT_Error   error;
    FT_Memory  memory;


    /* clear all fields */
    FT_MEM_ZERO( glyph, sizeof ( *glyph ) );

    memory = glyph->memory = globals->memory;

    /* allocate and setup points + contours arrays */
    if ( FT_NEW_ARRAY( glyph->points,   outline->n_points   ) ||
         FT_NEW_ARRAY( glyph->contours, outline->n_contours ) )
      goto Exit;

    glyph->num_points   = outline->n_points;
    glyph->num_contours = outline->n_contours;

    {
      FT_UInt      first = 0, next, n;
      PSH_Point    points  = glyph->points;
      PSH_Contour  contour = glyph->contours;


      for ( n = 0; n < glyph->num_contours; n++ )
      {
        FT_Int     count;
        PSH_Point  point;


        next  = outline->contours[n] + 1;
        count = next - first;

        contour->start = points + first;
        contour->count = (FT_UInt)count;

        if ( count > 0 )
        {
          point = points + first;

          point->prev    = points + next - 1;
          point->contour = contour;

          for ( ; count > 1; count-- )
          {
            point[0].next = point + 1;
            point[1].prev = point;
            point++;
            point->contour = contour;
          }
          point->next = points + first;
        }

        contour++;
        first = next;
      }
    }

    {
      PSH_Point   points = glyph->points;
      PSH_Point   point  = points;
      FT_Vector*  vec    = outline->points;
      FT_UInt     n;


      for ( n = 0; n < glyph->num_points; n++, point++ )
      {
        FT_Int  n_prev = (FT_Int)( point->prev - points );
        FT_Int  n_next = (FT_Int)( point->next - points );
        FT_Pos  dxi, dyi, dxo, dyo;


        if ( !( outline->tags[n] & FT_CURVE_TAG_ON ) )
          point->flags = PSH_POINT_OFF;

        dxi = vec[n].x - vec[n_prev].x;
        dyi = vec[n].y - vec[n_prev].y;

        point->dir_in = (FT_Char)psh_compute_dir( dxi, dyi );

        dxo = vec[n_next].x - vec[n].x;
        dyo = vec[n_next].y - vec[n].y;

        point->dir_out = (FT_Char)psh_compute_dir( dxo, dyo );

        /* detect smooth points */
        if ( point->flags & PSH_POINT_OFF )
          point->flags |= PSH_POINT_SMOOTH;
        else if ( point->dir_in  != PSH_DIR_NONE ||
                  point->dir_out != PSH_DIR_NONE )
        {
          if ( point->dir_in == point->dir_out )
            point->flags |= PSH_POINT_SMOOTH;
        }
        else
        {
          FT_Angle  angle_in, angle_out, diff;


          angle_in  = FT_Atan2( dxi, dyi );
          angle_out = FT_Atan2( dxo, dyo );

          diff = angle_in - angle_out;
          if ( diff < 0 )
            diff = -diff;

          if ( diff > FT_ANGLE_PI )
            diff = FT_ANGLE_2PI - diff;

          if ( diff < FT_ANGLE_PI / 16 )
            point->flags |= PSH_POINT_SMOOTH;
        }
      }
    }

    glyph->outline = outline;
    glyph->globals = globals;

#ifdef COMPUTE_INFLEXS
    psh_glyph_load_points( glyph, 0 );
    psh_glyph_compute_inflections( glyph );
#endif /* COMPUTE_INFLEXS */

    /* now deal with hints tables */
    error = psh_hint_table_init( &glyph->hint_tables [0],
                                 &ps_hints->dimension[0].hints,
                                 &ps_hints->dimension[0].masks,
                                 &ps_hints->dimension[0].counters,
                                 memory );
    if ( error )
      goto Exit;

    error = psh_hint_table_init( &glyph->hint_tables [1],
                                 &ps_hints->dimension[1].hints,
                                 &ps_hints->dimension[1].masks,
                                 &ps_hints->dimension[1].counters,
                                 memory );
    if ( error )
      goto Exit;

  Exit:
    return error;
  }


  /* compute all extrema in a glyph for a given dimension */
  static void
  psh_glyph_compute_extrema( PSH_Glyph  glyph )
  {
    FT_UInt  n;


    /* first of all, compute all local extrema */
    for ( n = 0; n < glyph->num_contours; n++ )
    {
      PSH_Point  first = glyph->contours[n].start;
      PSH_Point  point, before, after;


      if ( glyph->contours[n].count == 0 )
        continue;

      point  = first;
      before = point;
      after  = point;

      do
      {
        before = before->prev;
        if ( before == first )
          goto Skip;

      } while ( before->org_u == point->org_u );

      first = point = before->next;

      for (;;)
      {
        after = point;
        do
        {
          after = after->next;
          if ( after == first )
            goto Next;

        } while ( after->org_u == point->org_u );

        if ( before->org_u < point->org_u )
        {
          if ( after->org_u < point->org_u )
          {
            /* local maximum */
            goto Extremum;
          }
        }
        else /* before->org_u > point->org_u */
        {
          if ( after->org_u > point->org_u )
          {
            /* local minimum */
          Extremum:
            do
            {
              psh_point_set_extremum( point );
              point = point->next;

            } while ( point != after );
          }
        }

        before = after->prev;
        point  = after;

      } /* for  */

    Next:
      ;
    }

    /* for each extremum, determine its direction along the */
    /* orthogonal axis                                      */
    for ( n = 0; n < glyph->num_points; n++ )
    {
      PSH_Point  point, before, after;


      point  = &glyph->points[n];
      before = point;
      after  = point;

      if ( psh_point_is_extremum( point ) )
      {
        do
        {
          before = before->prev;
          if ( before == point )
            goto Skip;

        } while ( before->org_v == point->org_v );

        do
        {
          after = after->next;
          if ( after == point )
            goto Skip;

        } while ( after->org_v == point->org_v );
      }

      if ( before->org_v < point->org_v &&
           after->org_v  > point->org_v )
      {
        psh_point_set_positive( point );
      }
      else if ( before->org_v > point->org_v &&
                after->org_v  < point->org_v )
      {
        psh_point_set_negative( point );
      }

    Skip:
      ;
    }
  }


  /* major_dir is the direction for points on the bottom/left of the stem; */
  /* Points on the top/right of the stem will have a direction of          */
  /* -major_dir.                                                           */

  static void
  psh_hint_table_find_strong_point( PSH_Hint_Table  table,
                                    PSH_Point       point,
                                    FT_Int          threshold,
                                    FT_Int          major_dir )
  {
    PSH_Hint*  sort      = table->sort;
    FT_UInt    num_hints = table->num_hints;
    FT_Int     point_dir = 0;


    if ( PSH_DIR_COMPARE( point->dir_in, major_dir ) )
      point_dir = point->dir_in;

    else if ( PSH_DIR_COMPARE( point->dir_out, major_dir ) )
      point_dir = point->dir_out;

    if ( point_dir )
    {
      FT_UInt  flag;


      for ( ; num_hints > 0; num_hints--, sort++ )
      {
        PSH_Hint  hint = sort[0];
        FT_Pos    d;


        if ( point_dir == major_dir )
        {
          flag = PSH_POINT_EDGE_MIN;
          d    = point->org_u - hint->org_pos;

          if ( FT_ABS( d ) < threshold )
          {
          Is_Strong:
            psh_point_set_strong( point );
            point->flags2 |= flag;
            point->hint    = hint;
            break;
          }
        }
        else if ( point_dir == -major_dir )
        {
          flag = PSH_POINT_EDGE_MAX;
          d    = point->org_u - hint->org_pos - hint->org_len;

          if ( FT_ABS( d ) < threshold )
            goto Is_Strong;
        }
      }
    }

#if 1
    else if ( psh_point_is_extremum( point ) )
    {
      /* treat extrema as special cases for stem edge alignment */
      FT_UInt  min_flag, max_flag;


      if ( major_dir == PSH_DIR_HORIZONTAL )
      {
        min_flag = PSH_POINT_POSITIVE;
        max_flag = PSH_POINT_NEGATIVE;
      }
      else
      {
        min_flag = PSH_POINT_NEGATIVE;
        max_flag = PSH_POINT_POSITIVE;
      }

      for ( ; num_hints > 0; num_hints--, sort++ )
      {
        PSH_Hint  hint = sort[0];
        FT_Pos    d;
        FT_Int    flag;


        if ( point->flags2 & min_flag )
        {
          flag = PSH_POINT_EDGE_MIN;
          d    = point->org_u - hint->org_pos;

          if ( FT_ABS( d ) < threshold )
          {
          Is_Strong2:
            point->flags2 |= flag;
            point->hint    = hint;
            psh_point_set_strong( point );
            break;
          }
        }
        else if ( point->flags2 & max_flag )
        {
          flag = PSH_POINT_EDGE_MAX;
          d    = point->org_u - hint->org_pos - hint->org_len;

          if ( FT_ABS( d ) < threshold )
            goto Is_Strong2;
        }

        if ( point->org_u >= hint->org_pos                 &&
             point->org_u <= hint->org_pos + hint->org_len )
        {
          point->hint = hint;
        }
      }
    }

#endif /* 1 */
  }


  /* the accepted shift for strong points in fractional pixels */
#define PSH_STRONG_THRESHOLD  32

  /* the maximum shift value in font units */
#define PSH_STRONG_THRESHOLD_MAXIMUM  30


  /* find strong points in a glyph */
  static void
  psh_glyph_find_strong_points( PSH_Glyph  glyph,
                                FT_Int     dimension )
  {
    /* a point is `strong' if it is located on a stem edge and       */
    /* has an `in' or `out' tangent parallel to the hint's direction */

    PSH_Hint_Table  table     = &glyph->hint_tables[dimension];
    PS_Mask         mask      = table->hint_masks->masks;
    FT_UInt         num_masks = table->hint_masks->num_masks;
    FT_UInt         first     = 0;
    FT_Int          major_dir = dimension == 0 ? PSH_DIR_VERTICAL
                                               : PSH_DIR_HORIZONTAL;
    PSH_Dimension   dim       = &glyph->globals->dimension[dimension];
    FT_Fixed        scale     = dim->scale_mult;
    FT_Int          threshold;


    threshold = (FT_Int)FT_DivFix( PSH_STRONG_THRESHOLD, scale );
    if ( threshold > PSH_STRONG_THRESHOLD_MAXIMUM )
      threshold = PSH_STRONG_THRESHOLD_MAXIMUM;

    /* process secondary hints to `selected' points */
    if ( num_masks > 1 && glyph->num_points > 0 )
    {
      first = mask->end_point;
      mask++;
      for ( ; num_masks > 1; num_masks--, mask++ )
      {
        FT_UInt  next;
        FT_Int   count;


        next  = mask->end_point;
        count = next - first;
        if ( count > 0 )
        {
          PSH_Point  point = glyph->points + first;


          psh_hint_table_activate_mask( table, mask );

          for ( ; count > 0; count--, point++ )
            psh_hint_table_find_strong_point( table, point,
                                              threshold, major_dir );
        }
        first = next;
      }
    }

    /* process primary hints for all points */
    if ( num_masks == 1 )
    {
      FT_UInt    count = glyph->num_points;
      PSH_Point  point = glyph->points;


      psh_hint_table_activate_mask( table, table->hint_masks->masks );
      for ( ; count > 0; count--, point++ )
      {
        if ( !psh_point_is_strong( point ) )
          psh_hint_table_find_strong_point( table, point,
                                            threshold, major_dir );
      }
    }

    /* now, certain points may have been attached to a hint and */
    /* not marked as strong; update their flags then            */
    {
      FT_UInt    count = glyph->num_points;
      PSH_Point  point = glyph->points;


      for ( ; count > 0; count--, point++ )
        if ( point->hint && !psh_point_is_strong( point ) )
          psh_point_set_strong( point );
    }
  }


  /* find points in a glyph which are in a blue zone and have `in' or */
  /* `out' tangents parallel to the horizontal axis                   */
  static void
  psh_glyph_find_blue_points( PSH_Blues  blues,
                              PSH_Glyph  glyph )
  {
    PSH_Blue_Table  table;
    PSH_Blue_Zone   zone;
    FT_UInt         glyph_count = glyph->num_points;
    FT_UInt         blue_count;
    PSH_Point       point = glyph->points;


    for ( ; glyph_count > 0; glyph_count--, point++ )
    {
      FT_Pos  y;


      /* check tangents */
      if ( !PSH_DIR_COMPARE( point->dir_in,  PSH_DIR_HORIZONTAL ) &&
           !PSH_DIR_COMPARE( point->dir_out, PSH_DIR_HORIZONTAL ) )
        continue;

      /* skip strong points */
      if ( psh_point_is_strong( point ) )
        continue;

      y = point->org_u;

      /* look up top zones */
      table      = &blues->normal_top;
      blue_count = table->count;
      zone       = table->zones;

      for ( ; blue_count > 0; blue_count--, zone++ )
      {
        FT_Pos  delta = y - zone->org_bottom;


        if ( delta < -blues->blue_fuzz )
          break;

        if ( y <= zone->org_top + blues->blue_fuzz )
          if ( blues->no_overshoots || delta <= blues->blue_threshold )
          {
            point->cur_u = zone->cur_bottom;
            psh_point_set_strong( point );
            psh_point_set_fitted( point );
          }
      }

      /* look up bottom zones */
      table      = &blues->normal_bottom;
      blue_count = table->count;
      zone       = table->zones + blue_count - 1;

      for ( ; blue_count > 0; blue_count--, zone-- )
      {
        FT_Pos  delta = zone->org_top - y;


        if ( delta < -blues->blue_fuzz )
          break;

        if ( y >= zone->org_bottom - blues->blue_fuzz )
          if ( blues->no_overshoots || delta < blues->blue_threshold )
          {
            point->cur_u = zone->cur_top;
            psh_point_set_strong( point );
            psh_point_set_fitted( point );
          }
      }
    }
  }


  /* interpolate strong points with the help of hinted coordinates */
  static void
  psh_glyph_interpolate_strong_points( PSH_Glyph  glyph,
                                       FT_Int     dimension )
  {
    PSH_Dimension  dim   = &glyph->globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;

    FT_UInt        count = glyph->num_points;
    PSH_Point      point = glyph->points;


    for ( ; count > 0; count--, point++ )
    {
      PSH_Hint  hint = point->hint;


      if ( hint )
      {
        FT_Pos  delta;


        if ( psh_point_is_edge_min( point ) )
          point->cur_u = hint->cur_pos;

        else if ( psh_point_is_edge_max( point ) )
          point->cur_u = hint->cur_pos + hint->cur_len;

        else
        {
          delta = point->org_u - hint->org_pos;

          if ( delta <= 0 )
            point->cur_u = hint->cur_pos + FT_MulFix( delta, scale );

          else if ( delta >= hint->org_len )
            point->cur_u = hint->cur_pos + hint->cur_len +
                             FT_MulFix( delta - hint->org_len, scale );

          else if ( hint->org_len > 0 )
            point->cur_u = hint->cur_pos +
                             FT_MulDiv( delta, hint->cur_len,
                                        hint->org_len );
          else
            point->cur_u = hint->cur_pos;
        }
        psh_point_set_fitted( point );
      }
    }
  }


  static void
  psh_glyph_interpolate_normal_points( PSH_Glyph  glyph,
                                       FT_Int     dimension )
  {

#if 1
    /* first technique: a point is strong if it is a local extremum */

    PSH_Dimension  dim   = &glyph->globals->dimension[dimension];
    FT_Fixed       scale = dim->scale_mult;

    FT_UInt        count = glyph->num_points;
    PSH_Point      point = glyph->points;


    for ( ; count > 0; count--, point++ )
    {
      if ( psh_point_is_strong( point ) )
        continue;

      /* sometimes, some local extrema are smooth points */
      if ( psh_point_is_smooth( point ) )
      {
        if ( point->dir_in == PSH_DIR_NONE   ||
             point->dir_in != point->dir_out )
          continue;

        if ( !psh_point_is_extremum( point ) &&
             !psh_point_is_inflex( point )   )
          continue;

        point->flags &= ~PSH_POINT_SMOOTH;
      }

      /* find best enclosing point coordinates */
      {
        PSH_Point  before = 0;
        PSH_Point  after  = 0;

        FT_Pos     diff_before = -32000;
        FT_Pos     diff_after  =  32000;
        FT_Pos     u = point->org_u;

        FT_Int     count2 = glyph->num_points;
        PSH_Point  cur    = glyph->points;


        for ( ; count2 > 0; count2--, cur++ )
        {
          if ( psh_point_is_strong( cur ) )
          {
            FT_Pos  diff = cur->org_u - u;


            if ( diff <= 0 )
            {
              if ( diff > diff_before )
              {
                diff_before = diff;
                before      = cur;
              }
            }

            else if ( diff >= 0 )
            {
              if ( diff < diff_after )
              {
                diff_after = diff;
                after      = cur;
              }
            }
          }
        }

        if ( !before )
        {
          if ( !after )
            continue;

          /* we are before the first strong point coordinate; */
          /* simply translate the point                       */
          point->cur_u = after->cur_u +
                           FT_MulFix( point->org_u - after->org_u, scale );
        }
        else if ( !after )
        {
          /* we are after the last strong point coordinate; */
          /* simply translate the point                     */
          point->cur_u = before->cur_u +
                           FT_MulFix( point->org_u - before->org_u, scale );
        }
        else
        {
          if ( diff_before == 0 )
            point->cur_u = before->cur_u;

          else if ( diff_after == 0 )
            point->cur_u = after->cur_u;

          else
            point->cur_u = before->cur_u +
                             FT_MulDiv( u - before->org_u,
                                        after->cur_u - before->cur_u,
                                        after->org_u - before->org_u );
        }

        psh_point_set_fitted( point );
      }
    }

#endif /* 1 */

  }


  /* interpolate other points */
  static void
  psh_glyph_interpolate_other_points( PSH_Glyph  glyph,
                                      FT_Int     dimension )
  {
    PSH_Dimension  dim          = &glyph->globals->dimension[dimension];
    FT_Fixed       scale        = dim->scale_mult;
    FT_Fixed       delta        = dim->scale_delta;
    PSH_Contour    contour      = glyph->contours;
    FT_UInt        num_contours = glyph->num_contours;


    for ( ; num_contours > 0; num_contours--, contour++ )
    {
      PSH_Point  start = contour->start;
      PSH_Point  first, next, point;
      FT_UInt    fit_count;


      /* count the number of strong points in this contour */
      next      = start + contour->count;
      fit_count = 0;
      first     = 0;

      for ( point = start; point < next; point++ )
        if ( psh_point_is_fitted( point ) )
        {
          if ( !first )
            first = point;

          fit_count++;
        }

      /* if there are less than 2 fitted points in the contour, we */
      /* simply scale and eventually translate the contour points  */
      if ( fit_count < 2 )
      {
        if ( fit_count == 1 )
          delta = first->cur_u - FT_MulFix( first->org_u, scale );

        for ( point = start; point < next; point++ )
          if ( point != first )
            point->cur_u = FT_MulFix( point->org_u, scale ) + delta;

        goto Next_Contour;
      }

      /* there are more than 2 strong points in this contour; we */
      /* need to interpolate weak points between them            */
      start = first;
      do
      {
        point = first;

        /* skip consecutive fitted points */
        for (;;)
        {
          next = first->next;
          if ( next == start )
            goto Next_Contour;

          if ( !psh_point_is_fitted( next ) )
            break;

          first = next;
        }

        /* find next fitted point after unfitted one */
        for (;;)
        {
          next = next->next;
          if ( psh_point_is_fitted( next ) )
            break;
        }

        /* now interpolate between them */
        {
          FT_Pos    org_a, org_ab, cur_a, cur_ab;
          FT_Pos    org_c, org_ac, cur_c;
          FT_Fixed  scale_ab;


          if ( first->org_u <= next->org_u )
          {
            org_a  = first->org_u;
            cur_a  = first->cur_u;
            org_ab = next->org_u - org_a;
            cur_ab = next->cur_u - cur_a;
          }
          else
          {
            org_a  = next->org_u;
            cur_a  = next->cur_u;
            org_ab = first->org_u - org_a;
            cur_ab = first->cur_u - cur_a;
          }

          scale_ab = 0x10000L;
          if ( org_ab > 0 )
            scale_ab = FT_DivFix( cur_ab, org_ab );

          point = first->next;
          do
          {
            org_c  = point->org_u;
            org_ac = org_c - org_a;

            if ( org_ac <= 0 )
            {
              /* on the left of the interpolation zone */
              cur_c = cur_a + FT_MulFix( org_ac, scale );
            }
            else if ( org_ac >= org_ab )
            {
              /* on the right on the interpolation zone */
              cur_c = cur_a + cur_ab + FT_MulFix( org_ac - org_ab, scale );
            }
            else
            {
              /* within the interpolation zone */
              cur_c = cur_a + FT_MulFix( org_ac, scale_ab );
            }

            point->cur_u = cur_c;

            point = point->next;

          } while ( point != next );
        }

        /* keep going until all points in the contours have been processed */
        first = next;

      } while ( first != start );

    Next_Contour:
      ;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     HIGH-LEVEL INTERFACE                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_Error
  ps_hints_apply( PS_Hints        ps_hints,
                  FT_Outline*     outline,
                  PSH_Globals     globals,
                  FT_Render_Mode  hint_mode )
  {
    PSH_GlyphRec  glyphrec;
    PSH_Glyph     glyph = &glyphrec;
    FT_Error      error;
#ifdef DEBUG_HINTER
    FT_Memory     memory;
#endif
    FT_Int        dimension;


    /* something to do? */
    if ( outline->n_points == 0 || outline->n_contours == 0 )
      return PSH_Err_Ok;

#ifdef DEBUG_HINTER

    memory = globals->memory;

    if ( ps_debug_glyph )
    {
      psh_glyph_done( ps_debug_glyph );
      FT_FREE( ps_debug_glyph );
    }

    if ( FT_NEW( glyph ) )
      return error;

    ps_debug_glyph = glyph;

#endif /* DEBUG_HINTER */

    error = psh_glyph_init( glyph, outline, ps_hints, globals );
    if ( error )
      goto Exit;

    /* try to optimize the y_scale so that the top of non-capital letters
     * is aligned on a pixel boundary whenever possible
     */
    {
      PSH_Dimension  dim_x = &glyph->globals->dimension[0];
      PSH_Dimension  dim_y = &glyph->globals->dimension[1];

      FT_Fixed x_scale = dim_x->scale_mult;
      FT_Fixed y_scale = dim_y->scale_mult;

      FT_Fixed scaled;
      FT_Fixed fitted;


      scaled = FT_MulFix( globals->blues.normal_top.zones->org_ref, y_scale );
      fitted = FT_PIX_ROUND( scaled );

      if ( fitted != 0 && scaled != fitted )
      {
        y_scale = FT_MulDiv( y_scale, fitted, scaled );

        if ( fitted < scaled )
          x_scale -= x_scale / 50;

        psh_globals_set_scale( glyph->globals, x_scale, y_scale, 0, 0 );
      }
    }

    glyph->do_horz_hints = 1;
    glyph->do_vert_hints = 1;

    glyph->do_horz_snapping = FT_BOOL( hint_mode == FT_RENDER_MODE_MONO ||
                                       hint_mode == FT_RENDER_MODE_LCD  );

    glyph->do_vert_snapping = FT_BOOL( hint_mode == FT_RENDER_MODE_MONO  ||
                                       hint_mode == FT_RENDER_MODE_LCD_V );

    glyph->do_stem_adjust   = FT_BOOL( hint_mode != FT_RENDER_MODE_LIGHT );

    for ( dimension = 0; dimension < 2; dimension++ )
    {
      /* load outline coordinates into glyph */
      psh_glyph_load_points( glyph, dimension );

      /* compute local extrema */
      psh_glyph_compute_extrema( glyph );

      /* compute aligned stem/hints positions */
      psh_hint_table_align_hints( &glyph->hint_tables[dimension],
                                  glyph->globals,
                                  dimension,
                                  glyph );

      /* find strong points, align them, then interpolate others */
      psh_glyph_find_strong_points( glyph, dimension );
      if ( dimension == 1 )
        psh_glyph_find_blue_points( &globals->blues, glyph );
      psh_glyph_interpolate_strong_points( glyph, dimension );
      psh_glyph_interpolate_normal_points( glyph, dimension );
      psh_glyph_interpolate_other_points( glyph, dimension );

      /* save hinted coordinates back to outline */
      psh_glyph_save_points( glyph, dimension );
    }

  Exit:

#ifndef DEBUG_HINTER
    psh_glyph_done( glyph );
#endif

    return error;
  }


/* END */
